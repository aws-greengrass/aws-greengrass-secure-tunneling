
#include "tunnel.h"
#include "secure-tunnel.h"
#include "tunnel_notification_parser.h"
#include <fcntl.h>
#include <gg/buffer.h>
#include <gg/cleanup.h>
#include <gg/error.h>
#include <gg/file.h>
#include <gg/log.h>
#include <gg/vector.h>
#include <pthread.h>
#include <signal.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#define LOCALPROXY_LOG_LEVEL "2" // 2=warnings/errors, 4=debug

static pthread_mutex_t tunnel_mutex = PTHREAD_MUTEX_INITIALIZER;
static int active_tunnels = 0;
static TunnelCreationContext tunnel_contexts[20];
static uint32_t tunnel_slots_mask = 0;
static const SecureTunnelConfig *tunnel_config = NULL;

static void cleanup_tunnel_slot(TunnelCreationContext **ctx) {
    if (*ctx != NULL) {
        GG_MTX_SCOPE_GUARD(&tunnel_mutex);
        active_tunnels--;
        int slot = (int) (*ctx - tunnel_contexts);
        tunnel_slots_mask &= ~(1U << slot);
        GG_LOGI("Tunnel closed (active tunnels: %d)", active_tunnels);
        *ctx = NULL;
    }
}

static int prepare_localproxy_fd(void) {
    char localproxy_path[512];
    GgByteVec path_vec = GG_BYTE_VEC(localproxy_path);
    GgError ret = gg_byte_vec_append(&path_vec, tunnel_config->artifact_path);
    gg_byte_vec_chain_append(&ret, &path_vec, GG_STR("/localproxy"));
    if (ret != GG_ERR_OK) {
        GG_LOGE("Failed to build localproxy path");
        return -1;
    }
    localproxy_path[path_vec.buf.len] = '\0';

    int fd = open(localproxy_path, O_RDONLY);
    if (fd == -1) {
        GG_LOGE("Localproxy not found in artifact directory");
        return -1;
    }

    if (faccessat(AT_FDCWD, "/proc/self/fd", X_OK, 0) != 0) {
        GG_LOGE(
            "Cannot access localproxy binary - execute permission check failed"
        );
        close(fd);
        return -1;
    }

    return fd;
}

static void execute_localproxy(
    int localproxy_fd, const char *const *args, const char *access_token
) {
    // Fork and execute localproxy
    pid_t pid = fork();
    if (pid == 0) {
        // Child process: kill localproxy if parent dies
        prctl(PR_SET_PDEATHSIG, SIGTERM);

        // Check if parent died between fork and prctl
        if (getppid() == 1) {
            _exit(1);
        }

        // Set access token via environment variable
        // NOLINTNEXTLINE(concurrency-mt-unsafe)
        setenv("AWSIOT_TUNNEL_ACCESS_TOKEN", access_token, 1);

        // Execute localproxy
        fexecve(localproxy_fd, (char *const *) args, environ);

        GG_LOGE("Failed to exec localproxy");
        _exit(1);

    } else if (pid > 0) {
        // Parent process: wait for completion
        int status;
        waitpid(pid, &status, 0);

        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            GG_LOGI("Tunnel completed successfully");
        } else {
            GG_LOGW("Tunnel exited with status: %d", status);
        }
    } else {
        GG_LOGE("Failed to fork process");
    }
}

static void *tunnel_worker(void *arg) {
    TunnelCreationContext *ctx = (TunnelCreationContext *) arg;
    GG_CLEANUP(cleanup_tunnel_slot, ctx);

    GG_LOGI("Starting tunnel for service: %s", ctx->service);

    if (tunnel_config->artifact_path.len == 0) {
        return NULL;
    }

    int localproxy_fd = prepare_localproxy_fd();
    if (localproxy_fd == -1) {
        return NULL;
    }
    GG_CLEANUP(cleanup_close, localproxy_fd);

    char dest_addr[32];
    int written
        = snprintf(dest_addr, sizeof(dest_addr), "localhost:%u", ctx->port);
    if (written < 0 || (size_t) written >= sizeof(dest_addr)) {
        GG_LOGE("Failed to format destination address");
        return NULL;
    }

    // Prepare localproxy arguments (without access token)
    const char *args[] = { "localproxy", "-r",      ctx->region,
                           "-d",         dest_addr, "--destination-client-type",
                           "V1",         "-v",      LOCALPROXY_LOG_LEVEL,
                           NULL };

    GG_LOGI(
        "Using localproxy for service: %s on port %u", ctx->service, ctx->port
    );

    execute_localproxy(localproxy_fd, args, ctx->access_token);

    return NULL;
}

GgError handle_tunnel_notification(
    GgMap notification, const SecureTunnelConfig *config
) {
    if (tunnel_config == NULL) {
        tunnel_config = config;
    }

    TunnelCreationContext request = { 0 };

    GgError ret = parse_and_validate_notification(notification, &request);
    if (ret != GG_ERR_OK) {
        return ret;
    }

    {
        GG_MTX_SCOPE_GUARD(&tunnel_mutex);
        if (active_tunnels >= config->max_concurrent_tunnels) {
            GG_LOGE(
                "Maximum concurrent tunnels reached (%d)",
                config->max_concurrent_tunnels
            );
            return GG_ERR_NOMEM;
        }

        // Find free slot using bitmask
        uint32_t free_mask = ~tunnel_slots_mask;
        if ((free_mask & 0xFFFFF)
            == 0) { // Check if any of first 20 bits are free
            GG_LOGE("No available tunnel slots");
            return GG_ERR_NOMEM;
        }
        int slot = __builtin_ctz(free_mask);
        tunnel_slots_mask |= (1U << slot); // Mark slot as occupied

        // Store tunnel request in allocated slot
        tunnel_contexts[slot] = request;

        pthread_t thread;
        if (pthread_create(&thread, NULL, tunnel_worker, &tunnel_contexts[slot])
            != 0) {
            tunnel_slots_mask
                &= ~(1U << slot); // Free slot on thread creation failure
            GG_LOGE("Failed to create tunnel worker thread");
            return GG_ERR_FAILURE;
        }

        pthread_detach(thread);
        active_tunnels++;
        GG_LOGI(
            "Started tunnel worker for service: %s (active tunnels: %d)",
            tunnel_contexts[slot].service,
            active_tunnels
        );
    }

    return GG_ERR_OK;
}

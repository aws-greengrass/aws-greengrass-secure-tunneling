
#include "tunnel.h"
#include "secure-tunnel.h"
#include "tunnel_notification_parser.h"
#include <errno.h>
#include <fcntl.h>
#include <gg/buffer.h>
#include <gg/cleanup.h>
#include <gg/error.h>
#include <gg/file.h>
#include <gg/log.h>
#include <gg/vector.h>
#include <pthread.h>
#include <signal.h>
#include <sys/epoll.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
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

static void wait_for_child(
    int pidfd, int epoll_fd, pid_t pid, int timeout_seconds
) {
    struct timespec start;
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &start);
    int timeout_ms = timeout_seconds * 1000;
    int ret;
    struct epoll_event event;

    do {
        ret = epoll_wait(epoll_fd, &event, 1, timeout_ms);
        if (ret < 0 && errno == EINTR) {
            clock_gettime(CLOCK_MONOTONIC, &now);
            int elapsed_ms
                = (int) (((now.tv_sec - start.tv_sec) * 1000)
                         + ((now.tv_nsec - start.tv_nsec) / 1000000));
            timeout_ms = (timeout_seconds * 1000) - elapsed_ms;
            if (timeout_ms <= 0) {
                ret = 0;
                break;
            }
        }
    } while (ret < 0 && errno == EINTR);

    close(epoll_fd);
    int status;

    if (ret < 0) {
        GG_LOGE("epoll_wait failed, errno: %d", errno);
        // Kill entire process group to clean up localproxy and any children.
        killpg(pid, SIGKILL);
    } else if (ret == 0) {
        GG_LOGW(
            "Tunnel timeout (%d seconds) reached, killing localproxy",
            timeout_seconds
        );
        // Kill entire process group to clean up localproxy and any children.
        killpg(pid, SIGKILL);
    } else {
        close(pidfd);
        waitpid(pid, &status, 0);
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            GG_LOGI("Tunnel completed successfully");
        } else {
            GG_LOGW("Tunnel exited with status: %d", status);
        }
        return;
    }

    close(pidfd);
    waitpid(pid, &status, 0);
}

static void execute_localproxy(
    int localproxy_fd,
    const char *const *args,
    const char *access_token,
    int timeout_seconds
) {
    // Fork and execute localproxy
    pid_t pid = fork();
    if (pid == 0) {
        // Child process block

        // Create new process group so we can kill all children
        setpgid(0, 0);

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
        // Parent process block

        // Get a file descriptor for the child process to use with epoll.
        int pidfd = (int) syscall(SYS_pidfd_open, pid, 0);
        int status = 0;

        if (pidfd == -1) {
            GG_LOGE("pidfd_open failed");
            // Use killpg to signal the entire process group, ensuring any
            // grandchildren spawned by localproxy are also killed.
            killpg(pid, SIGKILL);
            // Reap the child to prevent zombie process.
            waitpid(pid, &status, 0);
            return;
        }

        int epoll_fd = epoll_create1(EPOLL_CLOEXEC);
        if (epoll_fd == -1) {
            GG_LOGE("epoll_create1 failed");
            // Kill entire process group.
            killpg(pid, SIGKILL);
            close(pidfd);
            // Reap the child to prevent zombie process.
            waitpid(pid, &status, 0);
            return;
        }

        struct epoll_event ev = { .events = EPOLLIN, .data.fd = pidfd };
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, pidfd, &ev);
        wait_for_child(pidfd, epoll_fd, pid, timeout_seconds);
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

    execute_localproxy(
        localproxy_fd,
        args,
        ctx->access_token,
        tunnel_config->tunnel_timeout_seconds
    );

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

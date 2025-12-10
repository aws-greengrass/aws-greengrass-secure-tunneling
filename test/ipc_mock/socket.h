#ifndef GG_MOCK_SOCKET_H
#define GG_MOCK_SOCKET_H

#include <gg/error.h>

GgError configure_client_timeout(int socket_fd, int client_timeout) {
    if (client_timeout < 0) {
        client_timeout = 5;
    }

    // To prevent deadlocking on hanged client, add a timeout
    struct timeval timeout = { .tv_sec = client_timeout };
    int sys_ret = setsockopt(
        socket_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)
    );
    if (sys_ret == -1) {
        GG_LOGE("Failed to set receive timeout on socket: %d.", errno);
        return GG_ERR_FATAL;
    }
    sys_ret = setsockopt(
        socket_fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)
    );
    if (sys_ret == -1) {
        GG_LOGE("Failed to set send timeout on socket: %d.", errno);
        return GG_ERR_FATAL;
    }
    return GG_ERR_OK;
}

static GgError configure_server_socket(
    int socket_fd, GgBuffer path, mode_t mode
) {
    assert(socket_fd >= 0);

    struct sockaddr_un addr = { .sun_family = AF_UNIX, .sun_path = { 0 } };

    // TODO: Handle long paths by creating socket in temp dir and moving
    if (path.len >= sizeof(addr.sun_path)) {
        GG_LOGE(
            "Socket path too long (len %zu, max %zu).",
            path.len,
            sizeof(addr.sun_path) - 1
        );
        return GG_ERR_FAILURE;
    }

    memcpy(addr.sun_path, path.data, path.len);

    if ((unlink(addr.sun_path) == -1) && (errno != ENOENT)) {
        GG_LOGE("Failed to unlink server socket: %d.", errno);
        return GG_ERR_FAILURE;
    }

    if (bind(socket_fd, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
        GG_LOGE("Failed to bind server socket: %d.", errno);
        return GG_ERR_FAILURE;
    }

    if (chmod(addr.sun_path, mode) == -1) {
        GG_LOGE("Failed to chmod server socket: %d.", errno);
        return GG_ERR_FAILURE;
    }

    // To prevent deadlocking on hanged client, add a timeout
    struct timeval timeout = { .tv_sec = 5 };
    int sys_ret = setsockopt(
        socket_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)
    );
    if (sys_ret == -1) {
        GG_LOGE("Failed to set receive timeout on socket: %d.", errno);
        return GG_ERR_FATAL;
    }
    sys_ret = setsockopt(
        socket_fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)
    );
    if (sys_ret == -1) {
        GG_LOGE("Failed to set send timeout on socket: %d.", errno);
        return GG_ERR_FATAL;
    }

    // Client must only attempt to connect once during tests
    static const int MAX_SOCKET_BACKLOG = 1;
    if (listen(socket_fd, MAX_SOCKET_BACKLOG) == -1) {
        GG_LOGE("Failed to listen on server socket: %d.", errno);
        return GG_ERR_FAILURE;
    }

    return GG_ERR_OK;
}

static GgError gg_socket_open(GgBuffer path, mode_t mode, int *socket_fd) {
    assert(socket_fd != NULL);
    int server_fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (server_fd == -1) {
        GG_LOGE("Failed to create socket: %d.", errno);
        return GG_ERR_FAILURE;
    }

    GgError ret = configure_server_socket(server_fd, path, mode);
    if (ret != GG_ERR_OK) {
        cleanup_close(&server_fd);
        return ret;
    }
    *socket_fd = server_fd;
    return ret;
}

#endif

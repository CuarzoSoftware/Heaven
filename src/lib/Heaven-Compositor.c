#include "Heaven-Compositor.h"
#include <sys/poll.h>
#include <sys/select.h>
#include <pthread.h>

struct hv_compositor_struct
{
    void *user_data;
    struct pollfd fd;
    struct sockaddr_un name;
    hv_compositor_events_interface *events_interface;
    pthread_mutex_t mutex;
    hv_client_pid active_client;
    int initialized;
};

/* Utils */

int hv_server_read(hv_compositor *compositor, void *dst, ssize_t bytes)
{
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 500000;

    fd_set set;
    FD_ZERO(&set);
    FD_SET(compositor->fd.fd, &set);

    if(select(compositor->fd.fd + 1, &set, NULL, NULL, &timeout) <= 0)
    {
        printf("Error: Server read timeout.\n");
        hv_compositor_destroy(compositor);
        return 1;
    }

    u_int8_t *data = dst;
    ssize_t readBytes;

    readBytes = read(compositor->fd.fd , data, bytes);

    if(readBytes < 1)
    {
        printf("Error: Server read failed. read() call returned %zd\n", readBytes);
        hv_compositor_destroy(compositor);
        return 1;
    }
    else if(readBytes < bytes)
        hv_server_read(compositor, &data[readBytes], bytes - readBytes);

    return 0;
}

int hv_server_write(hv_compositor *compositor, void *src, ssize_t bytes)
{
    u_int8_t *data = src;

    ssize_t writtenBytes = write(compositor->fd.fd, data, bytes);

    if(writtenBytes < 1)
    {
        hv_compositor_destroy(compositor);
        return 1;
    }
    else if(writtenBytes < bytes)
        return hv_server_write(compositor, &data[writtenBytes], bytes - writtenBytes);

    return 0;
}

/* COMPOSITOR */

hv_compositor *hv_compositor_create(const char *socket_name, void *user_data, hv_compositor_events_interface *events_interface)
{
    const char *sock_name;

    if(socket_name)
    {
        if(socket_name[0] == '/')
        {
            printf("Error: Invalid socket name \"%s\". Can not start with \"/\".\n", socket_name);
            return NULL;
        }
        sock_name = socket_name;
    }
    else
        sock_name = HV_DEFAULT_SOCKET;

    char *xdg_runtime_dir = getenv("XDG_RUNTIME_DIR");

    if(!xdg_runtime_dir)
    {
        printf("Error: XDG_RUNTIME_DIR env not set.\n");
        return NULL;
    }

    hv_compositor *compositor = malloc(sizeof(hv_compositor));
    compositor->events_interface = events_interface;
    compositor->fd.events = POLLIN | POLLHUP;
    compositor->fd.revents = 0;
    compositor->user_data = user_data;
    compositor->active_client = 0;

    if(pthread_mutex_init(&compositor->mutex, NULL) != 0)
    {
        printf("Error: Could not create mutex.\n");
        free(compositor);
        return NULL;
    }

    if( (compositor->fd.fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
    {
        printf("Error: Could not create socket.\n");
        free(compositor);
        return NULL;
    }

    int xdg_runtime_dir_len = strlen(xdg_runtime_dir);

    memset(&compositor->name.sun_path, 0, 108);
    compositor->name.sun_family = AF_UNIX;
    memcpy(compositor->name.sun_path, xdg_runtime_dir, xdg_runtime_dir_len);

    if(xdg_runtime_dir[xdg_runtime_dir_len-1] != '/')
    {
        compositor->name.sun_path[xdg_runtime_dir_len] = '/';
        xdg_runtime_dir_len++;
    }

    strncpy(&compositor->name.sun_path[xdg_runtime_dir_len], sock_name, 107 - xdg_runtime_dir_len);

    if(connect(compositor->fd.fd, (const struct sockaddr*)&compositor->name, sizeof(struct sockaddr_un)) == -1)
    {
        printf("Error: No server currently running in %s.\n", compositor->name.sun_path);
        free(compositor);
        return NULL;
    }

    /* IDENTIFY */

    pthread_mutex_lock(&compositor->mutex);

    hv_connection_type type = HV_CONNECTION_TYPE_COMPOSITOR;

    /* CONNECTION TYPE: UInt32 */

    if(hv_server_write(compositor, &type, sizeof(hv_connection_type)))
    {
        pthread_mutex_unlock(&compositor->mutex);
        return NULL;
    }

    /* CHECK IF SERVER ACCEPTED CONNECTION */

    hv_compositor_auth_reply reply;

    if(hv_server_read(compositor, &reply, sizeof(hv_connection_type)))
    {
        pthread_mutex_unlock(&compositor->mutex);
        return NULL;
    }

    if(reply != HV_COMPOSITOR_ACCEPTED)
    {
        printf("Error: There is already another compositor connected to the global menu.\n");
        hv_compositor_destroy(compositor);
        pthread_mutex_unlock(&compositor->mutex);
        return NULL;
    }

    compositor->initialized = 1;

    pthread_mutex_unlock(&compositor->mutex);

    return compositor;

}

void hv_compositor_destroy(hv_compositor *compositor)
{
    pthread_mutex_destroy(&compositor->mutex);

    close(compositor->fd.fd);

    if(compositor->initialized)
        compositor->events_interface->disconnected_from_server(compositor);

    free(compositor);
}

/* REQUESTS */

int hv_compositor_set_active_client(hv_compositor *compositor, hv_client_pid client_pid)
{
    if(!compositor)
        return HV_ERROR;

    if(client_pid == compositor->active_client)
        return HV_SUCCESS;

    hv_message_id msg_id;

    pthread_mutex_lock(&compositor->mutex);

    if(hv_server_write(compositor, &msg_id, sizeof(hv_message_id)))
    {
        pthread_mutex_unlock(&compositor->mutex);
        return HV_CONNECTION_LOST;
    }

    if(hv_server_write(compositor, &client_pid, sizeof(hv_client_pid)))
    {
        pthread_mutex_unlock(&compositor->mutex);
        return HV_CONNECTION_LOST;
    }

    compositor->active_client = client_pid;

    pthread_mutex_unlock(&compositor->mutex);

    return HV_SUCCESS;
}

int hv_compositor_send_custom_request(hv_compositor *compositor, void *data, u_int32_t size)
{
    if(!compositor)
        return HV_ERROR;

    if(size == 0)
        return HV_SUCCESS;

    if(!data)
        return HV_ERROR;

    // Memory access test
    u_int8_t *test = data;
    test = &test[size-1];
    HV_UNUSED(test);

    hv_message_id msg_id = HV_COMPOSITOR_SEND_CUSTOM_REQUEST_ID;

    pthread_mutex_lock(&compositor->mutex);

    if(hv_server_write(compositor, &msg_id, sizeof(hv_message_id)))
    {
        pthread_mutex_unlock(&compositor->mutex);
        return HV_CONNECTION_LOST;
    }

    if(hv_server_write(compositor, &size, sizeof(u_int32_t)))
    {
        pthread_mutex_unlock(&compositor->mutex);
        return HV_CONNECTION_LOST;
    }

    if(hv_server_write(compositor, data, size))
    {
        pthread_mutex_unlock(&compositor->mutex);
        return HV_CONNECTION_LOST;
    }

    pthread_mutex_unlock(&compositor->mutex);

    return HV_SUCCESS;
}

int hv_compositor_get_fd(hv_compositor *compositor)
{
    return compositor->fd.fd;
}

/* EVENTS */

int hv_send_custom_event_handler(hv_compositor *compositor)
{
    u_int32_t data_len;

    if(hv_server_read(compositor, &data_len, sizeof(u_int32_t)))
        return HV_CONNECTION_LOST;

    if(data_len == 0)
        return HV_SUCCESS;

    u_int8_t data[data_len];

    if(hv_server_read(compositor, data, data_len))
        return HV_CONNECTION_LOST;

    compositor->events_interface->server_send_custom_event(compositor, data, data_len);

    return HV_SUCCESS;
}

/* CONNECTION */

int hv_compositor_handle_server_event(hv_compositor *compositor)
{
    hv_message_id msg_id;

    if(hv_server_read(compositor, &msg_id, sizeof(hv_message_id)))
        return HV_CONNECTION_LOST;

    switch (msg_id)
    {
        case HV_SERVER_TO_COMPOSITOR_SEND_CUSTOM_EVENT_ID:
            return hv_send_custom_event_handler(compositor);
        break;
    }

    hv_compositor_destroy(compositor);
    return HV_CONNECTION_LOST;
}

int hv_compositor_dispatch_events(hv_compositor *compositor, int timeout)
{
    int ret = poll(&compositor->fd, 1, timeout);

    if(ret == -1)
    {
        hv_compositor_destroy(compositor);
        return HV_CONNECTION_LOST;
    }

    if(ret != 0)
    {
        // Disconnected from server
        if(compositor->fd.revents & POLLHUP)
        {
            hv_compositor_destroy(compositor);
            return HV_CONNECTION_LOST;
        }

        // Server event
        if(compositor->fd.revents & POLLIN)
        {
            return hv_compositor_handle_server_event(compositor);
        }
    }

    return HV_SUCCESS;
}

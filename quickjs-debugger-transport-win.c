#include "quickjs-debugger.h"

#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <winsock2.h>

struct js_transport_data {
    int handle;
} js_transport_data;

static size_t js_transport_read(void *udata, char *buffer, size_t length) {
    struct js_transport_data* data = (struct js_transport_data *)udata;
    if (data->handle <= 0)
        return -1;

    if (length == 0)
        return -2;

    if (buffer == NULL)
        return -3;

    //ssize_t ret = read(data->handle, (void *)buffer, length);
	ssize_t ret = recv( data->handle, (void*)buffer, length, 0);

    if (ret == SOCKET_ERROR )
        return -4;

    if (ret == 0)
        return -5;

    if (ret > length)
        return -6;

    return ret;
}

static size_t js_transport_write(void *udata, const char *buffer, size_t length) {
    struct js_transport_data* data = (struct js_transport_data *)udata;
    if (data->handle <= 0)
        return -1;

    if (length == 0)
        return -2;

    if (buffer == NULL) {
        return -3;
	}

    //size_t ret = write(data->handle, (const void *) buffer, length);
	size_t ret = send( data->handle, (const void *) buffer, length, 0);
    if (ret <= 0 || ret > (ssize_t) length)
        return -4;

    return ret;
}

static size_t js_transport_peek(void *udata) {
    struct js_transport_data* data = (struct js_transport_data *)udata;
    if (data->handle <= 0)
        return -1;
    fd_set read_fds={0};
    FD_SET(data->handle,&read_fds);
    TIMEVAL time_out={0,0};
    int ret = select(1,&read_fds,NULL,NULL,&time_out);
    if(ret==0)
        return 0;//time out. no data
    else if(ret == SOCKET_ERROR)
        return -2;
    else
        return 1;
    /*
    WSAPOLLFD  fds[1];
    int poll_rc;
    fds[0].fd = data->handle;
    fds[0].events = POLLIN;
    fds[0].revents = 0;

    poll_rc = WSAPoll(fds, 1, 0);
    if (poll_rc < 0)
        return -2;
    if (poll_rc > 1)
        return -3;
    // no data
    if (poll_rc == 0)
        return 0;
    // has data    
    return 1;
    */
}

static void js_transport_close(JSRuntime* rt, void *udata) {
    struct js_transport_data* data = (struct js_transport_data *)udata;
    if (data->handle <= 0)
        return;

    close(data->handle);
	data->handle = 0;

    free(udata);
}

// todo: fixup asserts to return errors.
static struct sockaddr_in js_debugger_parse_sockaddr(const char* address) {
    char* port_string = strstr(address, ":");
    assert(port_string);

    int port = atoi(port_string + 1);
    assert(port);

    char host_string[256];
    strcpy(host_string, address);
    host_string[port_string - address] = 0;

    struct hostent *host = gethostbyname(host_string);
    assert(host);
    struct sockaddr_in addr;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    memcpy((char *)&addr.sin_addr.s_addr, (char *)host->h_addr, host->h_length);
    addr.sin_port = htons(port);

    return addr;
}

int js_debugger_connect(JSContext *ctx, const char *address) {

    int ret = 0;
    char* port_string = strstr(address, ":");
    assert(port_string);

    int port = atoi(port_string + 1);
    assert(port);

    int client = socket(AF_INET, SOCK_STREAM, 0);
    assert(client > 0);
    char host_string[256];
    strcpy(host_string, address);
    host_string[port_string - address] = 0;

    struct hostent *host = gethostbyname(host_string);
    assert(host);
    struct sockaddr_in addr;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    memcpy((char *)&addr.sin_addr.s_addr, (char *)host->h_addr, host->h_length);
    addr.sin_port = htons(port);
    ret = connect(client, (const struct sockaddr *)&addr, sizeof(addr));
    if(ret == 0){
        struct js_transport_data *data = (struct js_transport_data *)malloc(sizeof(struct js_transport_data));
        data->handle = client;
        js_debugger_attach(ctx, js_transport_read, js_transport_write, js_transport_peek, js_transport_close, data);
    }else{
        printf("connect ret:%d,err:%d\n",ret,(int)GetLastError());
    }
    return ret;
}

void js_debugger_wait_connection(JSContext *ctx, const char* address) {
    struct sockaddr_in addr = js_debugger_parse_sockaddr(address);

    int server = socket(AF_INET, SOCK_STREAM, 0);
    assert(server >= 0);

    int reuseAddress = 1;
    assert(setsockopt(server, SOL_SOCKET, SO_REUSEADDR, (const char *) &reuseAddress, sizeof(reuseAddress)) >= 0);

    assert(bind(server, (struct sockaddr *) &addr, sizeof(addr)) >= 0);

    listen(server, 1);

    struct sockaddr_in client_addr;
    int client_addr_size = (int) sizeof(addr);
    int client = accept(server, (struct sockaddr *) &client_addr, &client_addr_size);
    close(server);
    assert(client >= 0);

    struct js_transport_data *data = (struct js_transport_data *)malloc(sizeof(struct js_transport_data));
    memset(data, 0, sizeof(js_transport_data));
    data->handle = client;
    js_debugger_attach(ctx, js_transport_read, js_transport_write, js_transport_peek, js_transport_close, data);
}

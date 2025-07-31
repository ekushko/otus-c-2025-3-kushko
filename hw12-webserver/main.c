#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <sys/sendfile.h>

#define BUF_SZ 2048
#define STR_LEN 256
#define EPOLL_MAX 128
#define QUEUE 128

#define FORBIDDEN_RESPONSE \
"HTTP/1.1 403 Forbidden\r\n\
Content-Length: %d\r\n\
Connection: close\r\n\
Content-Type: text/html\r\n\r\n\
<html><head><title>403 Forbidden</title></head>\
<body>\
<h1>Forbidden</h1>\
<p>You don't have permission to access /%s on this server!</p>\
</body></html>\n"


#define NOT_FOUND_RESPONSE \
"HTTP/1.1 404 Not Found\r\n\
Content-Length: 133\r\n\
Connection: close\r\n\
Content-Type: text/html\r\n\r\n\
<html><head><title>404 Not Found</title></head><body><h1>Not Found</h1>The requested URL was not found on this server.</body></html>\n"


#define OK_RESPONSE \
"HTTP/1.1 200 OK\r\n\
Server: chttp\r\n\
Content-Length: %ld\r\n\
Connection: close\r\n\
Content-Type: text/html\r\n\r\n"


static struct epoll_event events[EPOLL_MAX];

typedef struct {
    int    sock;
    char   req_buf[BUF_SZ];
    size_t req_size;
    char   res_buf[BUF_SZ];
    size_t res_size;
    char   url[STR_LEN];
    size_t url_len;
} req_data_t;


char *default_dir_path = ".";
char *default_addr = "127.0.0.1";
int   default_port = 30020;


int parse_url(char* line, char* url) {
    char *token;
    token = strtok(line, " ");
    if (token == NULL) {
        fprintf(stderr, "Failed to parse %s: %s\n", line, strerror(errno));
        url[0] = '\0';
        return 0;
    }
    token = strtok(NULL, " ");
    if (token == NULL) {
        fprintf(stderr, "Failed to parse %s: %s\n", line, strerror(errno));
        url[0] = '\0';
        return 0;
    }
    token++;
    strncpy(url, token, STR_LEN - 1);
    url[STR_LEN - 1] = '\0';
    return strlen(url);
}

int set_nonblocking(int sock) {
    int opts;
    opts = fcntl(sock, F_GETFL);
    if (opts < 0) {
        perror("fcntl(F_GETFL)");
        return -1;
    }
    opts = (opts | O_NONBLOCK);
    if (fcntl(sock, F_SETFL, opts) < 0) {
        perror("fcntl(F_SETFL)");
        return -1;
    }
    return 0;
}

void read_req(req_data_t* req) {
    int res = 0;
    char *header = NULL;
    while (header == NULL) {
        if ((res = recv(req->sock, req->req_buf, BUF_SZ, 0)) < 0) {
            perror("read_req");
            return;
        }
        header = strstr(req->req_buf, "\r\n\r\n");
    }
    req->req_buf[res] = '\0';
    req->req_size = res;
    req->url_len = parse_url(req->req_buf, req->url);
}

void write_res(req_data_t* req) {
    if (req->url_len != 0) {
        if ((access(req->url, R_OK) != 0) && (errno == EACCES)) {
            req->res_size = snprintf(req->res_buf, BUF_SZ - 1, FORBIDDEN_RESPONSE, (146 + (int)req->url_len), req->url);
            req->res_buf[req->res_size] = '\0';
            if (send(req->sock, req->res_buf, req->res_size, 0) < 0) {
                perror("write_res");
                return;
            }
            return;
        }
        int fd;
        if ((fd = open(req->url, O_RDONLY)) == -1) {
            req->res_size = snprintf(req->res_buf, BUF_SZ - 1, NOT_FOUND_RESPONSE);
            req->res_buf[req->res_size] = '\0';
            if (send(req->sock, req->res_buf, req->res_size, 0) < 0) {
                perror("write_res");
                return;
            }
            return;
        } else {
            struct stat stat_buf;
            fstat(fd, &stat_buf);
            req->res_size = sprintf(req->res_buf, OK_RESPONSE, stat_buf.st_size);
            req->res_buf[req->res_size] = '\0';
            if (send(req->sock, req->res_buf, req->res_size, 0) < 0) {
                perror("write_res");
            }
            if (sendfile(req->sock, fd, NULL, stat_buf.st_size) < 0) {
                perror("write_res");
            }
        }
        close(fd);
    }
}

int main(int argc, char* argv[]) {
    char *dir_path;
    char *addr;
    int   port;

    if (argc >= 4) {
        port = atoi(argv[3]);
    } else {
        port = 30020;
        printf("Port wasn't specified. Default(%d) used\n", port);
    }

    if (argc >= 3) {
        size_t addr_len = strlen(argv[2]);
        addr = malloc(addr_len + 1);
        addr[addr_len] = '\0';
        strcpy(addr, argv[2]);
    } else {
        size_t addr_len = strlen(default_addr);
        addr = malloc(addr_len + 1);
        addr[addr_len] = '\0';
        strcpy(addr, default_addr);
        printf("IP wasn't specified. Default(%s) used\n", addr);
    }

    if (argc >= 2) {
        size_t dir_path_len = strlen(argv[1]);
        dir_path = malloc(dir_path_len + 1);
        dir_path[dir_path_len] = '\0';
        strcpy(dir_path, argv[1]);
    } else {
        size_t dir_path_len = strlen(default_dir_path);
        dir_path = malloc(dir_path_len + 1);
        dir_path[dir_path_len] = '\0';
        strcpy(dir_path, default_dir_path);
        printf("Working DIR wasn't specified. Default(%s) used\n", dir_path);
    }

    int epoll, server, clients, conn;
    struct sockaddr_in server_addr = { 0 };
    struct epoll_event listen_ev, conn_ev;
    int ev_count;
    req_data_t  req = {0};
    req_data_t* req_ptr = &req;

    chdir(dir_path);

    signal(SIGPIPE, SIG_IGN);

    epoll = epoll_create(EPOLL_MAX);
    server = socket(AF_INET, SOCK_STREAM, 0);
    if (server < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    int reuse = 1;
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse));
    setsockopt(server, SOL_SOCKET, SO_REUSEPORT, (const char*)&reuse, sizeof(reuse));

    set_nonblocking(server);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(addr);
    server_addr.sin_port = htons(port);

    if (bind(server, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if (listen(server, QUEUE) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Server(%s:%d) started. Working DIR: %s\n", addr, port, dir_path);

    listen_ev.events = EPOLLIN | EPOLLET;
    listen_ev.data.fd = server;
    if (epoll_ctl(epoll, EPOLL_CTL_ADD, server, &listen_ev) < 0) {
        perror("epoll_ctl");
        exit(EXIT_FAILURE);
    }

    ev_count = 1;
    for (;;) {
        clients = epoll_wait(epoll, events, EPOLL_MAX, -1);
        for (int i = 0; i < clients; i++) {
            if (events[i].data.fd == server) {
                conn = accept(server, NULL, NULL);
                if (conn < 0) {
                    perror("accept");
                    continue;
                }
                if (ev_count == EPOLL_MAX - 1) {
                    close(conn);
                    continue;
                }
                set_nonblocking(conn);
                conn_ev.data.ptr = &req;
                req.sock = conn;
                conn_ev.events = EPOLLIN | EPOLLOUT | EPOLLET | EPOLLRDHUP;
                if (epoll_ctl(epoll, EPOLL_CTL_ADD, conn, &conn_ev) < 0) {
                    perror("epoll_ctl");
                    close(conn);
                    continue;
                }
                ev_count++;
            } else {
                req_ptr = (req_data_t*)events[i].data.ptr;
                if (events[i].events & EPOLLIN) {
                    read_req(req_ptr);
                }
                if (events[i].events & EPOLLOUT) {
                    write_res(req_ptr);
                }
                if (events[i].events & EPOLLRDHUP) {
                    fprintf(stderr, "fd %d error!\n", req_ptr->sock);
                }
                if (req_ptr->res_size != 0) {
                    epoll_ctl(epoll, EPOLL_CTL_DEL, req_ptr->sock, &conn_ev);
                    close(req_ptr->sock);
                    memset(&req, 0, sizeof(req));
                    ev_count--;
                }
            }
        }
    }
    exit(EXIT_SUCCESS);
}

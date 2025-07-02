#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <errno.h>

#define BUF_SIZE 5000
#define SERVER "telehack.com"
#define PROTOCOL "telnet"

int main(int argc, char* argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Bad arguments. Usage: ./hw11-telnet font message\n");
        return 0;
    }

    const char* font = argv[1];
    const char* msg = argv[2];

    struct addrinfo hints = {0};
    struct addrinfo* rp;
    struct timeval tv;
    int res, sock;
    char* request;
    size_t  acc_len = 0, req_len = 0;
    ssize_t trn_len = 0, rcv_len = 0;
    char buf[BUF_SIZE];

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_ADDRCONFIG;

    res = getaddrinfo(SERVER, PROTOCOL, &hints, &rp);
    if (res != 0) {
        fprintf(stderr, "Error: %s\n", gai_strerror(res));
        return 0;
    }

    sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (sock < 0) {
        fprintf(stderr, "Error: Socket not opened!");
        freeaddrinfo(rp);
        return 0;
    }

    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

    if (connect(sock, rp->ai_addr, rp->ai_addrlen) < 0) {
        fprintf(stderr, "Error: Socket not connected!");
        freeaddrinfo(rp);
        close(sock);
        return 0;
    }

    freeaddrinfo(rp);

    while ((rcv_len = recv(sock, &buf, BUF_SIZE, 0)) > 0)
        continue;

    const char *cmd = "figlet";
    req_len = strlen(cmd) + 2 + strlen(font) + 1 + strlen(msg) + 2;
    request = malloc(req_len + 1);
    snprintf(request, req_len + 1, "%s /%s %s\r\n", cmd, font, msg);

    if ((trn_len = send(sock, request, req_len, 0)) != (ssize_t)req_len) {
        fprintf(stderr, "Error: Sent %ld but transmitted %ld (%s)\n", req_len, trn_len, strerror(errno));
        free(request);
        close(sock);
        return 0;
    }

    memset(buf, 0, BUF_SIZE);
    while ((rcv_len = recv(sock, &buf[acc_len], BUF_SIZE - acc_len, 0)) > 0)
        acc_len = acc_len + rcv_len;

    for (size_t i = strlen(request); i < acc_len - 1; ++i)
        printf("%c", buf[i]);

    shutdown(sock, SHUT_RDWR);
    close(sock);

    return 0;
}

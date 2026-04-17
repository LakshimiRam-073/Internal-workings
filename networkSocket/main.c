#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

typedef struct addrinfo addressInfo;

int main(void) {

    addressInfo hints = {0};
    addressInfo *res, *p;
    char address_string[INET6_ADDRSTRLEN];
    int status;

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((status = getaddrinfo("www.google.com", "ssh", &hints, &res)) != 0) {
        fprintf(stderr, "gai_address_info %s\n", gai_strerror(status));
        return 1;
    }

    for (p = res; p != NULL; p = p->ai_next) {

        void *address;
        char *ipVer;
        in_port_t port;

        if (p->ai_family == AF_INET) {
            struct sockaddr_in *ipv4 = (struct sockaddr_in*) p->ai_addr;
            ipVer = "IPv4";
            address = &(ipv4->sin_addr);
            port = ipv4->sin_port;

        } else if (p->ai_family == AF_INET6) {
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6*) p->ai_addr;
            ipVer = "IPv6";
            address = &(ipv6->sin6_addr);
            port = ipv6->sin6_port;

        } else {
            continue;
        }

        inet_ntop(p->ai_family, address, address_string, sizeof address_string);
        printf("IP version %s %s %d \n", ipVer, address_string,port);
    }

    freeaddrinfo(res);
    return 0;
}
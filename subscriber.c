/*
 * Protocoale de comunicatii
 * Laborator 7 - TCP si mulplixare
 * client.c
 */

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"
#include "helpers.h"

void run_client(int sockfd, char *argv[]) {
    char buf[MSG_MAXSIZE + 1];
    memset(buf, 0, MSG_MAXSIZE + 1);

    // id-ul
    send(sockfd, argv[1], strlen(argv[1]), 0);

    struct pollfd poll_fds[2];
    int num_sockets = 2;
    int rc;
    struct chat_packet sent_packet;
    struct chat_packet received_packet;

    // Adaugam noul file descriptor (socketul pe care se asculta conexiuni) in
    // multimea poll_fds
    poll_fds[0].fd = sockfd;
    poll_fds[0].events = POLLIN;

    poll_fds[1].fd = STDIN_FILENO;
    poll_fds[1].events = POLLIN;

    while (1) {
        // Asteptam sa primim ceva pe unul dintre cei num_sockets socketi
        rc = poll(poll_fds, num_sockets, -1);
        DIE(rc < 0, "poll");

        for (int i = 0; i < num_sockets; i++) {
            if (poll_fds[i].revents & POLLIN) {
                // daca primesc mesaj de la server
                if (poll_fds[i].fd == sockfd) {
                    // mesajul
                    rc = recv_all(sockfd, &received_packet,
                                  sizeof(received_packet));
                    if (rc <= 0) {
                        return;
                    }
                } else {
                    printf("intra?\n");
                    // mesaje de la tastatura
                    char buff_msg[1501];
                    fgets(buff_msg, 1500, stdin);

                    if (strncmp("exit", buff_msg, 4) == 0) {
                        return;
                    }
                }
            }
        }
    }
}

int main(int argc, char *argv[]) {
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    // ./subscriber id ip port
    if (argc != 4) {
        printf("\n ID: %s <ip> <port>\n", argv[0]);
        return 1;
    }

    // Parsam port-ul ca un numar
    uint16_t port;
    int rc = sscanf(argv[3], "%hu", &port);
    DIE(rc != 1, "Given port is invalid");

    // Obtinem un socket TCP pentru conectarea la server
    const int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    DIE(sockfd < 0, "socket");

    // Completăm in serv_addr adresa serverului, familia de adrese si portul
    // pentru conectare
    struct sockaddr_in serv_addr;
    socklen_t socket_len = sizeof(struct sockaddr_in);

    memset(&serv_addr, 0, socket_len);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    rc = inet_pton(AF_INET, argv[2], &serv_addr.sin_addr.s_addr);
    DIE(rc <= 0, "inet_pton");

    // Ne conectăm la server
    rc = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    DIE(rc < 0, "connect");

    run_client(sockfd, argv);

    // Inchidem conexiunea si socketul creat
    close(sockfd);

    return 0;
}

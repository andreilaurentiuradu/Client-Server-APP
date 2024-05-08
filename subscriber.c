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
    // struct chat_packet sent_packet;
    udp_msg received_packet;

    // adaugam noul file descriptor (socketul pe care se asculta conexiuni) in
    // multimea poll_fds
    poll_fds[0].fd = sockfd;
    poll_fds[0].events = POLLIN;

    poll_fds[1].fd = STDIN_FILENO;
    poll_fds[1].events = POLLIN;

    while (1) {
        // asteptam sa primim ceva pe unul dintre cei num_sockets socketi
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

                    switch (received_packet.data_type) {
                        case 0:
                            printf("%s:%u - %s - %s - %s\n",
                                   inet_ntoa(received_packet.ip_address),
                                   received_packet.port, received_packet.topic,
                                   "INT", received_packet.payload);
                            break;
                        case 1:
                            printf("%s:%u - %s - %s - %s\n",
                                   inet_ntoa(received_packet.ip_address),
                                   received_packet.port, received_packet.topic,
                                   "SHORT_REAL", received_packet.payload);
                            break;
                        case 2:
                            printf("%s:%u - %s - %s - %s\n",
                                   inet_ntoa(received_packet.ip_address),
                                   received_packet.port, received_packet.topic,
                                   "FLOAT", received_packet.payload);
                            break;
                        case 3:
                            printf("%s:%u - %s - %s - %s\n",
                                   inet_ntoa(received_packet.ip_address),
                                   received_packet.port, received_packet.topic,
                                   "STRING", received_packet.payload);
                            break;
                    }
                } else {
                    // mesaje de la tastatura
                    char buff_msg[1501];
                    fgets(buff_msg, 1500, stdin);
                    buff_msg[strlen(buff_msg) - 1] = '\0';

                    if (strncmp("exit", buff_msg, 4) == 0) {
                        return;
                    } else if (strncmp("subscribe", buff_msg, 9) == 0) {
                        udp_msg_received subscribe_packet;
                        strcpy(subscribe_packet.topic, buff_msg + 10);
                        // tipul pachetului pe care il trimit
                        subscribe_packet.data_type = 1;
                        // trimitem pachetul
                        send(sockfd, &subscribe_packet,
                             sizeof(subscribe_packet), 0);
                        printf("Subscribed to topic %s\n",
                               subscribe_packet.topic);
                    } else if (strncmp("unsubscribe", buff_msg, 11) == 0) {
                        udp_msg_received unsubscribe_packet;
                        strcpy(unsubscribe_packet.topic, buff_msg + 12);
                        // tipul pachetului pe care il trimit
                        unsubscribe_packet.data_type = 0;
                        // trimitem pachetul
                        send(sockfd, &unsubscribe_packet,
                             sizeof(unsubscribe_packet), 0);
                        printf("Unsubscribed from topic %s\n",
                               unsubscribe_packet.topic);
                    }
                }
            }
        }
    }
}

int main(int argc, char *argv[]) {
    // setam bufferul
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    // ./subscriber id ip port
    if (argc != 4) {
        printf("\n ID: %s <ip> <port>\n", argv[0]);
        return 1;
    }

    // parsam port-ul ca un numar
    uint16_t port;
    int rc = sscanf(argv[3], "%hu", &port);
    DIE(rc != 1, "Given port is invalid");

    // obtinem un socket TCP pentru conectarea la server
    const int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    DIE(sockfd < 0, "socket");

    // completăm in serv_addr adresa serverului, familia de adrese si portul
    // pentru conectare
    struct sockaddr_in serv_addr;
    socklen_t socket_len = sizeof(struct sockaddr_in);

    memset(&serv_addr, 0, socket_len);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    rc = inet_pton(AF_INET, argv[2], &serv_addr.sin_addr.s_addr);
    DIE(rc <= 0, "inet_pton");

    // ne conectăm la server
    rc = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    DIE(rc < 0, "connect");

    // pornim clientul
    run_client(sockfd, argv);

    // inchidem conexiunea si socketul creat
    close(sockfd);

    return 0;
}

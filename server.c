/*
 * Protocoale de comunicatii
 * Laborator 7 - TCP
 * Echo Server
 * server.c
 */

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"
#include "helpers.h"

#define MAX_CONNECTIONS 32

// Primeste date de pe connfd1 si trimite mesajul receptionat pe connfd2
int receive_and_send(int connfd1, int connfd2, size_t len) {
    int bytes_received;
    char buffer[len];

    // Primim exact len octeti de la connfd1
    bytes_received = recv_all(connfd1, buffer, len);
    // S-a inchis conexiunea
    if (bytes_received == 0) {
        return 0;
    }
    DIE(bytes_received < 0, "recv");

    // Trimitem mesajul catre connfd2
    int rc = send_all(connfd2, buffer, len);
    if (rc <= 0) {
        perror("send_all");
        return -1;
    }

    return bytes_received;
}

void run_server(int listenfd) {
    // declar un client (adresa ip, port)
    struct sockaddr_in client_addr1;
    socklen_t clen1 = sizeof(client_addr1);

    int connfd1 = -1;
    int rc;

    // Setam socket-ul listenfd pentru ascultare
    rc = listen(listenfd, 2);
    DIE(rc < 0, "listen");

    // Acceptam o conexiune
    printf("Astept conectarea primului client...\n");
    connfd1 = accept(listenfd, (struct sockaddr *)&client_addr1, &clen1);
    DIE(connfd1 < 0, "accept");

    // ID
    char buffer_msg[1501];
    rc = read(connfd1, buffer_msg, 1500);
    DIE(rc < 0, "read");
    // printf("id: %s\n", buffer_msg);
    memset(buffer_msg, 0, 1500);

    // IP
    rc = read(connfd1, buffer_msg, 1500);
    DIE(rc < 0, "read");
    // printf("ip: %s\n", buffer_msg);
    memset(buffer_msg, 0, 1500);

    // port
    rc = read(connfd1, buffer_msg, 1500);
    DIE(rc < 0, "read");
    // printf("port: %s\n", buffer_msg);
    memset(buffer_msg, 0, 1500);

    // while (1) {
    // }

    // Inchidem conexiunile si socketii creati
    close(connfd1);
}

void run_chat_multi_server(int listenfd) {
    struct pollfd poll_fds[MAX_CONNECTIONS];
    int num_sockets = 1;
    int rc;

    struct chat_packet received_packet;

    // Setam socket-ul listenfd pentru ascultare
    rc = listen(listenfd, MAX_CONNECTIONS);
    DIE(rc < 0, "listen");

    // Adaugam noul file descriptor (socketul pe care se asculta conexiuni) in
    // multimea poll_fds
    poll_fds[0].fd = listenfd;
    poll_fds[0].events = POLLIN;

    /*
      TODO 3: Adaugati un timerfd. Read-ul pe el se va debloca periodic, moment
      in care veti trimite anuntul promotional catre toti clientii.
    */

    while (1) {
        // Asteptam sa primim ceva pe unul dintre cei num_sockets socketi
        rc = poll(poll_fds, num_sockets, -1);
        DIE(rc < 0, "poll");

        for (int i = 0; i < num_sockets; i++) {
            if (poll_fds[i].revents & POLLIN) {
                if (poll_fds[i].fd == listenfd) {
                    // Am primit o cerere de conexiune pe socketul de listen, pe
                    // care o acceptam
                    struct sockaddr_in cli_addr;
                    socklen_t cli_len = sizeof(cli_addr);
                    const int newsockfd = accept(
                        listenfd, (struct sockaddr *)&cli_addr, &cli_len);
                    DIE(newsockfd < 0, "accept");

                    // Adaugam noul socket intors de accept() la multimea
                    // descriptorilor de citire
                    poll_fds[num_sockets].fd = newsockfd;
                    poll_fds[num_sockets].events = POLLIN;
                    num_sockets++;

                    printf(
                        "Noua conexiune de la %s, port %d, socket client %d\n",
                        inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port),
                        newsockfd);
                } else {
                    // Am primit date pe unul din socketii de client, asa ca le
                    // receptionam
                    int rc = recv_all(poll_fds[i].fd, &received_packet,
                                      sizeof(received_packet));
                    DIE(rc < 0, "recv");

                    if (rc == 0) {
                        printf("Socket-ul client %d a inchis conexiunea\n", i);
                        close(poll_fds[i].fd);

                        // Scoatem din multimea de citire socketul inchis
                        for (int j = i; j < num_sockets - 1; j++) {
                            poll_fds[j] = poll_fds[j + 1];
                        }

                        num_sockets--;
                    } else {
                        printf(
                            "S-a primit de la clientul de pe socketul %d "
                            "mesajul: %s\n",
                            poll_fds[i].fd, received_packet.message);
                        /* TODO 2.1: Trimite mesajul catre toti ceilalti clienti
                         */
                    }
                }
            }
        }
    }
}

int main(int argc, char *argv[]) {
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    // primesc portul la care se deschide serverul
    if (argc != 2) {
        printf("\n<Port> %s\n", argv[1]);
        return 1;
    }

    // Parsam port-ul ca un numar
    uint16_t port;
    int rc = sscanf(argv[1], "%hu", &port);
    DIE(rc != 1, "Given port is invalid");

    // Obtinem un socket TCP pentru receptionarea conexiunilor
    const int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    DIE(listenfd < 0, "socket");

    // Completăm in serv_addr adresa serverului, familia de adrese si portul
    // pentru conectare
    struct sockaddr_in serv_addr;
    socklen_t socket_len = sizeof(struct sockaddr_in);

    // Facem adresa socket-ului reutilizabila, ca sa nu primim eroare in caz ca
    // rulam de 2 ori rapid
    const int enable = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) <
        0)
        perror("setsockopt(SO_REUSEADDR) failed");

    // datele despre server
    memset(&serv_addr, 0, socket_len);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    // Asociem adresa serverului cu socketul creat folosind bind
    rc = bind(listenfd, (const struct sockaddr *)&serv_addr, sizeof(serv_addr));
    DIE(rc < 0, "bind");
    listen(listenfd, 2);

    run_server(listenfd);

    // Inchidem listenfd
    close(listenfd);

    return 0;
}

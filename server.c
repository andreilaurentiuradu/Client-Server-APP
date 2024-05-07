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
#define MAX_LEN_ID 10

// pornim serverul
void run_server(int listen_tcp, int listen_udp) {
    // struct pollfd poll_fds[MAX_CONNECTIONS];
    struct pollfd *poll_fds = malloc(sizeof(struct pollfd) * 3);
    // id-urile clientilor
    // char ids[MAX_CONNECTIONS][MAX_LEN_ID];
    char **ids = malloc(sizeof(char *) * 3);

    int nr_clients = 0;
    int num_sockets = 2;
    int rc;
    struct chat_packet received_packet;

    // aetam socket-ul listenfd pentru ascultare
    rc = listen(listen_tcp, MAX_CONNECTIONS);
    DIE(rc < 0, "listen");

    // adaugam noii file descriptors (socketuri pe care se asculta conexiuni) in
    // multimea poll_fds

    // pentru TCP
    poll_fds[0].fd = listen_tcp;
    poll_fds[0].events = POLLIN;

    // pentru input de la tastatura
    poll_fds[1].fd = STDIN_FILENO;
    poll_fds[1].events = POLLIN;

    // pentru UDP
    poll_fds[2].fd = listen_udp;
    poll_fds[2].events = POLLIN;

    while (1) {
        // asteptam sa primim ceva pe unul dintre cei num_sockets socketi
        rc = poll(poll_fds, num_sockets, -1);
        DIE(rc < 0, "poll");

        // parcurgem multimea poll_fds
        for (int i = 0; i < num_sockets; i++) {
            if (poll_fds[i].revents & POLLIN) {
                if (poll_fds[i].fd == listen_tcp) {
                    /* am primit o cerere de conexiune pe socketul de
                    listen_tcp, pe care o acceptam */

                    struct sockaddr_in cli_addr;
                    socklen_t cli_len = sizeof(cli_addr);
                    const int cfd1 = accept(
                        listen_tcp, (struct sockaddr *)&cli_addr, &cli_len);
                    DIE(cfd1 < 0, "accept");

                    // primim mesajul(id-ul)
                    char buffer_msg[1501];
                    memset(buffer_msg, 0, 1500);
                    rc = recv(cfd1, buffer_msg, 1500, 0);
                    DIE(rc < 0, "read");

                    // // verificam daca id-ul exista deja
                    // int closed = 0;
                    // for (int j = 2; j <= nr_clients; ++j) {
                    //     if (strcmp(ids[j], ids[nr_clients]) == 0) {
                    //         printf("Client %s already connected.\n",
                    //                ids[nr_clients]);

                    //         send(cfd1, "exit\n", strlen("exit\n"), 0);
                    //         close(cfd1);
                    //         memset(ids[nr_clients], 0,
                    //         sizeof(ids[nr_clients])); closed = 1; break;
                    //     }
                    // }
                    // if (closed == 1) {
                    //     return;
                    // }
                    ids = realloc(ids, sizeof(char *) * (3 + nr_clients));
                    ids[2 + nr_clients] = malloc(strlen(buffer_msg) + 1);
                    if (ids[2 + nr_clients] == NULL) {
                        printf("Eroare la alocarea memoriei pentru ID.\n");
                        exit(EXIT_FAILURE);
                    }

                    // copiem id-ul clientului in vectorul de id-uri
                    strcpy(ids[2 + nr_clients], buffer_msg);

                    /* adaugam noul socket intors de accept() la multimea
                    descriptorilor de citire */
                    poll_fds = realloc(
                        poll_fds, sizeof(struct pollfd) * (num_sockets + 1));
                    poll_fds[num_sockets].fd = cfd1;
                    poll_fds[num_sockets].events = POLLIN;
                    num_sockets++;

                    // afisam faptul ca avem conectat un client nou
                    printf("New client %s connected from %s:%d.\n", buffer_msg,
                           inet_ntoa(cli_addr.sin_addr),
                           ntohs(cli_addr.sin_port));
                    ++nr_clients;
                    break;
                } else if (poll_fds[i].fd == STDIN_FILENO) {
                    // primim mesaje de la tastatura
                    char buff_msg[1501];
                    memset(buff_msg, 0, 1500);
                    fgets(buff_msg, 1500, stdin);

                    // inchidem serverul
                    if (strncmp("exit", buff_msg, 4) == 0) {
                        for (int j = 2; j < num_sockets; ++j) {
                            free(ids[j]);
                            close(poll_fds[j].fd);
                        }
                        free(ids);
                        free(poll_fds);
                        return;
                    }
                } else if (poll_fds[i].fd == listen_udp) {
                    /* am primit o cerere de conexiune pe socketul de
                   listen_udp, pe care o acceptam */
                    //    TODO
                    socklen_t udp_len = 0;
                    struct sockaddr_in serv_addr;
                    int rc_udp = recvfrom(
                        listen_udp, &received_packet, sizeof(received_packet),
                        0, (struct sockaddr *)&serv_addr, &udp_len);
                    DIE(rc_udp < 0, "recv");

                } else {
                    // se deconecteaza un client
                    int rc = recv_all(poll_fds[i].fd, &received_packet,
                                      sizeof(received_packet));
                    DIE(rc < 0, "recv");

                    if (rc == 0) {
                        printf("Client %s disconnected.\n", ids[i]);
                        free(ids[i]);
                        close(poll_fds[i].fd);

                        // scoatem din multimea de citire socketul inchis
                        for (int j = i; j < num_sockets - 1; j++) {
                            poll_fds[j] = poll_fds[j + 1];
                            strcpy(ids[j], ids[j + 1]);
                        }
                        // actualizam nr de socketi si de clienti
                        nr_clients--;
                        num_sockets--;
                        poll_fds = realloc(poll_fds,
                                           sizeof(struct pollfd) * num_sockets);
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

    // parsam port-ul ca un numar
    uint16_t port;
    int rc = sscanf(argv[1], "%hu", &port);
    DIE(rc != 1, "Given port is invalid");

    // obtinem un socket TCP pentru receptionarea conexiunilor
    const int listen_tcp = socket(AF_INET, SOCK_STREAM, 0);
    DIE(listen_tcp < 0, "socket");

    // completăm in serv_addr adresa serverului, familia de adrese si portul
    // pentru conectare
    struct sockaddr_in serv_addr;
    socklen_t socket_len = sizeof(struct sockaddr_in);

    // facem adresa socket-ului reutilizabila, ca sa nu primim eroare in caz
    // ca rulam de 2 ori rapid
    const int enable = 1;
    if (setsockopt(listen_tcp, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) <
        0)
        perror("setsockopt(SO_REUSEADDR) failed");

    // datele despre server
    memset(&serv_addr, 0, socket_len);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    // asociem adresa serverului cu socketul creat folosind bind
    rc = bind(listen_tcp, (const struct sockaddr *)&serv_addr,
              sizeof(serv_addr));
    DIE(rc < 0, "bind");

    // obtinem un socket UDP pentru receptionarea conexiunilor
    const int listen_udp = socket(AF_INET, SOCK_STREAM, 0);
    DIE(listen_udp < 0, "socket");

    // facem adresa socket-ului reutilizabila, ca sa nu primim eroare in caz
    // ca rulam de 2 ori rapid
    const int enable_udp = 1;
    if (setsockopt(listen_udp, SOL_SOCKET, SO_REUSEADDR, &enable_udp,
                   sizeof(int)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");

    // asociem adresa serverului cu socketul creat folosind bind
    rc = bind(listen_udp, (const struct sockaddr *)&serv_addr,
              sizeof(serv_addr));
    DIE(rc < 0, "bind");

    // pornim serverul
    run_server(listen_tcp, listen_udp);

    // inchidem listenfd
    close(listen_tcp);
    close(listen_udp);

    return 0;
}

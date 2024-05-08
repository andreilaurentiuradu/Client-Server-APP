#ifndef _HELPERS_H
#define _HELPERS_H 1

#include <stdio.h>
#include <stdlib.h>

/*
 * Macro de verificare a erorilor
 * Exemplu:
 * 		int fd = open (file_name , O_RDONLY);
 * 		DIE( fd == -1, "open failed");
 */
typedef struct {
    char topic[50];
    uint8_t data_type;
    char payload[1500];
} udp_msg_received;

typedef struct {
    struct in_addr ip_address;
    in_port_t port;
    uint8_t data_type;
    char topic[51];
    char payload[1501];
} udp_msg;

typedef struct {
    int nr_topics;
    char id[10];
    char topics[100][100];
} client_topics;

#define DIE(assertion, call_description)                       \
    do {                                                       \
        if (assertion) {                                       \
            fprintf(stderr, "(%s, %d): ", __FILE__, __LINE__); \
            perror(call_description);                          \
            exit(EXIT_FAILURE);                                \
        }                                                      \
    } while (0)

#endif

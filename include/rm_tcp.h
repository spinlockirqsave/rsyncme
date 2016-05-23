/*
 * @file        rm_tcp.h
 * @brief       Send data over TCP.
 * @author      Piotr Gregor <piotrek.gregor at gmail.com>
 * @version     0.1.2
 * @date        18 Apr 2016 00:45 AM
 * @copyright   LGPLv2.1
 */


#ifndef RSYNCME_TCP_H
#define RSYNCME_TCP_H


#include "rm_defs.h"
#include "rm_serialize.h"

#include <fcntl.h>


struct rm_ch_ch_ref;

ssize_t
rm_tcp_rx(int fd, void *buf, size_t bytes_n);

ssize_t
rm_tcp_tx(int fd, void *buf, size_t bytes_n);

int
rm_tcp_tx_ch_ch_ref(int fd, const struct rm_ch_ch_ref *e);

/* @brief       Set socket blocking mode.
 * @details     @on is either 0 or 1, if it is 0 blocking mode is turned off
 *              and socket becomes nonblocking, if @on == 1 socket blocking
 *              mode is turned on */
int
rm_set_socket_blocking_mode(int fd, uint8_t on);


#endif  /* RSYNCME_TCP_H */

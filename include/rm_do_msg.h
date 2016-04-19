/*
 * @file        rm_do_msg.h
 * @brief       Execute TCP messages.
 * @author      Piotr Gregor <piotrek.gregor at gmail.com>
 * @version     0.1.2
 * @date        02 Nov 2016 04:29 PM
 * @copyright   LGPLv2.1
 */


#ifndef RSYNCME_DO_MSG_H
#define RSYNCME_DO_MSG_H


#include "rm_defs.h"
#include "rm_session.h"

struct rm_msg_hdr
{
	uint32_t	hash;                           /* security token */
	uint8_t		pt;                             /* payload type */
	uint8_t		x;                              /* unused yet */
	uint8_t		y;                              /* unused yet */
	uint8_t		z;                              /* unused yet */
};

struct rm_msg_push
{
    struct rm_msg_hdr	hdr;                    /* header */
    uint32_t		L;                          /* block size */
    uint16_t		x_name_len;                 /* x file name length */
    char			x_name[RM_FILE_LEN_MAX];    /* x file name */
    uint16_t		y_name_len;                 /* y file name length */
    char			y_name[RM_FILE_LEN_MAX];    /* y file name */
    uint32_t		ip;                         /* IPv4	*/
};

struct rm_msg_pull
{
    struct rm_msg_hdr	hdr;                    /* header */
    uint32_t		L;                          /* block size */
    uint32_t		ch_ch_n;                    /* number of elements in the ch_ch list,
                                                   that follows this msg, ch_ch elements
                                                   are being sent in chunks while computing
                                                   hashes on file */
};

struct rsyncme;

/* @brief       Handles incoming rsync push request in new sesion.
 * @details     Starts ch_ch_tx and delta_rx threads. Used by daemon. */
int
rm_do_msg_push_rx(struct rsyncme* rm,
		unsigned char *buf);

/* @brief       Makes outgoing rsync push request.
 * @details     Used by rsyncme executable. */
int
rm_do_msg_push_tx(struct rsyncme* rm,
		unsigned char *buf);

/* @brief       Handles incoming rsync pull request in new sesion.
 * @details     Daemon's message. */
int
rm_do_msg_pull_rx(struct rsyncme* rm,
		unsigned char *buf);

/* @brief       Makes outgoing rsync pull request.
 * @details     Used by rsyncme executable. */
int
rm_do_msg_pull_tx(struct rsyncme* rm,
		unsigned char *buf);


#endif  /* RSYNCME_DO_MSG_H */


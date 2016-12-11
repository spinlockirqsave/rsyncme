/* @file        rm_do_msg.h
 * @brief       Execute TCP messages.
 * @author      Piotr Gregor <piotrgregor@rsyncme.org>
 * @date        02 Nov 2016 04:29 PM
 * @copyright   LGPLv2.1 */


#ifndef RSYNCME_DO_MSG_H
#define RSYNCME_DO_MSG_H


#include "rm_defs.h"
#include "rm_wq.h"


#define RM_MSG_HDR_HASH_OFFSET  0u
#define RM_MSG_HDR_PT_OFFSET    4u
#define RM_MSG_HDR_FLAGS_OFFSET 5u
#define RM_MSG_HDR_LEN_OFFSET   6u

#define RM_MSG_PUSH_BODY_OFFSET (sizeof(void*))


/* Remember to change rm_calc_msg_hdr_len and all rm_get_msg_(...) methods if changes to messages are made */

struct rm_msg_hdr
{
    uint32_t	hash;                           /* security token   */
    uint8_t		pt;                             /* payload type     */
    uint8_t     flags;                          /* push flags        */
    uint16_t    len;                            /* message length   */
};
#define RM_MSG_HDR_LEN          8

struct rm_msg {                                 /* base type for derivation of messages */
    struct rm_msg_hdr   *hdr;
    void                *body;
};

struct rm_msg_ack {
    struct rm_msg_hdr   *hdr;                   /* pt = rm_pt_type (*_ACK), flags = status (enum rm_error) */
};
#define RM_MSG_ACK_LEN          RM_MSG_HDR_LEN

struct rm_msg_push
{
    struct rm_msg_hdr	*hdr;                   /* header, MUST be first */
    unsigned char       ssid[16];               /* transmitter's session id */
    size_t              L;                      /* block size   */
    uint16_t            x_sz;                   /* size of string including terminating NULL byte '\0' */
    char                x[RM_FILE_LEN_MAX];     /* x file name  */
    uint16_t            y_sz;                   /* size of string including terminating NULL byte '\0' */
    char                y[RM_FILE_LEN_MAX];     /* y file name  */
    uint16_t            z_sz;                   /* size of string including terminating NULL byte '\0' */
    char                z[RM_FILE_LEN_MAX];     /* z file name  */
};

/* transmitter sends PULL(x,y) -> this means receiver does PUSH(y,x) */
struct rm_msg_pull
{
    struct rm_msg_hdr	*hdr;                   /* header, MUST be first */
    unsigned char       ssid[16];               /* transmitter's session id */
    size_t              L;                      /* block size   */
    uint32_t            ch_ch_n;                /* number of elements in the ch_ch list,
                                                   that follows this msg, ch_ch elements
                                                   are being sent in chunks while computing
                                                   hashes on file */
    uint16_t            x_sz;                   /* size of string including terminating NULL byte '\0' */
    char                x[RM_FILE_LEN_MAX];     /* x file name  */
    uint16_t            y_sz;                   /* size of string including terminating NULL byte '\0' */
    char                y[RM_FILE_LEN_MAX];     /* y file name  */
    uint16_t            z_sz;                   /* size of string including terminating NULL byte '\0' */
    char                z[RM_FILE_LEN_MAX];     /* z file name  */
};

enum rm_error rm_msg_push_init(struct rm_msg_push *msg) __attribute__ ((nonnull(1)));
void rm_msg_push_free(struct rm_msg_push *msg) __attribute__ ((nonnull(1)));

enum rm_error rm_msg_ack_init(struct rm_msg_ack *msg) __attribute__ ((nonnull(1)));
void rm_msg_ack_free(struct rm_msg_ack *msg) __attribute__ ((nonnull(1)));

/* @brief       Handles incoming rsync push request in new sesion.
 * @details     Work struct callback for RM_WORK_PROCESS_MSG_PUSH.
 *              Starts ch_ch_tx and delta_rx threads.
 *              Used by daemon.
 *              The session is created and freed in this callback.
 *              The work dctor is not called in this function
 *              but as synchronous dctor by worker thread. */
void* rm_do_msg_push_rx(void* arg) __attribute__ ((nonnull(1)));

/* @brief       Handles incoming rsync pull request in new sesion.
 * @details     Daemon's message. */
int rm_do_msg_pull_rx(struct rsyncme* rm, unsigned char *buf) __attribute__ ((nonnull(1,2)));

/* @brief       Return the size of the header. */
uint16_t rm_calc_msg_hdr_len(struct rm_msg_hdr *hdr);

/* @brief       Return length of message including header, appropriate to put into hdr->len field */
uint16_t rm_calc_msg_len(void *arg);


/* destructors */

void rm_msg_push_dtor(void *arg);

#endif  /* RSYNCME_DO_MSG_H */


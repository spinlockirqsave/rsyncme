/* @file        rm_serialize.h
 * @brief       Serialize TCP messages, checksums.
 * @author      Piotr Gregor <piotrgregor@rsyncme.org>
 * @date        03 Nov 2016 01:56 PM
 * @copyright   LGPLv2.1 */


#ifndef RSYNCME_SERIALIZE_H
#define RSYNCME_SERIALIZE_H


#include "rm_defs.h"
#include "rm_do_msg.h"


unsigned char* rm_serialize_char(unsigned char *buf, char v) __attribute__ ((nonnull(1)));
unsigned char* rm_serialize_u8(unsigned char *buf, uint8_t v) __attribute__ ((nonnull(1)));
unsigned char* rm_serialize_u16(unsigned char *buf, uint16_t v) __attribute__ ((nonnull(1)));
unsigned char* rm_serialize_u32(unsigned char *buf, uint32_t v) __attribute__ ((nonnull(1)));
unsigned char* rm_serialize_u64(unsigned char *buf, uint64_t v) __attribute__ ((nonnull(1)));
unsigned char* rm_serialize_size_t(unsigned char *buf, size_t v) __attribute__ ((nonnull(1)));
unsigned char* rm_serialize_string(unsigned char *buf, const void *src, size_t bytes_n);
unsigned char* rm_serialize_mem(void *buf, const void *src, size_t bytes_n);
unsigned char* rm_serialize_msg_hdr(unsigned char *buf, struct rm_msg_hdr *h);
unsigned char* rm_serialize_msg_push(unsigned char *buf, struct rm_msg_push *m) __attribute__ ((nonnull(1,2)));
unsigned char* rm_serialize_msg_ack(unsigned char *buf, struct rm_msg_ack *m) __attribute__ ((nonnull(1,2)));
unsigned char* rm_serialize_msg_push_ack(unsigned char *buf, struct rm_msg_push_ack *m) __attribute__ ((nonnull(1,2)));
unsigned char* rm_serialize_msg_pull(unsigned char *buf, struct rm_msg_pull *m) __attribute__ ((nonnull(1,2)));


#define RM_MSG_HDR_HASH(b) ntohl(*((uint32_t *)b))


uint32_t rm_get_msg_hdr_hash(unsigned char* buf) __attribute__ ((nonnull(1)));
uint8_t rm_get_msg_hdr_pt(unsigned char* buf) __attribute__ ((nonnull(1)));
uint8_t rm_get_msg_hdr_flags(unsigned char* buf) __attribute__ ((nonnull(1)));
uint16_t rm_get_msg_hdr_len(unsigned char* buf) __attribute__ ((nonnull(1)));
unsigned char* rm_deserialize_u8(unsigned char *buf, uint8_t *v) __attribute__ ((nonnull(1,2)));
unsigned char* rm_deserialize_u16(unsigned char *buf, uint16_t *v) __attribute__ ((nonnull(1,2)));
unsigned char* rm_deserialize_u32(unsigned char *buf, uint32_t *v) __attribute__ ((nonnull(1,2)));
unsigned char* rm_deserialize_u64(unsigned char *buf, uint64_t *v) __attribute__ ((nonnull(1,2)));
unsigned char* rm_deserialize_size_t(unsigned char *buf, size_t *v) __attribute__ ((nonnull(1,2)));
unsigned char* rm_deserialize_string(const void *buf, char *dst, size_t bytes_n);
unsigned char* rm_deserialize_mem(const void *buf, void *dst, size_t bytes_n);
unsigned char* rm_deserialize_msg_hdr(unsigned char *buf, struct rm_msg_hdr *hdr);
unsigned char* rm_deserialize_msg_push_body(unsigned char *buf, struct rm_msg_push *m) __attribute__ ((nonnull(1,2)));
unsigned char* rm_deserialize_msg_push(unsigned char *buf, struct rm_msg_hdr *hdr, struct rm_msg_push **m) __attribute__ ((nonnull(1,2,3)));
unsigned char* rm_deserialize_msg_ack(unsigned char *buf, struct rm_msg_ack *ack) __attribute__ ((nonnull(1,2)));
unsigned char* rm_deserialize_msg_push_ack(unsigned char *buf, struct rm_msg_push_ack *ack) __attribute__ ((nonnull(1,2)));
struct rm_msg* rm_deserialize_msg(enum rm_pt_type pt, struct rm_msg_hdr *hdr, unsigned char *body_raw) __attribute__ ((nonnull(2,3)));


#endif	/* RSYNCME_SERIALIZE_H */

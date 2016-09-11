/* @file        rm_serialize.c
 * @brief       Serialize TCP messages, checksums.
 * @author      Piotr Gregor <piotrgregor@rsyncme.org>
 * @date        03 Nov 2016 01:56 PM
 * @copyright   LGPLv2.1 */


#include "rm_serialize.h"


unsigned char*
rm_serialize_char(unsigned char *buf, char v) {
    buf[0] = v;
    return buf + 1;
}

unsigned char*
rm_serialize_u8(unsigned char *buf, uint8_t v) {
    buf[0] = v;
    return buf + 1;
}

unsigned char*
rm_serialize_u16(unsigned char *buf, uint16_t v) {
    buf[0] = (v >> 8) & 0xFF;
    buf[1] = v & 0xFF;
    return buf + 2;
}

/* @brief   Write big-endian int value into buffer assumes 32-bit int and 8-bit char. */
unsigned char*
rm_serialize_u32(unsigned char *buf, uint32_t v) {
    buf[0] = (v >> 24) & 0xFF;
    buf[1] = (v >> 16) & 0xFF;
    buf[2] = (v >> 8) & 0xFF;
    buf[3] = v & 0xFF;
    return buf + 4;
}

/* @brief   Write big-endian int value into buffer assumes 8-bit char. */
unsigned char *
rm_serialize_size_t(unsigned char *buf, size_t v) {
    if (sizeof(size_t) == 4) {
        buf = rm_serialize_u32(buf, v);
        return buf;
    } else if (sizeof(size_t) == 8) {
        buf[0] = (v >> (24 + 32)) & 0xFF;
        buf[1] = (v >> (16 + 32)) & 0xFF;
        buf[2] = (v >> (8 + 32)) & 0xFF;
        buf[3] = (v >> 32) & 0xFF;
        buf[4] = (v >> 24) & 0xFF;
        buf[5] = (v >> 16) & 0xFF;
        buf[6] = (v >> 8) & 0xFF;
        buf[7] = v & 0xFF;
        return buf + 8;
    } else {
        assert(0 && "Unsuported architecture!");
    }
}

unsigned char *
rm_serialize_msg_hdr(unsigned char *buf, struct rm_msg_hdr *h) {
    buf = rm_serialize_u32(buf, h->hash);
    buf = rm_serialize_u8(buf, h->pt);
    buf = rm_serialize_u8(buf, h->flags);
    buf = rm_serialize_u16(buf, h->len);
    return buf;
}

unsigned char *
rm_serialize_msg_push(unsigned char *buf, struct rm_msg_push *m) {
    buf = rm_serialize_msg_hdr(buf, m->hdr);
    buf = rm_serialize_u32(buf, m->L);
    return buf;
}

unsigned char *
rm_serialize_msg_pull(unsigned char *buf, struct rm_msg_pull *m) {
    buf = rm_serialize_msg_hdr(buf, m->hdr);
    buf = rm_serialize_u32(buf, m->L);
    buf = rm_serialize_u32(buf, m->ch_ch_n);
    return buf;
}

uint32_t
rm_get_msg_hdr_hash(unsigned char *buf) {
    return ntohl(*(uint32_t*)(buf + RM_MSG_HDR_HASH_OFFSET));
}

uint8_t
rm_get_msg_hdr_pt(unsigned char *buf) {
    return *((uint8_t*)(buf + RM_MSG_HDR_PT_OFFSET));
}

uint8_t
rm_get_msg_hdr_flags(unsigned char *buf) {
    return *((uint8_t*)(buf + RM_MSG_HDR_FLAGS_OFFSET));
}

uint16_t
rm_get_msg_hdr_len(unsigned char *buf) {
    uint16_t tmp;
    rm_deserialize_u16(buf + RM_MSG_HDR_LEN_OFFSET, &tmp);
    return tmp;
}

unsigned char*
rm_deserialize_u8(unsigned char *buf, uint8_t *v) {
    *v = 0;
    *v = *buf;
    return buf + 1;
}

unsigned char*
rm_deserialize_u16(unsigned char *buf, uint16_t *v) {
    *v = 0;
    *v = *(buf + 0) << 8;
    *v += *(buf + 1);
    return buf + 2;
}

unsigned char*
rm_deserialize_u32(unsigned char *buf, uint32_t *v) {
    *v = 0;
    *v = *(buf + 0) << 24;
    *v += (*(buf + 1) << 16);
    *v += (*(buf + 2) << 8);
    *v += (*(buf + 3));
    return buf + 4;
}

unsigned char *
rm_deserialize_size_t(unsigned char *buf, size_t *v) {
    if (sizeof(size_t) == 4) {
        buf = rm_deserialize_u32(buf, (uint32_t*)v);
        return buf;
    } else if (sizeof(size_t) == 8) {
        *v = 0;
        *v =  ((size_t)*(buf + 0) << (24 + 32));
        *v += ((size_t)*(buf + 1) << (16 + 32));
        *v += ((size_t)*(buf + 2) << (8 + 32));
        *v += ((size_t)*(buf + 3) << 32);
        *v += ((size_t)*(buf + 4) << 24);
        *v += ((size_t)*(buf + 5) << 16);
        *v += ((size_t)*(buf + 6) << 8);
        *v += *(buf + 7);
        return buf + 8;
    } else {
        assert(0 && "Unsuported architecture!");
    }
}

unsigned char *
rm_deserialize_msg_hdr(unsigned char *buf, struct rm_msg_hdr *hdr) {
    buf = rm_deserialize_u32(buf, &(hdr->hash));
    buf = rm_deserialize_u8(buf, &(hdr->pt));
    buf = rm_deserialize_u8(buf, &(hdr->flags));
    buf = rm_deserialize_u16(buf, &(hdr->len));
    return buf;
}

unsigned char *
rm_deserialize_msg_push(unsigned char *buf, struct rm_msg_push *m) {
    buf = rm_deserialize_msg_hdr(buf, m->hdr);
    buf = rm_deserialize_size_t(buf, &m->L);
    buf = rm_deserialize_u16(buf, &m->x_sz);
    return buf;
}

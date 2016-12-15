/* @file        rm_serialize.c
 * @brief       Serialize TCP messages, checksums.
 * @author      Piotr Gregor <piotrgregor@rsyncme.org>
 * @date        03 Nov 2016 01:56 PM
 * @copyright   LGPLv2.1 */


#include "rm_serialize.h"


unsigned char* rm_serialize_char(unsigned char *buf, char v) {
    buf[0] = v;
    return buf + 1;
}

unsigned char* rm_serialize_u8(unsigned char *buf, uint8_t v) {
    buf[0] = v;
    return buf + 1;
}

unsigned char* rm_serialize_u16(unsigned char *buf, uint16_t v) {
    buf[0] = (v >> 8) & 0xFF;
    buf[1] = v & 0xFF;
    return buf + 2;
}

/* @brief   Write big-endian int value into buffer assumes 32-bit int and 8-bit char. */
unsigned char* rm_serialize_u32(unsigned char *buf, uint32_t v) {
    buf[0] = (v >> 24) & 0xFF;
    buf[1] = (v >> 16) & 0xFF;
    buf[2] = (v >> 8) & 0xFF;
    buf[3] = v & 0xFF;
    return buf + 4;
}

unsigned char* rm_serialize_u64(unsigned char *buf, uint64_t v) {
    buf[0] = (v >> (24 + 32)) & 0xFF;
    buf[1] = (v >> (16 + 32)) & 0xFF;
    buf[2] = (v >> (8 + 32)) & 0xFF;
    buf[3] = (v >> 32) & 0xFF;
    buf[4] = (v >> 24) & 0xFF;
    buf[5] = (v >> 16) & 0xFF;
    buf[6] = (v >> 8) & 0xFF;
    buf[7] = v & 0xFF;
    return buf + 8;
}

/* @brief   Write big-endian int value into buffer assumes 8-bit char. */
unsigned char* rm_serialize_size_t(unsigned char *buf, size_t v) {
    if (sizeof(size_t) == 4) {
        buf = rm_serialize_u32(buf, v);
        return buf;
    } else if (sizeof(size_t) == 8) {
        buf[0] = ((uint64_t) v >> (24 + 32)) & 0xFF;
        buf[1] = ((uint64_t) v >> (16 + 32)) & 0xFF;
        buf[2] = ((uint64_t) v >> (8 + 32)) & 0xFF;
        buf[3] = ((uint64_t) v >> 32) & 0xFF;
        buf[4] = ((uint64_t) v >> 24) & 0xFF;
        buf[5] = ((uint64_t) v >> 16) & 0xFF;
        buf[6] = ((uint64_t) v >> 8) & 0xFF;
        buf[7] = (uint64_t) v & 0xFF;
        return buf + 8;
    } else {
        assert(0 && "Unsuported architecture!");
    }
}

unsigned char* rm_serialize_string(unsigned char *buf, const void *src, size_t bytes_n) {
    if (src == NULL) {
        return buf;
    }
    memcpy(buf, src, bytes_n);
    buf[bytes_n - 1] = '\0';    /* I am not going to produce invalid strings, if you wanted memcpy use serailize_mem instead */
    return (buf + bytes_n);
}

unsigned char* rm_serialize_mem(void *buf, const void *src, size_t bytes_n) {
    if (src == NULL) {
        return (unsigned char*)buf;
    }
    memcpy(buf, src, bytes_n);
    return (((unsigned char*)buf) + bytes_n);
}

unsigned char* rm_serialize_msg_hdr(unsigned char *buf, struct rm_msg_hdr *h) {
    buf = rm_serialize_u32(buf, h->hash);
    buf = rm_serialize_u8(buf, h->pt);
    buf = rm_serialize_u8(buf, h->flags);
    buf = rm_serialize_u16(buf, h->len);
    return buf;
}

unsigned char* rm_serialize_msg_push(unsigned char *buf, struct rm_msg_push *m) {
    buf = rm_serialize_msg_hdr(buf, m->hdr);
    buf = rm_serialize_mem(buf, m->ssid, sizeof(m->ssid));
    buf = rm_serialize_u64(buf, m->L);
    buf = rm_serialize_u16(buf, m->x_sz);
    buf = rm_serialize_string(buf, m->x, m->x_sz);
    buf = rm_serialize_u16(buf, m->y_sz);
    buf = rm_serialize_string(buf, m->y, m->y_sz);
    buf = rm_serialize_u16(buf, m->z_sz);
    buf = rm_serialize_string(buf, m->z, m->z_sz);
    return buf;
}

unsigned char* rm_serialize_msg_ack(unsigned char *buf, struct rm_msg_ack *m) {
    buf = rm_serialize_msg_hdr(buf, m->hdr);
    return buf;
}

unsigned char* rm_serialize_msg_pull(unsigned char *buf, struct rm_msg_pull *m) {
    buf = rm_serialize_msg_hdr(buf, m->hdr);
    buf = rm_serialize_u32(buf, m->L);
    buf = rm_serialize_u32(buf, m->ch_ch_n);
    return buf;
}

uint32_t rm_get_msg_hdr_hash(unsigned char *buf) {
    return ntohl(*(uint32_t*)(buf + RM_MSG_HDR_HASH_OFFSET));
}

uint8_t rm_get_msg_hdr_pt(unsigned char *buf) {
    return *((uint8_t*)(buf + RM_MSG_HDR_PT_OFFSET));
}

uint8_t rm_get_msg_hdr_flags(unsigned char *buf) {
    return *((uint8_t*)(buf + RM_MSG_HDR_FLAGS_OFFSET));
}

uint16_t rm_get_msg_hdr_len(unsigned char *buf) {
    uint16_t tmp;
    rm_deserialize_u16(buf + RM_MSG_HDR_LEN_OFFSET, &tmp);
    return tmp;
}

unsigned char* rm_deserialize_u8(unsigned char *buf, uint8_t *v) {
    *v = 0;
    *v = *buf;
    return buf + 1;
}

unsigned char* rm_deserialize_u16(unsigned char *buf, uint16_t *v) {
    *v = 0;
    *v = *(buf + 0) << 8;
    *v += *(buf + 1);
    return buf + 2;
}

unsigned char* rm_deserialize_u32(unsigned char *buf, uint32_t *v) {
    *v = 0;
    *v = *(buf + 0) << 24;
    *v += (*(buf + 1) << 16);
    *v += (*(buf + 2) << 8);
    *v += (*(buf + 3));
    return buf + 4;
}

unsigned char* rm_deserialize_u64(unsigned char *buf, uint64_t *v) {
    *v = 0;
    *v =  ((uint64_t) *(buf + 0) << (24 + 32));
    *v += ((uint64_t) *(buf + 1) << (16 + 32));
    *v += ((uint64_t) *(buf + 2) << (8 + 32));
    *v += ((uint64_t) *(buf + 3) << 32);
    *v += ((uint64_t) *(buf + 4) << 24);
    *v += ((uint64_t) *(buf + 5) << 16);
    *v += ((uint64_t) *(buf + 6) << 8);
    *v += (uint64_t) *(buf + 7);
    return buf + 8;
}

unsigned char* rm_deserialize_size_t(unsigned char *buf, size_t *v) {
    if (sizeof(size_t) == 4) {
        buf = rm_deserialize_u32(buf, (uint32_t*)v);
        return buf;
    } else if (sizeof(size_t) == 8) {
        *v = 0;
        *v =  ((uint64_t) *(buf + 0) << (24 + 32));
        *v += ((uint64_t) *(buf + 1) << (16 + 32));
        *v += ((uint64_t) *(buf + 2) << (8 + 32));
        *v += ((uint64_t) *(buf + 3) << 32);
        *v += ((uint64_t) *(buf + 4) << 24);
        *v += ((uint64_t) *(buf + 5) << 16);
        *v += ((uint64_t) *(buf + 6) << 8);
        *v += (uint64_t) *(buf + 7);
        return buf + 8;
    } else {
        assert(0 && "Unsuported architecture!");
    }
}

unsigned char* rm_deserialize_string(const void *buf, char *dst, size_t bytes_n) {
    if (dst == NULL) {
        return (unsigned char*)buf;
    }
    strncpy(dst, (char*)buf, bytes_n);
    dst[bytes_n - 1] = '\0';
    return (((unsigned char*) buf) + bytes_n);
}

unsigned char* rm_deserialize_mem(const void *buf, void *dst, size_t bytes_n) {
    if (dst == NULL) {
        return (unsigned char*)buf;
    }
    memcpy(dst, buf, bytes_n);
    return (((unsigned char*) buf) + bytes_n);
}

unsigned char* rm_deserialize_msg_hdr(unsigned char *buf, struct rm_msg_hdr *hdr) {
    buf = rm_deserialize_u32(buf, &(hdr->hash));
    buf = rm_deserialize_u8(buf, &(hdr->pt));
    buf = rm_deserialize_u8(buf, &(hdr->flags));
    buf = rm_deserialize_u16(buf, &(hdr->len));
    return buf;
}

unsigned char* rm_deserialize_msg_push_body(unsigned char *buf, struct rm_msg_push *m) {
    buf = rm_deserialize_mem(buf, (char*)m->ssid, sizeof(m->ssid));
    buf = rm_deserialize_u64(buf, (uint64_t*) &m->L);
    buf = rm_deserialize_u16(buf, &m->x_sz);
    if (m->x_sz > 0) {
        buf =rm_deserialize_string(buf, m->x, rm_min(m->x_sz, sizeof(m->x)));
    }
    buf = rm_deserialize_u16(buf, &m->y_sz);
    if (m->y_sz > 0) {
        buf = rm_deserialize_string(buf, m->y, rm_min(m->y_sz, sizeof(m->y)));
    }
    buf = rm_deserialize_u16(buf, &m->z_sz);
    if (m->z_sz > 0) {
        buf = rm_deserialize_string(buf, m->z, rm_min(m->z_sz, sizeof(m->z)));
    }
    return buf;
}

/* *m takes ownership of hdr */
unsigned char* rm_deserialize_msg_push(unsigned char *buf, struct rm_msg_hdr *hdr, struct rm_msg_push **m) {
    (*m)->hdr = hdr;
    buf = rm_deserialize_msg_push_body(buf, *m);
    return buf;
}

unsigned char* rm_deserialize_msg_ack(unsigned char *buf, struct rm_msg_ack *ack) {
    return rm_deserialize_msg_hdr(buf, ack->hdr);
}

struct rm_msg* rm_deserialize_msg(enum rm_pt_type pt, struct rm_msg_hdr *hdr, unsigned char *body_raw) {
    void *m = NULL;

    switch (pt) {

        case RM_PT_MSG_PUSH:

            m = malloc(sizeof(struct rm_msg_push));                     /* memory for the message struct is allocated here */
            if (m == NULL) {
                return NULL;
            }
            memset(m, 0, sizeof(struct rm_msg_push));
            rm_deserialize_msg_push(body_raw, hdr, (struct rm_msg_push**) &m);
            break;

        case RM_PT_MSG_PULL:
        default:
            return NULL;

    }
    return m;
}

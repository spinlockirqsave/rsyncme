/// @file	rm_serialize.h
/// @brief	Serialize TCP messages.
/// @author	peterg
/// @version	0.1.2
/// @date	03 Nov 2016 01:56 PM
/// @copyright	LGPLv2.1


#ifndef RSYNCME_SERIALIZE_H
#define RSYNCME_SERIALIZE_H


#include "rm_defs.h"
#include "rm_do_msg.h"


unsigned char *
rm_serialize_u32(unsigned char *buf, uint32_t v);

unsigned char *
rm_serialize_char(unsigned char *buf, char v);


unsigned char *
rm_serialize_u8(unsigned char *buf, uint8_t v);


unsigned char *
rm_serialize_u16(unsigned char *buf, uint16_t v);

unsigned char *
rm_serialize_msg_hdr(unsigned char *buf,
			struct rm_msg_hdr *h);

unsigned char *
rm_serialize_msg_push(unsigned char *buf,
			struct rm_msg_push *m);

unsigned char *
rm_serialize_msg_pull(unsigned char *buf,
			struct rm_msg_pull *m);

uint32_t
rm_msg_hdr_hash(unsigned char *buf);

#define RM_MSG_HDR_HASH(b) ntohl(*((uint32_t *)b))

uint8_t
rm_msg_hdr_pt(unsigned char *buf);

uint8_t
rm_msg_hdr_x(unsigned char *buf);

uint8_t
rm_msg_hdr_y(unsigned char *buf);

uint8_t
rm_msg_hdr_z(unsigned char *buf);


#endif	// RSYNCME_SERIALIZE_H

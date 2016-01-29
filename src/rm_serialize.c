/// @file	rm_serialize.c
/// @brief	Serialize TCP messages.
/// @author	Piotr Gregor piotrek.gregor at gmail.com
/// @version	0.1.2
/// @date	03 Nov 2016 01:56 PM
/// @copyright	LGPLv2.1


#include "rm_serialize.h"


unsigned char *
rm_serialize_u32(unsigned char *buf, uint32_t v)
{
	// write big-endian int value into buffer
	// assumes 32-bit int and 8-bit char.
	buf[0] = v >> 24;
	buf[1] = v >> 16;
	buf[2] = v >> 8;
	buf[3] = v;
	return buf + 4;
}

unsigned char *
rm_serialize_char(unsigned char *buf, char v)
{
	buf[0] = v;
	return buf + 1;
}

unsigned char *
rm_serialize_u8(unsigned char *buf, uint8_t v)
{
	buf[0] = v;
	return buf + 1;
}

unsigned char *
rm_serialize_u16(unsigned char *buf, uint16_t v)
{
	buf[0] = v >> 8;
	buf[1] = v;
	return buf + 2;
}

unsigned char *
rm_serialize_msg_hdr(unsigned char *buf,
			struct rm_msg_hdr *h)
{
	assert(buf != NULL);
	assert(h != NULL);

	buf = rm_serialize_u32(buf, h->hash);
	buf = rm_serialize_u8(buf, h->x);
	buf = rm_serialize_u8(buf, h->y);
	buf = rm_serialize_u8(buf, h->z);
	return buf;
}

unsigned char *
rm_serialize_msg_push(unsigned char *buf,
			struct rm_msg_push *m)
{
	assert(buf != NULL);
	assert(m != NULL);

	buf = rm_serialize_msg_hdr(buf, &m->hdr);
	buf = rm_serialize_u32(buf, m->L);
	return buf;
}

unsigned char *
rm_serialize_msg_pull(unsigned char *buf,
			struct rm_msg_pull *m)
{
	assert(buf != NULL);
	assert(m != NULL);

	buf = rm_serialize_msg_hdr(buf, &m->hdr);
	buf = rm_serialize_u32(buf, m->L);
	buf = rm_serialize_u32(buf, m->ch_ch_n);
	return buf;
}

uint32_t
rm_msg_hdr_hash(unsigned char *buf)
{
	uint32_t *h = (uint32_t *) buf;
	return ntohl(*h);
}

uint8_t
rm_msg_hdr_pt(unsigned char *buf)
{
	uint8_t *h = (uint8_t *) buf;
	return *(h + 4);
}

uint8_t
rm_msg_hdr_x(unsigned char *buf)
{
	uint8_t *h = (uint8_t *) buf;
	return *(h + 5);
}

uint8_t
rm_msg_hdr_y(unsigned char *buf)
{
	uint8_t *h = (uint8_t *) buf;
	return *(h + 6);
}

uint8_t
rm_msg_hdr_z(unsigned char *buf)
{
	uint8_t *h = (uint8_t *) buf;
	return *(h + 7);
}

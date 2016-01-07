/// @file	rm_do_msg.h
/// @brief	Execute TCP messages.
/// @author	peterg
/// @version	0.1.1
/// @date	02 Nov 2016 04:29 PM
/// @copyright	LGPLv2.1


#ifndef RSYNCME_DO_MSG_H
#define RSYNCME_DO_MSG_H


#include "rm_defs.h"
#include "rm_session.h"

struct rm_msg_hdr
{
	uint32_t	hash;                           // security token
	uint8_t		pt;                             // payload type
	uint8_t		x;                              // unused yet
	uint8_t		y;                              // unused yet
	uint8_t		z;                              // unused yet
};

struct rm_msg_push
{
        struct rm_msg_hdr	hdr;			// header
        uint32_t		L;			// block size	
};

struct rm_msg_pull
{
        struct rm_msg_hdr	hdr;			// header
        uint32_t		L;			// block size
	uint32_t		ch_ch_n;		// number of elements in the ch_ch list,
							// that follows this msg, ch_ch elements
							// are being sent in chunks while computing
							// hashes on file
};

struct rsyncme;

/// @brief	Handles incoming rsync push request in new sesion.
/// @details	Daemon's message.
int
rm_do_msg_push_in(struct rsyncme* rm,
		unsigned char *buf);

/// @brief	Makes outgoing rsync push request.
/// @details	rsyncme program message.
int
rm_do_msg_push_out(struct rsyncme* rm,
		unsigned char *buf);

/// @brief	Handles incoming rsync pull request in new sesion.
/// @details	Daemon's message.
int
rm_do_msg_pull_in(struct rsyncme* rm,
		unsigned char *buf);

/// @brief	Makes outgoing rsync pull request.
/// @details	rsyncme program message.
int
rm_do_msg_pull_out(struct rsyncme* rm,
		unsigned char *buf);


#endif  // RSYNCME_DO_MSG


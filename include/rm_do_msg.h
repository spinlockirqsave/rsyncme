/// @file      rm_do_msg.h
/// @brief     Execute TCP messages.
/// @author    peterg
/// @version   0.1.1
/// @date      02 Nov 2016 04:29 PM
/// @copyright LGPLv2.1

#ifndef RSYNCME_DO_MSG_H
#define RSYNCME_DO_MSG_H


struct rm_msg_hdr
{
	uint32_t	hash;                           // security token
	uint8_t		pt;                             // payload type
	uint8_t		x;                              // unused yet
	uint8_t		y;                              // unused yet
	uint8_t		z;                              // unused yet
};

struct rm_msg_session_add
{
        struct rm_msg_hdr	hdr;                            // header
        uint32_t		session_id;                     // id into hashtable
};


int
rm_do_msg_session_add(struct rsyncme* rm,
                        struct rm_msg_session_add *m,
                                        int read_n);


#endif  // RSYNCME_DO_MSG


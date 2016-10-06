/*  @file       rm_config.h
 *  @brief      Configuration.
 *  @author     Piotr Gregor <piotrgregor@rsyncme.org>
 *  @date       02 Jan 2016 02:38 PM
 *  @copyright  LGPLv2.1 */


#ifndef RSYNCME_CONFIG_H
#define RSYNCME_CONFIG_H


#define RM_SERVER_PORT			5556	    /* control */
#define RM_SERVER_LISTENQ		25         /* server's awaiting connections max, note: it is just a hint to the kernel */
#define RM_TCP_MSG_MAX_LEN		256         /* maximum length of TCP control message */
#define RM_IP_AUTH              "127.0.0.1" /* authorized IP, requests from which will be processed */
#define RM_DEFAULT_PORT         5048u       /* default daemon's port */


#endif /* RSYNCME_CONFIG_H */


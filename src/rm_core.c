///  @file      rm_core.c
///  @brief     Daemon's start up.
///  @author    peterg
///  @version   0.1.2
///  @date      02 Jan 2016 02:50 PM
///  @copyright LGPLv2.1


#include "rm_core.h"
#include "rm_do_msg.h"
#include "twlist.h"


int
rm_core_init(struct rsyncme *rm)
{
        assert(rm != NULL);
        memset(rm, 0, sizeof *rm);
        twhash_init(rm->sessions);
        TWINIT_LIST_HEAD(&rm->sessions_list);
        rm->state = RM_CORE_ST_INITIALIZED;
        return 0;
}

struct rm_session *
rm_core_session_find(struct rsyncme *rm,
                        uint32_t session_id)
{
        struct rm_session	*s;
        uint32_t                h;

        assert(rm != NULL);
        pthread_mutex_lock(&rm->mutex);
        h = twhash_min(session_id, RM_SESSION_HASH_BITS);
        twhlist_for_each_entry(s, &rm->sessions[h], hlink)
        {
                if (s->session_id == session_id)
                {
                        pthread_mutex_lock(&s->session_mutex);
                        return s;
                }
        }
        pthread_mutex_unlock(&rm->mutex);
        return NULL;
}

struct rm_session *
rm_core_session_add(struct rsyncme *rm)
{
        struct rm_session	*s = NULL;

        assert(rm != NULL);

        s = rm_session_create(rm);
        if (s == NULL)
                return NULL;
	pthread_mutex_lock(&rm->mutex);
	// create SID
	s->session_id = rm->sessions_n + 1;
	twlist_add(&rm->sessions_list, &s->link);
	twhash_add(rm->sessions, &s->hlink, s->session_id);
	rm->sessions_n++;
	pthread_mutex_unlock(&rm->mutex);

        return s;
}

int
rm_core_session_stop(struct rm_session *s)
{
        assert((s != NULL) && (s->rm != NULL));
        pthread_mutex_lock(&s->rm->mutex);
        twlist_del(&s->link);
        twhash_del(&s->hlink);
        s->rm->sessions_n--;
        pthread_mutex_unlock(&s->rm->mutex);
        return 0;
}

int
rm_core_authenticate(struct sockaddr_in *cli_addr)
{
        char a[INET_ADDRSTRLEN];
	// IPv4
	//cli_ip = cli_addr.sin_addr.s_addr;
	//inet_ntop(AF_INET, &cli_ip, cli_ip_str, INET_ADDRSTRLEN);
	//cli_port = ntohs(cli_addr.sin_port);
        inet_ntop(AF_INET, &cli_addr->sin_addr, a, INET_ADDRSTRLEN);
        if (strcmp(a, RM_IP_AUTH) != 0)
                return -1;
        return 0;
}

int
rm_core_tcp_msg_valid_pt(uint8_t pt)
{
	return (pt == RM_MSG_PUSH || pt == RM_MSG_PULL ||
		pt == RM_MSG_BYE);
}

int
rm_core_tcp_msg_validate(unsigned char *buf, int read_n)
{
	uint32_t	hash;
	uint8_t		pt;

        assert(buf != NULL && read_n >= 0);
        if (read_n == 0)
                rm_perr_abort("TCP message size is 0");
	// validate hash
	hash = RM_MSG_HDR_HASH(buf);
	if (hash != RM_CORE_HASH_OK)
	{
		RM_LOG_ERR("incorrect hash");
		return -1;
	}
	pt = rm_msg_hdr_pt(buf);
	if (rm_core_tcp_msg_valid_pt(pt) == 0)
		return -1;
        return pt;
}

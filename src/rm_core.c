///  @file      rm_core.c
///  @brief     Deamon's start up.
///  @author    peterg
///  @version   0.1.1
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

int
rm_core_session_start(struct rsyncme *rm,
                        uint32_t session_id,
                        void *(*f)(void*))
{
        int                     err;
        pthread_attr_t          attr;

        struct rm_session	*s = NULL;

        assert(rm != NULL);

        err = pthread_attr_init(&attr);
        if (err != 0)
                return -1;
        err = pthread_attr_setdetachstate(&attr,
                        PTHREAD_CREATE_JOINABLE);
        if (err != 0)
                goto fail;
        s = rm_session_create(session_id, rm);
        if (s == NULL)
                goto fail;

        err = pthread_create(&s->tid, &attr,
                          f, (void*)s);
	if (err != 0)
                goto fail;

	pthread_mutex_lock(&rm->mutex);
	twlist_add(&rm->sessions_list, &s->link);
	twhash_add(rm->sessions, &s->hlink, session_id);
	rm->sessions_n++;
	pthread_mutex_unlock(&rm->mutex);

        return 0;

fail:
        pthread_attr_destroy(&attr);
	if (s != NULL)
		rm_session_free(s);

        return -1;
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
        inet_ntop(AF_INET, &cli_addr->sin_addr, a, INET_ADDRSTRLEN);
        if (strcmp(a, RM_IP_AUTH) != 0)
                return -1;
        return 0;
}

int
rm_core_tcp_msg_validate(unsigned char *buf, int read_n)
{
        assert(buf != NULL && read_n >= 0);
        if (read_n == 0)
                rm_perr_abort("TCP message size is 0");

        return 0;
}

/// @brief      Process TCP connection's events.
/// @details    Thread routine.
static void *
rm_core_proc_con_events(void *data)
{
        // TODO implement
        struct rsyncme	*rm;
        int		connfd;
        struct rm_core_con_data	*con_data;
        int		read_n, err;
        unsigned char	buf[RM_TCP_MSG_MAX_LEN];

        assert(data != NULL);
        con_data = (struct rm_core_con_data *) data;
        rm = con_data->rm;
        connfd = con_data->connfd;

	for ( ; ; )
        {
                memset(&buf, 0, sizeof buf);
                read_n = read(connfd, &buf, RM_TCP_MSG_MAX_LEN);
                if (read_n == -1)
                {
                        switch (read_n)
                        {
                                case EAGAIN:
                                        rm_perr_abort("Nonblocking I/O requested"
                                                " on TCP control socket");
                                case EINTR:
                                        rm_err("TCP control socket: interrupted");
                                        continue;
                                case EBADF:
                                        rm_perr_abort("TCP control socket passed "
                                                        "is not a valid descriptor "
                                                        "or nor open for reading");
                                case EFAULT:
                                        rm_perr_abort("TCP control socket: buffer"
                                                       " is outside accessible "
                                                       "address space");
                                case EINVAL:
                                        rm_perr_abort("TCP control socket: unsuitable "
                                                        "for reading or wrong buffer len");
                                case EIO:
                                        rm_perr_abort("TCP control socket: I/O error");
                                case EISDIR:
                                        rm_perr_abort("TCP control socket: socket "
                                                        "descriptor refers to directory");
                                default:
                                        rm_perr_abort("TCP control socket: "
                                                        "unknown error");
                        }
                }
                // validate the message: do we understand that?
                err = rm_core_tcp_msg_validate(buf, read_n);
                if (err < 0)
                {
                        rm_err("TCP control socket: not a valid rsyncme message");
                        continue;
                }
                // process server message
                switch (err)
                {
                        case RM_MSG_SESSION_ADD:
                                pthread_mutex_lock(&rm->mutex);
                                rm_do_msg_session_add(rm,
                                        (struct rm_msg_session_add *) buf, read_n);
                                pthread_mutex_unlock(&rm->mutex);
                                break;
                        case RM_MSG_BYE:
                                break;
                        default:
                                rm_perr_abort("Unknown TCP message type");
                }
        }
        return 0;
}

int
rm_core_process_connection(struct rsyncme* rm, int connfd)
{
        struct rm_core_con	*c = NULL;
        pthread_attr_t		attr;
        pthread_attr_t		*attrp; // NULL or init &attr
        int			err;

        attrp = NULL;
        pthread_mutex_lock(&rm->mutex);
        if (rm->connections_n == RM_CORE_CONNECTIONS_MAX)
        {
                err = -1;
                goto fail;
        }
        c = malloc(sizeof *c);
        if (c == NULL)
        {
                err = -2;
                goto fail;
        }

        // start the thread
        err = pthread_attr_init(&attr);
        if (err != 0)
        {
                RM_ERR("pthread_attr_init failed");
                err = -3;
                goto fail;
        }
        attrp = &attr;

        err = pthread_attr_setdetachstate(&attr,
                                PTHREAD_CREATE_JOINABLE);
        if (err != 0)
        {
                RM_ERR("pthread_attr_setdetachstate failed");
                err = -3;
                goto fail;
        }

        err = pthread_attr_init(&attr);
        if (err != 0)
        {
                RM_ERR("pthread_attr_init failed");
                err = -3;
                goto fail;
        }

        err = pthread_create(&c->tid, &attr,
                rm_core_proc_con_events, c);
        if (err != 0)
        {
                RM_ERR("pthread_create failed");
                err = -3;
                goto fail;
        }

        c->connfd = connfd;

        // add to the list of connections
        twlist_add(&c->link, &rm->connections);
        rm->connections_n++;
        pthread_mutex_unlock(&rm->mutex);
        return 0;

fail:
        pthread_mutex_unlock(&rm->mutex);
        if (c != NULL)
                free(c);
        if (attrp)
                pthread_attr_destroy(attrp);

        return err;
}


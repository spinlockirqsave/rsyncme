/*  @file       rm_session.c
 *  @brief      Rsync session.
 *  @author     Piotr Gregor <piotrek.gregor at gmail.com>
 *  @version    0.1.2
 *  @date       02 Nov 2016 04:08 PM
 *  @copyright  LGPLv2.1
 */


#include "rm_session.h"


/* TODO: generate GUID here */
struct rm_session *
rm_session_create(struct rsyncme *rm, enum rm_session_type t)
{
	struct rm_session *s;

	s = malloc(sizeof *s);
	if (s == NULL)
		return NULL;
	memset(s, 0, sizeof(*s));
	pthread_mutex_init(&s->session_mutex, NULL);
	pthread_mutex_init(&s->rx_ch_ch_list_mutex, NULL);
    TWINIT_LIST_HEAD(&s->rx_ch_ch_list);
	s->rm = rm;

    switch (t)
    {
        case RM_PUSH_RX:
            s->prvt = malloc(sizeof(struct rm_session_push_rx));
            if (s->prvt == NULL)
                goto fail;
            break;
        case RM_PULL_RX:
            s->prvt = malloc(sizeof(struct rm_session_pull_rx));
            if (s->prvt == NULL)
                goto fail;
            break;
        default:
            goto fail;
    }

    return s;

fail:
    if (s->prvt)
        free(s->prvt);
    pthread_mutex_destroy(&s->session_mutex);
    pthread_mutex_destroy(&s->rx_ch_ch_list_mutex);
    free(s);
    return NULL;
}

void
rm_session_free(struct rm_session *s)
{
	assert(s != NULL);
	pthread_mutex_destroy(&s->session_mutex);
	free(s);
}

void *
rm_session_ch_ch_rx_f(void *arg)
{
	struct rm_session *s =
			(struct rm_session *)arg;
	assert(s != NULL);
	return 0;
}

void *
rm_session_delta_tx_f(void *arg)
{
	struct rm_session *s =
			(struct rm_session *)arg;
	assert(s != NULL);
	return 0;
}

void *
rm_session_ch_ch_tx_f(void *arg)
{
	struct rm_session *s =
			(struct rm_session *)arg;
	assert(s != NULL);
	return 0;
}

void *
rm_session_delta_rx_f(void *arg)
{
	struct rm_session *s =
			(struct rm_session *)arg;
	assert(s != NULL);
	return 0;
}

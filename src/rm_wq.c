/* @file	    rm_wq.c
 * @brief	    Workqueue.
 * @author	    Piotr Gregor <piotrgregor@rsyncme.org>
 * @date	    02 Jan 2016 02:50 PM
 * @copyright	LGPLv2.1 */


#include "rm_wq.h"


static void*
rm_wq_worker_f(void *arg) {
    twfifo_queue            *q;
    const struct rm_work    *work;     /* iterator over enqueued work elements */
    struct twlist_head      *lh;

    struct rm_worker *w = (struct rm_worker*) arg;

    pthread_mutex_lock(&w->mutex);   /* sleep on the queue and process queued work element once awoken */
    q = &w->queue;

    while (w->active == 1) {
        for (twfifo_dequeue(q, lh); lh != NULL; twfifo_dequeue(q, lh)) {
            work = tw_container_of(lh, struct rm_work, link);
            pthread_mutex_unlock(&w->mutex);    /* allow for further enquing while work is being processed */
            work->f(work->data);
        }
        pthread_mutex_lock(&w->mutex);
        pthread_cond_wait(&w->signal, &w->mutex);
    }
    pthread_mutex_unlock(&w->mutex);
    return NULL;
}

static void
rm_wq_worker_init(struct rm_worker *w, struct rm_workqueue *wq) {
    pthread_mutex_init(&w->mutex, NULL);
    w->active = 0;
    w->wq = wq;
    TWINIT_LIST_HEAD(&w->queue);
    pthread_cond_init(&w->signal, NULL);
}

static void
rm_wq_worker_deinit(struct rm_worker *w) {
    assert(twlist_empty(&w->queue) != 0 && "Queue NOT EMPTY!\n");
    if (twlist_empty(&w->queue) == 0) {
        RM_LOG_CRIT("QUEUE NOT EMPTY, worker [%u]", w->idx);
    }
    pthread_mutex_destroy(&w->mutex);
    pthread_cond_destroy(&w->signal);
}

enum rm_error
rm_wq_workqueue_init(struct rm_workqueue *wq, uint32_t workers_n, const char *name) {
    struct rm_worker    *w;
    uint8_t             first_active_set = 0;

    memset(wq, 0, sizeof(*wq));
    wq->workers = malloc(workers_n * sizeof(struct rm_worker));
    if (wq->workers == NULL) {
        return RM_ERR_MEM;
    }

    if (workers_n > 0) {
        wq->workers_active_n = 0;
        while (workers_n) {
            --workers_n;
            w = &wq->workers[workers_n];
            rm_wq_worker_init(w, wq);
            w->idx = workers_n;
            if (rm_launch_thread(&w->tid, rm_wq_worker_f, w, PTHREAD_CREATE_JOINABLE) == RM_ERR_OK) {
                w->active = 1;
                wq->workers_active_n++;    /* increase the number of active/running workers */
                if (first_active_set == 0) {
                    wq->first_active_worker_idx = w->idx;
                    first_active_set = 1;
                }
            }
        }
    }

    wq->name = strdup(name);
    wq->running = 1;
    wq->next_worker_idx_to_use = 0;
    if (wq->workers_n > 0) { /* if we have at least one worker thread then queue creation was successful */
        return RM_ERR_OK;
    } else {
        return RM_ERR_WORKQUEUE_CREATE;
    }
}

static void
rm_wq_workqueue_deinit(struct rm_workqueue *wq) {
    struct rm_worker    *w;
    uint32_t            workers_n = wq->workers_n;

    free((void*)wq->name);
    while (workers_n) {
        --workers_n;
        w = &wq->workers[workers_n];
        rm_wq_worker_deinit(w);
    }
    free(wq->workers);
}

void
rm_wq_workqueue_free(struct rm_workqueue *wq) {
    /* queue MUST be empty now */
    rm_wq_workqueue_deinit(wq);
    free(wq);
    return;
}

struct rm_workqueue*
rm_wq_workqueue_create(uint32_t workers_n, const char *name) {
    enum rm_error   err;
    struct rm_workqueue* wq;
    wq = malloc(sizeof(*wq));
    if (wq == NULL) {
        return NULL;
    }
    err = rm_wq_workqueue_init(wq, workers_n, name);
    if (err  != RM_ERR_OK) {
        switch (err) {

            case RM_ERR_MEM:
                free(wq);
                return NULL;

            case RM_ERR_WORKQUEUE_CREATE:
                rm_wq_workqueue_free(wq);
                return NULL;

            default:
                break;
        }
    }
    return wq;
}

/*  @brief  Work dispatcher (round-robin). */
void
rm_wq_queue_work(struct rm_workqueue *wq, struct rm_work* work) {
    struct rm_worker    *w;
    uint8_t             idx = wq->next_worker_idx_to_use, sanity = 0xFF;

    if (wq->workers_active_n == 0) {
        RM_LOG_CRIT("NO ACTIVE WORKER THREAD in the workqueue [%s]", wq->name);
    }
    assert(wq->workers_active_n > 0 && "NO ACTIVE THREAD in the workqueue");

    if (wq->workers_active_n > 1) { /* get next worker */
        do {
            w = &wq->workers[idx];
            idx = (idx + 1) % wq->workers_n;
        } while (w->active == 0 && (--sanity));
    } else {    /* there is only one active thread in the workers table */
        w = &wq->workers[wq->first_active_worker_idx];
    }
    wq->next_worker_idx_to_use = idx;
    pthread_mutex_lock(&w->mutex);    /* enqueue work (and move ownership to worker!) */
    twfifo_enqueue(&work->link, &w->queue);
    pthread_cond_signal(&w->signal);
    pthread_mutex_unlock(&w->mutex);
    return;
}

/* TODO */
void
rm_wq_queue_delayed_work(struct rm_workqueue *q, struct rm_work* work, unsigned int delay) {
    (void)q;
    (void)work;
    (void)delay;
    return;
}

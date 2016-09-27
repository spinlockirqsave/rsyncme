/* @file        rm_wq.h
 * @brief       Workqueue.
 * @author      Piotr Gregor <piotrgregor@rsyncme.org>
 * @date        06 Sep 2016 03:24 PM
 * @copyright   LGPLv2.1 */


#ifndef RSYNCME_WORKQUEUE_H
#define RSYNCME_WORKQUEUE_H


#include "rm.h"
#include "rm_error.h"
#include "rm_do_msg.h"

#include "twlist.h"


enum rm_work_type {
    RM_WORK_PROCESS_MSG_PUSH,
    RM_WORK_PROCESS_MSG_PULL,
    RM_WORK_PROCESS_MSG_BYE
};

struct rm_worker {              /* thread wrapper */
    uint8_t         idx;        /* index in workqueue table */
    pthread_t       tid;
    twfifo_queue    queue;      /* queue of work structs */
    pthread_mutex_t mutex;
    pthread_cond_t  signal;     /* signaled when new item is enqueued to this worker's queue */
    uint8_t         active;     /* 0 - no, 1 - yes (successfuly created and waiting for work) */
    struct rm_workqueue *wq;    /* owner */
};

struct rm_workqueue {
    struct rm_worker    *workers;
    uint8_t             workers_n;          /* number of worker threads */
    uint32_t            workers_active_n;   /* number of active/running worker threads (successfuly created and waiting for work) */
    const char          *name;
    uint8_t             running;            /* 0 - no, 1 - yes */
    uint8_t             first_active_worker_idx;
    uint8_t             next_worker_idx_to_use; /* index of next worker to use for enquing the work in round-robin fashion */
};

/* @brief   Start the worker threads.
 * @details After this returns the @workers_n variable in workqueue is set to the numbers of successfuly created
 *          and now running threads. It isn't neccessary the same number that has been passed to this function. */
enum rm_error
rm_wq_workqueue_init(struct rm_workqueue *q, uint32_t workers_n, const char *name);

struct rm_workqueue*
rm_wq_workqueue_create(uint32_t workers_n, const char *name);

struct rm_work {
    struct twlist_head  link;
    enum rm_work_type   task;
    struct rsyncme      *rm;
    struct rm_msg       *msg;               /* message handle */
    void* (*f)(void*);
};

#define RM_WORK_INITIALIZER(n, d, f) {      \
    .link  = { &(n).link, &(n).link },      \
    .data = (d),                            \
    .func = (f),                            \
}

#define DECLARE_WORK(n, d, f) \
    struct work_struct n = RM_WORK_INITIALIZER(n, d, f)

struct rm_work*
rm_work_init(struct rm_work* work, enum rm_work_type task, struct rsyncme* rm, struct rm_msg* msg, void*(*f)(void*));

struct rm_work*
rm_work_create(enum rm_work_type task, struct rsyncme* rm, struct rm_msg* msg, void*(*f)(void*));

void
rm_work_free(struct rm_work* work);

void
rm_wq_queue_work(struct rm_workqueue *q, struct rm_work* work);

void
rm_wq_queue_delayed_work(struct rm_workqueue *q, struct rm_work* work, unsigned int delay);


#endif  /* RSYNCME_WORKQUEUE_H */


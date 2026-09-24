#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "msgblock.h"
#include "serialqueue.h"
#include "pyhelper.h"

struct command_queue {
    int dummy;
};

struct serialqueue {
    int dummy;
};

struct queue_message *message_alloc(void)
{
    return calloc(1, sizeof(struct queue_message));
}

struct queue_message *message_fill(uint8_t *data, int len)
{
    struct queue_message *qm = message_alloc();
    qm->len = len;
    if (len > 0 && data)
        memcpy(qm->msg, data, len > MESSAGE_MAX ? MESSAGE_MAX : len);
    return qm;
}

struct queue_message *message_alloc_and_encode(uint32_t *data, int len)
{
    struct queue_message *qm = message_alloc();
    qm->len = len > MESSAGE_MAX ? MESSAGE_MAX : len;
    if (data && len > 0)
        memcpy(qm->msg, data, qm->len * sizeof(uint8_t));
    return qm;
}

void message_free(struct queue_message *qm)
{
    free(qm);
}

void message_queue_free(struct list_head *root)
{
    while (!list_empty(root)) {
        struct queue_message *qm = list_first_entry(root, struct queue_message, node);
        list_del(&qm->node);
        message_free(qm);
    }
}

struct command_queue *serialqueue_alloc_commandqueue(void)
{
    return calloc(1, sizeof(struct command_queue));
}

void serialqueue_free_commandqueue(struct command_queue *cq)
{
    free(cq);
}

void serialqueue_send_batch(struct serialqueue *sq, struct command_queue *cq
                            , struct list_head *msgs)
{
    (void)sq;
    (void)cq;
    while (!list_empty(msgs)) {
        struct queue_message *qm = list_first_entry(msgs, struct queue_message, node);
        list_del(&qm->node);
        message_free(qm);
    }
}

double get_monotonic(void)
{
    return 0.0;
}

struct timespec fill_time(double time)
{
    struct timespec ts;
    ts.tv_sec = (time_t)time;
    ts.tv_nsec = (long)((time - (double)ts.tv_sec) * 1000000000.0);
    return ts;
}

void set_python_logging_callback(void (*func)(const char *))
{
    (void)func;
}

void errorf(const char *fmt, ...)
{
    (void)fmt;
}

void report_errno(char *where, int rc)
{
    (void)where;
    (void)rc;
}

char *dump_string(char *outbuf, int outbuf_size, char *inbuf, int inbuf_size)
{
    int n = inbuf_size < outbuf_size - 1 ? inbuf_size : outbuf_size - 1;
    memcpy(outbuf, inbuf, n);
    outbuf[n] = '\0';
    return outbuf;
}

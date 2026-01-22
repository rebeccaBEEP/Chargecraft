#ifndef DS_QUEUE_H
#define DS_QUEUE_H

typedef struct QNode {
    void* data;
    struct QNode* next;
} QNode;
typedef struct Queue { QNode* head; QNode* tail; } Queue;

void q_init(Queue* q);                  /* O(1) */
int  q_is_empty(Queue* q);              /* O(1) */
int  q_enqueue(Queue* q, void* data);   /* O(1) */
int  q_dequeue(Queue* q, void** out);   /* O(1) */
void q_clear(Queue* q);                 /* O(n) */

#endif

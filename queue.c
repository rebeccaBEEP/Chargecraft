#include "queue.h"
#include <stdlib.h>
#include <stdio.h>

void q_init(Queue* q){ q->head=q->tail=0; }
int  q_is_empty(Queue* q){ return q->head==0; }

int  q_enqueue(Queue* q, void* data){
    QNode* n = malloc(sizeof *n);
    if (!n) return 0;

    n->data = data;
    n->next = NULL;

    if (!q->tail)
        q->head = q->tail = n;
    else {
        q->tail->next = n;
        q->tail = n;
    }

    return 1;
}
int  q_dequeue(Queue* q, void** out){
    if (!q->head) return 0;

    QNode* h = q->head;
    if (out) *out = h->data;

    q->head = h->next;
    if (!q->head) q->tail = NULL;

    free(h);

    return 1;
}

void q_clear(Queue* q){
    QNode* cur = q->head;
    while (cur) {
        QNode* next = cur->next;

        free(cur);
        printf("2 DEBUG: avant traitement événement\n");

        cur = next;
    }
    q->head = q->tail = NULL;
}

#include "queue.h"
#include <string.h>
#include <stdlib.h>

bool queue_is_empty(drv_queue_t *queue)
{
    return queue->length == 0;
}

void queue_pop(drv_queue_t *queue, drv_queue_node_t *node)
{
    mutex_enter_blocking(&queue->queue_lock);
    
    // Check to see if we have nodes in the queue
    if(!queue_is_empty(queue))
    {
        // Copy the data from the current node into the provided node
        drv_queue_node_t *start = queue->start;
        memcpy(node, start, sizeof(drv_queue_node_t));
        
        // Update the Queue as we are removing this node
        if(queue->length > 1)
            queue->start = start->next;
        else
        {
            queue->end = 0;
            queue->start = 0;
        }
        queue->length--;
        free(start);
    }
    mutex_exit(&queue->queue_lock);
}
void queue_peek(drv_queue_t *queue, drv_queue_node_t *node)
{
    // Probably could just return the direct pointer.

    mutex_enter_blocking(&queue->queue_lock);

    // Check to see if we have nodes in the queue
    if(!queue_is_empty(queue))
    {
        // Copy the data from the current node into the provided node
        memcpy(node, queue->start, sizeof(drv_queue_node_t));
    }
    mutex_exit(&queue->queue_lock);
}

void queue_push(drv_queue_t *queue, drv_queue_node_t* node)
{   
    mutex_enter_blocking(&queue->queue_lock);

    // Allocate Our own controlled memory
    drv_queue_node_t *new_end = (drv_queue_node_t *)malloc(sizeof(drv_queue_node_t));
    memcpy(new_end, node, sizeof(drv_queue_node_t));

    // Due to the fact they we are passing data and not pointers 
    // we need to know if the data is "valid" as we cannot compare to a nullptr
    new_end->initialized = true;

    // If no starting node exists this is the starting node
    if(!queue->start)
        queue->start = new_end;

    // If ending node exists this is the new ending node and old ending node knows that
    if(queue->end)
        queue->end->next = new_end;

    // Update State
    queue->end = new_end;
    queue->length++;

    mutex_exit(&queue->queue_lock);
}

void queue_init(drv_queue_t *queue)
{
    mutex_init(&queue->queue_lock);
    queue_enable(queue, true);
}

void queue_enable(drv_queue_t *queue, bool enable)
{
    mutex_enter_blocking(&queue->queue_lock);
    queue->running = enable;
    mutex_exit(&queue->queue_lock);
}
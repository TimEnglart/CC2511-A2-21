#ifndef QUEUE_H
#define QUEUE_H

#include <stdbool.h>
#include "pico/mutex.h"

typedef unsigned char uint8_t;
typedef unsigned long uint32_t;


// Queue Movements

typedef struct drv_queue_node_t {
    // Since Nodes will be statically allocated on the stack we cannot compare them to nullptrs;
    // So it will be hard to tell if it is valid
    bool initialized;
    // The next node in the queue
    struct drv_queue_node_t *next;

    /* Below properties are subject to change */

    // The amount of steps we want to take. 
    // The pico should calcuate the actual distance and let this be as "dumb" as possible
    uint32_t x_steps, y_steps, z_steps;
    // The Direction of the steps to perform on the axis
    bool x_dir, y_dir, z_dir;
    // The step mode for the steps
    bool mode_0, mode_1, mode_2;
} drv_queue_node_t;

typedef struct {
    drv_queue_node_t *start;
    drv_queue_node_t *end;
    uint32_t length;
    mutex_t queue_lock;
    bool running;
    bool processing;
} drv_queue_t;

// Checks to see if there are nodes in the queue
bool queue_is_empty(drv_queue_t* queue);
// Checks to see if there are nodes in the queue and locks mutex
bool queue_is_empty_mutex(drv_queue_t* queue);


// Return and remove the 1st node in the queue
void queue_pop(drv_queue_t* queue, drv_queue_node_t* node);
// Return the 1st node in the queue
void queue_peek(drv_queue_t* queue, drv_queue_node_t* node);
// Add a node to the end of the queue
void queue_push(drv_queue_t* queue, drv_queue_node_t* node);
// Initialse the Queue and its mutex lock
void queue_init(drv_queue_t* queue);
// Set Queue Running State. So we can run batches of nodes
void queue_enable(drv_queue_t* queue, bool enable);





#endif // QUEUE_H
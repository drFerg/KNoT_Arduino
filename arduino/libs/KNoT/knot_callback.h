/* author Fergus William Leahy */
#ifndef KNOT_CALLBACK_H
#define KNOT_CALLBACK_H

/* Applications must provide a function which expects:
 * - the name of the sending sensor
 * - data from the sensor
 */
typedef void (*knot_callback)(char name[],void * data);


typedef struct callback_control_block{
   knot_callback callback;
   struct process *client_process;
}CallbackControlBlock;





#endif /* KNOT_CALLBACK_H */
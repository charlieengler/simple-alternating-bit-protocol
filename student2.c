#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "project2.h"

// A timeout value of 500 seems to be the best compromise between reliability and speed
#define TIMEOUT 500

// Primitive definitions
int generate_checksum(struct pkt*);
void send_pkt(int);
void enqueue(struct pkt*);
struct pkt *dequeue();

// Global variable definitions
int A_sequence = 0;
int B_sequence = 0;
struct pkt last_pkt;

// Both a next and previous node are necessary in order to maintain FIFO functionality
// (If they aren't both needed, then I wasn't smart enough to think of another solution)
struct pkt_queue
{
    struct pkt *node_pkt;
    struct pkt_queue *previous;
    struct pkt_queue *next;
};

// Both a head and tail are definitiely necessary for maintaining FIFO functionality
struct pkt_queue *queue_head = NULL;
struct pkt_queue *queue_tail = NULL;

/*
 * Where message is a structure of type msg, containing data to be sent to the B-side. This routine
 * will be called whenever the upper layer at the sending side (A) has a message to send. It is the
 * job of your protocol to insure that the data in such a message is delivered in-order, and correctly,
 * to the receiving side upper layer.
*/
void A_output(struct msg message)
{
    struct pkt *new_pkt = malloc(sizeof(struct pkt));

    new_pkt->seqnum = A_sequence;
    // Alternate between 0 and 1 for the sequence number
    A_sequence = A_sequence == 0 ? 1 : 0;

    // Packet from A side has acknowledgment number of 0
    new_pkt->acknum = 0;

    for(int i = 0; i < MESSAGE_LENGTH; i++)
        new_pkt->payload[i] = message.data[i];

    // Checksum generated after payload inserted so payload is considered
    new_pkt->checksum = generate_checksum(new_pkt);

    // Add the new packet to the queue of packets to send
    enqueue(new_pkt);

    // Only send the packet if timer isn't running
    if(getTimerStatus(AEntity) == FALSE)
        send_pkt(AEntity);
}

/*
 * Just like A_output, but residing on the B side. USED only when the 
 * implementation is bi-directional.
 */
void B_output(struct msg message)
{

}

/* 
 * A_input(packet), where packet is a structure of type pkt. This routine 
 * will be called whenever a packet sent from the B-side (i.e., as a result 
 * of a tolayer3() being done by a B-side procedure) arrives at the A-side. 
 * packet is the (possibly corrupted) packet sent from the B-side.
 */
void A_input(struct pkt input_pkt)
{
    stopTimer(AEntity);

    // Resend the last packet if the ACK is corrupted
    int ack_checksum = generate_checksum(&input_pkt);
    if(ack_checksum != input_pkt.checksum)
    {
        // Send the packet
        tolayer3(AEntity, last_pkt);
        // Start the timer
        startTimer(AEntity, TIMEOUT);

        return;
    }

    // Received a NAK
    if(input_pkt.acknum == 0)
    {
        // Send the last packet if last sequence number is in NAK
        if(input_pkt.seqnum == last_pkt.seqnum)
        {
            // Send the packet
            tolayer3(AEntity, last_pkt);
            // Start the timer
            startTimer(AEntity, TIMEOUT);

            return;
        }
    }

    // If there are no problems with the ACK, send the next packet in the queue
    // Also send it if the NAK has the wrong sequence number
    send_pkt(AEntity);
}

/*
 * A_timerinterrupt()  This routine will be called when A's timer expires 
 * (thus generating a timer interrupt). You'll probably want to use this 
 * routine to control the retransmission of packets. See starttimer() 
 * and stoptimer() in the writeup for how the timer is started and stopped.
 */
void A_timerinterrupt()
{
    // Send the packet
    tolayer3(AEntity, last_pkt);
    // Start the timer
    startTimer(AEntity, TIMEOUT);
}  

/* The following routine will be called once (only) before any other    */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
    // I didn't need this method
}

/* 
 * Note that with simplex transfer from A-to-B, there is no routine B_output() 
 */

/*
 * B_input(packet),where packet is a structure of type pkt. This routine 
 * will be called whenever a packet sent from the A-side (i.e., as a result 
 * of a tolayer3() being done by a A-side procedure) arrives at the B-side. 
 * packet is the (possibly corrupted) packet sent from the A-side.
 */
void B_input(struct pkt input_pkt)
{
    // Debugging statements (couldn't access TraceLevel)
    // printf("Received Message Data: %s\n", input_pkt.payload);
    // printf("Received Message Checksum: %d\n", input_pkt.checksum);
    // printf("Received Message Seqnum: %d\n", input_pkt.seqnum);

    // Check to make sure the packet isn't corrupted and the sequence numbers correspond
    int pkt_checksum = generate_checksum(&input_pkt);
    if(pkt_checksum == input_pkt.checksum && B_sequence == input_pkt.seqnum)
    {
        // Alternate the sequence between 0 and 1
        B_sequence = B_sequence == 0 ? 1 : 0;

        // Generate a message to send to layer 5
        struct msg received_msg;

        for(int i = 0; i < MESSAGE_LENGTH; i++)
            received_msg.data[i] = input_pkt.payload[i];

        tolayer5(BEntity, received_msg);

        // Create a ACK message and send it back to A
        struct pkt ack;
        ack.seqnum = input_pkt.seqnum;
        ack.acknum = 1;
        ack.checksum = generate_checksum(&ack);

        tolayer3(BEntity, ack);
    }
    else
    {
        // Generate a NAK if the packet is corrupted and send it back to A
        struct pkt nak;
        nak.seqnum = B_sequence;
        nak.acknum = 0;
        nak.checksum = generate_checksum(&nak);

        tolayer3(BEntity, nak);
    }
}

/*
 * B_timerinterrupt()  This routine will be called when B's timer expires 
 * (thus generating a timer interrupt). You'll probably want to use this 
 * routine to control the retransmission of packets. See starttimer() 
 * and stoptimer() in the writeup for how the timer is started and stopped.
 */
void B_timerinterrupt()
{

}

/* 
 * The following routine will be called once (only) before any other   
 * entity B routines are called. You can use it to do any initialization 
 */
void B_init()
{
    // I didn't use this function
}

int generate_checksum(struct pkt *input_pkt)
{
    int checksum = 0;
    // Starts at one so seqnum isn't multiplied by zero
    int checksum_index = 1;

    // Add the sequence number plus one so seqnum zero isn't ignored
    checksum += (input_pkt->seqnum + 1) * checksum_index;
    checksum_index++;

    // Add the acknum plus two so acknum zero isn't ignored
    checksum += (input_pkt->acknum + 2) * checksum_index;
    checksum_index++;

    for(int i = 0; i < sizeof(input_pkt->payload)/sizeof(char); i++)
    {
        // Add the integer representation of each character times its index
        checksum += checksum_index * (int)input_pkt->payload[i];

        checksum_index++;
    }

    return checksum;
}

void send_pkt(int entity)
{
    struct pkt *input_pkt = dequeue();

    // Don't send anything if the queue is empty
    if(input_pkt == NULL)
        return;

    // Send the packet
    tolayer3(entity, *input_pkt);
    // Start the timer
    startTimer(entity, TIMEOUT);

    // Assign the last packet to the input packet
    last_pkt = *input_pkt;

    // Debugging statements (couldn't access TraceLevel)
    // printf("Sent Message Data: %s\n", input_pkt->payload);
    // printf("Sent Message Checksum: %d\n", input_pkt->checksum);
    // printf("Sent Message Seqnum: %d\n", input_pkt->seqnum);
}

void enqueue(struct pkt *input_pkt)
{
    // You can't add null to a queue
    if(input_pkt == NULL)
        return;

    // Allocate a new node
    struct pkt_queue *new_node = malloc(sizeof(struct pkt_queue));

    // Assign the input packet to the node's packet
    new_node->node_pkt = input_pkt;
    // At the top of queue, so it doesn't have any previous nodes
    new_node->previous = NULL;
    // Make the old head the next in queue
    new_node->next = queue_head;

    if(queue_head != NULL)
    {
        // Set the last head's previous to the new node (the new head)
        queue_head->previous = new_node;
    }

    // Make the new node the new head
    queue_head = new_node;

    if(queue_tail == NULL)
        queue_tail = queue_head;
}

struct pkt *dequeue()
{
    if(queue_tail == NULL)
        return NULL;

    // Extract the packet from the previous head to return
    struct pkt *return_pkt = queue_tail->node_pkt;

    // Pop the previous head off of the queue and set the next node as the head
    queue_tail = queue_tail->previous;

    return return_pkt;
}
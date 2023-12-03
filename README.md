# A simulation of the Alternating Bit Protocol written in C for CS 3516. All code written by me is within the student2.c file.

## Compilation:
    
  Enter the following command in the root directory:
  
  `make`

## Running the Program:
  Enter the following command in the root directory:
  
  `./p2 num_msgs, loss_prob, corrupt_prob, out_of_order_prob, avg_time, trace_lvl, randomized, bidirectional`

    num_msgs
        Number of messages to simulate [Integer, >2]

    loss_prob
        Probability that a packet will be lost on its way to the destination [Decimal, 0 to 1]
        
    corrupt_prob
        Probability that a packet will be corrupted on its way to the destination [Decimal, 0 to 1]

    out_of_order_prob
        Probability that packets will be sent in the incorrect order [Decimal, 0 to 1]

    avg_time
        Average time between messages from sender's layer 5 [Integer, >0]

    trace_lvl
        Level of debug messages printed to the output terminal [Integer, 1 to 4]

    randomized
        Randomizes the modification of packets [0 or 1]

    bidirectional
        Unidirectional (0) or bidirectional (1) behavior [0 or 1]
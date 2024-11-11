## CS118 Project 1

- In this project, I built a TCP layer ontop of UDP sockets.
- I did this by keeping both a sending and recieving buffer, and sending out acks for packets recieved.
- Any outgoing packets were put into a sending buffer (unless the buffer was full), and any acks recieved could remove those from the buffer.
- Any incoming packets were put into a recieving buffer (unless the packet was a dup), and periodically flushing the packets in order, by sequence number could remove those from the buffer.
- I choose a fixed-size array of 20 for both buffers. By the network sliding window invariant, both buffers can be the same size. This design decision was made so that I could keep my code simpler, and use array techniques such as bubble insertion and element removal to handle packets
- I ran into a number of problems with my project.
- The first was that my bytes recieved would always be 1 packet less than the packets sent to my client.
- I solved this by printing out each packet, and figuring out that the first packet was not being processed.
- I figured out that this was due to a faulty element removal algorithm, which I fixed.

- Overall, this was a tough project, but I learned alot.

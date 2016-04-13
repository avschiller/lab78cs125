But when you’re trying to do this file transfer across the country (or across the world), you’re not going to get <1ms RTTs, and one datagram at a time will be painfully slow.

In your readings you have seen several ways to take better advantage of the available bandwidth (Windows! Selective ACKs! Others!). Design a better -U scheme, and write it up the detailed design in a new file ReliableUDP.md . (Don’t forget to add this file to your repository!) This file should be self-contained (intelligible without reference to your code or other readings), and it should be clear how you are getting better performance while maintaining reliability.


Create an array of length X. Have a index of the head of the queue and another variable storing the number of values stored in the array. This represents a queue that will hold the packets that have been sent, but not yet acknowledged by the server.

First, send a packet and store it along with its sendTime in the queue.

Second, we check the first value in the queue and see if the time it has been waiting is greater than out timeOut value. If it is not, then we can keep sending packets. This lets us set a preference on sending packets instead of looking for potential waiting acknowledgements. If it is greater than the timeOut value, then we must look to see if an acknowledgement has been sent by the server. To do this we use select to check if any data has been sent to the client from the server. A very small timeout will be used for this select call. This lets us not wait on the server ack-ing our packets, rather we focus on sending the packets to the server.

If nothing has been received, then we must resend that packet since it is over the timeout value. 

If we get an Ack from the server, then check the head of the queue of un-Acked packets. If it is that one, then we can delete it since it has now been acknowledged. We can also take the difference between the current time and the send time for that packet to get a better idea about what the timeout should be. We can then adjust our timeout to be: timeOut = X*timeOut + (1 - X)*(currTime - sendTime) where X is a variable that judges to what degree the current measurement should influence the overall timeout. However if it referencing a later packet, then we know that all the packets in the queue that come before it failed to be delivered, since this scheme is being used in a way that we know packets will be delivered in order. Thus we resend the packets that come before the Ack-ed packet. 

If the acknowledgement does not reference anything in the sent array, then it is either a duplicate Ack or just wrong. If it is a duplicate Ack, that means we sent a second Ack when we didn't have to and our timeOut is too short. We can tell if it's a duplicate Ack by storing sent sequence numbers in a hashtable. If this is the case then we can change the timeout by: timeOut = Y*timeOut where Y > 1.

The next step is to keep sending packets until the queue is full. If the queue is ever full, then we can use select to see if any acknowledgements have been received. As long as the queue is full, we use select and check if the timeOut has been exceeded as described in the previous step. This lets us send more data without having to wait for the possibly long RTT of the server - client.

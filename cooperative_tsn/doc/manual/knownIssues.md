# Time Sensitive Networking
## Multicast and Streams
If the packet is mutlicast, by default a copy is sent to the loopback interface.
The IP module requests the loopback interface and the Message Dispatcher can't find it.
The interface is not directly registered due to the existence of the bridging layer which is enabled via streams.

To solve this issue, the ´´´defaultMulticastLoop = false´´´ in the ´´´Udp´´´ module should be set to false.
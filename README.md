<A name="toc1-0" title="Some Work on a standalone DHT-22 client that connects to existing infrastructure" />
# Some Work on a standalone DHT-22 client that connects to existing infrastructure

Before it will work, you will need to create `user_config.h`

    cp ./user/user_config.example.h ./user/user_config.h


Modify as you see fit, then use `make` to compile.

<A name="toc1-10" title="Dependencies" />
# Dependencies

You'll want to follow https://github.com/esp8266/esp8266-wiki/wiki in order to do the builds - this covers how to set up the toolchain.


<A name="toc1-16" title="Burning" />
# Burning

    make burn

<A name="toc1-21" title="Thingsbus Integration" />
# Thingsbus Integration

To configure your module to transmit to a Thingsbus node, you must configure the IP, port, and namespace to use. To learn more about Thingsbus, check out https://github.com/eastein/thingsbus - the IP to use is the broker IP, and the port is the UDP input port.

    #define TBB_IP "a.b.c.d"
    #define TBB_PORT 7955
    #define TBB_NAMESPACE "esp8266.prototype"

<A name="toc2-30" title="Thingsbus data schema" />
## Thingsbus data schema


    {
	    "TBD": "data format is TBD.",
		"metadata" : {
			"loops": 2633,
			"msgs_dropped": 1,
			"msgs_sent": 2632
		}
	}


The metadata fields have the following meanings:

* `loops` how many times the sample and transmit loop has looped.
* `msgs_dropped` how many packets either couldn't be sent as there was no connection, or errored when an attempt was made to send.
* `msgs_sent` how many UDP samples have been sent successfully, including this one. 

Additional messages could have been dropped after leaving the ESP8266, but that isn't knowable by the sender as it's UDP.

# Work In Progress Thingsbus Node for ESP8266

Before it will work, you will need to create `user_config.h`

    cp ./user/user_config.example.h ./user/user_config.h

Modify as you see fit, then use `make` to compile. If you want to have multiple configs, you can name them `user_config.whatever.h` and then do this to apply a nice symlink to them (this will delete `user_config.h`):

    CONFIG=whatever make config

# Dependencies

* Check out https://github.com/pfalcon/esp-open-sdk - it's an attempt to make the ESP SDK as open as it can be. Some parts are still closed, unfortunately.
* Read its readme, do the build steps (as standalone)
* make a symlink *here* called `esp-open-sdk` to wherever you checked out `esp-open-sdk`.

You could also follow https://github.com/esp8266/esp8266-wiki/wiki in order to do the builds - this covers how to set up the toolchain. It's more involved and manual and it will certainly take longer. You'll also have to edit the Makefile of this project to use it - although we left parts for it in there, we aren't using it that way anymore...

# Building

    make all

# Burning

    sudo make burn

Another useful Makefile feature is the 'term' command which will set up a screen(1) terminal via the FTDI cable. You can do it in one fell swoop to burn, run, and watch the output:

    sudo make burn term

# Thingsbus Integration

To configure your module to transmit to a Thingsbus node, you must configure the IP, port, and namespace to use. To learn more about Thingsbus, check out https://github.com/eastein/thingsbus - the IP to use is the broker IP, and the port is the UDP input port.

    #define TBB_IP "a.b.c.d"
    #define TBB_PORT 7955
    #define TBB_NAMESPACE "esp8266.prototype"

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

# Coding Style

* Try to avoid putting additional items in the main loop without functionalizing.
* Be consistent with what's already present
* Please run all code through astyle before committing to help avoid whitespace merge conflicts.

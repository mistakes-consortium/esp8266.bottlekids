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

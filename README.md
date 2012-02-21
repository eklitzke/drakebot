Drakebot is a C++ IRC robot. It's built using boost::asio.

To build, first build the Makefile using gyp like this:

    gyp --depth=. drakebot.gyp

Then run `make` as usual. Your binary will (probably) be located at
`./out/Default/drakebot`.

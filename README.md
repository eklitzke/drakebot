Drakebot is a C++ IRC robot. It's built using boost::asio.

To build, first build the Makefile using [GYP](http://code.google.com/p/gyp/)
like this:

    gyp --depth=. drakebot.gyp

Then run `make drakebot` (or `make drakebot_opt`). Your binary will (probably)
be located at `./out/Default/drakebot`.

You'll need the following libraries installed to successfully build:

* [boost](http://www.boost.org/)
* [gflags](http://code.google.com/p/gflags/)
* [glog](http://code.google.com/p/google-glog/)

Look at the `--help` or `--helpshort` options to get an idea on how to invoke
the binary.

fchat-pidgin
------------

Pidgin plugin for using FList's chat. Forked from TestPanther's version over at https://code.google.com/p/flist-pidgin/

Building
--------

Prerequisites (on Debian-like systems)

    aptitude install build-essential make pidgin-dev libjson-glib-dev

To build the plugin for Pidgin, run :

    make 
    
To build the plugin for other libpurple clients (Finch, Spectrum 2, Minbif, Bitlbee), run :
    
    make FLIST_PURPLE_ONLY=1

Then run `make install` as root to install the plugin in the system-wide plugin directory, or simply copy the `flist.so` file in `~/.purple/plugins`

Cross Compilation
-----------------

Prerequisites (on Debian-like systems)

    apt-get install gcc-mingw-w64-i686 make

Run `make prepare_cross` to fetch the dependencies you need to build a 32-bit Windows version of the plugin from Linux.

Then, run

    make flist.dll

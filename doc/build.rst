Building iocatcher
==================

Requirements
------------

In order to build iocatcher you need two libraries:

* `libfabric <https://ofiwg.github.io/libfabric/>`_ (tested 1.12.1)
* `libevent <https://libevent.org/>`_ (tested 2.1.12)

In order to build the code you need:

* `cmake <https://cmake.org/>`_ (tested 2.8.8)
* A C++ compiler with C++11 support. (tested g++/clang++)

Building
--------

To build you can either call directly cmake of use the configure wrapper script.
This wrapper just prepare options by providing an autotools like semantic and
just call cmake. You will find the cmake command line used as the first line
printed by the script.

.. code-block:: shell

  mkdir build
  ../configure
  make
  make test
  make install

Simple run
----------

You can make a first run launching the server locally and connecting a simple
client to it.

.. code-block:: shell

  #launch the server
  iocatcher-server-no-mero 127.0.0.1

  #in another terminal launch the client
  src/client/iocatcher-client 127.0.0.1

If you want to run on an Infiniband card you need to use the IP address of this
device on the server host.

You can find more client examples in the *examples* directory.

Server options
--------------

You can query the server semantic with the common **--help** option.

Debugging
---------

If you want to enable debug messages you need to rebuild the server in debug 
mode:

.. code-block:: shell

  ../configure --enable-debug
  make
  make install

You can then enable the debug messages of the server by listing the debugging
groups you want to filter of providing all to enable all.

.. code-block:: shell

  # display all debug messages
  iocatcher-server-no-mero -v all 127.0.0.1

  # display messages from hook:ping
  iocatcher-server-no-mero -v hook:ping 127.0.0.1
  
  # display all subclass hook messages
  iocatcher-server-no-mero -v hook 127.0.0.1
  
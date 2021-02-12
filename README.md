libfabric-poc
=============

This is a proof of concept code to test and learn about libfabric in RDM mode.
The goal is to look on it for implementation of the storage intermediate server
for the SAGE2 Global Memory Abstraction server.

Dependencies
------------

 - libfabric (tested 1.11.0), requierd.

Building
--------

You can build by using:

```sh
mkdir build
cd build
../configure --with-libfabric=$HOME/usr
make
```

Running the full chaine
-----------------------

This run the full implementation. You can use options to configure the server,
look with --help.

```sh
#server
./src/server/iocatcher-server-no-mero 127.0.0.1
#client
./src/client/iocatcher-client 127.0.0.1
```

Running the basic benchmark
---------------------------

You need to launch the server providing the listening address (127.0.0.1 by
default) then you need to launch the client.

```sh
#server
./ 192.168.10.1
#client
./poc -c 192.168.10.1
```

You can launch multiple client threads. On the server side you need to declare
the number of expected clients with -C, on the client side you can lauch serveral
clients of use internal threads with -t option.

```sh
#server
./poc -s -C 8 192.168.10.1
#client
./poc -c -t 8 192.168.10.1
```

You can use passive pooling instead of active by enabling -w option on server
or/and client. You can change the number of messages by applying -m option on
both server and client with the same value.

```sh
#server
./poc -m 10000000 -s 192.168.10.1
#client
./poc -m 10000000 -c 192.168.10.1
```

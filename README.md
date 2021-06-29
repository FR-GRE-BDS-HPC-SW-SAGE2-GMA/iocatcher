iocatcher
=========

This project is a proof of concept to implement a kind of cache (or burst buffer)
between an application using ummap-io-v2 and the Motr object storage from seagate
https://github.com/Seagate/cortx-motr. It explores usage of RDMA operations to 
tranmis data via libfabric with the goal to store the temporary data in NVDIMMs.

We recap here that Motr objects are just identified as two 64 bit integers 
marking a 128 bit uniq key.

Dependencies
------------

The project depends on: 

 - libfabric (tested 1.11.0, 1.12.1), requierd, https://ofiwg.github.io/libfabric/
 - libevent (tested 2.1.12), required, https://libevent.org/
 - cmake (tested 2.8.11), required to build, https://cmake.org/
 - A C++ compiler supporting C++11.

Supported plateform
-------------------

The project uses libfabric for its network implementation so it supports fallback
on TCP. It uses the RDM protocol from libfabric so it allows using all the related
network drivers (verbs, tcp, ucx, psm).

The code should not be so dependent on the architecture but was developped for
x86_64.

Building
--------

You can build by using:

```sh
mkdir build
cd build
../configure --prefix=$HOME/usr --with-libfabric=$HOME/usr --with-libevent=$HOME/usr
make
make test
make install
```

If you want generate code coverage:

```sh
mkdir build-coverage
cd build-coverage
../configure --prefix=$HOME/usr --with-libfabric=$HOME/usr --with-libevent=$HOME/usr --enable-coverage
make
make test
../dev/gen-coverage.sh
#find the report in the html directory
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

The client API
--------------

The client API is very simple and is summarized here:


```c
//library lifecycle
ioc_client_t * ioc_client_init(const char * ip, const char * port);
void ioc_client_fini(ioc_client_t * client);

//object actions
ssize_t ioc_client_obj_read(ioc_client_t * client, int64_t high, int64_t low, void* buffer, size_t size, size_t offset);
ssize_t ioc_client_obj_write(ioc_client_t * client, int64_t high, int64_t low, const void* buffer, size_t size, size_t offset);
int ioc_client_obj_flush(ioc_client_t * client, int64_t high, int64_t low, uint64_t offset, uint64_t size);
int ioc_client_obj_create(ioc_client_t * client, int64_t high, int64_t low);
int ioc_client_obj_cow(ioc_client_t * client, int64_t orig_high, int64_t orig_low, int64_t dest_high, int64_t dest_low, bool allow_exist, size_t offset, size_t size);

//access protection
int32_t ioc_client_obj_range_register(ioc_client_t * client, int64_t high, int64_t low, size_t offset, size_t size, bool write);
int ioc_client_obj_range_unregister(ioc_client_t * client, int32_t id, int64_t high, int64_t low, size_t offset, size_t size, bool write);

//get current network provider name
const char * ioc_client_provider_name(ioc_client_t * client);

//move from active to passiv wait mode
void ioc_client_set_passive_wait(ioc_client_t * client, bool value);
```

License & finance
-----------------

This project is distributed under Apache 2.0 license.

The project was developped for the SAGE2 european project.

Authors
-------

Sébastien Valat (ATOS)
Christophe Laferrière (ATOS)

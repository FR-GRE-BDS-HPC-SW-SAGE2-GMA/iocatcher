Client API
==========

This section will describe the C client API to access Mero objects via iocatcher
or use iocatcher alone.

Include & compiling
-------------------

In order to use iocatcher you need to include the **ioc-client.h** file.

.. code-block:: C

  #include <ioc-client.h>

Compiling
---------

To compile you need to link the the **libioc-client.so**.

.. code-block:: shell

  gcc -lioc-client

Life cycle
----------

You first need to initialize the library. The initialization function provides a
client handler so the library can perfectly be initialized multiple times if
want to join several servers.

.. code-block:: C

  //connect to server
  struct ioc_client_t * client = ioc_client_init("127.0.0.1", "8556");

  //get provider name to be printed
  const char * provider = ioc_client_provider_name(client);
  printf("Provider in use : %s\n", provider);

  //clean connection
  ioc_client_fini(client);

Object identification
---------------------

Iocatcher is aimed to cache objects from the Motr object storage so it takes back
his object identification mechanism which correspond to two 64-bit integers (low
and high).

Object access API
-----------------

The API provide the given functions. You can refer to a more precise help in the
doxygen provided by the **ioc-client.h** header file.

.. code-block:: C

  //create objects
  int ioc_client_obj_create(ioc_client_t * client, int64_t high, int64_t low);

  // read & write
  ssize_t ioc_client_obj_read(ioc_client_t * client, int64_t high, int64_t low, void* buffer, size_t size, size_t offset);
  ssize_t ioc_client_obj_write(ioc_client_t * client, int64_t high, int64_t low, void* buffer, size_t size, size_t offset);

  //flush
  int ioc_client_obj_flush(ioc_client_t * client, int64_t high, int64_t low, uint64_t offset, uint64_t size);

  //make copy on write on the ioc server
  int ioc_client_obj_cow(ioc_client_t * client, int64_t orig_high, int64_t orig_low, int64_t dest_high, int64_t dest_low, bool allow_exist, size_t offset, size_t size);

Range exclusion
---------------

With iocatcher you can register a range to say you are mapping it in read or
write mode. This give you exclusive access to this range so other client be
rejected if they try to register again this range until you deregister it after
use.

On registration you get a registration ID which you need to use again when you
want to deregister your registration.

This aimed to be used inside ummap-io to get exclusive write mapping and guaranty
we stay in valid conditions.

.. code-block:: C

  //range registration
  int32_t ioc_client_obj_range_register(ioc_client_t * client, int64_t high, int64_t low, size_t offset, size_t size, bool write);
  int ioc_client_obj_range_unregister(ioc_client_t * client, int32_t id, int64_t high, int64_t low, size_t offset, size_t size, bool write);


Configuration API
-----------------

.. code-block:: C

  //enable or disable the active/passive polling
  void ioc_client_set_passive_wait(ioc_client_t * client, bool value);

If you want to experiment with multiple write mapping by enforcing at your level
a correct semantic, you can disable the consistency checking by using the 
**--no-consistency-check** option while launching the server.

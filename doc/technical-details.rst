Technical details
=================

This section will take some specific technical points and will describe them in more details.

Client tracking
---------------

Iocatcher use the RDM protocol from libfabric which does not track connections this imply two limitations:

* We need to track client disconnection by another way.
* We need to find a way to identify clients to know to whom to respond when we receive a request.

For the first point we added a TCP connection aside the libfabric one. We first establish this connection
then establish the libfabric one. When the client disconnects, we are notified via the TCP connection through
the libevent library.

When he joins the server, the client is assigned to a TCP client ID and a protection key. Those two
identifier needs to be used in every latter communication through the libfabric connection and are checked
on reception. This checking can be disabled if we notice issues.

Finally, when we receive a message through the libfabric RDM protocol we do not know from which client it comes.
In order to know, the client needs to add a client ID into the request it sends to the server. This client ID
is assigned after the first handshake message received from the server when establishing the libfabric connection.
On the server it is matched with the TCP client ID and key so we safely check each packet to be well identified.

This make all the iocatcher server code using this client ID to identify the clients when needing to contact them.

Protocol description
--------------------

The protocol used in the libfabric connection is described in the src/base/network/Protocol.hpp file. 
For a first prototyping we directly use flat C struct to be copy pasted on the connection and not
implementing manual serialization. This can be done in a second step.

The protocol is composed of two layers. A low level layer stored into the header (LibfabricMessageHeader) 
field of the LibfabricMessage struct. It only describes the type of message and the client authentication.
From the type of message we can know how to treat the data field of the LibfabricMessage struct.

Hook handling
-------------

On the server side we handle the actions attached to a request by register hooks to the LibfabricConnection
object so it knows how to treat a request based on the type field from the low level protocol header. Each
hook on the server side is an object with polymorphism.

On the client side we avoid registering/deregistering hooks by passively (or actively) waiting for a
specific message all other type of message leading to an error.

Segment handling
----------------

The object segments represented by the ObjectSegment class are represented by an offset, a size and a memory
pointer. The memory pointer is allocated through the MemoryBackend implementations which offer support
of caching, nvdimm mapping or simple allocation via malloc. All those segments are pinned into memory
to be ready for RDMA transfers.

NVDIMM allocation
-----------------

The way to get NVDIMM memory is to open a file, truncate it and mmap it into the memory space of the server.
But in iocatcher we have one segment per first request in the object or we can group them up to a certain
size which is currently 8 MB. This means that allocated a huge object will create tons of files to tons of
file descriptors to open.

In order to fix this issue we first tried to truncate an already opened file to get a larger space.
This work well until we try to memory register that memory in libfabric leading to a kernel deadlock
situation.

To bypass the file descriptor limit, we currently allocate a new file which is two times larger than the 
previous one up to a limit of 32 GB which limits the number of files we need to create. 

If you look in the code, we have several layers which goes on top of each other to get this semantic.
We use the MemoryBackendNvdimm to handle the NVDIMM mapping, then we add a cache on top of it then we
at lastly a MemoryBackendBalance layer ot be able to round robin the allocatoions on several NVDIMMs
if we have several NUMA nodes on the server.

Copy-on-write
-------------

The object class support the copy-on-write semantic allowing to copy a range of an object to another object.
Notice we currently only consider cowing a range from an offset to the same offset in the destination object.
This is a restriction compared to the CLONERANGE ioctl call supported by XFS, BTRFS and luster.

The copy is triggered by the first call to getBuffers() for a write access. Other accessed will return the shared
copy.

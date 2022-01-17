/*****************************************************
*  PROJECT  : IO Catcher                             *
*  LICENSE  : Apache 2.0                             *
*  COPYRIGHT: 2020-2022 Bull SAS All rights reserved *
*****************************************************/

#ifndef IOC_LIBFABRIC_DOMAIN_HPP
#define IOC_LIBFABRIC_DOMAIN_HPP

/****************************************************/
//std
#include <string>
#include <list>
#include <map>
#include <mutex>
//libfabric
#include <rdma/fabric.h>
#include <rdma/fi_errno.h>
#include <rdma/fi_domain.h>
//local
#include "Protocol.hpp"
#include "ClientRegistry.hpp"

//TMP
//#define TEST_RDMA_SIZE (4*1024*1024)
#define TEST_RDMA_SIZE (4096)

/****************************************************/
namespace IOC
{

/****************************************************/
/**
 * Used to keep track of the memory region for pre-RDMA registratin.
 * @brief Tracking of an RDMA registered memory region.
**/
struct MemoryRegion
{
	/** Base pointer of the memory region. **/
	void * ptr;
	/** Size of the memory region. **/
	size_t size;
	/** Libfabric regitration id for the given memory region. **/
	fid_mr * mr;
};

/****************************************************/
/**
 * Wrapper of a libfabric network domain. This is used to setup the library
 * and is a prelude to a connection establishment. It also handle the memory
 * registration operations.
 * @brief Wrapper of a libfabric domain to setup connections.
**/
class LibfabricDomain
{
	public:
		LibfabricDomain(const std::string & server, const std::string & port, bool isDomainServer);
		~LibfabricDomain(void);
		Iov registerSegment(void * ptr, size_t size, bool read, bool write, bool pmem);
		void unregisterSegment(void * ptr, size_t size);
		fi_info * getFiInfo(void);
		fid_fabric * getFabric(void);
		fid_domain * getDomain(void);
		fid_mr* getFidMR ( void* ptr, size_t size );
		MemoryRegion* getMR ( void* ptr, size_t size );
		void setMsgBuffeSize(size_t size);
		size_t getMsgBufferSize(void);
		void * getMsgBuffer(void);
		void retMsgBuffer(void * buffer);
		const char * getLFProviderName(void) const;
	private:
		/** Libfabric configuration info being setup before the domain. **/
		fi_info *fi;
		/** libfabric fabric to be used. **/
		fid_fabric *fabric;
		/** libfabric domain to be used. **/
		fid_domain *domain;
		/** List of memory regions registered for RDMA transfers. Index point the last bytes of the segment. **/
		std::map<void*, MemoryRegion> segments;
		/** 
		 * Boolean to know the memory region mode to know if we need to send
		 * to the remote node the absolute address of the semgement for RDMA
		 * operation of the relative address inside the segment considering
		 * 0 as the base address.
		**/
		bool virtMrMode;
		/** 
		 * The domain can handle pre-registered buffers for message sending. This
		 * varibale define their size to be allocated. 
		**/
		size_t msgBufferSize;
		/** Keep track of the pre-registered buffers for message sending. **/
		std::list<void *> msgBuffers;
		/** 
		 * Mutex to access the pre-registered buffer for message sending as
		 * multiple threads might want to send messages via their local
		 * connection instance.
		**/
		std::mutex msgBuffersMutex;
		/**
		 * Mutex to protect the memory region registration which can be done
		 * by multiple thread to handle the final operatoin in their local
		 * connection instance.
		**/
		std::mutex segmentMutex;
};

}

#endif //IOC_LIBFABRIC_DOMAIN_HPP

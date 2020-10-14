/*****************************************************
			 PROJECT  : IO Catcher
			 LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
			 COPYRIGHT: 2020 Bull SAS
*****************************************************/

#ifndef IOC_LIBFABRIC_CONNECTION_HPP
#define IOC_LIBFABRIC_CONNECTION_HPP

/****************************************************/
//libfabric
#include <rdma/fabric.h>
#include <rdma/fi_errno.h>
#include <rdma/fi_endpoint.h>
//local
#include "LibfabricDomain.hpp"

/****************************************************/
namespace IOC
{

/****************************************************/
class LibfabricConnection
{
	public:
		LibfabricConnection(LibfabricDomain * domain, bool wait);
		~LibfabricConnection(void);
	private:
		LibfabricDomain * domain;
		/** Transmission completion queue. **/
		fid_cq *tx_cq;
		//** Revive completion queue. **/
		fid_cq *rx_cq;
		/** Address vector. **/
		fid_av *av;
		/** Endpoint **/
		fid_ep *ep;
		bool wait;
};

}

#endif //IOC_LIBFABRIC_CONNECTION_HPP

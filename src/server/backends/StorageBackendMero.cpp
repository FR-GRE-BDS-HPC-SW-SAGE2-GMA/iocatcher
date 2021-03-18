/*****************************************************
			PROJECT  : IO Catcher
			LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
*****************************************************/

/****************************************************/
//std
#include <iostream>
#include "base/common/Debug.hpp"
#include "StorageBackendMero.hpp"
#ifndef NOMERO
	extern "C" {
		#include "clovis_api.h"
	}
#endif

/****************************************************/
using namespace IOC;
using namespace std;

/****************************************************/
/* This value is fixed based on the HW that you run,
 * 50 for VM execution and 25 for Juelich prototype execution has been tested OK
 * Just a bit of explanation here:
 * Internally, MERO is splitting the write ops into data units
 * There is an arbitrary limitation within on the number of data units
 * that could be written in a single MERO ops.
 *
 * An error on the IO size limitation overhead looks like this:
 * mero[XXXX]:  eb80  ERROR  [clovis/io_req.c:452:ioreq_iosm_handle_executed]  iro_dgmode_write() failed, rc=-7
 *
 * A data unit size is directly based on the layout id of the object
 * Default layout id (defined in clovis config) for this example is 9 which leads to a data units size of 1MB
 * A value of 50 means that we can write up to 50MB (50 data units of 1MB) per ops
*/
#ifndef CLOVIS_MAX_DATA_UNIT_PER_OPS
	#define CLOVIS_MAX_DATA_UNIT_PER_OPS  50
#endif


/****************************************************/
#ifndef NOMERO
	/**
	 * Operator to help debugging by printing a mero object ID to std::streams.
	 * @param out Reference to the output stream to be used.
	 * @param object_id Object ID to print.
	 * @return Reference to the output stream after printing.
	**/
	inline std::ostream& operator<<(std::ostream& out, struct m0_uint128 object_id)
	{
		out << object_id.u_hi << ":" << object_id.u_lo;
		out << std::flush;
		return out;
	}
#endif

/****************************************************/
/**
 * Constructor of the Mero storage backend, currently don't do anything.
**/
StorageBackendMero::StorageBackendMero(void)
{
}

/****************************************************/
/**
 * Destructor of the Mero storage backend, currently don't do anything.
**/
StorageBackendMero::~StorageBackendMero(void)
{
}

/****************************************************/
int StorageBackendMero::create(int64_t high, int64_t low)
{
	#ifndef NOMERO
		struct m0_uint128 id;
		id.u_hi = high;
		id.u_lo = low;
		return create_object(id);
	#else
		return 0;
	#endif
}

/****************************************************/
ssize_t StorageBackendMero::pread(int64_t high, int64_t low, void * buffer, size_t size, size_t offset)
{
	#ifndef NOMERO
		//check
		assert(buffer != NULL);

		struct m0_indexvec ext;
		struct m0_bufvec data;
		struct m0_bufvec attr;

		struct m0_uint128 m_object_id;
		m_object_id.u_hi = high;
		m_object_id.u_lo = low;

		char *char_buf = (char *)buffer;
		int ret = 0;
		// We assume here 2 things: 
		// -> That the object is opened
		// -> And opened with the same layout as the default clovis one
		int layout_id = m0_clovis_layout_id(clovis_instance);
		size_t data_units_size = (size_t) m0_clovis_obj_layout_id_to_unit_size(layout_id);

		assert(data_units_size > 0);

		int total_blocks_to_read = 0;

		if (data_units_size > size) {
			data_units_size = size;
			total_blocks_to_read = 1;        
		} else {
			total_blocks_to_read =  size / data_units_size;
			if (size % data_units_size != 0) {
				assert(false && "We don't handle the case where the IO size is not a multiple of the data units");
			}
		}

		int last_index = offset;
		int j = 0;

		while(total_blocks_to_read > 0) {

			int block_size = (total_blocks_to_read > CLOVIS_MAX_DATA_UNIT_PER_OPS)?
						CLOVIS_MAX_DATA_UNIT_PER_OPS : total_blocks_to_read;

			m0_bufvec_alloc(&data, block_size, data_units_size);
			m0_bufvec_alloc(&attr, block_size, 1);
			m0_indexvec_alloc(&ext, block_size);

				/* Initialize the different arrays */
			int i;
			for (i = 0; i < block_size; i++) {
				attr.ov_vec.v_count[i] = 1;
				ext.iv_index[i] = last_index;
				ext.iv_vec.v_count[i] = data_units_size;
				last_index += data_units_size;
			}

			ret = read_data_from_object(m_object_id, &ext, &data, &attr);
			// @todo: handling partial read 
			if (ret != 0)
				break;

			for (i = 0; i < block_size; i++) {
				memcpy((char_buf + i*data_units_size + j*block_size*data_units_size), data.ov_buf[i], data_units_size);
			}

			m0_indexvec_free(&ext);
			m0_bufvec_free(&data);
			m0_bufvec_free(&attr);
			
			total_blocks_to_read -= block_size;
			j++;
		};

		if (ret == 0) {
			IOC_DEBUG_ARG("mero", "Success executing the MERO helperPread op, object ID=%1, offset=%3, size=%2")
				.arg(m_object_id)
				.arg(offset)
				.arg(size)
				.end();
			return size;
		} else {
			cerr << "[Failed] Error executing the MERO helperPread op, object ID=" << m_object_id << " , size=" << size
						<< ", offset=" << offset << endl;
			errno = EIO;
			return -1;
		}
	#else
		return size;
	#endif
}

/****************************************************/
ssize_t StorageBackendMero::pwrite(int64_t high, int64_t low, void * buffer, size_t size, size_t offset)
{
	#ifndef NOMERO
		//check
		assert(buffer != NULL);
		struct m0_indexvec ext;
		struct m0_bufvec data;
		struct m0_bufvec attr;

		char *char_buf = (char *) buffer;
		int ret = 0;

		struct m0_uint128 m_object_id;
		m_object_id.u_hi = high;
		m_object_id.u_lo = low;

		// We assume here 2 things:
		// -> That the object is opened
		// -> And opened with the same layout as the default clovis one
		int layout_id = m0_clovis_layout_id(clovis_instance);
		size_t data_units_size = (size_t) m0_clovis_obj_layout_id_to_unit_size(layout_id);

		assert(data_units_size > 0);

		int total_blocks_to_write = 0;

		if (data_units_size > size) {
			data_units_size = size;
			total_blocks_to_write = 1;        
		} else {
			total_blocks_to_write =  size / data_units_size;
			if (size % data_units_size != 0) {
				assert(false && "We don't handle the case where the IO size is not a multiple of the data units");
			}
		}

		int last_index = offset;
		int j = 0;

		while(total_blocks_to_write > 0) {

			int block_size = (total_blocks_to_write > CLOVIS_MAX_DATA_UNIT_PER_OPS)?
						CLOVIS_MAX_DATA_UNIT_PER_OPS : total_blocks_to_write;

			m0_bufvec_alloc(&data, block_size, data_units_size);
			m0_bufvec_alloc(&attr, block_size, 1);
			m0_indexvec_alloc(&ext, block_size);

				/* Initialize the different arrays */
			int i;
			for (i = 0; i < block_size; i++) {

				//@todo: Can we remove this extra copy ?
				memcpy(data.ov_buf[i], (char_buf + i*data_units_size + j*block_size*data_units_size), data_units_size);

				attr.ov_vec.v_count[i] = 1;

				ext.iv_index[i] = last_index;
				ext.iv_vec.v_count[i] = data_units_size;
				last_index += data_units_size;
			}

			cout << "[Pending] Send MERO write ops, object ID=" << m_object_id;
			cout << ", size=" << block_size << ", bs=" << data_units_size << endl;
			
			// Send the write ops to MERO and wait for completion 
			ret = write_data_to_object(m_object_id, &ext, &data, &attr);

			m0_indexvec_free(&ext);
			m0_bufvec_free(&data);
			m0_bufvec_free(&attr);

			// @todo: handling partial write
			if (ret != 0) 
				break;
			
			total_blocks_to_write -= block_size;
			j++;
		};

		if (ret == 0) {
			IOC_DEBUG_ARG("mero", "Success executing the MERO helperWrite op, object ID=%1, offset=%3, size=%2")
				.arg(m_object_id)
				.arg(offset)
				.arg(size)
				.end();
			return size;
		} else {
			cerr << "[Failed] Error executing the MERO helperPwrite op, object ID=" << m_object_id << " , size=" << size
						<< ", offset=" << offset << endl;
			errno = EIO;
			return -1;
		}
	#else
		return size;
	#endif
}

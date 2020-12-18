/*****************************************************
            PROJECT  : IO Catcher
            LICENSE  : Apache 2.0
			COPYRIGHT: 2020 Bull SAS
COPYRIGHT: 2020 Bull SAS
*****************************************************/

/****************************************************/
#include <cassert>
#include <iostream>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "Object.hpp"
#include "../../base/common/Debug.hpp"
#ifndef NOMERO
	extern "C" {
		#include "clovis_api.h"
	}
#endif

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
using namespace IOC;
using namespace std;

/****************************************************/
#ifndef NOMERO
	inline std::ostream&
	operator<<(std::ostream& out, struct m0_uint128 object_id)
	{
	out << object_id.u_hi << ":" << object_id.u_lo;
	out << std::flush;
	return out;
	}
#endif

/****************************************************/
static ssize_t pwrite(int64_t high, int64_t low, const void * buffer, size_t size, size_t offset)
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

			//cout << "[Pending] Send MERO write ops, object ID=" << m_object_id;
			//cout << ", size=" << block_size << ", bs=" << data_units_size << endl;
			
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
			//cout << "[Success] Executing the MERO pwrite op, object ID="<< m_object_id << ", size=" << size
			//			<< ", offset=" << offset << endl;
			return size;
		} else {
			cerr << "[Failed] Error executing the MERO pwrite op, object ID=" << m_object_id << " , size=" << size
						<< ", offset=" << offset << endl;
			errno = EIO;
			return -1;
		}
	#else
		return size;
	#endif
}

/****************************************************/
static ssize_t pread(int64_t high, int64_t low, void * buffer, size_t size, size_t offset)
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
			cout << "[Success] Executing the MERO pread op, object ID="<< m_object_id << ", size=" << size
					<< ", offset=" << offset << endl;
			return size;
		} else {
			cerr << "[Failed] Error executing the MERO pread op, object ID=" << m_object_id << " , size=" << size
						<< ", offset=" << offset << endl;
			errno = EIO;
			return -1;
		}
	#else
		return size;
	#endif
}

/****************************************************/
bool ObjectSegment::overlap(size_t segBase, size_t segSize)
{
	return (this->offset < segBase + segSize && this->offset + this->size > segBase);
}

/****************************************************/
Object::Object(LibfabricDomain * domain, int64_t low, int64_t high)
{
	this->domain = domain;
	this->objectId.low = low;
	this->objectId.high = high;
}

/****************************************************/
const ObjectId & Object::getObjectId(void)
{
	return this->objectId;
}

/****************************************************/
void Object::markDirty(size_t base, size_t size)
{
	//extract
	for (auto it : this->segmentMap) {
		//if overlap
		if (it.second.overlap(base, size)) {
			it.second.dirty = true;
		}
	}
}

/****************************************************/
void Object::getBuffers(ObjectSegmentList & segments, size_t base, size_t size, bool load)
{
	//align
	size_t align = 8*1024*1024;
	size += base % align;
	base -= base % align;
	size += align - (size % align);

	//extract
	for (auto it : this->segmentMap) {
		//if overlap
		if (it.second.overlap(base, size)) {
			segments.push_back(it.second);
		}
	}

	//complete
	size_t lastOffset = base;
	ObjectSegmentList tmp;
	for (auto it : segments) {
		if (it.offset > lastOffset) {
			tmp.push_back(this->loadSegment(lastOffset, it.offset - lastOffset, load));
		}
		lastOffset = it.offset + it.size;
	}

	//merge list
	for (auto it : tmp)
		segments.push_back(it);

	//load last
	if (lastOffset < base + size)
		segments.push_back(this->loadSegment(lastOffset, base + size - lastOffset, load));

	//sort
	segments.sort();
}

/****************************************************/
char * allocateNvdimm(int64_t high, int64_t low, size_t offset, size_t size)
{
	//open
	char fname[1024];
	sprintf(fname, "/mnt/pmem1/valats/ioc/ioc-%ld:%ld-%lu.raw", high, low, offset);
	unlink(fname);
	int fd = open(fname, O_CREAT|O_RDWR);
	assume(fd >= 0, "Fail to open nvdimm file !");

	//truncate
	printf("%d %lu\n", fd, size);
	int status = ftruncate(fd, size);
	assumeArg(status == 0, "Fail to ftruncate: %1").argStrErrno().end();

	//mmap
	void * ptr = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	assumeArg(ptr != MAP_FAILED, "Fail to mmap : %1").argStrErrno().end();
	memset(ptr, 0, size);

	//madvie
	//status = madvise(ptr, size, MADV_DONTFORK);
	//assumeArg(status != 0, "Fail to madvise: %1").argStrErrno().end();

	return (char*)ptr;
}

/****************************************************/
ObjectSegment Object::loadSegment(size_t offset, size_t size, bool load)
{
	ObjectSegment & segment = this->segmentMap[offset];
	segment.offset = offset;
	segment.size = size;
	segment.ptr = (char*)malloc(size);
	segment.dirty = false;
	//segment.ptr = allocateNvdimm(this->objectId.high, this->objectId.low, offset, size);
	if (this->domain != NULL)
		this->domain->registerSegment(segment.ptr, segment.size, true, true, true);
	if (load) {
		size_t status = pread(this->objectId.high, this->objectId.low, segment.ptr, size, offset);
		assume(status == size, "Fail to pread from object !");
		if (status != size)
			status = pwrite(this->objectId.high, this->objectId.low, segment.ptr, size, offset);
		assume(status == size, "Fail to write from object !");
	}
	return segment;
}

/****************************************************/
int Object::flush(size_t offset, size_t size)
{
	int ret = 0;
	for (auto it : this->segmentMap) {
		if (it.second.dirty) {
			if (size == 0 || it.second.overlap(offset, size)) {
				if (pwrite(this->objectId.high, this->objectId.low, it.second.ptr, it.second.size, it.second.offset) != (ssize_t)it.second.size)
					ret = -1;
				it.second.dirty = false;
			}
		}
	}
	return ret;
}

/****************************************************/
bool IOC::operator< (const ObjectSegment & seg1, const ObjectSegment & seg2)
{
	return seg1.offset < seg2.offset;
}

/****************************************************/
bool IOC::operator<(const ObjectId & objId1, const ObjectId & objId2)
{
	if (objId1.high < objId2.high)
		return true;
	else if (objId1.high == objId2.high)
		return objId1.low < objId2.low;
	else
		return false;
}

/* -*- C -*- */
/*
 * Copyright (c) 2017-2020 Seagate Technology LLC and/or its Affiliates
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * For any questions about this software or licensing,
 * please email opensource@seagate.com or cortx-questions@seagate.com.
 *
 * Original author:  Ganesan Umanesan <ganesan.umanesan@seagate.com>
 * Original creation date: 10-Jan-2017
 */

#pragma once

#ifndef __C0APPZ_H__
#define __C0APPZ_H__

#include <stdint.h>
#include "motr/client.h"
#include "lib/types.h" /* uint32_t */

#ifdef __cplusplus
extern "C" {
#endif


extern bool m0trace_on;
extern const char *c0appz_help_txt;

/**
 * Initialise Object Store
 *
 * idx selects which local endpoint and proc fid to use from the
 * startup configuration file at $HOME/.c0appz/ (starts from 0).
 */
int c0appz_init(int idx);
int c0appz_free(void);

/**
 * Open m0-entity (object)
 *
 * @retval 0 on success
 */
int open_entity(struct m0_entity *entity);

/**
 * Calculate the optimal block size for the object store I/O
 *
 * @param idhi high number of the object id
 * @param idlo low number of the object id
 * @param obj_sz estimated total object size
 * @param tier tier index
 *
 * @retval 0 on error
 */
uint64_t c0appz_m0bs(uint64_t idhi, uint64_t idlo, uint64_t obj_sz, int tier);

/**
 * Create object in the object store
 *
 * @param idhi high number of the object id
 * @param idlo low number of the object id
 * @param tier tier index where to create object
 * @param m0bs block size for the object store I/O,
 *             used to set the optimal object's unit size,
 *             refer to c0appz_m0bs()
 *
 * @retval 0 on success
 * @retval 1 the object already exists
 * @retval <0 error code
 */
int c0appz_cr(uint64_t idhi, uint64_t idlo, int tier, uint64_t m0bs);

/**
 * Remove object
 */
int c0appz_rm(uint64_t idhi, uint64_t idlo);

/**
 * Check whether the object exists
 *
 * @param idhi high number of the object id
 * @param idlo low number of the object id
 * @param obj_out returned if not NULL and obj exists
 *
 * @retval 1 yes, the object exists
 * @retval 0 no such object
 */
int c0appz_ex(uint64_t idhi, uint64_t idlo, struct m0_obj *obj_out);

/**
 * Copy file to the object store
 *
 * The object should be created beforehand with c0appz_cr().
 *
 * @param filename where to copy the data from
 * @param bsz block size for I/O on the file, must be
 *            multiple of PAGE_SIZE (4K)
 * @param cnt number of bsz-blocks to do
 * @param m0bs block size for the object store I/O, must be
 *             multiple of bsz, refer to c0appz_m0bs().
 *
 * @retval 0 on success
 */
int c0appz_cp(uint64_t idhi, uint64_t idlo, char *filename,
	      uint64_t bsz, uint64_t cnt, uint64_t m0bs);

/**
 * Cat/read into file from the object store
 *
 * @param filename where to read the data to
 * @param bsz block size for I/O on the file, must be
 *            multiple of PAGE_SIZE (4K)
 * @param cnt number of bsz-blocks to do
 * @param m0bs block size for the object store I/O, must be
 *             multiple of bsz, refer to c0appz_m0bs().
 *
 * @retval 0 on success
 */
int c0appz_cat(uint64_t idhi, uint64_t idlo, char *filename,
	      uint64_t bsz, uint64_t cnt, uint64_t m0bs);

/**
 * Copy file to the object store asynchronously in batches
 *
 * @param filename where to copy the data from
 * @param bsz block size for I/O on the file
 * @param cnt total number of blocks to do, must be
 *            multiple of op_cnt
 * @param op_cnt number of operations in a batch
 * @param m0bs block size for the object store I/O, must be
 *             multiple of bsz, refer to c0appz_m0bs().
 *
 * @retval 0 on success
 */
int c0appz_cp_async(uint64_t idhi, uint64_t idlo, char *filename, uint64_t bsz,
		    uint64_t cnt, uint32_t op_cnt, uint64_t m0bs);

int c0appz_set_manual_rc(const char * rc);
int c0appz_setrc(char *progname);
void c0appz_putrc(void);
int c0appz_timeout(uint64_t sz);
int c0appz_timein();
int c0appz_dump_perf(void);
/* qos */
int qos_pthread_wait();
int qos_pthread_start();
int qos_pthread_stop();
int qos_pthread_cond_signal();
int qos_pthread_cond_wait();

int c0appz_fw(char *buf, char *ouf, uint64_t bsz, uint64_t cnt);
int c0appz_fr(char *buf, char *inf, uint64_t bsz, uint64_t cnt);
int c0appz_mr(char *buf, uint64_t idhi, uint64_t idlo, uint64_t off,
	      uint64_t bsz, uint64_t cnt, uint64_t m0bs);
int c0appz_mw(const char *buf, uint64_t idhi, uint64_t idlo, uint64_t off,
	      uint64_t bsz, uint64_t cnt, uint64_t m0bs);
int c0appz_mw_async(const char *buf, uint64_t idhi, uint64_t idlo, uint64_t off,
		    uint64_t bsz, uint64_t cnt, uint32_t op_cnt, uint64_t m0bs);

int write_data_to_object(struct m0_obj *o, struct m0_indexvec *ext,
			 struct m0_bufvec *data, struct m0_bufvec *attr);
int read_data_from_object(struct m0_obj *o, struct m0_indexvec *ext,
			  struct m0_bufvec *data,struct m0_bufvec *attr);

int ppf(const char *fmt, ...);
int create_object(struct m0_uint128 id);

/**
 * Loads a library in all available m0ds.
 */

/* Import */
struct m0_fid;
struct c0appz_isc_req;
struct m0_buf;
struct m0_rpc_link;
//enum m0_conf_service_type;


extern struct m0_client          *m0_instance;

/*
 ** ECMWF REQUIREMENTS
 ** synchronous/asynchronous get/put etc.
 ** Note to Nikita: c0appz uses int64_t idhi, int64_t idlo to refer Motr object
 ** ids due to popular demand from other Sage developers. Please use the same
 ** convention below. ECMWF has been informed about it.
 */

#if 0
/* generate a unique object id */
int c0appz_generate_id(struct m0_uint128* id);
/* synchronous put a full object */
int c0appz_put(char* buffer, size_t size, struct m0_uint128* id);
/* synchronous get a full object */
int c0appz_get(char* buffer, size_t* size, struct m0_uint128* id);
/* asynchronous put a full object, allocates op */
struct m0_op* c0appz_async_put(char* buffer, size_t size, struct m0_uint128* id);
/* asynchronous get a full object, allocates op */
struct m0_op* c0appz_async_get(char* buffer, size_t size, struct m0_uint128* id);
/* wait for an operation to complete and check status */
int c0appz_wait(struct m0_op* op);
/* free ops */
void c0appz_free_op(struct m0_op*);
/* flush list of objects */
int c0appz_flush(struct m0_uint128* id, size_t size);
#endif


int c0appz_generate_id(int64_t *idh, int64_t *idl);

#ifdef __cplusplus
}
#endif

int create_object(struct m0_uint128 id);

#endif /* __C0APPZ_H__ */


/*
 *  Local variables:
 *  c-indentation-style: "K&R"
 *  c-basic-offset: 8
 *  tab-width: 8
 *  fill-column: 80
 *  scroll-step: 1
 *  End:
 */

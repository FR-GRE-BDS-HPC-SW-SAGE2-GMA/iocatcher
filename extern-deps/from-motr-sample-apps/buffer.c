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
 * Original creation date: 21-May-2020
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/user.h>		/* PAGE_SIZE */
#include <pthread.h>
#include <inttypes.h>
#include <assert.h>

#include "motr/client.h"
#include "layout/layout.h"	/* M0_DEFAULT_LAYOUT_ID */
//#include "c0appz.h"
#include "clovis_api.h"
#include "c0appz_internal.h"

/*
 ******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************
 */
extern int qos_total_weight;	/* total bytes read or written  */
extern pthread_mutex_t qos_lock;	/* lock  qos_total_weight */

/*
 ******************************************************************************
 * STATIC FUNCTION PROTOTYPES
 ******************************************************************************
 */

static uint64_t bufvecw(struct m0_bufvec *data, const char *buf,
			uint64_t bsz, uint32_t cnt);
static uint64_t bufvecr(const struct m0_bufvec *data, char *buf,
			uint64_t bsz, uint32_t cnt);

/*
 ******************************************************************************
 * EXTERN FUNCTIONS
 ******************************************************************************
 */

/*
 * c0appz_fr()
 * read from file.
 * copy cnt number of bsz size of blocks from input
 * file inf to a memory buffer buf. Assume buf is large
 * enough to copy bsz*cnt bytes.
 */
int c0appz_fr(char *buf, char *inf, uint64_t bsz, uint64_t cnt)
{
	uint64_t n;
	FILE *fp;

	/* open input file */
	fp = fopen(inf, "rb");
	if (fp == NULL) {
		fprintf(stderr, "%s(): error! - ", __FUNCTION__);
		fprintf(stderr, "could not open input file %s: %s\n",
			inf, strerror(errno));
		return 11;
	}

	/* read into buf */
	n = fread(buf, 1, bsz * cnt, fp);
	if (n <= bsz * (cnt - 1)) {
		fprintf(stderr, "%s(): error! - ", __FUNCTION__);
		fprintf(stderr, "reading from %s failed: expected at least %lu "
			", but read %lu bytes\n", inf, bsz * (cnt - 1) + 1, n);
		fclose(fp);
		return 22;
	}

	/* success */
	fclose(fp);
	return 0;
}

/*
 * c0appz_fw()
 * write to file.
 * copy cnt number of bsz size of blocks from input
 * memory buffer to output file ouf. Assume ouf is
 * large enough to copy bsz*cnt bytes.
 */
int c0appz_fw(char *buf, char *ouf, uint64_t bsz, uint64_t cnt)
{
	uint64_t n;
	FILE *fp = NULL;

	/* open output file */
	fp = fopen(ouf, "wb");
	if (!fp) {
		ERRS("could not open input file %s", ouf);
		return 11;
	}

	/* write to ouf */
	n = fwrite(buf, bsz, cnt, fp);
	if (n != cnt) {
		ERRS("writing to %s failed", ouf);
		fclose(fp);
		return 22;
	}

	/* success */
	fclose(fp);
	return 0;
}

/*
 * c0appz_mr()
 * Reads data from a motr object to memory buffer.
 * Reads cnt number of blocks, each of size bsz starting at
 * off (byte) offset of the object.
 */
int c0appz_mr(char *buf, uint64_t idhi, uint64_t idlo, uint64_t off,
	      uint64_t bsz, uint64_t cnt, uint64_t m0bs)
{
	int rc;
	unsigned cnt_per_op;
	struct m0_uint128 id = { idhi, idlo };
	struct m0_obj obj = { };
	struct m0_indexvec ext;
	struct m0_bufvec   data;
	struct m0_bufvec   attr;

	CHECK_BSZ_ARGS(bsz, m0bs);

	cnt_per_op = m0bs / bsz;

	rc = alloc_segs(&data, &ext, &attr, bsz, cnt_per_op);
	if (rc != 0) {
		fprintf(stderr, "%s(): error! not enough memory\n",
			__func__);
		return rc;
	}

	/* Set the object entity we want to write */
	m0_obj_init(&obj, &uber_realm, &id,
			   M0_DEFAULT_LAYOUT_ID);

	rc = open_entity(&obj.ob_entity);
	if (rc != 0) {
		fprintf(stderr, "%s(): open_entity() failed: rc=%d\n",
			__func__, rc);
		goto free;
	}

	for (; cnt > 0; cnt -= cnt_per_op) {
		if (cnt < cnt_per_op)
			cnt_per_op = cnt;

		off += set_exts(&ext, off, bsz);

		rc = read_data_from_object(&obj, &ext, &data, &attr);
		if (rc != 0) {
			fprintf(stderr,
				"%s(): reading object failed: rc=%d\n",
				__func__, rc);
			free_segs(&data, &ext, &attr);
			break;
		}

		/* copy to memory */
		buf += bufvecr(&data, buf, bsz, cnt_per_op);

		/* QOS */
		pthread_mutex_lock(&qos_lock);
		qos_total_weight += cnt_per_op * bsz;
		pthread_mutex_unlock(&qos_lock);
		/* END */
	}
 free:
	free_segs(&data, &ext, &attr);
	m0_entity_fini(&obj.ob_entity);

	return rc;
}

/*
 * c0appz_mw()
 * Writes data from memory buffer to a motr object.
 * Writes cnt number of blocks, each of size bsz starting at
 * off (byte) offset of the object.
 */
int c0appz_mw(const char *buf, uint64_t idhi, uint64_t idlo, uint64_t off,
	      uint64_t bsz, uint64_t cnt, uint64_t m0bs)
{
	int rc;
	unsigned cnt_per_op;
	struct m0_uint128 id = { idhi, idlo };
	struct m0_obj obj = { };
	struct m0_indexvec ext;
	struct m0_bufvec   data;
	struct m0_bufvec   attr;

	CHECK_BSZ_ARGS(bsz, m0bs);

	cnt_per_op = m0bs / bsz;

	rc = alloc_segs(&data, &ext, &attr, bsz, cnt_per_op);
	if (rc != 0) {
		fprintf(stderr, "%s(): error! not enough memory\n",
			__func__);
		return rc;
	}

	/* Set the object entity we want to write */
	m0_obj_init(&obj, &uber_realm, &id,
			   M0_DEFAULT_LAYOUT_ID);

	rc = open_entity(&obj.ob_entity);
	if (rc != 0) {
		fprintf(stderr, "%s(): open_entity() failed: rc=%d\n",
			__func__, rc);
		goto free;
	}

	for (; cnt > 0; cnt -= cnt_per_op) {
		if (cnt < cnt_per_op)
			cnt_per_op = cnt;

		off += set_exts(&ext, off, bsz);
		buf += bufvecw(&data, buf, bsz, cnt_per_op);
		rc = write_data_to_object(&obj, &ext, &data, &attr);
		if (rc != 0) {
			fprintf(stderr,
				"%s(): writing object failed: rc=%d\n",
				__func__, rc);
			free_segs(&data, &ext, &attr);
			break;
		}

		/* QOS */
		pthread_mutex_lock(&qos_lock);
		qos_total_weight += cnt_per_op * bsz;
		pthread_mutex_unlock(&qos_lock);
		/* END */
	}
 free:
	free_segs(&data, &ext, &attr);
	m0_entity_fini(&obj.ob_entity);

	return rc;
}

/*
 * c0appz_mw_async()
 * Writes data from memory buffer to a motr object in async mode op_cnt
 * operations at a time. Writes cnt number of blocks, each of size
 * bsz at off (byte) offset of the object.
 */
int c0appz_mw_async(const char *buf, uint64_t idhi, uint64_t idlo, uint64_t off,
		    uint64_t bsz, uint64_t cnt, uint32_t op_cnt, uint64_t m0bs)
{
	int                       rc = 0;
	uint32_t                  i;
	uint32_t                  nr_ops_sent;
	uint32_t                  cnt_per_op;
	struct m0_uint128         id = {idhi, idlo};
	struct m0_aio_op     *aio;
	struct m0_aio_opgrp   aio_grp;

	CHECK_BSZ_ARGS(bsz, m0bs);

	cnt_per_op = m0bs / bsz;

	/* Initialise operation group. */
	rc = m0_aio_opgrp_init(&aio_grp, bsz, cnt_per_op, op_cnt);
	if (rc != 0) {
		fprintf(stderr, "%s(): m0_aio_opgrp_init() failed: rc=%d\n",
			__func__, rc);
		return rc;
	}

	/* Open the object. */
	m0_obj_init(&aio_grp.cag_obj, &uber_realm,
			   &id, M0_DEFAULT_LAYOUT_ID);
	rc = open_entity(&aio_grp.cag_obj.ob_entity);
	if (rc != 0) {
		fprintf(stderr, "%s(): open_entity() failed: rc=%d\n",
			__func__, rc);
		goto fini;
	}

	while (cnt > 0) {
		/* Set each op. */
		nr_ops_sent = 0;
		for (i = 0; i < op_cnt && cnt > 0; i++, cnt -= cnt_per_op) {
			aio = &aio_grp.cag_aio_ops[i];
			if (cnt < cnt_per_op)
				cnt_per_op = cnt;

			off += set_exts(&aio->cao_ext, off, bsz);
			buf += bufvecw(&aio->cao_data, buf, bsz, cnt_per_op);

			/* Launch IO op. */
			rc = write_data_to_object_async(aio);
			if (rc != 0) {
				fprintf(stderr, "%s(): writing to object failed\n",
					__func__);
				break;
			}
			nr_ops_sent++;

			/* QOS */
			pthread_mutex_lock(&qos_lock);
			qos_total_weight += cnt_per_op * bsz;
			pthread_mutex_unlock(&qos_lock);
			/* END */
		}

		/* Wait for all ops to complete. */
		for (i = 0; i < nr_ops_sent; i++)
			m0_semaphore_down(&aio_grp.cag_sem);

		/* Finalise ops and group. */
		rc = rc ?: aio_grp.cag_rc;
		for (i = 0; i < nr_ops_sent; i++)
			m0_aio_op_fini_free(aio_grp.cag_aio_ops + i);

		/* Not all ops are launched and executed successfully. */
		if (rc != 0)
			break;
	}
 fini:
	m0_entity_fini(&aio_grp.cag_obj.ob_entity);
	m0_aio_opgrp_fini(&aio_grp);

	return rc;
}


/******************************************************************************
 * STATIC FUNCTIONS
 ******************************************************************************/

static uint64_t bufvecw(struct m0_bufvec *data, const char *buf,
			uint64_t bsz, uint32_t cnt)
{
	uint32_t i;

	if (cnt > data->ov_vec.v_nr)
		cnt = data->ov_vec.v_nr;
	/* copy block by block */
	for (i = 0; i < cnt; i++) {
		memmove(data->ov_buf[i], buf, bsz);
		buf += bsz;
	}

	return i * bsz;
}

static uint64_t bufvecr(const struct m0_bufvec *data, char *buf,
			uint64_t bsz, uint32_t cnt)
{
	uint32_t i;

	if (cnt > data->ov_vec.v_nr)
		cnt = data->ov_vec.v_nr;
	/* copy block by block */
	for (i = 0; i < cnt; i++) {
		memmove(buf, data->ov_buf[i], bsz);
		buf += bsz;
	}

	return i * bsz;
}

/*
 *  Local variables:
 *  c-indentation-style: "K&R"
 *  c-basic-offset: 8
 *  tab-width: 8
 *  fill-column: 80
 *  scroll-step: 1
 *  End:
 */

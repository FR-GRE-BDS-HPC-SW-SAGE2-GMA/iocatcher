/* -*- C -*- */
/*
 * COPYRIGHT 2014 SEAGATE LLC
 *
 * THIS DRAWING/DOCUMENT, ITS SPECIFICATIONS, AND THE DATA CONTAINED
 * HEREIN, ARE THE EXCLUSIVE PROPERTY OF SEAGATE LLC,
 * ISSUED IN STRICT CONFIDENCE AND SHALL NOT, WITHOUT
 * THE PRIOR WRITTEN PERMISSION OF SEAGATE TECHNOLOGY LIMITED,
 * BE REPRODUCED, COPIED, OR DISCLOSED TO A THIRD PARTY, OR
 * USED FOR ANY PURPOSE WHATSOEVER, OR STORED IN A RETRIEVAL SYSTEM
 * EXCEPT AS ALLOWED BY THE TERMS OF SEAGATE LICENSES AND AGREEMENTS.
 *
 * YOU SHOULD HAVE RECEIVED A COPY OF SEAGATE'S LICENSE ALONG WITH
 * THIS RELEASE. IF NOT PLEASE CONTACT A SEAGATE REPRESENTATIVE
 * http://www.xyratex.com/contact
 *
 * Original author:  Ganesan Umanesan <ganesan.umanesan@seagate.com>
 * Original creation date: 10-Jan-2017
 *
 * Subsequent Modifications: Abhishek Saha <abhishek.saha@seagate.com>
 * Date of Modification: 02-Nov-2018
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include <assert.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <lib/semaphore.h>
#include <lib/trace.h>
#include <lib/memory.h>
#include <lib/mutex.h>

#include "clovis_api.h"
#include "clovis/clovis.h"
#include "clovis/clovis_idx.h"

/* variables */
struct m0_clovis          *clovis_instance = NULL;
struct m0_clovis_container clovis_container;
struct m0_clovis_realm     clovis_uber_realm;
struct m0_clovis_config    clovis_conf;
struct m0_idx_dix_config   dix_conf;

char    c0rc[8][SZC0RCSTR];
struct  timeval wclk_t = {0, 0};
clock_t cput_t = 0;


/*
 ******************************************************************************
 * EXTERN FUNCTIONS
 ******************************************************************************
 */

/*
 * c0appz_timeout()
 * time out execution.
 */
int c0appz_timeout(uint64_t sz)
{
	double ct;        /* cpu time in seconds  */
	double wt;        /* wall time in seconds */
	double bw_ctime;  /* bandwidth in MBs     */
	double bw_wtime;  /* bandwidth in MBs     */
	struct timeval tv;

	/* cpu time */
	ct = (double)(clock() - cput_t) / CLOCKS_PER_SEC;
	bw_ctime = (double)(sz) / 1000000.0 / ct;

	/* wall time */
	gettimeofday(&tv, 0);
	wt  = (double)(tv.tv_sec - wclk_t.tv_sec);
	wt += (double)(tv.tv_usec - wclk_t.tv_usec)/1000000;
	bw_wtime  = (double)(sz) / 1000000.0 / wt;


	fprintf(stderr,"[ cput: %08.4lf s %08.4lf MB/s ]", ct, bw_ctime);
	fprintf(stderr,"[ wclk: %08.4lf s %08.4lf MB/s ]", wt, bw_wtime);
	fprintf(stderr,"\n");
	return 0;
}

/*
 * c0appz_timein()
 * time in execution.
 */
int c0appz_timein()
{
	cput_t = clock();
	gettimeofday(&wclk_t, 0);
	return 0;
}

/*
 * c0appz_cp()
 * copy to an object.
 */
int c0appz_cp(int64_t idhi, int64_t idlo, char *filename, int bsz, int cnt)
{
	int                i;
	int                rc;
	int                max_bcnt_per_op;
	int                block_count;
	int                bcnt_read;
	uint64_t           last_index;
	struct m0_uint128  id;
	struct m0_indexvec ext;
	struct m0_bufvec   data;
	struct m0_bufvec   attr;
	FILE              *fp;
	m0_time_t          st;
	m0_time_t          read_time;
	m0_time_t          write_time;
	double             time;
	double             fs_bw;
	double             clovis_bw;

	/* open src file */
	fp = fopen(filename, "rb");
	if (fp == NULL) {
		fprintf(stderr,"error! could not open input file %s\n", filename);
		return 1;
	}

	/* ids */
	id.u_hi = idhi;
	id.u_lo = idlo;

	/* create object */
	rc = create_object(id);
	if (rc != 0) {
		fprintf(stderr, "can't create object!\n");
		fclose(fp);
		return rc;
	}

	/* Loop for WRITE ops */
	max_bcnt_per_op = CLOVIS_MAX_PER_WIO_SIZE / bsz;
	assert(max_bcnt_per_op != 0);
	last_index = 0;
	M0_SET0(&read_time);
	M0_SET0(&write_time);
	while (cnt > 0) {
		block_count = (cnt > CLOVIS_MAX_BLOCK_COUNT)?
			       CLOVIS_MAX_BLOCK_COUNT:cnt;

		if (block_count > max_bcnt_per_op)
			block_count = max_bcnt_per_op;

		/* Allocate data buffers, bufvec and indexvec for write. */
		rc = m0_bufvec_alloc(&data, block_count, bsz) ?:
		     m0_bufvec_alloc(&attr, block_count, 1) ?:
		     m0_indexvec_alloc(&ext, block_count);
		if (rc != 0)
			goto free_vecs;

		/*
		 * Prepare indexvec for write: <clovis_block_count> from the
		 * beginning of the object.
		 */
		for (i = 0; i < block_count; i++) {
			ext.iv_index[i] = last_index ;
			ext.iv_vec.v_count[i] = bsz;
			last_index += bsz;

			/* we don't want any attributes */
			attr.ov_vec.v_count[i] = 0;
		}

		/* Read data from source file. */
		st = m0_time_now();
		bcnt_read = read_data_from_file(fp, &data, bsz);
		if (bcnt_read != block_count) {
			fprintf(stderr, "reading from file failed!\n");
			rc = -EIO;
			goto free_vecs;
		}
		read_time = m0_time_add(read_time,
					m0_time_sub(m0_time_now(), st));

		/* Copy data to the object*/
		st = m0_time_now();
		rc = write_data_to_object(id, &ext, &data, &attr);
		if (rc != 0)
			fprintf(stderr, "writing to object failed!\n");
		write_time = m0_time_add(write_time,
					 m0_time_sub(m0_time_now(), st));

free_vecs:
		/* Free bufvec's and indexvec's */
		m0_indexvec_free(&ext);
		m0_bufvec_free(&data);
		m0_bufvec_free(&attr);
		if (rc != 0)
			break;

		cnt -= block_count;
	}
	fclose(fp);

	if (rc == 0) {
		time = (double) read_time / M0_TIME_ONE_SECOND;
		fs_bw = last_index / 1000000.0 / time;
		fprintf(stderr," i/o[ read from fs: %08.4lf s %08.4lf MB/s ]",
			time, fs_bw);

		time = (double) write_time / M0_TIME_ONE_SECOND;
		clovis_bw = last_index / 1000000.0 / time;
		fprintf(stderr,"[ write to mero: %08.4lf s %08.4lf MB/s ]\n",
			time, clovis_bw);
	}
	return rc;
}

/*
 * c0appz_cp_async()
 * copy to and object in an async manner
 */
int c0appz_cp_async(int64_t idhi, int64_t idlo, char *src, int block_size,
                    int block_count, int op_cnt)
{
	int                       i;
	int                       j;
	int                       rc = 0;
	int                       bcnt_per_op;
	int                       max_bcnt_per_op;
	int                       nr_ops_sent;
	uint64_t                  last_index;
	struct m0_uint128         id;
	struct clovis_aio_op     *aio;
	struct clovis_aio_opgrp   aio_grp;
	FILE                      *fp;

	assert(block_count % op_cnt == 0);
	assert(CLOVIS_MAX_PER_WIO_SIZE > block_size);
	max_bcnt_per_op = CLOVIS_MAX_PER_WIO_SIZE / block_size >
			  CLOVIS_MAX_BLOCK_COUNT ?
			  CLOVIS_MAX_BLOCK_COUNT :
			  CLOVIS_MAX_PER_WIO_SIZE / block_size;

	/* Open source file */
	fp = fopen(src, "rb");
	if (fp == NULL)
		return -1;

	/* ids */
	id.u_hi = idhi;
	id.u_lo = idlo;

	/* Create the object */
	rc = create_object(id);
	if (rc != 0) {
		fclose(fp);
		return rc;
	}

	last_index = 0;
	while (block_count > 0) {
		bcnt_per_op = block_count > max_bcnt_per_op * op_cnt?
			      max_bcnt_per_op : block_count / op_cnt;

		/* Initialise operation group. */
		rc = clovis_aio_opgrp_init(
			&aio_grp, bcnt_per_op * op_cnt, op_cnt);
		if (rc != 0) {
			fclose(fp);
			return rc;
		}

		/* Open the object. */
		m0_clovis_obj_init(&aio_grp.cag_obj,
			&clovis_uber_realm, &id,
			 m0_clovis_layout_id(clovis_instance));
		open_entity(&aio_grp.cag_obj.ob_entity);

		/* Set each op. */
		nr_ops_sent = 0;
		for (i = 0; i < op_cnt; i++) {
			aio = &aio_grp.cag_aio_ops[i];
			rc = clovis_aio_vec_alloc(aio, bcnt_per_op, block_size);
			if (rc != 0)
				break;

			for (j = 0; j < bcnt_per_op; j++) {
				aio->cao_ext.iv_index[j] = last_index;
				aio->cao_ext.iv_vec.v_count[j] = block_size;
				last_index += block_size;

				aio->cao_attr.ov_vec.v_count[j] = 0;
			}

			/* Read data from source file. */
			rc = read_data_from_file(
				fp, &aio->cao_data, block_size);
			M0_ASSERT(rc == bcnt_per_op);

			/* Launch IO op. */
			rc = write_data_to_object_async(aio);
			if (rc != 0) {
				fprintf(stderr, "Writing to object failed!\n");
				break;
			}
			nr_ops_sent++;
		}

		/* Wait for all ops to complete. */
		for (i = 0; i < nr_ops_sent; i++)
			m0_semaphore_down(&aio_grp.cag_sem);

		/* Finalise ops and group. */
		rc = rc ?: aio_grp.cag_rc;
		for (i = 0; i < nr_ops_sent; i++) {
			aio = &aio_grp.cag_aio_ops[i];
			m0_clovis_op_fini(aio->cao_op);
			m0_clovis_op_free(aio->cao_op);
			clovis_aio_vec_free(aio);
		}
		m0_clovis_entity_fini(&aio_grp.cag_obj.ob_entity);
		clovis_aio_opgrp_fini(&aio_grp);

		/* Not all ops are launched and executed successfully. */
		if (rc != 0)
			break;

		block_count -= op_cnt * bcnt_per_op;
	}

	fclose(fp);
	return rc;
}

/*
 * c0appz_cat()
 * cat object.
 */
int c0appz_cat(int64_t idhi, int64_t idlo, int bsz, int cnt, char *filename)
{
	int                i;
	int                rc=0;
	int                max_bcnt_per_op;
	int                block_count;
	int                bcnt_written;
	uint64_t           last_index;
	struct m0_uint128  id;
	struct m0_indexvec ext;
	struct m0_bufvec   data;
	struct m0_bufvec   attr;
	FILE              *fp;
	m0_time_t          st;
	m0_time_t          read_time;
	m0_time_t          write_time;
	double             time;
	double             fs_bw;
	double             clovis_bw;

	/* open output file */
	fp = fopen(filename, "w");
	if (fp == NULL) {
		fprintf(stderr,"error! could not open output file %s\n", filename);
		return 1;
	}

	/* ids */
	id.u_hi = idhi;
	id.u_lo = idlo;

	/* Compute maximum number of block per op. */
	max_bcnt_per_op = CLOVIS_MAX_PER_RIO_SIZE / bsz;
	assert(max_bcnt_per_op != 0);
	max_bcnt_per_op = max_bcnt_per_op < CLOVIS_MAX_BLOCK_COUNT ?
			  max_bcnt_per_op :
			  CLOVIS_MAX_BLOCK_COUNT;

	/* Loop for READ ops */
	last_index = 0;
	M0_SET0(&read_time);
	M0_SET0(&write_time);
	while (cnt > 0) {
		block_count = cnt > max_bcnt_per_op? max_bcnt_per_op : cnt;

		/* Allocate data buffers, bufvec and indexvec for write. */
		rc = m0_bufvec_alloc(&data, block_count, bsz) ?:
		     m0_bufvec_alloc(&attr, block_count, 1) ?:
		     m0_indexvec_alloc(&ext, block_count);
		if (rc != 0)
			goto free_vecs;

		/*
		 * Prepare indexvec for write: <clovis_block_count> from the
		 * beginning of the object.
		 */
		for (i = 0; i < block_count; i++) {
			ext.iv_index[i] = last_index ;
			ext.iv_vec.v_count[i] = bsz;
			last_index += bsz;

			/* we don't want any attributes */
			attr.ov_vec.v_count[i] = 0;
		}

		/* Copy data from the object*/
		st = m0_time_now();
		rc = read_data_from_object(id, &ext, &data, &attr);
		read_time = m0_time_add(read_time,
					m0_time_sub(m0_time_now(), st));
		if (rc != 0) {
			fprintf(stderr, "writing to object failed!\n");
			goto free_vecs;
		}

		/* Output data to file. */
		st = m0_time_now();
		bcnt_written = write_data_to_file(fp, &data, bsz);
		write_time = m0_time_add(write_time,
					 m0_time_sub(m0_time_now(), st));
		if (bcnt_written != block_count)
			rc = -EIO;

free_vecs:
		/* Free bufvec's and indexvec's */
		m0_indexvec_free(&ext);
		m0_bufvec_free(&data);
		m0_bufvec_free(&attr);
		if (rc != 0)
			break;

		cnt -= block_count;
	}

	/* Fs will flush data back to device when closing a file. */
	st = m0_time_now();
	fclose(fp);
	write_time = m0_time_add(write_time,
				 m0_time_sub(m0_time_now(), st));

	if (rc == 0) {
		time = (double) read_time / M0_TIME_ONE_SECOND;
		clovis_bw = last_index / 1000000.0 / time;
		fprintf(stderr," i/o[ read from mero: %08.4lf s %08.4lf MB/s ]",
			time, clovis_bw);

		time = (double) write_time / M0_TIME_ONE_SECOND;
		fs_bw = last_index / 1000000.0 / time;
		fprintf(stderr,"[ write to fs: %08.4lf s %08.4lf MB/s ]\n",
			time, fs_bw);
	}
	return rc;
}

/*
 * c0appz_rm()
 * delete object.
 */
int c0appz_rm(int64_t idhi, int64_t idlo)
{
	int                  rc;
	struct m0_clovis_obj obj;
	struct m0_clovis_op *ops[1] = {NULL};
	struct m0_uint128    id;

	id.u_hi = idhi;
	id.u_lo = idlo;

	memset(&obj, 0, sizeof(struct m0_clovis_obj));
	m0_clovis_obj_init(&obj, &clovis_uber_realm, &id,
			   m0_clovis_layout_id(clovis_instance));
	rc = open_entity(&obj.ob_entity);
	if (rc < 0) {
		fprintf(stderr,"error! [%d]\n", rc);
		fprintf(stderr,"object not found\n");
		return rc;
	}

	m0_clovis_entity_delete(&obj.ob_entity, &ops[0]);
	m0_clovis_op_launch(ops, ARRAY_SIZE(ops));
	rc = m0_clovis_op_wait(ops[0], M0_BITS(M0_CLOVIS_OS_FAILED,
					       M0_CLOVIS_OS_STABLE),
				   M0_TIME_NEVER);

	m0_clovis_op_fini(ops[0]);
	m0_clovis_op_free(ops[0]);
	m0_clovis_entity_fini(&obj.ob_entity);

	return rc;
}

/*
 * c0appz_init()
 * init clovis resources.
 */
int c0appz_init(int idx, char *ressource_file)
{
	int   i;
	int   rc;
	FILE *fp;
	char *str = NULL;
	char  buf[SZC0RCSTR];

	/* read c0rc file */
	char* filename = ressource_file;
	fp = fopen(filename, "r");
	if (fp == NULL) {
		fprintf(stderr,"error! could not open resource file %s\n", filename);
		return 1;
	}

	/* move fp */
	i = 0;
	while ((i != idx * MAXC0RC) && (fgets(buf, SZC0RCSTR, fp) != NULL)) {
		str = trim(buf);
		if (str[0] == '#') continue;     /* ignore comments */
		if (strlen(str) == 0) continue;  /* ignore empty space */
		i++;
	}

	/* read c0rc */
	i = 0;
	while (fgets(buf, SZC0RCSTR, fp) != NULL) {

#if DEBUG
	fprintf(stderr,"rd:->%s<-", buf);
	fprintf(stderr,"\n");
#endif

	str = trim(buf);
	if (str[0] == '#') continue;		/* ignore comments */
	if (strlen(str) == 0) continue;		/* ignore empty space */

	memset(c0rc[i], 0x00, SZC0RCSTR);
	strncpy(c0rc[i], str, SZC0RCSTR);

#if DEBUG
	fprintf(stderr,"wr:->%s<-", c0rc[i]);
	fprintf(stderr,"\n");
#endif

	i++;
	if (i == MAXC0RC) break;
	}
	fclose(fp);

	/* idx check */
	if (i != MAXC0RC) {
		fprintf(stderr,"error! [[ %d ]] wrong resource index.\n", idx);
		return 2;
	}

	clovis_conf.cc_is_oostore            = true;
	clovis_conf.cc_is_read_verify        = false;
	clovis_conf.cc_local_addr            = c0rc[0];
	clovis_conf.cc_ha_addr               = c0rc[1];
	clovis_conf.cc_profile               = c0rc[2];
	clovis_conf.cc_process_fid           = c0rc[3];
#if 0
	/* set to default values */
	clovis_conf.cc_tm_recv_queue_min_len = M0_NET_TM_RECV_QUEUE_DEF_LEN;
	clovis_conf.cc_max_rpc_msg_size      = M0_RPC_DEF_MAX_RPC_MSG_SIZE;
#endif
	/* set to Sage cluster specific values */
	clovis_conf.cc_tm_recv_queue_min_len = 64;
	clovis_conf.cc_max_rpc_msg_size      = 65536;
	clovis_conf.cc_layout_id             = 9;

	/* IDX_MERO */
	clovis_conf.cc_idx_service_id   = M0_CLOVIS_IDX_DIX;
	dix_conf.kc_create_meta = false;
	clovis_conf.cc_idx_service_conf = &dix_conf;

#if DEBUG
	fprintf(stderr,"\n---\n");
	fprintf(stderr,"%s,", (char *)clovis_conf.cc_local_addr);
	fprintf(stderr,"%s,", (char *)clovis_conf.cc_ha_addr);
	fprintf(stderr,"%s,", (char *)clovis_conf.cc_profile);
	fprintf(stderr,"%s,", (char *)clovis_conf.cc_process_fid);
	fprintf(stderr,"%s,", (char *)clovis_conf.cc_idx_service_conf);
	fprintf(stderr,"\n---\n");
#endif

	/* clovis instance */
	rc = m0_clovis_init(&clovis_instance, &clovis_conf, true);
	if (rc != 0) {
		fprintf(stderr,"failed to initilise Clovis\n");
		return rc;
	}

	/* And finally, clovis root realm */
	m0_clovis_container_init(&clovis_container,
				 NULL, &M0_CLOVIS_UBER_REALM,
				 clovis_instance);
	rc = clovis_container.co_realm.re_entity.en_sm.sm_rc;
	if (rc != 0) {
		fprintf(stderr,"failed to open uber realm\n");
		return rc;
	}

	/* success */
	clovis_uber_realm = clovis_container.co_realm;
	return 0;
}

/*
 * c0appz_free()
 * free clovis resources.
 */
int c0appz_free(void)
{
	m0_clovis_fini(clovis_instance, true);
	return 0;
}

/*
 ******************************************************************************
 * FUNCTIONS
 ******************************************************************************
 */

/*
 * open_entity()
 * open clovis entity.
 */
int open_entity(struct m0_clovis_entity *entity)
{
	int                  rc;
	struct m0_clovis_op *ops[1] = {NULL};

	m0_clovis_entity_open(entity, &ops[0]);
	m0_clovis_op_launch(ops, 1);
	m0_clovis_op_wait(ops[0], M0_BITS(M0_CLOVIS_OS_FAILED,
					  M0_CLOVIS_OS_STABLE),
			  M0_TIME_NEVER);
	rc = m0_clovis_rc(ops[0]);
	m0_clovis_op_fini(ops[0]);
	m0_clovis_op_free(ops[0]);
	ops[0] = NULL;

	return rc;
}

/*
 * create_object()
 * create clovis object.
 */
int create_object(struct m0_uint128 id)
{
	int                  rc;
	struct m0_clovis_obj obj;
	struct m0_clovis_op *ops[1] = {NULL};

	memset(&obj, 0, sizeof(struct m0_clovis_obj));

	m0_clovis_obj_init(&obj, &clovis_uber_realm, &id,
			   m0_clovis_layout_id(clovis_instance));

	rc = open_entity(&obj.ob_entity);
	if (!(rc < 0)) {
		fprintf(stderr,"Object already exists\n");
		return 1;
	}


	/*
	 * use default pool by default:
	 * struct m0_fid *pool = NULL
	 */
	rc = m0_clovis_entity_create(NULL, &obj.ob_entity, &ops[0]);
	if (rc < 0) {
		fprintf(stderr,"Error! [%d]\n", rc);
		fprintf(stderr,"Call to m0_clovis_entity_create failed\n");
		return 2;
	}
	/*
	 * use a Tier 2 SSD pool
	 */
/*
	struct m0_fid pool_fid;
	pool_fid.f_container = 0x6f00000000000001;
	pool_fid.f_key       = 0x3f8;
	m0_clovis_entity_create(&pool_fid, &obj.ob_entity, &ops[0]);
*/

	/*
	 * use a Tier 3 HDD pool
	 */
/*
	struct m0_fid pool_fid;
	pool_fid.f_container = 0x6f00000000000001;
	pool_fid.f_key       = 0x426;
	m0_clovis_entity_create(&pool_fid, &obj.ob_entity, &ops[0]);
*/

	m0_clovis_op_launch(ops, ARRAY_SIZE(ops));

	rc = m0_clovis_op_wait(ops[0], M0_BITS(M0_CLOVIS_OS_FAILED,
					       M0_CLOVIS_OS_STABLE),
			       m0_time_from_now(3,0));

	m0_clovis_op_fini(ops[0]);
	m0_clovis_op_free(ops[0]);
	m0_clovis_entity_fini(&obj.ob_entity);

	return 0;
}

/*
 * read_data_from_file()
 * reads data from file and populates buffer.
 */
int read_data_from_file(FILE *fp, struct m0_bufvec *data, int bsz)
{
	int i;
	int rc;
	int nr_blocks;

	nr_blocks = data->ov_vec.v_nr;
	for (i = 0; i < nr_blocks; i++) {
		rc = fread(data->ov_buf[i], bsz, 1, fp);
		if (rc != 1)
			break;

		if (feof(fp))
			break;
	}

	return i;
}

int write_data_to_file(FILE *fp, struct m0_bufvec *data, int bsz)
{
	int i;
	int rc;
	int nr_blocks;

	nr_blocks = data->ov_vec.v_nr;
	for (i = 0; i < nr_blocks; i++) {
		rc = fwrite(data->ov_buf[i], bsz, 1, fp);
		if (rc != 1)
			break;
	}

	return i;
}

/*
 * write_data_to_object()
 * writes data to an object
 */
int write_data_to_object(struct m0_uint128 id,
                                struct m0_indexvec *ext,
                                struct m0_bufvec *data,
                                struct m0_bufvec *attr)
{
	int                  rc;
	struct m0_clovis_obj obj;
	struct m0_clovis_op *ops[1] = {NULL};

	memset(&obj, 0, sizeof(struct m0_clovis_obj));

	/* Set the object entity we want to write */
	m0_clovis_obj_init(&obj, &clovis_uber_realm, &id,
			   m0_clovis_layout_id(clovis_instance));

	open_entity(&obj.ob_entity);

	/* Create the write request */
	m0_clovis_obj_op(&obj, M0_CLOVIS_OC_WRITE,
			 ext, data, attr, 0, &ops[0]);

	/* Launch the write request*/
	m0_clovis_op_launch(ops, 1);

	/* wait */
	rc = m0_clovis_op_wait(ops[0],
				M0_BITS(M0_CLOVIS_OS_FAILED,
				M0_CLOVIS_OS_STABLE),
				M0_TIME_NEVER);

	/* fini and release */
	m0_clovis_op_fini(ops[0]);
	m0_clovis_op_free(ops[0]);
	m0_clovis_entity_fini(&obj.ob_entity);

	return rc;
}

/*
 * write_data_to_object_async()
 * writes to and object in async manner
 */
int write_data_to_object_async(struct clovis_aio_op *aio)
{
	struct m0_clovis_obj    *obj;
	struct m0_clovis_op_ops  op_ops;
	struct clovis_aio_opgrp *grp;

	grp = aio->cao_grp;
	obj = &grp->cag_obj;

	/* Create an WRITE op. */
	m0_clovis_obj_op(obj, M0_CLOVIS_OC_WRITE,
			 &aio->cao_ext, &aio->cao_data,
			 &aio->cao_attr, 0, &aio->cao_op);
	aio->cao_op->op_datum = aio;

	/* Set op's Callbacks */
	op_ops.oop_executed = clovis_aio_executed_cb;
	op_ops.oop_stable = clovis_aio_stable_cb;
	op_ops.oop_failed = clovis_aio_failed_cb;
	m0_clovis_op_setup(aio->cao_op, &op_ops, 0);

	/* Launch the write request */
	m0_clovis_op_launch(&aio->cao_op, 1);

	return 0;
}

/*
 * read_data_from_object()
 * read data from an object
 */
int read_data_from_object(struct m0_uint128 id,
				 struct m0_indexvec *ext,
				 struct m0_bufvec *data,
				 struct m0_bufvec *attr)
{
	int                  rc;
	struct m0_clovis_obj obj;
	struct m0_clovis_op *ops[1] = {NULL};

	memset(&obj, 0, sizeof(struct m0_clovis_obj));

	/* Open object. */
	m0_clovis_obj_init(&obj, &clovis_uber_realm, &id,
			   m0_clovis_layout_id(clovis_instance));
	open_entity(&obj.ob_entity);

	/* Creat, launch and wait on an READ op. */
	m0_clovis_obj_op(&obj, M0_CLOVIS_OC_READ,
			 ext, data, attr, 0, &ops[0]);
	m0_clovis_op_launch(ops, 1);
	rc = m0_clovis_op_wait(ops[0],
				M0_BITS(M0_CLOVIS_OS_FAILED,
				M0_CLOVIS_OS_STABLE),
				M0_TIME_NEVER);
	rc = rc ?: m0_clovis_rc(ops[0]);

	/* Finalise and release op. */
	m0_clovis_op_fini(ops[0]);
	m0_clovis_op_free(ops[0]);
	m0_clovis_entity_fini(&obj.ob_entity);

	return rc;
}


int clovis_aio_vec_alloc(struct clovis_aio_op *aio,
				uint32_t blk_count, uint32_t blk_size)
{
	int rc = 0;

	rc = m0_bufvec_alloc(&aio->cao_data, blk_count, blk_size) ?:
	     m0_bufvec_alloc(&aio->cao_attr, blk_count, 1) ?:
	     m0_indexvec_alloc(&aio->cao_ext, blk_count);
	if (rc != 0) {
		m0_bufvec_free(&aio->cao_data);
		m0_bufvec_free(&aio->cao_attr);
	}
	return rc;
}

void clovis_aio_vec_free(struct clovis_aio_op *aio)
{
	/* Free bufvec's and indexvec's */
	m0_indexvec_free(&aio->cao_ext);
	m0_bufvec_free(&aio->cao_data);
	m0_bufvec_free(&aio->cao_attr);
}

int clovis_aio_opgrp_init(struct clovis_aio_opgrp *grp,
				 int blk_cnt, int op_cnt)
{
	int                   i;
	struct clovis_aio_op *ops;

	M0_SET0(grp);

	M0_ALLOC_ARR(ops, op_cnt);
	if (ops == NULL)
		return -ENOMEM;
	grp->cag_aio_ops = ops;
	for (i = 0; i < op_cnt; i++)
		grp->cag_aio_ops[i].cao_grp = grp;

	m0_mutex_init(&grp->cag_mlock);
	m0_semaphore_init(&grp->cag_sem, 0);
	grp->cag_blocks_to_write = blk_cnt;
	grp->cag_blocks_written = 0;

	return 0;
}

void clovis_aio_opgrp_fini(struct clovis_aio_opgrp *grp)
{
	m0_mutex_fini(&grp->cag_mlock);
	m0_semaphore_fini(&grp->cag_sem);
	grp->cag_blocks_to_write = 0;
	grp->cag_blocks_written = 0;

	m0_free(grp->cag_aio_ops);
}

void clovis_aio_executed_cb(struct m0_clovis_op *op)
{
	/** Stuff to do when OP is in excecuted state */
}

void clovis_aio_stable_cb(struct m0_clovis_op *op)
{
	struct clovis_aio_opgrp *grp;

	grp = ((struct clovis_aio_op *)op->op_datum)->cao_grp;
	m0_mutex_lock(&grp->cag_mlock);
	if (op->op_rc == 0) {
		grp->cag_blocks_written += 0;
		grp->cag_blocks_to_write   += 0;
		grp->cag_rc = grp->cag_rc ?: op->op_rc;
	}
	m0_mutex_unlock(&grp->cag_mlock);

	m0_semaphore_up(&grp->cag_sem);
}

void clovis_aio_failed_cb(struct m0_clovis_op *op)
{
	struct clovis_aio_opgrp *grp;

	m0_console_printf("Write operation failed!");
	grp = ((struct clovis_aio_op *)op->op_datum)->cao_grp;
	m0_mutex_lock(&grp->cag_mlock);
	grp->cag_rc = grp->cag_rc ?: op->op_rc;
	m0_mutex_unlock(&grp->cag_mlock);

	m0_semaphore_up(&grp->cag_sem);
}

/*
 ******************************************************************************
 * UTILITY FUNCTIONS
 ******************************************************************************
 */

/*
 * trim()
 * trim leading/trailing white spaces.
 */
char *trim(char *str)
{
	char *end;

	/* null */
	if (!str) return str;

	/* trim leading spaces */
	while (isspace((unsigned char)*str)) str++;

	/* all spaces */
	if (*str == 0) return str;

	/* trim trailing spaces, and */
	/* write new null terminator */
	end = str + strlen(str) - 1;
	while (end > str && isspace((unsigned char)*end)) end--;
	*(end + 1) = 0;

	/* success */
	return str;
}
/*
 ******************************************************************************
 * END
 ******************************************************************************
 */
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
 *
 * Subsequent Modifications: Abhishek Saha <abhishek.saha@seagate.com>
 * Date of Modification: 02-Nov-2018
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <lib/semaphore.h>
#include <lib/trace.h>
#include <lib/memory.h>
#include <lib/mutex.h>
#include <sys/user.h>           /* PAGE_SIZE */

#include "motr/client.h"
#include "motr/client_internal.h"
#include "conf/confc.h"       /* m0_confc_open_sync */
#include "conf/dir.h"         /* m0_conf_dir_len */
#include "conf/helpers.h"     /* m0_confc_root_open */
#include "conf/diter.h"       /* m0_conf_diter_next_sync */
#include "conf/obj_ops.h"     /* M0_CONF_DIREND */
#include "spiel/spiel.h"      /* m0_spiel_process_lib_load */
#include "reqh/reqh.h"        /* m0_reqh */
#include "lib/types.h"        /* uint32_t */

#include "clovis_api.h"
#include "c0appz_internal.h"
#include "iscservice/isc.h"
#include "lib/buf.h"
#include "rpc/rpclib.h"
#include "c0appz_isc.h"

#ifndef DEBUG
#define DEBUG 0
#endif

const char *c0appz_help_txt = "\
The utility requires startup configuration file with a few cluster-\n\
specific parameters which usually don't change until the cluster is\n\
reconfigured by the system administrator. For example:\n\
\n\
$ cat $HOME/.c0appz/c0cprc/client-22\n\
HA_ENDPOINT_ADDR = 172.18.1.22@o2ib:12345:34:101\n\
PROFILE_FID   = 0x7000000000000001:0xcfd\n\
M0_POOL_TIER1 = 0x6f00000000000001:0xc74 # tier1-nvme\n\
M0_POOL_TIER2 = 0x6f00000000000001:0xc8a # tier2-ssd\n\
M0_POOL_TIER3 = 0x6f00000000000001:0xca5 # tier3-hdd\n\
LOCAL_ENDPOINT_ADDR0 = 172.18.1.22@o2ib:12345:41:351\n\
LOCAL_PROC_FID0      = 0x7200000000000001:0x645\n\
\n\
c0cprc is the utility name (c0cp in this case) + rc suffix.\n\
client-22 is the client node name where the utility is run.\n\
HA_ENDPOINT_ADDR is the address of HA agent running on the node.\n\
LOCAL_ENDPOINT_ADDRx and LOCAL_PROC_FIDx addresses and fids are\n\
allocated by system administrator for each user's application\n\
working with the object store cluster.\n\
The easiest way to generate this file is like this:\n\
\n\
$ cd ~/m0client-sample-apps\n\
$ make c=22 sagercf\n\
\n\
Where 22 is the last byte in the client's node IP address.\n";


enum {
	MAX_M0_BUFSZ = 128*1024*1024, /* max bs for object store I/O  */
	MAX_POOLS = 16,
	MAX_RCFILE_NAME_LEN = 512,
	MAX_CONF_STR_LEN = 128,
	MAX_CONF_PARAMS = 32,
};

/* static variables */
struct m0_client          *m0_instance = NULL;
static struct m0_container container;
static struct m0_config    m0_conf;
static struct m0_idx_dix_config   dix_conf;
static struct m0_spiel            spiel_inst;

static char c0rcfile[MAX_RCFILE_NAME_LEN] = "./.c0appzrc";
char * prog = "ummapio";

/*
 ******************************************************************************
 * STATIC FUNCTION PROTOTYPES
 ******************************************************************************
 */
static size_t read_data_from_file(FILE *fp, struct m0_bufvec *data,
				  size_t bsz, uint32_t cnt);
static size_t write_data_to_file(FILE *fp, struct m0_bufvec *data,
				 size_t bsz, uint32_t cnt);

/*
 ******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************
 */
struct m0_realm uber_realm;
unsigned unit_size = 0;
int perf=0;				/* performance option 		*/
extern int qos_total_weight; 		/* total bytes read or written 	*/
extern pthread_mutex_t qos_lock;	/* lock  qos_total_weight 	*/
int trace_level=0;
bool m0trace_on = false;

struct param {
	char name[MAX_CONF_STR_LEN];
	char value[MAX_CONF_STR_LEN];
};

static struct m0_fid pools[MAX_POOLS] = {};

/*
 ******************************************************************************
 * STATIC FUNCTIONS
 ******************************************************************************
 */

/**
 * Read parameters into array p[] from file.
 */
static int read_params(FILE *in, struct param *p, int max_params)
{
	int ln, n=0;
	char s[MAX_CONF_STR_LEN];

	for (ln=1; max_params > 0 && fgets(s, MAX_CONF_STR_LEN, in); ln++) {
		if (sscanf(s, " %[#\n\r]", p->name))
			continue; /* skip emty line or comment */
		if (sscanf(s, " %[a-z_A-Z0-9] = %[^#\n\r]",
		           p->name, p->value) < 2) {
			ERR("error at line %d: %s\n", ln, s);
			return -1;
		}
		DBG("%d: name='%s' value='%s'\n", ln, p->name, p->value);
		p++, max_params--, n++;
	}

	return n;
}

static const char* param_get(const char *name, const struct param p[], int n)
{
	while (n-- > 0)
		if (strcmp(p[n].name, name) == 0)
			return p[n].value;
	return NULL;
}

/**
 * Set pools fids at pools[] from parameters array p[].
 */
static int pools_fids_set(struct param p[], int n)
{
	int i;
	char pname[32];
	const char *pval;

	for (i = 0; i < MAX_POOLS; i++) {
		sprintf(pname, "M0_POOL_TIER%d", i + 1);
		if ((pval = param_get(pname, p, n)) == NULL)
			break;
		if (m0_fid_sscanf(pval, pools + i) != 0) {
			ERR("failed to parse FID of %s\n", pname);
			return -1;
		}
	}

	return i;
}

/**
 * Return pool fid from pools[] at tier_idx.
 */
static struct m0_fid *tier2pool(uint8_t tier_idx)
{
	if (tier_idx < 1 || tier_idx > MAX_POOLS)
		return NULL;
	return pools + tier_idx - 1;
}

/* Warning: non-reentrant. */
static char* fid2str(const struct m0_fid *fid)
{
	static char buf[256];

	if (fid)
		sprintf(buf, FID_F, FID_P(fid));
	else
		sprintf(buf, "%p", fid);

	return buf;
}

static uint64_t roundup_power2(uint64_t x)
{
	uint64_t power = 1;

	while (power < x)
		power *= 2;

	return power;
}

/*
 ******************************************************************************
 * EXTERN FUNCTIONS
 ******************************************************************************
 */

/**
 * Calculate the optimal block size for the object store I/O
 */
uint64_t c0appz_m0bs(uint64_t idhi, uint64_t idlo, uint64_t obj_sz, int tier)
{
	int                     rc;
	unsigned long           usz; /* unit size */
	unsigned long           gsz; /* data units in parity group */
	uint64_t                max_bs;
	unsigned                lid;
	struct m0_reqh         *reqh = &m0_instance->m0c_reqh;
	struct m0_pool_version *pver;
	struct m0_pdclust_attr *pa;
	struct m0_fid          *pool = tier2pool(tier);
	struct m0_obj    obj;

	if (obj_sz > MAX_M0_BUFSZ)
		obj_sz = MAX_M0_BUFSZ;

	rc = m0_pool_version_get(reqh->rh_pools, pool, &pver);
	if (rc != 0) {
		ERR("m0_pool_version_get(pool=%s) failed: rc=%d\n",
		    fid2str(pool), rc);
		return 0;
	}

	if (c0appz_ex(idhi, idlo, &obj))
		lid = obj.ob_attr.oa_layout_id;
	else if (unit_size) /* set explicitly via -u option ? */
		lid = m0_client_layout_id(m0_instance);
	else
		lid = m0_layout_find_by_buffsize(&reqh->rh_ldom, &pver->pv_id,
						 obj_sz);

	usz = m0_obj_layout_id_to_unit_size(lid);
	pa = &pver->pv_attr;
	gsz = usz * pa->pa_N;
	/* max 2-times pool-width deep, otherwise we may get -E2BIG */
	max_bs = usz * 2 * pa->pa_P * pa->pa_N / (pa->pa_N + 2 * pa->pa_K);
	max_bs = max_bs / gsz * gsz; /* make it multiple of group size */

	DBG("usz=%lu (N,K,P)=(%u,%u,%u) max_bs=%lu\n",
	    usz, pa->pa_N, pa->pa_K, pa->pa_P, max_bs);

	if (obj_sz >= max_bs)
		return max_bs;
	else if (obj_sz <= gsz)
		return gsz;
	else
		return roundup_power2(obj_sz);
}

void free_segs(struct m0_bufvec *data, struct m0_indexvec *ext,
	       struct m0_bufvec *attr)
{
	m0_indexvec_free(ext);
	m0_bufvec_free(data);
	m0_bufvec_free(attr);
}

int alloc_segs(struct m0_bufvec *data, struct m0_indexvec *ext,
	       struct m0_bufvec *attr, uint64_t bsz, uint32_t cnt)
{
	int i, rc;

	rc = m0_bufvec_alloc(data, cnt, bsz) ?:
	     m0_bufvec_alloc(attr, cnt, 1) ?:
	     m0_indexvec_alloc(ext, cnt);
	if (rc != 0)
		goto err;

	for (i = 0; i < cnt; i++)
		attr->ov_vec.v_count[i] = 0; /* no attrs */

	return 0;
 err:
	free_segs(data, ext, attr);
	return rc;
}

uint64_t set_exts(struct m0_indexvec *ext, uint64_t off, uint64_t bsz)
{
	uint32_t i;

	for (i = 0; i < ext->iv_vec.v_nr; i++) {
		ext->iv_index[i] = off;
		ext->iv_vec.v_count[i] = bsz;
		off += bsz;
	}

	return i * bsz;
}

/*
 * c0appz_cp()
 * copy to an object.
 */
int c0appz_cp(uint64_t idhi, uint64_t idlo, char *filename,
	      uint64_t bsz, uint64_t cnt, uint64_t m0bs)
{
	int                rc=0;
	uint32_t           cnt_per_op;
	uint64_t           off = 0;
	struct m0_uint128  id = {idhi, idlo};
	struct m0_indexvec ext;
	struct m0_bufvec   data;
	struct m0_bufvec   attr;
	FILE              *fp;
	m0_time_t          st;
	m0_time_t          read_time = 0;
	m0_time_t          write_time = 0;
	double             time;
	double             fs_bw;
	double             client_bw;
	struct m0_obj obj = {};

	CHECK_BSZ_ARGS(bsz, m0bs);

	cnt_per_op = m0bs / bsz;

	/* Open src file */
	fp = fopen(filename, "rb");
	if (fp == NULL) {
		fprintf(stderr,"%s(): error on opening input file %s: %s\n",
			__func__, filename, strerror(errno));
		return -errno;
	}

	/* Allocate data buffers, bufvec and indexvec for write */
	rc = alloc_segs(&data, &ext, &attr, bsz, cnt_per_op);
	if (rc != 0)
		goto out;

	/* Open the object entity we want to write to */
	m0_obj_init(&obj, &uber_realm, &id, M0_DEFAULT_LAYOUT_ID);
	rc = open_entity(&obj.ob_entity);
	if (rc != 0) {
		fprintf(stderr, "%s(): open_entity() failed: rc=%d\n",
			__func__, rc);
		goto free_vecs;
	}

	for (; cnt > 0; cnt -= cnt_per_op) {
		if (cnt < cnt_per_op)
			cnt_per_op = cnt;

		/* Read data from source file. */
		st = m0_time_now();
		rc = read_data_from_file(fp, &data, bsz, cnt_per_op);
		if (rc != cnt_per_op) {
			fprintf(stderr, "%s(): reading from %s at %lu failed: "
				"expected %u, got %d records\n",
				__func__, filename, off, cnt_per_op, rc);
			rc = -EIO;
			break;
		}
		read_time = m0_time_add(read_time,
					m0_time_sub(m0_time_now(), st));

		DBG("IO block: off=0x%lx len=0x%lx\n", off, bsz);
		off += set_exts(&ext, off, bsz);

		/* Copy data to the object */
		st = m0_time_now();
		rc = write_data_to_object(&obj, &ext, &data, &attr);
		if (rc != 0) {
			ERR("writing to object failed: rc=%d\n", rc);
			break;
		}
		write_time = m0_time_add(write_time,
					 m0_time_sub(m0_time_now(), st));

		/* QOS */
		pthread_mutex_lock(&qos_lock);
		qos_total_weight += cnt_per_op * bsz;
		pthread_mutex_unlock(&qos_lock);
		/* END */
	}

	m0_entity_fini(&obj.ob_entity);
 free_vecs:
	free_segs(&data, &ext, &attr);
 out:
	fclose(fp);

	if (perf && rc == 0) {
		time = (double) read_time / M0_TIME_ONE_SECOND;
		fs_bw = off / 1000000.0 / time;
		ppf("Motr I/O[ \033[0;31mOSFS: %10.4lf s %10.4lf MB/s\033[0m ]",time, fs_bw);
		time = (double) write_time / M0_TIME_ONE_SECOND;
		client_bw = off / 1000000.0 / time;
		ppf("[ \033[0;31mMOTR: %10.4lf s %10.4lf MB/s\033[0m ]\n",time, client_bw);
	}

	return rc;
}

/*
 * c0appz_cp_async()
 * copy to an object from a file in an asynchronous manner
 */
int c0appz_cp_async(uint64_t idhi, uint64_t idlo, char *src, uint64_t bsz,
		    uint64_t cnt, uint32_t op_cnt, uint64_t m0bs)
{
	int                       rc = 0;
	uint32_t                  i;
	uint32_t                  nr_ops_sent;
	uint32_t                  cnt_per_op;
	uint64_t                  off = 0;
	struct m0_uint128         id = {idhi, idlo};
	struct m0_aio_op     *aio;
	struct m0_aio_opgrp   aio_grp;
	FILE                     *fp;

	CHECK_BSZ_ARGS(bsz, m0bs);

	cnt_per_op = m0bs / bsz;

	/* open file */
	fp = fopen(src, "rb");
	if (fp == NULL) {
		fprintf(stderr,"%s(): failed to open output file %s: %s\n",
			__func__, src, strerror(errno));
		return -errno;
	}

	/* Initialise operation group. */
	rc = m0_aio_opgrp_init(&aio_grp, bsz, cnt_per_op, op_cnt);
	if (rc != 0)
		goto out;

	/* Open the object. */
	m0_obj_init(&aio_grp.cag_obj, &uber_realm,
			   &id, M0_DEFAULT_LAYOUT_ID);
	rc = open_entity(&aio_grp.cag_obj.ob_entity);
	if (rc != 0) {
		fprintf(stderr, "%s(): open_entity() failed: rc=%d\n",
			__func__, rc);
		goto fini;
	}

	while (cnt > 0 && rc == 0) {
		/* Set each op. */
		for (i = 0, nr_ops_sent = 0; i < op_cnt && cnt > 0;
		     i++, cnt -= cnt_per_op, nr_ops_sent++) {
			aio = &aio_grp.cag_aio_ops[i];
			if (cnt < cnt_per_op)
				cnt_per_op = cnt;

			/* Read data from source file. */
			rc = read_data_from_file(fp, &aio->cao_data,
						 bsz, cnt_per_op);
			if (rc != cnt_per_op) {
				fprintf(stderr,
					"%s(): reading from %s at %lu failed: "
					"expected %u, read %u records\n",
					__func__, src, off, cnt_per_op, rc);
				rc = -EIO;
				break;
			}

			off += set_exts(&aio->cao_ext, off, bsz);

			/* Launch IO op. */
			rc = write_data_to_object_async(aio);
			if (rc != 0) {
				fprintf(stderr, "%s(): writing to object failed\n",
					__func__);
				break;
			}

			/* QOS */
			pthread_mutex_lock(&qos_lock);
			qos_total_weight += cnt_per_op * bsz;
			pthread_mutex_unlock(&qos_lock);
			/* END */
		}

		/* Wait for all ops to complete. */
		for (i = 0; i < nr_ops_sent; i++)
			m0_semaphore_down(&aio_grp.cag_sem);

		/* Finalise ops. */
		for (i = 0; i < nr_ops_sent; i++)
			m0_aio_op_fini_free(aio_grp.cag_aio_ops + i);

		rc = rc ?: aio_grp.cag_rc;
	}
 fini:
	m0_entity_fini(&aio_grp.cag_obj.ob_entity);
	m0_aio_opgrp_fini(&aio_grp);
 out:
	fclose(fp);
	return rc;
}

/*
 * c0appz_cat()
 * cat object.
 */
int c0appz_cat(uint64_t idhi, uint64_t idlo, char *filename,
	      uint64_t bsz, uint64_t cnt, uint64_t m0bs)
{
	int                rc=0;
	uint32_t           cnt_per_op;
	uint64_t           off = 0;
	struct m0_uint128  id = {idhi, idlo};
	struct m0_indexvec ext;
	struct m0_bufvec   data;
	struct m0_bufvec   attr;
	FILE              *fp;
	m0_time_t          st;
	m0_time_t          read_time = 0;
	m0_time_t          write_time = 0;
	double             time;
	double             fs_bw;
	double             client_bw;
	struct m0_obj obj = {};

	CHECK_BSZ_ARGS(bsz, m0bs);

	cnt_per_op = m0bs / bsz;

	/* open output file */
	fp = fopen(filename, "w");
	if (fp == NULL) {
		fprintf(stderr,"error! could not open output file %s\n", filename);
		return 1;
	}

	/* Allocate data buffers, bufvec and indexvec for write. */
	rc = alloc_segs(&data, &ext, &attr, bsz, cnt_per_op);
	if (rc != 0)
		goto out;

	/* Open object. */
	m0_obj_init(&obj, &uber_realm, &id, M0_DEFAULT_LAYOUT_ID);
	rc = open_entity(&obj.ob_entity);
	if (rc != 0) {
		fprintf(stderr, "%s(): open_entity() failed: rc=%d\n",
			__func__, rc);
		goto free_vecs;
	}

	for (; cnt > 0; cnt -= cnt_per_op) {
		if (cnt < cnt_per_op)
			cnt_per_op = cnt;

		off += set_exts(&ext, off, bsz);

		/* Copy data from the object*/
		st = m0_time_now();
		rc = read_data_from_object(&obj, &ext, &data, &attr);
		read_time = m0_time_add(read_time,
					m0_time_sub(m0_time_now(), st));
		if (rc != 0) {
			fprintf(stderr, "%s(): reading from object failed: rc=%d\n",
				__func__, rc);
			break;
		}

		/* Output data to file. */
		st = m0_time_now();
		if (write_data_to_file(fp, &data, bsz, cnt_per_op)
		    != cnt_per_op) {
			rc = -EIO;
			break;
		}
		write_time = m0_time_add(write_time,
					 m0_time_sub(m0_time_now(), st));

		/* QOS */
		pthread_mutex_lock(&qos_lock);
		qos_total_weight += cnt_per_op * bsz;
		pthread_mutex_unlock(&qos_lock);
		/* END */
	}

	m0_entity_fini(&obj.ob_entity);
free_vecs:
	free_segs(&data, &ext, &attr);
out:
	/* block */
	qos_pthread_cond_wait();
	fprintf(stderr,"writing to file...\n");

	/* Fs will flush data back to device when closing a file. */
	st = m0_time_now();
	fclose(fp);
	write_time = m0_time_add(write_time,
				 m0_time_sub(m0_time_now(), st));
	if (perf && rc == 0) {
		time = (double) read_time / M0_TIME_ONE_SECOND;
		client_bw = off / 1000000.0 / time;
		ppf("Motr I/O[ \033[0;31mMOTR: %10.4lf s %10.4lf MB/s\033[0m ]",time, client_bw);
		time = (double) write_time / M0_TIME_ONE_SECOND;
		fs_bw = off / 1000000.0 / time;
		ppf("[ \033[0;31mOSFS: %10.4lf s %10.4lf MB/s\033[0m ]\n",time, fs_bw);
	}

	return rc;
}

/*
 * c0appz_rm()
 * delete object.
 */
int c0appz_rm(uint64_t idhi,uint64_t idlo)
{
	int                  rc;
	struct m0_obj obj = {};
	struct m0_op *op = NULL;
	struct m0_uint128    id;

	id.u_hi = idhi;
	id.u_lo = idlo;

	m0_obj_init(&obj, &uber_realm, &id, M0_DEFAULT_LAYOUT_ID);
	rc = open_entity(&obj.ob_entity);
	if (rc < 0) {
		fprintf(stderr,"error! [%d]\n", rc);
		fprintf(stderr,"object not found\n");
		return rc;
	}

	m0_entity_delete(&obj.ob_entity, &op);
	m0_op_launch(&op, 1);
	rc = m0_op_wait(op, M0_BITS(M0_OS_FAILED,
					   M0_OS_STABLE),
				   M0_TIME_NEVER);

	m0_op_fini(op);
	m0_op_free(op);
	m0_entity_fini(&obj.ob_entity);

	return rc;
}

int c0appz_rmach_bulk_cutoff(struct m0_rpc_link *link, uint32_t *bulk_cutoff)
{
	if (link == NULL || bulk_cutoff == NULL)
		return -EINVAL;
	*bulk_cutoff = link->rlk_conn.c_rpc_machine->rm_bulk_cutoff;	
	return 0;
}

static int c0appz_spiel_prepare(struct m0_spiel *spiel)
{
	struct m0_reqh *reqh = &m0_instance->m0c_reqh;
	int             rc;

	rc = m0_spiel_init(spiel, reqh);
	if (rc != 0) {
		fprintf(stderr, "error! spiel initialisation failed.\n");
		return rc;
	}

	rc = m0_spiel_cmd_profile_set(spiel, m0_conf.mc_profile);
	if (rc != 0) {
		fprintf(stderr, "error! spiel initialisation failed.\n");
		return rc;
	}
	rc = m0_spiel_rconfc_start(spiel, NULL);
	if (rc != 0) {
		fprintf(stderr, "error! starting of rconfc failed in spiel failed.\n");
		return rc;
	}

	return 0;
}

static void c0appz_spiel_destroy(struct m0_spiel *spiel)
{
	m0_spiel_rconfc_stop(spiel);
	m0_spiel_fini(spiel);
}

static bool conf_obj_is_svc(const struct m0_conf_obj *obj)
{
	return m0_conf_obj_type(obj) == &M0_CONF_SERVICE_TYPE;
}

struct m0_rpc_link * c0appz_isc_rpc_link_get(struct m0_fid *svc_fid)
{
	struct m0_reqh *reqh = &m0_instance->m0c_reqh;
	struct m0_reqh_service_ctx *ctx;
	struct m0_pools_common *pc = reqh->rh_pools;

	m0_tl_for(pools_common_svc_ctx, &pc->pc_svc_ctxs, ctx) {
		if (ctx->sc_type == M0_CST_ISCS &&
		    m0_fid_eq(&ctx->sc_fid, svc_fid))
			return &ctx->sc_rlink;
	} m0_tl_endfor;

	return NULL;
}
/*
 * Loads a library into m0d instances.
 */
int c0appz_isc_api_register(const char *libpath)
{
	int                     rc;
	struct m0_reqh         *reqh = &m0_instance->m0c_reqh;
	struct m0_confc        *confc;
	struct m0_conf_root    *root;
	struct m0_conf_process *proc;
	struct m0_conf_service *svc;
	struct m0_conf_diter    it;

	rc = c0appz_spiel_prepare(&spiel_inst);
	if (rc != 0) {
		fprintf(stderr, "error! spiel initialization failed");
		return rc;
	}

	confc = m0_reqh2confc(reqh);
	rc = m0_confc_root_open(confc, &root);
	if (rc != 0) {
		c0appz_spiel_destroy(&spiel_inst);
		return rc;
	}

	rc = m0_conf_diter_init(&it, confc,
				&root->rt_obj,
				M0_CONF_ROOT_NODES_FID,
				M0_CONF_NODE_PROCESSES_FID,
				M0_CONF_PROCESS_SERVICES_FID);
	if (rc != 0) {
		m0_confc_close(&root->rt_obj);
		c0appz_spiel_destroy(&spiel_inst);
		return rc;
	}

	while (M0_CONF_DIRNEXT ==
	       (rc = m0_conf_diter_next_sync(&it, conf_obj_is_svc))) {

		svc = M0_CONF_CAST(m0_conf_diter_result(&it), m0_conf_service);
		if (svc->cs_type != M0_CST_ISCS)
			continue;
		proc = M0_CONF_CAST(m0_conf_obj_grandparent(&svc->cs_obj),
				    m0_conf_process);
		rc = m0_spiel_process_lib_load(&spiel_inst, &proc->pc_obj.co_id,
					       libpath);
		if (rc != 0) {
			fprintf(stderr, "error! loading the library %s failed "
				        "for process "FID_F": rc=%d\n", libpath,
					FID_P(&proc->pc_obj.co_id), rc);
			m0_conf_diter_fini(&it);
			m0_confc_close(&root->rt_obj);
			c0appz_spiel_destroy(&spiel_inst);
			return rc;
		}
	}
	m0_conf_diter_fini(&it);
	m0_confc_close(&root->rt_obj);
	c0appz_spiel_destroy(&spiel_inst);

	return 0;
}

int c0appz_isc_nxt_svc_get(struct m0_fid *svc_fid, struct m0_fid *nxt_fid,
			   enum m0_conf_service_type s_type)
{
	struct m0_reqh             *reqh       = &m0_instance->m0c_reqh;
	struct m0_reqh_service_ctx *ctx;
	struct m0_pools_common     *pc         = reqh->rh_pools;
	struct m0_fid               null_fid   = M0_FID0;
	struct m0_fid               current_fid = *svc_fid;

	m0_tl_for(pools_common_svc_ctx, &pc->pc_svc_ctxs, ctx) {
		if (ctx->sc_type == s_type && m0_fid_eq(&current_fid,
							&null_fid)) {
			*nxt_fid = ctx->sc_fid;
			return 0;
		} else if (ctx->sc_type == s_type &&
		           m0_fid_eq(svc_fid, &ctx->sc_fid))
			current_fid = null_fid;
	} m0_tl_endfor;
	*nxt_fid = M0_FID0;

	return -ENOENT;
}

int c0appz_isc_req_prepare(struct c0appz_isc_req *req, struct m0_buf *args,
			   const struct m0_fid *comp_fid,
			   struct m0_buf *reply_buf, struct m0_fid *svc_fid,
			   uint32_t reply_len)
{

	struct m0_fop_isc  *fop_isc = &req->cir_isc_fop;
	struct m0_fop      *arg_fop = &req->cir_fop;
	struct m0_rpc_link *link;
	int                 rc;

	req->cir_args = args;
	req->cir_result = reply_buf;
	fop_isc->fi_comp_id = *comp_fid;
	req->cir_rpc_link = c0appz_isc_rpc_link_get(svc_fid);
	if (req->cir_rpc_link == NULL) {
		fprintf(stderr, "error! cannot find rpc_link for isc service "
			FID_F, FID_P(svc_fid));
		return -EINVAL;
	}
	link = req->cir_rpc_link;
	m0_rpc_at_init(&fop_isc->fi_args);
	rc = m0_rpc_at_add(&fop_isc->fi_args, args, &link->rlk_conn);
	if (rc != 0) {
		m0_rpc_at_fini(&fop_isc->fi_args);
		fprintf(stderr, "error! m0_rpc_at_add() failed with %d\n", rc);
		return rc;
	}
	/* Initialise the reply RPC AT buffer to be received.*/
	m0_rpc_at_init(&fop_isc->fi_ret);
	rc = m0_rpc_at_recv(&fop_isc->fi_ret, &link->rlk_conn,
			    reply_len, false);
	if (rc != 0) {
		m0_rpc_at_fini(&fop_isc->fi_args);
		m0_rpc_at_fini(&fop_isc->fi_ret);
		fprintf(stderr, "error! m0_rpc_at_recv() failed with %d\n", rc);
		return rc;
	}
	m0_fop_init(arg_fop, &m0_fop_isc_fopt, fop_isc, m0_fop_release);

	return rc;
}

int c0appz_isc_req_send_sync(struct c0appz_isc_req *req)
{
	struct m0_fop         *reply_fop;
	struct m0_fop_isc_rep  isc_reply;
	int                    rc;

	rc = m0_rpc_post_sync(&req->cir_fop, &req->cir_rpc_link->rlk_sess, NULL,
			      M0_TIME_IMMEDIATELY);
	if (rc != 0) {
		fprintf(stderr, "Failed to send request to "FID_F": rc=%d\n",
			FID_P(&req->cir_proc), rc);
		return rc;
	}
	reply_fop = m0_rpc_item_to_fop(req->cir_fop.f_item.ri_reply);
	isc_reply = *(struct m0_fop_isc_rep *)m0_fop_data(reply_fop);
	req->cir_rc = isc_reply.fir_rc;
	if (req->cir_rc != 0)
		fprintf(stderr, "Got error from "FID_F": rc=%d\n",
			FID_P(&req->cir_proc), rc);
	rc = m0_rpc_at_rep_get(&req->cir_isc_fop.fi_ret, &isc_reply.fir_ret,
			       req->cir_result);
	if (rc != 0)
		fprintf(stderr, "rpc_at_rep_get() from "FID_F" failed: rc=%d\n",
			FID_P(&req->cir_proc), rc);

	return req->cir_rc == 0 ? rc : req->cir_rc;
}

static void fop_fini_lock(struct m0_fop *fop)
{
	struct m0_rpc_machine *mach = m0_fop_rpc_machine(fop);

	m0_rpc_machine_lock(mach);
	m0_fop_fini(fop);
	m0_rpc_machine_unlock(mach);
}

void c0appz_isc_req_fini(struct c0appz_isc_req *req)
{
	struct m0_fop *reply_fop;

	reply_fop = m0_rpc_item_to_fop(req->cir_fop.f_item.ri_reply);
	if (reply_fop != NULL)
		m0_fop_put_lock(reply_fop);
	req->cir_fop.f_item.ri_reply = NULL;
	m0_rpc_at_fini(&req->cir_isc_fop.fi_args);
	m0_rpc_at_fini(&req->cir_isc_fop.fi_ret);
	req->cir_fop.f_data.fd_data = NULL;
	fop_fini_lock(&req->cir_fop);
}

/*
 * c0appz_ex()
 * object exists test.
 */
int c0appz_ex(uint64_t idhi, uint64_t idlo, struct m0_obj *obj_out)
{
	int                  rc;
	struct m0_obj obj = {};
	struct m0_uint128    id;

	id.u_hi = idhi;
	id.u_lo = idlo;

	m0_obj_init(&obj, &uber_realm, &id, M0_DEFAULT_LAYOUT_ID);
	rc = open_entity(&obj.ob_entity);
	if (rc != 0)
		return 0;

	if (obj_out != NULL)
		*obj_out = obj;
	/* success */
	return 1;
}

static int read_conf_params(int idx, const struct param params[], int n)
{
	int i;
	struct conf_params_to_read {
		const char *name;
		const char **conf_ptr;
	} p[] = { { "HA_ENDPOINT_ADDR",      &m0_conf.mc_ha_addr },
		  { "PROFILE_FID",           &m0_conf.mc_profile },
		  { "LOCAL_ENDPOINT_ADDR%d", &m0_conf.mc_local_addr },
		  { "LOCAL_PROC_FID%d",      &m0_conf.mc_process_fid }, };
	char pname[256];

	for (i = 0; i < ARRAY_SIZE(p); i++) {
		sprintf(pname, p[i].name, idx);
		*(p[i].conf_ptr) = param_get(pname, params, n);
		if (*(p[i].conf_ptr) == NULL) {
			ERR("%s is not set at %s\n", pname, c0rcfile);
			return -EINVAL;
		}
	}

	return 0;
}

/*
 * c0appz_init()
 * init client resources.
 */
int c0appz_init(int idx)
{
	int   rc;
	int   lid;
	int   rc_params_nr;
	FILE *fp;
	struct param rc_params[MAX_CONF_PARAMS] = {};

	fp = fopen(c0rcfile, "r");
	if (fp == NULL) {
		ERR("failed to open resource file %s: %s\n", c0rcfile,
		    strerror(errno));
		return -errno;
	}

	rc = read_params(fp, rc_params, ARRAY_SIZE(rc_params));
	fclose(fp);
	if (rc < 0) {
		ERR("failed to read parameters from %s: rc=%d", c0rcfile, rc);
		return rc;
	}
	rc_params_nr = rc;

	rc = pools_fids_set(rc_params, rc_params_nr);
	if (rc <= 0) {
		if (rc == 0) {
			ERR("no pools configured at %s\n", c0rcfile);
		} else {
			ERR("failed to set pools from %s\n", c0rcfile);
		}
		return -EINVAL;
	}

	rc = read_conf_params(idx, rc_params, rc_params_nr);
	if (rc != 0) {
		ERR("failed to read conf parameters from %s\n", c0rcfile);
		return rc;
	}

	m0_conf.mc_is_oostore            = true;
	m0_conf.mc_is_read_verify        = false;
#if 0
	/* set to default values */
	m0_conf.mc_tm_recv_queue_min_len = M0_NET_TM_RECV_QUEUE_DEF_LEN;
	m0_conf.mc_max_rpc_msg_size      = M0_RPC_DEF_MAX_RPC_MSG_SIZE;
#endif
	/* set to Sage cluster specific values */
	m0_conf.mc_tm_recv_queue_min_len = 64;
	m0_conf.mc_max_rpc_msg_size      = 65536;
	m0_conf.mc_layout_id             = 0;
	if (unit_size) {
		lid = m0_obj_unit_size_to_layout_id(unit_size * 1024);
		if (lid == 0) {
			fprintf(stderr, "invalid unit size %u, it should be: "
				"power of 2, >= 4 and <= 4096\n", unit_size);
			return -EINVAL;
		}
		m0_conf.mc_layout_id = lid;
	}

	/* IDX_MOTR */
	m0_conf.mc_idx_service_id   = M0_IDX_DIX;
	dix_conf.kc_create_meta = false;
	m0_conf.mc_idx_service_conf = &dix_conf;

#if DEBUG
	fprintf(stderr,"\n---\n");
	fprintf(stderr,"%s,", (char *)m0_conf.mc_local_addr);
	fprintf(stderr,"%s,", (char *)m0_conf.mc_ha_addr);
	fprintf(stderr,"%s,", (char *)m0_conf.mc_profile);
	fprintf(stderr,"%s,", (char *)m0_conf.mc_process_fid);
	fprintf(stderr,"%s,", (char *)m0_conf.mc_idx_service_conf);
	fprintf(stderr,"\n---\n");
#endif

	if (!m0trace_on)
		m0_trace_set_mmapped_buffer(false);
	/* m0_instance */
	rc = m0_client_init(&m0_instance, &m0_conf, true);
	if (rc != 0) {
		fprintf(stderr, "failed to initilise the Client API\n");
		return rc;
	}

	/* And finally, client root realm */
	m0_container_init(&container, NULL, &M0_UBER_REALM,
				 m0_instance);
	rc = container.co_realm.re_entity.en_sm.sm_rc;
	if (rc != 0) {
		fprintf(stderr,"failed to open uber realm\n");
		return rc;
	}

	/* success */
	uber_realm = container.co_realm;
	return 0;
}

/*
 * c0appz_free()
 * free client resources.
 */
int c0appz_free(void)
{
	m0_client_fini(m0_instance, true);
	memset(c0rcfile, 0, sizeof(c0rcfile));
	return 0;
}

int c0appz_set_manual_rc(const char * rc)
{
	snprintf(c0rcfile, MAX_RCFILE_NAME_LEN, "%s", rc);
	return 0;
}

/*
 * c0appz_setrc()
 * set c0apps resource filename
 */
int c0appz_setrc(char *prog)
{
	char hostname[MAX_RCFILE_NAME_LEN / 2] = {0};

	/* null */
	if (!prog) {
		fprintf(stderr, "error! null progname.\n");
		return -EINVAL;
	}

	gethostname(hostname, MAX_RCFILE_NAME_LEN / 2);
	snprintf(c0rcfile, MAX_RCFILE_NAME_LEN, "%s/.c0appz/%src/%s",
		 getenv("HOME"), prog, hostname);

	/* success */
	return 0;
}

/*
 * c0appz_putrc()
 * print c0apps resource filename
 */
void c0appz_putrc(void)
{
	/* print rc filename */
	if (trace_level > 0)
		fprintf(stderr, "%s\n", c0rcfile);
}

/*
 * open_entity()
 * open m0 entity.
 */
int open_entity(struct m0_entity *entity)
{
	int                  rc;
	struct m0_op *op = NULL;

	m0_entity_open(entity, &op);
	m0_op_launch(&op, 1);
	rc = m0_op_wait(op, M0_BITS(M0_OS_FAILED,
					       M0_OS_STABLE),
			       M0_TIME_NEVER) ?: m0_rc(op);
	m0_op_fini(op);
	m0_op_free(op);

	return rc;
}

/**
 * Create m0 object.
 */
int c0appz_cr(uint64_t idhi, uint64_t idlo, int tier, uint64_t bsz)
{
	int                     rc;
	unsigned                lid;
	struct m0_reqh         *reqh = &m0_instance->m0c_reqh;
	struct m0_pool_version *pver;
	struct m0_op    *op = NULL;
	struct m0_fid          *pool = tier2pool(tier);
	struct m0_uint128       id = {idhi, idlo};
	struct m0_obj    obj = {};

	DBG("tier=%d pool=%s\n", tier, fid2str(pool));
	rc = m0_pool_version_get(reqh->rh_pools, pool, &pver);
	if (rc != 0) {
		ERR("m0_pool_version_get(pool=%s) failed: rc=%d\n",
		    fid2str(pool), rc);
		return rc;
	}

	if (unit_size)
		lid = m0_client_layout_id(m0_instance);
	else
		lid = m0_layout_find_by_buffsize(&reqh->rh_ldom, &pver->pv_id,
						 bsz);

	DBG("pool="FID_F" width=%u parity=(%u+%u) unit=%dK m0bs=%luK\n",
	    FID_P(&pver->pv_pool->po_id), pver->pv_attr.pa_P,
	    pver->pv_attr.pa_N, pver->pv_attr.pa_K,
	    m0_obj_layout_id_to_unit_size(lid) / 1024, bsz / 1024);

	m0_obj_init(&obj, &uber_realm, &id, lid);

	rc = open_entity(&obj.ob_entity);
	if (rc == 0) {
		DBG("object already exists!\n");
		return 1;
	}

	/* create object */
	rc = m0_entity_create(pool, &obj.ob_entity, &op);
	if (rc != 0) {
		fprintf(stderr,"%s(): entity_create() failed: rc=%d\n",
			__func__, rc);
		goto err;
	}

	m0_op_launch(&op, 1);

	rc = m0_op_wait(op, M0_BITS(M0_OS_FAILED,
					   M0_OS_STABLE),
			       m0_time_from_now(3,0)) ?: m0_rc(op);

	m0_op_fini(op);
	m0_op_free(op);
err:
	m0_entity_fini(&obj.ob_entity);

	if (rc != 0)
		fprintf(stderr,"%s() failed: rc=%d, check pool parameters\n",
			__func__, rc);
	return rc;
}

/*
 * read_data_from_file()
 * reads data from file and populates buffer.
 */
static size_t read_data_from_file(FILE *fp, struct m0_bufvec *data, size_t bsz,
				  uint32_t cnt)
{
	size_t i;
	size_t read;

	if (cnt > data->ov_vec.v_nr)
		cnt = data->ov_vec.v_nr;
	for (i = 0; i < cnt; i++) {
		read = fread(data->ov_buf[i], 1, bsz, fp);
		if (read < bsz) {
			if (read > 0)
				i++;
			break;
		}

		if (feof(fp))
			break;
	}

	return i;
}

static size_t write_data_to_file(FILE *fp, struct m0_bufvec *data, size_t bsz,
				 uint32_t cnt)
{
	uint32_t i;
	size_t wrtn;

	if (cnt > data->ov_vec.v_nr)
		cnt = data->ov_vec.v_nr;
	for (i = 0; i < cnt; i++) {
		wrtn = fwrite(data->ov_buf[i], bsz, 1, fp);
		if (wrtn != 1)
			break;
	}
	if (i != cnt)
		fprintf(stderr, "%s(): writing to file failed: %s\n",
			__func__, strerror(errno));
	return i;
}

static int do_io_op(struct m0_obj *obj, enum m0_obj_opcode opcode,
		    struct m0_indexvec *ext, struct m0_bufvec *data,
		    struct m0_bufvec *attr)
{
	int                  rc;
	struct m0_op *op = NULL;

	/* Create the write request */
	m0_obj_op(obj, opcode, ext, data, attr, 0, 0, &op);

	/* Launch the write request*/
	m0_op_launch(&op, 1);

	/* wait */
	rc = m0_op_wait(op, M0_BITS(M0_OS_FAILED,
					   M0_OS_STABLE),
			       M0_TIME_NEVER) ?: m0_rc(op);

	m0_op_fini(op);
	m0_op_free(op);

	if (rc != 0)
		fprintf(stderr,"%s() failed: rc=%d\n", __func__, rc);

	return rc;
}

/*
 * write_data_to_object()
 * writes data to an object
 */
int write_data_to_object(struct m0_obj *obj, struct m0_indexvec *ext,
			 struct m0_bufvec *data, struct m0_bufvec *attr)
{
	return do_io_op(obj, M0_OC_WRITE, ext, data, attr);
}

/*
 * read_data_from_object()
 * read data from an object
 */
int read_data_from_object(struct m0_obj *obj, struct m0_indexvec *ext,
			  struct m0_bufvec *data, struct m0_bufvec *attr)
{
	return do_io_op(obj, M0_OC_READ, ext, data, attr);
}

static void m0_aio_executed_cb(struct m0_op *op)
{
	/** Stuff to do when OP is in excecuted state */
}

static void m0_aio_stable_cb(struct m0_op *op)
{
	struct m0_aio_opgrp *grp;

	grp = ((struct m0_aio_op *)op->op_datum)->cao_grp;
	m0_mutex_lock(&grp->cag_mlock);
	if (op->op_rc == 0) {
		grp->cag_blocks_written += 0;
		grp->cag_blocks_to_write   += 0;
		grp->cag_rc = grp->cag_rc ?: op->op_rc;
	}
	m0_mutex_unlock(&grp->cag_mlock);

	m0_semaphore_up(&grp->cag_sem);
}

static void m0_aio_failed_cb(struct m0_op *op)
{
	struct m0_aio_opgrp *grp;

	m0_console_printf("Write operation failed!");
	grp = ((struct m0_aio_op *)op->op_datum)->cao_grp;
	m0_mutex_lock(&grp->cag_mlock);
	grp->cag_rc = grp->cag_rc ?: op->op_rc;
	m0_mutex_unlock(&grp->cag_mlock);

	m0_semaphore_up(&grp->cag_sem);
}

/*
 * write_data_to_object_async()
 * writes to and object in async manner
 */
int write_data_to_object_async(struct m0_aio_op *aio)
{
	struct m0_obj    *obj;
	struct m0_aio_opgrp *grp;
	static struct m0_op_ops op_ops = {
		.oop_executed = m0_aio_executed_cb,
		.oop_stable   = m0_aio_stable_cb,
		.oop_failed   = m0_aio_failed_cb };

	grp = aio->cao_grp;
	obj = &grp->cag_obj;

	/* Create an WRITE op. */
	m0_obj_op(obj, M0_OC_WRITE,
			 &aio->cao_ext, &aio->cao_data,
			 &aio->cao_attr, 0, 0, &aio->cao_op);
	aio->cao_op->op_datum = aio;

	/* Set op's Callbacks */
	m0_op_setup(aio->cao_op, &op_ops, 0);

	/* Launch the write request */
	m0_op_launch(&aio->cao_op, 1);

	return 0;
}


int m0_aio_vec_alloc(struct m0_aio_op *aio, uint64_t bsz, uint32_t cnt)
{
	return alloc_segs(&aio->cao_data, &aio->cao_ext, &aio->cao_attr,
			  bsz, cnt);
}

void m0_aio_vec_free(struct m0_aio_op *aio)
{
	free_segs(&aio->cao_data, &aio->cao_ext, &aio->cao_attr);
}

void m0_aio_op_fini_free(struct m0_aio_op *aio)
{
	m0_op_fini(aio->cao_op);
	m0_op_free(aio->cao_op);
	aio->cao_op = NULL;
}

int m0_aio_opgrp_init(struct m0_aio_opgrp *grp, uint64_t bsz,
			  uint32_t cnt_per_op, uint32_t op_cnt)
{
	int rc = 0;
	uint32_t i;
	struct m0_aio_op *ops;

	M0_SET0(grp);

	M0_ALLOC_ARR(ops, op_cnt);
	if (ops == NULL)
		return -ENOMEM;

	grp->cag_aio_ops = ops;
	for (i = 0; i < op_cnt; i++) {
		ops[i].cao_grp = grp;
		rc = m0_aio_vec_alloc(ops + i, bsz, cnt_per_op);
		if (rc != 0)
			goto err;
	}

	grp->cag_op_cnt = op_cnt;
	m0_mutex_init(&grp->cag_mlock);
	m0_semaphore_init(&grp->cag_sem, 0);
	grp->cag_blocks_to_write = cnt_per_op * op_cnt;
	grp->cag_blocks_written = 0;

	return 0;
 err:
	while (i--)
		m0_aio_vec_free(ops + i);
	m0_free(grp->cag_aio_ops);
	return rc;
}

void m0_aio_opgrp_fini(struct m0_aio_opgrp *grp)
{
	uint32_t i = grp->cag_op_cnt;

	m0_mutex_fini(&grp->cag_mlock);
	m0_semaphore_fini(&grp->cag_sem);
	grp->cag_blocks_to_write = 0;
	grp->cag_blocks_written = 0;

	while (i--)
		m0_aio_vec_free(grp->cag_aio_ops + i);
	m0_free(grp->cag_aio_ops);
}

int create_object(struct m0_uint128 id)
{
	uint64_t block_size = 1024*1024;
	int pool = 0;
	int rc = c0appz_cr(id.u_hi, id.u_lo, pool, block_size);
	return rc;
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

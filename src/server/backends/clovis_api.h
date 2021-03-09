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
 */

#ifndef __CLOVIS__API__H
#define __CLOVIS__API__H

#include <stdint.h>
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

#include "clovis/clovis.h"
#include "clovis/clovis_idx.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef DEBUG
#define DEBUG 0
#endif

#define MAXC0RC                  4
#define SZC0RCSTR                256
#define SZC0RCFILE               256
#define CLOVIS_MAX_BLOCK_COUNT  (200)
#define CLOVIS_MAX_PER_WIO_SIZE (128*1024*1024)
#define CLOVIS_MAX_PER_RIO_SIZE (512*1024*1024)

struct clovis_aio_op;
struct clovis_aio_opgrp {
	struct m0_semaphore   cag_sem;

	struct m0_mutex       cag_mlock;
	uint32_t              cag_blocks_to_write;
	uint32_t              cag_blocks_written;
	int                   cag_rc;

	struct m0_clovis_obj  cag_obj;
	struct clovis_aio_op *cag_aio_ops;
};

struct clovis_aio_op {
	struct clovis_aio_opgrp *cao_grp;

	struct m0_clovis_op     *cao_op;
	struct m0_indexvec       cao_ext;
	struct m0_bufvec         cao_data;
	struct m0_bufvec         cao_attr;
};


extern struct m0_clovis *clovis_instance;


/*
 * High Level API functions used for the clovis sample apps
 */

int c0appz_init(int idx, char *ressource_file);
int c0appz_free(void);
int c0appz_rm(int64_t idhi, int64_t idlo);
int c0appz_cat(int64_t idhi, int64_t idlo, int bsz, int cnt, char *filename);
int c0appz_cp(int64_t idhi, int64_t idlo, char *filename, int bsz, int cnt);
int c0appz_cp_async(int64_t idhi, int64_t idlo, char *filename,
		    int bsz, int cnt, int op_cnt);
int c0appz_setrc(char *rcfile);
int c0appz_putrc(void);
int c0appz_timeout(uint64_t sz);
int c0appz_timein();
int c0appz_generate_id(int64_t *idh, int64_t *idl);


//************************************************************************

/* 
 * Lower Level API Functions 
 */ 
char *trim(char *str);
int open_entity(struct m0_clovis_entity *entity);
int create_object(struct m0_uint128 id);
int read_data_from_file(FILE *fp, struct m0_bufvec *data, int bsz);
int write_data_to_file(FILE *fp, struct m0_bufvec *data, int bsz);

int read_data_from_object(struct m0_uint128 id,
				 struct m0_indexvec *ext,
				 struct m0_bufvec *data,
				 struct m0_bufvec *attr);
int write_data_to_object(struct m0_uint128 id, struct m0_indexvec *ext,
                                struct m0_bufvec *data, struct m0_bufvec *attr);

int write_data_to_object_async(struct clovis_aio_op *aio);
int clovis_aio_vec_alloc(struct clovis_aio_op *aio,
			       uint32_t blk_count, uint32_t blk_size);
void clovis_aio_vec_free(struct clovis_aio_op *aio);
int clovis_aio_opgrp_init(struct clovis_aio_opgrp *grp,
				 int blk_cnt, int op_cnt);
void clovis_aio_opgrp_fini(struct clovis_aio_opgrp *grp);
void clovis_aio_executed_cb(struct m0_clovis_op *op);
void clovis_aio_stable_cb(struct m0_clovis_op *op);
void clovis_aio_failed_cb(struct m0_clovis_op *op);

#ifdef __cplusplus
}
#endif

#endif
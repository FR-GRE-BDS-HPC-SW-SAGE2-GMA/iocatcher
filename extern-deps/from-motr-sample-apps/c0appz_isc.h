/* -*- C -*- */
/*
 * Copyright (c) 2018-2020 Seagate Technology LLC and/or its Affiliates
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
 * Original author:  Nachiket Sahasrabuddhe <nachiket.sahasrabuddhe@seagate.com>
 * Original creation date: 06-Sep-2018
 */

#include "iscservice/isc.h"
#include "rpc/rpc_machine.h"

#ifdef __cplusplus
extern "C" {
#endif

enum c0appz_buffer_len {
	/** 32K */
	CBL_DEFAULT_MAX = M0_RPC_DEF_MAX_RPC_MSG_SIZE >> 2,
	CBL_DEFAULT_MIN = 256,
};

/** A request holding all parameters relevant to a computation. */
struct c0appz_isc_req {
	/** Arguments for computation. */
	struct m0_buf      *cir_args;
	/** Buffer to store returned result. */
	struct m0_buf      *cir_result;
	/** Error code for the computation. */
	int                 cir_rc;
	/** FOP for ISC service. */
	struct m0_fop_isc   cir_isc_fop;
	/** A generic fop for ISC service. */
	struct m0_fop       cir_fop;
	/** Unique fid of process hosting ISC service. */
	struct m0_fid       cir_proc;
	/** RPC link for ISC service. */
	struct m0_rpc_link *cir_rpc_link;
};

/** Returns the cut-off of message size beyond which RPC bulk is used. */
int c0appz_rmach_bulk_cutoff(struct m0_rpc_link *link, uint32_t *bulk_cutoff);

/**
 * Loads a library to all Motr instances hosting an ISC service.
 * It assumes that library is located at identical path on all
 * Motr instances.
 * */
int c0appz_isc_api_register(const char *libpath);

/** RPC link for ISC service specified by the fid. */
struct m0_rpc_link * c0appz_isc_rpc_link_get(struct m0_fid *svc_fid);

/**
 * Returns the fid of next service of given type in configuration.
 * @param[in]  svc_fid Represents the current service fid. In order
 *		       to fetch the fid of the first service of given
 *		       this should be set to M0_FID0.
 * @param[in]  s_type  Type of the service being searched for. eg. M0_CST_ISC
 *                     for ISC service. See more types in conf/schema.h
 * @param[out] nxt_fid "fid" associated with the next service.
 * @retval     0       When the next service is found.
 * @retval    -ENOENT  When no service is found.
 */
int c0appz_isc_nxt_svc_get(struct m0_fid *svcc_fid, struct m0_fid *nxt_fid,
			    enum m0_conf_service_type s_type);

/**
 * Prepares a request using provided parameters.
 * @param[in] args      Holds arguments for the computation.
 * @param[in] comp_fid  The unique fid associated with the computation.
 * @param[in] reply_buf A buf to be used for holding reply.
 * @param[in] svc_fid   "fid" associated with service.
 * @param[in] reply_len Expected length of reply buffer. This is allowed to
 *                      be greater than the actual length of reply.
 */
int c0appz_isc_req_prepare(struct c0appz_isc_req *req, struct m0_buf *args,
			   const struct m0_fid *comp_fid,
			   struct m0_buf *reply_buf, struct m0_fid *proc_fid,
			   uint32_t reply_len);

/**
 * Sends a request and waits till reply is received. The received reply is
 * populated in req->cir_result. The returned errpr cpde is in req->cir_rc.
 */
int c0appz_isc_req_send_sync(struct c0appz_isc_req *req);

/**
 * Finalizes the request, including input and result buffers.
 */
void c0appz_isc_req_fini(struct c0appz_isc_req *req);

#ifdef __cplusplus
}
#endif

/*
 *  Local variables:
 *  c-indentation-style: "K&R"
 *  c-basic-offset: 8
 *  tab-width: 8
 *  fill-column: 80
 *  scroll-step: 1
 *  End:
 */

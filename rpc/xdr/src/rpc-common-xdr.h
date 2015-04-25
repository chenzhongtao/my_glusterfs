/*
  Copyright (c) 2007-2014 Red Hat, Inc. <http://www.redhat.com>
  This file is part of GlusterFS.

  This file is licensed to you under your choice of the GNU Lesser
  General Public License, version 3 or any later version (LGPLv3 or
  later), or the GNU General Public License, version 2 (GPLv2), in all
  cases as published by the Free Software Foundation.
*/

#include "compat.h"
#include "xdr-common.h"
#include "xdr-nfs3.h"

#if defined(__GNUC__)
#if __GNUC__ >= 4
#if !defined(__clang__)
#if !defined(__NetBSD__)
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wunused-variable"
#endif
#else
#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wunused-value"
#endif
#endif
#endif

/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#ifndef _RPC_COMMON_XDR_H_RPCGEN
#define _RPC_COMMON_XDR_H_RPCGEN

#include <rpc/rpc.h>


#ifdef __cplusplus
extern "C" {
#endif


struct auth_glusterfs_parms_v2 {
	int pid;
	u_int uid;
	u_int gid;
	struct {
		u_int groups_len;
		u_int *groups_val;
	} groups;
	struct {
		u_int lk_owner_len;
		char *lk_owner_val;
	} lk_owner;
};
typedef struct auth_glusterfs_parms_v2 auth_glusterfs_parms_v2;

struct auth_glusterfs_parms {
	u_quad_t lk_owner;
	u_int pid;
	u_int uid;
	u_int gid;
	u_int ngrps;
	u_int groups[16];
};
typedef struct auth_glusterfs_parms auth_glusterfs_parms;

struct gf_dump_req {
	u_quad_t gfs_id;
};
typedef struct gf_dump_req gf_dump_req;

struct gf_prog_detail {
	char *progname;
	u_quad_t prognum;
	u_quad_t progver;
	struct gf_prog_detail *next;
};
typedef struct gf_prog_detail gf_prog_detail;

struct gf_dump_rsp {
	u_quad_t gfs_id;
	int op_ret;
	int op_errno;
	struct gf_prog_detail *prog;
};
typedef struct gf_dump_rsp gf_dump_rsp;

struct gf_common_rsp {
	int op_ret;
	int op_errno;
	struct {
		u_int xdata_len;
		char *xdata_val;
	} xdata;
};
typedef struct gf_common_rsp gf_common_rsp;

/* the xdr functions */

#if defined(__STDC__) || defined(__cplusplus)
extern  bool_t xdr_auth_glusterfs_parms_v2 (XDR *, auth_glusterfs_parms_v2*);
extern  bool_t xdr_auth_glusterfs_parms (XDR *, auth_glusterfs_parms*);
extern  bool_t xdr_gf_dump_req (XDR *, gf_dump_req*);
extern  bool_t xdr_gf_prog_detail (XDR *, gf_prog_detail*);
extern  bool_t xdr_gf_dump_rsp (XDR *, gf_dump_rsp*);
extern  bool_t xdr_gf_common_rsp (XDR *, gf_common_rsp*);

#else /* K&R C */
extern bool_t xdr_auth_glusterfs_parms_v2 ();
extern bool_t xdr_auth_glusterfs_parms ();
extern bool_t xdr_gf_dump_req ();
extern bool_t xdr_gf_prog_detail ();
extern bool_t xdr_gf_dump_rsp ();
extern bool_t xdr_gf_common_rsp ();

#endif /* K&R C */

#ifdef __cplusplus
}
#endif

#endif /* !_RPC_COMMON_XDR_H_RPCGEN */

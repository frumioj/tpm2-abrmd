/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2017 - 2018, Intel Corporation
 * All rights reserved.
 */
#ifndef TSS2_TABD_H
#define TSS2_TABD_H

#include <gio/gio.h>

#include <tss2/tss2_tpm2_types.h>
#include <tss2/tss2_tcti.h>

#define TABD_INIT_THREAD_NAME "tss2-tabrmd_init-thread"

/* implementation specific RCs */
#define TSS2_RESMGR_RC_INTERNAL_ERROR (TSS2_RC)(TSS2_RESMGR_RC_LAYER | (1 << TSS2_LEVEL_IMPLEMENTATION_SPECIFIC_SHIFT))
#define TSS2_RESMGR_RC_SAPI_INIT      (TSS2_RC)(TSS2_RESMGR_RC_LAYER | (2 << TSS2_LEVEL_IMPLEMENTATION_SPECIFIC_SHIFT))
#define TSS2_RESMGR_RC_OUT_OF_MEMORY  (TSS2_RC)(TSS2_RESMGR_RC_LAYER | (3 << TSS2_LEVEL_IMPLEMENTATION_SPECIFIC_SHIFT))
/* RCs in the RESMGR layer */
#define TSS2_RESMGR_RC_BAD_VALUE       (TSS2_RC)(TSS2_RESMGR_RC_LAYER | TSS2_BASE_RC_BAD_VALUE)
#define TSS2_RESMGR_RC_NOT_PERMITTED   (TSS2_RC)(TSS2_RESMGR_RC_LAYER | TSS2_BASE_RC_NOT_PERMITTED)
#define TSS2_RESMGR_RC_NOT_IMPLEMENTED (TSS2_RC)(TSS2_RESMGR_RC_LAYER | TSS2_BASE_RC_NOT_IMPLEMENTED)
#define TSS2_RESMGR_RC_GENERAL_FAILURE (TSS2_RC)(TSS2_RESMGR_RC_LAYER | TSS2_BASE_RC_GENERAL_FAILURE)
#define TSS2_RESMGR_RC_OBJECT_MEMORY   (TSS2_RC)(TSS2_RESMGR_RC_LAYER | TPM2_RC_OBJECT_MEMORY)
#define TSS2_RESMGR_RC_SESSION_MEMORY  (TSS2_RC)(TSS2_RESMGR_RC_LAYER | TPM2_RC_SESSION_MEMORY)

GQuark  tabrmd_error_quark (void);

TSS2_RC tss2_tcti_tabrmd_dump_trans_state (TSS2_TCTI_CONTEXT *tcti_context);

typedef struct tabrmd_handle {
    // ... existing fields ...
    GThread *erlang_thread;
    TSS2_TCTI_CONTEXT *erlang_tcti;
    // ... existing fields ...
} TabrmdHandle;

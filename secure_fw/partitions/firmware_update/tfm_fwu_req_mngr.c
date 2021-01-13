/*
 * Copyright (c) 2021, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include "tfm_fwu_req_mngr.h"
#include "tfm_fwu_internal.h"
#include "firmware_update.h"
#include "service_api.h"
#include "tfm_api.h"

#ifdef TFM_PSA_API
#include "psa/service.h"
#include "psa_manifest/tfm_firmware_update.h"
#include "tfm_memory_utils.h"
#endif

typedef enum {
    FWU_IMAGE_STATE_INVALID = 0,
    FWU_IMAGE_STATE_CANDIDATE,
    FWU_IMAGE_STATE_INSTALL,
    FWU_IMAGE_STATE_ACCEPTED,
    FWU_IMAGE_STATE_REJECTED,
} tfm_fwu_image_state_t;

typedef struct tfm_fwu_ctx_s {
    tfm_image_id_t image_id;
    tfm_fwu_image_state_t image_state;
    bool initialized;
} tfm_fwu_ctx_t;

/**
 * \brief The context of FWU service.
 */
static tfm_fwu_ctx_t fwu_ctx;

#ifndef TFM_PSA_API
psa_status_t tfm_fwu_write_req(psa_invec *in_vec, size_t in_len,
                               psa_outvec *out_vec, size_t out_len)
{
    tfm_image_id_t image_id;
    size_t image_offset;
    uint8_t * p_data;
    size_t data_length;
    int error;
    bool accept_new_image = false;
    psa_status_t status = PSA_SUCCESS;

    (void)out_vec;

    /* Check input parameters. */
    if (in_vec[0].len != sizeof(image_id) ||
        in_vec[1].len != sizeof(image_offset)) {
        /* The size of one of the arguments is incorrect */
        return PSA_ERROR_INVALID_ARGUMENT;
    }

    p_data = (uint8_t * const)in_vec[2].base;

    if (p_data == NULL) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

    image_offset = *((size_t *)in_vec[1].base);
    data_length = in_vec[2].len;
    image_id = *((tfm_image_id_t *)in_vec[0].base);

    if (!fwu_ctx.initialized) {
        error = tfm_internal_fwu_initialize(image_id);
        if (error == 0) {
            accept_new_image = true;
        } else if (error < 0) {
            status = PSA_ERROR_INVALID_ARGUMENT;
        } else {
            status = PSA_ERROR_STORAGE_FAILURE;
        }
    } else {
        if (fwu_ctx.image_id != image_id) {
            accept_new_image = true;
        } else {
            if ((fwu_ctx.image_state == FWU_IMAGE_STATE_CANDIDATE) ||
                (fwu_ctx.image_state == FWU_IMAGE_STATE_REJECTED)) {
                accept_new_image = true;
            } else {
                status = TFM_ERROR_WRITE_FAILURE;
            }
        }
    }

    if (accept_new_image) {
        fwu_ctx.image_state = FWU_IMAGE_STATE_CANDIDATE;
        fwu_ctx.image_id = image_id;
        fwu_ctx.initialized = true;
    } else {
        return status;
    }

    error = tfm_internal_fwu_write(image_id,
                                   image_offset,
                                   p_data,
                                   data_length);
    if (error == 0) {
        return PSA_SUCCESS;
    } else {
        return PSA_ERROR_STORAGE_FAILURE;
    }
}

psa_status_t tfm_fwu_install_req(psa_invec *in_vec, size_t in_len,
                                 psa_outvec *out_vec, size_t out_len)
{
    tfm_image_id_t image_id;
    tfm_image_id_t * dependency_id;
    tfm_image_version_t * dependency_version;
    int result;

    /* Check input/output parameters. */
    if (in_vec[0].len != sizeof(image_id) ||
        out_vec[0].len != sizeof(*dependency_id) ||
        out_vec[1].len != sizeof(*dependency_version)) {
        /* The size of one of the arguments is incorrect. */
        return PSA_ERROR_INVALID_ARGUMENT;
    }

    image_id = *((tfm_image_id_t *)in_vec[0].base);
    dependency_id = out_vec[0].base;
    dependency_version = out_vec[1].base;

    if ((fwu_ctx.image_id != image_id) ||
       (fwu_ctx.image_state != FWU_IMAGE_STATE_CANDIDATE)) {
        return PSA_ERROR_INVALID_ARGUMENT;
    } else {
        fwu_ctx.image_state = FWU_IMAGE_STATE_INSTALL;
        result = tfm_internal_fwu_install(image_id,
                                          dependency_id,
                                          dependency_version);
        if (result == 0) {
            return PSA_SUCCESS;
        } else if (result == 1) {
            return TFM_SUCCESS_REBOOT;
        } else if (result == 2) {
            return TFM_SUCCESS_DEPENDENCY_NEEDED;
        } else {
            return PSA_ERROR_SERVICE_FAILURE;
        }
    }
}

psa_status_t tfm_fwu_query_req(psa_invec *in_vec, size_t in_len,
                               psa_outvec *out_vec, size_t out_len)
{
    tfm_image_id_t image_id;
    tfm_image_info_t * info_p;
    int result = 0;

    if ((in_vec[0].len != sizeof(image_id)) ||
       (out_vec[0].len != sizeof(*info_p))) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

    image_id = *((tfm_image_id_t *)in_vec[0].base);
    info_p = (tfm_image_info_t *)out_vec[0].base;
    result = tfm_internal_fwu_query(image_id, info_p);
    if (result == 0) {
        return PSA_SUCCESS;
    } else if (result < 0) {
        return PSA_ERROR_INVALID_ARGUMENT;
    } else {
        return PSA_ERROR_SERVICE_FAILURE;
    }
}

psa_status_t tfm_fwu_request_reboot_req(psa_invec *in_vec, size_t in_len,
                               psa_outvec *out_vec, size_t out_len)
{
    (void)in_vec;
    (void)out_vec;
    (void)in_len;
    (void)out_len;

    tfm_internal_fwu_request_reboot();

    return PSA_SUCCESS;
}

psa_status_t tfm_fwu_accept_req(psa_invec *in_vec, size_t in_len,
                               psa_outvec *out_vec, size_t out_len)
{
    (void)in_vec;
    (void)out_vec;
    (void)in_len;
    (void)out_len;

    /* The accept should after a reboot, so the image state should be
     * INVALID.
     */
    if(fwu_ctx.image_state != FWU_IMAGE_STATE_INVALID) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

    if(tfm_internal_fwu_accept() == 0) {
        fwu_ctx.image_state = FWU_IMAGE_STATE_ACCEPTED;
    } else {
        return PSA_ERROR_SERVICE_FAILURE;
    }

    return PSA_SUCCESS;
}

/* Abort the currently running FWU. */
psa_status_t tfm_fwu_abort_req(psa_invec *in_vec, size_t in_len,
                               psa_outvec *out_vec, size_t out_len)
{
    tfm_image_id_t image_id;

    (void)out_vec;

    if (in_vec[0].len != sizeof(image_id)) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

    image_id = *((tfm_image_id_t *)in_vec[0].base);
    if (((fwu_ctx.image_state == FWU_IMAGE_STATE_CANDIDATE) ||
        (fwu_ctx.image_state == FWU_IMAGE_STATE_INSTALL)) &&
        (fwu_ctx.image_id == image_id)) {
        fwu_ctx.image_state = FWU_IMAGE_STATE_INVALID;
        fwu_ctx.image_id = TFM_FWU_INVALID_IMAGE_ID;
        fwu_ctx.initialized = false;
        tfm_internal_fwu_abort(image_id);
        return PSA_SUCCESS;
    } else {
        /* No image with the provided UUID is currently being installed. */
        return PSA_ERROR_INVALID_ARGUMENT;
    }
}
#else
typedef psa_status_t (*fwu_func_t)(void);
static psa_msg_t msg;

static psa_status_t tfm_fwu_write_ipc(void)
{
    tfm_image_id_t image_id;
    size_t image_offset;
    size_t data_length, num;
    int error;
    bool accept_new_image = false;
    psa_status_t status = PSA_SUCCESS;
    uint8_t data_block[TFM_FWU_MAX_BLOCK_SIZE];

    /* Check input parameters. */
    if (msg.in_size[0] != sizeof(image_id) ||
        msg.in_size[1] != sizeof(image_offset)) {
        return PSA_ERROR_PROGRAMMER_ERROR;
    }

    num = psa_read(msg.handle, 0, &image_id, sizeof(image_id));
    if (num != sizeof(image_id)) {
        return PSA_ERROR_PROGRAMMER_ERROR;
    }

    num = psa_read(msg.handle, 1, &image_offset, sizeof(image_offset));
    if (num != sizeof(image_offset)) {
        return PSA_ERROR_PROGRAMMER_ERROR;
    }

    tfm_memset(data_block, 0, sizeof(data_block));
    data_length = msg.in_size[2];
    num = psa_read(msg.handle, 2, data_block, data_length);
    if (num != data_length) {
        return PSA_ERROR_PROGRAMMER_ERROR;
    }

    if (!fwu_ctx.initialized) {
        error = tfm_internal_fwu_initialize(image_id);
        if (error == 0) {
            accept_new_image = true;
        } else if (error < 0) {
            status = PSA_ERROR_INVALID_ARGUMENT;
        } else {
            status = PSA_ERROR_STORAGE_FAILURE;
        }
    } else {
        if (fwu_ctx.image_id != image_id) {
            accept_new_image = true;
        } else {
            if ((fwu_ctx.image_state == FWU_IMAGE_STATE_CANDIDATE) ||
                (fwu_ctx.image_state == FWU_IMAGE_STATE_REJECTED)) {
                accept_new_image = true;
            } else {
                status = TFM_ERROR_WRITE_FAILURE;
            }
        }
    }

    if (accept_new_image) {
        fwu_ctx.image_state = FWU_IMAGE_STATE_CANDIDATE;
        fwu_ctx.image_id = image_id;
        fwu_ctx.initialized = true;
    } else {
        return status;
    }

    error = tfm_internal_fwu_write(image_id,
                                   image_offset,
                                   data_block,
                                   data_length);
    if (error == 0) {
        return PSA_SUCCESS;
    } else {
        return PSA_ERROR_STORAGE_FAILURE;
    }
}

static psa_status_t tfm_fwu_install_ipc(void)
{
    tfm_image_id_t image_id;
    tfm_image_id_t dependency_id;
    tfm_image_version_t dependency_version;
    size_t num;
    int result;

    /* Check input parameters. */
    if (msg.in_size[0] != sizeof(image_id) ||
        msg.out_size[0] != sizeof(dependency_id) ||
        msg.out_size[1] != sizeof(dependency_version)) {
        return PSA_ERROR_PROGRAMMER_ERROR;
    }

    num = psa_read(msg.handle, 0, &image_id, sizeof(image_id));
    if (num != sizeof(image_id)) {
        return PSA_ERROR_PROGRAMMER_ERROR;
    }

    if ((fwu_ctx.image_id != image_id) ||
       (fwu_ctx.image_state != FWU_IMAGE_STATE_CANDIDATE)) {
        return PSA_ERROR_INVALID_ARGUMENT;
    } else {
        fwu_ctx.image_state = FWU_IMAGE_STATE_INSTALL;
        result = tfm_internal_fwu_install(image_id,
                                          &dependency_id,
                                          &dependency_version);
        if (result == 0) {
            return PSA_SUCCESS;
        } else if (result == 1) {
            return TFM_SUCCESS_REBOOT;
        } else if (result == 2) {
            psa_write(msg.handle, 0, &dependency_id, sizeof(dependency_id));
            psa_write(msg.handle, 1, &dependency_version,
                      sizeof(dependency_version));
            return TFM_SUCCESS_DEPENDENCY_NEEDED;
        } else {
            return PSA_ERROR_SERVICE_FAILURE;
        }
    }
}

static psa_status_t tfm_fwu_query_ipc(void)
{
    tfm_image_id_t image_id;
    size_t num;
    tfm_image_info_t info;
    int result;

    /* Check input parameters. */
    if (msg.in_size[0] != sizeof(image_id)) {
        return PSA_ERROR_PROGRAMMER_ERROR;
    }
    num = psa_read(msg.handle, 0, &image_id, sizeof(image_id));
    if (num != sizeof(image_id)) {
        return PSA_ERROR_PROGRAMMER_ERROR;
    }
    result = tfm_internal_fwu_query(image_id, &info);
    if (result == 0) {
        psa_write(msg.handle, 0, &info, sizeof(info));
        return PSA_SUCCESS;
    } else if (result < 0) {
        return PSA_ERROR_INVALID_ARGUMENT;
    } else {
        return PSA_ERROR_SERVICE_FAILURE;
    }
}

static psa_status_t tfm_fwu_request_reboot_ipc(void)
{
    tfm_internal_fwu_request_reboot();

    return PSA_SUCCESS;
}

static psa_status_t tfm_fwu_accept_ipc(void)
{
    /* The accept should after a reboot, so the image state should be
     * INVALID.
     */
    if(fwu_ctx.image_state != FWU_IMAGE_STATE_INVALID) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

    if(tfm_internal_fwu_accept() == 0) {
        fwu_ctx.image_state = FWU_IMAGE_STATE_ACCEPTED;
    } else {
        return PSA_ERROR_SERVICE_FAILURE;
    }

    return PSA_SUCCESS;
}

static psa_status_t tfm_fwu_abort_ipc(void)
{
    tfm_image_id_t image_id;
    size_t num;

    if (msg.in_size[0] != sizeof(image_id)) {
        return PSA_ERROR_PROGRAMMER_ERROR;
    }

    num = psa_read(msg.handle, 0, &image_id, sizeof(image_id));
    if (num != sizeof(image_id)) {
        return PSA_ERROR_PROGRAMMER_ERROR;
    }

    if (fwu_ctx.image_state == FWU_IMAGE_STATE_INVALID) {
        return PSA_SUCCESS;
    } else if ((fwu_ctx.image_state == FWU_IMAGE_STATE_CANDIDATE) ||
       (fwu_ctx.image_state == FWU_IMAGE_STATE_INSTALL)) {
        fwu_ctx.image_state = FWU_IMAGE_STATE_INVALID;
        fwu_ctx.image_id = TFM_FWU_INVALID_IMAGE_ID;
        fwu_ctx.initialized = false;
        tfm_internal_fwu_abort(image_id);
        return PSA_SUCCESS;
    } else {
        /* No image with the provided UUID is currently being installed. */
        return PSA_ERROR_INVALID_ARGUMENT;
    }
}

/*
 * Fixme: Temporarily implement abort as infinite loop,
 * will replace it later.
 */
static void tfm_abort(void)
{
    while (1)
        ;
}

static void fwu_signal_handle(psa_signal_t signal, fwu_func_t pfn)
{
    psa_status_t status;

    status = psa_get(signal, &msg);
    if (status != PSA_SUCCESS) {
        return;
    }

    switch (msg.type) {
    case PSA_IPC_CONNECT:
        psa_reply(msg.handle, PSA_SUCCESS);
        break;
    case PSA_IPC_CALL:
        status = pfn();
        psa_reply(msg.handle, status);
        break;
    case PSA_IPC_DISCONNECT:
        psa_reply(msg.handle, PSA_SUCCESS);
        break;
    default:
        tfm_abort();
    }
}
#endif

psa_status_t tfm_fwu_init(void)
{
#ifdef TFM_PSA_API
    psa_signal_t signals = 0;

    if (fwu_bootloader_init() != 0) {
        tfm_abort();
    }

    while (1) {
        signals = psa_wait(PSA_WAIT_ANY, PSA_BLOCK);
        if (signals & TFM_FWU_WRITE_SIGNAL) {
            fwu_signal_handle(TFM_FWU_WRITE_SIGNAL, tfm_fwu_write_ipc);
        } else if (signals & TFM_FWU_INSTALL_SIGNAL) {
            fwu_signal_handle(TFM_FWU_INSTALL_SIGNAL, tfm_fwu_install_ipc);
        } else if (signals & TFM_FWU_ABORT_SIGNAL) {
            fwu_signal_handle(TFM_FWU_ABORT_SIGNAL, tfm_fwu_abort_ipc);
        } else if (signals & TFM_FWU_QUERY_SIGNAL) {
            fwu_signal_handle(TFM_FWU_QUERY_SIGNAL, tfm_fwu_query_ipc);
        } else if (signals & TFM_FWU_REQUEST_REBOOT_SIGNAL) {
            fwu_signal_handle(TFM_FWU_REQUEST_REBOOT_SIGNAL,
                              tfm_fwu_request_reboot_ipc);
        } else if (signals & TFM_FWU_ACCEPT_SIGNAL) {
            fwu_signal_handle(TFM_FWU_ACCEPT_SIGNAL, tfm_fwu_accept_ipc);
        } else {
            tfm_abort();
        }
    }
#else
    if (fwu_bootloader_init() != 0) {
        return PSA_ERROR_SERVICE_FAILURE;
    } else {
        return PSA_SUCCESS;
    }
#endif
}

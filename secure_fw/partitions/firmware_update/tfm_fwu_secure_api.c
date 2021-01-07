/*
 * Copyright (c) 2021, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "firmware_update.h"
#include "tfm_api.h"

#ifdef TFM_PSA_API
#include "psa/client.h"
#include "psa_manifest/sid.h"
#else
#include "tfm_veneers.h"
#endif

#define IOVEC_LEN(x) (sizeof(x)/sizeof(x[0]))

psa_status_t tfm_fwu_write(uint32_t uuid,
                           size_t image_offset,
                           const void * block,
                           size_t block_size)
{
    psa_status_t status;

    psa_invec in_vec[] = {
        { .base = &uuid, .len = sizeof(uuid) },
        { .base = &image_offset, .len = sizeof(image_offset) },
        { .base = block, .len = block_size }
    };

    status = tfm_tfm_fwu_write_req_veneer(in_vec, IOVEC_LEN(in_vec), NULL, 0);

    /* A parameter with a buffer pointer where its data length is longer than
     * maximum permitted, it is treated as a secure violation.
     * TF-M framework rejects the request with TFM_ERROR_INVALID_PARAMETER.
     * The firmware update secure PSA implementation returns
     * PSA_ERROR_INVALID_ARGUMENT in that case.
     */
    if (status == (psa_status_t)TFM_ERROR_INVALID_PARAMETER) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

    return status;
}

psa_status_t tfm_fwu_install(const tfm_image_id_t uuid,
                             tfm_image_id_t * dependency_uuid,
                             tfm_image_version_t *dependency_version)
{
    psa_status_t status;

    psa_invec in_vec[] = {
        { .base = &uuid, .len = sizeof(uuid) }
    };

    psa_outvec out_vec[] = {
        { .base = dependency_uuid, .len = sizeof(*dependency_uuid) },
        { .base = dependency_version, .len = sizeof(*dependency_version)}
    };

    if ((dependency_uuid == NULL) || (dependency_version == NULL)) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

    status = tfm_tfm_fwu_install_req_veneer(in_vec, IOVEC_LEN(in_vec),
                                            out_vec, IOVEC_LEN(out_vec));

    /* A parameter with a buffer pointer where its data length is longer than
     * maximum permitted, it is treated as a secure violation.
     * TF-M framework rejects the request with TFM_ERROR_INVALID_PARAMETER.
     * The firmware update secure PSA implementation returns
     * PSA_ERROR_INVALID_ARGUMENT in that case.
     */
    if (status == (psa_status_t)TFM_ERROR_INVALID_PARAMETER) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

    return status;
}

psa_status_t tfm_fwu_abort(const tfm_image_id_t uuid)
{
    psa_status_t status;

    psa_invec in_vec[] = {
        { .base = &uuid, .len = sizeof(uuid) }
    };

    status = tfm_tfm_fwu_abort_req_veneer(in_vec, IOVEC_LEN(in_vec),
                                          NULL, 0);

    /* A parameter with a buffer pointer where its data length is longer than
     * maximum permitted, it is treated as a secure violation.
     * TF-M framework rejects the request with TFM_ERROR_INVALID_PARAMETER.
     * The firmware update secure PSA implementation returns
     * PSA_ERROR_INVALID_ARGUMENT in that case.
     */
    if (status == (psa_status_t)TFM_ERROR_INVALID_PARAMETER) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

    return status;
}

psa_status_t tfm_fwu_query(const tfm_image_id_t uuid, tfm_image_info_t *info)
{
    psa_status_t status;

    psa_invec in_vec[] = {
        { .base = &uuid, .len = sizeof(uuid) }
    };
    psa_outvec out_vec[] = {
        { .base = info, .len = sizeof(*info)}
    };

    status = tfm_tfm_fwu_query_req_veneer(in_vec, IOVEC_LEN(in_vec),
                                          out_vec, IOVEC_LEN(out_vec));

    return status;
}

psa_status_t tfm_fwu_request_reboot(void)
{
    psa_status_t status;

    status = tfm_tfm_fwu_request_reboot_req_veneer(NULL, 0, NULL, 0);

    if (status == (psa_status_t)TFM_ERROR_INVALID_PARAMETER) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

    return status;
}

psa_status_t tfm_fwu_accept(void)
{
    psa_status_t status;

    status = tfm_tfm_fwu_accept_req_veneer(NULL, 0, NULL, 0);

    if (status == (psa_status_t)TFM_ERROR_INVALID_PARAMETER) {
        return PSA_ERROR_INVALID_ARGUMENT;
    }

    return status;
}

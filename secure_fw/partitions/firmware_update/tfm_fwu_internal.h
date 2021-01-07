/*
 * Copyright (c) 2021, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef __TFM_FWU_INTERNAL_H__
#define __TFM_FWU_INTERNAL_H__

#include <stddef.h>

#include "firmware_update.h"
#include "tfm_bootloader_fwu_abstraction.h"

#ifdef __cplusplus
extern "C" {
#endif

int tfm_internal_fwu_initialize(tfm_image_id_t image_id);

int tfm_internal_fwu_write(tfm_image_id_t image_id,
                           size_t image_offset,
                           const void * block,
                           size_t block_size);

int tfm_internal_fwu_install(tfm_image_id_t image_id,
                             tfm_image_id_t * dependency_uuid,
                             tfm_image_version_t *dependency_version);

void tfm_internal_fwu_abort(tfm_image_id_t image_id);

int tfm_internal_fwu_query(tfm_image_id_t uuid,
                           tfm_image_info_t *info);

void tfm_internal_fwu_request_reboot(void);

int tfm_internal_fwu_accept(void);

#ifdef __cplusplus
}
#endif

#endif /* __TFM_FWU_INTERNAL_H__ */

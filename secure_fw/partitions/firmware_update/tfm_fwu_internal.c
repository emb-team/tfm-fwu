/*
 * Copyright (c) 2021, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <stdint.h>
#include "log/tfm_log.h"
#include "tfm_hal_platform.h"
#include "tfm_fwu_internal.h"

int tfm_internal_fwu_initialize(tfm_image_id_t image_id)
{
    uint8_t image_type = (uint8_t)FWU_IMAGE_ID_GET_TYPE(image_id);
    uint8_t slot_id = (uint8_t)FWU_IMAGE_ID_GET_SLOT(image_id);

    /* Check the image slot, the target should be the staging slot. */
    if (slot_id != FWU_IMAGE_ID_SLOT_1) {
        LOG_MSG("TFM FWU: invalid slot_id: %d", slot_id);
        return -1;
    }

    if (fwu_bootloader_staging_area_init(image_type) != 0) {
        return 1;
    }

    return 0;
}

int tfm_internal_fwu_write(tfm_image_id_t image_id,
                           size_t image_offset,
                           const void * block,
                           size_t block_size)
{
    uint8_t image_type = (uint8_t)FWU_IMAGE_ID_GET_TYPE(image_id);
    uint8_t slot_id = (uint8_t)FWU_IMAGE_ID_GET_SLOT(image_id);

    if ((block == NULL) || (slot_id != FWU_IMAGE_ID_SLOT_1)) {
        return -1;
    }

    if (fwu_bootloader_load_image(image_type,
                                  image_offset,
                                  block,
                                  block_size) != 0) {
        return 1;
    }

    return 0;
}

int tfm_internal_fwu_install(tfm_image_id_t image_id,
                             tfm_image_id_t * dependency,
                             tfm_image_version_t *dependency_version)
{
    uint8_t image_type = (uint8_t)FWU_IMAGE_ID_GET_TYPE(image_id);
    uint8_t slot_id = (uint8_t)FWU_IMAGE_ID_GET_SLOT(image_id);
    bl_image_id_t dependency_bl;
    tfm_image_version_t version;
    int result;

    /* Check the image slot, the target should be the staging slot. */
    if (slot_id != FWU_IMAGE_ID_SLOT_1) {
        LOG_MSG("TFM FWU: invalid slot_id: %d", slot_id);
        return -1;
    }

    if (dependency == NULL || dependency_version == NULL) {
        return -1;
    }

    result = fwu_bootloader_mark_image_candidate(image_type,
                                                 &dependency_bl,
                                                 &version);
    if (result == 1) {
        *dependency = (tfm_image_id_t)FWU_CALCULATE_IMAGE_ID(FWU_IMAGE_ID_SLOT_1, \
                                                            dependency_bl);
        *dependency_version = version;
    }

    return result;
}

void tfm_internal_fwu_abort(tfm_image_id_t image_id)
{
    uint8_t image_type = (uint8_t)FWU_IMAGE_ID_GET_TYPE(image_id);
    uint8_t slot_id = (uint8_t)FWU_IMAGE_ID_GET_SLOT(image_id);

    if (slot_id != FWU_IMAGE_ID_SLOT_1) {
        return -1;
    }

    fwu_bootloader_abort(image_type);
}

/* The image version of the given image. */
int tfm_internal_fwu_query(tfm_image_id_t uuid,
                           tfm_image_info_t *info)
{
    uint8_t image_type = (uint8_t)FWU_IMAGE_ID_GET_TYPE(uuid);
    uint8_t slot_id = (uint8_t)FWU_IMAGE_ID_GET_SLOT(uuid);
    bool active_image = 0;

    if (slot_id == FWU_IMAGE_ID_SLOT_1) {
        active_image = false;
    } else if (slot_id == FWU_IMAGE_ID_SLOT_0) {
        active_image = true;
    } else {
        LOG_MSG("TFM FWU: invalid slot_id: %d", slot_id);
        return -1;
    }

    return fwu_bootloader_get_image_info(image_type, active_image, info);
}

void tfm_internal_fwu_request_reboot(void)
{
    tfm_hal_system_reset();
}

int tfm_internal_fwu_accept(void)
{
    return fwu_bootloader_mark_image_accepted();
}

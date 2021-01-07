/*
 * Copyright (c) 2021, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "flash_map_backend/flash_map_backend.h"
#include "sysflash/sysflash.h"
#include "bootutil_priv.h"
#include "bootutil/bootutil.h"
#include "bootutil/image.h"
#include "tfm_bootloader_fwu_abstraction.h"
#include "tfm_boot_status.h"
#include "psa/crypto.h"
#include "log/tfm_log.h"
#include "service_api.h"
#include "tfm_memory_utils.h"

#define MAX_IMAGE_INFO_LENGTH    (sizeof(struct image_version) + \
                                  SHARED_DATA_ENTRY_HEADER_SIZE)
#define TFM_MCUBOOT_FWU_INVALD_IMAGE_ID    0xFF
/*
 * \struct fwu_image_info_data
 *
 * \brief Contains the received boot status information from bootloader
 *
 * \details This is a redefinition of \ref tfm_boot_data to allocate the
 *          appropriate, service dependent size of \ref boot_data.
 */
typedef struct fwu_image_info_data_s {
    struct shared_data_tlv_header header;
    uint8_t data[MAX_IMAGE_INFO_LENGTH];
} fwu_image_info_data_t;

static fwu_image_info_data_t boot_shared_data;

/* The target area of the active image in firmware update. */
static const struct flash_area *fap = NULL;

/* The active image in firmware update. */
static bl_image_id_t active_image_id = TFM_MCUBOOT_FWU_INVALD_IMAGE_ID;

static int convert_id_from_bl_to_mcuboot(bl_image_id_t bl_image_id,
                                         uint8_t *mcuboot_image_id)
{
#if (MCUBOOT_IMAGE_NUMBER == 1)
    /* Only full image upgrade is supported in this case. */
    if (bl_image_id != FWU_IMAGE_TYPE_FULL) {
        LOG_MSG("TFM FWU: multi-image is not supported in current mcuboot configuration.");
        return -1;
    }

    /* The image id in mcuboot. 0: the full image. */
    *mcuboot_image_id = 0;
#else
    if (bl_image_id == FWU_IMAGE_TYPE_SECURE) {
        /* The image id in mcuboot. 0: the secure image. */
        *mcuboot_image_id = 0;
    } else if (bl_image_id == FWU_IMAGE_TYPE_NONSECURE) {
        /* The image id in mcuboot. 1: the non-secure image. */
        *mcuboot_image_id = 1;
    }  else {
        LOG_MSG("TFM FWU: invalid image_type: %d", bl_image_id);
        return -1;
    }
#endif
    return 0;
}

static int convert_id_from_mcuboot_to_bl(uint8_t mcuboot_image_id,
                                         bl_image_id_t *bl_image_id)
{
    uint8_t image_type;

    if (bl_image_id == NULL) {
        return -1;
    }

#if (MCUBOOT_IMAGE_NUMBER == 1)
    /* Only full image upgrade is supported in this case. */
    if (mcuboot_image_id != 0) {
        LOG_MSG("TFM FWU: multi-image is not supported in current mcuboot \
                configuration.\n\r");
        return -1;
    }

    /* The image id in mcuboot. 0: the full image. */
    image_type = FWU_IMAGE_TYPE_FULL;
#else
    if (mcuboot_image_id == 0) {
        /* The image id in mcuboot. 0: the secure image. */
        image_type == FWU_IMAGE_TYPE_SECURE
    } else if (mcuboot_image_id == 1) {
        /* The image id in mcuboot. 1: the non-secure image. */
        image_type == FWU_IMAGE_TYPE_NONSECURE
    }  else {
        LOG_MSG("TFM FWU: invalid mcuboot image id\n\r: %d",
                mcuboot_image_id);
        return -1;
    }
#endif
    *bl_image_id = image_type;
    return 0;
}

static int fwu_bootloader_get_shared_data(void)
{
    return tfm_core_get_boot_data(TLV_MAJOR_FWU,
                                  (struct tfm_boot_data *)&boot_shared_data,
                                  sizeof(boot_shared_data));
}

int fwu_bootloader_init(void)
{
    return fwu_bootloader_get_shared_data();
}

int fwu_bootloader_staging_area_init(bl_image_id_t bootloader_image_id)
{
    uint8_t mcuboot_image_id = 0;

    if (convert_id_from_bl_to_mcuboot(bootloader_image_id, &mcuboot_image_id)
        != 0) {
        return -1;
    }

    if (flash_area_open(FLASH_AREA_IMAGE_SECONDARY(mcuboot_image_id), &fap) !=
        0) {
        LOG_MSG("TFM FWU: opening flash failed.\r\n");
        fap = NULL;
        return BOOT_EFLASH;
    }

    if (flash_area_erase(fap, 0, fap->fa_size) != 0)
    {
        LOG_MSG("TFM FWU: erasing flash failed.\r\n");
        return BOOT_EFLASH;
    }

    active_image_id = mcuboot_image_id;
    return 0;
}

int fwu_bootloader_load_image(bl_image_id_t bootloader_image_id,
                              size_t image_offset,
                              const void * block,
                              size_t block_size)
{
    uint8_t mcuboot_image_id = 0;

    if (block == NULL) {
        return -1;
    }

    if (convert_id_from_bl_to_mcuboot(bootloader_image_id, &mcuboot_image_id)
        != 0) {
        return -1;
    }

    if ((fap == NULL) || (active_image_id != mcuboot_image_id)) {
        return -2;
    }

    if (flash_area_write(fap, image_offset, block, block_size) != 0) {
        LOG_MSG("TFM FWU: write flash failed.\r\n");
        return BOOT_EFLASH;
    }
}

static bool check_image_dependency(uint8_t mcuboot_image_id,
                                uint8_t *dependency,
                                tfm_image_version_t *version)
{
    bool found = false;

    if ((dependency == NULL || version == NULL)) {
        return found;
    }

    /* Currently only single image update is supported. So no dependency is
     * required.
     */
    //TODO: Add the dependency check to support multiple image update.
    *dependency = TFM_MCUBOOT_FWU_INVALD_IMAGE_ID;
    *version = (tfm_image_version_t){.iv_major = 0, .iv_minor = 0,
                           .iv_revision = 0, .iv_build_num = 0};

    return found;
}

int fwu_bootloader_mark_image_candidate(bl_image_id_t bootloader_image_id,
                                        bl_image_id_t *dependency,
                                       tfm_image_version_t *dependency_version)
{
    uint8_t mcuboot_image_id = 0;
    uint8_t dependency_mcuboot;
    bl_image_id_t dependency_bl;
    tfm_image_version_t version;

    if ((dependency == NULL || dependency_version == NULL)) {
        return -1;
    }

    if (convert_id_from_bl_to_mcuboot(bootloader_image_id, &mcuboot_image_id)
        != 0) {
        return -1;
    }

    if ((fap == NULL) || (active_image_id != mcuboot_image_id)) {
        return -2;
    }

    if (check_image_dependency(mcuboot_image_id,
                               &dependency_mcuboot,
                               &version)) {
        if (convert_id_from_mcuboot_to_bl(dependency_mcuboot,
                                          &dependency_bl) != 0) {
            return -1;
        }

        *dependency = dependency_bl;
        *dependency_version = version;

        /* '1' indicates that a dependency is required. See the function
         * description in tfm_bootloader_fwu_abstraction.h. */
        return 1;
    } else {
        /* Write the magic in the image trailer so that this image will be set
         * taken as a candidate.
         */
        if (boot_write_magic(fap) != 0) {
            return -3;
        } else {
            /* Sytem reboot is always required. */
            return 1;
        }
    }
}

int fwu_bootloader_mark_image_accepted(void)
{
    /* As DIRECT_XIP, RAM_LOAD and OVERWRITE_ONLY do not support image revert.
     * So there is nothing to do in these three upgrade strategies. Image
     * revert is only supported in SWAP upgrade strategy. In this case, the
     * image need to be set as a pernement image, so that the next time reboot
     * up, the image will still be the running image, otherwise, the image will
     * be reverted and the original image will be chosen as the running image.
     */
#if !defined(MCUBOOT_DIRECT_XIP) && !defined(MCUBOOT_RAM_LOAD) && \
    !defined(MCUBOOT_OVERWRITE_ONLY)
    return boot_set_confirmed();
#else
    return 0;
#endif
}

int fwu_bootloader_abort(bl_image_id_t bootloader_image_id)
{
    uint8_t mcuboot_image_id = 0;

    if (convert_id_from_bl_to_mcuboot(bootloader_image_id, &mcuboot_image_id)
        != 0) {
        return -1;
    }

    if ((fap == NULL) || (active_image_id != mcuboot_image_id)) {
        return -2;
    }

    if (fap != NULL) {
        flash_area_close(fap);
    }

    fap = NULL;
    active_image_id = TFM_MCUBOOT_FWU_INVALD_IMAGE_ID;
}

static int
util_img_hash(struct image_header *hdr,
              const struct flash_area *fap,
              uint8_t *hash_result, size_t buf_size, size_t *hash_size)
{
    psa_hash_operation_t handle = psa_hash_operation_init();
    psa_status_t status;
    uint8_t tmpbuf[BOOT_TMPBUF_SZ];
    uint32_t tmp_buf_sz = BOOT_TMPBUF_SZ;
    uint32_t size;
#ifndef MCUBOOT_RAM_LOAD
    uint32_t blk_sz;
    uint32_t off;
    int rc;
#endif /* MCUBOOT_RAM_LOAD */

    /* The whole flash area are hashed. */
    size = fap->fa_size;

    /* Setup the hash object for the desired hash. */
    status = psa_hash_setup(&handle, PSA_ALG_SHA_256);
    if (status != PSA_SUCCESS) {
        return status;
    }

#ifdef MCUBOOT_RAM_LOAD
    status = psa_hash_update(&handle, (void*)(hdr->ih_load_addr), size);
#else
    for (off = 0; off < size; off += blk_sz) {
        blk_sz = size - off;
        if (blk_sz > tmp_buf_sz) {
            blk_sz = tmp_buf_sz;
        }
        rc = flash_area_read(fap, off, tmpbuf, blk_sz);
        if (rc) {
            return rc;
        }
        status = psa_hash_update(&handle, tmpbuf, blk_sz);
    }
#endif
    if (status != PSA_SUCCESS) {
        return status;
    }
    status = psa_hash_finish(&handle, hash_result, buf_size, hash_size);

    return status;
}

static int get_secondary_image_info(uint8_t image_id, tfm_image_info_t * info)
{
    int area_id;
    const struct flash_area *fap = NULL;
    struct image_header hdr = {0};
    uint8_t hash[TFM_FWU_MAX_DIGEST_SIZE] = {0};
    size_t hash_size = 0;
    int ret;

    if (info == NULL) {
        return -1;
    }

    area_id = flash_area_id_from_multi_image_slot(image_id, 1);

    /* The image version in the primary slot should be returned. */
    if ((flash_area_open(area_id, &fap)) != 0) {
        LOG_MSG("TFM FWU: opening flash failed.\r\n");
        return -2;
    }

    if (flash_area_read(fap, 0, &hdr, sizeof(hdr)) != 0) {
        flash_area_close(fap);
        LOG_MSG("TFM FWU: reading flash failed.\r\n");
        return -3;
    }

    if (hdr.ih_magic != IMAGE_MAGIC)
    {
        flash_area_close(fap);
        LOG_MSG("TFM_FWU: header of image %d in %d is not valid.\n\r",
                image_id, 1);
        return 1;
    }

    info->version.iv_major = hdr.ih_ver.iv_major;
    info->version.iv_minor = hdr.ih_ver.iv_minor;
    info->version.iv_revision = hdr.ih_ver.iv_revision;
    info->version.iv_build_num = hdr.ih_ver.iv_build_num;
    LOG_MSG("version= %d., %d., %d.,+ %d\n\r",
            info->version.iv_major,
            info->version.iv_minor,
            info->version.iv_revision,
            info->version.iv_build_num);

    if (util_img_hash(&hdr, fap, hash, (size_t)TFM_FWU_MAX_DIGEST_SIZE,
                      &hash_size) == PSA_SUCCESS) {
        tfm_memcpy(info->digest, hash, hash_size);
        ret = 0;
    } else {
        ret = -4;
    }

    flash_area_close(fap);
    return ret;
}

int fwu_bootloader_get_image_info(bl_image_id_t bootloader_image_id,
                                  bool active_image,
                                  tfm_image_info_t * info)
{
    struct image_version image_ver = { 0 };
    struct shared_data_tlv_entry tlv_entry;
    uint8_t *tlv_end;
    uint8_t *tlv_curr;
    bool found = false;
    uint8_t mcuboot_image_id = 0;

    if (convert_id_from_bl_to_mcuboot(bootloader_image_id, &mcuboot_image_id)
        != 0) {
        return -1;
    }

    //TODO: Add the module identity when adding the image version into the shared memory.
    if ((info == NULL) || (mcuboot_image_id != 0)) {
        return -1;
    }

    memset(info, 0, sizeof(tfm_image_info_t));
    memset(info->digest, TFM_IMAGE_INFO_INVALID_DIGEST, sizeof(info->digest));

    /* When getting the primary image information, read it from the
     * shared memory. */
    if (active_image) {
        if (boot_shared_data.header.tlv_magic != SHARED_DATA_TLV_INFO_MAGIC) {
            return -2;
        }

        tlv_end = (uint8_t *)&boot_shared_data + \
                  boot_shared_data.header.tlv_tot_len;
        tlv_curr = boot_shared_data.data;

        while (tlv_curr < tlv_end) {
            (void)memcpy(&tlv_entry, tlv_curr, SHARED_DATA_ENTRY_HEADER_SIZE);
            if (GET_FWU_CLAIM(tlv_entry.tlv_type) == SW_VERSION) {
                if (tlv_entry.tlv_len != sizeof(struct image_version)) {
                    return -3;
                }
                memcpy(&image_ver,
                    tlv_curr + SHARED_DATA_ENTRY_HEADER_SIZE,
                    tlv_entry.tlv_len );
                found = true;
                break;
            }
            tlv_curr += SHARED_DATA_ENTRY_HEADER_SIZE + tlv_entry.tlv_len;
        }
        if (found) {
            info->version.iv_major = image_ver.iv_major;
            info->version.iv_minor = image_ver.iv_minor;
            info->version.iv_revision = image_ver.iv_revision;
            info->version.iv_build_num = image_ver.iv_build_num;

            /* The image in the primary slot is verified by the bootloader.
             * The image digest in the primary slot should not be exposed to
             * nonsecure.
             */
            return 0;
        } else {
            return 1;
        }
    } else {
        return get_secondary_image_info(mcuboot_image_id, info);
    }
}

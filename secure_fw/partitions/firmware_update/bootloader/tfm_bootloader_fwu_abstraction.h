/*
 * Copyright (c) 2021, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef __TFM_BOOTLOADER_FWU_ABSTRACTION_H__
#define __TFM_BOOTLOADER_FWU_ABSTRACTION_H__

#include "stdbool.h"
#include "firmware_update.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t bl_image_id_t;

/**
 * Bootloader related initialization for firmware update, such as reading
 * some necessary shared data from the memory if needed.
 *
 * \return 0            On success
 *         error_code   Implementation defined error code on failure
 */
int fwu_bootloader_init(void);

/**
 * \brief Initialize the staging area of the image with given ID.
 *
 * Prepare the staging area of the image with given ID for image download.
 * For example, initialize the staging area, open the flash area, and so on.
 * The image will be written into the staging area later.
 *
 * \param[in] bootloader_image_id The identifier of the target image in
 *                                bootloader
 *
 * \return 0            On success
 *         error_code   Implementation defined error code on failure
 *
 */
int fwu_bootloader_staging_area_init(bl_image_id_t bootloader_image_id);

/**
 * \brief Load the image into the target device.
 *
 * Prepare the staging area of the image with given ID for image download.
 * For example, initialize the staging area, open the flash area, and so on.
 * The image will be written into the staging area later.
 *
 * \param[in] bootloader_image_id The identifier of the target image in
 *                                bootloader
 * \param[in] image_offset        The offset of the image being passed into
 *                                block, in bytes
 * \param[in] block               A buffer containing a block of image data.
 *                                This might be a complete image or a subset.
 * \param[in] block_size          Size of block.

 * \return 0            On success
 *         error_code   Implementation defined error code on failure
 *
 */
int fwu_bootloader_load_image(bl_image_id_t bootloader_image_id,
                              size_t image_offset,
                              const void * block,
                              size_t block_size);

/**
 * \brief Mark staging image as a candidate.
 *
 * Mark the image in staging area as a candidate for bootloader so that the
 * next time bootloader runs, it will take this image as a candidate one to
 * bootup.
 * \param[in] bootloader_image_id The identifier of the target image in
 *                                bootloader
 * \param[out] dependency         Bootloader image ID of dependency if needed
 * \param[out] dependency_version Bootloader image version of dependency if
 *                                needed

 * \return 0            On success
 *         1            A system reboot is needed to finish installation
 *         2            If dependency is required
 *         error_code   Implementation defined error code on failure
 */
int fwu_bootloader_mark_image_candidate(bl_image_id_t bootloader_image_id,
                                        bl_image_id_t *dependency,
                                      tfm_image_version_t *dependency_version);

/**
 * \brief Marks the image in the primary slot as confirmed.
 *
 * Call this API to mark the running images as permanent/accepted to avoid
 * revert when next time bootup. Usually, this API is called after the running
 * images have been verified as valid.
 *
 * \return 0            On success
 *         error_code   Implementation defined error code on failure
 */
int fwu_bootloader_mark_image_accepted(void);

/*
 * Abort the current image download process.
 */
/**
 * \brief Abort the current image download process.
 *
 * \return 0            On success
 *         error_code   Implementation defined error code on failure
 */
int fwu_bootloader_abort(bl_image_id_t bootloader_image_id);

/**
 * \brief Get the image information.
 *
 * Get the image information of the given bootloader_image_id in staging area
 * or the running area.
 *
 * \param[in] bootloader_image_id  The identifier of the target image in
 *                                 bootloader
 * \param[in] active_image         Indicates image location
 *                                 True: the running image.
                                   False: the image in the passive(or staging)
                                   slot.
 * \param[out] info                Buffer containing return the image
 *                                 information

 * \return 0            On success
 *         error_code   Implementation defined error code on failure
 */
int fwu_bootloader_get_image_info(bl_image_id_t bootloader_image_id,
                                  bool active_image,
                                  tfm_image_info_t * info);
#ifdef __cplusplus
}
#endif
#endif /* __TFM_BOOTLOADER_FWU_ABSTRACTION_H__ */

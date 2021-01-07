/*
 * Copyright (c) 2021, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef TFM_FIRMWARE_UPDATE_H
#define TFM_FIRMWARE_UPDATE_H

#include <stddef.h>
#include <stdint.h>

#include "psa/error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TFM_FWU_MAX_BLOCK_SIZE          1024
#define TFM_FWU_INVALID_IMAGE_ID        0

/* The maximum size of an image digest in bytes. This is dependent
 * on the hash algorithm used. */
#define TFM_FWU_MAX_DIGEST_SIZE         32
#define TFM_IMAGE_INFO_INVALID_DIGEST   0xFF

/* The image ID macros. */
#define FWU_IMAGE_ID_SLOT_POSITION      0U

/* The area where the image is running. */
#define FWU_IMAGE_ID_SLOT_0             0x01U

/* The are to stage the image. */
#define FWU_IMAGE_ID_SLOT_1             0x02U
#define FWU_IMAGE_ID_SLOT_MASK          0x00FF

#define FWU_IMAGE_ID_TYPE_POSITION      8U
#define FWU_IMAGE_TYPE_NONSECURE        0x01U
#define FWU_IMAGE_TYPE_SECURE           0x02U
#define FWU_IMAGE_TYPE_FULL             0x03U
#define FWU_IMAGE_ID_RANDOM_POSITION    16U

#define FWU_CALCULATE_IMAGE_ID(slot, type) ((slot & FWU_IMAGE_ID_SLOT_MASK) | \
                                         (type << FWU_IMAGE_ID_TYPE_POSITION))
#define FWU_IMAGE_ID_GET_TYPE(image_id) (image_id >> FWU_IMAGE_ID_TYPE_POSITION)
#define FWU_IMAGE_ID_GET_SLOT(image_id) (image_id & FWU_IMAGE_ID_SLOT_MASK)

/*
 * uuid[7:0]: slot.
 * uuid[15:8]: image type.
 * uuid[31:16]: specific image ID.
 */
typedef uint32_t tfm_image_id_t;

typedef struct tfm_image_version_s {
    uint8_t iv_major;
    uint8_t iv_minor;
    uint16_t iv_revision;
    uint32_t iv_build_num;
} tfm_image_version_t;

typedef struct tfm_image_info_s {
    tfm_image_id_t type;
    tfm_image_version_t version;
    uint8_t state;
    uint8_t digest[TFM_FWU_MAX_DIGEST_SIZE];
} tfm_image_info_t;

/**
 * \brief Writes an image to its staging area.
 *
 * Writes the image data 'block' with length 'block_size' to its staging area.
 * If the image size is less than or equal to TFM_FWU_MAX_BLOCK_SIZE, the
 * caller can send the entire image in one call.
 * If the image size is greater than TFM_FWU_MAX_BLOCK_SIZE, the caller must
 * send parts of the image by calling this API multiple times with different
 * data blocks.
 *
 * \param[in] uuid            The identifier of the image type
 * \param[in] image_offset    The offset of the image being passed into block,
 *                             in bytes
 * \param[in] block            A buffer containing a block of image data. This
 *                             might be a complete image or a subset.
 * \param[in] block_size       Size of block. The size must not be greater than
 *                             PSA_FWU_MAX_BLOCK_SIZE.
 *
 * \return A status indicating the success/failure of the operation
 *
 * \retval PSA_SUCCESS                     The data in block has been
 *                                         successfully stored.
 * \retval PSA_ERROR_INVALID_ARGUMENT      One of the following error
 *                                         conditions occured:
 *                                         The parameter size is greater than
 *                                         PSA_FWU_MAX_BLOCK_SIZE;
 *                                         The parameter size is 0;
 *                                         The combination of offset and size
 *                                         is out of bounds.
 *
 * \retval PSA_ERROR_INSUFFICIENT_MEMORY   There is not enough memory to
 *                                         process the update
 * \retval PSA_ERROR_INSUFFICIENT_STORAGE  There is not enough storage to
 *                                         process the update
 * \retval PSA_ERROR_GENERAL_ERROR         A fatal error occured
 * \retval TFM_ERROR_WRITE_FAILURE        The image is currently locked for
 *                                         writing.
 */
psa_status_t tfm_fwu_write(tfm_image_id_t uuid,
                           size_t image_offset,
                           const void *block,
                           size_t block_size);


/**
 * \brief Starts the installation of an image.
 *
 * The authenticity and integrity of the image is checked during installation.
 * If a reboot is required to complete installation then the implementation
 * can choose to defer the authenticity checks to that point.
 *
 * \param[in] uuid                The identifier of the image to install
 * \param[in] dependency_uuid     If TFM_SUCCESS_DEPENDENCY_NEEDED is returned,
 *                                this parameter is filled with dependency
 *                                information.
 * \param[in] dependency_version  If TFM_SUCCESS_DEPENDENCY_NEEDED is returned,
 *                                this parameter is filled with the minimum
 *                                required version for the dependency
 *
 * \return A status indicating the success/failure of the operation
 *
 * \retval PSA_SUCCESS                     The image was successfully
 *                                         installed. The platform does not
 *                                         require a reboot.
 * \retval TFM_SUCCESS_REBOOT              A system reboot is needed to finish
 *                                         installation.
 * \retval PSA_ERROR_INVALID_ARGUMENT      Bad input parameter
 * \retval PSA_ERROR_DEPENDENCY_NEEDED     A different image requires
 *                                         installation first
 * \retval PSA_ERROR_STORAGE_FAILURE       Some persistent storage could not be
 *                                         read or written by the
 *                                         implementation
 */
psa_status_t tfm_fwu_install(const tfm_image_id_t uuid,
                             tfm_image_id_t *dependency_uuid,
                             tfm_image_version_t *dependency_version);

/**
 * \brief Aborts an ongoing installation and erases the staging area of the
 *        image.
 *
 * \param[in] uuid            The identifier of the image to abort installation
 *
 * \return A status indicating the success/failure of the operation
 *
 * \retval PSA_SUCCESS                     Installation of the provided UUID
 *                                         has been aborted
 * \retval PSA_ERROR_INVALID_ARGUMENT      No image with the provided UUID is
 *                                         currently being installed
 * \retval PSA_ERROR_NOT_PERMITTED         The caller is not authorized to to
 *                                         abort an installation
 */
psa_status_t tfm_fwu_abort(const tfm_image_id_t uuid);

/**
 * \brief Returns information for an image of a particular UUID.
 *
 * \param[in] uuid             The UUID of the image to query
 *
 * \param[out] info            Output parameter for image information
 *                             related to the UUID.
 *
 * \return A status indicating the success/failure of the operation
 *
 * \retval PSA_SUCCESS                     Image information has been returned
 * \retval PSA_ERROR_NOT_PERMITTED         The caller is not authorized to
 *                                         access platform version information
 */
psa_status_t tfm_fwu_query(const tfm_image_id_t uuid, tfm_image_info_t *info);

/**
 * \brief Requests the platform to reboot. On success, the platform initiates
 *        a reboot, and might not return to the caller.
 *
 * \return A status indicating the success/failure of the operation
 *
 * \retval PSA_SUCCESS                   The platform will reboot soon.
 * \retval PSA_ERROR_NOT_PERMITTED       The caller is not authorized to
 *                                       reboot the platform
 */
psa_status_t tfm_fwu_request_reboot(void);

/**
 * \brief Indicates to the implementation that the upgrade was successful.
 *
 * \return A status indicating the success/failure of the operation
 *
 * \retval PSA_SUCCESS                  The image and its dependencies have
 *                                      transitioned into a PSA_IMAGE_INSTALLED
 *                                      state
 * \retval PSA_ERROR_NOT_SUPPORTED      The implementation does not support a
 *                                      PSA_IMAGE_PENDING_INSTALL state
 * \retval PSA_ERROR_NOT_PERMITTED      The caller is not permitted to make
 *                                      this call
 */
psa_status_t tfm_fwu_accept(void);

#ifdef __cplusplus
}
#endif
#endif /* TFM_FIRMWARE_UPDATE_H */

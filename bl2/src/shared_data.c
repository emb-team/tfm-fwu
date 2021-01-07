/*
 * Copyright (c) 2021, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include "bootutil/boot_status.h"
#include "bootutil/boot_record.h"
#include "bootutil/image.h"
#include "flash_map/flash_map.h"
#include <string.h>

/* Firmware Update specific macros */
#define TLV_MAJOR_FWU       0x2
#define SET_FWU_MINOR(sw_module, claim) (((sw_module) << 6) | (claim))

/**
 * @brief Add a data item to the shared data area between bootloader and
 *        runtime SW
 *
 * @param[in] major_type  TLV major type, identify consumer
 * @param[in] minor_type  TLV minor type, identify TLV type
 * @param[in] size        length of added data
 * @param[in] data        pointer to data
 *
 * @return                0 on success; nonzero on failure.
 */
static int
boot_add_data_to_shared_area(uint8_t        major_type,
                             uint16_t       minor_type,
                             size_t         size,
                             const uint8_t *data)
{
    struct shared_data_tlv_entry tlv_entry = {0};
    struct shared_boot_data *boot_data;
    uint16_t boot_data_size;
    uintptr_t tlv_end, offset;
    uint32_t tmp;

    boot_data = (struct shared_boot_data *)MCUBOOT_SHARED_DATA_BASE;

    if (boot_data->header.tlv_magic != SHARED_DATA_TLV_INFO_MAGIC) {
        boot_data->header.tlv_magic = SHARED_DATA_TLV_INFO_MAGIC;
        boot_data->header.tlv_tot_len = SHARED_DATA_HEADER_SIZE;
    }

    /* Check whether TLV entry is already added.
     * Get the boundaries of TLV section
     */
    tlv_end = MCUBOOT_SHARED_DATA_BASE + boot_data->header.tlv_tot_len;
    offset  = MCUBOOT_SHARED_DATA_BASE + SHARED_DATA_HEADER_SIZE;

    /* Iterates over the TLV section looks for the same entry if found then
     * returns with error -1
     */
    while (offset < tlv_end) {
        /* Create local copy to avoid unaligned access */
        memcpy(&tlv_entry,
               (const void *)offset,
               SHARED_DATA_ENTRY_HEADER_SIZE);
        if (GET_MAJOR(tlv_entry.tlv_type) == major_type &&
            GET_MINOR(tlv_entry.tlv_type) == minor_type) {
            return -1;
        }

        offset += SHARED_DATA_ENTRY_SIZE(tlv_entry.tlv_len);
    }

    /* Add TLV entry */
    tlv_entry.tlv_type = SET_TLV_TYPE(major_type, minor_type);
    tlv_entry.tlv_len  = size;
    tmp = boot_data->header.tlv_tot_len + SHARED_DATA_ENTRY_SIZE(size);
    if (tmp > UINT16_MAX) {
        return -2;
    } else {
        boot_data_size = (uint16_t)tmp;
    }

    /* Verify overflow of shared area */
    if (boot_data_size > MCUBOOT_SHARED_DATA_SIZE) {
        return -3;
    }

    offset = tlv_end;
    memcpy((void *)offset, &tlv_entry, SHARED_DATA_ENTRY_HEADER_SIZE);

    offset += SHARED_DATA_ENTRY_HEADER_SIZE;
    memcpy((void *)offset, data, size);

    boot_data->header.tlv_tot_len += SHARED_DATA_ENTRY_SIZE(size);

    return 0;
}

/**
 * Add application specific data to the shared memory area between the
 * bootloader and runtime SW.
 *
 * @param[in]  hdr        Pointer to the image header stored in RAM.
 * @param[in]  fap        Pointer to the flash area where image is stored.
 *
 * @return                0 on success; nonzero on failure.
 */
int boot_save_shared_data(const struct image_header *hdr,
                          const struct flash_area *fap)
{
    uint16_t fwu_minor;
    struct image_version image_ver;

    if (hdr == NULL || fap == NULL) {
        return -1;
    }

    image_ver = hdr->ih_ver;

    //TODO: add the module identifier into the fwu_minor after the module
    //arg is supported.
    /* Currently hardcode it to 0 which indicates the full image. */
    fwu_minor = SET_FWU_MINOR(0, SW_VERSION);
    return boot_add_data_to_shared_area(TLV_MAJOR_FWU,
                                        fwu_minor,
                                        sizeof(image_ver),
                                        (const uint8_t *)&image_ver);
}

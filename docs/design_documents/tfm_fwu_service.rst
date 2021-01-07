#######################
Firmware Update Service
#######################

:Author: Sherry Zhang
:Organization: Arm Limited
:Contact: Sherry Zhang <Sherry.Zhang2@arm.com>
:Status: Draft

***************************************
Introduction of Firmware Update service
***************************************
The Firmware Update(FWU) service provides the functionality of updating firmware
images. It provides a standard interface for updating firmware and it is platform
independent. TF-M defines a shim layer to support cooperation between bootloader
and FWU service.

This partition supports the following features:

- Query image information
  Fetch information about the firmware images on the device. This covers the
  images in the running area and also the staging area.
- Store image
  Write a candidate image to its staging area.
- Validate image
  Starts the validation of the image.
- Trigger reboot
  Trigger a reboot to restart the platform.

**************
Code structure
**************
The code structure of the service will be as follows:

- ``interface/``

    - ``interface/include/firmware_update.h`` - TFM FWU API
    - ``src/tfm_firmware_update_func_api.c`` - TFM FWU API implementation for NSPE

- ``secure_fw/partitions/firware_update/``

    - ``tfm_firmware_update.yaml`` - Partition manifest
    - ``tfm_fwu_secure_api.c`` - TFM FWU API implementation for SPE
    - ``tfm_fwu_req_mngr.c`` - Uniform secure functions and IPC request handlers
    - ``tfm_fwu_internal.c`` - Converts the image structure between bootloader
                               and FWU
    - ``tfm_bootloader_fwu_abstraction.h`` - The shim layer of between FWU and
                                             bootloader
    - ``bootloader/mcuboot/tfm_mcuboot_fwu.c`` - The implementation of shim
                                                 layer APIs based on MCUboot

***********************
TF-M FWU implementation
***********************

User interfaces
===============

The user interfaces exposed by this service are listed below:

tfm_fwu_write(function)
-----------------------
Prototype
^^^^^^^^^
.. code-block:: c

    psa_status_t tfm_fwu_write(tfm_image_id_t uuid,
                               size_t         image_offset,
                               const void     *block,
                               size_t         block_size);

Description
^^^^^^^^^^^
  Writes an image to its staging area.

Parameters
^^^^^^^^^^
    - ``uuid``: The identifier of the image type.
    - ``image_offset``: The offset of the block in the whole image data.
    - ``block``: A buffer containing image data. This may be the full image or a subset.
    - ``block_size``: Size of block.

tfm_fwu_install(function)
-------------------------
Prototype
^^^^^^^^^
.. code-block:: c

    psa_status_t tfm_fwu_install(const tfm_image_id_t uuid,
                                 tfm_image_id_t       *dependency_uuid,
                                 tfm_image_version_t  *dependency_version);

Description
^^^^^^^^^^^
    Starts the installation of an image.
    The authenticity and integrity of the image are checked during installation.
    If a reboot is required to complete installation then the implementation
    can choose to defer the authenticity checks to that point. In this case
    the ``TFM_SUCCESS_REBOOT`` code is returned.

Parameters
^^^^^^^^^^

    - ``uuid``: The identifier of the image to install.
    - ``dependency_uuid``: If TFM_SUCCESS_DEPENDENCY_NEEDED is returned,
                       this parameter is filled with dependency information.
    - ``dependency_version``: If TFM_SUCCESS_DEPENDENCY_NEEDED is returned,
                          this parameter is filled with the minimum
                          required version for the dependency.

tfm_fwu_abort(function)
-----------------------
Prototype
^^^^^^^^^
.. code-block:: c

    psa_status_t tfm_fwu_abort(const tfm_image_id_t uuid);

Description
^^^^^^^^^^^
Aborts an ongoing installation and erases the staging area of the image.

Parameters
^^^^^^^^^^
    - ``uuid``: The identifier of the image to abort installation.

tfm_fwu_query(function)
-----------------------
Prototype
^^^^^^^^^
.. code-block:: c

    psa_status_t tfm_fwu_query(const tfm_image_id_t uuid,
                               tfm_image_info_t     *info);

Description
^^^^^^^^^^^
Returns image digest and image version information for an image of a particular UUID.

Parameters
^^^^^^^^^^
    - ``uuid``: The UUID of the image to query.

tfm_fwu_request_reboot(function)
--------------------------------
Prototype
^^^^^^^^^
.. code-block:: c

    psa_status_t tfm_fwu_request_reboot(void);

Description
^^^^^^^^^^^
Requests the platform to reboot. On success, the platform initiates a reboot.

Parameters
^^^^^^^^^^
    N/A

tfm_fwu_accept(function)
------------------------
Prototype
^^^^^^^^^
.. code-block:: c

    psa_status_t tfm_fwu_accept(void);

Description
^^^^^^^^^^^
Indicates to the implementation that the upgrade was successful.

Parameters
^^^^^^^^^^
    N/A

Firmware update process based on TF-M FWU service
=================================================

The firmware update process based on TF-M FWU service is listed below:

- Call ``tfm_fwu_write`` to download the image into the staging area
- Call ``tfm_fwu_install`` to start the installation
- Depending on the result of the last step, call ``tfm_fwu_reboot`` or repeat
  steps 1-2 to update the dependency image if needed.
- After the image has been verified(usually after a reboot), call ``tfm_fwu_accept``
  to mark this image as accepted if needed.

Please refer the file ``interface/include/firmware_update.h`` for details of each FWU API.

Detailed design considerations
==============================

The APIs in ``tfm_fwu_req_mngr.c`` should provide the entries to the firmware update
partition. The image state is managed in this file to ensure the firmware update
operations are only processed when the image is at the right state.

The firmware update operations are achieved by calling the shim layer APIs between
bootloader and FWU.

Shim layer between bootloader and Firmware Update partition
-----------------------------------------------------------

This shim layer provides the APIs with the functionality of operating the
bootloader to cooperate with the Firmware Update service. This shim layer
is decoupled from bootloader implementation. Users can specify a specific
bootloader by setting ``TFM_FWU_BOOTLOADER_LIB`` build configuration and
adding the specific build scripts into that file. By default, the MCUboot
is chosen as the bootloader.

The shim layer APIs are:

fwu_bootloader_init(function)
-----------------------------
Prototype
^^^^^^^^^
.. code-block:: c

    int fwu_bootloader_init(void);

Description
^^^^^^^^^^^
Bootloader related initialization for the firmware update, such as reading
some necessary shared data from the memory if needed.

Parameters
^^^^^^^^^^
    N/A

fwu_bootloader_staging_area_init(function)
------------------------------------------
Prototype
^^^^^^^^^
.. code-block:: c

    int fwu_bootloader_staging_area_init(bl_image_id_t bootloader_image_id);

Description
^^^^^^^^^^^
Prepare the staging area of the image with the given ID for image download.
For example, initialize the staging area, open the flash area, and so on.
The image will be written into the staging area later.

Parameters
^^^^^^^^^^
- ``bootloader_image_id``: The identifier of the target image in bootloader.

fwu_bootloader_load_image(function)
-----------------------------------
Prototype
^^^^^^^^^

.. code-block:: c

    int fwu_bootloader_load_image(bl_image_id_t bootloader_image_id,
                                  size_t        image_offset,
                                  const void    *block,
                                  size_t        block_size);

Description
^^^^^^^^^^^
Prepare the staging area of the image with the given ID for image download.
For example, initialize the staging area, open the flash area, and so on.
The image will be written into the staging area later.

Parameters
^^^^^^^^^^
- ``bootloader_image_id``: The identifier of the target image in bootloader.
- ``image_offset``: The offset of the image being passed into block, in bytes.
- ``block``: A buffer containing a block of image data. This might be a complete
         image or a subset.
- ``block_size``: Size of block.

fwu_bootloader_mark_image_candidate(function)
---------------------------------------------
Prototype
^^^^^^^^^
.. code-block:: c

    int fwu_bootloader_mark_image_candidate(bl_image_id_t       bootloader_image_id,
                                            bl_image_id_t       *dependency,
                                            tfm_image_version_t *dependency_version);

Description
^^^^^^^^^^^
Mark the image in the staging area as a candidate for bootloader so that the
next time bootloader runs, it will take this image as a candidate one to
bootup.

Parameters
^^^^^^^^^^
- ``bootloader_image_id``: The identifier of the target image in bootloader.
- ``dependency``: Bootloader image ID of dependency if needed.
- ``dependency_version``: Bootloader image version of dependency if needed.

fwu_bootloader_mark_image_accepted(function)
--------------------------------------------
Prototype
^^^^^^^^^
.. code-block:: c

    int fwu_bootloader_mark_image_accepted(void);

Description
^^^^^^^^^^^
Call this API to mark the running images as permanent/accepted to avoid
revert when next time bootup. Usually, this API is called after the running
images have been verified as valid.

Parameters
^^^^^^^^^^
    N/A

fwu_bootloader_abort(function)
------------------------------
Prototype
^^^^^^^^^
.. code-block:: c

    int fwu_bootloader_abort(void);

Description
^^^^^^^^^^^
Abort the current image download process.

Parameters
^^^^^^^^^^
    N/A

fwu_bootloader_get_image_info(function)
---------------------------------------
Prototype
^^^^^^^^^
.. code-block:: c

    int fwu_bootloader_get_image_info(bl_image_id_t    bootloader_image_id,
                                      bool             staging_area,
                                      tfm_image_info_t *info);

Description
^^^^^^^^^^^
Get the image information of the given bootloader_image_id in the staging area
or the running area.

Parameters
^^^^^^^^^^
    - ``bootloader_image_id``: The identifier of the target image in bootloader.
    - ``active_image``: Indicates image location.
                    True: the running image.
                    False: the image in the passive(or staging) slot.
    - ``info``: Buffer containing the image information.

Additional shared data between BL2 and SPE
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

An additional TLV area "image version" is added into the shared memory between
BL2 and TF-M. So that the firmware update partition can get the image version.
Even though the image version information is also included in the ``BOOT RECORD``
TLV area which is encoded by CBOR, adding a dedicated ``image version`` TLV area
is preferred to avoid involving the CBOR encoder which can increase the code
size. The FWU partition will read the shared data at the partition
initialization.

Image ID structure
^^^^^^^^^^^^^^^^^^

The structure of image ID is:
    uuid[7:0]: slot.
    uuid[15:8]: image type.
    uuid[31:16]: specific image ID.

Three image types are defined in this partition.
- FWU_IMAGE_TYPE_NONSECURE: the non_secure image
- FWU_IMAGE_TYPE_SECURE: the secure image
- FWU_IMAGE_TYPE_FULL: the secure + non_secure image

.. Note::

    Only **FWU_IMAGE_TYPE_FULL** is supported currently.

Macros **FWU_CALCULATE_IMAGE_ID**, **FWU_IMAGE_ID_GET_TYPE** and **FWU_IMAGE_ID_GET_SLOT**
are dedicated to converting the image id, type, and slot.

***********************************
Benefits Analysis on this Partition
***********************************

Implement the FWU functionality in the non-secure side
======================================================
The Firmware Update APIs listed in `User interfaces`_ can also be implemented
in the non-secure side. The library model implementation can be referred to for
the non-secure side implementation.

Pros and Cons for Implementing FWU APIs in Secure Side
======================================================

Pros
----
- It protects the image in the passive or staging area from being tampered with
  by the NSPE. Otherwise, a malicious actor from NSPE can tamper the image
  stored in the non-secure area to break image update.

- It protects secure image information from disclosure. In some cases, the
  non-secure side shall not be permitted to get secure image information.

- It protects the active image from being manipulated by NSPE. Some bootloader
  supports testing the image. After the image is successfully installed and
  starts to run, the user should set the image as permanent image if the image
  passes the test. To achieve this, the area of the active image needs to be
  accessed. In this case, implementing FWU service in SPE can prevent NSPE
  from manipulating the active image area.

- On some devices, such as the Arm Musca_b1 board, the passive or staging area
  is restricted as secure access only. In this case, the FWU partition should
  be implemented in the secure side.

Cons
----
- It increases the image size of the secure image.
- It increases the execution latency and footprint. Compared to implementing
  FWU in NSPE directly, calling the Firmware Update APIs which are implemented
  in the secure side increases the execution latency and footprint.

Users can decide whether to call the FWU service in TF-M directly or implement
the Firmware Update APIs in the non-secure side based on the pros and cons
analysis above.

*Copyright (c) 2021, Arm Limited. All rights reserved.*

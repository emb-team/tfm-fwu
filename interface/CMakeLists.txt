#-------------------------------------------------------------------------------
# Copyright (c) 2020, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.15)
cmake_policy(SET CMP0076 NEW)
cmake_policy(SET CMP0079 NEW)


if (TFM_MULTI_CORE_TOPOLOGY)
    include(${CMAKE_SOURCE_DIR}/platform/ext/target/${TFM_PLATFORM}/preload_ns.cmake)
    tfm_toolchain_reload_compiler()
    # The platform target is created in this directory/file so that it has the
    # same settings as the main ns target.
    add_library(platform_ns STATIC EXCLUDE_FROM_ALL)
endif()

###################### PSA interface (header only) #############################

add_library(psa_interface INTERFACE)

target_include_directories(psa_interface
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_BINARY_DIR}/generated/interface/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/os_wrapper
)

# PSA interface files are generated from a template
add_dependencies(psa_interface
    tfm_generated_files
)

target_link_libraries(psa_interface
    INTERFACE
        tfm_partition_defs
)

target_compile_definitions(psa_interface
    INTERFACE
        $<$<BOOL:${TFM_PSA_API}>:TFM_PSA_API>
        $<$<BOOL:${TFM_NS_CLIENT_IDENTIFICATION}>:TFM_NS_CLIENT_IDENTIFICATION>
        $<$<BOOL:${CONFIG_TFM_ENABLE_CTX_MGMT}>:CONFIG_TFM_ENABLE_CTX_MGMT>
        $<$<BOOL:${TFM_ISOLATION_LEVEL}>:TFM_LVL=${TFM_ISOLATION_LEVEL}>
        $<$<BOOL:${TFM_MULTI_CORE_TOPOLOGY}>:TFM_MULTI_CORE_TOPOLOGY>
        $<$<BOOL:${TFM_MULTI_CORE_MULTI_CLIENT_CALL}>:TFM_MULTI_CORE_MULTI_CLIENT_CALL>
        $<$<BOOL:${FORWARD_PROT_MSG}>:FORWARD_PROT_MSG=${FORWARD_PROT_MSG}>
)

###################### PSA api (S lib) #########################################

target_sources(tfm_secure_api
    INTERFACE
        $<$<BOOL:${TFM_PSA_API}>:${CMAKE_CURRENT_SOURCE_DIR}/src/psa/psa_client.c>
        $<$<BOOL:${TFM_PSA_API}>:${CMAKE_CURRENT_SOURCE_DIR}/src/psa/psa_service.c>
        $<$<BOOL:${TFM_PSA_API}>:${CMAKE_CURRENT_SOURCE_DIR}/src/psa/psa_lifecycle.c>
        ${CMAKE_CURRENT_SOURCE_DIR}/src/log/tfm_log_raw.c
)

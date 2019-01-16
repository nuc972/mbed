/*
 * Copyright (c) 2018-2020, Nuvoton Technology Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include "platform/include/tfm_spm_hal.h"
#include "spm_api.h"
#include "spm_db.h"
#include "target_cfg.h"
#include "mpu_armv8m_drv.h"
#include "secure_utilities.h"
#include "mbed_error.h"

struct mpu_armv8m_dev_t dev_mpu_s = { MPU_BASE };

void tfm_spm_hal_init_isolation_hw(void)
{
    TZ_SAU_Setup();
    SCU_Setup();
    FMC_NSBA_Setup();
}

void tfm_spm_hal_configure_default_isolation(
        const struct tfm_spm_partition_platform_data_t *platform_data)
{
    if (platform_data) {
        /* TODO */
    }
}

#if TFM_LVL > 1

/* TODO */
#error("TFM level 2/3 are not supported yet")

#define MPU_REGION_VENEERS           0
#define MPU_REGION_TFM_UNPRIV_CODE   1
#define MPU_REGION_TFM_UNPRIV_DATA   2
#define MPU_REGION_NS_DATA           3
#define PARTITION_REGION_RO          4
#define PARTITION_REGION_RW_STACK    5
#define PARTITION_REGION_PERIPH      6
#define PARTITION_REGION_SHARE       7

REGION_DECLARE(Image$$, TFM_UNPRIV_CODE, $$RO$$Base);
REGION_DECLARE(Image$$, TFM_UNPRIV_CODE, $$RO$$Limit);
REGION_DECLARE(Image$$, TFM_UNPRIV_RO_DATA, $$RW$$Base);
REGION_DECLARE(Image$$, TFM_UNPRIV_RO_DATA, $$ZI$$Limit);
REGION_DECLARE(Image$$, TFM_UNPRIV_SCRATCH, $$ZI$$Base);
REGION_DECLARE(Image$$, TFM_UNPRIV_SCRATCH, $$ZI$$Limit);

static enum spm_err_t tfm_spm_mpu_init(void)
{
    struct mpu_armv8m_region_cfg_t region_cfg;

    mpu_armv8m_clean(&dev_mpu_s);

    /* Veneer region */
    region_cfg.region_nr = MPU_REGION_VENEERS;
    region_cfg.region_base = memory_regions.veneer_base;
    region_cfg.region_limit = memory_regions.veneer_limit;
    region_cfg.attr_access = MPU_ARMV8M_AP_RO_PRIV_UNPRIV;
    region_cfg.attr_sh = MPU_ARMV8M_SH_NONE;
    region_cfg.attr_exec = MPU_ARMV8M_XN_EXEC_OK;
    if (mpu_armv8m_region_enable(&dev_mpu_s, &region_cfg) != MPU_ARMV8M_OK) {
        return SPM_ERR_INVALID_CONFIG;
    }

    /* TFM Core unprivileged code region */
    region_cfg.region_nr = MPU_REGION_TFM_UNPRIV_CODE;
    region_cfg.region_base =
        (uint32_t)&REGION_NAME(Image$$, TFM_UNPRIV_CODE, $$RO$$Base);
    region_cfg.region_limit =
        (uint32_t)&REGION_NAME(Image$$, TFM_UNPRIV_CODE, $$RO$$Limit);
    region_cfg.attr_access = MPU_ARMV8M_AP_RO_PRIV_UNPRIV;
    region_cfg.attr_sh = MPU_ARMV8M_SH_NONE;
    region_cfg.attr_exec = MPU_ARMV8M_XN_EXEC_OK;
    if (mpu_armv8m_region_enable(&dev_mpu_s, &region_cfg) != MPU_ARMV8M_OK) {
        return SPM_ERR_INVALID_CONFIG;
    }

    /* TFM Core unprivileged data region */
    region_cfg.region_nr = MPU_REGION_TFM_UNPRIV_DATA;
    region_cfg.region_base =
        (uint32_t)&REGION_NAME(Image$$, TFM_UNPRIV_RO_DATA, $$RW$$Base);
    region_cfg.region_limit =
        (uint32_t)&REGION_NAME(Image$$, TFM_UNPRIV_RO_DATA, $$ZI$$Limit);
    region_cfg.attr_access = MPU_ARMV8M_AP_RO_PRIV_UNPRIV;
    region_cfg.attr_sh = MPU_ARMV8M_SH_NONE;
    region_cfg.attr_exec = MPU_ARMV8M_XN_EXEC_NEVER;
    if (mpu_armv8m_region_enable(&dev_mpu_s, &region_cfg) != MPU_ARMV8M_OK) {
        return SPM_ERR_INVALID_CONFIG;
    }

    /* TFM Core unprivileged non-secure data region */
    region_cfg.region_nr = MPU_REGION_NS_DATA;
    region_cfg.region_base = NS_DATA_START;
    region_cfg.region_limit = NS_DATA_LIMIT;
    region_cfg.attr_access = MPU_ARMV8M_AP_RW_PRIV_UNPRIV;
    region_cfg.attr_sh = MPU_ARMV8M_SH_NONE;
    region_cfg.attr_exec = MPU_ARMV8M_XN_EXEC_NEVER;
    if (mpu_armv8m_region_enable(&dev_mpu_s, &region_cfg) != MPU_ARMV8M_OK) {
        return SPM_ERR_INVALID_CONFIG;
    }

    mpu_armv8m_enable(&dev_mpu_s, PRIVILEGED_DEFAULT_ENABLE,
                      HARDFAULT_NMI_ENABLE);

    return SPM_ERR_OK;
}

enum spm_err_t tfm_spm_hal_partition_sandbox_config(
        const struct tfm_spm_partition_memory_data_t *memory_data,
        const struct tfm_spm_partition_platform_data_t *platform_data)
{
    /* This function takes a partition id and enables the
     * SPM partition for that partition
     */

    struct mpu_armv8m_region_cfg_t region_cfg;

    mpu_armv8m_disable(&dev_mpu_s);

    /* Configure Regions */
    if (memory_data->ro_start) {
        /* RO region */
        region_cfg.region_nr = PARTITION_REGION_RO;
        region_cfg.region_base = memory_data->ro_start;
        region_cfg.region_limit = memory_data->ro_limit;
        region_cfg.attr_access = MPU_ARMV8M_AP_RO_PRIV_UNPRIV;
        region_cfg.attr_sh = MPU_ARMV8M_SH_NONE;
        region_cfg.attr_exec = MPU_ARMV8M_XN_EXEC_OK;

        if (mpu_armv8m_region_enable(&dev_mpu_s, &region_cfg)
            != MPU_ARMV8M_OK) {
            return SPM_ERR_INVALID_CONFIG;
        }
    }

    /* RW, ZI and stack as one region */
    region_cfg.region_nr = PARTITION_REGION_RW_STACK;
    region_cfg.region_base = memory_data->rw_start;
    region_cfg.region_limit = memory_data->stack_top;
    region_cfg.attr_access = MPU_ARMV8M_AP_RW_PRIV_UNPRIV;
    region_cfg.attr_sh = MPU_ARMV8M_SH_NONE;
    region_cfg.attr_exec = MPU_ARMV8M_XN_EXEC_NEVER;

    if (mpu_armv8m_region_enable(&dev_mpu_s, &region_cfg) != MPU_ARMV8M_OK) {
        return SPM_ERR_INVALID_CONFIG;
    }

    if (platform_data) {
        /* Peripheral */
        region_cfg.region_nr = PARTITION_REGION_PERIPH;
        region_cfg.region_base = platform_data->periph_start;
        region_cfg.region_limit = platform_data->periph_limit;
        region_cfg.attr_access = MPU_ARMV8M_AP_RW_PRIV_UNPRIV;
        region_cfg.attr_sh = MPU_ARMV8M_SH_NONE;
        region_cfg.attr_exec = MPU_ARMV8M_XN_EXEC_NEVER;
        if (mpu_armv8m_region_enable(&dev_mpu_s, &region_cfg)
            != MPU_ARMV8M_OK) {
            return SPM_ERR_INVALID_CONFIG;
        }

        ppc_en_secure_unpriv(platform_data->periph_ppc_bank,
                             platform_data->periph_ppc_loc);
    }

    mpu_armv8m_enable(&dev_mpu_s, PRIVILEGED_DEFAULT_ENABLE,
                      HARDFAULT_NMI_ENABLE);

    return SPM_ERR_OK;
}

enum spm_err_t tfm_spm_hal_partition_sandbox_deconfig(
        const struct tfm_spm_partition_memory_data_t *memory_data,
        const struct tfm_spm_partition_platform_data_t *platform_data)
{
    /* This function takes a partition id and disables the
     * SPM partition for that partition
     */

    if (platform_data) {
        /* Peripheral */
        ppc_clr_secure_unpriv(platform_data->periph_ppc_bank,
                              platform_data->periph_ppc_loc);
    }

    mpu_armv8m_disable(&dev_mpu_s);
    mpu_armv8m_region_disable(&dev_mpu_s, PARTITION_REGION_RO);
    mpu_armv8m_region_disable(&dev_mpu_s, PARTITION_REGION_RW_STACK);
    mpu_armv8m_region_disable(&dev_mpu_s, PARTITION_REGION_PERIPH);
    mpu_armv8m_region_disable(&dev_mpu_s, PARTITION_REGION_SHARE);
    mpu_armv8m_enable(&dev_mpu_s, PRIVILEGED_DEFAULT_ENABLE,
                      HARDFAULT_NMI_ENABLE);

    return SPM_ERR_OK;
}

/**
 * Set share region to which the partition needs access
 */
enum spm_err_t tfm_spm_hal_set_share_region(
        enum tfm_buffer_share_region_e share)
{
    struct mpu_armv8m_region_cfg_t region_cfg;
    enum spm_err_t res = SPM_ERR_INVALID_CONFIG;
    uint32_t scratch_base =
        (uint32_t)&REGION_NAME(Image$$, TFM_UNPRIV_SCRATCH, $$ZI$$Base);
    uint32_t scratch_limit =
        (uint32_t)&REGION_NAME(Image$$, TFM_UNPRIV_SCRATCH, $$ZI$$Limit);

    mpu_armv8m_disable(&dev_mpu_s);

    if (share == TFM_BUFFER_SHARE_DISABLE) {
        mpu_armv8m_region_disable(&dev_mpu_s, PARTITION_REGION_SHARE);
    } else {

        region_cfg.region_nr = PARTITION_REGION_SHARE;
        region_cfg.attr_access = MPU_ARMV8M_AP_RW_PRIV_UNPRIV;
        region_cfg.attr_sh = MPU_ARMV8M_SH_NONE;
        region_cfg.attr_exec = MPU_ARMV8M_XN_EXEC_NEVER;
        switch (share) {
        case TFM_BUFFER_SHARE_SCRATCH:
            /* Use scratch area for SP-to-SP data sharing */
            region_cfg.region_base = scratch_base;
            region_cfg.region_limit = scratch_limit;
            res = SPM_ERR_OK;
            break;
        case TFM_BUFFER_SHARE_NS_CODE:
            region_cfg.region_base = memory_regions.non_secure_partition_base;
            region_cfg.region_limit = memory_regions.non_secure_partition_limit;
            /* Only allow read access to NS code region and keep
             * exec.never attribute
             */
            region_cfg.attr_access = MPU_ARMV8M_AP_RO_PRIV_UNPRIV;
            res = SPM_ERR_OK;
            break;
        default:
            /* Leave res to be set to SPM_ERR_INVALID_CONFIG */
            break;
        }
        if (res == SPM_ERR_OK) {
            mpu_armv8m_region_enable(&dev_mpu_s, &region_cfg);
        }
    }
    mpu_armv8m_enable(&dev_mpu_s, PRIVILEGED_DEFAULT_ENABLE,
                      HARDFAULT_NMI_ENABLE);

    return res;
}

#endif

void tfm_spm_hal_setup_isolation_hw(void)
{
#if TFM_LVL > 1
    if (tfm_spm_mpu_init() != SPM_ERR_OK) {
        /* FIXME: Replace with MBED_ERROR1??? */
        ERROR_MSG("Failed to set up initial MPU configuration! Halting.");
        while (1);
    }
#endif
}

void SCU_IRQHandler(void)
{
    uint32_t scu_sv_ints = SCU_GET_INT_FLAG(NU_SCU_SV_MSAK);

    /* TODO: More detailed debug message */
    LOG_MSG("Oops... MPC fault!!!");

    /* Clear SCU interrupt flag and pending SCU IRQ */
    SCU_CLR_INT_FLAG(scu_sv_ints);
    NVIC_ClearPendingIRQ(SCU_IRQn);

    /* Inform TF-M core that isolation boundary has been violated */
    tfm_access_violation_handler();
}

uint32_t tfm_spm_hal_get_ns_VTOR(void)
{
    return memory_regions.non_secure_code_start;
}

uint32_t tfm_spm_hal_get_ns_MSP(void)
{
    return *((uint32_t *)memory_regions.non_secure_code_start);
}

uint32_t tfm_spm_hal_get_ns_entry_point(void)
{
    return *((uint32_t *)(memory_regions.non_secure_code_start+ 4));
}

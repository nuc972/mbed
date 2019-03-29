/*
 * Copyright (c) 2019-2020, Nuvoton Technology Corporation
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

#if defined(DOMAIN_NS) && (DOMAIN_NS == 1L) && (TFM_LVL > 0)

#include <stdint.h>
#include "mbed_critical.h"
#include "mbed_error.h"
#include "nu_tfm_ns_lock_sup.h"

/* Enforce locked entry into TFM for platform-specific NSC calls
 *
 * On M2351, some spaces like SYS/CLK are hard-wired to secure and cannot change.
 * To access these spaces from non-secure world, we must provide platform-specific NSC
 * functions. With TFM introduced, we must synchronize NSC calls into TFM to keep TFM
 * in sync instead of straight NSC calls. To achieve this goal, we go with the following
 * approach:
 *
 * 1. Like standard PSA APIs, enforce locked entry through tfm_ns_lock_dispatch().
 * 2. Run platform-specific secure functions in default secure partition, in which these
 *    SYS/CLK spaces must have configured to be accessible.
 */

int32_t nu_tfm_ns_lock_dispatch(veneer_fn fn,
                                 uint32_t arg0,
                                 uint32_t arg1,
                                 uint32_t arg2,
                                 uint32_t arg3)
{
    if (!tfm_ns_lock_get_init_state()) {
        /* NS lock not initialized yet and so pre-rtos, call into secure world straight. */
        return fn(arg0, arg1, arg2, arg3);
    } else if (!tfm_ns_lock_get_lock_state() && !core_util_are_interrupts_enabled()) {
        /* NS lock non-locked and interrupt disabled, regard it as the current thread has 
         * acquired NS lock and can call into secure world straight.
         *
         * NOTE: The call into secure world in interrupt-disabled condition isn't always
         *       allowed. Application will get in trouble with it. Currently, we can
         *       only rely on this approach to allow interrupt-disabled secure call to
         *       some degree until there's a sound solution.
         *
         * Know paths of interrupt-disabled secure call:
         * - mbed-os/platform/mbed_sleep_manager.c > sleep_manager_sleep_auto >
         *   hal_sleep/hal_deepsleep > nu_idle_s/nu_powerdown_s
         * - mbed-os/hal/LowPowerTickerWrapper.cpp > LowPowerTickerWrapper::init > 
         *   lp_ticker_init > SYS_ResetModule_S/CLK_SetModuleClock_S/CLK_EnableModuleClock_S
         * - mbed-os/platform/mbed_board.c > mbed_die > pin_function_s
         * - mbed-os-tests-mbed_hal-rtc > rtc_write_read_test > rtc_write >
         *   CLK_IsRTCClockEnabled_S
         */
        return fn(arg0, arg1, arg2, arg3);
    } else if (tfm_ns_lock_dispatch(fn, arg0, arg1, arg2, arg3) != 0) {
        /* NS lock owned by another thread, we are now in error path. Error it. */
        MBED_ERROR1(MBED_MAKE_ERROR(MBED_MODULE_HAL, MBED_ERROR_CODE_MUTEX_LOCK_FAILED), "Cannot acquire NS lock with interrupt disabled", (uintptr_t) fn);
    }
}

#endif

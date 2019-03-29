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

/* Enforce locked entry into TFM for target-specific NSC calls
 *
 * On M2351, some spaces like SYS/CLK are hard-wired to secure and cannot change.
 * To access these spaces from non-secure world, we must provide target-specific NSC
 * functions. With TFM introduced, we must synchronize NSC calls into TFM to keep TFM
 * in sync instead of straight NSC calls. To achieve this goal, we go with the following
 * approach:
 *
 * 1. Like standard PSA APIs, enforce locked entry through tfm_ns_lock_dispatch().
 * 2. Run target-specific secure functions in default secure partition, in which these
 *    SYS/CLK spaces must have configured to be accessible.
 *
 * Problem: Acquire NS lock when interrupt is disabled
 *          In tfm_ns_lock_dispatch(), mutex operations requuire "NOT IN IRQ" and
 *          "IRQ NOT MASKED". An already-known use case will disobey this requirement:
 *          In Sleep Manager, interrupt is disabled and then NSC functions are called:
 * 
 *          sleep_manager_sleep_auto() > core_util_critical_section_enter() >
 *          hal_sleep()/hal_deepsleep() > SYS_UnlockReg_S() > CLK_Idle_S()/
 *          CLK_PowerDown_S() > SYS_LockReg_S()
 *
 * Solution: Replace tfm_ns_lock_dispatch() with nu_tfm_ns_lock_dispatch() which takes
 *           interrupt into consideration.
 *           For the Sleep Manager use case, degenerate to __WFI().
 *           For others, error it.
 */

int32_t nu_tfm_ns_lock_dispatch(veneer_fn fn,
                                uint32_t arg0,
                                uint32_t arg1,
                                uint32_t arg2,
                                uint32_t arg3)
{
    return nu_tfm_ns_lock_dispatch2(fn, arg0, arg1, arg2, arg3, NULL);
}

int32_t nu_tfm_ns_lock_dispatch2(veneer_fn fn,
                                 uint32_t arg0,
                                 uint32_t arg1,
                                 uint32_t arg2,
                                 uint32_t arg3,
                                 veneer_fn ns_repl_fn)
{
    if (tfm_ns_lock_get_init_state()) {
        /* NS lock initialized and so rtos ready */
        if (core_util_are_interrupts_enabled()) {
            /* Interrupt enabled, we can acquire NS lock in tfm_ns_lock_dispatch(). */
            return tfm_ns_lock_dispatch(fn, arg0, arg1, arg2, arg3);
        } else {
            /* Interrupt disabled, we cannot acquire NS lock in tfm_ns_lock_dispatch(). */
            if (tfm_ns_lock_get_lock_state()) {
                /* NS lock owned by another thread, we are now in error path because
                 * acquiring NS lock when interrupt is disabled is prohibited. */
                if (ns_repl_fn) {
                    /* NS replacement for veneer, call it. */
                    return ns_repl_fn(arg0, arg1, arg2, arg3);
                } else {
                    /* Error it. */
                    MBED_ERROR(MBED_MAKE_ERROR(MBED_MODULE_HAL, MBED_ERROR_CODE_MUTEX_LOCK_FAILED), "Cannot acquire NS lock with interrupt disabled");
                }
            } else {
                /* NS lock not owned by any thread, we can see it as we have acquired
                 * NS lock due to interrupt disabled and can call veneer straight. */
                return fn(arg0, arg1, arg2, arg3);
            }
        }
    } else {
        /* NS lock not initialized yet and so pre-rtos, we can call veneer straight. */
        return fn(arg0, arg1, arg2, arg3);
    }
}

#endif

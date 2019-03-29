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

#ifndef __NU_TFM_NS_LOCK_SUP_H__
#define __NU_TFM_NS_LOCK_SUP_H__

#if defined(DOMAIN_NS) && (DOMAIN_NS == 1L) && (TFM_LVL > 0)

#include <stdint.h>
#include "tfm_ns_lock.h"

#ifdef __cplusplus
extern "C" {
#endif

/* An enhanced version of tfm_ns_lock_dispatch()
 *
 * 1. Allow secure call in pre-rtos stage
 * 2. Allow secure call in some interrupt-disabled condition
 * 3. Error it when secure call is not allowed
 */
int32_t nu_tfm_ns_lock_dispatch(veneer_fn fn,
                                 uint32_t arg0,
                                 uint32_t arg1,
                                 uint32_t arg2,
                                 uint32_t arg3);

#ifdef __cplusplus
}
#endif

#endif
#endif

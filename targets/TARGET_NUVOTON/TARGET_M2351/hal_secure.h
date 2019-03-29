/* mbed Microcontroller Library
 * Copyright (c) 2017-2018 Nuvoton
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

#ifndef HAL_SECURE_H
#define HAL_SECURE_H

#include "cmsis.h"

#ifdef __cplusplus
extern "C" {
#endif

/* pin_function (secure version)
 *
 * Guard access to secure GPIO from non-secure domain.
 * 
 * Its synopsis is the same as normal version except change of return/argument type for
 * binary-compatible across compilers.
 */
#if defined(__ARM_FEATURE_CMSE) && (__ARM_FEATURE_CMSE == 3U) && (TFM_LVL == 0)
__NONSECURE_ENTRY
#endif
void pin_function_s(int32_t pin, int32_t data);
#if (TFM_LVL > 0)
__NONSECURE_ENTRY
int32_t pin_function_veneer(uint32_t arg0, uint32_t arg1, uint32_t arg2, uint32_t arg3);
#endif

#ifdef __cplusplus
}
#endif

#endif

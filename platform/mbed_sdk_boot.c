/* mbed Microcontroller Library
 * Copyright (c) 2017 ARM Limited
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

#include "mbed_toolchain.h"
#include <stdlib.h>
#include <stdint.h>
#include "cmsis.h"

/* This startup is for mbed 2 baremetal. There is no config for RTOS for mbed 2,
 * therefore we protect this file with MBED_CONF_RTOS_PRESENT
 * Note: The new consolidated started for mbed OS is in rtos/mbed_boot code file.
 */
#if !defined(MBED_CONF_RTOS_PRESENT)

/* mbed_main is a function that is called before main()
 * mbed_sdk_init() is also a function that is called before main(), but unlike
 * mbed_main(), it is not meant for user code, but for the SDK itself to perform
 * initializations before main() is called.
 */
MBED_WEAK void mbed_main(void) 
{

}

/* This function can be implemented by the target to perform higher level target initialization
 */
MBED_WEAK void mbed_sdk_init(void) 
{

}

MBED_WEAK void software_init_hook_rtos()
{
    // Nothing by default
}

void mbed_copy_nvic(void)
{
    /* If vector address in RAM is defined, copy and switch to dynamic vectors. Exceptions for M0 which doesn't have
    VTOR register and for A9 for which CMSIS doesn't define NVIC_SetVector; in both cases target code is
    responsible for correctly handling the vectors.
    */
#if !defined(__CORTEX_M0) && !defined(__CORTEX_A9)
#ifdef NVIC_RAM_VECTOR_ADDRESS
    uint32_t *old_vectors = (uint32_t *)SCB->VTOR;
    uint32_t *vectors = (uint32_t*)NVIC_RAM_VECTOR_ADDRESS;
    for (int i = 0; i < NVIC_NUM_VECTORS; i++) {
        vectors[i] = old_vectors[i];
    }
    SCB->VTOR = (uint32_t)NVIC_RAM_VECTOR_ADDRESS;
#endif /* NVIC_RAM_VECTOR_ADDRESS */
#endif /* !defined(__CORTEX_M0) && !defined(__CORTEX_A9) */
}

/* Toolchain specific main code */

#if defined (__CC_ARM) || (defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 5010060))

int $Super$$main(void);

int $Sub$$main(void) 
{
    mbed_main();
    return $Super$$main();
}

void _platform_post_stackheap_init(void) 
{
    mbed_copy_nvic();
    mbed_sdk_init();
}

/* Fix __user_setup_stackheap and ARM_LIB_STACK/ARM_LIB_HEAP cannot co-exist in RTOS-less build
 *
 * According AN241 (http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dai0241b/index.html),
 * __rt_entry has the following call sequence:
 * 1. _platform_pre_stackheap_init
 * 2. __user_setup_stackheap or setup the Stack Pointer (SP) by another method
 * 3. _platform_post_stackheap_init
 * 4. __rt_lib_init
 * 5. _platform_post_lib_init
 * 6. main()
 * 7. exit()
 *
 * Per our check, when __user_setup_stackheap and ARM_LIB_STACK/ARM_LIB_HEAP co-exist, neither
 * does __user_setup_stackheap get called and nor is ARM_LIB_HEAP used to get heap base/limit,
 * which are required to pass to __rt_lib_init later. To fix the issue, by subclass'ing
 * __rt_lib_init, heap base/limit are replaced with Image$$ARM_LIB_HEAP$$ZI$$Base/Limit if
 * ARM_LIB_HEAP region is defined in scatter file.
 */
#include <rt_misc.h>
extern __value_in_regs struct __argc_argv $Super$$__rt_lib_init(unsigned heapbase, unsigned heaptop);
extern MBED_WEAK char Image$$ARM_LIB_HEAP$$ZI$$Base[];
extern MBED_WEAK char Image$$ARM_LIB_HEAP$$ZI$$Limit[];

__value_in_regs struct __argc_argv $Sub$$__rt_lib_init (unsigned heapbase, unsigned heaptop)
{
    if (Image$$ARM_LIB_HEAP$$ZI$$Limit) {
        return $Super$$__rt_lib_init((unsigned) Image$$ARM_LIB_HEAP$$ZI$$Base, (unsigned) Image$$ARM_LIB_HEAP$$ZI$$Limit);
    } else {
        return $Super$$__rt_lib_init(heapbase, heaptop);
    }
}

#elif defined (__GNUC__) 

extern int __real_main(void);

void software_init_hook(void)
{
    mbed_copy_nvic();
    mbed_sdk_init();
    software_init_hook_rtos();
}


int __wrap_main(void) 
{
    mbed_main();
    return __real_main();
}

#elif defined (__ICCARM__)

int __low_level_init(void)
{
  mbed_copy_nvic();
  return 1;
}

#endif

#endif

/* config.h -- Autogenerated! Do not edit. */

#ifndef __INCLUDE_CHINOS_CONFIG_H
#define __INCLUDE_CHINOS_CONFIG_H

/* General Definitions ***********************************/
/* Used to represent the values of tristate options */

#define CONFIG_y 1
#define CONFIG_m 2

/* Architecture-specific options *************************/

#define CONFIG_HOST_LINUX 1
#define CONFIG_COMPILER_TOOLCHAIN_GNU_EABIL 1
#define CONFIG_BUILD_WARNINGS 1
#define CONFIG_BUILD_NOOPT 1
#define CONFIG_DEBUG_ALERT 1
#define CONFIG_DEBUG_FEATURES 1
#define CONFIG_DEBUG_ERROR 1
#define CONFIG_DEBUG_WARN 1
#define CONFIG_DEBUG_INFO 1
#define CONFIG_DEBUG_ASSERTIONS 1
#define CONFIG_ARCH_ARM64 1
#define CONFIG_ARCH "arm64"
#define CONFIG_ARCH_64 1
#define CONFIG_PAGE_SIZE 4096
#define CONFIG_ARCH_CHIP_QEMU_ARM_VIRT 1
#define CONFIG_ARCH_ARMV8A 1
#define CONFIG_ARCH_CORTEXA53 1
#define CONFIG_ARM64_4K_PAGES 1
#define CONFIG_ARM64_PAGE_SHIFT 12
#define CONFIG_ARM64_CONT_SHIFT 4
#define CONFIG_PGTABLE_LEVELS 3
#define CONFIG_ARM64_VA_BITS_39 1
#define CONFIG_ARM64_VA_BITS 39
#define CONFIG_ARM64_PA_BITS_48 1
#define CONFIG_ARM64_PA_BITS 48
#define CONFIG_CPU_LITTLE_ENDIAN 1
#define CONFIG_ARCH_HAVE_FPU 1
#define CONFIG_ARCH_FPU 1
#define CONFIG_ARCH_CFLAGS 1
#define CONFIG_ARCH_PMU 1
#define CONFIG_ARCH_BOARD_QEMU_ARM_VIRT 1
#define CONFIG_SYSTEM_CLOCK_HZ 62500000
#define CONFIG_ARCH_BOARD "qemu-arm-virt"
#define CONFIG_TASK_NAME_SIZE 63
#define CONFIG_DEFAULT_TASK_PRIORITY 10
#define CONFIG_MAX_TASK_PRIORITY 32
#define CONFIG_RR_INTERVAL 1000
#define CONFIG_PRIORITY_INHERITANCE 1
#define CONFIG_SEM_PREALLOCHOLDERS 16
#define CONFIG_SEM_NNESTPRIO 16
#define CONFIG_DEFAULT_TASK_STACKSIZE 8192
#define CONFIG_IDLE_TASK_STACKSIZE 1024
#define CONFIG_MEMSET_OPTSPEED 1
#define CONFIG_MEMSET_64BIT 1
#define CONFIG_LIBC_LONG_LONG	1
#define CONFIG_LIBC_FLOATINGPOINT 1
#define CONFIG_HAVE_LONG_LONG 1
#define CONFIG_HAVE_FLOAT 1
#define CONFIG_HAVE_DOUBLE 1
#define CONFIG_HAVE_LONG_DOUBLE 1
//#define CONFIG_MM_GRAN 1
#define CONFIG_MM_BUDDY 1
#define CONFIG_MAX_TASKS 0x20000

#define CONFIG_KERNEL_LOAD_ADDR         0x0000000040080000

#define CONFIG_HYPERVISOR_SUPPORT 1
#ifdef CONFIG_HYPERVISOR_SUPPORT
#define CONFIG_KERNEL_EXEC_ADDR         0x0000000800080000
#define CONFIG_KERNEL_DEVICE_EXEC_ADDR  0x0000000f00000000
#else
#define CONFIG_KERNEL_EXEC_ADDR         0xffffff8000080000
#define CONFIG_KERNEL_DEVICE_EXEC_ADDR  0xffffff8f00000000
#endif

#endif /* __INCLUDE_CHINOS_CONFIG_H */

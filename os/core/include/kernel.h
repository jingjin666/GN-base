#ifndef __KERNEL_H
#define __KERNEL_H

#include <chinos/config.h>

#define BOOTONLY __attribute__((section(".boot")))
#define BOOTDATA __attribute__((section(".boot.data")))
#define BOOTPHYSIC __attribute__((section(".boot.physic")))

#endif

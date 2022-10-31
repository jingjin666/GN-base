#define BOOTPHYSIC __attribute__((section(".boot")))

void BOOTPHYSIC boot_setup_mmu(void)
{
    int i = 1;
    int j = 2;
    int k;

    k = i + j;
}


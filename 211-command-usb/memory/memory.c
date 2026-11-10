#include <stdio.h>
#include <stdint.h>
#include "hardware/regs/addressmap.h"
#include "pico/stdlib.h"
#include "memory.h"

extern char __flash_binary_start;
extern char __flash_binary_end;
extern char __boot2_start__;
extern char __boot2_end__;
extern char __etext;
extern char __data_start__;
extern char __data_end__;
extern char __bss_start__;
extern char __bss_end__;
extern char __HeapLimit;
extern char __StackBottom;
extern char __StackTop;


static void row(const char *name, uintptr_t start, uintptr_t end)
{
    printf("%-10s 0x%08x 0x%08x %8u\n",
           name, (unsigned)start, (unsigned)end,
           (unsigned)(end - start));
}

void mem_info(void)
{
    uintptr_t flash_start = XIP_BASE;
    uintptr_t flash_end = flash_start + PICO_FLASH_SIZE_BYTES;
    uintptr_t ram_start = SRAM_BASE;
    uintptr_t ram_end = ram_start + 264 * 1024;
    uintptr_t rom_start = ROM_BASE;
    uintptr_t rom_end = rom_start + 16 * 1024;

    uintptr_t image = ((uintptr_t)&__flash_binary_end) - ((uintptr_t)&__flash_binary_start);
    uintptr_t boot2 = ((uintptr_t)&__boot2_end__) - ((uintptr_t)&__boot2_start__);
    uintptr_t text = ((uintptr_t)&__etext) - ((uintptr_t)&__boot2_end__);
    uintptr_t data = ((uintptr_t)&__data_end__) - ((uintptr_t)&__data_start__);
    uintptr_t bss = ((uintptr_t)&__bss_end__) - ((uintptr_t)&__bss_start__);
    uintptr_t heap = ((uintptr_t)&__HeapLimit) - ((uintptr_t)&__bss_end__);
    uintptr_t stack = ((uintptr_t)&__StackTop) - ((uintptr_t)&__StackBottom);
    uintptr_t free_flash = flash_end - ((uintptr_t)&__flash_binary_end);

    printf("area       start      end        size\n");

    row("flash", flash_start, flash_end);
    row("sram", ram_start, ram_end);
    row("rom", rom_start, rom_end);
    row("image", ((uintptr_t)&__flash_binary_start), ((uintptr_t)&__flash_binary_end));
    row("free", ((uintptr_t)&__flash_binary_end), flash_end);
    row("boot2", ((uintptr_t)&__boot2_start__), ((uintptr_t)&__boot2_end__));
    row("text", ((uintptr_t)&__boot2_end__), ((uintptr_t)&__etext));
    row("data flash", ((uintptr_t)&__etext), ((uintptr_t)&__etext) + data);
    row("data ram", ((uintptr_t)&__data_start__), ((uintptr_t)&__data_end__));
    row("bss", ((uintptr_t)&__bss_start__), ((uintptr_t)&__bss_end__));
    row("heap", ((uintptr_t)&__bss_end__), ((uintptr_t)&__HeapLimit));
    row("stack", ((uintptr_t)&__StackBottom), ((uintptr_t)&__StackTop));

    printf("\ntotal\n");
    printf("  flash image %8u = boot2 %u + text %u + data %u\n",
           (unsigned)image, (unsigned)boot2,
           (unsigned)text, (unsigned)data);
    printf("  flash free  %8u of %u\n",
           (unsigned)free_flash, (unsigned)PICO_FLASH_SIZE_BYTES);
    printf("  ram used    %8u = data %u + bss %u\n",
           (unsigned)(data + bss), (unsigned)data, (unsigned)bss);
    printf("  ram free    %8u for heap and %u for stack\n",
           (unsigned)heap, (unsigned)stack);
}

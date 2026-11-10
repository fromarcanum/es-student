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


#include <stdlib.h>
#include "command.h"
#include "device.h"

int main(void);

uint32_t data_variable = 100;
uint32_t bss_variable;

void fw_info(void)
{
    data_variable++;
    bss_variable++;

    uint32_t stack_variable = 1946;
    uint32_t *heap_variable = malloc(sizeof(uint32_t));

    uint16_t *main_code =
        (uint16_t *)((uintptr_t)main & ~1u);
    uint16_t *fw_code =
        (uint16_t *)((uintptr_t)fw_info & ~1u);

    printf("object          address     value\n");
    printf("main            0x%08x  0x%04x\n",
           (unsigned)(uintptr_t)main, *main_code);
    printf("fw_info         0x%08x  0x%04x\n",
           (unsigned)(uintptr_t)fw_info, *fw_code);

    printf("commands        0x%08x\n",
           (unsigned)(uintptr_t)commands);

    for (uint i = 0; i < command_count; i++)
    {
        printf("- %-14s 0x%08x\n",
               commands[i].name,
               (unsigned)(uintptr_t)commands[i].handler);
    }

    printf("DEVICE_PROJECT  0x%08x  %s\n",
           (unsigned)(uintptr_t)DEVICE_PROJECT, DEVICE_PROJECT);
    printf("DEVICE_BOARD    0x%08x  %s\n",
           (unsigned)(uintptr_t)DEVICE_BOARD, DEVICE_BOARD);

    printf("data_variable   0x%08x  %u\n",
           (unsigned)(uintptr_t)&data_variable,
           (unsigned)data_variable);
    printf("bss_variable    0x%08x  %u\n",
           (unsigned)(uintptr_t)&bss_variable,
           (unsigned)bss_variable);
    printf("stack_variable  0x%08x  %u\n",
           (unsigned)(uintptr_t)&stack_variable,
           (unsigned)stack_variable);

    if (heap_variable != NULL)
    {
        *heap_variable = 1951;
        printf("heap_variable   0x%08x  %u\n",
               (unsigned)(uintptr_t)heap_variable,
               (unsigned)*heap_variable);
        free(heap_variable);
    }
    else
    {
        printf("heap_variable   allocation failed\n");
    }
}

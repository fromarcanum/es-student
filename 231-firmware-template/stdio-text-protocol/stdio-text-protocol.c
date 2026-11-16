#include "stdio-text-protocol.h"

#include <stddef.h>
#include <stdio.h>
#include "pico/stdlib.h"

static char line[STDIO_TEXT_PROTOCOL_LINE_SIZE];
static uint32_t line_length = 0;
static bool overflow = false;
static command_t command;

void stdio_text_protocol_init(void)
{
    line_length = 0;
    overflow = false;
    command = (command_t){0};
}

static const command_t *parse_line(void)
{
    line[line_length] = '\0';

    command = (command_t){0};
    command.truncated = overflow;

    char *p = line;

    while (*p != '\0')
    {
        while (*p == ' ')
        {
            *p++ = '\0';
        }

        if (*p == '\0')
            break;

        char *word = p;

        while (*p != '\0' && *p != ' ')
        {
            p++;
        }

        if (*p == ' ')
        {
            *p++ = '\0';
        }

        if (command.name == NULL)
        {
            command.name = word;
        }
        else if (command.argc < STDIO_TEXT_PROTOCOL_MAX_ARGS)
        {
            command.argv[command.argc++] = word;
        }
        else
        {
            command.truncated = true;
        }
    }

    line_length = 0;
    overflow = false;

    return command.name ? &command : NULL;
}

const command_t *stdio_text_protocol_handle(void)
{
    int symbol = getchar_timeout_us(0);

    if (symbol == PICO_ERROR_TIMEOUT)
        return NULL;

    if (symbol == '\r' || symbol == '\n')
    {
        if (line_length == 0 && !overflow)
            return NULL;

        putchar('\n');
        return parse_line();
    }

    if (symbol == 0x08 || symbol == 0x7f)
    {
        if (line_length > 0)
        {
            line_length--;
            overflow = false;
            printf("\b \b");
        }
        return NULL;
    }

    if (symbol >= 0x20 && symbol <= 0x7e)
    {
        if (line_length < STDIO_TEXT_PROTOCOL_LINE_SIZE - 1)
        {
            line[line_length++] = (char)symbol;
            putchar(symbol);
        }
        else
        {
            overflow = true;
        }
    }

    return NULL;
}

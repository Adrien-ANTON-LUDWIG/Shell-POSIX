#include "echo.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "argument_handler.h"
#include "b_utils.h"
#include "echo_utils.h"
#include "my_err.h"
#include "string_manipulation.h"

#define ESCAPE_CHAR 27
#define NB_ESCAPE_SEQ 7

#define CHAR_ESCAPE                                                            \
    {                                                                          \
        'a', 'b', 'f', 'n', 'r', 't', 'v'                                      \
    }
#define STRING_ESCAPE                                                          \
    {                                                                          \
        '\a', '\b', '\f', '\n', '\r', '\t', '\v'                               \
    }

static int get_escape_index(char c)
{
    char char_escape[] = CHAR_ESCAPE;
    if (c == 'c')
        return -1;
    if (c == '0')
        return -2;
    if (c == 'x')
        return -3;
    if (c == 'e')
        return -4;
    for (int i = 0; i < NB_ESCAPE_SEQ; i++)
    {
        if (c == char_escape[i])
            return i;
    }
    return -5;
}

static int get_ascii_conversion(char *argv, size_t *i, int index)
{
    int to_read = 0;
    int tmp = *i;
    if (index == -2)
    {
        to_read = (nb_to_read_oct(argv, tmp));
        *i += to_read;
        return str_oct_to_dec(argv + 1, tmp, to_read);
    }
    else
    {
        to_read = nb_to_read_hx(argv, tmp);
        *i += to_read;
        return str_hx_to_dec(argv + 1, tmp, to_read);
    }
    return 42;
}

static int should_interpret(char c)
{
    if (is_in(c, "bfrv"))
        return -1;
    else if (c == 'a')
        return 0;
    else if (c == 'n')
        return 3;
    else if (c == 't')
        return 5;
    else
        return -2;
}

static int strong_quotes(char *argv, size_t i, size_t len, int e)
{
    char str_escape[] = STRING_ESCAPE;
    int to_interpret = 0;

    while (i < len)
    {
        char c = argv[i];
        if (c == '\'')
            return i + 1;

        else if (e && c == '\\' && i++ < len)
        {
            c = argv[i];
            to_interpret = should_interpret(c);
            if (to_interpret != -1 && to_interpret != -2)
                printf("%c", str_escape[to_interpret]);
            else if (to_interpret == -2)
                printf("\\%c", c);
        }
        else
            printf("%c", argv[i]);
        i++;
    }
    return i;
}

static void handles_bkslash_wk(size_t *i, char c, char *argv, size_t len)
{
    char str_escape[] = STRING_ESCAPE;
    if (*i < len)
        c = argv[*i];
    int escape = get_escape_index(c);
    if (escape == -2 || escape == -3)
        printf("%c", get_ascii_conversion(argv, i, escape));
    else if (escape == -4)
        printf("%c", ESCAPE_CHAR);
    else if (escape == -5 && argv[*i] != '\\' && argv[*i] != '\"')
        printf("\\%c", c);
    else if (escape == -5)
        printf("%c", c);
    else
        printf("%c", str_escape[escape]);
}

static int weak_quotes(char *argv, size_t i, size_t len, int e)
{
    char c;
    while (i < len)
    {
        c = argv[i];
        if (c == '\"')
            return i + 1;
        else if (!e && c == '\\' && i + 1 < len && argv[i + 1] != '\\'
                 && argv[i + 1] != '\"')
        {
            printf("\\%c", argv[i + 1]);
            i += 2;
        }
        else if (c == '\\' && ++i)
        {
            handles_bkslash_wk(&i, c, argv, len);
            i++;
        }
        else
        {
            printf("%c", c);
            i++;
        }
    }
    return i;
}

static void handles_bkslash(char *argv, size_t i, char c, int *n)
{
    char str_escape[] = STRING_ESCAPE;
    int escape = get_escape_index(c);
    if (escape == -1 && (*n = 1))
        return;
    else if (escape == -2 || escape == -3)
        printf("%c", get_ascii_conversion(argv, &i, escape));
    else if (escape == -4)
        printf("%c", ESCAPE_CHAR);
    else if (escape == -5)
        printf("\\%c", c);
    else
        printf("%c", str_escape[escape]);
    i++;
}

static void echo_display(char *argv, int e, int *n)
{
    size_t len = strlen(argv);
    size_t i = 0;
    while (i < len)
    {
        char c = argv[i];
        if (c == '\'')
            i = strong_quotes(argv, i + 1, len, e);
        else if (c == '\"')
            i = weak_quotes(argv, i + 1, len, e);
        else if (c == '\\' && i + 1 < len && (c = argv[++i]) && e && c == '\\'
                 && i++)
        {
            if (i < len)
                c = argv[i];
            if (c == '\\')
                continue;
            handles_bkslash(argv, i, c, n);
        }
        else
        {
            printf("%c", c);
            i++;
        }
    }
}

int b_echo(char **argv)
{
    struct stat statbuff;
    if (fstat(STDOUT_FILENO, &statbuff) == -1)
        return 1;
    int argc = argv_len(argv);

    if (argc < 2)
    {
        printf("\n");
        return 0;
    }

    int n = 0;
    int e = 0;
    int E = 0;

    int nb_opt = set_options(argv, &n, &e, &E);
    char *str = merge_arguments(argc - nb_opt - 1, argv + nb_opt + 1);

    echo_display(str, e, &n);
    free(str);
    if (!n)
        printf("\n");

    fflush(stdout);
    return 0;
}
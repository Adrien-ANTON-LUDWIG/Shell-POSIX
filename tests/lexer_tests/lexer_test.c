#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "argument_handler.h"
#include "ast.h"
#include "custom_descriptor.h"
#include "execution.h"
#include "lexer.h"
#include "my_xmalloc.h"
#include "printer.h"

/**
 * @brief Does nothing... yet
 *
 * @param i
 * @param argv
 */
void shopt_options(int *i, char **argv)
{
    if (!argv[*i])
        return;
    while (!strcmp(argv[*i], "-O") || !strcmp(argv[*i], "+O"))
    {
        if (argv[*i][0] == '-')
            *i += 1;
        else
            *i += 1;
        *i += 1;
    }
}

static int run_command_line(int argc, char *argv[], int from, struct major *mj)
{
    char *args = merge_arguments(argc - from, argv + from);
    mj->file = createfrom_string(mj, args);
    mj->rvalue = 0;
    free(args);

    struct token *tk = NULL;
    do
    {
        token_free(tk);
        tk = get_next_token(mj);
        print_token(tk);
    } while (tk->word != WORD_EOF);

    token_free(tk);
    major_free(mj);
    return 0;
}

/**
 * @brief It's the main function :)
 *
 * @param argc
 * @param argv
 * @return int
 */
int main(int argc, char **argv)
{
    int i = 0;

    if (argc > 1)
        i = 1;

    struct major *mj = major_init();
    int from = get_index_command_string(i, argc, argv);

    if (strcmp(argv[i], "-c") == 0)
    {
        if (i + 1 == argc)
            errx(2, "-c: option requires an argument");

        return run_command_line(argc, argv, from, mj);
    }
    else
    {
        struct custom_FILE *file;
        if (argc >= 2)
            file = custom_fopen(mj, argv[from]);
        else
        {
            file = my_xcalloc(mj, 1, sizeof(struct custom_FILE));
            file->file = stdin;
            file->fd = stdin->_fileno;
        }
        mj->file = file;

        struct token *tk = NULL;
        do
        {
            token_free(tk);
            tk = get_next_token(mj);
            print_token(tk);
        } while (tk->word != WORD_EOF);
        token_free(tk);
    }

    major_free(mj);
    return 0;
}
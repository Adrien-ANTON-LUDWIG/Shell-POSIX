#define _POSIX_C_SOURCE 200809L

#include "lexer.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "custom_descriptor.h"
#include "list.h"
#include "my_err.h"
#include "my_xmalloc.h"
#include "string_manipulation.h"
#include "tokens.h"

static struct token *get_token_redir(struct major *mj, char c)
{
    char *tokens_strings_redir[] = TOKENS_STRINGS_REDIR;
    char default_value[] = DEFAULT_REDIR_VALUE;
    char s[] = { c, '\0', '\0', '\0' };
    char n = '\0';
    struct token *tk = token_init(mj, WORD_REDIR_LLM);

    if (mj->file->lexer_index - 1 > 0)
        n = mj->file->str[mj->file->lexer_index - 2];

    c = get_char(mj->file, 0);
    if (is_in(c, IS_REDIR))
    {
        s[1] = get_char(mj->file, 1);

        c = get_char(mj->file, 0);
        if (s[1] == '<' && c == '-')
            s[2] = get_char(mj->file, 1);
    }

    for (size_t i = 0; i < NUMBER_OF_REDIR; i++)
    {
        if (!strcmp(s, tokens_strings_redir[i]))
        {
            if (!isdigit(n))
                n = default_value[i];
            char *number = strndup(&n, 1);
            tk->data = list_append(mj, tk->data, number);
            tk->word = i + WORD_REDIR_LR;
            return tk;
        }
    }

    return tk;
}

static struct token *get_token_and(struct major *mj)
{
    struct custom_FILE *f = mj->file;

    if (get_char(f, 0) == '&')
    {
        f->lexer_index++;
        return token_init(mj, WORD_AND);
    }

    return token_init(mj, WORD_SEMIC);
}

static struct token *get_token_pipe(struct major *mj)
{
    struct custom_FILE *f = mj->file;

    if (get_char(f, 0) == '|')
    {
        f->lexer_index++;
        return token_init(mj, WORD_OR);
    }

    return token_init(mj, WORD_PIPE);
}

static struct token *get_token_parenthesis(struct major *mj)
{
    struct custom_FILE *f = mj->file;
    skip(f, is_in, MY_IS_SPACE);

    if (get_char(f, 0) == ')')
    {
        f->lexer_index++;
        return token_init(mj, WORD_DPARENTHESIS);
    }

    return token_init(mj, WORD_LPARENTHESIS);
}

static struct token *get_token_operator(struct major *mj)
{
    struct custom_FILE *f = mj->file;
    char c = f->str[f->lexer_index++];

    if (c == ';')
    {
        if (get_char(f, 0) == ';')
        {
            f->lexer_index++;
            return token_init(mj, WORD_DSEMIC);
        }
        return token_init(mj, WORD_SEMIC);
    }
    if (c == '\n')
        return token_init(mj, WORD_NEWLINE);
    if (c == '&')
        return get_token_and(mj);
    if (c == '|')
        return get_token_pipe(mj);
    if (c == '(')
        return get_token_parenthesis(mj);
    if (c == ')')
        return token_init(mj, WORD_RPARENTHESIS);
    return get_token_redir(mj, c);
}

static struct token *get_token_operator_skip_newline(struct major *mj)
{
    struct token *tk = get_token_operator(mj);

    if (at_end(mj->file))
        custom_getline(mj);

    struct custom_FILE *f = mj->file;
    while (!at_end(f)
           && (!(is_in(get_char(f, 0), IS_OPERATOR)
                 || is_not_in(get_char(f, 0), IS_NOT_WORD))
               || get_char(f, 0) == '#' || get_char(mj->file, 0) == '\n'))
    {
        skip(f, is_in, MY_IS_SPACE);

        if (!at_end(f) && get_char(f, 0) == '#')
            skip(f, is_not_in, IS_NEWLINE);

        if (get_char(f, 0) == '\n')
            f->lexer_index++;

        if (at_end(f))
            custom_getline(mj);
    }

    return tk;
}

static void skip_quote(struct major *mj, char c)
{
    struct custom_FILE *f = mj->file;

    if (f->lexer_index > 0 && f->str[f->lexer_index - 2] == '\\')
        return;

    if (at_end(f))
        my_err(2, mj, "get_token_quote: unexpected EOF");

    char quote = c;
    char lastchar = c;
    c = get_char(f, 1);

    while (c != quote || (quote == '"' && lastchar == '\\'))
    {
        lastchar = c;
        c = get_char(f, 1);

        if (at_end(f) && c != quote)
            if (!custom_getline_same_buf(mj))
                my_err(2, mj, "get_token_quote: unexpected EOF");
    }
}

// /!\ 25 lignes
static struct token *get_token_word(struct major *mj)
{
    struct custom_FILE *f = mj->file;
    size_t start = f->lexer_index;
    char c = get_char(f, 0);

    while (!at_end(f) && is_not_in(c, IS_NOT_WORD))
    {
        if (is_in(c, "'\""))
        {
            f->lexer_index++;
            skip_quote(mj, c);
        }
        else
            skip(f, is_not_in, IS_NOT_WORD_OR_QUOTE);
        c = get_char(f, 0);
    }

    size_t end = f->lexer_index;
    size_t len = end - start;

    if (len == 1 && '0' <= f->str[start] && f->str[start] <= '9'
        && (f->str[end] == '>' || f->str[end] == '<'))
    {
        f->lexer_index++;
        return get_token_redir(mj, f->str[end]);
    }

    struct token *tk = token_init(mj, WORD_WORD);
    char *s = strndup(f->str + start, len);
    tk->data = list_append(mj, tk->data, s);
    char *tokens_strings[] = TOKENS_STRINGS;

    for (size_t i = 0; i < WORD_AND; i++)
        if (!strcmp(s, tokens_strings[i]))
        {
            tk->word = i;
            return tk;
        }

    return tk;
}

static void set_if_assignment(struct token *tk)
{
    char *s = tk->data->head->data;

    if ('0' <= *s && *s <= '9')
        return;

    char *equal = strstr(s, "=");

    if (!equal || s == equal)
        return;

    for (; s != equal; s++)
        if (!(isalnum(*s) || *s == '_'))
            return;

    tk->word = WORD_ASSIGNMENT;
}

struct token *get_next_token(struct major *mj)
{
    struct custom_FILE *f = mj->file;
    if (!f->buffer_size || at_end(f))
        if (!custom_getline(mj))
            return token_init(mj, WORD_EOF);

    while (!at_end(f)
           && (!(is_in(get_char(f, 0), IS_OPERATOR)
                 || is_not_in(get_char(f, 0), IS_NOT_WORD))
               || get_char(f, 0) == '#'))
    {
        skip(f, is_in, MY_IS_SPACE);

        if (!at_end(f) && get_char(f, 0) == '#')
            skip(f, is_not_in, IS_NEWLINE);

        if (at_end(f))
            if (!(custom_getline(mj)))
                return token_init(mj, WORD_EOF);
    }

    if (is_in(get_char(f, 0), IS_OPERATOR))
        return get_token_operator_skip_newline(mj);

    struct token *word = get_token_word(mj);
    set_if_assignment(word);

    return word;
}
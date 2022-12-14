#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "list.h"
#include "my_err.h"
#include "my_xmalloc.h"
#include "parser.h"
#include "tokens.h"

/**
 * @brief Predicate used by parser_function() to know if this is still a
 * compound list.
 *
 * @param w
 * @return int
 */
static int should_loop_bracket(enum words w)
{
    return w != WORD_RBRACKET;
}

static int should_loop_parenthesis(enum words w)
{
    return w != WORD_RPARENTHESIS;
}

static void make_function_ast(struct major *mj, struct token **tk,
                              struct ast *newast)
{
    if ((*tk)->word == WORD_LBRACKET && (*tk = token_renew(mj, *tk, 1)))
        parser_cpdlist(mj, tk, newast, should_loop_bracket);
    else if ((*tk)->word == WORD_LPARENTHESIS
             && (*tk = token_renew(mj, *tk, 1)))
        parser_cpdlist(mj, tk, newast, should_loop_parenthesis);
    else
        newast->right = take_action(mj, newast->right, tk);
}

/**
 * @brief Handles the parsing of an "function" token
 *
 * @param mj major structure
 * @param ast
 * @param tk
 * @return struct ast*
 */
struct ast *parser_function(struct major *mj, struct ast *ast,
                            struct token **tk, struct token *tk2)
{
    superand_creator(mj, &ast);

    struct token *func_token = *tk;

    if (!((*tk)->word == WORD_FUNCTION))
    {
        if ((*tk)->word < WORD_LBRACKET || (*tk)->word == WORD_WORD)
        {
            (*tk)->word = WORD_FUNCTION;
            *tk = tk2;
        }
        else
            my_err(2, mj, "parser_function: Bad function declaration");
    }
    else
    {
        if (!(tk2->word < WORD_LBRACKET || tk2->word == WORD_WORD))
            my_err(2, mj, "parser_function: Bad function declaration");
        list_free(func_token->data);
        func_token->data = tk2->data;
        tk2->data = NULL;
        token_free(tk2);
        *tk = get_next_token(mj);

        if ((*tk)->word != WORD_DPARENTHESIS)
            my_err(2, mj, "parser_function : need '()' after 'function'");
    }
    *tk = token_renew(mj, *tk, 1);
    struct ast *newast = create_ast(mj, func_token);
    make_function_ast(mj, tk, newast);

    *tk = token_renew(mj, *tk, 0);
    return newast;
}
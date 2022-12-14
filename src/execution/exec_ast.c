#define _POSIX_C_SOURCE 200809L

#include "exec_ast.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "char_array_op.h"
#include "command_execution.h"
#include "exec_case.h"
#include "list.h"
#include "pipe.h"
#include "redir.h"
#include "variable_assignment_exec.h"
#include "variable_declaration.h"
#include "variables_substitution.h"

static int redir_execution(struct major *mj, struct ast *ast, struct token *tk)
{
    if (tk->word == WORD_PIPE)
    {
        mj->rvalue = exec_pipe(mj, ast);
        return mj->rvalue;
    }
    else if (WORD_PIPE < tk->word && tk->word < WORD_COMMAND)
    {
        mj->rvalue = exec_redir(mj, ast);
        return mj->rvalue;
    }
    else
        return -1;
}

static int exec_while(struct major *mj, struct ast *ast, struct token *tk,
                      int err)
{
    mj->loop_counter++;

    while ((tk->word == WORD_UNTIL) ^ (!err))
    {
        exec_ast(mj, ast->right);

        if (mj->break_counter)
        {
            mj->break_counter--;
            mj->loop_counter--;
            return mj->rvalue;
        }

        if (mj->continue_counter)
        {
            mj->continue_counter--;
            if (mj->continue_counter)
                break;
        }

        err = exec_ast(mj, ast->left);

        if (mj->break_counter)
        {
            mj->break_counter--;
            mj->loop_counter--;
            return mj->rvalue;
        }

        if (mj->continue_counter)
        {
            mj->continue_counter--;
            if (mj->continue_counter)
                break;
        }
    }
    mj->rvalue = 0;
    mj->loop_counter--;

    return 0;
}

static int conditional_execution(struct major *mj, struct ast *ast,
                                 struct token *tk, int err)
{
    if (tk->word == WORD_IF)
    {
        if (!err)
            exec_ast(mj, ast->right);
        else if (ast->middle)
            exec_ast(mj, ast->middle);
        mj->rvalue = 0;
        return 0;
    }
    else if (tk->word == WORD_WHILE || tk->word == WORD_UNTIL)
        return exec_while(mj, ast, tk, err);
    else
        return 1;
}

int exec_ast(struct major *mj, struct ast *ast)
{
    if (!ast || mj->break_counter != 0 || mj->continue_counter != 0)
        return 0;
    int err = 0;
    struct token *tk = ast->data;
    if (redir_execution(mj, ast, tk) == -1)
        err = exec_ast(mj, ast->left);
    else
        return mj->rvalue;

    if (!conditional_execution(mj, ast, tk, err))
        return 0;
    else if (tk->word == WORD_COMMAND)
        return execution_command(mj, tk);
    else if (tk->word == WORD_AND)
        return !(!err && (!exec_ast(mj, ast->right)));
    else if (tk->word == WORD_OR)
        return !(!err || (!exec_ast(mj, ast->right)));
    else if (tk->word == WORD_SUPERAND)
        return exec_ast(mj, ast->right);
    else if (tk->word == WORD_FOR)
        return exec_for(mj, ast);
    else if (tk->word == WORD_FUNCTION)
        return add_to_funclist(mj, ast);
    else if (tk->word == WORD_ASSIGNMENT)
        return assign_variable(mj, ast);
    else if (tk->word == WORD_CASE)
        return exec_case(mj, ast);
    return err;
}

int exec_for(struct major *mj, struct ast *ast)
{
    mj->loop_counter++;
    int rvalue = 0;
    if (!ast->middle)
        return 0;

    char **list = variables_substitution(mj, ast->middle->data->data);

    for (int i = 0; list[i]; i++)
    {
        char *var_name = ast->left->data->data->head->data;
        char *var_value = list[i];
        variable_declare(mj, var_name, var_value);
        rvalue = exec_ast(mj, ast->right);

        if (mj->continue_counter)
        {
            mj->continue_counter--;
            if (mj->continue_counter)
                break;
        }

        if (mj->break_counter)
        {
            mj->break_counter--;
            mj->loop_counter--;
            char_array_free(list);
            return mj->rvalue;
        }
    }
    char_array_free(list);
    mj->rvalue = 0;
    mj->loop_counter--;
    return rvalue;
}
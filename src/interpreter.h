#pragma once

#include "lexer.h"
#include "program.h"
#include "vars.h"

typedef struct interpreter_t interpreter_t;

interpreter_t *interp_init(pool_t *pool, program_t *program, vars_t *vars);
void		   interp_run(interpreter_t *interp);
void		   interp_eval_line(interpreter_t *interp, token_t *tokens);
void		   interp_free(interpreter_t *interp);
#pragma once

#include "interpreter.h"
#include "pool.h"
#include "program.h"
#include "vars.h"

typedef struct repl_t repl_t;

repl_t *repl_init(pool_t *pool, program_t *program, vars_t *vars, interpreter_t *interp);
void	repl_run(const repl_t *repl);
void	repl_free(repl_t *repl);
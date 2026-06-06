#pragma once

#include "lexer.h"
#include "pool.h"

typedef struct program_t program_t;
typedef struct line_t line_t;

program_t *program_init(pool_t *pool);
void	   program_insert(program_t *program, int line_number, const token_t *tokens, size_t token_count);
void	   program_delete(program_t *program, int line_number);
void	   program_clear(program_t *program);
void	   program_print(const program_t *program);
void	   program_free(program_t *program);

line_t	  *program_head(const program_t *program);
line_t	  *program_find(const program_t *program, int line_number);
line_t	  *program_next(const line_t *line);
token_t	  *line_tokens(const line_t *line);
int		   line_number(const line_t *line);
size_t	   line_token_count(const line_t *line);
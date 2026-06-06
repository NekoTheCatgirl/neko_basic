#include "program.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct program_t {
	line_t *head;
	pool_t *pool;
};

struct line_t {
	int		 number;
	token_t	*tokens;
	size_t	 token_count;
	line_t	*next;
};

program_t *program_init(pool_t *pool) {
	program_t *program = malloc(sizeof(program_t));
	if (program == nullptr) return nullptr;

	program->head = nullptr;
	program->pool = pool;
	return program;
}

void program_insert(program_t *program, const int line_number, const token_t *tokens, const size_t token_count) {
	line_t *line = pool_alloc(program->pool, sizeof(line_t));
	if (line == nullptr) return;

	token_t *stored = pool_alloc(program->pool, sizeof(token_t) * token_count);
	if (stored == nullptr) return;
	memcpy(stored, tokens, sizeof(token_t) * token_count);

	line->number = line_number;
	line->tokens = stored;
	line->token_count = token_count;
	line->next = nullptr;

	line_t **cur = &program->head;
	while (*cur != nullptr && (*cur)->number < line_number) {
		cur = &(*cur)->next;
	}

	if (*cur != nullptr && (*cur)->number == line_number) {
		line->next = (*cur)->next;
	} else {
		line->next = *cur;
	}

	*cur = line;
}

void program_delete(program_t *program, const int line_number) {
	line_t **cur = &program->head;
	while (*cur != nullptr) {
		if ((*cur)->number == line_number) {
			*cur = (*cur)->next;
			return;
		}
		cur = &(*cur)->next;
	}
}

void program_clear(program_t *program) {
	program->head = nullptr;
	pool_reset(program->pool);
}

void program_print(const program_t *program) {
	const line_t *cur = program->head;
	while (cur != nullptr) {
		printf("%d ", cur->number);
		for (size_t i = 0; i < cur->token_count; i++) {
			if (i > 0) {
				const token_type_t prev = cur->tokens[i - 1].type;
				const token_type_t curr = cur->tokens[i].type;
				// no space before ) or , or after (
				if (curr != TOKEN_RPAREN && curr != TOKEN_COMMA &&
					prev != TOKEN_LPAREN) {
					printf(" ");
					}
			}
			if (cur->tokens[i].type == TOKEN_STRING) {
				printf("\"%s\"", cur->tokens[i].value);
			} else {
				printf("%s", cur->tokens[i].value);
			}
		}
		printf("\n");
		cur = cur->next;
	}
}

void program_free(program_t *program) {
	pool_reset(program->pool);
	free(program);
}

line_t *program_head(const program_t *program) {
	return program->head;
}

line_t *program_find(const program_t *program, int line_number) {
	line_t *cur = program->head;
	while (cur != nullptr) {
		if (cur->number == line_number) return cur;
		cur = cur->next;
	}
	return nullptr;
}

line_t *program_next(const line_t *line) {
	return line->next;
}

token_t *line_tokens(const line_t *line) {
	return line->tokens;
}

int line_number(const line_t *line) {
	return line->number;
}

size_t line_token_count(const line_t *line) {
	return line->token_count;
}
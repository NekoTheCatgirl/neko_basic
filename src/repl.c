#include "repl.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "signals.h"

struct repl_t {
	pool_t		  *pool;
	program_t	  *program;
	vars_t		  *vars;
	interpreter_t *interp;
};

static void repl_save(const repl_t *repl, const char *filename);
static void repl_load(const repl_t *repl, const char *filename);
static void repl_handle_line(const repl_t *repl, const char *line);

repl_t *repl_init(pool_t *pool, program_t *program, vars_t *vars, interpreter_t *interp) {
	repl_t *repl = malloc(sizeof(repl_t));
	if (repl == nullptr) return nullptr;

	repl->pool	= pool;
	repl->program = program;
	repl->vars	= vars;
	repl->interp  = interp;
	return repl;
}

void repl_run(const repl_t *repl) {
	signals_init();
	char buf[1024];

	printf("READY.\n");

	while (true) {
		signals_reset();
		printf("> ");
		fflush(stdout);

		if (fgets(buf, sizeof(buf), stdin) == nullptr) {
			if (errno == EINTR) {
				printf("\nBREAK\n");
				clearerr(stdin);
				continue;
			}
			break;
		}

		buf[strcspn(buf, "\n")] = '\0';

		if (strcmp(buf, "EXIT") == 0 || strcmp(buf, "QUIT") == 0) {
			printf("\nGOODBYE\n");
			break;
		}

		repl_handle_line(repl, buf);
	}
}

void repl_free(repl_t *repl) {
	free(repl);
}

static void repl_handle_line(const repl_t *repl, const char *line) {
	token_t *tokens = tokenize(line);
	if (tokens == nullptr) return;

	if (tokens[0].type == TOKEN_NUMBER) {
		const int line_number = (int)strtol(tokens[0].value, nullptr, 10);

		size_t count = 0;
		while (tokens[count + 1].type != TOKEN_EOF) count++;
		count++;

		if (tokens[1].type == TOKEN_EOF || tokens[1].type == TOKEN_NEWLINE) {
			program_delete(repl->program, line_number);
		} else {
			program_insert(repl->program, line_number, &tokens[1], count);
		}
	} else if (tokens[0].type == TOKEN_RUN) {
		interp_run(repl->interp);
	} else if (tokens[0].type == TOKEN_LIST) {
		program_print(repl->program);
	} else if (tokens[0].type == TOKEN_NEW) {
		program_clear(repl->program);
		printf("OK\n");
	} else if (tokens[0].type == TOKEN_SAVE) {
		if (tokens[1].type != TOKEN_STRING) {
			printf("?EXPECTED FILENAME\n");
		} else {
			repl_save(repl, tokens[1].value);
		}
	} else if (tokens[0].type == TOKEN_LOAD) {
		if (tokens[1].type != TOKEN_STRING) {
			printf("?EXPECTED FILENAME\n");
		} else {
			repl_load(repl, tokens[1].value);
		}
	} else if (tokens[0].type == TOKEN_EOF || tokens[0].type == TOKEN_NEWLINE) {
		// empty line, do nothing
	} else {
		interp_eval_line(repl->interp, tokens);
	}

	tokens_free(tokens);
}

static void repl_save(const repl_t *repl, const char *filename) {
	FILE *f = fopen(filename, "w");
	if (f == nullptr) {
		printf("?CANNOT OPEN FILE '%s'\n", filename);
		return;
	}

	// walk program lines and write them out
	line_t *cur = program_head(repl->program);
	while (cur != nullptr) {
		fprintf(f, "%d ", line_number(cur));
		token_t *toks = line_tokens(cur);
		size_t   count = line_token_count(cur);
		for (size_t i = 0; i < count; i++) {
			if (toks[i].type == TOKEN_EOF) break;
			if (i > 0) {
				token_type_t prev = toks[i - 1].type;
				token_type_t curr = toks[i].type;
				if (curr != TOKEN_RPAREN && curr != TOKEN_COMMA &&
					prev != TOKEN_LPAREN) {
					fprintf(f, " ");
					}
			}
			if (toks[i].type == TOKEN_STRING) {
				fprintf(f, "\"%s\"", toks[i].value);
			} else {
				fprintf(f, "%s", toks[i].value);
			}
		}
		fprintf(f, "\n");
		cur = program_next(cur);
	}

	fclose(f);
	printf("OK\n");
}

static void repl_load(const repl_t *repl, const char *filename) {
	FILE *f = fopen(filename, "r");
	if (f == nullptr) {
		printf("?CANNOT OPEN FILE '%s'\n", filename);
		return;
	}

	// clear current program first
	program_clear(repl->program);

	char buf[1024];
	while (fgets(buf, sizeof(buf), f) != nullptr) {
		buf[strcspn(buf, "\n")] = '\0';
		if (buf[0] == '\0') continue;
		repl_handle_line(repl, buf);
	}

	fclose(f);
	printf("OK\n");
}

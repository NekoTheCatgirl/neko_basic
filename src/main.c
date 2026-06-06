#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "interpreter.h"
#include "pool.h"
#include "program.h"
#include "repl.h"
#include "vars.h"

static size_t parse_memory_arg(int argc, const char **argv);

int main(const int argc, const char **argv) {
	srand((unsigned int)time(nullptr));
	const size_t memory = parse_memory_arg(argc, argv);

	pool_t *pool = pool_init(memory);
	if (pool == nullptr) {
		fprintf(stderr, "error: failed to allocate memory pool\n");
		exit(1);
	}

	program_t *program = program_init(pool);
	if (program == nullptr) {
		fprintf(stderr, "error: failed to initialise program storage\n");
		pool_free(pool);
		exit(1);
	}

	vars_t *vars = vars_init(pool);
	if (vars == nullptr) {
		fprintf(stderr, "error: failed to initialise variable storage\n");
		program_free(program);
		pool_free(pool);
		exit(1);
	}

	interpreter_t *interp = interp_init(pool, program, vars);
	if (interp == nullptr) {
		fprintf(stderr, "error: failed to initialise interpreter\n");
		vars_free(vars);
		program_free(program);
		pool_free(pool);
		exit(1);
	}

	repl_t *repl = repl_init(pool, program, vars, interp);
	if (repl == nullptr) {
		fprintf(stderr, "error: failed to initialise REPL\n");
		interp_free(interp);
		vars_free(vars);
		program_free(program);
		pool_free(pool);
		exit(1);
	}

	printf("Neko BASIC\n");
	printf("%zu Bytes Free\n\n", memory);

	repl_run(repl);

	repl_free(repl);
	interp_free(interp);
	vars_free(vars);
	program_free(program);
	pool_free(pool);

	exit(0);
}

static size_t parse_memory_arg(const int argc, const char **argv) {
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-m") == 0 || strcmp(argv[i], "--mem") == 0) {
			if (i + 1 < argc) {
				char *end;
				const long mb = strtol(argv[i + 1], &end, 10);
				if (*end != '\0' || mb <= 0) {
					fprintf(stderr, "error: invalid memory size '%s'\n", argv[i + 1]);
					exit(1);
				}
				return (size_t)mb * 1024 * 1024;
			} else {
				fprintf(stderr, "error: -m requires a value\n");
				exit(1);
			}
		}
	}

	// Not specified, prompt the user
	char buf[32];
	printf("Memory size (MiB)? ");
	auto _ = fgets(buf, sizeof(buf), stdin);

	char *end;
	const long mb = strtol(buf, &end, 10);
	if (*end != '\n' && *end != '\0') {
		fprintf(stderr, "error: invalid memory size\n");
		exit(1);
	}
	if (mb <= 0) {
		fprintf(stderr, "error: memory size must be positive\n");
		exit(1);
	}
	return (size_t)mb * 1024 * 1024;
}

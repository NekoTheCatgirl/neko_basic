#include "interpreter.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef NEKO_GRAPHICS

#include <raylib.h>

#endif

#include "signals.h"

#define MATH_FUNC(fn) \
    advance(interp); \
    if (!expect(interp, TOKEN_LPAREN)) return make_num(0); \
    value_t arg = parse_expression(interp); \
    if (interp->error) return make_num(0); \
    if (!expect(interp, TOKEN_RPAREN)) return make_num(0); \
    return make_num(fn(arg.value_num));

typedef enum {
    VALUE_NUMBER,
    VALUE_STRING,
} value_type_t;

typedef struct value_t {
	value_type_t type;
	union {
		double value_num;
		char   value_str[256];
	};
} value_t;

typedef struct for_frame_t {
	char    var[64];
	double  limit;
	double  step;
	line_t *line;
} for_frame_t;

typedef struct gosub_frame_t {
	line_t *line;
} gosub_frame_t;

typedef struct while_frame_t {
	line_t *line;
} while_frame_t;

struct interpreter_t {
	token_t        *tokens;
	size_t          pos;
	program_t      *program;
	vars_t         *vars;
	bool            error;
	bool            running;
	int             goto_target;
	char            error_msg[256];
	int             current_line;
	for_frame_t     for_stack[32];
	size_t          for_top;
	gosub_frame_t   gosub_stack[32];
	size_t          gosub_top;
	while_frame_t   while_stack[32];
	size_t          while_top;
#ifdef NEKO_GRAPHICS
    char*           window_title;
    bool            window_open;
    Color           current_color;
#endif
};

/* Forward Declarations */

#ifdef NEKO_GRAPHICS
static bool interp_is_window_open(interpreter_t *interp) { return interp->window_open; }
static void interp_set_window_open(interpreter_t *interp, bool open) { interp->window_open = open; }
static Color interp_get_color(interpreter_t *interp) { return interp->current_color; }
static void interp_set_color_val(interpreter_t *interp, Color c) { interp->current_color = c; }
static void exec_screen(interpreter_t *interp);
static void exec_close(interpreter_t *interp);
static void exec_color(interpreter_t *interp);
static void exec_clear(interpreter_t *interp);
static void exec_flip(interpreter_t *interp);
static void exec_circle(interpreter_t *interp, bool filled);
static void exec_line_gfx(interpreter_t *interp);
static void exec_rect(interpreter_t *interp, bool filled);
static void exec_point(interpreter_t *interp);
static void exec_text(interpreter_t *interp);
#endif

static void interp_error(interpreter_t *interp, const char *msg);
static token_t *peek(interpreter_t *interp);
static token_t *advance(interpreter_t *interp);
static bool check(interpreter_t *interp, token_type_t type);
static bool expect(interpreter_t *interp, token_type_t type);
static value_t make_num(double n);
static value_t make_str(const char *s);
static value_t parse_expression(interpreter_t *interp);
static value_t parse_primary(interpreter_t *interp);
static value_t parse_power(interpreter_t *interp);
static value_t parse_unary(interpreter_t *interp);
static value_t parse_mul(interpreter_t *interp);
static value_t parse_add(interpreter_t *interp);
static value_t parse_comparison(interpreter_t *interp);
static value_t parse_and(interpreter_t *interp);
static void exec_print(interpreter_t *interp);
static void exec_let(interpreter_t *interp);
static void exec_if(interpreter_t *interp);
static void exec_goto(interpreter_t *interp);
static void exec_end(interpreter_t *interp);
static void exec_input(interpreter_t *interp);
static void exec_for(interpreter_t *interp);
static void exec_next(interpreter_t *interp);
static void exec_gosub(interpreter_t *interp);
static void exec_return(interpreter_t *interp);
static void exec_stop(interpreter_t *interp);
static void exec_while(interpreter_t *interp);
static void exec_wend(interpreter_t *interp);
static void exec_dim(interpreter_t *interp);

/* Implementation */

interpreter_t *interp_init(pool_t *pool, program_t *program, vars_t *vars) {
    (void)pool;
	interpreter_t *interp = malloc(sizeof(interpreter_t));
	if (interp == nullptr) return nullptr;

	interp->tokens       = nullptr;
	interp->pos          = 0;
	interp->program      = program;
	interp->vars         = vars;
	interp->error        = false;
    interp->running      = false;
    interp->goto_target  = -1;
	interp->error_msg[0] = '\0';
    interp->current_line = 0;
    interp->for_top      = 0;
    interp->gosub_top    = 0;
    interp->while_top    = 0;

#ifdef NEKO_GRAPHICS
    interp->window_open = false;
    interp->current_color = WHITE;
#endif

	return interp;
}

void interp_run(interpreter_t *interp) {
    interp->running     = true;
    interp->goto_target = -1;
    interp->error       = false;

    line_t *cur = program_head(interp->program);

    while (interp->running && cur != nullptr && !interrupted) {
        interp->current_line = line_number(cur);
        interp->goto_target  = -1;

        interp_eval_line(interp, line_tokens(cur));

        if (interp->error) {
            printf("%s\n", interp->error_msg);
            break;
        }

        if (interp->goto_target != -1) {
            cur = program_find(interp->program, interp->goto_target);
            if (cur == nullptr) {
                printf("?UNDEFINED LINE NUMBER %d\n", interp->goto_target);
                break;
            }
        } else {
            cur = program_next(cur);
        }
    }

    if (interrupted) {
        printf("\nBREAK IN LINE %d\n", interp->current_line);
    }

    interp->running = false;
}

void interp_eval_line(interpreter_t *interp, token_t *tokens) {
    interp->tokens = tokens;
    interp->pos    = 0;
    interp->error  = false;

    while (!interp->error && !check(interp, TOKEN_EOF) && !check(interp, TOKEN_NEWLINE)) {
        switch (peek(interp)->type) {
            case TOKEN_PRINT:  advance(interp); exec_print(interp);  break;
            case TOKEN_LET:    advance(interp); exec_let(interp);    break;
            case TOKEN_IF:     advance(interp); exec_if(interp);     break;
            case TOKEN_GOTO:   advance(interp); exec_goto(interp);   break;
            case TOKEN_END:    advance(interp); exec_end(interp);    break;
            case TOKEN_INPUT:  advance(interp); exec_input(interp);  break;
            case TOKEN_FOR:    advance(interp); exec_for(interp);    break;
            case TOKEN_NEXT:   advance(interp); exec_next(interp);   break;
            case TOKEN_GOSUB:  advance(interp); exec_gosub(interp);  break;
            case TOKEN_RETURN: advance(interp); exec_return(interp); break;
            case TOKEN_STOP:   advance(interp); exec_stop(interp);   break;
            case TOKEN_WHILE: advance(interp); exec_while(interp); break;
            case TOKEN_WEND:  advance(interp); exec_wend(interp);  break;
            case TOKEN_DIM: advance(interp); exec_dim(interp); break;
#ifdef NEKO_GRAPHICS
            case TOKEN_SCREEN: advance(interp); exec_screen(interp);            break;
            case TOKEN_CLOSE:  advance(interp); exec_close(interp);             break;
            case TOKEN_COLOR:  advance(interp); exec_color(interp);             break;
            case TOKEN_CLEAR:  advance(interp); exec_clear(interp);             break;
            case TOKEN_FLIP:   advance(interp); exec_flip(interp);              break;
            case TOKEN_POINT:  advance(interp); exec_point(interp);             break;
            case TOKEN_TEXT:   advance(interp); exec_text(interp);              break;
            case TOKEN_LINE:   advance(interp); exec_line_gfx(interp);          break;
            case TOKEN_RECT:
                advance(interp);
                if (check(interp, TOKEN_FILLED)) {
                    advance(interp);
                    exec_rect(interp, true);
                } else {
                    exec_rect(interp, false);
                }
                break;
            case TOKEN_CIRCLE:
                advance(interp);
                if (check(interp, TOKEN_FILLED)) {
                    advance(interp);
                    exec_circle(interp, true);
                } else {
                    exec_circle(interp, false);
                }
                break;
            case TOKEN_FILLED:
                advance(interp);
                if (check(interp, TOKEN_CIRCLE)) {
                    advance(interp);
                    exec_circle(interp, true);
                } else if (check(interp, TOKEN_RECT)) {
                    advance(interp);
                    exec_rect(interp, true);
                } else {
                    interp_error(interp, "EXPECTED CIRCLE OR RECT AFTER FILLED");
                }
                break;
#else
            case TOKEN_SCREEN:
            case TOKEN_CLOSE:
            case TOKEN_COLOR:
            case TOKEN_CLEAR:
            case TOKEN_FLIP:
            case TOKEN_POINT:
            case TOKEN_TEXT:
            case TOKEN_LINE:
            case TOKEN_RECT:
            case TOKEN_CIRCLE:
            case TOKEN_FILLED:
                interp_error(interp, "GRAPHICS NOT SUPPORTED");
                break;
#endif
            case TOKEN_REM:    return; // skip rest of line
            default:
                interp_error(interp, "SYNTAX ERROR");
                break;
        }
    }

    if (interp->error) {
        printf("%s\n", interp->error_msg);
    }
}

void interp_free(interpreter_t *interp) {
#ifdef NEKO_GRAPHICS
    if (interp->window_open) CloseWindow();
#endif
	free(interp);
}

/* Helper Functions */

static void interp_error(interpreter_t *interp, const char *msg) {
	interp->error = true;
	if (interp->current_line > 0) {
		snprintf(interp->error_msg, 256, "?%s IN LINE %d", msg, interp->current_line);
	} else {
		snprintf(interp->error_msg, 256, "?%s", msg);
	}
}

static token_t *peek(interpreter_t *interp) {
	return &interp->tokens[interp->pos];
}

static token_t *advance(interpreter_t *interp) {
	return &interp->tokens[interp->pos++];
}

static bool check(interpreter_t *interp, token_type_t type) {
	return peek(interp)->type == type;
}

static bool expect(interpreter_t *interp, token_type_t type) {
	if (check(interp, type)) {
		advance(interp);
		return true;
	}
	interp_error(interp, "SYNTAX ERROR");
	return false;
}

static value_t make_num(double n) {
	return (value_t){ .type = VALUE_NUMBER, .value_num = n };
}

static value_t make_str(const char *s) {
	value_t v = { .type = VALUE_STRING };
	strncpy(v.value_str, s, 255);
	v.value_str[255] = '\0';
	return v;
}

static value_t parse_primary(interpreter_t *interp) {
    if (interp->error) return make_num(0);

    token_t *tok = peek(interp);

    switch (tok->type) {
        case TOKEN_NUMBER:
            advance(interp);
            return make_num(strtod(tok->value, nullptr));

        case TOKEN_STRING:
            advance(interp);
            return make_str(tok->value);

        case TOKEN_IDENTIFIER: {
            advance(interp);
            char name[64];
            strncpy(name, tok->value, 63);
            name[63] = '\0';

            // check for array access
            if (check(interp, TOKEN_LPAREN)) {
                advance(interp);

                size_t indices[MAX_ARRAY_DIMS];
                size_t dims = 0;

                while (!interp->error && !check(interp, TOKEN_RPAREN) && !check(interp, TOKEN_EOF)) {
                    if (dims >= MAX_ARRAY_DIMS) {
                        interp_error(interp, "TOO MANY DIMENSIONS");
                        return make_num(0);
                    }
                    value_t idx = parse_expression(interp);
                    if (interp->error) return make_num(0);
                    indices[dims++] = (size_t)idx.value_num;
                    if (check(interp, TOKEN_COMMA)) advance(interp);
                }

                if (!expect(interp, TOKEN_RPAREN)) return make_num(0);

                bool is_str = name[strlen(name) - 1] == '$';
                if (is_str) {
                    return make_str(var_array_get_str(interp->vars, name, indices));
                } else {
                    return make_num(var_array_get_num(interp->vars, name, indices));
                }
            }

            // scalar variable lookup
            bool is_str = name[strlen(name) - 1] == '$';
            if (is_str) {
                return make_str(var_get_str(interp->vars, name));
            } else {
                return make_num(var_get_num(interp->vars, name));
            }
        }

        case TOKEN_LPAREN: {
            advance(interp);
            value_t val = parse_expression(interp);
            expect(interp, TOKEN_RPAREN);
            return val;
        }

        case TOKEN_SQR: { MATH_FUNC(sqrt) }
        case TOKEN_ABS: { MATH_FUNC(fabs) }
        case TOKEN_SIN: { MATH_FUNC(sin) }
        case TOKEN_COS: { MATH_FUNC(cos) }
        case TOKEN_TAN: { MATH_FUNC(tan) }
        case TOKEN_LOG: { MATH_FUNC(log) }
        case TOKEN_EXP: { MATH_FUNC(exp) }
        case TOKEN_ATN: { MATH_FUNC(atan) }

        case TOKEN_INT: {
            advance(interp);
            if (!expect(interp, TOKEN_LPAREN)) return make_num(0);
            value_t arg = parse_expression(interp);
            if (interp->error) return make_num(0);
            if (!expect(interp, TOKEN_RPAREN)) return make_num(0);
            return make_num(floor(arg.value_num));
        }

        case TOKEN_SGN: {
            advance(interp);
            if (!expect(interp, TOKEN_LPAREN)) return make_num(0);
            value_t arg = parse_expression(interp);
            if (interp->error) return make_num(0);
            if (!expect(interp, TOKEN_RPAREN)) return make_num(0);
            return make_num(arg.value_num > 0 ? 1 : arg.value_num < 0 ? -1 : 0);
        }

        case TOKEN_RND: {
            advance(interp);
            if (!expect(interp, TOKEN_LPAREN)) return make_num(0);
            value_t arg = parse_expression(interp);
            if (interp->error) return make_num(0);
            if (!expect(interp, TOKEN_RPAREN)) return make_num(0);
            (void)arg;
            return make_num((double)rand() / (double)RAND_MAX);
        }

        case TOKEN_LEN: {
            advance(interp);
            if (!expect(interp, TOKEN_LPAREN)) return make_num(0);
            value_t arg = parse_expression(interp);
            if (interp->error) return make_num(0);
            if (!expect(interp, TOKEN_RPAREN)) return make_num(0);
            return make_num((double)strlen(arg.value_str));
        }

        case TOKEN_MID: {
            advance(interp);
            if (!expect(interp, TOKEN_LPAREN)) return make_str("");
            value_t str = parse_expression(interp);
            if (interp->error) return make_str("");
            if (!expect(interp, TOKEN_COMMA)) return make_str("");
            value_t start = parse_expression(interp);
            if (interp->error) return make_str("");

            // optional length argument
            size_t len = strlen(str.value_str);
            if (check(interp, TOKEN_COMMA)) {
                advance(interp);
                value_t length = parse_expression(interp);
                if (interp->error) return make_str("");
                len = (size_t)length.value_num;
            }

            if (!expect(interp, TOKEN_RPAREN)) return make_str("");

            size_t s = (size_t)start.value_num - 1; // BASIC is 1-indexed
            if (s >= strlen(str.value_str)) return make_str("");

            char buf[256];
            strncpy(buf, str.value_str + s, len);
            buf[len] = '\0';
            return make_str(buf);
        }

        case TOKEN_LEFT: {
            advance(interp);
            if (!expect(interp, TOKEN_LPAREN)) return make_str("");
            value_t str = parse_expression(interp);
            if (interp->error) return make_str("");
            if (!expect(interp, TOKEN_COMMA)) return make_str("");
            value_t len = parse_expression(interp);
            if (interp->error) return make_str("");
            if (!expect(interp, TOKEN_RPAREN)) return make_str("");

            char buf[256];
            strncpy(buf, str.value_str, (size_t)len.value_num);
            buf[(size_t)len.value_num] = '\0';
            return make_str(buf);
        }

        case TOKEN_RIGHT: {
            advance(interp);
            if (!expect(interp, TOKEN_LPAREN)) return make_str("");
            value_t str = parse_expression(interp);
            if (interp->error) return make_str("");
            if (!expect(interp, TOKEN_COMMA)) return make_str("");
            value_t len = parse_expression(interp);
            if (interp->error) return make_str("");
            if (!expect(interp, TOKEN_RPAREN)) return make_str("");

            size_t slen = strlen(str.value_str);
            size_t n    = (size_t)len.value_num;
            if (n > slen) n = slen;
            return make_str(str.value_str + slen - n);
        }

        case TOKEN_STR: {
            advance(interp);
            if (!expect(interp, TOKEN_LPAREN)) return make_str("");
            value_t arg = parse_expression(interp);
            if (interp->error) return make_str("");
            if (!expect(interp, TOKEN_RPAREN)) return make_str("");

            char buf[64];
            snprintf(buf, sizeof(buf), "%g", arg.value_num);
            return make_str(buf);
        }

        case TOKEN_VAL: {
            advance(interp);
            if (!expect(interp, TOKEN_LPAREN)) return make_num(0);
            value_t arg = parse_expression(interp);
            if (interp->error) return make_num(0);
            if (!expect(interp, TOKEN_RPAREN)) return make_num(0);

            char *end;
            double val = strtod(arg.value_str, &end);
            return make_num(val);
        }

        case TOKEN_CHR: {
            advance(interp);
            if (!expect(interp, TOKEN_LPAREN)) return make_str("");
            value_t arg = parse_expression(interp);
            if (interp->error) return make_str("");
            if (!expect(interp, TOKEN_RPAREN)) return make_str("");

            char buf[2] = { (char)(int)arg.value_num, '\0' };
            return make_str(buf);
        }

        case TOKEN_ASC: {
            advance(interp);
            if (!expect(interp, TOKEN_LPAREN)) return make_num(0);
            value_t arg = parse_expression(interp);
            if (interp->error) return make_num(0);
            if (!expect(interp, TOKEN_RPAREN)) return make_num(0);

            if (arg.value_str[0] == '\0') return make_num(0);
            return make_num((double)(unsigned char)arg.value_str[0]);
        }

        default:
            interp_error(interp, "UNEXPECTED TOKEN");
            return make_num(0);
    }
}

static value_t parse_power(interpreter_t *interp) {
    if (interp->error) return make_num(0);
    value_t base = parse_primary(interp);
    if (check(interp, TOKEN_CARET)) {
        advance(interp);
        value_t exp = parse_power(interp); // right associative
        return make_num(pow(base.value_num, exp.value_num));
    }
    return base;
}

static value_t parse_unary(interpreter_t *interp) {
    if (interp->error) return make_num(0);
    if (check(interp, TOKEN_MINUS)) {
        advance(interp);
        value_t val = parse_power(interp);
        return make_num(-val.value_num);
    }
    if (check(interp, TOKEN_NOT)) {
        advance(interp);
        value_t val = parse_power(interp);
        return make_num(!val.value_num);
    }
    return parse_power(interp);
}

static value_t parse_mul(interpreter_t *interp) {
    if (interp->error) return make_num(0);
    value_t left = parse_unary(interp);
    while (!interp->error) {
        if (check(interp, TOKEN_STAR)) {
            advance(interp);
            value_t right = parse_unary(interp);
            left = make_num(left.value_num * right.value_num);
        } else if (check(interp, TOKEN_SLASH)) {
            advance(interp);
            value_t right = parse_unary(interp);
            if (right.value_num == 0.0) {
                interp_error(interp, "DIVISION BY ZERO");
                return make_num(0);
            }
            left = make_num(left.value_num / right.value_num);
        } else if (check(interp, TOKEN_MOD)) {
            advance(interp);
            value_t right = parse_unary(interp);
            if (right.value_num == 0.0) {
                interp_error(interp, "DIVISION BY ZERO");
                return make_num(0);
            }
            left = make_num(fmod(left.value_num, right.value_num));
        } else {
            break;
        }
    }
    return left;
}

static value_t parse_add(interpreter_t *interp) {
    if (interp->error) return make_num(0);
    value_t left = parse_mul(interp);
    while (!interp->error) {
        if (check(interp, TOKEN_PLUS)) {
            advance(interp);
            value_t right = parse_mul(interp);
            // handle string concatenation with +
            if (left.type == VALUE_STRING && right.type == VALUE_STRING) {
                char buf[512];
                snprintf(buf, sizeof(buf), "%s%s", left.value_str, right.value_str);
                left = make_str(buf);
            } else {
                left = make_num(left.value_num + right.value_num);
            }
        } else if (check(interp, TOKEN_MINUS)) {
            advance(interp);
            value_t right = parse_mul(interp);
            left = make_num(left.value_num - right.value_num);
        } else {
            break;
        }
    }
    return left;
}

static value_t parse_comparison(interpreter_t *interp) {
    if (interp->error) return make_num(0);
    value_t left = parse_add(interp);
    while (!interp->error) {
        token_type_t op = peek(interp)->type;
        if (op != TOKEN_EQUAL    && op != TOKEN_NOT_EQUAL &&
            op != TOKEN_LESS     && op != TOKEN_LESS_EQ   &&
            op != TOKEN_GREATER  && op != TOKEN_GREATER_EQ) break;
        advance(interp);
        value_t right = parse_add(interp);
        if (left.type == VALUE_STRING && right.type == VALUE_STRING) {
            int cmp = strcmp(left.value_str, right.value_str);
            switch (op) {
                case TOKEN_EQUAL:      left = make_num(cmp == 0); break;
                case TOKEN_NOT_EQUAL:  left = make_num(cmp != 0); break;
                case TOKEN_LESS:       left = make_num(cmp <  0); break;
                case TOKEN_LESS_EQ:    left = make_num(cmp <= 0); break;
                case TOKEN_GREATER:    left = make_num(cmp >  0); break;
                case TOKEN_GREATER_EQ: left = make_num(cmp >= 0); break;
                default: break;
            }
        } else {
            switch (op) {
                case TOKEN_EQUAL:      left = make_num(left.value_num == right.value_num); break;
                case TOKEN_NOT_EQUAL:  left = make_num(left.value_num != right.value_num); break;
                case TOKEN_LESS:       left = make_num(left.value_num <  right.value_num); break;
                case TOKEN_LESS_EQ:    left = make_num(left.value_num <= right.value_num); break;
                case TOKEN_GREATER:    left = make_num(left.value_num >  right.value_num); break;
                case TOKEN_GREATER_EQ: left = make_num(left.value_num >= right.value_num); break;
                default: break;
            }
        }
    }
    return left;
}

static value_t parse_and(interpreter_t *interp) {
    if (interp->error) return make_num(0);
    value_t left = parse_comparison(interp);
    while (!interp->error && check(interp, TOKEN_AND)) {
        advance(interp);
        value_t right = parse_comparison(interp);
        left = make_num(left.value_num && right.value_num);
    }
    return left;
}

static value_t parse_expression(interpreter_t *interp) {
    if (interp->error) return make_num(0);
    value_t left = parse_and(interp);
    while (!interp->error && check(interp, TOKEN_OR)) {
        advance(interp);
        value_t right = parse_and(interp);
        left = make_num(left.value_num || right.value_num);
    }
    return left;
}

static void exec_print(interpreter_t *interp) {
    while (!interp->error && !check(interp, TOKEN_NEWLINE) && !check(interp, TOKEN_EOF)) {
        value_t val = parse_expression(interp);
        if (val.type == VALUE_STRING) {
            printf("%s", val.value_str);
        } else {
            // BASIC traditionally prints a leading space for positive numbers
            if (val.value_num >= 0) printf(" ");
            printf("%g", val.value_num);
        }

        if (check(interp, TOKEN_SEMICOLON)) {
            advance(interp); // suppress newline, continue on same line
        } else if (check(interp, TOKEN_COMMA)) {
            advance(interp); // tab stop, just print a tab for now
            printf("\t");
        } else {
            break;
        }
    }
    printf("\n");
}

static void exec_let(interpreter_t *interp) {
    if (!check(interp, TOKEN_IDENTIFIER)) {
        interp_error(interp, "EXPECTED VARIABLE NAME");
        return;
    }

    char name[64];
    strncpy(name, peek(interp)->value, 63);
    name[63] = '\0';
    advance(interp);

    // check for array assignment
    if (check(interp, TOKEN_LPAREN)) {
        advance(interp);

        size_t indices[MAX_ARRAY_DIMS];
        size_t dims = 0;

        while (!interp->error && !check(interp, TOKEN_RPAREN) && !check(interp, TOKEN_EOF)) {
            if (dims >= MAX_ARRAY_DIMS) {
                interp_error(interp, "TOO MANY DIMENSIONS");
                return;
            }
            value_t idx = parse_expression(interp);
            if (interp->error) return;
            indices[dims++] = (size_t)idx.value_num;
            if (check(interp, TOKEN_COMMA)) advance(interp);
        }

        if (!expect(interp, TOKEN_RPAREN)) return;
        if (!expect(interp, TOKEN_EQUAL))  return;

        value_t val = parse_expression(interp);
        if (interp->error) return;

        bool is_str = name[strlen(name) - 1] == '$';
        if (is_str) {
            var_array_set_str(interp->vars, name, indices, val.value_str);
        } else {
            var_array_set_num(interp->vars, name, indices, val.value_num);
        }
        return;
    }

    // scalar assignment
    if (!expect(interp, TOKEN_EQUAL)) return;

    value_t val = parse_expression(interp);
    if (interp->error) return;

    bool is_str = name[strlen(name) - 1] == '$';
    if (is_str) {
        var_set_str(interp->vars, name, val.value_str);
    } else {
        var_set_num(interp->vars, name, val.value_num);
    }
}

static void exec_if(interpreter_t *interp) {
    value_t cond = parse_expression(interp);
    if (interp->error) return;
    if (!expect(interp, TOKEN_THEN)) return;
    if (cond.value_num == 0.0) {
        // skip rest of line
        while (!check(interp, TOKEN_NEWLINE) && !check(interp, TOKEN_EOF)) {
            advance(interp);
        }
    }
    // if true, fall through and let the dispatch loop handle GOTO
}

static void exec_goto(interpreter_t *interp) {
    if (!check(interp, TOKEN_NUMBER)) {
        interp_error(interp, "EXPECTED LINE NUMBER");
        return;
    }
    interp->goto_target = (int)strtol(peek(interp)->value, nullptr, 10);
    advance(interp);
}

static void exec_end(interpreter_t *interp) {
    interp->running = false;
}

static void exec_input(interpreter_t *interp) {
    // optional prompt string
    if (check(interp, TOKEN_STRING)) {
        printf("%s", peek(interp)->value);
        advance(interp);
        if (check(interp, TOKEN_SEMICOLON)) advance(interp);
    }

    if (!check(interp, TOKEN_IDENTIFIER)) {
        interp_error(interp, "EXPECTED VARIABLE NAME");
        return;
    }

    char name[64];
    strncpy(name, peek(interp)->value, 63);
    name[63] = '\0';
    advance(interp);

    printf("? ");
    char buf[256];
    auto _ = fgets(buf, sizeof(buf), stdin);
    buf[strcspn(buf, "\n")] = '\0'; // strip newline

    bool is_str = name[strlen(name) - 1] == '$';
    if (is_str) {
        var_set_str(interp->vars, name, buf);
    } else {
        char *end;
        double val = strtod(buf, &end);
        if (*end != '\0') {
            interp_error(interp, "TYPE MISMATCH");
            return;
        }
        var_set_num(interp->vars, name, val);
    }
}

static void exec_for(interpreter_t *interp) {
    if (interp->for_top >= 32) {
        interp_error(interp, "FOR STACK OVERFLOW");
        return;
    }

    // get loop variable name
    if (!check(interp, TOKEN_IDENTIFIER)) {
        interp_error(interp, "EXPECTED VARIABLE NAME");
        return;
    }
    char var[64];
    strncpy(var, peek(interp)->value, 63);
    var[63] = '\0';
    advance(interp);

    // expect =
    if (!expect(interp, TOKEN_EQUAL)) return;

    // get start value
    value_t start = parse_expression(interp);
    if (interp->error) return;
    var_set_num(interp->vars, var, start.value_num);

    // expect TO
    if (!expect(interp, TOKEN_TO)) return;

    // get limit
    value_t limit = parse_expression(interp);
    if (interp->error) return;

    // optional STEP
    double step = 1.0;
    if (check(interp, TOKEN_STEP)) {
        advance(interp);
        value_t step_val = parse_expression(interp);
        if (interp->error) return;
        step = step_val.value_num;
    }

    // push frame
    for_frame_t *frame = &interp->for_stack[interp->for_top++];
    strncpy(frame->var, var, 63);
    frame->var[63] = '\0';
    frame->limit   = limit.value_num;
    frame->step    = step;
    frame->line    = program_next(program_find(interp->program, interp->current_line));
}

static void exec_next(interpreter_t *interp) {
    // optional variable name
    char var[64] = {0};
    if (check(interp, TOKEN_IDENTIFIER)) {
        strncpy(var, peek(interp)->value, 63);
        var[63] = '\0';
        advance(interp);
    }

    if (interp->for_top == 0) {
        interp_error(interp, "NEXT WITHOUT FOR");
        return;
    }

    for_frame_t *frame = &interp->for_stack[interp->for_top - 1];

    // if variable specified, check it matches
    if (var[0] != '\0' && strcmp(var, frame->var) != 0) {
        interp_error(interp, "NEXT VARIABLE MISMATCH");
        return;
    }

    // increment variable
    double val = var_get_num(interp->vars, frame->var) + frame->step;
    var_set_num(interp->vars, frame->var, val);

    // check if loop is done
    bool done = frame->step >= 0 ? val > frame->limit : val < frame->limit;

    if (done) {
        interp->for_top--; // pop frame
    } else {
        // jump back to FOR line
        interp->goto_target = line_number(frame->line);
    }
}

static void exec_gosub(interpreter_t *interp) {
    if (interp->gosub_top >= 32) {
        interp_error(interp, "GOSUB STACK OVERFLOW");
        return;
    }

    if (!check(interp, TOKEN_NUMBER)) {
        interp_error(interp, "EXPECTED LINE NUMBER");
        return;
    }

    int target = (int)strtol(peek(interp)->value, nullptr, 10);
    advance(interp);

    // push return line
    interp->gosub_stack[interp->gosub_top++].line =
        program_find(interp->program, interp->current_line);

    interp->goto_target = target;
}

static void exec_return(interpreter_t *interp) {
    if (interp->gosub_top == 0) {
        interp_error(interp, "RETURN WITHOUT GOSUB");
        return;
    }

    gosub_frame_t *frame = &interp->gosub_stack[--interp->gosub_top];
    interp->goto_target  = line_number(program_next(frame->line));
}

static void exec_stop(interpreter_t *interp) {
    printf("BREAK IN LINE %d\n", interp->current_line);
    interp->running = false;
}

static void exec_while(interpreter_t *interp) {
    if (interp->while_top >= 32) {
        interp_error(interp, "WHILE STACK OVERFLOW");
        return;
    }

    value_t cond = parse_expression(interp);
    if (interp->error) return;

    if (cond.value_num != 0.0) {
        // condition true, push frame and continue
        while_frame_t *frame = &interp->while_stack[interp->while_top++];
        frame->line = program_find(interp->program, interp->current_line);
    } else {
        // condition false, skip to matching WEND
        line_t *cur = program_next(program_find(interp->program, interp->current_line));
        int depth = 1;
        while (cur != nullptr && depth > 0) {
            token_t *toks = line_tokens(cur);
            if (toks[0].type == TOKEN_WHILE) depth++;
            if (toks[0].type == TOKEN_WEND)  depth--;
            if (depth > 0) cur = program_next(cur);
        }
        if (cur == nullptr) {
            interp_error(interp, "WHILE WITHOUT WEND");
            return;
        }

        line_t *next = program_next(cur);
        if (next == nullptr) {
            interp->running = false;
        } else {
            interp->goto_target = line_number(next);
        }
    }
}

static void exec_wend(interpreter_t *interp) {
    if (interp->while_top == 0) {
        interp_error(interp, "WEND WITHOUT WHILE");
        return;
    }

    while_frame_t *frame = &interp->while_stack[interp->while_top - 1];

    // re-evaluate condition on the WHILE line
    interp->goto_target = line_number(frame->line);
    interp->while_top--;
}

static void exec_dim(interpreter_t *interp) {
    if (!check(interp, TOKEN_IDENTIFIER)) {
        interp_error(interp, "EXPECTED ARRAY NAME");
        return;
    }

    char name[64];
    strncpy(name, peek(interp)->value, 63);
    name[63] = '\0';
    advance(interp);

    if (!expect(interp, TOKEN_LPAREN)) return;

    size_t sizes[MAX_ARRAY_DIMS];
    size_t dims = 0;

    while (!interp->error && !check(interp, TOKEN_RPAREN) && !check(interp, TOKEN_EOF)) {
        if (dims >= MAX_ARRAY_DIMS) {
            interp_error(interp, "TOO MANY DIMENSIONS");
            return;
        }
        value_t size = parse_expression(interp);
        if (interp->error) return;
        sizes[dims++] = (size_t)size.value_num + 1; // +1 for BASIC's 0-based indexing
        if (check(interp, TOKEN_COMMA)) advance(interp);
    }

    if (!expect(interp, TOKEN_RPAREN)) return;
    if (dims == 0) {
        interp_error(interp, "EXPECTED DIMENSION");
        return;
    }

    var_dim(interp->vars, name, dims, sizes);
}

#ifdef NEKO_GRAPHICS

static void exec_screen(interpreter_t *interp) {
    if (interp->window_open) {
        interp_error(interp, "WINDOW ALREADY OPEN");
        return;
    }

    value_t width = parse_expression(interp);
    if (interp->error) return;
    if (!expect(interp, TOKEN_COMMA)) return;

    value_t height = parse_expression(interp);
    if (interp->error) return;

    interp->window_title = "Hello BASIC";
    if (check(interp, TOKEN_COMMA)) {
        advance(interp);
        value_t t = parse_expression(interp);
        if (interp->error) return;
        char *title_copy = strdup(t.value_str);
        if (!title_copy) {
            interp_error(interp, "OUT OF MEMORY");
            return;
        }
        interp->window_title = title_copy;
    }

    InitWindow((int)width.value_num, (int)height.value_num, interp->window_title);
    SetTargetFPS(60);
    interp->window_open = true;
    interp->current_color = WHITE;
}

static void exec_close(interpreter_t *interp) {
    if (!interp->window_open) {
        interp_error(interp, "NO WINDOW OPEN");
        return;
    }
    CloseWindow();
    free(interp->window_title);
    interp->window_title = nullptr;
    interp->window_open = false;
    interp->running     = false;
}

static void exec_color(interpreter_t *interp) {
    if (!interp->window_open) {
        interp_error(interp, "NO WINDOW OPEN");
        return;
    }

    value_t r = parse_expression(interp);
    if (interp->error) return;
    if (!expect(interp, TOKEN_COMMA)) return;

    value_t g = parse_expression(interp);
    if (interp->error) return;
    if (!expect(interp, TOKEN_COMMA)) return;

    value_t b = parse_expression(interp);
    if (interp->error) return;

    // optional alpha
    int alpha = 255;
    if (check(interp, TOKEN_COMMA)) {
        advance(interp);
        value_t a = parse_expression(interp);
        if (interp->error) return;
        alpha = (int)a.value_num;
    }

    interp->current_color = (Color){
        (unsigned char)r.value_num,
        (unsigned char)g.value_num,
        (unsigned char)b.value_num,
        (unsigned char)alpha
    };
}

static void exec_clear(interpreter_t *interp) {
    if (!interp->window_open) {
        interp_error(interp, "NO WINDOW OPEN");
        return;
    }

    // optional background color
    Color bg = BLACK;
    if (!check(interp, TOKEN_NEWLINE) && !check(interp, TOKEN_EOF)) {
        value_t r = parse_expression(interp);
        if (interp->error) return;
        if (!expect(interp, TOKEN_COMMA)) return;
        value_t g = parse_expression(interp);
        if (interp->error) return;
        if (!expect(interp, TOKEN_COMMA)) return;
        value_t b = parse_expression(interp);
        if (interp->error) return;
        bg = (Color){
            (unsigned char)r.value_num,
            (unsigned char)g.value_num,
            (unsigned char)b.value_num,
            255
        };
    }

    BeginDrawing();
    ClearBackground(bg);
}

static void exec_flip(interpreter_t *interp) {
    if (!interp->window_open) {
        interp_error(interp, "NO WINDOW OPEN");
        return;
    }
    EndDrawing();

    if (WindowShouldClose()) {
        CloseWindow();
        free(interp->window_title);
        interp->window_title = nullptr;
        interp->window_open = false;
        interp->running     = false;
    }
}

static void exec_circle(interpreter_t *interp, bool filled) {
    if (!interp->window_open) {
        interp_error(interp, "NO WINDOW OPEN");
        return;
    }

    value_t x = parse_expression(interp);
    if (interp->error) return;
    if (!expect(interp, TOKEN_COMMA)) return;

    value_t y = parse_expression(interp);
    if (interp->error) return;
    if (!expect(interp, TOKEN_COMMA)) return;

    value_t r = parse_expression(interp);
    if (interp->error) return;

    if (filled) {
        DrawCircle((int)x.value_num, (int)y.value_num, (float)r.value_num, interp->current_color);
    } else {
        DrawCircleLines((int)x.value_num, (int)y.value_num, (float)r.value_num, interp->current_color);
    }
}

static void exec_line_gfx(interpreter_t *interp) {
    if (!interp->window_open) {
        interp_error(interp, "NO WINDOW OPEN");
        return;
    }

    value_t x1 = parse_expression(interp);
    if (interp->error) return;
    if (!expect(interp, TOKEN_COMMA)) return;

    value_t y1 = parse_expression(interp);
    if (interp->error) return;
    if (!expect(interp, TOKEN_COMMA)) return;

    value_t x2 = parse_expression(interp);
    if (interp->error) return;
    if (!expect(interp, TOKEN_COMMA)) return;

    value_t y2 = parse_expression(interp);
    if (interp->error) return;

    DrawLine((int)x1.value_num, (int)y1.value_num,
             (int)x2.value_num, (int)y2.value_num,
             interp->current_color);
}

static void exec_rect(interpreter_t *interp, bool filled) {
    if (!interp->window_open) {
        interp_error(interp, "NO WINDOW OPEN");
        return;
    }

    value_t x = parse_expression(interp);
    if (interp->error) return;
    if (!expect(interp, TOKEN_COMMA)) return;

    value_t y = parse_expression(interp);
    if (interp->error) return;
    if (!expect(interp, TOKEN_COMMA)) return;

    value_t w = parse_expression(interp);
    if (interp->error) return;
    if (!expect(interp, TOKEN_COMMA)) return;

    value_t h = parse_expression(interp);
    if (interp->error) return;

    if (filled) {
        DrawRectangle((int)x.value_num, (int)y.value_num,
                      (int)w.value_num, (int)h.value_num,
                      interp->current_color);
    } else {
        DrawRectangleLines((int)x.value_num, (int)y.value_num,
                           (int)w.value_num, (int)h.value_num,
                           interp->current_color);
    }
}

static void exec_point(interpreter_t *interp) {
    if (!interp->window_open) {
        interp_error(interp, "NO WINDOW OPEN");
        return;
    }

    value_t x = parse_expression(interp);
    if (interp->error) return;
    if (!expect(interp, TOKEN_COMMA)) return;

    value_t y = parse_expression(interp);
    if (interp->error) return;

    DrawPixel((int)x.value_num, (int)y.value_num, interp->current_color);
}

static void exec_text(interpreter_t *interp) {
    if (!interp->window_open) {
        interp_error(interp, "NO WINDOW OPEN");
        return;
    }

    value_t x = parse_expression(interp);
    if (interp->error) return;
    if (!expect(interp, TOKEN_COMMA)) return;

    value_t y = parse_expression(interp);
    if (interp->error) return;
    if (!expect(interp, TOKEN_COMMA)) return;

    value_t str = parse_expression(interp);
    if (interp->error) return;

    // optional font size
    int size = 20;
    if (check(interp, TOKEN_COMMA)) {
        advance(interp);
        value_t s = parse_expression(interp);
        if (interp->error) return;
        size = (int)s.value_num;
    }

    DrawText(str.value_str, (int)x.value_num, (int)y.value_num, size, interp->current_color);
}

#endif

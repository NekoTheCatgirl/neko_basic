#include "lexer.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const keyword_t keywords[] = {
	{"PRINT",  TOKEN_PRINT},
	{"LET",	TOKEN_LET},
	{"IF",		TOKEN_IF},
	{"THEN",   TOKEN_THEN},
	{"ELSE",   TOKEN_ELSE},
	{"GOTO",   TOKEN_GOTO},
	{"GOSUB",  TOKEN_GOSUB},
	{"RETURN", TOKEN_RETURN},
	{"FOR",	TOKEN_FOR},
	{"TO",		TOKEN_TO},
	{"STEP",   TOKEN_STEP},
	{"NEXT",   TOKEN_NEXT},
	{"WHILE",  TOKEN_WHILE},
	{"WEND",   TOKEN_WEND},
	{"INPUT",  TOKEN_INPUT},
	{"DIM",	TOKEN_DIM},
	{"END",	TOKEN_END},
	{"STOP",   TOKEN_STOP},
	{"REM",	TOKEN_REM},
	{"LIST",   TOKEN_LIST},
	{"RUN",	TOKEN_RUN},
	{"NEW",	TOKEN_NEW},
	{"LOAD",   TOKEN_LOAD},
	{"SAVE",   TOKEN_SAVE},
	{"AND",	TOKEN_AND},
	{"OR",		TOKEN_OR},
	{"NOT",	TOKEN_NOT},
	{"MOD",	TOKEN_MOD},
	{"SQR", 	TOKEN_SQR},
	{"INT", 	TOKEN_INT},
	{"ABS", 	TOKEN_ABS},
	{"RND", 	TOKEN_RND},
	{"SIN", 	TOKEN_SIN},
	{"COS", 	TOKEN_COS},
	{"TAN", 	TOKEN_TAN},
	{"LOG", 	TOKEN_LOG},
	{"EXP", 	TOKEN_EXP},
	{"SGN", 	TOKEN_SGN},
	{"ATN", 	TOKEN_ATN},
	{"LEN",	TOKEN_LEN},
	{"MID$",   TOKEN_MID},
	{"LEFT$",  TOKEN_LEFT},
	{"RIGHT$", TOKEN_RIGHT},
	{"STR$",   TOKEN_STR},
	{"VAL",	TOKEN_VAL},
	{"CHR$",   TOKEN_CHR},
	{"ASC",	TOKEN_ASC},
	{"SCREEN",	TOKEN_SCREEN},
	{"CIRCLE", TOKEN_CIRCLE},
	{"LINE",	TOKEN_LINE},
	{"RECT",	TOKEN_RECT},
	{"COLOR",	TOKEN_COLOR},
	{"CLEAR",	TOKEN_CLEAR},
	{"FLIP",	TOKEN_FLIP},
	{"POINT",	TOKEN_POINT},
	{"TEXT",	TOKEN_TEXT},
	{"SPRITE",	TOKEN_SPRITE},
	{"FILLED",	TOKEN_FILLED},
	{"CLOSE",	TOKEN_CLOSE},
	{"DRAW",	TOKEN_DRAW},
	{"FREE",	TOKEN_FREE},
	{"POLL",	TOKEN_POLL},
	{"MOUSE",	TOKEN_MOUSE},
	{"KEY",	TOKEN_KEY},
};

static token_type_t lookup_keyword(const char* word);
static void append_token(token_t **tokens, size_t *count, size_t *capacity, const token_type_t type, const char *value);

token_t* tokenize(const char* source) {
	size_t count = 0;
	size_t capacity = 32;
	token_t *tokens = malloc(sizeof(token_t) * capacity);

	size_t i = 0; // Current position in source

	// While the current byte is not a null-byte, continue.
	while (source[i] != '\0') {
		if (source[i] == '\n') {
			append_token(&tokens, &count, &capacity, TOKEN_NEWLINE, "");
			i++;
			continue;
		}

		if (isspace((unsigned char)source[i])) {
			i++;
			continue;
		}

		if (isdigit((unsigned char)source[i])) {
			char value[64];
			size_t len = 0;
			value[len++] = source[i];

			i++;

			while (isdigit((unsigned char)source[i]) && len < 63) {
				value[len++] = source[i];
				i++;
			}

			// Handle decimal point for floats
			if (source[i] == '.') {
				value[len++] = source[i];
				i++;

				while (isdigit((unsigned char)source[i]) && len < 63) {
					value[len++] = source[i];
					i++;
				}
			}

			value[len] = '\0';
			append_token(&tokens, &count, &capacity, TOKEN_NUMBER, value);
			continue;
		}

		if (source[i] == '"') {
			char value[64];
			size_t len = 0;

			i++;

			while (source[i] != '"' && source[i] != '\0' && len < 63) {
				value[len++] = source[i];
				i++;
			}

			if (source[i] == '"') i++;

			value[len] = '\0';
			append_token(&tokens, &count, &capacity, TOKEN_STRING, value);
			continue;
		}

		if (isalpha((unsigned char)source[i])) {
			char value[64];
			size_t len = 0;

			while ((isalnum((unsigned char)source[i]) || source[i] == '_' || source[i] == '$') && len < 63) {
				value[len++] = (char)toupper((unsigned char)source[i]);
				i++;
			}

			value[len] = '\0';
			const token_type_t type = lookup_keyword(value);
			if (type == TOKEN_REM) {
				char rem_value[256];
				size_t rem_len = 0;
				// Keep "REM" as part of the value or just the comment?
				// The interpreter expects TOKEN_REM, so we'll store the comment part.
				while (source[i] != '\n' && source[i] != '\0' && rem_len < 255) {
					rem_value[rem_len++] = source[i];
					i++;
				}
				rem_value[rem_len] = '\0';
				append_token(&tokens, &count, &capacity, TOKEN_REM, rem_value);
				continue;
			}
			append_token(&tokens, &count, &capacity, type, value);
			continue;
		}

		switch (source[i]) {
			case '+': append_token(&tokens, &count, &capacity, TOKEN_PLUS,	  "+"); i++; break;
			case '-': append_token(&tokens, &count, &capacity, TOKEN_MINUS,	 "-"); i++; break;
			case '*': append_token(&tokens, &count, &capacity, TOKEN_STAR,	  "*"); i++; break;
			case '/': append_token(&tokens, &count, &capacity, TOKEN_SLASH,	 "/"); i++; break;
			case '^': append_token(&tokens, &count, &capacity, TOKEN_CARET,	 "^"); i++; break;
			case '(': append_token(&tokens, &count, &capacity, TOKEN_LPAREN,	"("); i++; break;
			case ')': append_token(&tokens, &count, &capacity, TOKEN_RPAREN,	")"); i++; break;
			case ',': append_token(&tokens, &count, &capacity, TOKEN_COMMA,	 ","); i++; break;
			case ';': append_token(&tokens, &count, &capacity, TOKEN_SEMICOLON, ";"); i++; break;
			case ':': append_token(&tokens, &count, &capacity, TOKEN_COLON,	 ":"); i++; break;

			case '=': append_token(&tokens, &count, &capacity, TOKEN_EQUAL,	 "="); i++; break;

			case '<':
				i++;
				if (source[i] == '=') {
					append_token(&tokens, &count, &capacity, TOKEN_LESS_EQ,	"<="); i++;
				}
				else if (source[i] == '>') {
					append_token(&tokens, &count, &capacity, TOKEN_NOT_EQUAL,  "<>"); i++;
				}
				else {
					append_token(&tokens, &count, &capacity, TOKEN_LESS,		"<");
				}
				break;

			case '>':
				i++;
				if (source[i] == '=') {
					append_token(&tokens, &count, &capacity, TOKEN_GREATER_EQ, ">="); i++;
				}
				else {
					append_token(&tokens, &count, &capacity, TOKEN_GREATER,	 ">");
				}
				break;

			default:
				i++; // unknown character, skip it
				break;
		}
	}

	append_token(&tokens, &count, &capacity, TOKEN_EOF, "");

	return tokens;
}

void tokens_free(token_t* tokens) {
	free(tokens);
}

static token_type_t lookup_keyword(const char* word) {
	for (size_t i = 0; i < sizeof(keywords) / sizeof(keywords[0]); i++) {
		if (strcmp(keywords[i].word, word) == 0)
			return keywords[i].type;
	}
	return TOKEN_IDENTIFIER;
}

static void append_token(token_t **tokens, size_t *count, size_t *capacity, const token_type_t type, const char *value) {
	if (*count >= *capacity) {
		*capacity *= 2;
		token_t *tmp = realloc(*tokens, *capacity * sizeof(token_t));
		if (tmp == nullptr) {
			fprintf(stderr, "out of memory\n");
			exit(1);
		}
		*tokens = tmp;
	}
	(*tokens)[*count].type = type;
	strncpy((*tokens)[*count].value, value, 63);
	(*tokens)[*count].value[63] = '\0';
	(*count)++;
}

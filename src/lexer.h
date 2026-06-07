#pragma once

typedef enum {
	/* Literals */

	TOKEN_NUMBER,
	TOKEN_STRING,
	TOKEN_IDENTIFIER,

	/* Keywords */

	TOKEN_PRINT,
	TOKEN_LET,
	TOKEN_IF,
	TOKEN_THEN,
	TOKEN_ELSE,
	TOKEN_GOTO,
	TOKEN_GOSUB,
	TOKEN_RETURN,
	TOKEN_FOR,
	TOKEN_TO,
	TOKEN_STEP,
	TOKEN_NEXT,
	TOKEN_WHILE,
	TOKEN_WEND,
	TOKEN_INPUT,
	TOKEN_DIM,
	TOKEN_END,
	TOKEN_STOP,
	TOKEN_REM,
	TOKEN_LIST,
	TOKEN_RUN,
	TOKEN_NEW,
	TOKEN_LOAD,
	TOKEN_SAVE,

	/* String Functions */

	TOKEN_LEN,
	TOKEN_MID,
	TOKEN_LEFT,
	TOKEN_RIGHT,
	TOKEN_STR,
	TOKEN_VAL,
	TOKEN_CHR,
	TOKEN_ASC,

	/* Arithmetic Operators */

	TOKEN_PLUS,
	TOKEN_MINUS,
	TOKEN_STAR,
	TOKEN_SLASH,
	TOKEN_CARET, // ^ (power)
	TOKEN_MOD,

	/* Mathematics Functions */

	TOKEN_SQR,
	TOKEN_INT,
	TOKEN_ABS,
	TOKEN_RND,
	TOKEN_SIN,
	TOKEN_COS,
	TOKEN_TAN,
	TOKEN_LOG,
	TOKEN_EXP,
	TOKEN_SGN,
	TOKEN_ATN,

	/* Comparison Operators */

	TOKEN_EQUAL,        // =
	TOKEN_NOT_EQUAL,    // <>
	TOKEN_LESS,         //
	TOKEN_LESS_EQ,      // <=
	TOKEN_GREATER,      // >
	TOKEN_GREATER_EQ,   // >=

	/* Logical Operators */

	TOKEN_AND,
	TOKEN_OR,
	TOKEN_NOT,

	/* Punctuation */

	TOKEN_LPAREN,       // (
	TOKEN_RPAREN,       // )
	TOKEN_COMMA,        // ,
	TOKEN_SEMICOLON,    // ;
	TOKEN_COLON,        // :
	TOKEN_NEWLINE,

	/* Graphics */

	TOKEN_SCREEN,
	TOKEN_CIRCLE,
	TOKEN_LINE,
	TOKEN_RECT,
	TOKEN_COLOR,
	TOKEN_CLEAR,
	TOKEN_FLIP,
	TOKEN_POINT,
	TOKEN_TEXT,
	TOKEN_SPRITE,
	TOKEN_FILLED,
	TOKEN_CLOSE,

	/* EOF */

	TOKEN_EOF,
} token_type_t;

typedef struct keyword_t {
	const char *word;
	token_type_t type;
} keyword_t;

typedef struct token_t {
	token_type_t type;
	char value[64];
} token_t;

token_t *tokenize(const char *source);
void tokens_free(token_t *tokens);

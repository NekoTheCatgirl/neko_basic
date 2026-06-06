#include "vars.h"

#include <stdlib.h>
#include <string.h>

typedef enum {
	VAR_NUMBER,
	VAR_STRING,
} var_type_t;

typedef struct var_t {
	char		name[64];
	var_type_t	type;
	union {
		double	value_num;
		char	value_str[256];
	};
} var_t;

typedef struct var_array_t {
	char		name[64];
	var_type_t	type;
	size_t		dims;
	size_t		sizes[MAX_ARRAY_DIMS];
	size_t		total;
	union {
		double *num_data;
		char   (*str_data)[256];
	};
} var_array_t;

struct vars_t {
	var_t		vars[MAX_VARS];
	size_t		var_count;
	var_array_t arrays[MAX_ARRAYS];
	size_t		array_count;
	pool_t		*pool;
};

static var_array_t *array_find(vars_t *vars, const char *name);
static size_t array_index(const var_array_t *arr, const size_t *indices);
static var_t *var_find(vars_t *vars, const char *name);
static var_t *var_alloc(vars_t *vars, const char *name, const var_type_t type);

vars_t *vars_init(pool_t *pool) {
	vars_t *vars = malloc(sizeof(vars_t));
	if (vars == nullptr) return nullptr;

	vars->var_count = 0;
	vars->array_count = 0;
	vars->pool  = pool;
	return vars;
}

void var_set_num(vars_t *vars, const char *name, const double value) {
	var_t *var = var_alloc(vars, name, VAR_NUMBER);
	if (var == nullptr) return;
	var->value_num = value;
}

void var_set_str(vars_t *vars, const char *name, const char *value) {
	var_t *var = var_alloc(vars, name, VAR_STRING);
	if (var == nullptr) return;
	strncpy(var->value_str, value, 255);
	var->value_str[255] = '\0';
}

double var_get_num(vars_t *vars, const char *name) {
	const var_t *var = var_find(vars, name);
	if (var == nullptr || var->type != VAR_NUMBER) return 0.0;
	return var->value_num;
}

const char *var_get_str(vars_t *vars, const char *name) {
	const var_t *var = var_find(vars, name);
	if (var == nullptr || var->type != VAR_STRING) return "";
	return var->value_str;
}

bool var_exists(vars_t *vars, const char *name) {
	return var_find(vars, name) != nullptr;
}

void var_dim(vars_t *vars, const char *name, const size_t dims, const size_t *sizes) {
	if (vars->array_count >= MAX_ARRAYS) return;

	var_array_t *arr = array_find(vars, name);
	if (arr == nullptr) {
		arr = &vars->arrays[vars->array_count++];
	}

	strncpy(arr->name, name, 63);
	arr->name[63] = '\0';
	arr->dims	 = dims;

	size_t total = 1;
	for (size_t i = 0; i < dims; i++) {
		arr->sizes[i] = sizes[i];
		total		*= sizes[i];
	}
	arr->total = total;

	const bool is_str = name[strlen(name) - 1] == '$';
	arr->type   = is_str ? VAR_STRING : VAR_NUMBER;

	if (is_str) {
		arr->str_data = pool_alloc(vars->pool, total * sizeof(*arr->str_data));
	} else {
		arr->num_data = pool_alloc(vars->pool, total * sizeof(double));
	}
}

void var_array_set_num(vars_t *vars, const char *name, const size_t *indices, const double value) {
	var_array_t *arr = array_find(vars, name);
	if (arr == nullptr || arr->type != VAR_NUMBER) return;
	const size_t idx = array_index(arr, indices);
	if (idx >= arr->total) return;
	arr->num_data[idx] = value;
}

double var_array_get_num(vars_t *vars, const char *name, const size_t *indices) {
	var_array_t *arr = array_find(vars, name);
	if (arr == nullptr || arr->type != VAR_NUMBER) return 0.0;
	const size_t idx = array_index(arr, indices);
	if (idx >= arr->total) return 0.0;
	return arr->num_data[idx];
}

void var_array_set_str(vars_t *vars, const char *name, const size_t *indices, const char *value) {
	var_array_t *arr = array_find(vars, name);
	if (arr == nullptr || arr->type != VAR_STRING) return;
	const size_t idx = array_index(arr, indices);
	if (idx >= arr->total) return;
	strncpy(arr->str_data[idx], value, 255);
	arr->str_data[idx][255] = '\0';
}

const char *var_array_get_str(vars_t *vars, const char *name, const size_t *indices) {
	var_array_t *arr = array_find(vars, name);
	if (arr == nullptr || arr->type != VAR_STRING) return "";
	size_t idx = array_index(arr, indices);
	if (idx >= arr->total) return "";
	return arr->str_data[idx];
}

void vars_free(vars_t *vars) {
	free(vars);
}

/* Helper Functions */

static var_array_t *array_find(vars_t *vars, const char *name) {
	for (size_t i = 0; i < vars->array_count; i++) {
		if (strcmp(vars->arrays[i].name, name) == 0)
			return &vars->arrays[i];
	}
	return nullptr;
}

static size_t array_index(const var_array_t *arr, const size_t *indices) {
	size_t index = 0;
	size_t stride = 1;
	for (size_t i = arr->dims; i > 0; i--) {
		index  += indices[i - 1] * stride;
		stride *= arr->sizes[i - 1];
	}
	return index;
}

static var_t *var_find(vars_t *vars, const char *name) {
	for (size_t i = 0; i < vars->var_count; i++) {
		if (strcmp(vars->vars[i].name, name) == 0)
			return &vars->vars[i];
	}
	return nullptr;
}

static var_t *var_alloc(vars_t *vars, const char *name, const var_type_t type) {
	var_t *var = var_find(vars, name);
	if (var != nullptr) {
		var->type = type;
		return var;
	}

	if (vars->var_count >= MAX_VARS) return nullptr;

	var = &vars->vars[vars->var_count++];
	strncpy(var->name, name, 63);
	var->name[63] = '\0';
	var->type = type;
	return var;
}

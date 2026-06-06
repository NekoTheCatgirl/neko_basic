#pragma once
#include "pool.h"

#define MAX_VARS 256
#define MAX_ARRAY_DIMS 4
#define MAX_ARRAYS	 64

typedef struct vars_t vars_t;

vars_t *vars_init(pool_t *pool);
void	var_set_num(vars_t *vars, const char *name, double value);
void	var_set_str(vars_t *vars, const char *name, const char *value);
double  var_get_num(vars_t *vars, const char *name);
const char *var_get_str(vars_t *vars, const char *name);
bool	var_exists(vars_t *vars, const char *name);
void	var_dim(vars_t *vars, const char *name, size_t dims, const size_t *sizes);
void	var_array_set_num(vars_t *vars, const char *name, const size_t *indices, double value);
double  var_array_get_num(vars_t *vars, const char *name, const size_t *indices);
void	var_array_set_str(vars_t *vars, const char *name, const size_t *indices, const char *value);
const char *var_array_get_str(vars_t *vars, const char *name, const size_t *indices);
void	vars_free(vars_t *vars);
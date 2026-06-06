#pragma once

#include <stddef.h>

typedef struct pool_t pool_t;

pool_t *pool_init(size_t size);
void   *pool_alloc(pool_t *pool, size_t size);
void	pool_reset(pool_t *pool);
void	pool_free(pool_t *pool);
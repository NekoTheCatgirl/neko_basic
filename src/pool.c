#include "pool.h"

#include <stdint.h>
#include <stdlib.h>

struct [[maybe_unused]] pool_t {
	uint8_t *base;
	size_t   offset;
	size_t   size;
};

pool_t *pool_init(size_t size) {
	pool_t *pool = malloc(sizeof(pool_t));
	if (pool == nullptr) return nullptr;

	pool->base = malloc(size);
	if (pool->base == nullptr) {
		free(pool);
		return nullptr;
	}

	pool->offset = 0;
	pool->size = size;
	return pool;
}

void *pool_alloc(pool_t *pool, size_t size) {
	size_t aligned = (size + 7) & ~7;

	if (pool->offset + aligned > pool->size) {
		return nullptr;
	}

	void *ptr = pool->base + pool->offset;
	pool->offset += aligned;
	return ptr;
}

void pool_reset(pool_t *pool) {
	pool->offset = 0;
}

void pool_free(pool_t *pool) {
	free(pool->base);
	free(pool);
}
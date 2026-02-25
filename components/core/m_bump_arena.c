#include "m_int.h"

int m_bump_arena_init_empty(m_bump_arena *arena)
{
	if (!arena)
		return ERR_NULL_PTR;
	
	arena->arena = NULL;
	arena->pos = 0;
	arena->capacity = 0;
	
	return NO_ERROR;
}

int m_bump_arena_init(m_bump_arena *arena, size_t capacity)
{
	if (!arena)
		return ERR_NULL_PTR;
	
	size_t align = M_BUMP_ARENA_INIT_ALIGN;
	arena->base_ua = m_alloc(capacity + align - 1);
	
	if (!arena->base_ua)
	{
		arena->pos = 0;
		arena->arena = NULL;
		arena->capacity = 0;
		return ERR_ALLOC_FAIL;
	}
	
	uintptr_t addr = (uintptr_t)arena->base_ua;
	uintptr_t aligned = (addr + (align - 1)) & ~(uintptr_t)(align - 1);
	
	arena->arena = (void*)aligned;
	arena->pos = 0;
	arena->capacity = capacity;
	
	return NO_ERROR;
}

void *m_bump_arena_alloc(m_bump_arena *arena, size_t size)
{
	if (!arena)
		return NULL;
	
	if (!arena->arena || arena->capacity == 0 || size == 0)
		return NULL;
	
	size = (size + (M_BUMP_ARENA_ALLOC_ALIGN - 1)) & ~(M_BUMP_ARENA_ALLOC_ALIGN - 1);
	
	if (size > arena->capacity - arena->pos)
		return NULL;
	
	uint8_t *ptr = (uint8_t*)arena->arena + arena->pos;
	arena->pos += size;
	
	return ptr;
}

int m_bump_arena_reset(m_bump_arena *arena)
{
	if (!arena)
		return ERR_NULL_PTR;
	
	arena->pos = 0;
	
	return NO_ERROR;
}

int m_bump_arena_destroy(m_bump_arena *arena)
{
	if (!arena)
		return ERR_NULL_PTR;
	
	if (!arena->base_ua)
	{
		arena->capacity = 0;
		arena->arena = NULL;
		arena->pos = 0;
		return NO_ERROR;
	}
	
	m_free(arena->base_ua);
	arena->base_ua = NULL;
	arena->arena = NULL;
	arena->capacity = 0;
	arena->pos = 0;
	
	return NO_ERROR;
}

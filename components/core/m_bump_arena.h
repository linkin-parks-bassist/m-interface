#ifndef M_BUMP_ARENA_H_
#define M_BUMP_ARENA_H_

#define M_BUMP_ARENA_ALLOC_ALIGN 4
#define M_BUMP_ARENA_INIT_ALIGN  64

#if (M_BUMP_ARENA_ALLOC_ALIGN & (M_BUMP_ARENA_ALLOC_ALIGN - 1)) != 0
#error M_BUMP_ARENA_ALLOC_ALIGN must be power of two
#endif
#if (M_BUMP_ARENA_INIT_ALIGN & (M_BUMP_ARENA_INIT_ALIGN - 1)) != 0
#error M_BUMP_ARENA_INIT_ALIGN must be power of two
#endif

typedef struct {
	void *base_ua;
	void *arena;
	size_t pos;
	size_t capacity;
} m_bump_arena;

int m_bump_arena_init_empty(m_bump_arena *arena);
int m_bump_arena_init(m_bump_arena *arena, size_t capacity);

void *m_bump_arena_alloc(m_bump_arena *arena, size_t size);
int   m_bump_arena_reset(m_bump_arena *arena);
int   m_bump_arena_destroy(m_bump_arena *arena);

#endif

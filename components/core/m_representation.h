#ifndef M_INT_REPRESENTATION_H_
#define M_INT_REPRESENTATION_H_

typedef struct
{
	int info;
	
	void *representer;
	void *representee;
	
	void (*update)(void *representer, void *representee);
} m_representation;

DECLARE_LINKED_PTR_LIST(m_representation);

void m_representation_pll_update_all(m_representation_pll *reps);

#ifdef M_ENABLE_REPRESENTATIONS
int init_representation_updater();
#endif
int queue_representation_list_update(m_representation_pll *reps);

#ifdef M_USE_FREERTOS
extern QueueHandle_t m_rep_update_queue;
#endif

m_representation_pll *m_representation_pll_remove(m_representation_pll *reps, m_representation *rep);

#endif

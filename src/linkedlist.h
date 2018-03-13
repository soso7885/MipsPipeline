#ifndef _LINKEDLIST_H
#define _LINKEDLIST_H

#include <stdint.h>
#include <stdbool.h>

#define MEM_LOAD false
#define MEM_STORE true

struct node {
	uint32_t addr;
	uint32_t data;
	bool ls;	// load or store
	struct node *next;
};

void ll_add_node(struct node **head, uint32_t addr, uint32_t data, bool ls);
void ll_free_node(struct node **head, uint32_t addr);
void ll_show_all(struct node *head);
void ll_free_all_node(struct node *head);

#endif

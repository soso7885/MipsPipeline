#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "linkedlist.h"

static struct node *_new(uint32_t addr, uint32_t data, bool ls)
{
	struct node *tmp;
	
	tmp = malloc(sizeof(struct node));
	assert(tmp);
	tmp->data = data;
	tmp->addr = addr;
	tmp->ls = ls;

	return tmp;
}

static inline int ll_get_len(struct node *head)
{
	int len = 0;
	
	while(head){
		head = head->next;
		len++;
	}
	
	return len;
}

static struct node *ll_find_tail(struct node *head)
{
	if(!head) return head;
	
	while(head->next)
		head = head->next;
	
	return head;
}

void ll_add_node(struct node **head, uint32_t addr, uint32_t data, bool ls)
{
	struct node *tail;
	
	if(*head == NULL){
		*head = _new(addr, data, ls);
	}else{
		tail = ll_find_tail(*head);
		tail->next = _new(addr, data, ls);
	}
}

void ll_show_all(struct node *head)
{
	printf("\nMemory dump:\n");
	printf("--------------------------------------------------\n");
	while(head){
		printf("%s:\taddress 0x%0x\tdata 0x%0x\n",
			(head->ls == MEM_LOAD ? "Load memory" : "Store memory"),
			head->addr, head->data);
		head = head->next;
	}
	printf("--------------------------------------------------\n");
}

void ll_free_node(struct node **head, uint32_t addr)
{
	struct node **p = head;
	
	if(!p) return;
	
	while(*p){
		if((*p)->addr == addr){
			free(*p);
			(*p) = (*p)->next;
		}else{
			p = &((*p)->next);
		}
	}

}

void ll_free_all_node(struct node *head)
{
	struct node *rm;

	if(!head) return;
	
	while(head){
		rm = head;
		head = head->next;
		free(rm);
	}
}

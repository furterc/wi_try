#include "linked_list.h"

#include <stdlib.h>

void linked_list_Init(sLinkedList_t *instance, uint8_t limit)
{
	instance->entries = 0;
	instance->entryCount = 0;
    instance->maxEntries = limit;
}

HAL_StatusTypeDef linked_list_Append(sLinkedList_t *instance, void *data, uint8_t len)
{
	if(instance->entryCount == instance->maxEntries)
		return HAL_ERROR;

	sLinkedListObj_t *child = 0;

	if(instance->entries)
	{

		child = instance->entries;
		sLinkedListObj_t *prevChild = 0;
		while(child)
		{
			prevChild = child;
			child = child->next;
		}

		child = (sLinkedListObj_t*)malloc(sizeof(sLinkedListObj_t));
		prevChild->next = child;
	} else
	{
		instance->entries = (sLinkedListObj_t*)malloc(sizeof(sLinkedListObj_t));
		child = instance->entries;
	}

	if(child)
	{
		instance->entryCount++;
		child->data = data;
		child->len = len;
		child->next = 0;
	}

	return HAL_OK;
}

int linked_list_Pop(sLinkedList_t *instance, void **data)
{
	int len = -1;
	if(instance->entries)
	{
		sLinkedListObj_t *prev = instance->entries;
		instance->entries = prev->next;
		instance->entryCount--;

		*data = prev->data;
		len =  prev->len;

		free(prev);
	}

	return len;
}



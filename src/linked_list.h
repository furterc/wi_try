#ifndef KSES_UTILITIES_LINKED_LIST_H_
#define KSES_UTILITIES_LINKED_LIST_H_

#include <stdint.h>
#include <stm32f4xx_hal.h>


#ifdef __cplusplus
extern "C" {
#endif

typedef struct{
    void *data;
    uint8_t len;
    void *next;
} sLinkedListObj_t;

typedef struct{
    uint8_t maxEntries;
    sLinkedListObj_t *entries;
    uint8_t entryCount;
} sLinkedList_t;

void linked_list_Init(sLinkedList_t *instance, uint8_t limit);
HAL_StatusTypeDef linked_list_Append(sLinkedList_t *instance, void *data, uint8_t len);
int linked_list_Pop(sLinkedList_t *instance, void **data);

#ifdef __cplusplus
}
#endif

#endif /* KSES_UTILITIES_LINKED_LIST_H_ */

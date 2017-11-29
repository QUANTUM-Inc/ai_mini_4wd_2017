/*
 * memory_manager.h
 *
 * Created: 2017/11/20 10:51:34
 *  Author: aks
 */ 

#include <stdint.h>

#ifndef MEMORY_MANAGER_H_
#define MEmORY_MANAGER_H_

void *aiMini4WdAllocateMemory(size_t size);
void aiMini4WdDestroyMemory(void *ptr, size_t size);

#endif/*MEmORY_MANAGER_H_*/
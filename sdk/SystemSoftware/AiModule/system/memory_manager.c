/*
 * Copyright (c) 2017 Quantum.inc. All rights reserved.
 * This software is released under the MIT License, see LICENSE.txt.
 */

#include <stdint.h>
#include <string.h>

#include <xmegaA4U_utils.h>

#include <error.h>

#include <memory_manager.h>
#include <system/memory_manager_system.h>

#include <system.h>


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
#define MEM_BLOCK_SIZE			(5L * 1024L)
#define MEM_BLOCK_UNIT_SIZE		(8L)
#define MEM_BLOCK_NUM			(MEM_BLOCK_SIZE / MEM_BLOCK_UNIT_SIZE)

typedef union MemoryBlock_t
{
	struct {
		union MemoryBlock_t *next;
		union MemoryBlock_t *prev;
	} control;
	uint8_t array[MEM_BLOCK_UNIT_SIZE];
} MemoryBlock;


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
static uint8_t _next_block_is_continuous(MemoryBlock *ptr);
static uint8_t _is_in_memory_block(MemoryBlock *ptr);
static void _link_memory_block(MemoryBlock *ptr, size_t n_blocks);

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
static MemoryBlock *sFreeHead = NULL;
static MemoryBlock __attribute__ ((aligned(16))) sMemoryBlockPool[MEM_BLOCK_NUM];

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
int8_t aiMini4WdSystemInitMemoryBlock(void)
{	
	//J Head��ݒ�
	sFreeHead = &(sMemoryBlockPool[0]);

	_link_memory_block(sMemoryBlockPool, MEM_BLOCK_NUM);

	return 0;
}

/*---------------------------------------------------------------------------*/
void *aiMini4WdAllocateMemory(size_t size)
{
	if (sFreeHead == NULL) {
		return NULL;
	}
	
	//J �T�C�Y���u���b�N�T�C�Y�ɐ��K�����܂�
	size = (size + (MEM_BLOCK_UNIT_SIZE-1)) & ~(MEM_BLOCK_UNIT_SIZE-1);

	//J Head����H���ĘA�������u���b�N��size�����邩�T��
	MemoryBlock *ptr = sFreeHead;
	MemoryBlock *memblock = sFreeHead;
	uint32_t continuous_size = 0;
	while (ptr != NULL) {
		if (_next_block_is_continuous(ptr)) {
			continuous_size += sizeof (MemoryBlock);

			if (continuous_size >= size) {
				//J �����N���X�g����Amemblock ���� ptr �܂ł͈̔͂�؂���
				if (memblock == sFreeHead) {
					sFreeHead = ptr->control.next;
					sFreeHead->control.prev = NULL;
				}
				else {
					memblock->control.prev->control.next = ptr->control.next;
					ptr->control.next->control.prev = memblock->control.prev;
				}
				
				return (void *)memblock;
			}
		}
		else {
			//J ���̃u���b�N���w�b�h�Ƃ��ĕK�v�ȃT�C�Y�������邩���m�F
			memblock = ptr;
			continuous_size = sizeof (MemoryBlock);
		}
		
		//J ���ɑ���
		ptr = ptr->control.next;
	}		

	return NULL;
}

/*---------------------------------------------------------------------------*/
void aiMini4WdDestroyMemory(void *ptr, size_t size)
{
	if (sFreeHead == NULL) {
		return;
	}
	else if (ptr == NULL) {
		return;
	}

	//J �T�C�Y���u���b�N�T�C�Y�ɐ��K�����܂�
	size = (size + (MEM_BLOCK_UNIT_SIZE-1)) & ~(MEM_BLOCK_UNIT_SIZE-1);

	MemoryBlock *head = (MemoryBlock *)ptr;
	MemoryBlock *tail = (MemoryBlock *) (((uint16_t)ptr + (uint16_t)size - 1) & ~(uint16_t)(MEM_BLOCK_UNIT_SIZE-1));

	//J �Ԃ��ꂽ���������v�[���O
	if (!_is_in_memory_block(head) || !_is_in_memory_block(tail)) {
		return;
	}

	//J �Ԃ��Ă�������������������N�Ōq��
	_link_memory_block(head, size/sizeof(MemoryBlock));

	//J �Ԃ��ꂽptr �̕ԋp�ʒu��T���ĕԋp����
	// sFreeHead����H��
	MemoryBlock *free_head = sFreeHead;
	
	//J Head����O�̃u���b�N�Ȃ�A�擪��Head�Ƃ��Čq��
	if ((uint16_t)free_head > (uint16_t)head) {
		sFreeHead = head;
		tail->control.next = free_head;
		free_head->control.prev = tail;
	}
	//J free_head���珇�ɒH���āA�Ȃ��ׂ������Ɍq��
	else {
		MemoryBlock *op = sFreeHead;
		while (op->control.next != NULL) {
			
			if ((uint16_t)op > (uint16_t)head) {
				head->control.prev = op->control.prev;
				tail->control.next = op;
				
				op->control.prev->control.next = head;
				op->control.prev = tail;
				
				return;
			}

			op = op->control.next;
		}

		//J ��Ԍ��Ɍq���K�v������
		if (op->control.next == NULL) {
			op->control.next = head;
			head->control.prev = op;
		}
	}
	
	return;
}


/*---------------------------------------------------------------------------*/
uint16_t aiMini4WdDebugSystemMemoryMap(void)
{
	uint16_t free_size = 0;

	aiMini4WdSystemLog("------------\r\n");
	aiMini4WdSystemLog("Dump Free Blocks\r\n");

	
	//J Head����H���ĘA�������u���b�N��\������
	MemoryBlock *tail = sFreeHead;
	MemoryBlock *head = sFreeHead;
	MemoryBlock *ptr  = sFreeHead;
	uint16_t continuous_size = 0;
	while (ptr != NULL) {
		if (!_next_block_is_continuous(ptr)) {
			aiMini4WdSystemLog("  Free Blocs: 0x%04x to 0x%04x (Size = %u)\r\n", (uint16_t)head, (uint16_t)ptr->control.prev, continuous_size);
			free_size += continuous_size;
			
			continuous_size = 0;
			
			head = ptr;
		}

		//J ���ɑ���
		tail = ptr;
		ptr  = ptr->control.next;
		continuous_size += sizeof (MemoryBlock);
	}

	aiMini4WdSystemLog("  Free Blocs: 0x%04x to 0x%04x (Size = %u)\r\n", (uint16_t)head, (uint16_t)tail, continuous_size);
	free_size += continuous_size;

	aiMini4WdSystemLog("------------\r\n");


	return free_size;
}


/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
static uint8_t _next_block_is_continuous(MemoryBlock *ptr)
{
	//J ���X�g�w�b�h�̏ꍇ�ɂ́A�����Ƃ��ďI��点��
	if (ptr->control.prev == NULL) {
		return 1;
	}
	//J 1�O�̃u���b�N�ƃA�h���X�������ɂȂ��Ă���
	else if ((uint16_t)ptr == ((uint16_t)ptr->control.prev + sizeof(MemoryBlock))) {
		return 1;
	}

	return 0;
}

/*---------------------------------------------------------------------------*/
static uint8_t _is_in_memory_block(MemoryBlock *ptr)
{
	uint16_t i = 0;
	for(i=0 ; i<MEM_BLOCK_NUM ; ++i) {
		if (&sMemoryBlockPool[i] == ptr) {
			return 1;
		}
	}

	return 0;
}

static void _link_memory_block(MemoryBlock *ptr, size_t n_blocks)
{
	memset (ptr, 0x00, sizeof(MemoryBlock) * n_blocks);
	ptr[0].control.prev = NULL;

	uint16_t i=0;
	for (i=0 ; i<n_blocks-1 ; ++i) {
		ptr[i].control.next = &ptr[i+1];
		ptr[i+1].control.prev = &ptr[i];
	}

	ptr[n_blocks-1].control.next = NULL;

	return;
}

#pragma once

typedef struct
{
	u8 *Memory;
	u64 BlockSize;
	u64 BytesAllocated;
} memory_block;

#ifdef __linux__
#define _GNU_SOURCE
#include <sys/mman.h>
#endif

internal memory_block
CreateLinearAllocator(u64 MemoryBlockSize)
{
	memory_block Result = { 0 };
	Result.BlockSize = MemoryBlockSize;
#ifdef __linux__
	Result.Memory = mmap(0, Result.BlockSize, PROT_READ|PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);;
#endif
	return (Result);
}

internal void *
AllocateMemory(memory_block *Memory, u64 BytesToAllocate)
{
	Assert(Memory->BlockSize > Memory->BytesAllocated + BytesToAllocate);
	void *MemoryResult = Memory->Memory + Memory->BytesAllocated;
	Memory->BytesAllocated += BytesToAllocate;
	return (MemoryResult);
}

internal void
FreeMemoryBlock(memory_block *Memory)
{
#ifdef __linux__
	munmap(Memory->Memory, Memory->BlockSize);
#endif
	*Memory = (memory_block) { 0 };
}

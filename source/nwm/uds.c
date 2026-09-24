#include <3ds/types.h>
#include <3ds/ipc.h>
#include <3ds/result.h>
#include <3ds/svc.h>
#include <3ds/srv.h>
#include <3ds/synchronization.h>

#include "../../include/nwm/uds.h"



Handle nwmUdsRefCount;
static int nwmUdsRefCount = {0};

Result nwmUdsInit(void)
{
	Result res=0;
	if (AtomicPostIncrement(&nwmUdsRefCount)) return 0;
	res = srvGetServiceHandle(&nwmUdsHandle, "nwm::UDS");
	if (R_FAILED(res)) AtomicDecrement(&nwmUdsRefCount);
	return res;
}

void nwmUdsExit(void)
{
	if (AtomicDecrement(&nwmUdsRefCount)) return;
	svcCloseHandle(nwmUdsHandle);
}

static Result NwmUDS_InitializeWithVersion(nwmUdsHandle *nodeinfo, Handle sharedmem_handle, u32 sharedmem_size)
{
	u32* cmdbuf = getThreadCommandBuffer();

	cmdbuf[0] = 0x001B0302;
	cmdbuf[1] = sharedmem_size;
	memcpy(&cmdbuf[2], nodeinfo, sizeof(nwmUdsHandle));
	cmdbuf[12] = 0x400;//version
	cmdbuf[13] = 0;
	cmdbuf[14] = sharedmem_handle;

	Result ret = 0;
	if((ret = svcSendSyncRequest(nwmUdsHandle)))return ret;
	ret = cmdbuf[1];

	return ret;
}

static Result NwmUDS_Bind(u32 BindNodeID, u32 input0, u8 data_channel, u16 NetworkNodeID)
{
	u32* cmdbuf = getThreadCommandBuffer();

	cmdbuf[0] = 0x120100;
	cmdbuf[1] = BindNodeID;
	cmdbuf[2] = input0;
	cmdbuf[3] = data_channel;
	cmdbuf[4] = NetworkNodeID;

	Result ret=0;
	if((ret = svcSendSyncRequest(nwmUdsHandle)))return ret;
	ret = cmdbuf[1];

	return ret;
}

static Result NwmUDS_Unbind(u32 BindNodeID)
{
	u32* cmdbuf = getThreadCommandBuffer();

	cmdbuf[0] = 0x130040;
	cmdbuf[1] = BindNodeID;

	Result ret = 0;
	if((ret = svcSendSyncRequest(nwmUdsHandle)))return ret;

	return cmdbuf[1];
}

static Result NwmUDS_Shutdown()
{
	u32* cmdbuf = getThreadCommandBuffer();

	cmdbuf[0] = 0x30000;

	Result ret = 0;
	if((ret = svcSendSyncRequest(nwmUdsHandle)))return ret;

	return cmdbuf[1];
}

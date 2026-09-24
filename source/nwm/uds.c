static Result UDS_InitializeWithVersion(Handle* handle, udsNodeInfo *nodeinfo, Handle sharedmem_handle, u32 sharedmem_size)
{
	u32* cmdbuf = getThreadCommandBuffer();

	cmdbuf[0] = 0x001B0302;
	cmdbuf[1] = sharedmem_size;
	memcpy(&cmdbuf[2], nodeinfo, sizeof(udsNodeInfo));
	cmdbuf[12] = 0x400;//version
	cmdbuf[13] = 0;
	cmdbuf[14] = sharedmem_handle;

	Result ret = 0;
	if((ret = svcSendSyncRequest(*handle)))return ret;
	ret = cmdbuf[1];

	return ret;
}

static Result UDS_Bind(Handle* handle, u32 BindNodeID, u32 input0, u8 data_channel, u16 NetworkNodeID)
{
	u32* cmdbuf = getThreadCommandBuffer();

	cmdbuf[0] = 0x120100;
	cmdbuf[1] = BindNodeID;
	cmdbuf[2] = input0;
	cmdbuf[3] = data_channel;
	cmdbuf[4] = NetworkNodeID;

	Result ret=0;
	if((ret = svcSendSyncRequest(*handle)))return ret;
	ret = cmdbuf[1];

	return ret;
}

static Result UDS_Unbind(Handle* handle, u32 BindNodeID)
{
	u32* cmdbuf = getThreadCommandBuffer();

	cmdbuf[0] = 0x130040;
	cmdbuf[1] = BindNodeID;

	Result ret = 0;
	if((ret = svcSendSyncRequest(*handle)))return ret;

	return cmdbuf[1];
}

static Result UDS_Shutdown(Handle* handle)
{
	u32* cmdbuf = getThreadCommandBuffer();

	cmdbuf[0] = 0x30000;

	Result ret = 0;
	if((ret = svcSendSyncRequest(*handle)))return ret;

	return cmdbuf[1];
}

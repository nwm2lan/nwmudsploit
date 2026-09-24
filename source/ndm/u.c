
Result NDM_EnterExclusiveState(Handle* handle, u32 state)
{
	u32* cmdbuf = getThreadCommandBuffer();

	cmdbuf[0] = 0x10042;
	cmdbuf[1] = state;
	cmdbuf[2] = 0x20;

	Result ret = 0;
	if((ret = svcSendSyncRequest(*handle)))return ret;

	return cmdbuf[1];
}

Result NDM_LeaveExclusiveState(Handle* handle)
{
	u32* cmdbuf = getThreadCommandBuffer();

	cmdbuf[0] = 0x20002;
	cmdbuf[1] = 0x20;

	Result ret = 0;
	if((ret = svcSendSyncRequest(*handle)))return ret;

	return cmdbuf[1];
}

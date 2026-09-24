/**

*/

static Result UDS_InitializeWithVersion(Handle* handle, udsNodeInfo *nodeinfo, Handle sharedmem_handle, u32 sharedmem_size);

static Result UDS_Bind(Handle* handle, u32 BindNodeID, u32 input0, u8 data_channel, u16 NetworkNodeID);

static Result UDS_Unbind(Handle* handle, u32 BindNodeID);

static Result UDS_Shutdown(Handle* handle);

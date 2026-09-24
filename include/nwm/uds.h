/**
 * @file nwm.h
 * @brief NWmUDS service. https://3dbrew.org/wiki/NWM_Services
 */
#pragma once

typedef Handle uwmNodeInfo;

/// Initializes nwmUds.
Result nwmUdsInit(void);

/// Exits nwmUds.
void nwmUdsExit(void);

Result NwmUDS_InitializeWithVersion(uwmNodeInfo *nodeinfo, Handle sharedmem_handle, u32 sharedmem_size);

Result NwmUDS_Bind(u32 BindNodeID, u32 input0, u8 data_channel, u16 NetworkNodeID);

Result NwmUDS_Unbind(u32 BindNodeID);

Result NwmUDS_Shutdown();

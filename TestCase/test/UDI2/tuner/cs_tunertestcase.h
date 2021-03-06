#ifndef _CS_TUNER_TEST_CASE_H_
#define _CS_TUNER_TEST_CASE_H_

#include "cs_testkit.h"

#ifdef __cplusplus
extern "C" {
#endif

CSUDI_BOOL CSTC_TUNER_Init(void);
CSUDI_BOOL CSTC_TUNER_UnInit(void);

CSUDI_BOOL CSTC_TUNER_IT_GetAllDeviceId_0001(void);
CSUDI_BOOL CSTC_TUNER_IT_GetAllDeviceId_0002(void);
CSUDI_BOOL CSTC_TUNER_IT_GetAllDeviceId_0003(void);
CSUDI_BOOL CSTC_TUNER_IT_GetAllDeviceId_0004(void);
CSUDI_BOOL CSTC_TUNER_IT_GetAllDeviceId_0005(void);
CSUDI_BOOL CSTC_TUNER_IT_GetDeviceInfo_0001( void );
CSUDI_BOOL CSTC_TUNER_IT_GetDeviceInfo_0002( void );
CSUDI_BOOL CSTC_TUNER_IT_GetDeviceInfo_0003( void );
CSUDI_BOOL CSTC_TUNER_IT_AddCallback_0001( void );
CSUDI_BOOL CSTC_TUNER_IT_AddCallback_0002( void );
CSUDI_BOOL CSTC_TUNER_IT_AddCallback_0003( void );
CSUDI_BOOL CSTC_TUNER_IT_AddCallback_0004( void );
CSUDI_BOOL CSTC_TUNER_IT_AddCallback_0005( void );
CSUDI_BOOL CSTC_TUNER_IT_AddCallback_0006( void );
CSUDI_BOOL CSTC_TUNER_IT_AddCallback_0007( void );
CSUDI_BOOL CSTC_TUNER_IT_AddCallback_0008( void );
CSUDI_BOOL CSTC_TUNER_IT_AddCallback_0009( void );
CSUDI_BOOL CSTC_TUNER_IT_DelCallback_0001( void );
CSUDI_BOOL CSTC_TUNER_IT_DelCallback_0002( void );
CSUDI_BOOL CSTC_TUNER_IT_DelCallback_0003( void );
CSUDI_BOOL CSTC_TUNER_IT_DelCallback_0004( void );
CSUDI_BOOL CSTC_TUNER_IT_DelCallback_0005( void );
CSUDI_BOOL CSTC_TUNER_IT_AddDelCallback_0001( void );
CSUDI_BOOL CSTC_TUNER_IT_AddDelCallback_0002( void );
CSUDI_BOOL CSTC_TUNER_IT_Connect_0001( void );
CSUDI_BOOL CSTC_TUNER_IT_Connect_0002( void );
CSUDI_BOOL CSTC_TUNER_IT_Connect_0003( void );
CSUDI_BOOL CSTC_TUNER_IT_Connect_0004( void );
CSUDI_BOOL CSTC_TUNER_IT_Connect_0005( void );
CSUDI_BOOL CSTC_TUNER_IT_Connect_0006( void );
CSUDI_BOOL CSTC_TUNER_IT_Connect_0007( void );
CSUDI_BOOL CSTC_TUNER_IT_Connect_0008( void );
CSUDI_BOOL CSTC_TUNER_IT_Connect_0009( void );
CSUDI_BOOL CSTC_TUNER_IT_Connect_0010( void );
CSUDI_BOOL CSTC_TUNER_IT_Connect_0011( void );
CSUDI_BOOL CSTC_TUNER_IT_Connect_0012( void );
CSUDI_BOOL CSTC_TUNER_IT_Connect_0013( void );
CSUDI_BOOL CSTC_TUNER_IT_Connect_0014( void );
CSUDI_BOOL CSTC_TUNER_IT_Connect_0015( void );
CSUDI_BOOL CSTC_TUNER_IT_Connect_0016( void );
CSUDI_BOOL CSTC_TUNER_IT_Connect_0017( void );
CSUDI_BOOL CSTC_TUNER_IT_Connect_0018( void );
CSUDI_BOOL CSTC_TUNER_IT_Connect_0019( void );
CSUDI_BOOL CSTC_TUNER_IT_Connect_0020( void );
CSUDI_BOOL CSTC_TUNER_IT_GetSignalInfo_0001( void );
CSUDI_BOOL CSTC_TUNER_IT_GetSignalInfo_0002( void );
CSUDI_BOOL CSTC_TUNER_IT_GetSignalInfo_0003( void );
CSUDI_BOOL CSTC_TUNER_IT_GetSignalInfo_0004( void );
CSUDI_BOOL CSTC_TUNER_IT_GetSignalInfo_0005( void );
CSUDI_BOOL CSTC_TUNER_IT_GetSignalInfo_0006( void );
CSUDI_BOOL CSTC_TUNER_IT_GetSignalInfo_0007( void );
CSUDI_BOOL CSTC_TUNER_IT_GetSignalInfo_0008( void );
CSUDI_BOOL CSTC_TUNER_IT_AddPID_0001( void );
CSUDI_BOOL CSTC_TUNER_IT_AddPID_0002( void );
CSUDI_BOOL CSTC_TUNER_IT_AddPID_0003( void );
CSUDI_BOOL CSTC_TUNER_IT_RemovePID_0001( void );
CSUDI_BOOL CSTC_TUNER_IT_AddRemovePID_0001( void );
CSUDI_BOOL CSTC_TUNER_IT_RDIAddCallback_0001( void );
CSUDI_BOOL CSTC_TUNER_IT_RDIAddCallback_0002( void );
CSUDI_BOOL CSTC_TUNER_IT_RDIAddCallback_0003( void );
CSUDI_BOOL CSTC_TUNER_IT_RDIRemoveCallback_0001( void );
CSUDI_BOOL CSTC_TUNER_IT_RDIRemoveCallback_0002( void );
CSUDI_BOOL CSTC_TUNER_IT_RDIRemoveCallback_0003( void );
CSUDI_BOOL CSTC_TUNER_IT_RDIRemoveCallback_0004( void );
CSUDI_BOOL CSTC_TUNER_IT_RDI_0001( void );
CSUDI_BOOL CSTC_TUNER_IT_RDI_0002( void );
CSUDI_BOOL CSTC_TUNER_IT_RDI_0003( void );
CSUDI_BOOL CSTC_TUNER_IT_UnifiedFreqTrans_0001(void);
CSUDI_BOOL CSTC_TUNER_IT_QamConnectTime_0001(void);
CSUDI_BOOL CSTC_TUNER_IT_QamConnectTime_0002(void);
CSUDI_BOOL CSTC_TUNER_IT_QamConnectTime_0003(void);
CSUDI_BOOL CSTC_TUNER_IT_QamConnectTime_0004(void);
CSUDI_BOOL CSTC_TUNER_IT_QamConnectTime_0005(void);
CSUDI_BOOL CSTC_TUNER_IT_QamConnectTime_0006(void);
CSUDI_BOOL CSTC_TUNER_IT_QamConnectTime_0007(void);

#ifdef __cplusplus
}
#endif

#endif



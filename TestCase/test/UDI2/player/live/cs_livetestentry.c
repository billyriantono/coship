#include "cs_testkit.h"
#include "cs_livetestentry.h"
#include "cs_livetestcase.h"

/*begin change*/
static CSTestInit_S s_sTestInit = {CSTC_LIVE_Init, CSTC_LIVE_UnInit};
static CSTestCase_S s_asTestCase[] = 
{
	{CS_TK_AUTOCASE(CSTC_LIVE_TEST_IT_Add_DelPlayerCallback_0001)},
	{CS_TK_AUTOCASE(CSTC_LIVE_TEST_IT_Add_DelPlayerCallback_0002)},
	{CS_TK_AUTOCASE(CSTC_LIVE_TEST_IT_Add_DelPlayerCallback_0003)},
	{CS_TK_AUTOCASE(CSTC_LIVE_TEST_IT_Add_DelPlayerCallback_0004)},

	{CS_TK_AUTOCASE(CSTC_LIVE_IT_CSUDIPLAYEROpenClose_0001)},
	{CS_TK_AUTOCASE(CSTC_LIVE_IT_CSUDIPLAYEROpenClose_0002)},
	{CS_TK_AUTOCASE(CSTC_LIVE_IT_CSUDIPLAYEROpenClose_0003)},
	{CS_TK_AUTOCASE(CSTC_LIVE_IT_CSUDIPLAYEROpenClose_0004)},
	{CS_TK_AUTOCASE(CSTC_LIVE_IT_CSUDIPLAYEROpenClose_0005)},
	{CS_TK_AUTOCASE(CSTC_LIVE_IT_CSUDIPLAYEROpenClose_0006)},
	{CS_TK_AUTOCASE(CSTC_LIVE_IT_CSUDIPLAYEROpenClose_0007)},
	{CS_TK_AUTOCASE(CSTC_LIVE_IT_CSUDIPLAYEROpenClose_0008)},

	{CS_TK_AUTOCASE(CSTC_LIVE_TEST_IT_SetGetStream_0001)},
	{CS_TK_AUTOCASE(CSTC_LIVE_TEST_IT_SetGetStream_0002)},
	{CS_TK_AUTOCASE(CSTC_LIVE_TEST_IT_SetGetStream_0003)},
	{CS_TK_AUTOCASE(CSTC_LIVE_TEST_IT_SetGetStream_0004)},
	{CS_TK_AUTOCASE(CSTC_LIVE_TEST_IT_SetGetStream_0005)},
	{CS_TK_AUTOCASE(CSTC_LIVE_TEST_IT_SetGetStream_0006)},
	{CS_TK_AUTOCASE(CSTC_LIVE_TEST_IT_SetGetStream_0007)},
	{CS_TK_AUTOCASE(CSTC_LIVE_TEST_IT_SetGetStream_0008)},
	{CS_TK_AUTOCASE(CSTC_LIVE_TEST_IT_SetGetStream_0009)},

	{CS_TK_AUTOCASE(CSTC_LIVE_TEST_IT_GetPlaybackParam_0001)},

	{CS_TK_AUTOCASE(CSTC_LIVE_TEST_IT_ModifyStream_0001)},
	{CS_TK_AUTOCASE(CSTC_LIVE_TEST_IT_ModifyStream_0002)},
	{CS_TK_AUTOCASE(CSTC_LIVE_TEST_IT_ModifyStream_0003)},
	{CS_TK_AUTOCASE(CSTC_LIVE_TEST_IT_ModifyStream_0004)},
	{CS_TK_AUTOCASE(CSTC_LIVE_TEST_IT_ModifyStream_0005)},
	{CS_TK_AUTOCASE(CSTC_LIVE_TEST_IT_ModifyStream_0006)},

	{CS_TK_AUTOCASE(CSTC_LIVE_TEST_IT_StartStop_0001)},
	{CS_TK_AUTOCASE(CSTC_LIVE_TEST_IT_StartStop_0002)},
	{CS_TK_AUTOCASE(CSTC_LIVE_TEST_IT_StartStop_0003)},

	{CS_TK_AUTOCASE(CSTC_LIVE_TEST_IT_PauseResume_0001)},
	{CS_TK_AUTOCASE(CSTC_LIVE_TEST_IT_PauseResume_0002)},

	{CS_TK_AUTOCASE(CSTC_LIVE_TEST_IT_SetSpeed_0001)},
	{CS_TK_AUTOCASE(CSTC_LIVE_TEST_IT_Seek_0001)},
	//{CS_TK_AUTOCASE(CSTC_LIVE_TEST_IT_GetCurPosInSec_0001)},

	{CS_TK_AUTOCASE(CSTC_LIVE_TEST_IT_GetCurPTS_0001)},
	{CS_TK_AUTOCASE(CSTC_LIVE_TEST_IT_GetCurPTS_0002)},
	{CS_TK_AUTOCASE(CSTC_LIVE_TEST_IT_GetCurPTS_0003)},
	{CS_TK_AUTOCASE(CSTC_LIVE_TEST_IT_GetCurPTS_0004)},

	{CS_TK_AUTOCASE(CSTC_LIVE_TEST_IT_GetPacketCount_0001)},
	{CS_TK_AUTOCASE(CSTC_LIVE_TEST_IT_GetPacketCount_0002)},
	{CS_TK_AUTOCASE(CSTC_LIVE_TEST_IT_GetPacketCount_0003)},
	{CS_TK_AUTOCASE(CSTC_LIVE_TEST_IT_GetPacketCount_0004)},
	{CS_TK_MANUALCASE(CSTC_LIVE_TEST_IT_GetPacketCount_0005)},
	{CS_TK_MANUALCASE(CSTC_LIVE_TEST_IT_GetPacketCount_0006)},
	{CS_TK_MANUALCASE(CSTC_LIVE_TEST_IT_GetPacketCount_0007)},
#if 0
	{CS_TK_AUTOCASE(CSTC_LIVE_TEST_IT_GetTrickMode_0001)},
	{CS_TK_AUTOCASE(CSTC_LIVE_TEST_IT_GetTrickMode_0002)},
	{CS_TK_AUTOCASE(CSTC_LIVE_TEST_IT_GetTrickMode_0003)},
	{CS_TK_AUTOCASE(CSTC_LIVE_TEST_IT_GetTrickMode_0004)},
	{CS_TK_AUTOCASE(CSTC_LIVE_TEST_IT_GetTrickMode_0005)},
	{CS_TK_AUTOCASE(CSTC_LIVE_TEST_IT_GetTrickMode_0006)},	
#endif
	{CS_TK_AUTOCASE(CSTC_LIVE_TEST_IT_Clear_0001)},

	{CS_TK_MANUALCASE(CSTC_LIVE_TEST_IT_LIVE_0001)},
	{CS_TK_MANUALCASE(CSTC_LIVE_TEST_IT_LIVE_0002)},
	{CS_TK_MANUALCASE(CSTC_LIVE_TEST_IT_LIVE_0003)},
	{CS_TK_MANUALCASE(CSTC_LIVE_TEST_IT_LIVE_0004)},
	{CS_TK_MANUALCASE(CSTC_LIVE_TEST_IT_LIVE_0005)},
	{CS_TK_MANUALCASE(CSTC_LIVE_TEST_IT_LIVE_0006)},
	{CS_TK_MANUALCASE(CSTC_LIVE_TEST_IT_LIVE_0007)},
	{CS_TK_MANUALCASE(CSTC_LIVE_TEST_IT_LIVE_0008)},
	{CS_TK_MANUALCASE(CSTC_LIVE_TEST_IT_LIVE_0009)},
	{CS_TK_MANUALCASE(CSTC_LIVE_TEST_IT_LIVE_0010)},
	{CS_TK_MANUALCASE(CSTC_LIVE_TEST_IT_LIVE_0011)},
	{CS_TK_MANUALCASE(CSTC_LIVE_TEST_IT_LIVE_0012)},
	{CS_TK_MANUALCASE(CSTC_LIVE_TEST_IT_LIVE_0013)},
	{CS_TK_MANUALCASE(CSTC_LIVE_TEST_IT_LIVE_0014)},
	{CS_TK_MANUALCASE(CSTC_LIVE_TEST_IT_LIVE_0015)},
	{CS_TK_MANUALCASE(CSTC_LIVE_TEST_IT_LIVE_0016)},
	{CS_TK_MANUALCASE(CSTC_LIVE_TEST_IT_LIVE_0017)},
	{CS_TK_MANUALCASE(CSTC_LIVE_TEST_IT_LIVE_0018)},
	{CS_TK_MANUALCASE(CSTC_LIVE_TEST_IT_LIVE_0019)},
	{CS_TK_MANUALCASE(CSTC_LIVE_TEST_IT_LIVE_0020)},
	{CS_TK_MANUALCASE(CSTC_LIVE_TEST_IT_LIVE_0021)},
	{CS_TK_AUTOCASE(CSTC_LIVE_TEST_IT_LIVE_0022)},
	{CS_TK_MANUALCASE(CSTC_LIVE_TEST_IT_LIVE_0023)},
	{CS_TK_AUTOCASE(CSTC_LIVE_TEST_IT_LIVE_0024)},
	{CS_TK_AUTOCASE(CSTC_LIVE_TEST_IT_LIVE_0025)},
	{CS_TK_AUTOCASE(CSTC_LIVE_TEST_IT_LIVE_0026)},
	{CS_TK_MANUALCASE(CSTC_LIVE_TEST_IT_LIVE_0027)},

	{CS_TK_MANUALCASE(CSTC_LIVE_TEST_IT_SWITCH_0001)},
	{CS_TK_MANUALCASE(CSTC_LIVE_TEST_IT_SWITCH_0002)},
	{CS_TK_MANUALCASE(CSTC_LIVE_TEST_IT_SWITCH_0003)},
	{CS_TK_MANUALCASE(CSTC_LIVE_TEST_IT_SWITCH_0004)},
	{CS_TK_MANUALCASE(CSTC_LIVE_TEST_IT_SWITCH_0005)},
	{CS_TK_MANUALCASE(CSTC_LIVE_TEST_IT_SWITCH_0006)},
	{CS_TK_MANUALCASE(CSTC_LIVE_TEST_IT_SWITCH_0007)},
	//{CS_TK_MANUALCASE(CSTC_LIVE_TEST_IT_SWITCH_0008)},
	{CS_TK_MANUALCASE(CSTC_LIVE_TEST_IT_SWITCH_0009)},

	{CS_TK_MANUALCASE(CSTC_LIVE_TEST_IT_Program_0001)},
	{CS_TK_MANUALCASE(CSTC_LIVE_TEST_IT_Program_0002)},
	{CS_TK_MANUALCASE(CSTC_LIVE_TEST_IT_Program_0003)},
	CS_TK_ENDCASE //测试用例结束标记，必须要有，且在最后一个
};

static CSTestGetObj_F s_afnTestGetObj[]= 
{
	CSLIVETESTGetObj,	//对应自己，必须是第一个，必须要有
	//CSSUBTESTGetObj,	//下级测试用例目录入口，可以没有

};
/*end change*/

CS_TEST_BUILD_FRAME_CODE(LIVE)
/*end don't change*/


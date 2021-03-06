/* --------------------------------------------------------------------
注意：
1.在需要与用户交互的测试用例中，可以：
	a. 使用CSTKWaitAnyKey等待用户输入任意按键
	b. 使用CSTKWaitYes等待用户输入YES
2.测试用例函数命名：测试用例ID，"测试用例ID"定义在测试用例文档中
-----------------------------------------------------------------------*/
#include "cs_screentestcase.h"
#include "udi2_typedef.h"
#include "udi2_error.h"
#include "udi2_player.h"
#include "udi2_demux.h"
#include "udi2_descramble.h"
#include "udi2_video.h"
#include "udi2_os.h"
#include "udi2_screen.h"
#include "udi2_tuner.h"
#include "udiplus_debug.h"
#include "cs_testkit.h"
#include "../cs_udi2testcase.h"

static CSUDISCREENResolution_E g_scResolution[2];
	
CSUDI_BOOL CSTC_SCREEN_Init(void)
{
	//在本测试用例集执行前调用
	return CSUDI_TRUE;
}

CSUDI_BOOL CSTC_SCREEN_UnInit(void)
{
	//在本测试用例集执行后调用
	return CSUDI_TRUE;
}

//通过读取配置文件查询平台是否支持高清视频输出通道类型
static CSUDI_BOOL NotSupportHD( )
{
	CSUDI_BOOL bRet = CSUDI_FALSE;
	const char acSection[] = "SCREEN";
	char acKey[] = "EM_UDI_VOUT_DEVICE_HD";
	char acResult[255] = {0};
	int nResultSize = (int)sizeof(acResult);

	if (CS_TK_CONFIG_SUCCESS == CSTKGetConfigInfo(acSection, acKey, acResult, nResultSize))
	{
		if (0 == CSTKGetIntFromStr(acResult, 10))
		{
			bRet = CSUDI_TRUE;	// 表示不支持高清平台
		}
	}
	else
	{
		CSTCPrint("获取EM_UDI_VOUT_DEVICE_HD配置项信息失败!\n");
	}
	
	return bRet;
}

//通过配置文件查询平台是否高标清同源
static CSUDI_BOOL IsShareHDDisplay()
{
	CSUDI_BOOL bRet = CSUDI_FALSE;
	char cBuf[32] = {0};
	
	memset(cBuf, '\0', sizeof(cBuf));
	if (CS_TK_CONFIG_SUCCESS == CSTKGetConfigInfo("OSG", "CS_OSGUDI2_SHARE_HD_DISPLAY_SERFACE", cBuf, sizeof(cBuf)))
	{
		if (1 == CSTKGetIntFromStr(cBuf,10))
		{
			bRet = CSUDI_TRUE;
		}
	}
	else
	{
		CSTCPrint("获取CS_OSGUDI2_SHARE_HD_DISPLAY_SERFACE配置项信息失败!\n");
	}

	return bRet;
}

//寻找视频解码器
static int FindVideoDecoder(int nVideoDecCount, CSUDIVIDStreamType_E eFindVideoType)
{
	CSUDIVIDEOCapability_S sVideoCapabilityInfo;
	int nVideoID = -1;
	int i = 0;
	int k = 0;
	
	for (i=0; i<nVideoDecCount; i++)
	{
		if (CSASSERT_FAILED(CSUDI_SUCCESS == CSUDIVIDEOGetCapability(i, &sVideoCapabilityInfo)))
		{
			break;
		}

		for (k=0; k<EM_UDI_VID_STREAMTYPE_NUM; k++)
		{
			if (eFindVideoType == sVideoCapabilityInfo.m_eStreamType[k])
			{
				nVideoID = i;
				break;
			}
			if (EM_UDI_VID_STREAM_UNKNOWN == sVideoCapabilityInfo.m_eStreamType[k])
			{
				break;
			}
		}

		if (nVideoID != -1)
		{
			break;
		}
	}

	return nVideoID;
}


//寻找平台支持的分辨率
static CSUDI_BOOL IsScreenSupport(CSUDISCREENType_E * peScreenDevice, CSUDISCREENResolution_E eResolution)
{
	CSUDISCREENCapability_S sCapabilityInfo;
	CSUDI_BOOL bRet = CSUDI_FALSE;
	int k = 0;

	if (CSASSERT_FAILED(CSUDI_SUCCESS == CSUDISCREENGetCapability( *peScreenDevice, &sCapabilityInfo)))
	{
		return CSUDI_FALSE;
	}

	for (k=0; k<EM_UDISCREEN_RESOLUTION_NUM; k++)
	{
		if (eResolution == sCapabilityInfo.m_eResolution[k])
		{
			bRet = CSUDI_TRUE;
			break;
		}
		if (EM_UDISCREEN_RESOLUTION_INVALID == sCapabilityInfo.m_eResolution[k])
		{
			break;
		}
	}

	return bRet;   
}


// 播放标清视频节目源
static CSUDI_HANDLE PlaySDProgram(CSUDIVIDStreamType_E eVideoType, CSUDISCREENResolution_E eResolution)
{
	CSUDI_HANDLE hPlayer = CSUDI_NULL;
	CSUDIDEMUXCapability_S  sDemuxCapabilityInfo;
	CSUDIPlayerChnl_S sPlayerChnl;
	CSUDIStreamInfo_S sStreamInfo;
	int nVideoDecCount = 0;
	int nVideoIndex = -1;
	int nDemuxCount = 0 ; 
	int nDemuxIndex = -1;
	int i = 0;

	if (CSUDI_SUCCESS == CSUDIVIDEOGetCount(&nVideoDecCount))
	{
		if (nVideoDecCount > 0)
		{
			nVideoIndex = FindVideoDecoder(nVideoDecCount, eVideoType);
		}
	}

	if (CSUDI_SUCCESS == CSUDIDEMUXGetCount(&nDemuxCount))
	{
		if (nDemuxCount > 0)
		{
			for(i=0; i<nDemuxCount; i++)
			{
				if (CSUDI_SUCCESS == CSUDIDEMUXGetCapability(i, &sDemuxCapabilityInfo))
				{
					if (EM_UDI_DEMUX_PLAY == (sDemuxCapabilityInfo.m_dwWorkTypeMask&EM_UDI_DEMUX_PLAY))  
					{
						nDemuxIndex = i;
						break;
					}
				}
			}
		}
	}

	if ((nVideoIndex==-1) || (nDemuxIndex==-1))
	{
		CSTCPrint("获取Video , demux错误\n");
		
		return CSUDI_NULL;
	}

	sPlayerChnl.m_nDemux = nDemuxIndex;
	sPlayerChnl.m_nVideoDecoder = nVideoIndex;
	sPlayerChnl.m_nAudioDecoder = 0;

	//锁测试码流频点
	if (CSTC_UDI2PortingLock(0, "Audio&Video Test_27Mbps_20070524.ts"))
	{
		CSUDIVIDEOSetStopMode(nVideoIndex, EM_UDIVIDEO_STOPMODE_BLACK);

		if (CSUDI_SUCCESS == CSUDIPLAYEROpen(&sPlayerChnl, EM_UDIPLAYER_LIVE, &hPlayer))
		{
			if (hPlayer != CSUDI_NULL)
			{
				if (EM_UDISCREEN_RESOLUTION_PAL == eResolution)
				{
					sStreamInfo.m_nPid = 1140;  //PAL节目源
					sStreamInfo.m_eContentType = EM_UDI_CONTENT_VIDEO;
					sStreamInfo.m_uStreamType.m_eVideoType = eVideoType;
				}

				if (EM_UDISCREEN_RESOLUTION_NTSC == eResolution)
				{
					sStreamInfo.m_nPid = 5137;  //NTSC节目源
					sStreamInfo.m_eContentType = EM_UDI_CONTENT_VIDEO;
					sStreamInfo.m_uStreamType.m_eVideoType = eVideoType;
				}

				if (CSASSERT_FAILED(CSUDI_SUCCESS == CSUDIPLAYERSetStream(hPlayer, &sStreamInfo, 1, CSUDI_NULL)))
				{
					CSTCPrint("Player set stream 失败\n");
					
					if (CSASSERT_FAILED(CSUDI_SUCCESS == CSUDIPLAYERClose(hPlayer)))
					{
						CSTCPrint("关闭播放器失败\n");
					}
					
					return CSUDI_NULL;
				}

				if (CSASSERT_FAILED(CSUDI_SUCCESS == CSUDIPLAYERStart(hPlayer)))
				{
					CSTCPrint("Player start 失败\n");
					
					if (CSASSERT_FAILED(CSUDI_SUCCESS == CSUDIPLAYERClose(hPlayer)))
					{
						CSTCPrint("关闭播放器失败\n");
					}
					
					return CSUDI_NULL;
				}

				if (CSASSERT_FAILED(CSUDI_SUCCESS == CSUDIVIDEOShow(0,TRUE)))
				{
					CSTCPrint("Video show 失败\n");
					
					if (CSASSERT_FAILED(CSUDI_SUCCESS == CSUDIPLAYERClose(hPlayer)))
					{
						CSTCPrint("关闭播放器失败\n");
					}
					
					return CSUDI_NULL;
				}
			}
		}
		else
		{
			CSTCPrint("Player open 失败\n");
		}
	}
	else
	{
		CSTCPrint("锁定Audio&Video Test_27Mbps_20070524.ts 失败\n");
	}

	return hPlayer;	
}

// 播放高清视频节目源
static CSUDI_HANDLE PlayHDProgram(CSUDIVIDStreamType_E eVideoType, CSUDISCREENResolution_E eResolution)
{
	CSUDI_HANDLE hPlayer = CSUDI_NULL;
	CSUDIDEMUXCapability_S  sDemuxCapabilityInfo;
	CSUDIPlayerChnl_S sPlayerChnl;
	CSUDIStreamInfo_S sStreamInfo;
	int nVideoDecCount = 0;
	int nVideoIndex = -1;
	int nDemuxCount = 0 ; 
	int nDemuxIndex = -1;
	int i = 0;
	
	if (CSUDI_SUCCESS == CSUDIVIDEOGetCount(&nVideoDecCount))
	{
		if (nVideoDecCount > 0)
		{
			nVideoIndex = FindVideoDecoder(nVideoDecCount, eVideoType);
		}
	}

	if (CSUDI_SUCCESS == CSUDIDEMUXGetCount(&nDemuxCount))
	{
		if (nDemuxCount > 0)
		{
			for(i=0; i<nDemuxCount; i++)
			{
				if (CSUDI_SUCCESS == CSUDIDEMUXGetCapability(i, &sDemuxCapabilityInfo))
				{
					if (EM_UDI_DEMUX_PLAY == (sDemuxCapabilityInfo.m_dwWorkTypeMask&EM_UDI_DEMUX_PLAY))  
					{
						nDemuxIndex = i;
						break;
					}
				}
			}
		}
	}

	if ((nVideoIndex==-1) || (nDemuxIndex==-1))
	{
		CSTCPrint("获取Video , demux错误\n");
		
		return CSUDI_NULL;
	}

	sPlayerChnl.m_nDemux = nDemuxIndex;
	sPlayerChnl.m_nVideoDecoder = nVideoIndex;
	sPlayerChnl.m_nAudioDecoder = 0;

	//锁测试码流频点
	switch(eResolution)
	{
		case EM_UDISCREEN_RESOLUTION_720P_50HZ :
		{
			if (CSTC_UDI2PortingLock(0, "SD_PAL_NTSC_and_HD_H.264_1080i_720P_DDplus_merge.ts"))
			{
				sStreamInfo.m_nPid = 0x328;  
				sStreamInfo.m_eContentType = EM_UDI_CONTENT_VIDEO;
				sStreamInfo.m_uStreamType.m_eVideoType = eVideoType;
			}
			else
			{
				CSTCPrint("锁定SD_PAL_NTSC_and_HD_H.264_1080i_720P_DDplus_merge.ts失败\n");

				return CSUDI_NULL;
			}
		}
		break;
		case EM_UDISCREEN_RESOLUTION_720P :
		{
			if (CSTC_UDI2PortingLock(0, "mpeg2hd+h264sd.ts"))
			{
				sStreamInfo.m_nPid = 0x328;  
				sStreamInfo.m_eContentType = EM_UDI_CONTENT_VIDEO;
				sStreamInfo.m_uStreamType.m_eVideoType = eVideoType;
			}
			else
			{
				CSTCPrint("锁定mpeg2hd+h264sd.ts 失败\n");
				
				return CSUDI_NULL;
			}
		}
		break;
		case EM_UDISCREEN_RESOLUTION_1080I_50HZ :
		{
			if (CSTC_UDI2PortingLock(0, "SD_PAL_NTSC_and_HD_H.264_1080i_720P_DDplus_merge.ts"))
			{
				sStreamInfo.m_nPid = 0x33c;  
				sStreamInfo.m_eContentType = EM_UDI_CONTENT_VIDEO;
				sStreamInfo.m_uStreamType.m_eVideoType = eVideoType;
			}
			else
			{
				CSTCPrint("锁定SD_PAL_NTSC_and_HD_H.264_1080i_720P_DDplus_merge.ts 失败\n");
				
				return NULL;
			}
		}
		break;
		case EM_UDISCREEN_RESOLUTION_1080I :
		{
			if (CSTC_UDI2PortingLock(0, "mpeg2hd+h264sd.ts"))
			{
				sStreamInfo.m_nPid = 0x33c;  
				sStreamInfo.m_eContentType = EM_UDI_CONTENT_VIDEO;
				sStreamInfo.m_uStreamType.m_eVideoType = eVideoType;
			}
			else
			{
				CSTCPrint("锁定mpeg2hd+h264sd.ts 失败\n");
				
				return NULL;
			}
		}
		break;
		default : break;
	}

	if (CSUDI_SUCCESS == CSUDIPLAYEROpen(&sPlayerChnl, EM_UDIPLAYER_LIVE, &hPlayer))
	{
		if (hPlayer != CSUDI_NULL)
		{
			if (EM_UDISCREEN_RESOLUTION_PAL == eResolution)
			{
				sStreamInfo.m_nPid = 1140;  //PAL节目源
				sStreamInfo.m_eContentType = EM_UDI_CONTENT_VIDEO;
				sStreamInfo.m_uStreamType.m_eVideoType = eVideoType;
			}

			if (EM_UDISCREEN_RESOLUTION_NTSC == eResolution)
			{
				sStreamInfo.m_nPid = 5137;  //NTSC节目源
				sStreamInfo.m_eContentType = EM_UDI_CONTENT_VIDEO;
				sStreamInfo.m_uStreamType.m_eVideoType = eVideoType;
			}

			if (CSASSERT_FAILED(CSUDI_SUCCESS == CSUDIPLAYERSetStream(hPlayer, &sStreamInfo, 1, CSUDI_NULL)))
			{
				CSTCPrint("Player set stream 失败\n");
				
				if (CSASSERT_FAILED(CSUDI_SUCCESS == CSUDIPLAYERClose(hPlayer)))
				{
					CSTCPrint("关闭播放器失败\n");
				}
				
				return CSUDI_NULL;
			}

			if (CSASSERT_FAILED(CSUDI_SUCCESS == CSUDIPLAYERStart(hPlayer)))
			{
				CSTCPrint("Player start 失败\n");
				
				if (CSASSERT_FAILED(CSUDI_SUCCESS == CSUDIPLAYERClose(hPlayer)))
				{
					CSTCPrint("关闭播放器失败\n");
				}
				
				return CSUDI_NULL;
			}

			if (CSASSERT_FAILED(CSUDI_SUCCESS == CSUDIVIDEOShow(0,TRUE)))
			{
				CSTCPrint("Video show 失败\n");
				
				if (CSASSERT_FAILED(CSUDI_SUCCESS == CSUDIPLAYERClose(hPlayer)))
				{
					CSTCPrint("关闭播放器失败\n");
				}
				
				return CSUDI_NULL;
			}
		}
	}
	else
	{
		CSTCPrint("Player open 失败\n");
	}

	return hPlayer;	
}

//屏幕分辨率改变回调函数
static void CSUDISCREENCallback (CSUDISCREENEvent_E eEvt, CSUDISCREENType_E eScreenDevice,void * pvUserData)
{
	(void)(eEvt);
	(void)(eScreenDevice);
	(void)(pvUserData);
}

//返回用户数据
static int g_nUserData;

static CSUDISCREENEvent_E g_eEvt;
static CSUDISCREENType_E g_eScreenDevice;

//屏幕分辨率改变回调函数
static void CSUDISCREENChangeCallback (CSUDISCREENEvent_E eEvt, CSUDISCREENType_E eScreenDevice,void * pvUserData)
{
	g_nUserData = *(int*)pvUserData;
	g_eEvt = eEvt;
	g_eScreenDevice = eScreenDevice;
}

//@CASEGROUP:CSUDISCREENAddCallback 
//@DESCRIPTION:测试回调函数为NULL的异常情况
//@PRECONDITION:
//@INPUT:1. fnScreenCallback = NULL
//@INPUT:2. pvUserData = NULL
//@EXPECTATION: 返回CSUDISCREEN_ERROR_BAD_PARAMETER
//@EXECUTIONFLOW:调用CSUDISCREENAddCallback,回调函数传入NULL,期望返回值为CSUDISCREEN_ERROR_BAD_PARAMETER
CSUDI_BOOL CSTC_SCREEN_TEST_IT_AddCallback_0001( void )
{
	CSUDI_Error_Code bResult = CSUDI_SUCCESS;
	int nUserData = 0x1234;

	bResult = CSUDISCREENAddCallback(CSUDI_NULL, &nUserData);

	CSTK_ASSERT_TRUE_FATAL((CSUDISCREEN_ERROR_BAD_PARAMETER == bResult), "参数检测失败");

	CSTK_FATAL_POINT;

	return CSUDI_TRUE;	
}

//@CASEGROUP:CSUDISCREENAddCallback 
//@CASEGROUP:CSUDISCREENDelCallback 
//@DESCRIPTION:测试所有参数均为正常,注册成功并且删除成功的情况
//@PRECONDITION:
//@INPUT:1. fnScreenCallback = 合法函数地址
//@INPUT:2. pvUserData = NULL
//@EXPECTATION: 注册成功并且删除成功
//@EXECUTIONFLOW:1. 调用CSUDISCREENAddCallback注册传入合法参数,期望返回CSUDI_SUCCESS
//@EXECUTIONFLOW:2. 调用CSUDISCREENDelCallback删除注册的函数,期望返回CSUDI_SUCCESS
CSUDI_BOOL CSTC_SCREEN_TEST_IT_AddCallback_0002( void )
{
	int nUserData = 0x1234;
	
	CSTK_ASSERT_TRUE_FATAL((CSUDI_SUCCESS == CSUDISCREENAddCallback(CSUDISCREENCallback, CSUDI_NULL)), "注册回调函数失败1");
	CSTK_ASSERT_TRUE_FATAL((CSUDI_SUCCESS == CSUDISCREENAddCallback(CSUDISCREENCallback, &nUserData)), "注册回调函数失败2");
	CSTK_ASSERT_TRUE_FATAL((CSUDI_SUCCESS == CSUDISCREENAddCallback(CSUDISCREENChangeCallback, CSUDI_NULL)), "注册回调函数失败3");
	CSTK_ASSERT_TRUE_FATAL((CSUDI_SUCCESS == CSUDISCREENAddCallback(CSUDISCREENChangeCallback, &nUserData)), "注册回调函数失败4");
	
	CSTK_ASSERT_TRUE_FATAL((CSUDI_SUCCESS == CSUDISCREENDelCallback(CSUDISCREENCallback, CSUDI_NULL)), "删除回调函数失败1");
	CSTK_ASSERT_TRUE_FATAL((CSUDI_SUCCESS == CSUDISCREENDelCallback(CSUDISCREENCallback, &nUserData)), "删除回调函数失败2");
	CSTK_ASSERT_TRUE_FATAL((CSUDI_SUCCESS == CSUDISCREENDelCallback(CSUDISCREENChangeCallback, CSUDI_NULL)), "删除回调函数失败3");
	CSTK_ASSERT_TRUE_FATAL((CSUDI_SUCCESS == CSUDISCREENDelCallback(CSUDISCREENChangeCallback, &nUserData)), "删除回调函数失败4");

	CSTK_FATAL_POINT;
	
	return CSUDI_TRUE;	
}

//@CASEGROUP:CSUDISCREENAddCallback 
//@CASEGROUP:CSUDISCREENDelCallback 
//@DESCRIPTION:测试注册数量超过最大注册数32的情况
//@PRECONDITION:
//@INPUT:注册时传入相同的回调函数,不同的pvUserData地址
//@EXPECTATION: 前32个回调注册成功,32以后注册失败
//@EXECUTIONFLOW:1. 反复调用CSUDISCREENAddCallback注册回调函数33次,每次注册传入的回调函数一样,但是回调函数参数地址不一致
//@EXECUTIONFLOW: 期望[0~31]应该注册成功,第32次应该注册失败
//@EXECUTIONFLOW:2. 调用CSUDISCREENDelCallback删除注册成功的回调函数,期望删除成功
CSUDI_BOOL CSTC_SCREEN_TEST_IT_AddCallback_0003( void )
{
	int i = 0;
	int j = 0;

	for (i=0; i<33; i++)
	{
		if (32 == i)
		{
			 CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS != CSUDISCREENAddCallback(CSUDISCREENCallback, (int *)i), "步骤1失败\n");
			 break;
		}

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENAddCallback(CSUDISCREENCallback, (int *)i), "步骤1失败，注册不成功\n");
		j ++;
	}

	CSTK_FATAL_POINT
	{
		for (i=0; i<j; i++)
		{
			CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENDelCallback(CSUDISCREENCallback, (int *)i), "步骤2失败，删除注册不成功\n");
		}
	}

	return CSUDI_TRUE;
}

//@CASEGROUP:CSUDISCREENDelCallback 
//@CASEGROUP:CSUDISCREENDelCallback 
//@DESCRIPTION:测试CSUDISCREENDelCallback对错误参数的检测能力
//@INPUT:删除时传入的参数错误
//@EXECUTIONFLOW:1. 传入错误的参数至CSUDISCREENDelCallback
//@EXECUTIONFLOW:2. 期望返回CSUDISCREEN_ERROR_BAD_PARAMETER
CSUDI_BOOL CSTC_SCREEN_TEST_IT_DelCallback_0001( void )
{
	CSUDI_Error_Code nResult = CSUDI_FAILURE;
	
	nResult = CSUDISCREENDelCallback(CSUDI_NULL, CSUDI_NULL);
	CSTK_ASSERT_TRUE_FATAL((CSUDISCREEN_ERROR_BAD_PARAMETER == nResult), "参数检测失败");

	CSTK_FATAL_POINT;

	return CSUDI_TRUE;
}

//@CASEGROUP:CSUDISCREENDelCallback 
//@CASEGROUP:CSUDISCREENDelCallback 
//@DESCRIPTION:测试CSUDISCREENDelCallback删除未定义的回调函数
//@PRECONDITION:
//@INPUT:传入合法参数
//@EXECUTIONFLOW:1. 注册回调函数，参数为CSUDISCREENCallback和&nUserData1
//@EXECUTIONFLOW:2. 尝试删除回调函数，参数为CSUDISCREENCallback和&nUserData2，期望失败
//@EXECUTIONFLOW:3. 尝试删除回调函数，参数为CSUDISCREENChangeCallback和&nUserData1，期望失败
//@EXECUTIONFLOW:4. 尝试删除回调函数，参数为CSUDISCREENCallback和&nUserData1，期望成功
CSUDI_BOOL CSTC_SCREEN_TEST_IT_DelCallback_0002( void )
{
	CSUDI_Error_Code nResult = CSUDI_FAILURE;
	int nUserData1 = 0x1234;
	int nUserData2 = 0x4321;
	
	nResult = CSUDISCREENAddCallback(CSUDISCREENCallback, &nUserData1);
	CSTK_ASSERT_TRUE_FATAL((CSUDI_SUCCESS == nResult), "注册回调函数失败");

	nResult = CSUDISCREENDelCallback(CSUDISCREENCallback, &nUserData2);
	CSTK_ASSERT_TRUE_FATAL((CSUDI_SUCCESS != nResult), "删除未注册的回调函数应该失败1");
	
	nResult = CSUDISCREENDelCallback(CSUDISCREENChangeCallback, &nUserData1);
	CSTK_ASSERT_TRUE_FATAL((CSUDI_SUCCESS != nResult), "删除未注册的回调函数应该失败2");

	nResult = CSUDISCREENDelCallback(CSUDISCREENCallback, &nUserData1);
	CSTK_ASSERT_TRUE_FATAL((CSUDI_SUCCESS == nResult), "删除回调函数失败");

	CSTK_FATAL_POINT;

	return CSUDI_TRUE;
}


//@CASEGROUP:CSUDISCREENDelCallback 
//@CASEGROUP:CSUDISCREENDelCallback 
//@DESCRIPTION:测试1000次删除注册回调函数
//@EXPECTATION:每次都注册成功并且删除成功
//@INPUT:传入合法参数
//@EXECUTIONFLOW:1. 调用CSUDISCREENAddCallback传入合法的回调函数以及userdata,期望注册成功
//@EXECUTIONFLOW:2.调用CSUDISCREENDelCallback删除注册的回调函数,期望返回成功
//@EXECUTIONFLOW:3.重复步骤1和2共1000次,期望每次都返回成功
CSUDI_BOOL CSTC_SCREEN_TEST_IT_DelCallback_0003( void )
{
	int i=0;
	int nUserData1 = 0x1234;
	
	for(i=0;i<1000;i++)
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENAddCallback(CSUDISCREENCallback,(void*)nUserData1),
				"step 1 fail to add screen callback\n");
		CSTK_ASSERT_TRUE_FATAL( CSUDI_SUCCESS == CSUDISCREENDelCallback(CSUDISCREENCallback, (void*)nUserData1),
				"step 2 fail to delete screen callback\n");
	}

	CSTK_FATAL_POINT;

	return CSUDI_TRUE;
}

//@CASEGROUP:CSUDISCREENGetResolution 
//@DESCRIPTION:在没有调用CSUDISCREENSetResolution函数前,调用CSUDISCREENGetResolution应能获取默认值
//@PRECONDITION: 系统没有调用CSUDISCREENSetResolution
//@EXPECTATION: 调用CSUDISCREENGetResolution应返回成功,并能获取默认值
//@EXECUTIONFLOW:1. 调用CSUDISCREENGetResolution获取标清输出通道的默认分辨率应返回成功,所获取的值应在EM_UDISCREEN_RESOLUTION_INVALID和EM_UDISCREEN_RESOLUTION_576P之间
//@EXECUTIONFLOW:2. 如果为高清平台,调用CSUDISCREENGetResolution获取高清输出通道的默认分辨率应返回成功,所获取的值应在EM_UDISCREEN_RESOLUTION_576P和EM_UDISCREEN_RESOLUTION_NUM之间
CSUDI_BOOL CSTC_SCREEN_TEST_IT_BeforeSetResolution_0001( void )
{
	CSUDI_Error_Code nResult = 0;
	CSUDISCREENResolution_E sResolution = EM_UDISCREEN_RESOLUTION_INVALID;
	
	nResult = CSUDISCREENGetResolution(EM_UDI_VOUT_DEVICE_SD, &sResolution);
	CSTK_ASSERT_TRUE_FATAL(
		(CSUDI_SUCCESS == nResult) && (sResolution > EM_UDISCREEN_RESOLUTION_INVALID && sResolution <= EM_UDISCREEN_RESOLUTION_576P), 
		"获取标清输出通道Resolution缺省值失败");

	if (!NotSupportHD())
	{
		sResolution = EM_UDISCREEN_RESOLUTION_INVALID;
		nResult = CSUDISCREENGetResolution(EM_UDI_VOUT_DEVICE_HD, &sResolution);
		CSTK_ASSERT_TRUE_FATAL(
			(CSUDI_SUCCESS == nResult) && (sResolution >= EM_UDISCREEN_RESOLUTION_720P && sResolution < EM_UDISCREEN_RESOLUTION_NUM), 
			"获取高清输出通道Resolution缺省值失败");
	}

	CSTK_FATAL_POINT;

	return CSUDI_TRUE;
}

//@CASEGROUP:CSUDISCREENSetResolution 
//@DESCRIPTION:测试设置设备为NULL的情况
//@PRECONDITION:
//@INPUT:1.peScreenDevice = NULL
//@INPUT:2.peResolution = CSUDISCREENResolution_E类型数组
//@INPUT:3.nCount=2
//@EXPECTATION: 返回错误值CSUDISCREEN_ERROR_BAD_PARAMETER
//@EXECUTIONFLOW:调用CSUDISCREENSetResolution peScreenDevice传入NULL,期望返回CSUDISCREEN_ERROR_BAD_PARAMETER
CSUDI_BOOL CSTC_SCREEN_TEST_IT_SetResolution_0001( void )
{
	CSUDISCREENResolution_E scResolution[2];
	CSUDI_Error_Code bResult = CSUDI_SUCCESS;

	bResult = CSUDISCREENSetResolution(CSUDI_NULL, scResolution, 2);

	CSTK_ASSERT_TRUE_FATAL((CSUDISCREEN_ERROR_BAD_PARAMETER == bResult), "参数检测失败");

	CSTK_FATAL_POINT;

	return CSUDI_TRUE;	
}

//@CASEGROUP:CSUDISCREENSetResolution 
//@DESCRIPTION:测试分辨率为NULL的情况
//@PRECONDITION:
//@INPUT:1.peScreenDevice = 设备数组
//@INPUT:2.peResolution = NULL
//@INPUT:3.nCount=2
//@EXPECTATION: 返回错误值CSUDISCREEN_ERROR_BAD_PARAMETER
//@EXECUTIONFLOW:调用CSUDISCREENSetResolution peResolution传入NULL,期望返回CSUDISCREEN_ERROR_BAD_PARAMETER
CSUDI_BOOL CSTC_SCREEN_TEST_IT_SetResolution_0002( void )
{
	CSUDISCREENType_E scScreenDevice[2];
	CSUDI_Error_Code bResult = CSUDI_SUCCESS;

	bResult = CSUDISCREENSetResolution(scScreenDevice, CSUDI_NULL, 2);

	CSTK_ASSERT_TRUE_FATAL((CSUDISCREEN_ERROR_BAD_PARAMETER == bResult), "参数检测失败");

	CSTK_FATAL_POINT;

	return CSUDI_TRUE;	
}

//@CASEGROUP:CSUDISCREENSetResolution 
//@DESCRIPTION:测试nCount 错误和CSUDISCREENType_E和CSUDISCREENResolution_E以及nCount不匹配的情况
//@PRECONDITION:
//@INPUT:1. peScreenDevice = 设备数组,CSUDISCREENResolution_E类型数组,nCount = 0
//@INPUT:2. peScreenDevice = 设备数组,CSUDISCREENResolution_E类型数组,nCount = -1
//@INPUT:2. peScreenDevice = 设备数组,CSUDISCREENResolution_E类型数组,nCount = 3
//@EXPECTATION: 返回错误值CSUDISCREEN_ERROR_BAD_PARAMETER
//@EXECUTIONFLOW:1. 调用CSUDISCREENSetResolution nCount传入0,期望返回CSUDISCREEN_ERROR_BAD_PARAMETER
//@EXECUTIONFLOW:2. 调用CSUDISCREENSetResolution nCount传入-1,期望返回CSUDISCREEN_ERROR_BAD_PARAMETER
//@EXECUTIONFLOW:3. 调用CSUDISCREENSetResolution nCount传入3,期望返回CSUDISCREEN_ERROR_BAD_PARAMETER
//@EXECUTIONFLOW:4. 调用CSUDISCREENSetResolution， CSUDISCREENType_E和CSUDISCREENResolution_E中的成员错误,期望返回CSUDISCREEN_ERROR_BAD_PARAMETER
CSUDI_BOOL CSTC_SCREEN_TEST_IT_SetResolution_0003( void )
{
	CSUDISCREENType_E scScreenDevice[2];
	CSUDISCREENType_E scScreenDevice_4[1];
	CSUDISCREENResolution_E scResolution[2];
	CSUDISCREENResolution_E scResolution_4[1];
	 		 
	CSTK_ASSERT_TRUE_FATAL((CSUDISCREEN_ERROR_BAD_PARAMETER == CSUDISCREENSetResolution(scScreenDevice, scResolution, 0)),
		"参数检测失败1");

	CSTK_ASSERT_TRUE_FATAL((CSUDISCREEN_ERROR_BAD_PARAMETER == CSUDISCREENSetResolution(scScreenDevice, scResolution, -1)),
		"参数检测失败2");

	CSTK_ASSERT_TRUE_FATAL((CSUDISCREEN_ERROR_BAD_PARAMETER == CSUDISCREENSetResolution(scScreenDevice, scResolution, 3)),
		"参数检测失败3");

	CSTK_ASSERT_TRUE_FATAL((CSUDISCREEN_ERROR_BAD_PARAMETER == CSUDISCREENSetResolution(scScreenDevice_4, scResolution, 2)),
		"参数检测失败4-1");

	CSTK_ASSERT_TRUE_FATAL((CSUDISCREEN_ERROR_BAD_PARAMETER == CSUDISCREENSetResolution(scScreenDevice, scResolution_4, 2)),
		"参数检测失败4-2");

	CSTK_FATAL_POINT;

	return CSUDI_TRUE;
}

//@CASEGROUP:CSUDISCREENSetResolution 
//@DESCRIPTION:测试节目源是标清PAL的情况下,对设备进行各种制式转换
//@PRECONDITION:
//@INPUT:1. 播放标清PAL节目源
//@EXPECTATION:1. 当对设备设置分辨率时分辨率发生正常切换
//@EXPECTATION:2. 当标清和高清输出的场频和节目源的场频一致,即均为50HZ时要求画面无任何异常
//@EXPECTATION:3. 当输出标清和高清场频不一致时画面可能有异常,但是要求不死机
//@REMARK:1.设备是否支持某种分辨率是通过CSUDISCREENGetCapability获得
//@REMARK:2.如果平台支持1080P,当设置输出为1080P时,确认画面是否正常需要电视也支持1080P
//@EXECUTIONFLOW:1.调用player模块接口播放标清PAL码流,详细步骤如下
//@EXECUTIONFLOW:1.1 锁测试码流频点
//@EXECUTIONFLOW:1.2 调用CSUDIVIDEOGetCount获取video解码器数量
//@EXECUTIONFLOW:1.3 调用CSUDIVIDEOGetCapability依次寻找第一个支持标清解码的decoder,并将其作为节目播放的视频解码器
//@EXECUTIONFLOW:1.4 调用CSUDIDEMUXGetCount获取demux的数量
//@EXECUTIONFLOW:1.5 调用CSUDIDEMUXGetCapability依次寻找第一个支持EM_UDI_DEMUX_PLAY的demux用于节目播放的demux
//@EXECUTIONFLOW:1.6 调用CSUDIPLAYEROpen打开一个播放器,期望打开成功
//@EXECUTIONFLOW:1.7 调用CSUDIPLAYERSetStream设置测试码流属性
//@EXECUTIONFLOW:1.8 调用CSUDIPLAYERStart播放测试码流,期望有视频输出
//@EXECUTIONFLOW:2. 设置标清视频输出格式为EM_UDISCREEN_RESOLUTION_PAL,高清为EM_UDISCREEN_RESOLUTION_720P_50HZ,期望分辨率正常切换,画面正常
//@EXECUTIONFLOW:3. 设置标清视频输出格式为EM_UDISCREEN_RESOLUTION_PAL,高清为EM_UDISCREEN_RESOLUTION_1080I_50HZ,期望分辨率正常切换,画面正常
//@EXECUTIONFLOW:4. 如果平台支持1080p 50HZ,设置标清视频输出格式为EM_UDISCREEN_RESOLUTION_PAL,高清为EM_UDISCREEN_RESOLUTION_1080P_50HZ,期望分辨率正常切换,画面正常
//@EXECUTIONFLOW:5. 设置标清视频输出格式为EM_UDISCREEN_RESOLUTION_NTSC,高清为EM_UDISCREEN_RESOLUTION_720P_50HZ,期望不死机,sleep 1S
//@EXECUTIONFLOW:6. 设置标清视频输出格式为EM_UDISCREEN_RESOLUTION_PAL,高清为EM_UDISCREEN_RESOLUTION_720P,期望不死机,sleep 1S
//@EXECUTIONFLOW:7. 设置标清视频输出格式为EM_UDISCREEN_RESOLUTION_NTSC,高清为EM_UDISCREEN_RESOLUTION_720P,期望不死机,sleep 1S
//@EXECUTIONFLOW:8. 设置标清视频输出格式为EM_UDISCREEN_RESOLUTION_NTSC,高清为EM_UDISCREEN_RESOLUTION_1080I_50HZ,期望不死机,sleep 1S
//@EXECUTIONFLOW:9. 如果平台支持1080i 60HZ,设置标清视频输出格式为EM_UDISCREEN_RESOLUTION_PAL,高清为EM_UDISCREEN_RESOLUTION_1080I,期望不死机,sleep 1S
//@EXECUTIONFLOW:10. 如果平台支持1080i 60HZ,设置标清视频输出格式为EM_UDISCREEN_RESOLUTION_NTSC,高清为EM_UDISCREEN_RESOLUTION_1080I,期望不死机,sleep 1S
//@EXECUTIONFLOW:11. 如果平台支持1080p 50HZ,设置标清视频输出格式为EM_UDISCREEN_RESOLUTION_NTSC,高清为EM_UDISCREEN_RESOLUTION_1080P_50HZ,期望不死机,sleep 1S
//@EXECUTIONFLOW:12. 如果平台支持1080p 60HZ,设置标清视频输出格式为EM_UDISCREEN_RESOLUTION_PAL,高清为EM_UDISCREEN_RESOLUTION_1080P,期望不死机,sleep 1S
//@EXECUTIONFLOW:13. 如果平台支持1080p 60HZ,设置标清视频输出格式为EM_UDISCREEN_RESOLUTION_NTSC,高清为EM_UDISCREEN_RESOLUTION_1080P,期望不死机,sleep 1S
//@EXECUTIONFLOW:14. 恢复测试前分辨率设置,该分辨为该模块测试初始化时获得的高标清分辨率
//@EXECUTIONFLOW:15. 调用CSUDIPLAYERStop停止测试节目播放
//@EXECUTIONFLOW:16. 调用CSUDIPLAYERClose关闭测试播放器
CSUDI_BOOL CSTC_SCREEN_TEST_IT_SetResolution_0004( void )
{
	CSUDI_HANDLE hPlayer = CSUDI_NULL;
	CSUDISCREENType_E scScreenDevice[2] = {EM_UDI_VOUT_DEVICE_SD, EM_UDI_VOUT_DEVICE_HD};
	CSUDISCREENResolution_E scResolution[2];

	hPlayer = PlaySDProgram(EM_UDI_VID_STREAM_MPEG2 , EM_UDISCREEN_RESOLUTION_PAL);  //调用PLAYER播放测试码流

	CSTK_ASSERT_TRUE_FATAL(CSUDI_NULL != hPlayer, "启动节目播放失败\n");

	CSTCPrint("[UDI2SCREENTEST]:视频节目播放画面是否正常?\r\n");
	CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes( ), "步骤1失败\n");

	////对于标清平台不支持标清高清通道一起设置，现单独对标清通道进行PAL及NTSC制式转换，能设置成功且画面正常则测试通过。
	if (NotSupportHD())
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENGetResolution(scScreenDevice[0], &g_scResolution[0]), "获取测试前分辨率失败\n");

		scResolution[0] = EM_UDISCREEN_RESOLUTION_PAL;
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(&scScreenDevice[0], &scResolution[0] , 1), "步骤1失败\n");
		CSTCPrint("[UDI2SCREENTEST]:Yes,设置分辨率为PAL分辨率正常切换,画面正常,无抖动、无闪烁\r\n");
		CSTCPrint("[UDI2SCREENTEST]:No,分辨率不能正常切换或画面不正常 \r\n");
		CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes( ), "步骤2失败\n");
			
		scResolution[0] = EM_UDISCREEN_RESOLUTION_NTSC;
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(&scScreenDevice[0], &scResolution[0] , 1), "步骤3失败\n");
		CSTCPrint("[UDI2SCREENTEST]:Yes,设置分辨率为NTSC分辨率正常切换,画面正常,无抖动、无闪烁\r\n");
		CSTCPrint("[UDI2SCREENTEST]:No,分辨率不能正常切换或画面不正常 \r\n");
		CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes( ), "步骤3失败\n");

		scResolution[0] = EM_UDISCREEN_RESOLUTION_PAL;
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(&scScreenDevice[0], &scResolution[0] , 1), "步骤1失败\n");
		CSTCPrint("[UDI2SCREENTEST]::Yes,设置分辨率为PAL分辨率正常切换,画面正常,无抖动、无闪烁\r\n");
		CSTCPrint("[UDI2SCREENTEST]::No,分辨率不能正常切换或画面不正常 \r\n");
		CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes( ), "步骤4失败\n");
	}
	else
	{
		CSTCPrint("********请确认已连接SD/HD 两种输出端***********\n");
	   	CSTKWaitAnyKey();
	   
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENGetResolution(scScreenDevice[0], &g_scResolution[0]), "获取测试前分辨率失败\n");
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENGetResolution(scScreenDevice[1], &g_scResolution[1]), "获取测试前分辨率失败\n");

		scResolution[0] = EM_UDISCREEN_RESOLUTION_NTSC;
		scResolution[1] = EM_UDISCREEN_RESOLUTION_720P_50HZ;
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤2失败\n");
		CSTCPrint("[UDI2SCREENTEST]::Yes,分辨率正常切换,画面正常,无抖动、无闪烁\r\n");
		CSTCPrint("[UDI2SCREENTEST]::No,分辨率不能正常切换或画面不正常 \r\n");
		CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes( ), "步骤2失败\n");

		scResolution[0] = EM_UDISCREEN_RESOLUTION_PAL;
		scResolution[1] = EM_UDISCREEN_RESOLUTION_1080I_50HZ;
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤3失败\n"); 
		CSTCPrint("[UDI2SCREENTEST]::Yes,分辨率正常切换,画面正常,无抖动、无闪烁\r\n");
		CSTCPrint("[UDI2SCREENTEST]::No,分辨率不能正常切换或画面不正常 \r\n");
		CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes( ), "步骤3失败\n");

		 //如果平台支持1080P 50HZ
		 if (IsScreenSupport(scScreenDevice, EM_UDISCREEN_RESOLUTION_1080P_50HZ ))
		 {
			scResolution[0] = EM_UDISCREEN_RESOLUTION_PAL;
			scResolution[1] = EM_UDISCREEN_RESOLUTION_1080P_50HZ;
			CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤4失败\n"); 
			CSTCPrint("[UDI2SCREENTEST]::Yes,分辨率正常切换,画面正常,无抖动、无闪烁\r\n");
			CSTCPrint("[UDI2SCREENTEST]::No,分辨率不能正常切换或画面不正常 \r\n");
			CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes( ), "步骤4失败\n");
		 }

		 CSTCPrint("[UDI2SCREENTEST] 下面将对设备进行制式转换自动化测试，按任意键开始...\n");
		 CSTKWaitAnyKey();

		scResolution[0] = EM_UDISCREEN_RESOLUTION_NTSC;
		scResolution[1] = EM_UDISCREEN_RESOLUTION_720P_50HZ;
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤5失败\n");
		CSUDIOSThreadSleep(1000);

		scResolution[0] = EM_UDISCREEN_RESOLUTION_PAL;
		scResolution[1] = EM_UDISCREEN_RESOLUTION_720P;
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤6失败\n"); 
		CSUDIOSThreadSleep(1000);

		scResolution[0] = EM_UDISCREEN_RESOLUTION_NTSC;
		scResolution[1] = EM_UDISCREEN_RESOLUTION_720P;
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤7失败\n"); 
		CSUDIOSThreadSleep(1000);

		scResolution[0] = EM_UDISCREEN_RESOLUTION_PAL;
		scResolution[1] = EM_UDISCREEN_RESOLUTION_1080I_50HZ;
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤8失败\n"); 
		CSUDIOSThreadSleep(1000);

		//如果平台支持1080i 60HZ  
		if (IsScreenSupport(scScreenDevice, EM_UDISCREEN_RESOLUTION_1080I ))
		{
			scResolution[0] = EM_UDISCREEN_RESOLUTION_PAL;
			scResolution[1] = EM_UDISCREEN_RESOLUTION_1080I;
			CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤9失败\n"); 
			CSUDIOSThreadSleep(1000);

			scResolution[0] = EM_UDISCREEN_RESOLUTION_NTSC;
			scResolution[1] = EM_UDISCREEN_RESOLUTION_1080I;
			CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤10失败\n"); 
			CSUDIOSThreadSleep(1000);
		}

		//如果平台支持1080p 50HZ
		if (IsScreenSupport(scScreenDevice, EM_UDISCREEN_RESOLUTION_1080P_50HZ ))
		{
			scResolution[0] = EM_UDISCREEN_RESOLUTION_NTSC;
			scResolution[1] = EM_UDISCREEN_RESOLUTION_1080P_50HZ;
			CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤11失败\n"); 
			CSUDIOSThreadSleep(1000);
		}

		 //如果平台支持1080P 60HZ
		if (IsScreenSupport(scScreenDevice, EM_UDISCREEN_RESOLUTION_1080P_50HZ ))
		{
			scResolution[0] = EM_UDISCREEN_RESOLUTION_PAL;
			scResolution[1] = EM_UDISCREEN_RESOLUTION_1080P;
			CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤12失败\n"); 
			CSUDIOSThreadSleep(1000);

			scResolution[0] = EM_UDISCREEN_RESOLUTION_NTSC;
			scResolution[1] = EM_UDISCREEN_RESOLUTION_1080P;
			CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤13失败\n"); 
			CSUDIOSThreadSleep(1000);
		}
	}

	CSTK_FATAL_POINT
	{
		//恢复测试前分辨率
		if (NotSupportHD())
		{
			if (g_scResolution[0]>EM_UDISCREEN_RESOLUTION_INVALID && g_scResolution[0]<=EM_UDISCREEN_RESOLUTION_576P)
			{
				CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(&scScreenDevice[0], &g_scResolution[0], 1), "步骤14失败\n");
			}
		}
		else
		{
			if ((g_scResolution[0]>EM_UDISCREEN_RESOLUTION_INVALID && g_scResolution[0]<=EM_UDISCREEN_RESOLUTION_576P)
				&& (g_scResolution[1]>=EM_UDISCREEN_RESOLUTION_720P && g_scResolution[0]<EM_UDISCREEN_RESOLUTION_NUM))
			{
				scResolution[0] = g_scResolution[0];
				scResolution[1] = g_scResolution[1];
				CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤14失败\n");
			}
		}

		if (CSUDI_NULL != hPlayer)
		{
			CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERStop(hPlayer), "停止播放节目失败\n");
			CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERClose(hPlayer), "关闭播放器失败\n");
			hPlayer = CSUDI_NULL;
		}
	}

	return CSUDI_TRUE;
}

//@CASEGROUP:CSUDISCREENSetResolution 
//@DESCRIPTION:测试节目源是标清NTSC的情况下,对设备进行各种制式转换
//@PRECONDITION:
//@INPUT:1. 播放标清NTSC节目源
//@EXPECTATION:1. 当对设备设置分辨率时分辨率发生正常切换
//@EXPECTATION:2. 当标清和高清输出的场频和节目源的场频一致,即均为60HZ时要求画面无任何异常
//@EXPECTATION:3. 当输出标清和高清场频不一致时画面可能有异常,但是要求不死机
//@REMARK:1.设备是否支持某种分辨率是通过CSUDISCREENGetCapability获得
//@REMARK:2.如果平台支持1080P,当设置输出为1080P时,确认画面是否正常需要电视也支持1080P
//@EXECUTIONFLOW:1.调用player模块接口播放标清NTSC码流,详细步骤清参见CSTC_SCREEN_TEST_IT_SetResolution_0003步骤1
//@EXECUTIONFLOW:2. 设置标清视频输出格式为EM_UDISCREEN_RESOLUTION_NTSC,高清为EM_UDISCREEN_RESOLUTION_720P,期望分辨率正常切换,画面正常
//@EXECUTIONFLOW:3. 如果平台支持1080i 60HZ,设置标清视频输出格式为EM_UDISCREEN_RESOLUTION_NTSC,高清为EM_UDISCREEN_RESOLUTION_1080I,期望分辨率正常切换,画面正常
//@EXECUTIONFLOW:4. 如果平台支持1080p 60HZ,设置标清视频输出格式为EM_UDISCREEN_RESOLUTION_NTSC,高清为EM_UDISCREEN_RESOLUTION_1080P,期望分辨率正常切换,画面正常
//@EXECUTIONFLOW:5. 设置标清视频输出格式为EM_UDISCREEN_RESOLUTION_PAL,高清为EM_UDISCREEN_RESOLUTION_1080I_50HZ,期望不死机,sleep 1S
//@EXECUTIONFLOW:6. 设置标清视频输出格式为EM_UDISCREEN_RESOLUTION_NTSC,高清为EM_UDISCREEN_RESOLUTION_1080I_50HZ,期望不死机,sleep 1S
//@EXECUTIONFLOW:7. 设置标清视频输出格式为EM_UDISCREEN_RESOLUTION_PAL,高清为EM_UDISCREEN_RESOLUTION_720P_50HZ,期望不死机,sleep 1S
//@EXECUTIONFLOW:8. 设置标清视频输出格式为EM_UDISCREEN_RESOLUTION_NTSC,高清为EM_UDISCREEN_RESOLUTION_720P_50HZ,期望不死机,sleep 1S
//@EXECUTIONFLOW:9. 设置标清视频输出格式为EM_UDISCREEN_RESOLUTION_PAL,高清为EM_UDISCREEN_RESOLUTION_720P,期望不死机,sleep 1S
//@EXECUTIONFLOW:10. 如果平台支持1080i 60HZ,设置标清视频输出格式为EM_UDISCREEN_RESOLUTION_PAL,高清为EM_UDISCREEN_RESOLUTION_1080I,期望不死机,sleep 1S
//@EXECUTIONFLOW:11. 如果平台支持1080p 50HZ,设置标清视频输出格式为EM_UDISCREEN_RESOLUTION_PAL,高清为EM_UDISCREEN_RESOLUTION_1080P_50HZ,期望不死机,sleep 1S
//@EXECUTIONFLOW:12. 如果平台支持1080p 50HZ,设置标清视频输出格式为EM_UDISCREEN_RESOLUTION_NTSC,高清为EM_UDISCREEN_RESOLUTION_1080P_50HZ,期望不死机,sleep 1S
//@EXECUTIONFLOW:13. 如果平台支持1080p 60HZ,设置标清视频输出格式为EM_UDISCREEN_RESOLUTION_PAL,高清为EM_UDISCREEN_RESOLUTION_1080P,期望不死机,sleep 1S
//@EXECUTIONFLOW:14. 恢复测试前分辨率设置,该分辨为该模块测试初始化时获得的高标清分辨率
//@EXECUTIONFLOW:15. 调用CSUDIPLAYERStop停止测试节目播放
//@EXECUTIONFLOW:16. 调用CSUDIPLAYERClose关闭测试播放器
CSUDI_BOOL CSTC_SCREEN_TEST_IT_SetResolution_0005( void )
{
	CSUDI_HANDLE hPlayer = CSUDI_NULL;
	CSUDISCREENType_E scScreenDevice[2] = {EM_UDI_VOUT_DEVICE_SD, EM_UDI_VOUT_DEVICE_HD};
	CSUDISCREENResolution_E scResolution[2];

	hPlayer = PlaySDProgram( EM_UDI_VID_STREAM_MPEG2, EM_UDISCREEN_RESOLUTION_NTSC);  //调用PLAYER播放测试码流

	CSTK_ASSERT_TRUE_FATAL(CSUDI_NULL != hPlayer, "启动节目播放失败\n");

	CSTCPrint("[UDI2SCREENTEST]::视频节目播放画面是否正常?\r\n");
	CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes( ), "步骤1失败\n");

	////对于标清平台不支持标清、高清通道一起设置，现单独对标清通道进行PAL及NTSC制式转换，能设置成功且画面正常则测试通过。
	if (NotSupportHD())
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENGetResolution(scScreenDevice[0], &g_scResolution[0]), "获取测试前分辨率失败\n");

		scResolution[0] = EM_UDISCREEN_RESOLUTION_PAL;
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(&scScreenDevice[0], &scResolution[0] , 1), "步骤3失败\n");
		CSTCPrint("[UDI2SCREENTEST]::Yes,设置分辨率为PAL分辨率正常切换,画面正常,无抖动、无闪烁\r\n");
		CSTCPrint("[UDI2SCREENTEST]::No,分辨率不能正常切换或画面不正常 \r\n");
		CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes( ), "步骤2失败\n");

		scResolution[0] = EM_UDISCREEN_RESOLUTION_NTSC;
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(&scScreenDevice[0], &scResolution[0] , 1), "步骤1失败\n");
		CSTCPrint("[UDI2SCREENTEST]::Yes,设置分辨率为NTSC分辨率正常切换,画面正常,无抖动、无闪烁\r\n");
		CSTCPrint("[UDI2SCREENTEST]::No,分辨率不能正常切换或画面不正常 \r\n");
		CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes( ), "步骤3失败\n");

		scResolution[0] = EM_UDISCREEN_RESOLUTION_PAL;
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(&scScreenDevice[0], &scResolution[0] , 1), "步骤3失败\n");
		CSTCPrint("[UDI2SCREENTEST]::Yes,设置分辨率为PAL分辨率正常切换,画面正常,无抖动、无闪烁\r\n");
		CSTCPrint("[UDI2SCREENTEST]::No,分辨率不能正常切换或画面不正常 \r\n");
		CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes( ), "步骤4失败\n");
	}
	else
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENGetResolution(scScreenDevice[0], &g_scResolution[0]), "获取测试前分辨率失败\n");
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENGetResolution(scScreenDevice[1], &g_scResolution[1]), "获取测试前分辨率失败\n");

		CSTCPrint("********请确认已连接SD/HD 两种输出端***********\n");
	  	CSTKWaitAnyKey();
		
		scResolution[0] = EM_UDISCREEN_RESOLUTION_NTSC;
		scResolution[1] = EM_UDISCREEN_RESOLUTION_720P;
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤2失败\n");
		CSTCPrint("[UDI2SCREENTEST]::Yes,分辨率正常切换,画面正常,无抖动、无闪烁\r\n");
		CSTCPrint("[UDI2SCREENTEST]::No,分辨率不能正常切换或画面不正常 \r\n");
		CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes( ), "步骤2失败\n");

		 //如果平台支持1080i 60HZ
		if (IsScreenSupport(scScreenDevice, EM_UDISCREEN_RESOLUTION_1080I ))
		{
			scResolution[0] = EM_UDISCREEN_RESOLUTION_PAL;
			scResolution[1] = EM_UDISCREEN_RESOLUTION_1080I;
			CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤3失败\n");
			CSTCPrint("[UDI2SCREENTEST]::Yes,分辨率正常切换,画面正常,无抖动、无闪烁\r\n");
			CSTCPrint("[UDI2SCREENTEST]::No,分辨率不能正常切换或画面不正常 \r\n");
			CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes( ), "步骤3失败\n");
		}
		 
		//如果平台支持1080p 60HZ
		if (IsScreenSupport(scScreenDevice, EM_UDISCREEN_RESOLUTION_1080P ))
		{
			scResolution[0] = EM_UDISCREEN_RESOLUTION_NTSC;
			scResolution[1] = EM_UDISCREEN_RESOLUTION_1080P;
			CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤4失败\n"); 
			CSTCPrint("[UDI2SCREENTEST]::Yes,分辨率正常切换,画面正常,无抖动、无闪烁\r\n");
			CSTCPrint("[UDI2SCREENTEST]::No,分辨率不能正常切换或画面不正常 \r\n");
			CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes( ), "步骤4失败\n");
		}

		CSTCPrint("[UDI2SCREENTEST] 下面将对设备进行制式转换自动化测试，按任意键开始..\n");
		CSTKWaitAnyKey();

		scResolution[0] = EM_UDISCREEN_RESOLUTION_PAL;
		scResolution[1] = EM_UDISCREEN_RESOLUTION_1080I_50HZ;
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤5失败\n");
		CSUDIOSThreadSleep(1000);

		scResolution[0] = EM_UDISCREEN_RESOLUTION_NTSC;
		scResolution[1] = EM_UDISCREEN_RESOLUTION_1080I_50HZ;
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤6失败\n"); 
		CSUDIOSThreadSleep(1000);

		scResolution[0] = EM_UDISCREEN_RESOLUTION_PAL;
		scResolution[1] = EM_UDISCREEN_RESOLUTION_720P_50HZ;
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤7失败\n"); 
		CSUDIOSThreadSleep(1000);

		scResolution[0] = EM_UDISCREEN_RESOLUTION_NTSC;
		scResolution[1] = EM_UDISCREEN_RESOLUTION_720P_50HZ;
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤8失败\n"); 
		CSUDIOSThreadSleep(1000);

		scResolution[0] = EM_UDISCREEN_RESOLUTION_PAL;
		scResolution[1] = EM_UDISCREEN_RESOLUTION_720P;
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤9失败\n"); 
		CSUDIOSThreadSleep(1000);

		//如果平台支持1080i 60HZ
		if (IsScreenSupport(scScreenDevice, EM_UDISCREEN_RESOLUTION_1080I ))
		{
			scResolution[0] = EM_UDISCREEN_RESOLUTION_NTSC;
			scResolution[1] = EM_UDISCREEN_RESOLUTION_1080I;
			CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤10失败\n"); 
			CSUDIOSThreadSleep(1000);
		}

		//如果平台支持1080p 50HZ
		if (IsScreenSupport(scScreenDevice, EM_UDISCREEN_RESOLUTION_1080P_50HZ ))
		{
			scResolution[0] = EM_UDISCREEN_RESOLUTION_PAL;
			scResolution[1] = EM_UDISCREEN_RESOLUTION_1080P_50HZ;
			CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤11失败\n"); 
			CSUDIOSThreadSleep(1000);

			scResolution[0] = EM_UDISCREEN_RESOLUTION_NTSC;
			scResolution[1] = EM_UDISCREEN_RESOLUTION_1080P_50HZ;
			CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤12失败\n"); 
			CSUDIOSThreadSleep(1000);
		}

		//如果平台支持1080p 60HZ
		if (IsScreenSupport(scScreenDevice, EM_UDISCREEN_RESOLUTION_1080P ))
		{
			scResolution[0] = EM_UDISCREEN_RESOLUTION_PAL;
			scResolution[1] = EM_UDISCREEN_RESOLUTION_1080P;
			CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤13失败\n"); 
			CSUDIOSThreadSleep(1000);
		}
	}

	 CSTK_FATAL_POINT
	{
		//恢复测试前分辨率
		if (NotSupportHD())
		{
			if (g_scResolution[0]>EM_UDISCREEN_RESOLUTION_INVALID && g_scResolution[0]<=EM_UDISCREEN_RESOLUTION_576P)
			{
				CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(&scScreenDevice[0], &g_scResolution[0], 1), "步骤14失败\n");
			}
		}
		else
		{
			if ((g_scResolution[0]>EM_UDISCREEN_RESOLUTION_INVALID && g_scResolution[0]<=EM_UDISCREEN_RESOLUTION_576P)
				&& (g_scResolution[1]>=EM_UDISCREEN_RESOLUTION_720P && g_scResolution[0]<EM_UDISCREEN_RESOLUTION_NUM))
			{
				scResolution[0] = g_scResolution[0];
				scResolution[1] = g_scResolution[1];
				CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤14失败\n");
			}
		}
		
		if (CSUDI_NULL != hPlayer)
		{
			CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERStop(hPlayer), "停止播放节目失败\n");
			CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERClose(hPlayer), "关闭播放器失败\n");
			hPlayer = CSUDI_NULL;
		}
	}

	return CSUDI_TRUE;
}

//@CASEGROUP:CSUDISCREENSetResolution 
//@DESCRIPTION:测试节目源是高清EM_UDISCREEN_RESOLUTION_720P_50HZ的情况下,对标清和高清设备进行标清制式转换,该测试用例主要用于高清平台
//@PRECONDITION:平台支持高清解码
//@INPUT:1. 播放高清720P_50HZ节目源
//@EXPECTATION:1. 当对设备设置分辨率时分辨率发生正常切换
//@EXPECTATION:2. 当标清和高清输出的场频和节目源的场频一致,即均为60HZ时要求画面无任何异常
//@EXPECTATION:3. 当输出标清和高清场频不一致时画面可能有异常,但是要求不死机
//@REMARK:1.设备是否支持某种分辨率是通过CSUDISCREENGetCapability获得
//@REMARK:2.如果平台支持1080P,当设置输出为1080P时,确认画面是否正常需要电视也支持1080P
//@EXECUTIONFLOW:1.调用player模块接口播放高清720P_50HZ码流,详细步骤如下:
//@EXECUTIONFLOW:1.1 锁测试码流频点
//@EXECUTIONFLOW:1.2 调用CSUDIVIDEOGetCount获取video解码器数量
//@EXECUTIONFLOW:1.3 调用CSUDIVIDEOGetCapability依次寻找第一个支持高清解码的decoder,并将其作为节目播放的视频解码器
//@EXECUTIONFLOW:1.4 调用CSUDIDEMUXGetCount获取demux的数量
//@EXECUTIONFLOW:1.5 调用CSUDIDEMUXGetCapability依次寻找第一个支持EM_UDI_DEMUX_PLAY的demux用于节目播放的demux
//@EXECUTIONFLOW:1.6 调用CSUDIPLAYEROpen打开一个播放器,期望打开成功
//@EXECUTIONFLOW:1.7 调用CSUDIPLAYERSetStream设置测试码流属性
//@EXECUTIONFLOW:1.8 调用CSUDIPLAYERStart播放测试码流,期望有视频输出
//@EXECUTIONFLOW:2. 设置标清视频输出格式为EM_UDISCREEN_RESOLUTION_PAL,高清为EM_UDISCREEN_RESOLUTION_720P_50HZ,期望分辨率正常切换,画面正常
//@EXECUTIONFLOW:3. 设置标清视频输出格式为EM_UDISCREEN_RESOLUTION_PAL,高清为EM_UDISCREEN_RESOLUTION_1080I_50HZ,期望分辨率正常切换,画面正常
//@EXECUTIONFLOW:4. 如果平台支持1080p 50HZ,设置标清视频输出格式为EM_UDISCREEN_RESOLUTION_PAL,高清为EM_UDISCREEN_RESOLUTION_1080P_50HZ,期望分辨率正常切换,画面正常
//@EXECUTIONFLOW:5. 设置标清视频输出格式为EM_UDISCREEN_RESOLUTION_NTSC,高清为EM_UDISCREEN_RESOLUTION_720P_50HZ,期望不死机,sleep 1S
//@EXECUTIONFLOW:6. 设置标清视频输出格式为EM_UDISCREEN_RESOLUTION_PAL,高清为EM_UDISCREEN_RESOLUTION_720P,期望不死机,sleep 1S
//@EXECUTIONFLOW:7. 设置标清视频输出格式为EM_UDISCREEN_RESOLUTION_NTSC,高清为EM_UDISCREEN_RESOLUTION_720P,期望不死机,sleep 1S
//@EXECUTIONFLOW:8. 设置标清视频输出格式为EM_UDISCREEN_RESOLUTION_NTSC,高清为EM_UDISCREEN_RESOLUTION_1080I_50HZ,期望不死机,sleep 1S
//@EXECUTIONFLOW:9. 如果平台支持1080i 60HZ,设置标清视频输出格式为EM_UDISCREEN_RESOLUTION_PAL,高清为EM_UDISCREEN_RESOLUTION_1080I,期望不死机,sleep 1S
//@EXECUTIONFLOW:10. 如果平台支持1080i 60HZ,设置标清视频输出格式为EM_UDISCREEN_RESOLUTION_NTSC,高清为EM_UDISCREEN_RESOLUTION_1080I,期望不死机,sleep 1S
//@EXECUTIONFLOW:11. 如果平台支持1080p 50HZ,设置标清视频输出格式为EM_UDISCREEN_RESOLUTION_NTSC,高清为EM_UDISCREEN_RESOLUTION_1080P_50HZ,期望不死机,sleep 1S
//@EXECUTIONFLOW:12. 如果平台支持1080p 60HZ,设置标清视频输出格式为EM_UDISCREEN_RESOLUTION_PAL,高清为EM_UDISCREEN_RESOLUTION_1080P,期望不死机,sleep 1S
//@EXECUTIONFLOW:13. 如果平台支持1080p 60HZ,设置标清视频输出格式为EM_UDISCREEN_RESOLUTION_NTSC,高清为EM_UDISCREEN_RESOLUTION_1080P,期望不死机,sleep 1S
//@EXECUTIONFLOW:14. 恢复测试前分辨率设置,该分辨为该模块测试初始化时获得的高标清分辨率
//@EXECUTIONFLOW:15. 调用CSUDIPLAYERStop停止测试节目播放
//@EXECUTIONFLOW:16. 调用CSUDIPLAYERClose关闭测试播放器
CSUDI_BOOL CSTC_SCREEN_TEST_IT_SetResolution_0006( void )
{
	CSUDI_HANDLE hPlayer = CSUDI_NULL;
	CSUDISCREENType_E scScreenDevice[2] = {EM_UDI_VOUT_DEVICE_SD, EM_UDI_VOUT_DEVICE_HD};
	CSUDISCREENResolution_E scResolution[2];

	if (NotSupportHD())
	{
		CSTCPrint("[UDI2SCREENTEST]  该用例用于测试高清平台\r\n");
		return CSUDI_TRUE;
	}

	hPlayer = PlayHDProgram(EM_UDI_VID_STREAM_H264 , EM_UDISCREEN_RESOLUTION_720P_50HZ);  //测试节目源是高清EM_UDISCREEN_RESOLUTION_720P_50HZ
	
	CSTK_ASSERT_TRUE_FATAL(CSUDI_NULL != hPlayer, "启动节目播放失败");

	CSTCPrint("[UDI2SCREENTEST]::视频节目播放画面是否正常?\r\n");
	CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes( ), "步骤1失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENGetResolution(scScreenDevice[0], &g_scResolution[0]), "获取测试前分辨率失败\n");
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENGetResolution(scScreenDevice[1], &g_scResolution[1]), "获取测试前分辨率失败\n");

	CSTCPrint("********请确认已连接SD/HD 两种输出端***********\n");
	CSTKWaitAnyKey();
	
	scResolution[0] = EM_UDISCREEN_RESOLUTION_PAL;
	scResolution[1] = EM_UDISCREEN_RESOLUTION_720P_50HZ;
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤2失败");
	CSTCPrint("[UDI2SCREENTEST]::Yes,分辨率正常切换,画面正常,无抖动、无闪烁\r\n");
	CSTCPrint("[UDI2SCREENTEST]::No,分辨率不能正常切换或画面不正常 \r\n");
	CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes( ), "步骤2失败");

	scResolution[0] = EM_UDISCREEN_RESOLUTION_NTSC;
	scResolution[1] = EM_UDISCREEN_RESOLUTION_1080I_50HZ;
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤3失败"); 
	CSTCPrint("[UDI2SCREENTEST]::Yes,分辨率正常切换,画面正常,无抖动、无闪烁\r\n");
	CSTCPrint("[UDI2SCREENTEST]::No,分辨率不能正常切换或画面不正常 \r\n");
	CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes( ), "步骤3失败");

	 //如果平台支持1080P 50HZ
	if (IsScreenSupport(scScreenDevice, EM_UDISCREEN_RESOLUTION_1080P_50HZ ))
	{
		scResolution[0] = EM_UDISCREEN_RESOLUTION_PAL;
		scResolution[1] = EM_UDISCREEN_RESOLUTION_1080P_50HZ;
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤4失败"); 
		CSTCPrint("[UDI2SCREENTEST]::Yes,分辨率正常切换,画面正常,无抖动、无闪烁\r\n");
		CSTCPrint("[UDI2SCREENTEST]::No,分辨率不能正常切换或画面不正常 \r\n");
		CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes( ), "步骤4失败");
	}

	CSTCPrint("[UDI2SCREENTEST] 下面将对设备进行制式转换自动化测试...\n");
	CSTKWaitAnyKey();

	scResolution[0] = EM_UDISCREEN_RESOLUTION_PAL;
	scResolution[1] = EM_UDISCREEN_RESOLUTION_720P_50HZ;
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤5失败");
	CSUDIOSThreadSleep(3000);

	scResolution[0] = EM_UDISCREEN_RESOLUTION_NTSC;
	scResolution[1] = EM_UDISCREEN_RESOLUTION_720P;
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤6失败"); 
	CSUDIOSThreadSleep(3000);

	scResolution[0] = EM_UDISCREEN_RESOLUTION_PAL;
	scResolution[1] = EM_UDISCREEN_RESOLUTION_720P_50HZ;
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤7失败"); 
	CSUDIOSThreadSleep(3000);

	scResolution[0] = EM_UDISCREEN_RESOLUTION_NTSC;
	scResolution[1] = EM_UDISCREEN_RESOLUTION_1080I_50HZ;
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤8失败"); 
	CSUDIOSThreadSleep(3000);

	//如果平台支持1080i 60HZ
	if (IsScreenSupport(scScreenDevice, EM_UDISCREEN_RESOLUTION_1080I ))
	{
		scResolution[0] = EM_UDISCREEN_RESOLUTION_PAL;
		scResolution[1] = EM_UDISCREEN_RESOLUTION_1080I;
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤9失败"); 
		CSUDIOSThreadSleep(3000);

		scResolution[0] = EM_UDISCREEN_RESOLUTION_NTSC;
		scResolution[1] = EM_UDISCREEN_RESOLUTION_1080I;
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤10失败"); 
		CSUDIOSThreadSleep(3000);
	}

	//如果平台支持1080p 50HZ
	if (IsScreenSupport(scScreenDevice, EM_UDISCREEN_RESOLUTION_1080P_50HZ ))
	{
		scResolution[0] = EM_UDISCREEN_RESOLUTION_NTSC;
		scResolution[1] = EM_UDISCREEN_RESOLUTION_1080P_50HZ;
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤11失败"); 
		CSUDIOSThreadSleep(3000);
	}

	//如果平台支持1080P 60HZ
	if (IsScreenSupport(scScreenDevice, EM_UDISCREEN_RESOLUTION_1080P ))
	{
		scResolution[0] = EM_UDISCREEN_RESOLUTION_PAL;
		scResolution[1] = EM_UDISCREEN_RESOLUTION_1080P;
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤12失败"); 
		CSUDIOSThreadSleep(3000);

		scResolution[0] = EM_UDISCREEN_RESOLUTION_NTSC;
		scResolution[1] = EM_UDISCREEN_RESOLUTION_1080P;
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤13失败"); 
		CSUDIOSThreadSleep(3000);
	}

	CSTK_FATAL_POINT
	{
		 //恢复测试前分辨
		if ((g_scResolution[0]>EM_UDISCREEN_RESOLUTION_INVALID && g_scResolution[0]<=EM_UDISCREEN_RESOLUTION_576P)
				&& (g_scResolution[1]>=EM_UDISCREEN_RESOLUTION_720P && g_scResolution[0]<EM_UDISCREEN_RESOLUTION_NUM))
		{
			scResolution[0] = g_scResolution[0];
			scResolution[1] = g_scResolution[1];
			CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤14失败\n");
		}

		if (CSUDI_NULL != hPlayer)
		{
			CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERStop(hPlayer), "停止播放节目失败");
			CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERClose(hPlayer), "关闭播放器失败");
			hPlayer = CSUDI_NULL;
		}
	}

	return CSUDI_TRUE;
}

//@CASEGROUP:CSUDISCREENSetResolution 
//@DESCRIPTION:测试节目源是高清EM_UDISCREEN_RESOLUTION_720P的情况下,对标清和高清设备进行标清制式转换,该测试用例主要用于高清平台
//@PRECONDITION:平台支持高清解码
//@INPUT:1. 播放高清720P节目源
//@EXPECTATION:1. 当对设备设置分辨率时分辨率发生正常切换
//@EXPECTATION:2. 当标清和高清输出的场频和节目源的场频一致,即均为60HZ时要求画面无任何异常
//@EXPECTATION:3. 当输出标清和高清场频不一致时画面可能有异常,但是要求不死机
//@REMARK:1.设备是否支持某种分辨率是通过CSUDISCREENGetCapability获得
//@REMARK:2.如果平台支持1080P,当设置输出为1080P时,确认画面是否正常需要电视也支持1080P
//@EXECUTIONFLOW:1.调用player模块接口播放标清NTSC码流,详细步骤清参见CSTC_SCREEN_TEST_IT_SetResolution_0006步骤1
//@EXECUTIONFLOW:2. 设置标清视频输出格式为EM_UDISCREEN_RESOLUTION_NTSC,高清为EM_UDISCREEN_RESOLUTION_720P,期望分辨率正常切换,画面正常
//@EXECUTIONFLOW:3. 如果平台支持1080i 60HZ,设置标清视频输出格式为EM_UDISCREEN_RESOLUTION_NTSC,高清为EM_UDISCREEN_RESOLUTION_1080I,期望分辨率正常切换,画面正常
//@EXECUTIONFLOW:4. 如果平台支持1080p 60HZ,设置标清视频输出格式为EM_UDISCREEN_RESOLUTION_NTSC,高清为EM_UDISCREEN_RESOLUTION_1080P,期望分辨率正常切换,画面正常
//@EXECUTIONFLOW:5. 设置标清视频输出格式为EM_UDISCREEN_RESOLUTION_PAL,高清为EM_UDISCREEN_RESOLUTION_1080I_50HZ,期望不死机,sleep 1S
//@EXECUTIONFLOW:6. 设置标清视频输出格式为EM_UDISCREEN_RESOLUTION_NTSC,高清为EM_UDISCREEN_RESOLUTION_1080I_50HZ,期望不死机,sleep 1S
//@EXECUTIONFLOW:7. 设置标清视频输出格式为EM_UDISCREEN_RESOLUTION_PAL,高清为EM_UDISCREEN_RESOLUTION_720P_50HZ,期望不死机,sleep 1S
//@EXECUTIONFLOW:8. 设置标清视频输出格式为EM_UDISCREEN_RESOLUTION_NTSC,高清为EM_UDISCREEN_RESOLUTION_720P_50HZ,期望不死机,sleep 1S
//@EXECUTIONFLOW:9. 设置标清视频输出格式为EM_UDISCREEN_RESOLUTION_PAL,高清为EM_UDISCREEN_RESOLUTION_720P,期望不死机,sleep 1S
//@EXECUTIONFLOW:10. 如果平台支持1080i 60HZ,设置标清视频输出格式为EM_UDISCREEN_RESOLUTION_PAL,高清为EM_UDISCREEN_RESOLUTION_1080I,期望不死机,sleep 1S
//@EXECUTIONFLOW:11. 如果平台支持1080p 50HZ,设置标清视频输出格式为EM_UDISCREEN_RESOLUTION_PAL,高清为EM_UDISCREEN_RESOLUTION_1080P_50HZ,期望不死机,sleep 1S
//@EXECUTIONFLOW:12. 如果平台支持1080p 50HZ,设置标清视频输出格式为EM_UDISCREEN_RESOLUTION_NTSC,高清为EM_UDISCREEN_RESOLUTION_1080P_50HZ,期望不死机,sleep 1S
//@EXECUTIONFLOW:13. 如果平台支持1080p 60HZ,设置标清视频输出格式为EM_UDISCREEN_RESOLUTION_PAL,高清为EM_UDISCREEN_RESOLUTION_1080P,期望不死机,sleep 1S
//@EXECUTIONFLOW:14. 恢复测试前分辨率设置,该分辨为该模块测试初始化时获得的高标清分辨率
//@EXECUTIONFLOW:15. 调用CSUDIPLAYERStop停止测试节目播放
//@EXECUTIONFLOW:16. 调用CSUDIPLAYERClose关闭测试播放器
CSUDI_BOOL CSTC_SCREEN_TEST_IT_SetResolution_0007( void )
{
	CSUDI_HANDLE hPlayer = CSUDI_NULL;
	CSUDISCREENType_E scScreenDevice[2] = {EM_UDI_VOUT_DEVICE_SD, EM_UDI_VOUT_DEVICE_HD};
	CSUDISCREENResolution_E scResolution[2];

	if (NotSupportHD())
	{
		CSTCPrint("[UDI2SCREENTEST]  该用例用于测试高清平台\r\n");
		return CSUDI_TRUE;
	}

	hPlayer = PlayHDProgram( EM_UDI_VID_STREAM_MPEG2, EM_UDISCREEN_RESOLUTION_720P);  //测试节目源是高清EM_UDISCREEN_RESOLUTION_720P

	CSTK_ASSERT_TRUE_FATAL(CSUDI_NULL != hPlayer, "启动节目播放失败\n");

	CSTCPrint("[UDI2SCREENTEST]::视频节目播放画面是否正常?\r\n");
	CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes( ), "步骤1失败\n");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENGetResolution(scScreenDevice[0], &g_scResolution[0]), "获取测试前分辨率失败\n");
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENGetResolution(scScreenDevice[1], &g_scResolution[1]), "获取测试前分辨率失败\n");

	CSTCPrint("********请确认已连接SD/HD 两种输出端***********\n");
	CSTKWaitAnyKey();

	scResolution[0] = EM_UDISCREEN_RESOLUTION_NTSC;
	scResolution[1] = EM_UDISCREEN_RESOLUTION_720P;
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤2失败\n");
	CSTCPrint("[UDI2SCREENTEST]::Yes,分辨率正常切换,画面正常,无抖动、无闪烁\r\n");
	CSTCPrint("[UDI2SCREENTEST]::No,分辨率不能正常切换或画面不正常 \r\n");
	CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes( ), "步骤2失败\n");

	//如果平台支持1080i 60HZ
	if (IsScreenSupport(scScreenDevice, EM_UDISCREEN_RESOLUTION_1080I ))
	{
		scResolution[0] = EM_UDISCREEN_RESOLUTION_NTSC;
		scResolution[1] = EM_UDISCREEN_RESOLUTION_1080I;
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤3失败\n"); 
		CSTCPrint("[UDI2SCREENTEST]::Yes,分辨率正常切换,画面正常,无抖动、无闪烁\r\n");
		CSTCPrint("[UDI2SCREENTEST]::No,分辨率不能正常切换或画面不正常 \r\n");
		CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes( ), "步骤3失败\n");
	}

	//如果平台支持1080p 60HZ
	if (IsScreenSupport(scScreenDevice, EM_UDISCREEN_RESOLUTION_1080P ))
	{
		scResolution[0] = EM_UDISCREEN_RESOLUTION_NTSC;
		scResolution[1] = EM_UDISCREEN_RESOLUTION_1080P;
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤4失败\n");
		CSTCPrint("[UDI2SCREENTEST]::Yes,分辨率正常切换,画面正常,无抖动、无闪烁\r\n");
		CSTCPrint("[UDI2SCREENTEST]::No,分辨率不能正常切换或画面不正常 \r\n");
		CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes( ), "步骤4失败\n");
	}

	CSTCPrint("[UDI2SCREENTEST] 下面将对设备进行制式转换自动化测试...\n");
	CSTKWaitAnyKey();

	scResolution[0] = EM_UDISCREEN_RESOLUTION_PAL;
	scResolution[1] = EM_UDISCREEN_RESOLUTION_1080I_50HZ;
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤5失败\n");
	CSUDIOSThreadSleep(3000);

	scResolution[0] = EM_UDISCREEN_RESOLUTION_NTSC;
	scResolution[1] = EM_UDISCREEN_RESOLUTION_1080I_50HZ;
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤6失败\n"); 
	CSUDIOSThreadSleep(3000);

	scResolution[0] = EM_UDISCREEN_RESOLUTION_PAL;
	scResolution[1] = EM_UDISCREEN_RESOLUTION_720P_50HZ;
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤7失败\n"); 
	CSUDIOSThreadSleep(3000);

	scResolution[0] = EM_UDISCREEN_RESOLUTION_NTSC;
	scResolution[1] = EM_UDISCREEN_RESOLUTION_720P_50HZ;
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤8失败\n"); 
	CSUDIOSThreadSleep(3000);

	scResolution[0] = EM_UDISCREEN_RESOLUTION_PAL;
	scResolution[1] = EM_UDISCREEN_RESOLUTION_720P;
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤9失败\n"); 
	CSUDIOSThreadSleep(3000);

	//如果平台支持1080i 60HZ
	if (IsScreenSupport(scScreenDevice, EM_UDISCREEN_RESOLUTION_1080I ))
	{
		scResolution[0] = EM_UDISCREEN_RESOLUTION_PAL;
		scResolution[1] = EM_UDISCREEN_RESOLUTION_1080I;
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤10失败\n"); 
		CSUDIOSThreadSleep(3000);
	}

	//如果平台支持1080p 50HZ
	if (IsScreenSupport(scScreenDevice, EM_UDISCREEN_RESOLUTION_1080P_50HZ ))
	{
		scResolution[0] = EM_UDISCREEN_RESOLUTION_PAL;
		scResolution[1] = EM_UDISCREEN_RESOLUTION_1080P_50HZ;
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤11失败\n"); 
		CSUDIOSThreadSleep(3000);

		scResolution[0] = EM_UDISCREEN_RESOLUTION_NTSC;
		scResolution[1] = EM_UDISCREEN_RESOLUTION_1080P_50HZ;
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤12失败\n"); 
		CSUDIOSThreadSleep(3000);
	}

	//如果平台支持1080P 60HZ
	if (IsScreenSupport(scScreenDevice, EM_UDISCREEN_RESOLUTION_1080P ))
	{
		scResolution[0] = EM_UDISCREEN_RESOLUTION_PAL;
		scResolution[1] = EM_UDISCREEN_RESOLUTION_1080P;
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤13失败\n"); 
		CSUDIOSThreadSleep(3000);
	}

	CSTK_FATAL_POINT
	{
		//恢复测试前分辨率
		if ((g_scResolution[0]>EM_UDISCREEN_RESOLUTION_INVALID && g_scResolution[0]<=EM_UDISCREEN_RESOLUTION_576P)
				&& (g_scResolution[1]>=EM_UDISCREEN_RESOLUTION_720P && g_scResolution[0]<EM_UDISCREEN_RESOLUTION_NUM))
		{
			scResolution[0] = g_scResolution[0];
			scResolution[1] = g_scResolution[1];
			CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤14失败\n");
		}
		
		if (CSUDI_NULL != hPlayer)
		{
			CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERStop(hPlayer), "停止播放节目失败\n");
			CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERClose(hPlayer), "关闭播放器失败\n");
			hPlayer = CSUDI_NULL;
		}
	}

	return CSUDI_TRUE;
}

//@CASEGROUP:CSUDISCREENSetResolution 
//@DESCRIPTION:测试节目源是高清EM_UDISCREEN_RESOLUTION_1080I_50HZ的情况下,对标清和高清设备进行标清制式转换,该测试用例主要用于高清平台
//@PRECONDITION:平台支持高清解码
//@INPUT:1. 播放高清1080I_50HZ节目源
//@EXPECTATION:1. 当对设备设置分辨率时分辨率发生正常切换
//@EXPECTATION:2. 当标清和高清输出的场频和节目源的场频一致,即均为60HZ时要求画面无任何异常
//@EXPECTATION:3. 当输出标清和高清场频不一致时画面可能有异常,但是要求不死机
//@REMARK:1.设备是否支持某种分辨率是通过CSUDISCREENGetCapability获得
//@REMARK:2.如果平台支持1080P,当设置输出为1080P时,确认画面是否正常需要电视也支持1080P
//@EXECUTIONFLOW:1.调用player模块接口播放高清720P_50HZ码流,详细步骤清参见CSTC_SCREEN_TEST_IT_SetResolution_0006步骤1
//@EXECUTIONFLOW:2. 设置标清视频输出格式为EM_UDISCREEN_RESOLUTION_PAL,高清为EM_UDISCREEN_RESOLUTION_720P_50HZ,期望分辨率正常切换,画面正常
//@EXECUTIONFLOW:3. 设置标清视频输出格式为EM_UDISCREEN_RESOLUTION_PAL,高清为EM_UDISCREEN_RESOLUTION_1080I_50HZ,期望分辨率正常切换,画面正常
//@EXECUTIONFLOW:4. 如果平台支持1080p 50HZ,设置标清视频输出格式为EM_UDISCREEN_RESOLUTION_PAL,高清为EM_UDISCREEN_RESOLUTION_1080P_50HZ,期望分辨率正常切换,画面正常
//@EXECUTIONFLOW:5. 设置标清视频输出格式为EM_UDISCREEN_RESOLUTION_NTSC,高清为EM_UDISCREEN_RESOLUTION_720P_50HZ,期望不死机,sleep 1S
//@EXECUTIONFLOW:6. 设置标清视频输出格式为EM_UDISCREEN_RESOLUTION_PAL,高清为EM_UDISCREEN_RESOLUTION_720P,期望不死机,sleep 1S
//@EXECUTIONFLOW:7. 设置标清视频输出格式为EM_UDISCREEN_RESOLUTION_NTSC,高清为EM_UDISCREEN_RESOLUTION_720P,期望不死机,sleep 1S
//@EXECUTIONFLOW:8. 设置标清视频输出格式为EM_UDISCREEN_RESOLUTION_NTSC,高清为EM_UDISCREEN_RESOLUTION_1080I_50HZ,期望不死机,sleep 1S
//@EXECUTIONFLOW:9. 如果平台支持1080i 60HZ,设置标清视频输出格式为EM_UDISCREEN_RESOLUTION_PAL,高清为EM_UDISCREEN_RESOLUTION_1080I,期望不死机,sleep 1S
//@EXECUTIONFLOW:10. 如果平台支持1080i 60HZ,设置标清视频输出格式为EM_UDISCREEN_RESOLUTION_NTSC,高清为EM_UDISCREEN_RESOLUTION_1080I,期望不死机,sleep 1S
//@EXECUTIONFLOW:11. 如果平台支持1080p 50HZ,设置标清视频输出格式为EM_UDISCREEN_RESOLUTION_NTSC,高清为EM_UDISCREEN_RESOLUTION_1080P_50HZ,期望不死机,sleep 1S
//@EXECUTIONFLOW:12. 如果平台支持1080p 60HZ,设置标清视频输出格式为EM_UDISCREEN_RESOLUTION_PAL,高清为EM_UDISCREEN_RESOLUTION_1080P,期望不死机,sleep 1S
//@EXECUTIONFLOW:13. 如果平台支持1080p 60HZ,设置标清视频输出格式为EM_UDISCREEN_RESOLUTION_NTSC,高清为EM_UDISCREEN_RESOLUTION_1080P,期望不死机,sleep 1S
//@EXECUTIONFLOW:14. 恢复测试前分辨率设置,该分辨为该模块测试初始化时获得的高标清分辨率
//@EXECUTIONFLOW:15. 调用CSUDIPLAYERStop停止测试节目播放
//@EXECUTIONFLOW:16. 调用CSUDIPLAYERClose关闭测试播放器
CSUDI_BOOL CSTC_SCREEN_TEST_IT_SetResolution_0008( void )
{
	CSUDI_HANDLE hPlayer = CSUDI_NULL;
	CSUDISCREENType_E scScreenDevice[2] = {EM_UDI_VOUT_DEVICE_SD, EM_UDI_VOUT_DEVICE_HD};
	CSUDISCREENResolution_E scResolution[2];

	if (NotSupportHD())
	{
		CSTCPrint("[UDI2SCREENTEST]  该用例用于测试高清平台\r\n");
		return CSUDI_TRUE;
	}

	hPlayer = PlayHDProgram( EM_UDI_VID_STREAM_H264, EM_UDISCREEN_RESOLUTION_1080I_50HZ);  //测试节目源是高清EM_UDISCREEN_RESOLUTION_1080I_50HZ

	CSTK_ASSERT_TRUE_FATAL(CSUDI_NULL != hPlayer, "启动节目播放失败\n");

	CSTCPrint("[UDI2SCREENTEST]::视频节目播放画面是否正常?\r\n");
	CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes( ), "步骤1失败\n");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENGetResolution(scScreenDevice[0], &g_scResolution[0]), "获取测试前分辨率失败\n");
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENGetResolution(scScreenDevice[1], &g_scResolution[1]), "获取测试前分辨率失败\n");

	CSTCPrint("********请确认已连接SD/HD 两种输出端***********\n");
	CSTKWaitAnyKey();

	scResolution[0] = EM_UDISCREEN_RESOLUTION_PAL;
	scResolution[1] = EM_UDISCREEN_RESOLUTION_720P_50HZ;
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤2失败\n");
	CSTCPrint("[UDI2SCREENTEST]::Yes,分辨率正常切换,画面正常,无抖动、无闪烁\r\n");
	CSTCPrint("[UDI2SCREENTEST]::No,分辨率不能正常切换或画面不正常 \r\n");
	CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes( ), "步骤2失败\n");

	scResolution[0] = EM_UDISCREEN_RESOLUTION_PAL;
	scResolution[1] = EM_UDISCREEN_RESOLUTION_1080I_50HZ;
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤3失败\n"); 
	CSTCPrint("[UDI2SCREENTEST]::Yes,分辨率正常切换,画面正常,无抖动、无闪烁\r\n");
	CSTCPrint("[UDI2SCREENTEST]::No,分辨率不能正常切换或画面不正常 \r\n");
	CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes( ), "步骤3失败\n");

	//如果平台支持1080p 50HZ
	if (IsScreenSupport(scScreenDevice, EM_UDISCREEN_RESOLUTION_1080P_50HZ ))
	{
		scResolution[0] = EM_UDISCREEN_RESOLUTION_PAL;
		scResolution[1] = EM_UDISCREEN_RESOLUTION_1080P_50HZ;
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤4失败\n"); 
		CSTCPrint("[UDI2SCREENTEST]::Yes,分辨率正常切换,画面正常,无抖动、无闪烁\r\n");
		CSTCPrint("[UDI2SCREENTEST]::No,分辨率不能正常切换或画面不正常 \r\n");
		CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes( ), "步骤4失败\n");
	}

	CSTCPrint("[UDI2SCREENTEST] 下面将对设备进行制式转换自动化测试...\n");
	CSTKWaitAnyKey();

	scResolution[0] = EM_UDISCREEN_RESOLUTION_NTSC;
	scResolution[1] = EM_UDISCREEN_RESOLUTION_720P_50HZ;
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤5失败\n");
	CSUDIOSThreadSleep(3000);

	scResolution[0] = EM_UDISCREEN_RESOLUTION_PAL;
	scResolution[1] = EM_UDISCREEN_RESOLUTION_720P;
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤6失败\n"); 
	CSUDIOSThreadSleep(3000);

	scResolution[0] = EM_UDISCREEN_RESOLUTION_NTSC;
	scResolution[1] = EM_UDISCREEN_RESOLUTION_720P;
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤7失败\n"); 
	CSUDIOSThreadSleep(3000);

	scResolution[0] = EM_UDISCREEN_RESOLUTION_NTSC;
	scResolution[1] = EM_UDISCREEN_RESOLUTION_1080I_50HZ;
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤8失败\n"); 
	CSUDIOSThreadSleep(3000);

	//如果平台支持1080i 60HZ
	if (IsScreenSupport(scScreenDevice, EM_UDISCREEN_RESOLUTION_1080I ))
	{
		scResolution[0] = EM_UDISCREEN_RESOLUTION_PAL;
		scResolution[1] = EM_UDISCREEN_RESOLUTION_1080I;
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤9失败\n"); 
		CSUDIOSThreadSleep(3000);

		scResolution[0] = EM_UDISCREEN_RESOLUTION_NTSC;
		scResolution[1] = EM_UDISCREEN_RESOLUTION_1080I;
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤10失败\n"); 
		CSUDIOSThreadSleep(3000);
	}

	//如果平台支持1080p 50HZ
	if (IsScreenSupport(scScreenDevice, EM_UDISCREEN_RESOLUTION_1080P_50HZ ))
	{
		scResolution[0] = EM_UDISCREEN_RESOLUTION_NTSC;
		scResolution[1] = EM_UDISCREEN_RESOLUTION_1080P_50HZ;
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤11失败\n"); 
		CSUDIOSThreadSleep(3000);
	}

	//如果平台支持1080P 60HZ
	if (IsScreenSupport(scScreenDevice, EM_UDISCREEN_RESOLUTION_1080P ))
	{
		scResolution[0] = EM_UDISCREEN_RESOLUTION_PAL;
		scResolution[1] = EM_UDISCREEN_RESOLUTION_1080P;
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤12失败\n"); 
		CSUDIOSThreadSleep(3000);

		scResolution[0] = EM_UDISCREEN_RESOLUTION_NTSC;
		scResolution[1] = EM_UDISCREEN_RESOLUTION_1080P;
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤13失败\n"); 
		CSUDIOSThreadSleep(3000);
	}

	CSTK_FATAL_POINT
	{
		//恢复测试前分辨率
		if ((g_scResolution[0]>EM_UDISCREEN_RESOLUTION_INVALID && g_scResolution[0]<=EM_UDISCREEN_RESOLUTION_576P)
				&& (g_scResolution[1]>=EM_UDISCREEN_RESOLUTION_720P && g_scResolution[0]<EM_UDISCREEN_RESOLUTION_NUM))
		{
			scResolution[0] = g_scResolution[0];
			scResolution[1] = g_scResolution[1];
			CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤14失败\n");
		}
		
		if (CSUDI_NULL != hPlayer)
		{
			CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERStop(hPlayer), "停止播放节目失败\n");
			CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERClose(hPlayer), "关闭播放器失败\n");
			hPlayer = CSUDI_NULL;
		}
	}

	return CSUDI_TRUE;
}

//@CASEGROUP:CSUDISCREENSetResolution 
//@DESCRIPTION:测试节目源是高清EM_UDISCREEN_RESOLUTION_1080I的情况下,对标清和高清设备进行标清制式转换,该测试用例主要用于高清平台
//@PRECONDITION:平台支持高清解码
//@INPUT:1. 播放高清1080I 60HZ节目源
//@EXPECTATION:1. 当对设备设置分辨率时分辨率发生正常切换
//@EXPECTATION:2. 当标清和高清输出的场频和节目源的场频一致,即均为60HZ时要求画面无任何异常
//@EXPECTATION:3. 当输出标清和高清场频不一致时画面可能有异常,但是要求不死机
//@REMARK:1.设备是否支持某种分辨率是通过CSUDISCREENGetCapability获得
//@REMARK:2.如果平台支持1080P,当设置输出为1080P时,确认画面是否正常需要电视也支持1080P
//@EXECUTIONFLOW:1.调用player模块接口播放标清NTSC码流,详细步骤清参见CSTC_SCREEN_TEST_IT_SetResolution_0006步骤1
//@EXECUTIONFLOW:2. 设置标清视频输出格式为EM_UDISCREEN_RESOLUTION_NTSC,高清为EM_UDISCREEN_RESOLUTION_720P,期望分辨率正常切换,画面正常
//@EXECUTIONFLOW:3. 如果平台支持1080i 60HZ,设置标清视频输出格式为EM_UDISCREEN_RESOLUTION_NTSC,高清为EM_UDISCREEN_RESOLUTION_1080I,期望分辨率正常切换,画面正常
//@EXECUTIONFLOW:4. 如果平台支持1080p 60HZ,设置标清视频输出格式为EM_UDISCREEN_RESOLUTION_NTSC,高清为EM_UDISCREEN_RESOLUTION_1080P,期望分辨率正常切换,画面正常
//@EXECUTIONFLOW:5. 设置标清视频输出格式为EM_UDISCREEN_RESOLUTION_PAL,高清为EM_UDISCREEN_RESOLUTION_1080I_50HZ,期望不死机,sleep 1S
//@EXECUTIONFLOW:6. 设置标清视频输出格式为EM_UDISCREEN_RESOLUTION_NTSC,高清为EM_UDISCREEN_RESOLUTION_1080I_50HZ,期望不死机,sleep 1S
//@EXECUTIONFLOW:7. 设置标清视频输出格式为EM_UDISCREEN_RESOLUTION_PAL,高清为EM_UDISCREEN_RESOLUTION_720P_50HZ,期望不死机,sleep 1S
//@EXECUTIONFLOW:8. 设置标清视频输出格式为EM_UDISCREEN_RESOLUTION_NTSC,高清为EM_UDISCREEN_RESOLUTION_720P_50HZ,期望不死机,sleep 1S
//@EXECUTIONFLOW:9. 设置标清视频输出格式为EM_UDISCREEN_RESOLUTION_PAL,高清为EM_UDISCREEN_RESOLUTION_720P,期望不死机,sleep 1S
//@EXECUTIONFLOW:10. 如果平台支持1080i 60HZ,设置标清视频输出格式为EM_UDISCREEN_RESOLUTION_PAL,高清为EM_UDISCREEN_RESOLUTION_1080I,期望不死机,sleep 1S
//@EXECUTIONFLOW:11. 如果平台支持1080p 50HZ,设置标清视频输出格式为EM_UDISCREEN_RESOLUTION_PAL,高清为EM_UDISCREEN_RESOLUTION_1080P_50HZ,期望不死机,sleep 1S
//@EXECUTIONFLOW:12. 如果平台支持1080p 50HZ,设置标清视频输出格式为EM_UDISCREEN_RESOLUTION_NTSC,高清为EM_UDISCREEN_RESOLUTION_1080P_50HZ,期望不死机,sleep 1S
//@EXECUTIONFLOW:13. 如果平台支持1080p 60HZ,设置标清视频输出格式为EM_UDISCREEN_RESOLUTION_PAL,高清为EM_UDISCREEN_RESOLUTION_1080P,期望不死机,sleep 1S
//@EXECUTIONFLOW:14. 恢复测试前分辨率设置,该分辨为该模块测试初始化时获得的高标清分辨率
//@EXECUTIONFLOW:15. 调用CSUDIPLAYERStop停止测试节目播放
//@EXECUTIONFLOW:16. 调用CSUDIPLAYERClose关闭测试播放器
CSUDI_BOOL CSTC_SCREEN_TEST_IT_SetResolution_0009( void )
{
	CSUDI_HANDLE hPlayer = CSUDI_NULL;
	CSUDISCREENType_E scScreenDevice[2] = {EM_UDI_VOUT_DEVICE_SD, EM_UDI_VOUT_DEVICE_HD};
	CSUDISCREENResolution_E scResolution[2];

	if (NotSupportHD())
	{
		CSTCPrint("[UDI2SCREENTEST]  该用例用于测试高清平台\r\n");
		return CSUDI_TRUE;
	}

	hPlayer = PlayHDProgram( EM_UDI_VID_STREAM_H264, EM_UDISCREEN_RESOLUTION_1080I);  //测试节目源是高清EM_UDISCREEN_RESOLUTION_1080I

	CSTK_ASSERT_TRUE_FATAL(CSUDI_NULL != hPlayer, "启动节目播放失败\n");

	CSTCPrint("[UDI2SCREENTEST]::视频节目播放画面是否正常?\r\n");
	CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes( ), "步骤1失败\n");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENGetResolution(scScreenDevice[0], &g_scResolution[0]), "获取测试前分辨率失败\n");
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENGetResolution(scScreenDevice[1], &g_scResolution[1]), "获取测试前分辨率失败\n");

	CSTCPrint("********请确认已连接SD/HD 两种输出端***********\n");
	CSTKWaitAnyKey();

	scResolution[0] = EM_UDISCREEN_RESOLUTION_NTSC;
	scResolution[1] = EM_UDISCREEN_RESOLUTION_720P;
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤2失败\n");
	CSTCPrint("[UDI2SCREENTEST]::Yes,分辨率正常切换,画面正常,无抖动、无闪烁\r\n");
	CSTCPrint("[UDI2SCREENTEST]::No,分辨率不能正常切换或画面不正常 \r\n");
	CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes( ), "步骤2失败\n");

	//如果平台支持1080i 60HZ
	if (IsScreenSupport(scScreenDevice, EM_UDISCREEN_RESOLUTION_1080I ))
	{
		scResolution[0] = EM_UDISCREEN_RESOLUTION_NTSC;
		scResolution[1] = EM_UDISCREEN_RESOLUTION_1080I;
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤3失败\n");
		CSTCPrint("[UDI2SCREENTEST]::Yes,分辨率正常切换,画面正常,无抖动、无闪烁\r\n");
		CSTCPrint("[UDI2SCREENTEST]::No,分辨率不能正常切换或画面不正常 \r\n");
		CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes( ), "步骤3失败\n");
	} 

	//如果平台支持1080p 60HZ
	if (IsScreenSupport(scScreenDevice, EM_UDISCREEN_RESOLUTION_1080P ))
	{
		scResolution[0] = EM_UDISCREEN_RESOLUTION_NTSC;
		scResolution[1] = EM_UDISCREEN_RESOLUTION_1080P;
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤4失败\n"); 
		CSTCPrint("[UDI2SCREENTEST]::Yes,分辨率正常切换,画面正常,无抖动、无闪烁\r\n");
		CSTCPrint("[UDI2SCREENTEST]::No,分辨率不能正常切换或画面不正常 \r\n");
		CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes( ), "步骤4失败\n");
	}

	CSTCPrint("[UDI2SCREENTEST] 下面将对设备进行制式转换自动化测试...\n");
	CSTKWaitAnyKey();

	scResolution[0] = EM_UDISCREEN_RESOLUTION_PAL;
	scResolution[1] = EM_UDISCREEN_RESOLUTION_1080I_50HZ;
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤5失败\n");
	CSUDIOSThreadSleep(3000);

	scResolution[0] = EM_UDISCREEN_RESOLUTION_NTSC;
	scResolution[1] = EM_UDISCREEN_RESOLUTION_1080I_50HZ;
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤6失败\n"); 
	CSUDIOSThreadSleep(3000);

	scResolution[0] = EM_UDISCREEN_RESOLUTION_PAL;
	scResolution[1] = EM_UDISCREEN_RESOLUTION_720P_50HZ;
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤7失败\n"); 
	CSUDIOSThreadSleep(3000);

	scResolution[0] = EM_UDISCREEN_RESOLUTION_NTSC;
	scResolution[1] = EM_UDISCREEN_RESOLUTION_720P_50HZ;
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤8失败\n"); 
	CSUDIOSThreadSleep(3000);

	scResolution[0] = EM_UDISCREEN_RESOLUTION_PAL;
	scResolution[1] = EM_UDISCREEN_RESOLUTION_720P;
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤9失败\n"); 
	CSUDIOSThreadSleep(3000);

	//如果平台支持1080i 60HZ
	if (IsScreenSupport(scScreenDevice, EM_UDISCREEN_RESOLUTION_1080I ))
	{
		scResolution[0] = EM_UDISCREEN_RESOLUTION_PAL;
		scResolution[1] = EM_UDISCREEN_RESOLUTION_1080I;
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤10失败\n"); 
		CSUDIOSThreadSleep(3000);
	}

	//如果平台支持1080p 50HZ
	if (IsScreenSupport(scScreenDevice, EM_UDISCREEN_RESOLUTION_1080P_50HZ ))
	{
		scResolution[0] = EM_UDISCREEN_RESOLUTION_PAL;
		scResolution[1] = EM_UDISCREEN_RESOLUTION_1080P_50HZ;
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤11失败\n"); 
		CSUDIOSThreadSleep(3000);

		scResolution[0] = EM_UDISCREEN_RESOLUTION_NTSC;
		scResolution[1] = EM_UDISCREEN_RESOLUTION_1080P_50HZ;
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤12失败\n"); 
		CSUDIOSThreadSleep(3000);
	}

	//如果平台支持1080P 60HZ
	if (IsScreenSupport(scScreenDevice, EM_UDISCREEN_RESOLUTION_1080P ))
	{
		scResolution[0] = EM_UDISCREEN_RESOLUTION_PAL;
		scResolution[1] = EM_UDISCREEN_RESOLUTION_1080P;
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤13失败\n"); 
		CSUDIOSThreadSleep(3000);
	}

	CSTK_FATAL_POINT
	{
		//恢复测试前分辨率
		if ((g_scResolution[0]>EM_UDISCREEN_RESOLUTION_INVALID && g_scResolution[0]<=EM_UDISCREEN_RESOLUTION_576P)
				&& (g_scResolution[1]>=EM_UDISCREEN_RESOLUTION_720P && g_scResolution[0]<EM_UDISCREEN_RESOLUTION_NUM))
		{
			scResolution[0] = g_scResolution[0];
			scResolution[1] = g_scResolution[1];
			CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤14失败\n");
		}
		
		if (CSUDI_NULL != hPlayer)
		{
			CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERStop(hPlayer), "停止播放节目失败\n");
			CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERClose(hPlayer), "关闭播放器失败\n");
			hPlayer = CSUDI_NULL;
		}
	}

	return CSUDI_TRUE;
}

//@CASEGROUP:CSUDISCREENSetResolution 
//@DESCRIPTION:测试节目源是标清PAL的情况下,对标清设备进行标清制式转换,该测试用例主要用于支持国际市场的标清平台
//@PRECONDITION: 平台支持国外部分或者全部视频分辨率输出
//@INPUT:1. 播放标清PAL节目源
//@EXPECTATION: 当对设备设置分辨率时分辨率发生正常切换
//@REMARK:1.设备是否支持某种分辨率是通过CSUDISCREENGetCapability获得
//@REMARK:2.有些分辨率输出,在确认画面是否正常时,需要支持该分辨率的电视,典型的如EM_UDISCREEN_RESOLUTION_SECAM
//@REMARK:3.没有该项测试需求请直接跳过
//@EXECUTIONFLOW:1.调用player模块接口播放标清NTSC码流,详细步骤清参见CSTC_SCREEN_TEST_IT_SetResolution_0003步骤1
//@EXECUTIONFLOW:2. 罗列所有平台支持的分辨率,并等待用户选择
//@EXECUTIONFLOW:3. 设置用户选择的制式
//@EXECUTIONFLOW:4. 重复3到4步直到用户选择退出为止
//@EXECUTIONFLOW:5. 恢复测试前分辨率设置,该分辨为该模块测试初始化时获得的高标清分辨率
//@EXECUTIONFLOW:6. 调用CSUDIPLAYERStop停止测试节目播放
//@EXECUTIONFLOW:7. 调用CSUDIPLAYERClose关闭测试播放器
CSUDI_BOOL CSTC_SCREEN_TEST_IT_SetResolution_0010( void )
{
	CSUDISCREENCapability_S sCapabilityInfo;
	CSUDISCREENType_E eScreenDevice = EM_UDI_VOUT_DEVICE_SD;
	CSUDISCREENResolution_E scResolution[2];
	CSUDI_HANDLE hPlayer = CSUDI_NULL;
	char arr[10];
	int j = 0;
	int k= 0;

	CSTCPrint("[UDI2SCREENTEST]该测试用例主要用于支持国际市场的标清平台\n");

	hPlayer = PlaySDProgram( EM_UDI_VID_STREAM_MPEG2, EM_UDISCREEN_RESOLUTION_PAL);  //测试节目
	CSTK_ASSERT_TRUE_FATAL(CSUDI_NULL != hPlayer, "启动节目播放失败\n");

	CSTCPrint("[UDI2SCREENTEST]::视频节目播放画面是否正常? \r\n");
	CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes( ), "步骤1失败\n");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENGetResolution(eScreenDevice, &g_scResolution[0]), "获取测试前分辨率失败\n");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENGetCapability( eScreenDevice, &sCapabilityInfo), "获取设备分辨率设置能力失败\n");

	do
	{
		memset(arr,0,sizeof(arr));
		for (k=0; k<EM_UDISCREEN_RESOLUTION_NUM; k++)
		{
			switch (sCapabilityInfo.m_eResolution[k])
			{
				case EM_UDISCREEN_RESOLUTION_NTSC: CSTCPrint("%d: EM_UDISCREEN_RESOLUTION_NTSC \n", k); break;
				case EM_UDISCREEN_RESOLUTION_NTSC_443: CSTCPrint("%d: EM_UDISCREEN_RESOLUTION_NTSC_443 \n", k); break;
				case EM_UDISCREEN_RESOLUTION_NTSC_JAPAN: CSTCPrint("%d: EM_UDISCREEN_RESOLUTION_NTSC_JAPAN \n", k); break;
				case EM_UDISCREEN_RESOLUTION_PAL: CSTCPrint("%d: EM_UDISCREEN_RESOLUTION_PAL \n", k); break;
				case EM_UDISCREEN_RESOLUTION_PAL_M: CSTCPrint("%d: EM_UDISCREEN_RESOLUTION_PAL_M \n", k); break;
				case EM_UDISCREEN_RESOLUTION_PAL_N: CSTCPrint("%d: EM_UDISCREEN_RESOLUTION_PAL_N \n", k); break;
				case EM_UDISCREEN_RESOLUTION_PAL_NC: CSTCPrint("%d: EM_UDISCREEN_RESOLUTION_PAL_NC \n", k); break;
				case EM_UDISCREEN_RESOLUTION_PAL_B: CSTCPrint("%d: EM_UDISCREEN_RESOLUTION_PAL_B \n", k); break;
				case EM_UDISCREEN_RESOLUTION_PAL_B1: CSTCPrint("%d: EM_UDISCREEN_RESOLUTION_PAL_B1 \n", k); break;
				case EM_UDISCREEN_RESOLUTION_PAL_D: CSTCPrint("%d: EM_UDISCREEN_RESOLUTION_PAL_D \n", k); break;
				case EM_UDISCREEN_RESOLUTION_PAL_D1: CSTCPrint("%d: EM_UDISCREEN_RESOLUTION_PAL_D1 \n", k); break;
				case EM_UDISCREEN_RESOLUTION_PAL_G: CSTCPrint("%d: EM_UDISCREEN_RESOLUTION_PAL_G \n", k); break;
				case EM_UDISCREEN_RESOLUTION_PAL_H: CSTCPrint("%d: EM_UDISCREEN_RESOLUTION_PAL_H \n", k); break;
				case EM_UDISCREEN_RESOLUTION_PAL_K: CSTCPrint("%d: EM_UDISCREEN_RESOLUTION_PAL_K \n", k); break;
				case EM_UDISCREEN_RESOLUTION_PAL_I: CSTCPrint("%d: EM_UDISCREEN_RESOLUTION_PAL_I \n", k); break;
				case EM_UDISCREEN_RESOLUTION_SECAM: CSTCPrint("%d: EM_UDISCREEN_RESOLUTION_SECAM \n", k); break;

				case EM_UDISCREEN_RESOLUTION_480P: CSTCPrint("%d: EM_UDISCREEN_RESOLUTION_480P \n", k); break;
				case EM_UDISCREEN_RESOLUTION_576P: CSTCPrint("%d: EM_UDISCREEN_RESOLUTION_576P \n", k); break;
				case EM_UDISCREEN_RESOLUTION_720P: CSTCPrint("%d: EM_UDISCREEN_RESOLUTION_720P \n", k); break;
				case EM_UDISCREEN_RESOLUTION_720P_24HZ: CSTCPrint("%d: EM_UDISCREEN_RESOLUTION_720P_24HZ \n", k); break;
				case EM_UDISCREEN_RESOLUTION_720P_50HZ: CSTCPrint("%d: EM_UDISCREEN_RESOLUTION_720P_50HZ \n", k); break;

				case EM_UDISCREEN_RESOLUTION_1080I: CSTCPrint("%d: EM_UDISCREEN_RESOLUTION_1080I \n", k); break;
				case EM_UDISCREEN_RESOLUTION_1080I_50HZ: CSTCPrint("%d: EM_UDISCREEN_RESOLUTION_1080I_50HZ \n", k); break;

				case EM_UDISCREEN_RESOLUTION_1080P: CSTCPrint("%d: EM_UDISCREEN_RESOLUTION_1080P \n", k); break;
				case EM_UDISCREEN_RESOLUTION_1080P_24HZ: CSTCPrint("%d: EM_UDISCREEN_RESOLUTION_1080P_24HZ \n",k); break;
				case EM_UDISCREEN_RESOLUTION_1080P_25HZ: CSTCPrint("%d: EM_UDISCREEN_RESOLUTION_1080P_25HZ \n", k); break;
				case EM_UDISCREEN_RESOLUTION_1080P_30HZ: CSTCPrint("%d: EM_UDISCREEN_RESOLUTION_1080P_30HZ \n", k); break;
				case EM_UDISCREEN_RESOLUTION_1080P_50HZ: CSTCPrint("%d: EM_UDISCREEN_RESOLUTION_1080P_50HZ \n", k); break;
				case EM_UDISCREEN_RESOLUTION_1250I_50HZ: CSTCPrint("%d: EM_UDISCREEN_RESOLUTION_1250I_50HZ \n", k); break;
				case EM_UDISCREEN_RESOLUTION_VESA: CSTCPrint("%d: EM_UDISCREEN_RESOLUTION_VESA \n", k); break;
				default: break;

			}

			if (EM_UDISCREEN_RESOLUTION_INVALID == sCapabilityInfo.m_eResolution[k])
			{
				break;
			}
		}

		CSTCPrint("根据屏幕显示选择相应的分辨率，按q 键 退出 \n");  

		if(CSTKGetString(arr, &j))
		{
			if((0<=j) && (j<k))
			{
				CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(&eScreenDevice, &sCapabilityInfo.m_eResolution[j], 1), "设置分辨率失败\n");
				CSTCPrint("分辨率设置是否与预期一致???\n");
				CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes(), "设置分辨率与预期不符");
			}
			else
			{
				CSTCPrint("没有对应的分辨率可以设置\r\n\n");  
			}
		}
		else
		{
			if('q' == arr[0])
			{
				CSTCPrint("退出\r\n");  
				break;
			}
			else
			{
				CSTCPrint("输入错误，请重新输入\r\n\n");  
			}
		}
	}while(1);      	

	CSTK_FATAL_POINT
	{
		//恢复测试前分辨率
		if (g_scResolution[0]>EM_UDISCREEN_RESOLUTION_INVALID && g_scResolution[0]<=EM_UDISCREEN_RESOLUTION_576P)
		{
			scResolution[0] = g_scResolution[0];
			CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(&eScreenDevice, &g_scResolution[0], 1), "步骤5失败\n");
		}
		
		if (CSUDI_NULL != hPlayer)
		{
			CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERStop(hPlayer), "停止播放节目失败\n");
			CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERClose(hPlayer), "关闭播放器失败\n");
			hPlayer = CSUDI_NULL;
		}
	}

	return CSUDI_TRUE;
}

//@CASEGROUP:CSUDISCREENSetResolution 
//@DESCRIPTION:测试节目源是标清NTSC的情况下,对标清设备进行标清制式转换,该测试用例主要用于支持国际市场的标清平台
//@PRECONDITION: 平台支持国外部分或者全部视频分辨率输出
//@INPUT:1. 播放标清NTSC节目源
//@EXPECTATION: 当对设备设置分辨率时分辨率发生正常切换
//@REMARK:1.设备是否支持某种分辨率是通过CSUDISCREENGetCapability获得
//@REMARK:2.有些分辨率输出,在确认画面是否正常时,需要支持该分辨率的电视,典型的如EM_UDISCREEN_RESOLUTION_SECAM
//@REMARK:3.没有该项测试需求请直接跳过
//@EXECUTIONFLOW:1.调用player模块接口播放标清NTSC码流,详细步骤清参见CSTC_SCREEN_TEST_IT_SetResolution_0003步骤1
//@EXECUTIONFLOW:2. 罗列所有平台支持的分辨率,并等待用户选择
//@EXECUTIONFLOW:3. 设置用户选择的制式
//@EXECUTIONFLOW:4. 重复3到4步直到用户选择退出为止
//@EXECUTIONFLOW:5. 恢复测试前分辨率设置,该分辨为该模块测试初始化时获得的高标清分辨率
//@EXECUTIONFLOW:6. 调用CSUDIPLAYERStop停止测试节目播放
//@EXECUTIONFLOW:7. 调用CSUDIPLAYERClose关闭测试播放器
CSUDI_BOOL CSTC_SCREEN_TEST_IT_SetResolution_0011( void )
{
	CSUDISCREENCapability_S sCapabilityInfo;
	CSUDISCREENType_E eScreenDevice = EM_UDI_VOUT_DEVICE_SD;
	CSUDISCREENResolution_E scResolution[2];
	CSUDI_HANDLE hPlayer = CSUDI_NULL;
	char arr[10];
	int j = 0;
	int k = 0;

	CSTCPrint("[UDI2SCREENTEST]该测试用例主要用于支持国际市场的标清平台\n");

	hPlayer = PlaySDProgram( EM_UDI_VID_STREAM_MPEG2, EM_UDISCREEN_RESOLUTION_NTSC);  //测试节目
	CSTK_ASSERT_TRUE_FATAL(CSUDI_NULL != hPlayer, "启动节目播放失败\n");

	CSTCPrint("[UDI2SCREENTEST]::视频节目播放画面是否正常?\r\n");
	CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes( ), "步骤1失败\n");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENGetCapability( eScreenDevice, &sCapabilityInfo), "获取设备分辨率设置能力失败\n");

	do
	{
		for (k=0; k<EM_UDISCREEN_RESOLUTION_NUM; k++)
		{
			switch (sCapabilityInfo.m_eResolution[k])
			{
				case EM_UDISCREEN_RESOLUTION_NTSC: CSTCPrint("%d: EM_UDISCREEN_RESOLUTION_NTSC \n", k); break;
				case EM_UDISCREEN_RESOLUTION_NTSC_443: CSTCPrint("%d: EM_UDISCREEN_RESOLUTION_NTSC_443 \n", k); break;
				case EM_UDISCREEN_RESOLUTION_NTSC_JAPAN: CSTCPrint("%d: EM_UDISCREEN_RESOLUTION_NTSC_JAPAN \n", k); break;
				case EM_UDISCREEN_RESOLUTION_PAL: CSTCPrint("%d: EM_UDISCREEN_RESOLUTION_PAL \n", k); break;
				case EM_UDISCREEN_RESOLUTION_PAL_M: CSTCPrint("%d: EM_UDISCREEN_RESOLUTION_PAL_M \n", k); break;
				case EM_UDISCREEN_RESOLUTION_PAL_N: CSTCPrint("%d: EM_UDISCREEN_RESOLUTION_PAL_N \n", k); break;
				case EM_UDISCREEN_RESOLUTION_PAL_NC: CSTCPrint("%d: EM_UDISCREEN_RESOLUTION_PAL_NC \n", k); break;
				case EM_UDISCREEN_RESOLUTION_PAL_B: CSTCPrint("%d: EM_UDISCREEN_RESOLUTION_PAL_B \n", k); break;
				case EM_UDISCREEN_RESOLUTION_PAL_B1: CSTCPrint("%d: EM_UDISCREEN_RESOLUTION_PAL_B1 \n", k); break;
				case EM_UDISCREEN_RESOLUTION_PAL_D: CSTCPrint("%d: EM_UDISCREEN_RESOLUTION_PAL_D \n", k); break;
				case EM_UDISCREEN_RESOLUTION_PAL_D1: CSTCPrint("%d: EM_UDISCREEN_RESOLUTION_PAL_D1 \n", k); break;
				case EM_UDISCREEN_RESOLUTION_PAL_G: CSTCPrint("%d: EM_UDISCREEN_RESOLUTION_PAL_G \n", k); break;
				case EM_UDISCREEN_RESOLUTION_PAL_H: CSTCPrint("%d: EM_UDISCREEN_RESOLUTION_PAL_H \n", k); break;
				case EM_UDISCREEN_RESOLUTION_PAL_K: CSTCPrint("%d: EM_UDISCREEN_RESOLUTION_PAL_K \n", k); break;
				case EM_UDISCREEN_RESOLUTION_PAL_I: CSTCPrint("%d: EM_UDISCREEN_RESOLUTION_PAL_I \n", k); break;
				case EM_UDISCREEN_RESOLUTION_SECAM: CSTCPrint("%d: EM_UDISCREEN_RESOLUTION_SECAM \n", k); break;

				case EM_UDISCREEN_RESOLUTION_480P: CSTCPrint("%d: EM_UDISCREEN_RESOLUTION_480P \n", k); break;
				case EM_UDISCREEN_RESOLUTION_576P: CSTCPrint("%d: EM_UDISCREEN_RESOLUTION_576P \n", k); break;
				case EM_UDISCREEN_RESOLUTION_720P: CSTCPrint("%d: EM_UDISCREEN_RESOLUTION_720P \n", k); break;
				case EM_UDISCREEN_RESOLUTION_720P_24HZ: CSTCPrint("%d: EM_UDISCREEN_RESOLUTION_720P_24HZ \n", k); break;
				case EM_UDISCREEN_RESOLUTION_720P_50HZ: CSTCPrint("%d: EM_UDISCREEN_RESOLUTION_720P_50HZ \n", k); break;

				case EM_UDISCREEN_RESOLUTION_1080I: CSTCPrint("%d: EM_UDISCREEN_RESOLUTION_1080I \n", k); break;
				case EM_UDISCREEN_RESOLUTION_1080I_50HZ: CSTCPrint("%d: EM_UDISCREEN_RESOLUTION_1080I_50HZ \n", k); break;

				case EM_UDISCREEN_RESOLUTION_1080P: CSTCPrint("%d: EM_UDISCREEN_RESOLUTION_1080P \n", k); break;
				case EM_UDISCREEN_RESOLUTION_1080P_24HZ: CSTCPrint("%d: EM_UDISCREEN_RESOLUTION_1080P_24HZ \n",k); break;
				case EM_UDISCREEN_RESOLUTION_1080P_25HZ: CSTCPrint("%d: EM_UDISCREEN_RESOLUTION_1080P_25HZ \n", k); break;
				case EM_UDISCREEN_RESOLUTION_1080P_30HZ: CSTCPrint("%d: EM_UDISCREEN_RESOLUTION_1080P_30HZ \n", k); break;
				case EM_UDISCREEN_RESOLUTION_1080P_50HZ: CSTCPrint("%d: EM_UDISCREEN_RESOLUTION_1080P_50HZ \n", k); break;
				case EM_UDISCREEN_RESOLUTION_1250I_50HZ: CSTCPrint("%d: EM_UDISCREEN_RESOLUTION_1250I_50HZ \n", k); break;
				case EM_UDISCREEN_RESOLUTION_VESA: CSTCPrint("%d: EM_UDISCREEN_RESOLUTION_VESA \n", k); break;
				default: break;

			}

			if (EM_UDISCREEN_RESOLUTION_INVALID == sCapabilityInfo.m_eResolution[k])
			{
				break;
			}
		}

		CSTCPrint("根据屏幕显示选择相应的分辨率，按q 键 退出 \n");  

		if(CSTKGetString(arr, &j))
		{
			if((0<=j) && (j<k))
			{
				CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(&eScreenDevice, &sCapabilityInfo.m_eResolution[j], 1), "设置分辨率失败\n");
				CSTCPrint("分辨率设置是否与预期一致???\n");
				CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes(), "设置分辨率与预期不符");
			}
			else
			{
				CSTCPrint("没有对应的分辨率可以设置\r\n\n");  
			}
		}
		else
		{
			if('q' == arr[0])
			{
				CSTCPrint("退出\r\n");  
				break;
			}
			else
			{
				CSTCPrint("输入错误，请重新输入\r\n\n");  
			}
		}
	}while(1);      	

	CSTK_FATAL_POINT
	{
		//恢复测试前分辨率
		if (g_scResolution[0]>EM_UDISCREEN_RESOLUTION_INVALID && g_scResolution[0]<=EM_UDISCREEN_RESOLUTION_576P)
		{
			scResolution[0] = g_scResolution[0];
			CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(&eScreenDevice, &g_scResolution[0], 1), "步骤5失败\n");
		}
		
		if (CSUDI_NULL != hPlayer)
		{
			CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERStop(hPlayer), "停止播放节目失败\n");
			CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERClose(hPlayer), "关闭播放器失败\n");
			hPlayer = CSUDI_NULL;
		}
	}

	return CSUDI_TRUE;
}

static void InitParam()
{
	g_nUserData = 0;
	g_eEvt = 3;
	g_eScreenDevice = 3;
}

//@CASEGROUP:CSUDISCREENSetResolution 
//@CASEGROUP:CSUDISCREENAddCallback 
//@CASEGROUP:CSUDISCREENDelCallback
//@DESCRIPTION:测试视频分辨率切换时回调函数的有效性以及正确性
//@PRECONDITION: 
//@INPUT: 正常播放一个标清PAL节目源,注册视频分辨率切换回调函数
//@EXPECTATION: 视频分辨率发生正常切换,输出无异常,并且回调函数功能正常
//@EXECUTIONFLOW:1.调用player模块接口播放标清PAL码流,详细步骤请参考测试用例CSTC_SCREEN_TEST_IT_SetResolution_0003步骤1
//@EXECUTIONFLOW:2. 调用CSUDISCREENAddCallback注册视频分辨率切换回调函数
//@EXECUTIONFLOW:3. 设置标清视频输出为EM_UDISCREEN_RESOLUTION_PAL,如果初始化时的分辨率不为EM_UDISCREEN_RESOLUTION_PAL 期望(1)回调函被调用(2)回调函数参数正确
//@EXECUTIONFLOW:(3)事件类型为EM_UDISCREEN_RESOLUTION_CHANGED (4)设备为EM_UDI_VOUT_DEVICE_SD
//@EXECUTIONFLOW:4. 设置标清视频输出为EM_UDISCREEN_RESOLUTION_NTSC,期望(1)回调函被调用(2)回调函数参数正确
//@EXECUTIONFLOW:(3)事件类型为EM_UDISCREEN_RESOLUTION_CHANGED (4)设备为EM_UDI_VOUT_DEVICE_SD
//@EXECUTIONFLOW:5. 如果平台支持高清输出设置高清视频输出为EM_UDISCREEN_RESOLUTION_1080I_50HZ,如果初始化时的分辨率不为EM_UDISCREEN_RESOLUTION_1080I_50HZ 期望(1)回调函被调用(2)回调函数参数正确
//@EXECUTIONFLOW:(3)事件类型为EM_UDISCREEN_RESOLUTION_CHANGED (4)设备为EM_UDI_VOUT_DEVICE_HD
//@EXECUTIONFLOW:6. 如果平台支持高清输出设置高清视频输出为EM_UDISCREEN_RESOLUTION_1080I_50HZ,标清为EM_UDISCREEN_RESOLUTION_PAL
//@EXECUTIONFLOW:期望(1)回调函被调用(2)回调函数参数正确(3)事件类型为EM_UDISCREEN_RESOLUTION_CHANGED (4)设备即有高清又有标清
//@EXECUTIONFLOW:7. 如果平台支持高清输出设置高清视频输出为EM_UDISCREEN_RESOLUTION_720P_50HZ,标清为EM_UDISCREEN_RESOLUTION_PAL
//@EXECUTIONFLOW:期望(1)回调函被调用(2)回调函数参数正确(3)事件类型为EM_UDISCREEN_RESOLUTION_CHANGED (4)设备只有高清没有标清
//@EXECUTIONFLOW:8. 调用CSUDISCREENDelCallback去除回调函数
//@EXECUTIONFLOW:9. 设置标清视频输出为EM_UDISCREEN_RESOLUTION_NTSC期望回调函数不被调用
//@EXECUTIONFLOW:10. 设置标清视频输出为EM_UDISCREEN_RESOLUTION_PAL,期望回调函数不被调用
//@EXECUTIONFLOW:11. 如果平台支持高清输出设置高清视频输出为EM_UDISCREEN_RESOLUTION_1080I_50HZ,期望回调函数不被调用
//@EXECUTIONFLOW:12. 如果平台支持高清输出设置高清视频输出为EM_UDISCREEN_RESOLUTION_720P_50HZ,
//@EXECUTIONFLOW:标清输出为EM_UDISCREEN_RESOLUTION_NTSC,期望回调函数不被调用
//@EXECUTIONFLOW:13. 设置标清视频输出为EM_UDISCREEN_RESOLUTION_PAL,期望回调函数不被调用
//@EXECUTIONFLOW:14. 恢复测试前分辨率设置,该分辨为该模块测试初始化时获得的高标清分辨率
//@EXECUTIONFLOW:15. 调用CSUDIPLAYERStop停止测试节目播放
//@EXECUTIONFLOW:16. 调用CSUDIPLAYERClose关闭测试播放器
CSUDI_BOOL CSTC_SCREEN_TEST_IT_SetResolution_0012( void )
{
	CSUDI_HANDLE hPlayer = CSUDI_NULL;
	CSUDISCREENType_E scScreenDevice[2] = {EM_UDI_VOUT_DEVICE_SD, EM_UDI_VOUT_DEVICE_HD};
	CSUDISCREENResolution_E scResolution[2];
	int nUserData = 0x1234;

	InitParam();

	hPlayer = PlaySDProgram(EM_UDI_VID_STREAM_MPEG2 , EM_UDISCREEN_RESOLUTION_PAL); //调用PLAYER播放测试码流

	CSTK_ASSERT_TRUE_FATAL(CSUDI_NULL != hPlayer, "步骤1，启动节目播放失败\n");
	CSTCPrint("视频节目播放画面是否正常?\n");
	CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes( ), "步骤1失败\n");

	if (NotSupportHD())
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENGetResolution(scScreenDevice[0], &g_scResolution[0]), "获取标清测试前分辨率失败\n");
	}
	else
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENGetResolution(scScreenDevice[0], &g_scResolution[0]), "获取标清测试前分辨率失败\n");
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENGetResolution(scScreenDevice[1], &g_scResolution[1]), "获取高清测试前分辨率失败\n");
	}

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENAddCallback(CSUDISCREENChangeCallback, (int*)&nUserData), "步骤2，注册回调函数失败\n");

	InitParam();
	scResolution[0] = EM_UDISCREEN_RESOLUTION_PAL;
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(&scScreenDevice[0], &scResolution[0], 1), "步骤3失败\n");

	if (scResolution[0] != g_scResolution[0])
	{
		CSTK_ASSERT_TRUE_FATAL(0x1234 == g_nUserData, "步骤3，回调用户数据不正确\n");
		CSTK_ASSERT_TRUE_FATAL(EM_UDISCREEN_RESOLUTION_CHANGED == g_eEvt, "步骤3，回调事件类型不正确\n");
		CSTK_ASSERT_TRUE_FATAL(EM_UDI_VOUT_DEVICE_SD == g_eScreenDevice, "步骤3，回调设备类型不正确\n");
	}

	InitParam();
	scResolution[0] = EM_UDISCREEN_RESOLUTION_NTSC;
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(&scScreenDevice[0], &scResolution[0], 1), "步骤4失败\n");
	CSTK_ASSERT_TRUE_FATAL(0x1234 == g_nUserData, "步骤4，回调用户数据不正确\n");
	CSTK_ASSERT_TRUE_FATAL(EM_UDISCREEN_RESOLUTION_CHANGED == g_eEvt, "步骤4，回调事件类型不正确\n");
	CSTK_ASSERT_TRUE_FATAL(EM_UDI_VOUT_DEVICE_SD == g_eScreenDevice, "步骤4，回调设备类型不正确\n");

	//如果平台支持高清输出
	if (IsScreenSupport(scScreenDevice, EM_UDISCREEN_RESOLUTION_1080I_50HZ ))
	{
		InitParam();
		scResolution[1] = EM_UDISCREEN_RESOLUTION_1080I_50HZ;
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(&scScreenDevice[1], &scResolution[1], 1), "步骤5失败\n");
		if (EM_UDISCREEN_RESOLUTION_1080I_50HZ != g_scResolution[1])
		{
			CSTK_ASSERT_TRUE_FATAL(0x1234 == g_nUserData, "步骤5，回调用户数据不正确\n");
			CSTK_ASSERT_TRUE_FATAL(EM_UDISCREEN_RESOLUTION_CHANGED == g_eEvt, "步骤5，回调事件类型不正确\n");
			CSTK_ASSERT_TRUE_FATAL(EM_UDI_VOUT_DEVICE_HD == g_eScreenDevice, "步骤5，回调设备类型不正确\n");
		}

		InitParam();
		scResolution[0] = EM_UDISCREEN_RESOLUTION_PAL;
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(&scScreenDevice[0], &scResolution[0], 1), "步骤6失败\n");
		CSTK_ASSERT_TRUE_FATAL(0x1234 == g_nUserData, "步骤6，回调用户数据不正确\n");
		CSTK_ASSERT_TRUE_FATAL(EM_UDISCREEN_RESOLUTION_CHANGED == g_eEvt, "步骤6，回调事件类型不正确\n");
		CSTK_ASSERT_TRUE_FATAL(EM_UDI_VOUT_DEVICE_SD == g_eScreenDevice, "步骤6，回调设备类型不正确\n");
	}

	if (IsScreenSupport(scScreenDevice, EM_UDISCREEN_RESOLUTION_720P_50HZ ))
	{
		InitParam();

		scResolution[0] = EM_UDISCREEN_RESOLUTION_PAL;
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(&scScreenDevice[0], &scResolution[0], 1), "步骤7失败\n");
		CSTK_ASSERT_TRUE_FATAL(0 == g_nUserData, "步骤7.1，回调用户数据不正确\n");
		CSTK_ASSERT_TRUE_FATAL(3 == g_eEvt, "步骤7.1，回调事件类型不正确\n");
		CSTK_ASSERT_TRUE_FATAL(3 == g_eScreenDevice, "步骤7.1，回调设备类型不正确\n");

		scResolution[1] = EM_UDISCREEN_RESOLUTION_720P_50HZ;
        
        CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENGetResolution(scScreenDevice[1], &g_scResolution[1]), "获取高清测试前分辨率失败\n");

        CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(&scScreenDevice[1], &scResolution[1], 1), "步骤7失败\n");

        if (EM_UDISCREEN_RESOLUTION_720P_50HZ != g_scResolution[1])
		{
			CSTK_ASSERT_TRUE_FATAL(0x1234 == g_nUserData, "步骤7.2，回调用户数据不正确\n");
			CSTK_ASSERT_TRUE_FATAL(EM_UDISCREEN_RESOLUTION_CHANGED == g_eEvt, "步骤7.2，回调事件类型不正确\n");
			CSTK_ASSERT_TRUE_FATAL(EM_UDI_VOUT_DEVICE_HD == g_eScreenDevice, "步骤7.2，回调设备类型不正确\n");
        }
	}

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENDelCallback(CSUDISCREENChangeCallback, (int*)&nUserData), "步骤8失败\n");

	InitParam();
	scResolution[0] = EM_UDISCREEN_RESOLUTION_NTSC;
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(&scScreenDevice[0], &scResolution[0], 1), "步骤9失败\n");
	CSTK_ASSERT_TRUE_FATAL(0 == g_nUserData, "步骤9，回调用户数据不正确\n");
	CSTK_ASSERT_TRUE_FATAL(3 == g_eEvt, "步骤9，回调事件类型不正确\n");
	CSTK_ASSERT_TRUE_FATAL(3 == g_eScreenDevice, "步骤9，回调设备类型不正确\n");

	scResolution[0] = EM_UDISCREEN_RESOLUTION_PAL;
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(&scScreenDevice[0], &scResolution[0], 1), "步骤10失败\n");
	CSTK_ASSERT_TRUE_FATAL(0 == g_nUserData, "步骤10，回调用户数据不正确\n");
	CSTK_ASSERT_TRUE_FATAL(3 == g_eEvt, "步骤10，回调事件类型不正确\n");
	CSTK_ASSERT_TRUE_FATAL(3 == g_eScreenDevice, "步骤10，回调设备类型不正确\n");

	//如果平台支持高清输出
	if (IsScreenSupport(scScreenDevice, EM_UDISCREEN_RESOLUTION_1080I_50HZ ))
	{
		scResolution[1] = EM_UDISCREEN_RESOLUTION_1080I_50HZ;
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(&scScreenDevice[1], &scResolution[1], 1), "步骤11失败\n");
		CSTK_ASSERT_TRUE_FATAL(0 == g_nUserData, "步骤11，回调用户数据不正确\n");
		CSTK_ASSERT_TRUE_FATAL(3 == g_eEvt, "步骤11，回调事件类型不正确\n");
		CSTK_ASSERT_TRUE_FATAL(3 == g_eScreenDevice, "步骤11，回调设备类型不正确\n");
	}

	if (IsScreenSupport(scScreenDevice, EM_UDISCREEN_RESOLUTION_720P_50HZ ))
	{
		scResolution[0] = EM_UDISCREEN_RESOLUTION_NTSC;
		scResolution[1] = EM_UDISCREEN_RESOLUTION_720P_50HZ;
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤12失败\n");
		CSTK_ASSERT_TRUE_FATAL(0 == g_nUserData, "步骤12，回调用户数据不正确\n");
		CSTK_ASSERT_TRUE_FATAL(3 == g_eEvt, "步骤12，回调事件类型不正确\n");
		CSTK_ASSERT_TRUE_FATAL(3 == g_eScreenDevice, "步骤12，回调设备类型不正确\n");
	}

	scResolution[0] = EM_UDISCREEN_RESOLUTION_PAL;
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(&scScreenDevice[0], &scResolution[0], 1), "步骤13失败\n");
	CSTK_ASSERT_TRUE_FATAL(0 == g_nUserData, "步骤13，回调用户数据不正确\n");
	CSTK_ASSERT_TRUE_FATAL(3 == g_eEvt, "步骤13，回调事件类型不正确\n");
	CSTK_ASSERT_TRUE_FATAL(3 == g_eScreenDevice, "步骤13，回调设备类型不正确\n");

	CSTK_FATAL_POINT
		
	//恢复测试前分辨率设置
	if (NotSupportHD())
	{
		if (g_scResolution[0]>EM_UDISCREEN_RESOLUTION_INVALID && g_scResolution[0]<=EM_UDISCREEN_RESOLUTION_576P)
		{
			CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(&scScreenDevice[0], &g_scResolution[0], 1), "步骤14失败\n");
		}
	}
	else
	{
		if ((g_scResolution[0]>EM_UDISCREEN_RESOLUTION_INVALID && g_scResolution[0]<=EM_UDISCREEN_RESOLUTION_576P)
			&& (g_scResolution[1]>=EM_UDISCREEN_RESOLUTION_720P && g_scResolution[0]<EM_UDISCREEN_RESOLUTION_NUM))
		{
			scResolution[0] = g_scResolution[0];
			scResolution[1] = g_scResolution[1];
			CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤14失败\n");
		}
	} 

	if (CSUDI_NULL != hPlayer)
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERStop(hPlayer), "停止播放节目失败\n");
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERClose(hPlayer), "关闭播放器失败\n");
		hPlayer = CSUDI_NULL;
	}

	return CSUDI_TRUE;
}

static char  g_cCB_AUserData[16] = {0};
static char  g_cCB_BUserData[16] = {0};
static char  g_cCB_CUserData[16] = {0};

static void InitGlobalParam()
{
	memset(g_cCB_AUserData, 0, sizeof(g_cCB_AUserData));
	memset(g_cCB_BUserData, 0, sizeof(g_cCB_BUserData));
	memset(g_cCB_CUserData, 0, sizeof(g_cCB_CUserData));
}

static void CB_A(CSUDISCREENEvent_E eEvt, CSUDISCREENType_E eScreenDevice, void * pvUserData)
{
	char cTemp_A[] = "CBP_A";
	char cTemp_B[] = "CBP_B";

	if (0 == strcmp(pvUserData, cTemp_A))
	{
		strcpy(g_cCB_AUserData, pvUserData);
		CSTCPrint("[UDI2SCREENTEST]g_cCB_AUserData回调参数为%s \r\n", g_cCB_AUserData);
	}

	if (0 == strcmp(pvUserData, cTemp_B))
	{
		strcpy(g_cCB_CUserData, pvUserData);
		CSTCPrint("[UDI2SCREENTEST]g_cCB_CUserData回调参数为%s \r\n", g_cCB_CUserData);
	}
}

static void CB_B(CSUDISCREENEvent_E eEvt, CSUDISCREENType_E eScreenDevice, void * pvUserData)
{
	strcpy(g_cCB_BUserData, pvUserData);
	CSTCPrint("[UDI2SCREENTEST]g_cCB_BUserData回调参数为%s \r\n", g_cCB_BUserData);
}

//@CASEGROUP:CSUDISCREENSetResolution 
//@CASEGROUP:CSUDISCREENAddCallback 
//@CASEGROUP:CSUDISCREENDelCallback
//@DESCRIPTION:测试注册多个回调函数的正确性
//@PRECONDITION: 
//@INPUT:1. 正常播放一个标清PAL节目源,注册多个视频分辨率切换回调函数
//@EXPECTATION: 视频分辨率发生正常切换,输出无异常,输出无异常,并且回调函数功能正常
//@EXECUTIONFLOW:1.调用player模块接口播放标清PAL码流,详细步骤请参考测试用例CSTC_SCREEN_TEST_IT_SetResolution_0003步骤1
//@EXECUTIONFLOW:2. 调用CSUDISCREENAddCallback注册回调函数CB_A,回调参数CBP_A,期望返回CSUDI_SUCCESS
//@EXECUTIONFLOW:3. 调用CSUDISCREENAddCallback注册回调函数CB_B,回调参数CBP_A,期望返回CSUDI_SUCCESS
//@EXECUTIONFLOW:4. 调用CSUDISCREENAddCallback注册回调函数CB_A,回调参数CBP_B,期望返回CSUDI_SUCCESS
//@EXECUTIONFLOW:5. 设置标清视频输出为EM_UDISCREEN_RESOLUTION_NTSC,期望三次注册的回调函数均能被回调,且返回参数分别为CBP_A,CBP_A,CBP_B
//@EXECUTIONFLOW:6. 设置标清视频输出为EM_UDISCREEN_RESOLUTION_PAL,期望(CB_A,CBP_A) (CB_B,CBP_A)和CB_A,CBP_B)均被回调
//@EXECUTIONFLOW:7. 调用CSUDISCREENDelCallback删除(CB_A,CBP_A)的回调函数
//@EXECUTIONFLOW:8. 设置标清视频输出为EM_UDISCREEN_RESOLUTION_NTSC,期望(CB_B,CBP_A)和CB_A,CBP_B)被回调(CB_A,CBP_A)不能被回调
//@EXECUTIONFLOW:9. 调用CSUDISCREENDelCallback删除(CB_B,CBP_A)的回调函数
//@EXECUTIONFLOW:10. 设置标清视频输出为EM_UDISCREEN_RESOLUTION_PAL,期望(CB_A,CBP_B)被回调,(CB_B,CBP_A) (CB_A,CBP_A)不能被回调
//@EXECUTIONFLOW:11. 调用CSUDISCREENDelCallback删除(CB_A,CBP_B)的回调函数
//@EXECUTIONFLOW:12. 设置标清视频输出为EM_UDISCREEN_RESOLUTION_NTSC,期望(CB_A,CBP_B)(CB_B,CBP_A) (CB_A,CBP_A)均不能被回调
//@EXECUTIONFLOW:13. 设置标清视频输出为EM_UDISCREEN_RESOLUTION_PAL
//@EXECUTIONFLOW:14. 恢复测试前分辨率设置,该分辨为该模块测试初始化时获得的高标清分辨率
//@EXECUTIONFLOW:15. 调用CSUDIPLAYERStop停止测试节目播放
//@EXECUTIONFLOW:16. 调用CSUDIPLAYERClose关闭测试播放器
CSUDI_BOOL CSTC_SCREEN_TEST_IT_SetResolution_0013( void )
{
	CSUDI_HANDLE hPlayer = CSUDI_NULL;
	CSUDISCREENType_E scScreenDevice[2] = {EM_UDI_VOUT_DEVICE_SD, EM_UDI_VOUT_DEVICE_HD};
	CSUDISCREENResolution_E scResolution[2];
	char scCB_AUserData[] = "CBP_A";
	char scCB_BUserData[] = "CBP_B";
	
	hPlayer = PlaySDProgram(EM_UDI_VID_STREAM_MPEG2 , EM_UDISCREEN_RESOLUTION_PAL);  //调用PLAYER播放测试码流

	CSTK_ASSERT_TRUE_FATAL(CSUDI_NULL != hPlayer, "启动节目播放失败\n");
	CSUDIOSThreadSleep(500);
	
	if (NotSupportHD())
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENGetResolution(scScreenDevice[0], &g_scResolution[0]), "获取标清测试前分辨率失败\n");
	}
	else
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENGetResolution(scScreenDevice[0], &g_scResolution[0]), "获取标清测试前分辨率失败\n");
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENGetResolution(scScreenDevice[1], &g_scResolution[1]), "获取高清测试前分辨率失败\n");
	}
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENAddCallback(CB_A, (char *) scCB_AUserData), "步骤2失败\n");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENAddCallback(CB_B, (char *) scCB_AUserData), "步骤3失败\n");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENAddCallback(CB_A, (char *) scCB_BUserData), "步骤4失败\n");

	InitGlobalParam();
	scResolution[0] = EM_UDISCREEN_RESOLUTION_NTSC;
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(&scScreenDevice[0], &scResolution[0], 1), "步骤5失败\n");
	CSUDIOSThreadSleep(500);
	CSTK_ASSERT_TRUE_FATAL((0 == strcmp(scCB_AUserData, g_cCB_AUserData)) && (0 == strcmp(scCB_AUserData, g_cCB_BUserData)) && (0 == strcmp(scCB_BUserData, g_cCB_CUserData)), "步骤5失败，回调用户数据不正确\n");
	   
	InitGlobalParam();
	scResolution[0] = EM_UDISCREEN_RESOLUTION_PAL;
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(&scScreenDevice[0], &scResolution[0], 1), "步骤6失败\n");
	CSUDIOSThreadSleep(500);
	CSTK_ASSERT_TRUE_FATAL((0 == strcmp(scCB_AUserData, g_cCB_AUserData)) && (0 == strcmp(scCB_AUserData, g_cCB_BUserData)) && (0 == strcmp(scCB_BUserData, g_cCB_CUserData)), "步骤6失败，回调用户数据不正确\n");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENDelCallback(CB_A, scCB_AUserData), "步骤7失败\n");

	InitGlobalParam();
	scResolution[0] = EM_UDISCREEN_RESOLUTION_NTSC;
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(&scScreenDevice[0], &scResolution[0], 1), "步骤8失败\n");
	CSUDIOSThreadSleep(500);
	CSTK_ASSERT_TRUE_FATAL((0 != strcmp(scCB_AUserData, g_cCB_AUserData)) && (0 == strcmp(scCB_AUserData, g_cCB_BUserData)) && (0 == strcmp(scCB_BUserData, g_cCB_CUserData)), "步骤8，回调用户数据不正确\n");
	
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENDelCallback(CB_B, scCB_AUserData), "步骤9失败\n");

	InitGlobalParam();
	scResolution[0] = EM_UDISCREEN_RESOLUTION_PAL;
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(&scScreenDevice[0], &scResolution[0], 1), "步骤10失败\n");
	CSUDIOSThreadSleep(500);
	CSTK_ASSERT_TRUE_FATAL((0 != strcmp(scCB_AUserData, g_cCB_AUserData)) && (0 != strcmp(scCB_AUserData, g_cCB_BUserData)) && (0 == strcmp(scCB_BUserData, g_cCB_CUserData)), "步骤10，回调用户数据不正确\n");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENDelCallback(CB_A, scCB_BUserData), "步骤11失败\n");

	InitGlobalParam();
	scResolution[0] = EM_UDISCREEN_RESOLUTION_NTSC;
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(&scScreenDevice[0], &scResolution[0], 1), "步骤12失败\n");
	CSUDIOSThreadSleep(500);
	CSTK_ASSERT_TRUE_FATAL((0 != strcmp(scCB_AUserData, g_cCB_AUserData)) && (0 != strcmp(scCB_AUserData, g_cCB_BUserData)) && (0 != strcmp(scCB_BUserData, g_cCB_CUserData)), "步骤12，回调用户数据不正确\n");
   
	scResolution[0] = EM_UDISCREEN_RESOLUTION_PAL;
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(&scScreenDevice[0], &scResolution[0], 1), "步骤13失败\n");

	CSTK_FATAL_POINT
	{
		//恢复测试前分辨率设置
		if (NotSupportHD())
		{
			if (g_scResolution[0]>EM_UDISCREEN_RESOLUTION_INVALID && g_scResolution[0]<=EM_UDISCREEN_RESOLUTION_576P)
			{
				CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(&scScreenDevice[0], &g_scResolution[0], 1), "步骤14失败\n");
			}
		}
		else
		{
			if ((g_scResolution[0]>EM_UDISCREEN_RESOLUTION_INVALID && g_scResolution[0]<=EM_UDISCREEN_RESOLUTION_576P)
				&& (g_scResolution[1]>=EM_UDISCREEN_RESOLUTION_720P && g_scResolution[0]<EM_UDISCREEN_RESOLUTION_NUM))
			{
				scResolution[0] = g_scResolution[0];
				scResolution[1] = g_scResolution[1];
				CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤14失败\n");
			}
		}

		if (CSUDI_NULL != hPlayer)
		{
			CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERStop(hPlayer), "停止播放节目失败\n");
			CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERClose(hPlayer), "关闭播放器失败\n");
			hPlayer = CSUDI_NULL;
		}
	}

	return CSUDI_TRUE;
}

//@CASEGROUP:CSUDISCREENSetResolution 
//@DESCRIPTION:测试在标清设备上设置高清分辨率以及在高清设备上设置标清分辨率
//@PRECONDITION: 
//@INPUT:1. 对SD设备设置高清分辨率
//@INPUT:2. 对HD设备设置标清分辨率
//@EXPECTATION: 在标清设备上设置高清分辨率以及在高清设备上设置标清分辨率时返回不支持
//@EXECUTIONFLOW:1.调用CSUDISCREENGetResolution获取标清和高清的分辨率
//@EXECUTIONFLOW:2.在标清设备上设置分辨率为EM_UDISCREEN_RESOLUTION_720P,期望返回CSUDISCREEN_ERROR_FEATURE_NOT_SUPPORTED
//@EXECUTIONFLOW:3.调用CSUDISCREENGetResolution获取标清分辨率,期望未变化
//@EXECUTIONFLOW:4.在标清设备上设置分辨率为EM_UDISCREEN_RESOLUTION_720P_50HZ,期望返回CSUDISCREEN_ERROR_FEATURE_NOT_SUPPORTED
//@EXECUTIONFLOW:5.调用CSUDISCREENGetResolution获取标清分辨率,期望未变化
//@EXECUTIONFLOW:6.在标清设备上设置分辨率为EM_UDISCREEN_RESOLUTION_1080I,期望返回CSUDISCREEN_ERROR_FEATURE_NOT_SUPPORTED
//@EXECUTIONFLOW:7.调用CSUDISCREENGetResolution获取标清分辨率,期望未变化
//@EXECUTIONFLOW:8.在标清设备上设置分辨率为EM_UDISCREEN_RESOLUTION_1080I_50HZ,期望返回CSUDISCREEN_ERROR_FEATURE_NOT_SUPPORTED
//@EXECUTIONFLOW:9.调用CSUDISCREENGetResolution获取标清分辨率,期望未变化
//@EXECUTIONFLOW:10.在标清设备上设置分辨率为EM_UDISCREEN_RESOLUTION_1080P,期望返回CSUDISCREEN_ERROR_FEATURE_NOT_SUPPORTED
//@EXECUTIONFLOW:11.调用CSUDISCREENGetResolution获取标清分辨率,期望未变化
//@EXECUTIONFLOW:12.在标清设备上设置分辨率为EM_UDISCREEN_RESOLUTION_1080P_50HZ,期望返回CSUDISCREEN_ERROR_FEATURE_NOT_SUPPORTED
//@EXECUTIONFLOW:13.调用CSUDISCREENGetResolution获取标清分辨率,期望未变化
//@EXECUTIONFLOW:14.在高清设备上设置分辨率为EM_UDISCREEN_RESOLUTION_NTSC,期望返回CSUDISCREEN_ERROR_FEATURE_NOT_SUPPORTED
//@EXECUTIONFLOW:15.调用CSUDISCREENGetResolution获取高清分辨率,期望未变化
//@EXECUTIONFLOW:16.在高清设备上设置分辨率为EM_UDISCREEN_RESOLUTION_PAL,期望返回CSUDISCREEN_ERROR_FEATURE_NOT_SUPPORTED
//@EXECUTIONFLOW:17.调用CSUDISCREENGetResolution获取高清分辨率,期望未变化
CSUDI_BOOL CSTC_SCREEN_TEST_IT_SetResolution_0014( void )
{
	CSUDISCREENType_E scScreenDevice[2] = {EM_UDI_VOUT_DEVICE_SD, EM_UDI_VOUT_DEVICE_HD};
	CSUDISCREENResolution_E  scResolution[2];
	CSUDI_HANDLE hPlayer = CSUDI_NULL;

	hPlayer = PlaySDProgram(EM_UDI_VID_STREAM_MPEG2 ,EM_UDISCREEN_RESOLUTION_PAL);  //调用PLAYER播放测试码流

	CSTK_ASSERT_TRUE_FATAL(CSUDI_NULL != hPlayer, "启动节目播放失败\n");

	do
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENGetResolution(scScreenDevice[0], &g_scResolution[0]), "获取测试前分辨率失败\n");

		if (!NotSupportHD())
		{
			CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENGetResolution(scScreenDevice[1], &g_scResolution[1]), "获取测试前分辨率失败\n");
		}

		scResolution[0] = EM_UDISCREEN_RESOLUTION_720P;
		CSTK_ASSERT_TRUE_FATAL(CSUDISCREEN_ERROR_FEATURE_NOT_SUPPORTED == CSUDISCREENSetResolution(&scScreenDevice[0], &scResolution[0] , 1), "步骤2失败\n");
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENGetResolution(scScreenDevice[0], &scResolution[0] ), "步骤3失败\n");
		CSTK_ASSERT_TRUE_FATAL(g_scResolution[0] == scResolution[0], "步骤3失败\n");

		scResolution[0]  = EM_UDISCREEN_RESOLUTION_720P_50HZ;
		CSTK_ASSERT_TRUE_FATAL(CSUDISCREEN_ERROR_FEATURE_NOT_SUPPORTED == CSUDISCREENSetResolution(&scScreenDevice[0], &scResolution[0] , 1), "步骤4失败\n");
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENGetResolution(scScreenDevice[0], &scResolution[0] ), "步骤5失败\n");
		CSTK_ASSERT_TRUE_FATAL(g_scResolution[0] == scResolution[0], "步骤5失败\n");

		scResolution[0]  = EM_UDISCREEN_RESOLUTION_1080I;
		CSTK_ASSERT_TRUE_FATAL(CSUDISCREEN_ERROR_FEATURE_NOT_SUPPORTED == CSUDISCREENSetResolution(&scScreenDevice[0], &scResolution[0] , 1), "步骤6失败\n");
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENGetResolution(scScreenDevice[0], &scResolution[0] ), "步骤7失败\n");
		CSTK_ASSERT_TRUE_FATAL(g_scResolution[0] == scResolution[0], "步骤7失败\n");

		scResolution[0]  = EM_UDISCREEN_RESOLUTION_1080I_50HZ;
		CSTK_ASSERT_TRUE_FATAL(CSUDISCREEN_ERROR_FEATURE_NOT_SUPPORTED == CSUDISCREENSetResolution(&scScreenDevice[0], &scResolution[0] , 1), "步骤8失败\n");
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENGetResolution(scScreenDevice[0], &scResolution[0] ), "步骤9失败\n");
		CSTK_ASSERT_TRUE_FATAL(g_scResolution[0] == scResolution[0], "步骤9失败\n");

		scResolution[0]  = EM_UDISCREEN_RESOLUTION_1080P;
		CSTK_ASSERT_TRUE_FATAL(CSUDISCREEN_ERROR_FEATURE_NOT_SUPPORTED == CSUDISCREENSetResolution(&scScreenDevice[0], &scResolution[0] , 1), "步骤10失败\n");
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENGetResolution(scScreenDevice[0], &scResolution[0] ), "步骤11失败\n");
		CSTK_ASSERT_TRUE_FATAL(g_scResolution[0] == scResolution[0], "步骤11失败\n");

		scResolution[0]  = EM_UDISCREEN_RESOLUTION_1080P_50HZ;
		CSTK_ASSERT_TRUE_FATAL(CSUDISCREEN_ERROR_FEATURE_NOT_SUPPORTED == CSUDISCREENSetResolution(&scScreenDevice[0], &scResolution[0] , 1), "步骤12失败\n");
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENGetResolution(scScreenDevice[0], &scResolution[0] ), "步骤13失败\n");
		CSTK_ASSERT_TRUE_FATAL(g_scResolution[0] == scResolution[0], "步骤13失败\n");
	} while(0); 

	if (!NotSupportHD())
	{
		scResolution[1]  = EM_UDISCREEN_RESOLUTION_NTSC;
		CSTK_ASSERT_TRUE_FATAL(CSUDISCREEN_ERROR_FEATURE_NOT_SUPPORTED == CSUDISCREENSetResolution(&scScreenDevice[1], &scResolution[1] , 1), "步骤14失败\n");
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENGetResolution(scScreenDevice[1], &scResolution[1] ), "步骤15失败\n");
		CSTK_ASSERT_TRUE_FATAL(g_scResolution[1] == scResolution[1], "步骤15失败\n");

		scResolution[1]  = EM_UDISCREEN_RESOLUTION_PAL;
		CSTK_ASSERT_TRUE_FATAL(CSUDISCREEN_ERROR_FEATURE_NOT_SUPPORTED == CSUDISCREENSetResolution(&scScreenDevice[1], &scResolution[1] , 1), "步骤16失败\n");
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENGetResolution(scScreenDevice[1], &scResolution[1] ), "步骤17失败\n");
		CSTK_ASSERT_TRUE_FATAL(g_scResolution[1] == scResolution[1], "步骤17失败\n");
	}

	CSTK_FATAL_POINT
	{
		//恢复测试前分辨率
		if (NotSupportHD())
		{
			if (g_scResolution[0]>EM_UDISCREEN_RESOLUTION_INVALID && g_scResolution[0]<=EM_UDISCREEN_RESOLUTION_576P)
			{
				CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(&scScreenDevice[0], &g_scResolution[0], 1), "步骤14失败\n");
			}
		}
		else
		{
			if ((g_scResolution[0]>EM_UDISCREEN_RESOLUTION_INVALID && g_scResolution[0]<=EM_UDISCREEN_RESOLUTION_576P)
				&& (g_scResolution[1]>=EM_UDISCREEN_RESOLUTION_720P && g_scResolution[0]<EM_UDISCREEN_RESOLUTION_NUM))
			{
				scResolution[0] = g_scResolution[0];
				scResolution[1] = g_scResolution[1];
				CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤14失败\n");
			}
		}
		
		if (CSUDI_NULL != hPlayer)
		{
			CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERStop(hPlayer), "停止播放节目失败\n");
			CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERClose(hPlayer), "关闭播放器失败\n");
			hPlayer = CSUDI_NULL;
		}
	}

	return CSUDI_TRUE;
}

//@CASEGROUP:CSUDISCREENGetResolution 
//@DESCRIPTION:测试获取指针为NULL的情况
//@PRECONDITION:
//@INPUT:1. eScreenDevice = EM_UDI_VOUT_DEVICE_SD,peResolution = NULL
//@INPUT:2. eScreenDevice = EM_UDI_VOUT_DEVICE_HD+!,peResolution = &sResolution
//@INPUT:2. eScreenDevice = EM_UDI_VOUT_DEVICE_SD-1,peResolution = &sResolution
//@EXPECTATION: 返回CSUDISCREEN_ERROR_BAD_PARAMETER
//@EXECUTIONFLOW:1.调用CSUDISCREENGetResolution,传入错误参数,期望返回CSUDISCREEN_ERROR_BAD_PARAMETER
CSUDI_BOOL CSTC_SCREEN_TEST_IT_GetResolution_0001( void )
{
	CSUDI_Error_Code nResult = CSUDI_FAILURE;
	CSUDISCREENResolution_E sResolution;

	nResult = CSUDISCREENGetResolution(EM_UDI_VOUT_DEVICE_SD, CSUDI_NULL);
	CSTK_ASSERT_TRUE_FATAL((CSUDISCREEN_ERROR_BAD_PARAMETER == nResult), "参数错误检测失败1");

	nResult = CSUDISCREENGetResolution(EM_UDI_VOUT_DEVICE_SD -1, &sResolution);
	CSTK_ASSERT_TRUE_FATAL((CSUDISCREEN_ERROR_BAD_PARAMETER == nResult), "参数错误检测失败2");

	nResult = CSUDISCREENGetResolution(EM_UDI_VOUT_DEVICE_HD +1, &sResolution);
	CSTK_ASSERT_TRUE_FATAL((CSUDISCREEN_ERROR_BAD_PARAMETER == nResult), "参数错误检测失败3");

	CSTK_FATAL_POINT;

	return CSUDI_TRUE;
}

//@CASEGROUP:CSUDISCREENGetResolution 
//@DESCRIPTION:测试分辨率转换后是否能够获取到正确的当前分辨率
//@PRECONDITION:
//@INPUT:1. 分别对分辨率进行各种切换
//@INPUT:2. CSUDISCREENGetResolution调用均传入合法参数
//@EXPECTATION: 分辨率产生变化后通过CSUDISCREENGetResolution能够获取到当前正确的分辨率
//@EXECUTIONFLOW:1.设置标清分辨率为EM_UDISCREEN_RESOLUTION_PAL,期望返回CSUDI_SUCCESS
//@EXECUTIONFLOW:2.调用CSUDISCREENGetResolution获取标清分辨率,期望为EM_UDISCREEN_RESOLUTION_PAL
//@EXECUTIONFLOW:3.设置标清分辨率为EM_UDISCREEN_RESOLUTION_NTSC,期望返回CSUDI_SUCCESS
//@EXECUTIONFLOW:4.调用CSUDISCREENGetResolution获取标清分辨率,期望为EM_UDISCREEN_RESOLUTION_NTSC
//@EXECUTIONFLOW:5.如果平台支持高清分辨率,则设置高清设备分辨率为EM_UDISCREEN_RESOLUTION_1080I_50HZ,期望返回CSUDI_SUCCESS
//@EXECUTIONFLOW:6.如果平台支持高清分辨率,调用CSUDISCREENGetResolution获取高清分辨率,期望为EM_UDISCREEN_RESOLUTION_1080I_50HZ
//@EXECUTIONFLOW:7.如果平台支持高清分辨率,则同时设置标清为EM_UDISCREEN_RESOLUTION_PAL,高清为EM_UDISCREEN_RESOLUTION_720P_50HZ,期望返回CSUDI_SUCCESS
//@EXECUTIONFLOW:8.如果平台支持高清分辨率,调用CSUDISCREENGetResolution获取标清分辨率,期望为EM_UDISCREEN_RESOLUTION_PAL
//@EXECUTIONFLOW:9.如果平台支持高清分辨率,调用CSUDISCREENGetResolution获取高清分辨率,期望为EM_UDISCREEN_RESOLUTION_720P_50HZ
//@EXECUTIONFLOW:10. 恢复测试前分辨率设置,该分辨为该模块测试初始化时获得的高标清分辨率
CSUDI_BOOL CSTC_SCREEN_TEST_IT_GetResolution_0002( void )
{
	CSUDISCREENType_E scScreenDevice[2] = {EM_UDI_VOUT_DEVICE_SD, EM_UDI_VOUT_DEVICE_HD};
	CSUDISCREENResolution_E scResolution[2];
	CSUDI_HANDLE hPlayer = CSUDI_NULL;

	hPlayer = PlaySDProgram(EM_UDI_VID_STREAM_MPEG2, EM_UDISCREEN_RESOLUTION_PAL );  //调用PLAYER播放测试码流

	CSTK_ASSERT_TRUE_FATAL(CSUDI_NULL != hPlayer, "启动节目播放失败\n");
	CSTCPrint("视频节目播放画面是否正常?\n");
	CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes( ), "步骤1失败\n");

	if (NotSupportHD())
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENGetResolution(scScreenDevice[0], &g_scResolution[0]), "获取标清测试前分辨率失败\n");
	}
	else
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENGetResolution(scScreenDevice[0], &g_scResolution[0]), "获取标清测试前分辨率失败\n");
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENGetResolution(scScreenDevice[1], &g_scResolution[1]), "获取高清测试前分辨率失败\n");
	}

	scResolution[0] = EM_UDISCREEN_RESOLUTION_PAL;
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(&scScreenDevice[0], &scResolution[0] , 1), "步骤1失败\n"); 
	CSUDIOSThreadSleep(500);
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENGetResolution(scScreenDevice[0], &scResolution[0]), "步骤2,获取分辨率\n");  
	CSTK_ASSERT_TRUE_FATAL(EM_UDISCREEN_RESOLUTION_PAL == scResolution[0], "步骤2失败,获取的分辨率与设置的不一样\n");

	scResolution[0] = EM_UDISCREEN_RESOLUTION_NTSC;
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(&scScreenDevice[0], &scResolution[0] , 1), "步骤3失败\n");
	CSUDIOSThreadSleep(500);
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENGetResolution(scScreenDevice[0], &scResolution[0]), "步骤4,获取分辨率\n"); 
	CSTK_ASSERT_TRUE_FATAL(EM_UDISCREEN_RESOLUTION_NTSC == scResolution[0], "步骤4失败,获取的分辨率与设置的不一样\n");

	//如果平台支持高清分辨率
	if (IsScreenSupport(scScreenDevice, EM_UDISCREEN_RESOLUTION_1080I_50HZ ))
	{    
		scResolution[1] = EM_UDISCREEN_RESOLUTION_1080I_50HZ;
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(&scScreenDevice[1], &scResolution[1], 2), "步骤5失败\n");
		CSUDIOSThreadSleep(500);
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENGetResolution(EM_UDI_VOUT_DEVICE_HD, &scResolution[1] ), "步骤6失败\n");
		CSTK_ASSERT_TRUE_FATAL(EM_UDISCREEN_RESOLUTION_1080I_50HZ == scResolution[1], "步骤6失败,获取的分辨率与设置的不一样\n")
	} 

	if (IsScreenSupport(scScreenDevice, EM_UDISCREEN_RESOLUTION_720P_50HZ ))
	{
		scResolution[0] = EM_UDISCREEN_RESOLUTION_PAL;
		scResolution[1] = EM_UDISCREEN_RESOLUTION_720P_50HZ;
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤7失败\n"); 
		CSUDIOSThreadSleep(500);
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENGetResolution(EM_UDI_VOUT_DEVICE_SD, &scResolution[0] ), "步骤8，获取分辨率失败\n");
		CSTK_ASSERT_TRUE_FATAL(EM_UDISCREEN_RESOLUTION_PAL == scResolution[0], "步骤8失败,获取的分辨率与设置的不一样\n");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENGetResolution(EM_UDI_VOUT_DEVICE_HD, &scResolution[1] ), "步骤9，获取分辨率失败\n");
		CSTK_ASSERT_TRUE_FATAL(EM_UDISCREEN_RESOLUTION_720P_50HZ == scResolution[1], "步骤9失败,获取的分辨率与设置的不一样\n");
	}

	CSTK_FATAL_POINT
	{
		//恢复测试前分辨率设置
		if (NotSupportHD())
		{
			if (g_scResolution[0]>EM_UDISCREEN_RESOLUTION_INVALID && g_scResolution[0]<=EM_UDISCREEN_RESOLUTION_576P)
			{
				CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(&scScreenDevice[0], &g_scResolution[0], 1), "步骤14失败\n");
			}
		}
		else
		{
			if ((g_scResolution[0]>EM_UDISCREEN_RESOLUTION_INVALID && g_scResolution[0]<=EM_UDISCREEN_RESOLUTION_576P)
				&& (g_scResolution[1]>=EM_UDISCREEN_RESOLUTION_720P && g_scResolution[0]<EM_UDISCREEN_RESOLUTION_NUM))
			{
				scResolution[0] = g_scResolution[0];
				scResolution[1] = g_scResolution[1];
				CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(scScreenDevice, scResolution, 2), "步骤14失败\n");
			}
		}
	
		if (CSUDI_NULL != hPlayer)
		{
			CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERStop(hPlayer), "停止播放节目失败\n");
			CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERClose(hPlayer), "关闭播放器失败\n");
			hPlayer = CSUDI_NULL;
		}
	}

	return CSUDI_TRUE;
}

//@CASEGROUP:CSUDISCREENGetCapability 
//@DESCRIPTION:测试获取指针为NULL的情况
//@PRECONDITION:
//@INPUT:1. eScreenDevice = EM_UDI_VOUT_DEVICE_HD,psCapabilityInfo = NULL
//@INPUT:2. eScreenDevice = EM_UDI_VOUT_DEVICE_HD+1,psCapabilityInfo = &sCapability
//@INPUT:3. eScreenDevice = EM_UDI_VOUT_DEVICE_SD-1,psCapabilityInfo = &sCapability
//@EXPECTATION: 返回CSUDISCREEN_ERROR_BAD_PARAMETER
//@EXECUTIONFLOW:1.调用CSUDISCREENGetCapability,传入错误参数，期望返回CSUDISCREEN_ERROR_BAD_PARAMETER
CSUDI_BOOL CSTC_SCREEN_TEST_IT_GetCapability_0001( void )
{
	CSUDI_Error_Code nResult = CSUDI_FAILURE;
	CSUDISCREENCapability_S sCapability;

	nResult = CSUDISCREENGetCapability(EM_UDI_VOUT_DEVICE_HD, CSUDI_NULL);
	CSTK_ASSERT_TRUE_FATAL((CSUDISCREEN_ERROR_BAD_PARAMETER == nResult), "参数错误检测失败1");

	nResult = CSUDISCREENGetCapability(EM_UDI_VOUT_DEVICE_HD+1, &sCapability);
	CSTK_ASSERT_TRUE_FATAL((CSUDISCREEN_ERROR_BAD_PARAMETER == nResult), "参数错误检测失败2");

	nResult = CSUDISCREENGetCapability(EM_UDI_VOUT_DEVICE_SD-1, &sCapability);
	CSTK_ASSERT_TRUE_FATAL((CSUDISCREEN_ERROR_BAD_PARAMETER == nResult), "参数错误检测失败3");

	CSTK_FATAL_POINT;

	return CSUDI_TRUE;	
}

//@CASEGROUP:CSUDISCREENGetCapability 
//@DESCRIPTION:测试相应的通道应该获取相应的分辨率
//@INPUT:1. eScreenDevice = EM_UDI_VOUT_DEVICE_SD,psCapabilityInfo = 合法指针
//@INPUT:2. eScreenDevice = EM_UDI_VOUT_DEVICE_HD,psCapabilityInfo = 合法指针(在支持HD的情况下)
//@EXPECTATION: 1.返回CSUDI_SUCCESS
//@EXPECTATION: 2.标清设备不应该有高清分辨率支持，高清设备不应该返回标清分辨率
//@EXPECTATION: 3.标清至少支持EM_UDISCREEN_RESOLUTION_NTSC和EM_UDISCREEN_RESOLUTION_PAL
//@EXPECTATION: 4.高清至少支持EM_UDISCREEN_RESOLUTION_720P_50HZ,EM_UDISCREEN_RESOLUTION_720P,EM_UDISCREEN_RESOLUTION_1080I_50HZ,EM_UDISCREEN_RESOLUTION_1080I
//@EXECUTIONFLOW:1.调用CSUDISCREENGetCapability获取标清设备支持的分辨率
//@EXECUTIONFLOW:2.遍历标清设备支持的分辨率,其所支持的分辨率中不应该有高清的分辨率
//@EXECUTIONFLOW:3.检查标清至少支持EM_UDISCREEN_RESOLUTION_NTSC和EM_UDISCREEN_RESOLUTION_PAL
//@EXECUTIONFLOW:4.遍历高清设备支持的分辨率,其所支持的分辨率中不应该有标清的分辨率(在平台支持HD的条件下)
//@EXECUTIONFLOW:5.检查高清至少支持EM_UDISCREEN_RESOLUTION_720P_50HZ,EM_UDISCREEN_RESOLUTION_720P,EM_UDISCREEN_RESOLUTION_1080I_50HZ,EM_UDISCREEN_RESOLUTION_1080I
CSUDI_BOOL CSTC_SCREEN_TEST_IT_GetCapability_0002( void )
{
	CSUDISCREENResolution_E  eResolution = EM_UDISCREEN_RESOLUTION_720P;
	CSUDISCREENType_E eScreenDevice = EM_UDI_VOUT_DEVICE_SD;

	for (eResolution = EM_UDISCREEN_RESOLUTION_720P; eResolution<EM_UDISCREEN_RESOLUTION_NUM; eResolution++)
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDI_FALSE == IsScreenSupport(&eScreenDevice, eResolution ), "Step2失败");
	} 

	CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == IsScreenSupport(&eScreenDevice, EM_UDISCREEN_RESOLUTION_PAL), "Step3 failure:SD must support PAL\n");
	CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == IsScreenSupport(&eScreenDevice, EM_UDISCREEN_RESOLUTION_NTSC), "Step3failure:SD must support NTSC\n");

	if(!NotSupportHD())
	{
		eScreenDevice = EM_UDI_VOUT_DEVICE_HD;

		for (eResolution = EM_UDISCREEN_RESOLUTION_NTSC; eResolution<EM_UDISCREEN_RESOLUTION_480P; eResolution++)
		{
			CSTK_ASSERT_TRUE_FATAL(CSUDI_FALSE == IsScreenSupport(&eScreenDevice, eResolution ), "Step3失败");
		} 

		CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == IsScreenSupport(&eScreenDevice, EM_UDISCREEN_RESOLUTION_720P), "Step3failure:HD must support 720P\n");
		CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == IsScreenSupport(&eScreenDevice, EM_UDISCREEN_RESOLUTION_720P_50HZ), "Step3failure:HD must support 720P_50HZ\n");
		CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == IsScreenSupport(&eScreenDevice, EM_UDISCREEN_RESOLUTION_1080I), "Step3failure:HD must support 1080I\n");
		CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == IsScreenSupport(&eScreenDevice, EM_UDISCREEN_RESOLUTION_1080I_50HZ), "Step3failure:HD must support 1080I_50HZ\n");
	}

	CSTK_FATAL_POINT;
	
	return CSUDI_TRUE;
}

//@CASEGROUP:CSUDISCREENSetBrightness 
//@DESCRIPTION:测试亮度参数分别为-1和101非法值的情况
//@PRECONDITION:节目播放正常
//@INPUT:1. nBrightness = -1
//@INPUT:2. nBrightness = 101
//@EXPECTATION:1.返回错误码CSUDISCREEN_ERROR_BAD_PARAMETER
//@EXECUTIONFLOW:1.调用player模块接口播放标清PAL码流,详细步骤请参考测试用例CSTC_SCREEN_TEST_IT_SetResolution_0003步骤1
//@EXECUTIONFLOW:2.调用CSUDISCREENSetBrightness设置亮度为-1,期望返回CSUDISCREEN_ERROR_BAD_PARAMETER
//@EXECUTIONFLOW:3.调用CSUDISCREENSetBrightness设置亮度为101,期望返回CSUDISCREEN_ERROR_BAD_PARAMETER
//@EXECUTIONFLOW:4.调用CSUDIPLAYERStop停止测试节目播放
//@EXECUTIONFLOW:5.调用CSUDIPLAYERClose关闭播放器
CSUDI_BOOL CSTC_SCREEN_TEST_IT_SetBrightness_0001( void )
{
	CSUDI_HANDLE hPlayer = CSUDI_NULL;

	hPlayer = PlaySDProgram(EM_UDI_VID_STREAM_MPEG2 , EM_UDISCREEN_RESOLUTION_PAL);  //调用PLAYER播放测试码流

	CSTK_ASSERT_TRUE_FATAL(CSUDI_NULL != hPlayer, "启动节目播放失败\n");

	CSTK_ASSERT_TRUE_FATAL(CSUDISCREEN_ERROR_BAD_PARAMETER == CSUDISCREENSetBrightness(-1), "参数检测失败\n");

	CSTK_ASSERT_TRUE_FATAL(CSUDISCREEN_ERROR_BAD_PARAMETER == CSUDISCREENSetBrightness(101), "参数检测失败\n");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERStop(hPlayer), "停止播放节目失败\n");

	CSTK_FATAL_POINT
	{
		if (CSUDI_NULL != hPlayer)
		{
			CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERClose(hPlayer), "关闭播放器失败\n");
			hPlayer = CSUDI_NULL;
		}
	}

	return CSUDI_TRUE;
}


//@CASEGROUP:CSUDISCREENSetContrast 
//@DESCRIPTION:测试亮度从0~100范围每级20 递增变化的情况
//@PRECONDITION:节目播放正常
//@INPUT:1. nBrightness = [0-100]
//@EXPECTATION:1.每次设置返回CSUDI_SUCCESS,且屏幕变化和设置值相符
//@EXECUTIONFLOW:1.调用player模块接口播放标清PAL码流,详细步骤请参考测试用例CSTC_SCREEN_TEST_IT_SetResolution_0003步骤1
//@EXECUTIONFLOW:2.调用CSUDISCREENSetContrast设置亮度为0
//@EXECUTIONFLOW:3.亮度值递增20并调用CSUDISCREENSetBrightness设置直到达到最大值100
//@EXECUTIONFLOW:4.测试完成恢复测试现场,将亮度设置为50
//@EXECUTIONFLOW:5.调用CSUDIPLAYERStop停止测试节目播放
//@EXECUTIONFLOW:6.调用CSUDIPLAYERClose关闭播放器
CSUDI_BOOL CSTC_SCREEN_TEST_IT_SetBrightness_0002( void )
{
	CSUDI_HANDLE hPlayer = CSUDI_NULL;
	int i = 0;
	
	hPlayer = PlaySDProgram(EM_UDI_VID_STREAM_MPEG2 , EM_UDISCREEN_RESOLUTION_PAL);  //调用PLAYER播放测试码流

	CSTK_ASSERT_TRUE_FATAL(CSUDI_NULL != hPlayer, "启动节目播放失败\n");

	CSTCPrint("以下每2秒设置一次屏幕亮度，使发生递增变化，请注意观察屏幕亮度，按任意键开始测试\r\n");
	CSTKWaitAnyKey();

	for(i=0; i<=100; i+=20)
	{
		CSTCPrint("设置屏幕亮度为%d\n", i);
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetBrightness(i), "设置亮度失败\n");
		CSUDIOSThreadSleep(2*1000);
	}

	CSTCPrint("屏幕亮度发生递增变化?\n");
	CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes(), "设置屏幕亮度后效果不正确");

	CSTK_FATAL_POINT
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetBrightness(50), "设置亮度失败\n");

		if (CSUDI_NULL != hPlayer)
		{
			CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERStop(hPlayer), "停止播放节目失败\n");
			
			CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERClose(hPlayer), "关闭播放器失败\n");
			hPlayer = CSUDI_NULL;
		}
	}

	return CSUDI_TRUE;
}

//@CASEGROUP:CSUDISCREENSetContrast 
//@DESCRIPTION:测试对比度参数分别为-1和101非法值的情况
//@PRECONDITION:节目播放正常
//@INPUT:1. Contrast = -1
//@INPUT:2. Contrast = 101
//@EXPECTATION:1.返回错误码CSUDISCREEN_ERROR_BAD_PARAMETER
//@EXECUTIONFLOW:1.调用player模块接口播放标清PAL码流,详细步骤请参考测试用例CSTC_SCREEN_TEST_IT_SetResolution_0003步骤1
//@EXECUTIONFLOW:2.调用CSUDISCREENSetContrast设置对比度为-1,期望返回CSUDISCREEN_ERROR_BAD_PARAMETER
//@EXECUTIONFLOW:3.调用CSUDISCREENSetContrast设置对比度为101,期望返回CSUDISCREEN_ERROR_BAD_PARAMETER
//@EXECUTIONFLOW:4.调用CSUDIPLAYERStop停止测试节目播放
//@EXECUTIONFLOW:5.调用CSUDIPLAYERClose关闭播放器
CSUDI_BOOL CSTC_SCREEN_TEST_IT_SetContrast_0001( void )
{
	CSUDI_HANDLE hPlayer = CSUDI_NULL;

	hPlayer = PlaySDProgram( EM_UDI_VID_STREAM_MPEG2, EM_UDISCREEN_RESOLUTION_PAL);  //调用PLAYER播放测试码流

	CSTK_ASSERT_TRUE_FATAL(CSUDI_NULL != hPlayer, "启动节目播放失败\n");

	CSTK_ASSERT_TRUE_FATAL(CSUDISCREEN_ERROR_BAD_PARAMETER == CSUDISCREENSetContrast(-1), "参数检测失败\n");

	CSTK_ASSERT_TRUE_FATAL(CSUDISCREEN_ERROR_BAD_PARAMETER == CSUDISCREENSetContrast(101), "参数检测失败\n");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERStop(hPlayer), "停止播放节目失败\n");

	CSTK_FATAL_POINT
	{
		if (CSUDI_NULL != hPlayer)
		{
			CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERClose(hPlayer), "关闭播放器失败\n");
			hPlayer = CSUDI_NULL;
		}
	}

	return CSUDI_TRUE;
}

//@CASEGROUP:CSUDISCREENSetBrightness 
//@DESCRIPTION:测试对比度从0~100范围每级20 递增变化的情况
//@PRECONDITION:节目播放正常
//@INPUT:1. Contrast = [0-100]
//@EXPECTATION:1.每次设置返回CSUDI_SUCCESS,且屏幕变化和设置值相符
//@EXECUTIONFLOW:1.调用player模块接口播放标清PAL码流,详细步骤请参考测试用例CSTC_SCREEN_TEST_IT_SetResolution_0003步骤1
//@EXECUTIONFLOW:2.调用CSUDISCREENSetBrightness设置对比度为0
//@EXECUTIONFLOW:3.对比度值递增20并调用CSUDISCREENSetContrast设置直到达到最大值100
//@EXECUTIONFLOW:4.测试完成恢复测试现场,将对比度设置为50
//@EXECUTIONFLOW:5.调用CSUDIPLAYERStop停止测试节目播放
//@EXECUTIONFLOW:6. 调用CSUDIPLAYERClose关闭播放器
CSUDI_BOOL CSTC_SCREEN_TEST_IT_SetContrast_0002( void )
{
	CSUDI_HANDLE hPlayer = CSUDI_NULL;
	int i = 0;
	
	hPlayer = PlaySDProgram( EM_UDI_VID_STREAM_MPEG2, EM_UDISCREEN_RESOLUTION_PAL);  //调用PLAYER播放测试码流

	CSTK_ASSERT_TRUE_FATAL(CSUDI_NULL != hPlayer, "启动节目播放失败\n");

	CSTCPrint("以下每2秒设置一次屏幕对比度，使发生递增变化，请注意观察屏幕对比度，按任意键开始测试\r\n");
	CSTKWaitAnyKey();
		
	for(i=0; i<=100; i+=20)
	{
		CSTCPrint("设置屏幕对比度为%d\n", i);
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetContrast(i), "设置对比度失败\n");
		CSUDIOSThreadSleep(2*1000);
	}

	CSTCPrint("屏幕对比度发生递增变化?\n");
	CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes(), "设置屏幕对比度后效果不正确");

	CSTK_FATAL_POINT
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetContrast(50), "设置对比度失败\n");
		
		if (CSUDI_NULL != hPlayer)
		{
			CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERStop(hPlayer), "停止播放节目失败\n");
			
			CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERClose(hPlayer), "关闭播放器失败\n");
			hPlayer = CSUDI_NULL;
		}
	}

	return CSUDI_TRUE;
}

//@CASEGROUP:CSUDISCREENSetSaturation 
//@DESCRIPTION:测试饱和度参数分别为-1和101非法值的情况
//@PRECONDITION:节目播放正常
//@INPUT:1. Saturation = -1
//@INPUT:2. Saturation = 101
//@EXPECTATION:1.返回错误码CSUDISCREEN_ERROR_BAD_PARAMETER
//@EXECUTIONFLOW:1.调用player模块接口播放标清PAL码流,详细步骤请参考测试用例CSTC_SCREEN_TEST_IT_SetResolution_0003步骤1
//@EXECUTIONFLOW:2.调用CSUDISCREENSetSaturation设置饱和度为-1,期望返回CSUDISCREEN_ERROR_BAD_PARAMETER
//@EXECUTIONFLOW:3.调用CSUDISCREENSetSaturation设置饱和度为101,期望返回CSUDISCREEN_ERROR_BAD_PARAMETER
//@EXECUTIONFLOW:4.调用CSUDIPLAYERStop停止测试节目播放
//@EXECUTIONFLOW:5. 调用CSUDIPLAYERClose关闭播放器
CSUDI_BOOL CSTC_SCREEN_TEST_IT_SetSaturation_0001( void )
{
	CSUDI_HANDLE hPlayer = CSUDI_NULL;

	hPlayer = PlaySDProgram(EM_UDI_VID_STREAM_MPEG2 , EM_UDISCREEN_RESOLUTION_PAL);  //调用PLAYER播放测试码流

	CSTK_ASSERT_TRUE_FATAL(CSUDI_NULL != hPlayer, "启动节目播放失败\n");

	CSTK_ASSERT_TRUE_FATAL(CSUDISCREEN_ERROR_BAD_PARAMETER == CSUDISCREENSetSaturation(-1), "参数检测失败\n");

	CSTK_ASSERT_TRUE_FATAL(CSUDISCREEN_ERROR_BAD_PARAMETER == CSUDISCREENSetSaturation(101), "参数检测失败\n");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERStop(hPlayer), "停止播放节目失败\n");

	CSTK_FATAL_POINT
	{
		if (CSUDI_NULL != hPlayer)
		{
			CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERClose(hPlayer), "关闭播放器失败\n");
			hPlayer = CSUDI_NULL;
		}
	}

	return CSUDI_TRUE;
}

//@CASEGROUP:CSUDISCREENSetSaturation 
//@DESCRIPTION:测试饱和度从0~100范围每级20 递增变化的情况
//@PRECONDITION:节目播放正常
//@INPUT:1. Saturation = [0-100]
//@EXPECTATION:1.每次设置返回CSUDI_SUCCESS,且屏幕变化和设置值相符
//@EXECUTIONFLOW:1.调用player模块接口播放标清PAL码流,详细步骤请参考测试用例CSTC_SCREEN_TEST_IT_SetResolution_0003步骤1
//@EXECUTIONFLOW:2.调用CSUDISCREENSetSaturation设置饱和度为0
//@EXECUTIONFLOW:3.饱和度值递增20并调用CSUDISCREENSetSaturation设置直到达到最大值100
//@EXECUTIONFLOW:4.测试完成恢复测试现场,将饱和度设置为50
//@EXECUTIONFLOW:5.调用CSUDIPLAYERStop停止测试节目播放
//@EXECUTIONFLOW:6. 调用CSUDIPLAYERClose关闭播放器
CSUDI_BOOL CSTC_SCREEN_TEST_IT_SetSaturation_0002( void )
{
	CSUDI_HANDLE hPlayer = CSUDI_NULL;
	int i = 0;
	
	hPlayer = PlaySDProgram( EM_UDI_VID_STREAM_MPEG2, EM_UDISCREEN_RESOLUTION_PAL);  //调用PLAYER播放测试码流

	CSTK_ASSERT_TRUE_FATAL(CSUDI_NULL != hPlayer, "启动节目播放失败\n");

	CSTCPrint("以下每2秒设置一次屏幕饱和度，使发生递增变化，请注意观察屏幕饱和度，按任意键开始测试\r\n");
	CSTKWaitAnyKey();

	for(i=0; i<=100; i+=20)
	{
		CSTCPrint("设置屏幕饱和度为%d\n", i);
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetSaturation(i), "设置饱和度失败\n");
		CSUDIOSThreadSleep(2*1000);
	}

	CSTCPrint("屏幕饱和度发生递增变化?\n");
	CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes(), "设置屏幕饱和度后效果不正确");

	CSTK_FATAL_POINT
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetSaturation(50), "设置饱和度失败\n");
		
		if (CSUDI_NULL != hPlayer)
		{
			CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERStop(hPlayer), "停止播放节目失败\n");
			
			CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERClose(hPlayer), "关闭播放器失败\n");
			hPlayer = CSUDI_NULL;
		}
	}

	return CSUDI_TRUE;
}


//@CASEGROUP:SetOSDTransparency 
//@DESCRIPTION:测试透明度参数分别为-1和101非法值的情况以及0和100的边界情况
//@INPUT:1. Saturation = -1,Saturation = 101
//@INPUT:2. Saturation = 0,Saturation = 100
//@EXPECTATION:1.Saturation = -1,Saturation = 101,返回错误码CSUDISCREEN_ERROR_BAD_PARAMETER
//@EXPECTATION:1.Saturation = 0,Saturation = 100,返回错误码CSUDI_SUCCESS
//@EXECUTIONFLOW:1.调用CSUDISCREENSetSaturation设置透明度为-1,期望返回CSUDISCREEN_ERROR_BAD_PARAMETER
//@EXECUTIONFLOW:2.调用CSUDISCREENSetSaturation设置透明度为101,期望返回CSUDISCREEN_ERROR_BAD_PARAMETER
//@EXECUTIONFLOW:3.调用CSUDISCREENSetSaturation设置透明度为0,期望返回CSUDI_SUCCESS
//@EXECUTIONFLOW:4.调用CSUDISCREENSetSaturation设置透明度为100,期望返回CSUDI_SUCCESS
CSUDI_BOOL CSTC_SCREEN_TEST_IT_SetOSDTransparency_0001( void )
{
	CSTK_ASSERT_TRUE_FATAL(CSUDISCREEN_ERROR_BAD_PARAMETER == CSUDISCREENSetOSDTransparency(-1), "step 1 错误参数检查失败\n");
	CSTK_ASSERT_TRUE_FATAL(CSUDISCREEN_ERROR_BAD_PARAMETER ==CSUDISCREENSetOSDTransparency(101), "step 2 错误参数检查失败\n");
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetOSDTransparency(0), "step 3 边界参数测试失败\n");
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS ==CSUDISCREENSetOSDTransparency(100), "step 4 边界参数测试失败\n");

	CSTK_FATAL_POINT

	return CSUDI_TRUE;
}

//@CASEGROUP:SetOSDTransparency 
//@DESCRIPTION:测试在没有video背景的情况设置OSG透明度也能起效果
//@PRECONDITION:平台OSG模块功能正常
//@INPUT:1. Saturation = [0-100]
//@EXPECTATION:1.每次设置返回CSUDI_SUCCESS
//@EXPECTATION:2. 屏幕OSG变化与透明度设置相符
//@EXECUTIONFLOW:1.按如下步骤在高标清显存上画测试图像(如果平台支持高清才包含高清):
//@EXECUTIONFLOW:1.1 调用CSUDIOSGGetDisplaySurface获取显存句柄,期望返回CSUDI_SUCCESS
//@EXECUTIONFLOW:1.2 调用CSUDIOSGFill在{260,188}位置填充{200,200}的红色OSG,期望返回CSUDI_SUCCESS
//@EXECUTIONFLOW:2.调用CSUDIOSGSync同步数据,期望返回CSUDI_SUCCESS
//@EXECUTIONFLOW:3.设置OSG透明度,从[0,100],每级递增20，每次设置后sleep 1.5s
//@EXECUTIONFLOW:4.等待用户判定测试结果
//@EXECUTIONFLOW:5.调用CSUDISCREENSetOSDTransparency恢复OSG的透明度为不透明,期望返回CSUDI_SUCCESS
//@EXECUTIONFLOW:6.调用CSUDIOSGFill恢复高标清的OSG 显存为值0x0,期望返回CSUDI_SUCCESS
CSUDI_BOOL CSTC_SCREEN_TEST_IT_SetOSDTransparency_0002( void )
{

	CSUDI_HANDLE hDisplaySurfaceSD = CSUDI_NULL,hDisplaySurfaceHD = CSUDI_NULL;
	CSUDIOSGSurfaceInfo_S sSurfaceInfoSD,sSurfaceInfoHD;
	CSUDIOSGRect_S stRect  = {
								260,
								188,
								200,
								200,
								};
	int i=0;
	BOOL bSupportHD=!NotSupportHD();

	if (!IsShareHDDisplay())
	{
		CSTK_ASSERT_TRUE_FATAL(
			(CSUDI_SUCCESS == CSUDIOSGGetDisplaySurface(0,&hDisplaySurfaceSD)),
			"get display surface failure");

		memset(&sSurfaceInfoSD, 0, sizeof(sSurfaceInfoSD));
		CSTK_ASSERT_TRUE_FATAL((CSUDI_SUCCESS == CSUDIOSGGetSurfaceInfo(hDisplaySurfaceSD, &sSurfaceInfoSD)), "获取存信息失败");
	}
	
	if (bSupportHD)
	{
		CSTK_ASSERT_TRUE_FATAL(
			(CSUDI_SUCCESS == CSUDIOSGGetDisplaySurface(1,&hDisplaySurfaceHD)),
			"get display surface failure");

		memset(&sSurfaceInfoHD, 0, sizeof(sSurfaceInfoHD));
		CSTK_ASSERT_TRUE_FATAL((CSUDI_SUCCESS == CSUDIOSGGetSurfaceInfo(hDisplaySurfaceHD, &sSurfaceInfoHD)), "获取存信息失败");
	}

	

	if (!IsShareHDDisplay())
	{
		stRect.m_nX = (sSurfaceInfoSD.m_nWidth - stRect.m_nWidth)/2;
		stRect.m_nY = (sSurfaceInfoSD.m_nHeight - stRect.m_nHeight)/2;
	
		CSTK_ASSERT_TRUE_FATAL(
			(CSUDI_SUCCESS == CSUDIOSGFill(hDisplaySurfaceSD, CSUDI_NULL, 0)),
			"Fill Failure !");
		CSTK_ASSERT_TRUE_FATAL(
			(CSUDI_SUCCESS == CSUDIOSGFill(hDisplaySurfaceSD, &stRect, 0x80ff0000)),
			"Fill Failure !");
	}

	if (bSupportHD)
	{
		stRect.m_nX = (sSurfaceInfoHD.m_nWidth - stRect.m_nWidth)/2;
		stRect.m_nY = (sSurfaceInfoHD.m_nHeight - stRect.m_nHeight)/2;

		CSTK_ASSERT_TRUE_FATAL(
			(CSUDI_SUCCESS == CSUDIOSGFill(hDisplaySurfaceHD, CSUDI_NULL, 0)),
			"Fill Failure !");
		CSTK_ASSERT_TRUE_FATAL(
			(CSUDI_SUCCESS == CSUDIOSGFill(hDisplaySurfaceHD, &stRect, 0x80ff0000)),
			"Fill Failure !");
	}
	CSTK_ASSERT_TRUE_FATAL((CSUDI_SUCCESS == CSUDIOSGSync()),"Sync Failure !");
	CSTCPrint("屏幕上(%d,%d)位置可以看见红色宽高为(%d,%d)的矩形框,包括高清(如果支持高清的话)\n",
	stRect.m_nX,stRect.m_nY,stRect.m_nWidth,stRect.m_nHeight);
	CSTK_ASSERT_TRUE_FATAL(CSTKWaitYes(), "check result failure!");

	CSTCPrint("请注意OSG的变化,OSG的透明度将由0变化到100，每级20,任意键继续\n");
	CSTKWaitAnyKey();

	for(i=0;i<=100;i+=20)
	{
		CSTCPrint("设置OSG透明度到:%d\n",i);
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetOSDTransparency(i), "错误参数检查失败\n");
		CSUDIOSThreadSleep(1500);
	}

	CSTCPrint("屏幕OSG从完全透明逐渐变成完全清晰?? (Y/N)\n");
	CSTK_ASSERT_TRUE_FATAL(CSTKWaitYes(), "check result failure!");

	CSTK_FATAL_POINT;

	if (!IsShareHDDisplay())
	{
		CSTK_ASSERT_TRUE_FATAL(
			(CSUDI_SUCCESS == CSUDIOSGFill(hDisplaySurfaceSD, CSUDI_NULL, 0)),
			"Fill Failure !");
	}
	
	if (bSupportHD)
	{
		CSTK_ASSERT_TRUE_FATAL(
			(CSUDI_SUCCESS == CSUDIOSGFill(hDisplaySurfaceHD, CSUDI_NULL, 0)),
			"Fill Failure !");
	}

	return CSUDI_TRUE;
}

//@CASEGROUP:SetOSDTransparency 
//@DESCRIPTION:测试在播放video的情况下,OSG透明度从0~100范围每级20 递增变化的情况
//@PRECONDITION:节目播放正常
//@INPUT:1. Saturation = [0-100]
//@EXPECTATION:1.每次设置返回CSUDI_SUCCESS
//@EXPECTATION:2. 屏幕变化和设置值相符,包括video不受OSG透明度设置变化;OSG透明度变化正常
//@EXECUTIONFLOW:1.调用player模块接口播放标清PAL码流,详细步骤请参考测试用例CSTC_SCREEN_TEST_IT_SetResolution_0003步骤1
//@EXECUTIONFLOW:2.调用CSUDIOSGGetDisplaySurface获取标清显存句柄
//@EXECUTIONFLOW:3.调用CSUDIOSGFill将标清显存填充成蓝色0xFF0000FF,期望返回CSUDI_SUCCESS
//@EXECUTIONFLOW:4.调用CSUDIOSGFill将标清显存坐标{200,200}处填充{250,250}长的透明色0x0,期望返回CSUDI_SUCCESS
//@EXECUTIONFLOW:5.调用CSUDIVIDEOSetWindowSize设置标清video窗口大小为OSG透明区域大小,期望返回CSUDI_SUCCESS
//@EXECUTIONFLOW:6.如果平台支持高清则重复步骤2~5,在高清上做相同的画图和设置，每个步骤期望返回CSUDI_SUCCESS
//@EXECUTIONFLOW:7.设置OSG透明度,从[0,100],每级递增20，每次设置后sleep 1.5s
//@EXECUTIONFLOW:8.等待用户判定测试结果
//@EXECUTIONFLOW:9.调用CSUDIPLAYERStop停止测试节目播放,期望返回CSUDI_SUCCESS
//@EXECUTIONFLOW:10. 调用CSUDIPLAYERClose关闭播放器,期望返回CSUDI_SUCCESS
//@EXECUTIONFLOW:11.调用CSUDIVIDEOSetWindowSize恢复高标清的视频窗口为全屏,期望返回CSUDI_SUCCESS
//@EXECUTIONFLOW:12.调用CSUDISCREENSetOSDTransparency恢复OSG的透明度为不透明,期望返回CSUDI_SUCCESS
//@EXECUTIONFLOW:12.调用CSUDIOSGFill恢复高标清的OSG 显存为值0x0,期望返回CSUDI_SUCCESS
CSUDI_BOOL CSTC_SCREEN_TEST_IT_SetOSDTransparency_0003( void )
{
	CSUDI_HANDLE hPlayer = CSUDI_NULL;
	CSUDI_HANDLE hSurfaceSD = CSUDI_NULL,hSurfaceHD = CSUDI_NULL;
	int i = 0;
	CSUDIOSGRect_S stRect={200,200,250,250};
	CSUDIWinRect_S  stVideoRect;
	CSUDIOSGSurfaceInfo_S stInfo;
	BOOL bSupportHD=!NotSupportHD();
	CSUDIWinRect_S sWinFullRect;

	hPlayer = PlaySDProgram( EM_UDI_VID_STREAM_MPEG2, EM_UDISCREEN_RESOLUTION_PAL);  //调用PLAYER播放测试码流

	CSTK_ASSERT_TRUE_FATAL(CSUDI_NULL != hPlayer, "启动节目播放失败");
	
	memcpy(&stVideoRect,&stRect,sizeof(stVideoRect));

	if (!IsShareHDDisplay())
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIOSGGetDisplaySurface(0,&hSurfaceSD),
			"fail to get SD display surface");
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIOSGFill(hSurfaceSD,NULL,0xFF0000FF),
			"fail to fill whole SD surface to blue");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIOSGFill(hSurfaceSD,&stRect,0x0),
				"fail to fill SD region {200,200,250,250} to transparence");
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIVIDEOSetWindowSize(0,EM_UDI_VOUT_DEVICE_SD,&stVideoRect),
			"fail to set SD video to little box");
	}

	if (bSupportHD)
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIOSGGetDisplaySurface(1,&hSurfaceHD),
		"fail to get SD display surface");
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIOSGFill(hSurfaceHD,NULL,0xFF0000FF),
			"fail to fill whole HD surface to blue");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIOSGGetSurfaceInfo(hSurfaceHD, &stInfo),
			"fail to get hd surface by CSUDIOSGGetSurfaceInfo");
		
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIOSGFill(hSurfaceHD,&stRect,0x0),
				"fail to fill HD region {200,200,250,250} to transparence");

		memset(&sWinFullRect, 0, sizeof(sWinFullRect));
		
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIVIDEOGetWindowSize(0, EM_UDI_VOUT_DEVICE_HD, &sWinFullRect),
			"fail to get HD full windowrect");
	
		stVideoRect.m_nX = (stRect.m_nX*sWinFullRect.m_nWidth)/stInfo.m_nWidth;
		stVideoRect.m_nWidth = (stRect.m_nWidth*sWinFullRect.m_nWidth)/stInfo.m_nWidth;
		stVideoRect.m_nY = (stRect.m_nY*sWinFullRect.m_nHeight)/stInfo.m_nHeight;
		stVideoRect.m_nHeight = (stRect.m_nHeight*sWinFullRect.m_nHeight)/stInfo.m_nHeight;
		
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIVIDEOSetWindowSize(0,EM_UDI_VOUT_DEVICE_HD,&stVideoRect),
			"fail to set HD video to little box");
	}

	CSTK_ASSERT_TRUE_FATAL((CSUDI_SUCCESS == CSUDIOSGSync()),"Sync Failure !");
	
	CSTCPrint("屏幕{200,200,250,250}位置为小窗口视频,其余为蓝色OSG(注意: 视频源本身左右稍有黑边)�?(Y/N)\n");
	CSTK_ASSERT_TRUE_FATAL(CSTKWaitYes(), "check result failure!");
	
	CSTCPrint("请注意观察屏幕OSG透明度变化\n");
	CSTKWaitAnyKey();

	for(i = 0; i <= 100; i += 20)
	{
		CSTCPrint("设置OSG透明度到:%d\n",i);
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetOSDTransparency(i), "设置透明度失败");
		CSUDIOSThreadSleep(1500);
	}

	CSTCPrint("屏幕OSG从完全透明逐渐变成完全清晰,且video不受影响?? (Y/N)\n");
	CSTK_ASSERT_TRUE_FATAL(CSTKWaitYes(), "check result failure!");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERStop(hPlayer), "停止播放节目失败");
	
	CSTK_FATAL_POINT
	 
	if (CSUDI_NULL != hPlayer)
	{
		 CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERClose(hPlayer), "关闭播放器失败");
		 hPlayer = CSUDI_NULL;
	}

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIVIDEOSetWindowSize(0,EM_UDI_VOUT_DEVICE_SD,NULL),
		"fail to set SD video to full screen");

	if (bSupportHD)
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIVIDEOSetWindowSize(0,EM_UDI_VOUT_DEVICE_HD,NULL),
			"fail to set HD video to full screen");
	}
	
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetOSDTransparency(100), "设置透明度失败\n");

	if (hSurfaceSD)
	{
		CSTK_ASSERT_TRUE_FATAL(
					(CSUDI_SUCCESS == CSUDIOSGFill(hSurfaceSD, NULL, 0x0)),
					"fill OSG to 0,recover test site failure"
				);
	}

	if (hSurfaceHD)
	{
		CSTK_ASSERT_TRUE_FATAL(
					(CSUDI_SUCCESS == CSUDIOSGFill(hSurfaceHD, NULL, 0x0)),
					"fill OSG to 0,recover test site failure"
				);
	}

	return CSUDI_TRUE;
}

//@CASEGROUP:SetOSDTransparency 
//@DESCRIPTION:测试分辨率的切换不会造成OSG透明度的丢失
//@PRECONDITION:节目播放正常
//@INPUT:1. Saturation = [0-100]
//@EXPECTATION:1.每次设置返回CSUDI_SUCCESS,且屏幕变化和设置值相符
//@EXECUTIONFLOW:1.调用player模块接口播放标清PAL码流,详细步骤请参考测试用例CSTC_SCREEN_TEST_IT_SetResolution_0003步骤1
//@EXECUTIONFLOW:2.按如下步骤在高标清显存上画测试图像(如果平台支持高清才包含高清):
//@EXECUTIONFLOW:2.1 调用CSUDIOSGGetDisplaySurface获取显存句柄,期望返回CSUDI_SUCCESS
//@EXECUTIONFLOW:2.2 调用CSUDIOSGFill在{50,50}位置填充{350,350}的红色OSG,期望返回CSUDI_SUCCESS
//@EXECUTIONFLOW:2.3 调用CSUDIVIDEOSetWindowSize设置视频输出小窗口模式,期望返回CSUDI_SUCCESS
//@EXECUTIONFLOW:3.调用CSUDISCREENSetOSDTransparency设置OSG的透明度为40,期望OSG为半透明效果.
//@EXECUTIONFLOW:4.调用CSUDISCREENSetResolution设置标清分辨率为NTSC,高清为720P_50HZ(如果支持高清的话)
//@EXECUTIONFLOW:5.调用CSUDISCREENSetResolution设置标清分辨率为PAL,高清为1080I_50HZ(如果支持高清的话)
//@EXECUTIONFLOW:6.等待确认分辨率的设置是否会造成OSG透明度设置的丢失
//@EXECUTIONFLOW:7.调用CSUDIPLAYERStop停止测试节目播放
//@EXECUTIONFLOW:8. 调用CSUDIPLAYERClose关闭播放器
//@EXECUTIONFLOW:9. 调用CSUDIVIDEOSetWindowSize设置高标清视频窗口为全屏
//@EXECUTIONFLOW:10.调用CSUDISCREENSetOSDTransparency设置OSG透明度为100,恢复测试现场
//@EXECUTIONFLOW:11.调用CSUDIOSGFill将高标清显存清为0x0
CSUDI_BOOL CSTC_SCREEN_TEST_IT_0001( void )
{
	CSUDI_HANDLE hPlayer = CSUDI_NULL;
	CSUDI_HANDLE hSurfaceSD = CSUDI_NULL,hSurfaceHD = CSUDI_NULL;
	int i = 0;
	CSUDIOSGRect_S stRect={50,50,350,350};
	CSUDIWinRect_S stRectVideo;
	BOOL bSupportHD=!NotSupportHD();

#define SCREEN_COUNT	(2)
	
	CSUDISCREENType_E eScreenDevice[SCREEN_COUNT];
	CSUDISCREENResolution_E eResolutionSD[SCREEN_COUNT] = 
	{
		EM_UDISCREEN_RESOLUTION_NTSC,
		EM_UDISCREEN_RESOLUTION_PAL
	};
	CSUDISCREENResolution_E eResolutionHD[2]=
	{
		EM_UDISCREEN_RESOLUTION_720P_50HZ,
		EM_UDISCREEN_RESOLUTION_1080I_50HZ
	};

	eScreenDevice[0] = EM_UDI_VOUT_DEVICE_SD;
	eScreenDevice[1] = EM_UDI_VOUT_DEVICE_HD;

	memcpy(&stRectVideo,&stRect,sizeof(stRect));
	stRectVideo.m_nX += 50;
	stRectVideo.m_nY += 50;
	
	hPlayer = PlaySDProgram(EM_UDI_VID_STREAM_MPEG2, EM_UDISCREEN_RESOLUTION_PAL);  //调用PLAYER播放测试码流

	CSTK_ASSERT_TRUE_FATAL(CSUDI_NULL != hPlayer, "启动节目播放失败");

	if (!IsShareHDDisplay())
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIOSGGetDisplaySurface(0,&hSurfaceSD),
			"fail to get SD display surface");
		
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIOSGFill(hSurfaceSD,&stRect,0xFFFF0000),
				"fail to fill SD region {50,50,350,350} to red");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIVIDEOSetWindowSize(0,EM_UDI_VOUT_DEVICE_SD,&stRectVideo),
			"fail to set SD video to little box");
	}

	if (bSupportHD)
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIOSGGetDisplaySurface(1,&hSurfaceHD),
		"fail to get SD display surface");
	
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIOSGFill(hSurfaceHD,&stRect,0xFFFF0000),
				"fail to fill HD region {50,50,350,350} to red");
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIVIDEOSetWindowSize(0,EM_UDI_VOUT_DEVICE_HD,&stRectVideo),
			"fail to set HD video to little box");
	}

	CSTK_ASSERT_TRUE_FATAL((CSUDI_SUCCESS == CSUDIOSGSync()),"Sync Failure !");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetOSDTransparency(40), "设置透明度失败");
	
	CSTCPrint("1. 屏幕显示为红色OSG框以及视频小窗口,且两者有部分重叠(Y/N)\n");
	CSTCPrint("2. OSG为半透明效果(Y/N)\n");
	CSTK_ASSERT_TRUE_FATAL(CSTKWaitYes(), "check result failure!");

	CSTCPrint("按任意键进行分辨率的设置,请留意分辨率的切换是否会造成OSG透明度的丢失\n");
	CSTKWaitAnyKey();

	g_scResolution[0] = EM_UDISCREEN_RESOLUTION_INVALID;
	g_scResolution[1] = EM_UDISCREEN_RESOLUTION_INVALID;
	
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENGetResolution(eScreenDevice[0], &g_scResolution[0]), "获取测试前分辨率失败");

	if (bSupportHD)
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENGetResolution(eScreenDevice[1], &g_scResolution[1]), "获取测试前分辨率失败");
	}

	for(i=0;i<SCREEN_COUNT;i++)
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(&eScreenDevice[0],&eResolutionSD[i],1),
				"fail to set SD screen resoluction");
		if (bSupportHD)
		{
			CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(&eScreenDevice[1],&eResolutionHD[i],1),
				"fail to set HD screen resoluction");
		}
		CSUDIOSThreadSleep(3000);
	}

	CSTCPrint("分辨率切换不会造成OSG透明度丢失?(Y/N)\n");
	CSTK_ASSERT_TRUE_FATAL(CSTKWaitYes(), "check result failure!");

	CSTK_FATAL_POINT
	 
	if (CSUDI_NULL != hPlayer)
	{
		 CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERClose(hPlayer), "关闭播放器失败");
		 hPlayer = CSUDI_NULL;
	}

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIVIDEOSetWindowSize(0,EM_UDI_VOUT_DEVICE_SD,NULL),
		"fail to set SD video to full screen");

	if (bSupportHD)
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIVIDEOSetWindowSize(0,EM_UDI_VOUT_DEVICE_HD,NULL),
		"fail to set HD video to full screen");
	}

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetOSDTransparency(100), "设置透明度失败");

	if (hSurfaceSD)
	{
		CSTK_ASSERT_TRUE_FATAL(
					(CSUDI_SUCCESS == CSUDIOSGFill(hSurfaceSD, NULL, 0x0)),
					"fill OSG to 0,recover test site failure"
				);
	}

	if (hSurfaceHD)
	{
		CSTK_ASSERT_TRUE_FATAL(
					(CSUDI_SUCCESS == CSUDIOSGFill(hSurfaceHD, NULL, 0x0)),
					"fill OSG to 0,recover test site failure"
				);
	}

	if (g_scResolution[0] > EM_UDISCREEN_RESOLUTION_INVALID && g_scResolution[0] <= EM_UDISCREEN_RESOLUTION_576P)
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(&eScreenDevice[0], &g_scResolution[0], 1),
				"还原测试前分辨率失败");
	}

	if (bSupportHD
		&& (g_scResolution[1] >= EM_UDISCREEN_RESOLUTION_720P && g_scResolution[1] < EM_UDISCREEN_RESOLUTION_NUM))
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDISCREENSetResolution(&eScreenDevice[1], &g_scResolution[1], 1),
				"还原测试前分辨率失败");
	}

	return CSUDI_TRUE;
}


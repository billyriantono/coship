/* --------------------------------------------------------------------
注意：
1.在需要与用户交互的测试用例中，可以：
	a. 使用CSTKWaitAnyKey等待用户输入任意按键
	b. 使用CSTKWaitYes等待用户输入YES
2.测试用例函数命名：测试用例ID，"测试用例ID"定义在测试用例文档中
-----------------------------------------------------------------------*/
#include "cs_filetestcase.h"
#include "udi2_player.h"
#include "udi2_demux.h"
#include "udi2_audio.h"
#include "udi2_video.h"
#include "udi2_tuner.h"
#include "udi2_os.h"
#include "udi2_public.h"
#include "udi2_inject.h"
#include "udi2_tuner.h"
#include "udi2_fs.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "../../cs_udi2testcase.h"
#include "cs_testkit_porting.h"
#include "udiplus_os.h"

#define REPEAT_TIMES (2)
#define MAX_SLEEP_NUM 32
#define NUSERDATA (1234)
#define INJECT_SEEK_SET (0)
#define INJECT_SEEK_CUR (1)
#define INJECT_SEEK_END (2)
#define INJECTTHREADPRO (100)
#define INJECTTHREADBUF (1024 * 148)
#define INJECT_INVALID_PID (0x1fff)
#define MAX_PESES_COUNT_IN_TS (EM_UDI_CONTENT_PCR+1)
#define LivePlayer (0)
#define SLEEP_TIME (2000)

#define REWIND_TO_BEGIN (1)
#define FORWARD_TO_END   (2)
#define PLAY_TO_END FORWARD_TO_END

typedef enum
{
	EM_UDI_FILEFORMAT_TS,
	EM_UDI_FILEFORMAT_WMV,
	EM_UDI_FILEFORMAT_AVI,
	EM_UDI_FILEFORMAT_RMVB,
	EM_UDI_FILEFORMAT_FLV,
	EM_UDI_FILEFORMAT_MP3,
	EM_UDI_FILEFORMAT_MP4,
	EM_UDI_FILEFORMAT_MOV,
	EM_UDI_FILEFORMAT_MKV,
	EM_UDI_FILEFORMAT_MAX
}CSUDIFileFormat_E;

typedef enum
{
	EM_UDI_FILENAME_TS_SD,
	EM_UDI_FILENAME_TS_HD,
	EM_UDI_FILENAME_WMV,
	EM_UDI_FILENAME_AVI_SD,
	EM_UDI_FILENAME_AVI_HD,
	EM_UDI_FILENAME_RMVB_SD,
	EM_UDI_FILENAME_RMVB_HD,
	EM_UDI_FILENAME_FLV,
	EM_UDI_FILENAME_MP3,
	EM_UDI_FILENAME_M4A,
	EM_UDI_FILENAME_AC3,
	EM_UDI_FILENAME_AAC,
	EM_UDI_FILENAME_MP4_SD,
	EM_UDI_FILENAME_MP4_HD,
	EM_UDI_FILENAME_3GP,
	EM_UDI_FILENAME_MOV,
	EM_UDI_FILENAME_MKV_SD,
	EM_UDI_FILENAME_MKV_HD,
	EM_UDI_FILENAME_MKV_HD_BSIZE,
	EM_UDI_FILENAME_MA,
	EM_UDI_FILENAME_MV,
	EM_UDI_FILENAME_MS,
	EM_UDI_FILENAME_MAX
}CSUDIFileNAME_E;

typedef struct _FilePath_S
{
	char *filename;

}FilePath_S;

typedef struct _FileTestStreamInfo_S
{
	CSUDIPlayerStreamType_E m_eStreamType; ///< 文件流类型
	CSUDI_INT64 m_n64FileSize; ///< 文件大小，单位字节
	CSUDI_INT64 m_n64StartTime; ///< 文件播放起始时间，单位ms
	CSUDI_INT64 m_n64Duration; ///< 文件总时长，单位ms
	CSUDI_UINT32 m_u32Bps; ///< 文件码率，bits/s
	char m_acFileName[CSUDI_PLAYER_MAX_FILE_NAME_LEN]; ///< 文件名称
	CSUDI_UINT32 m_u32ProgramNum; ///< 实际节目个数
	CSUDI_UINT32 m_u32AudStreamNum; ///< 音频流个数
	CSUDI_UINT32 m_u32SubTitleNum; ///< 字幕个数
	CSUDI_UINT32 m_u32VideoFormat; ///< 视频编码格式，值定义参考::HI_UNF_VCODEC_TYPE_E jia
	CSUDI_UINT16 m_u16Width; ///< 宽度，单位像素 jia
	CSUDI_UINT16 m_u16Height; ///< 高度，单位像素 jia
	CSUDI_UINT16 m_u16Channels; ///< 声道数, 1 or 2 jia
	CSUDI_UINT32 m_u32SampleRate; ///< 8000,11025,441000,... jia
	CSUDI_UINT32 m_u32AudioFormat; ///< 音频编码格式，值定义参考::HA_FORMAT_E jia
}FileTestStreamInfo_S;

static FilePath_S stFilePath[] =
{
	{"./testdata/fileplay/test_sd.ts"},
	{"./testdata/fileplay/test_hd.ts"},
	{"./testdata/fileplay/test.wmv"},
	{"./testdata/fileplay/test_sd.avi"},
	{"./testdata/fileplay/test_hd.avi"},
	{"./testdata/fileplay/test_sd.rmvb"},
	{"./testdata/fileplay/test_hd.rmvb"},
	{"./testdata/fileplay/test.flv"},
	{"./testdata/fileplay/test.mp3"},
	{"./testdata/fileplay/test.m4a"},
	{"./testdata/fileplay/test.ac3"},
	{"./testdata/fileplay/test.aac"},
	{"./testdata/fileplay/test_sd.mp4"},
	{"./testdata/fileplay/test_hd.mp4"},
	{"./testdata/fileplay/test.3gp"},
	{"./testdata/fileplay/test.mov"},
	{"./testdata/fileplay/test_sd.mkv"},
	{"./testdata/fileplay/test_hd.mkv"},
	{"./testdata/fileplay/test_hd_bs.mkv"},
	{"./testdata/fileplay/test_ma.trp"},
	{"./testdata/fileplay/test_mv.ts"},
	{"./testdata/fileplay/test_ms.ts"},
};

static FileTestStreamInfo_S s_sStreamInfo[] = {
	{
		EM_UDIFILEPLAYER_STREAM_ES,
		238699652,
		37,
		76354,
		25009505,
		"test_ma.trp",
		1,
		5,
		0,
		0,
	 	720,
		576,
		6,
	 	44100,
		3
	},

	{
	},

	{

	},

	{
	}
};

static int s_svrfiletime[5] = {145, 67, 459, 312, 259};

static CSUDIFilePlayerEventType_E s_ePlayerEvent = EM_UDIFILEPLAYER_MAXEVENTTYPE;

CSUDI_BOOL CSTC_FILE_Init(void)
{
	//在本测试用例集执行前调用
	CSUDIAUDIOSetVolume(0, 30);

	CSUDIVIDEOShow(0, CSUDI_TRUE);
	CSUDIVIDEOSetAspectRatio(0, EM_UDI_VOUT_DEVICE_SD, EM_UDIVIDEO_ASPECT_RATIO_AUTO);
	CSUDIVIDEOSetMatchMethod(0, EM_UDI_VOUT_DEVICE_HD,EM_UDIVIDEO_MATCH_METHOD_IGNORE);

	return CSUDI_TRUE;
}

BOOL CSTC_FILE_UnInit(void)
{
	//在本测试用例集执行后调用
	return TRUE;
}

static void PlayCallback (CSUDI_HANDLE hPlayer,CSUDIPlayerEventType_E eEvent,void * pvUserData)
{
       if(EM_UDIPLAYER_VIDEO_FRAME_COMING == eEvent)
       {
           if (pvUserData != NULL)
    		{
    			CSTCPrint("[%s, %d] eEvent:%d, eventData:%s\r\n", __FUNCTION__, __LINE__, eEvent, (char *)pvUserData);
    		}
       }
}

static void FilePlayCallback(CSUDI_HANDLE hPlayer, CSUDIFilePlayerEventType_E ePlayerEvent, void *pvEventData, void * pvUserData)
{
	if (EM_UDIFILEPLAYER_VIDEO_UNDERFLOW == ePlayerEvent)
	{
		if (pvEventData != NULL)
		{
			CSTCPrint("[%s, %d] eventType:%d, eventData:%s\r\n", __FUNCTION__, __LINE__, ePlayerEvent, (char *)pvEventData);
		}
	}
	else if (EM_UDIFILEPLAYER_BEGIN_OF_STREAM == ePlayerEvent)
	{
		unsigned char *pucMoveToend = pvUserData;

		CSTCPrint("已到最前！\n");
		if (pvUserData != NULL)
		{
			*pucMoveToend = REWIND_TO_BEGIN;
		}
	}
	else if (EM_UDIFILEPLAYER_END_OF_STREAM == ePlayerEvent)
	{
		unsigned char *pucMoveToend = pvUserData;

		CSTCPrint("已到最后！\n");
		if (pvUserData != NULL)
		{
			*pucMoveToend = FORWARD_TO_END;
		}
	}
}

static void FilePlayEventCallback(CSUDI_HANDLE hPlayer, CSUDIFilePlayerEventType_E ePlayerEvent, void *pvEventData, void * pvUserData)
{
	switch(ePlayerEvent)
	{
		case EM_UDIFILEPLAYER_ERR_ABORTED: ///< 用户中止操作，中止成功消息
			CSTCPrint("[FilePlayEventCallback]UDI File Play Aborted error !\n\r");
			break;
		case EM_UDIFILEPLAYER_ERR_DECODE: ///< 解码器出错
			CSTCPrint("[FilePlayEventCallback]UDI File Play Decoder error !\n\r");
			break;
		case EM_UDIFILEPLAYER_ERR_FORMAT: ///< 媒体文件不存在或文件格式出错
			s_ePlayerEvent = EM_UDIFILEPLAYER_ERR_FORMAT;
			CSTCPrint("[FilePlayEventCallback]UDI File Play Format error !\n\r");
			break;
		case EM_UDIFILEPLAYER_ERR_SEEK: ///< 播放定位出错时，抛一次消息
			s_ePlayerEvent = EM_UDIFILEPLAYER_ERR_SEEK;
			CSTCPrint("[FilePlayEventCallback]UDI File Play Seek error !\n\r");
			break;
		case EM_UDIFILEPLAYER_ERR_PAUSE: ///< 暂停操作失败时，抛一次消息
			CSTCPrint("[FilePlayEventCallback]UDI File Play Pause error !\n\r");
			break;
		case EM_UDIFILEPLAYER_ERR_RESUME:  ///< 状态恢复失败时，抛一次消息
			CSTCPrint("[FilePlayEventCallback]UDI File Play Resume error !\n\r");
			break;
		case EM_UDIFILEPLAYER_ERR_SETSPEED: ///< 设置速率失败时，抛一次消息
			s_ePlayerEvent = EM_UDIFILEPLAYER_ERR_SETSPEED;
			CSTCPrint("[FilePlayEventCallback]UDI File Play Set Speed error !\n\r");
			break;
		case EM_UDIFILEPLAYER_ERR_NETWORK:  ///< 网络异常，m_pvData为int，值为HTTP 标准error code, 比如http错误为 200, pvEventData值为200；
			CSTCPrint("[FilePlayEventCallback]UDI File Play Network error !\n\r");
			break;

		case EM_UDIPFILELAYER_STATE_LOADING: ///< 缓存开始,每次进入缓存状态都要抛一次
			CSTCPrint("[FilePlayEventCallback]UDI File Play State Loading !\n\r");
			break;
		case EM_UDIPFILELAYER_STATE_LOADED: ///< 缓存结束,有足够数据开始播放
			CSTCPrint("[FilePlayEventCallback]UDI File Play State Loaded !\n\r");
			break;
		case EM_UDIPFILELAYER_STATE_HAVEMETADATA:  ///< 能够获取一些基本信息，如片长
			CSTCPrint("[FilePlayEventCallback]UDI File Play State Havemetadata !\n\r");
			break;
		case EM_UDIPFILELAYER_STATE_ENOUGHDATA_FORPLAY:  ///< 有足够的数据进行播放，如进行快进快退都操作
			CSTCPrint("[FilePlayEventCallback]UDI File Play State Enoughdata For Play !\n\r");
			break;
		case EM_UDIPFILELAYER_STATE_DURATIONCHANGE:   ///< 节目总时长发生变化，需要播放器更新时长，可以通过CSUDIPLAYERGetDuration 获取新的总时长
			CSTCPrint("[FilePlayEventCallback]UDI File Play State Duration Change !\n\r");
			break;
		case EM_UDIPFILELAYER_STATE_RATECHANGE: ///< 节目总播放倍数发生变化，可以通过CSUDIPLAYERGetSpeed 获取新的播放速率
			CSTCPrint("[FilePlayEventCallback]UDI File Play State Rate Change !\n\r");
			break;
		case EM_UDIFILEPLAYER_STATE_LOADING_PROGRESS: ///< 等待播放，缓存数据的进度值；进度值和基值或起来，m_pvData为int, 百分制，比如缓冲到25%,  pvEventData值为25；
			CSTCPrint("[FilePlayEventCallback]UDI File Play State Loading Progress !\n\r");
			break;
		case EM_UDIFILEPLAYER_STATE_LOADING_DOWNLOADRATE: ///< 下载速率, pvEventData为int, 单位:kbytes/s, 比如网络下载速度25kbits/s, pvData值为25；
			CSTCPrint("[FilePlayEventCallback]UDI File Play State Loading Download Rate !\n\r");
			break;

		case EM_UDIFILEPLAYER_STATE_STOP:                 ///< 停止播放器操作成功
			s_ePlayerEvent = EM_UDIFILEPLAYER_STATE_STOP;
			CSTCPrint("[FilePlayEventCallback]UDI File Play State Stop !\n\r");
			break;
		case EM_UDIFILEPLAYER_STATE_START:                ///< 启动播放器操作成功
			s_ePlayerEvent = EM_UDIFILEPLAYER_STATE_START;
			CSTCPrint("[FilePlayEventCallback]UDI File Play State Start !\n\r");
			break;
		case EM_UDIFILEPLAYER_STATE_PAUSE:              ///< 暂停播放器操作成功
			s_ePlayerEvent = EM_UDIFILEPLAYER_STATE_PAUSE;
			CSTCPrint("[FilePlayEventCallback]UDI File Play State Pause !\n\r");
			break;
		case EM_UDIFILEPLAYER_SEEK_FINISH:           ///< 选时播放定位成功
			s_ePlayerEvent = EM_UDIFILEPLAYER_SEEK_FINISH;
			CSTCPrint("[FilePlayEventCallback]UDI File Play Seek Finish !\n\r");
			break;
		default:
			break;
	}
}

typedef struct
{
	const char*          m_pcTsFilename;  	  //码流文件名称
	const char* 		 m_pcServiceDescript; 	//码流描述
	int                  m_nVideoPid;           ///< 数据所在PID，-1表示不存在
	CSUDIVIDStreamType_E m_eVidStreamType;      ///视频类型
	int                  m_nAudioPid;           ///< 数据所在PID ，-1表示不存在
	CSUDIAUDStreamType_E m_eAudStreamType;      ///<音频类型
	int 				 m_nPcrPid;				///< PCR类型PID，-1表示不存在
	int 				 m_nSubPid;				///<SUBTITLE类型的PID，-1表示不存在
	int 				 m_nTelPid;				///<TELETEXT类型的PID，-1表示不存在
}Player_TestServiceInfo_S;

typedef struct _TSStreamInfo_S
{
    char * m_pcName; //注入文件的名
    CSUDIStreamInfo_S    m_TSContentInfo[MAX_PESES_COUNT_IN_TS];
    unsigned int m_uBufferLen;
    CSUDI_BOOL m_bRun;
    CSUDI_HANDLE m_hInjecter;
    CSUDI_BOOL m_bSeek;
    CSUDI_HANDLE m_hSeekEvent;
    CSUDI_BOOL m_bOpenFileSuccess;
}TS_StreamInfo_S;

static Player_TestServiceInfo_S g_sPlayer_SeviceInfo[] =
{
	{
		"Audio&Video Test_27Mbps_20070524.ts",
		"多语言AC3MP2",
		60,
		EM_UDI_VID_STREAM_MPEG2,
		62,
		EM_UDI_AUD_STREAM_MPEG2,
		60,
		-1,
		-1
	}
};

static CSUDI_BOOL PLAYER_iLockTuner(void)
{
	return CSTC_UDI2PortingLock(0, g_sPlayer_SeviceInfo[LivePlayer].m_pcTsFilename);
}

static TS_StreamInfo_S g_TS_StreamInfo[] =
{
	{
        	"EPG_0606_121458.ts",/*EM_INJECT_TS_MPEG2_MPEG2*/
		{
			{0x15AE, EM_UDI_CONTENT_VIDEO, {EM_UDI_VID_STREAM_MPEG2}},
			{0x15AF, EM_UDI_CONTENT_AUDIO, {EM_UDI_AUD_STREAM_MPEG2}},
			{INJECT_INVALID_PID, 0, {0}},
			{INJECT_INVALID_PID, 0, {0}},
		 	{INJECT_INVALID_PID, 0, {0}},
	   	},
		10*1024,
		CSUDI_TRUE,
		CSUDI_NULL,
		CSUDI_FALSE,
		CSUDI_NULL,
		CSUDI_FALSE,
	}
};

static void * InjectFSOpen(const char * pcFileName, const char * pcMode)
{
	return (void *)CSTKP_FOpen(pcFileName, pcMode);
}

static int  InjectFSSeek(void *pFile, long lOffset, unsigned int uOrigin)
{
	return CSTKP_FSeek((CSUDI_HANDLE)pFile, lOffset,uOrigin);
}

static long  InjectFSTell(void *pFile)
{
	return CSTKP_FTell((CSUDI_HANDLE)pFile);
}

static int InjectFSRead(void* pFile, char * pcBuf, unsigned int uCount)
{
	return CSTKP_FRead(pcBuf, 1, uCount, (CSUDI_HANDLE)pFile);
}

static int  InjectFSClose(void *pFile)
{
	return CSTKP_FClose((CSUDI_HANDLE)pFile);
}

static CSUDI_BOOL getInjecterFile(const char *pFileName, void **file, long *pLength)
{
	void *pFileTemp;
	int nReturn = -1;

	pFileTemp = InjectFSOpen(pFileName, "r");

	if (!pFileTemp)
	{
		CSTCPrint("测试所需文件 %s 打开失败，请确保testdata目录下存在该文件!!!\n", pFileName);
		CSTCPrint("[in getInjecterFile]InjectFSOpen is failed!file name is %s\n", pFileName);
		return CSUDI_FALSE;
	}

	nReturn = InjectFSSeek(pFileTemp,0,INJECT_SEEK_END);
	if(nReturn != 0)
	{
		CSTCPrint("[in getInjecterFile]InjectFSSeek end is failed!\n\r");
		return CSUDI_FALSE;
	}

	*pLength = InjectFSTell(pFileTemp);
	if(*pLength < 0)
	{
		CSTCPrint("[in getInjecterFile]InjectFSTell is failed!\n\r");
		return CSUDI_FALSE;
	}

	nReturn = InjectFSSeek(pFileTemp,0,INJECT_SEEK_SET);
	if(nReturn != 0)
	{
		CSTCPrint("[in getInjecterFile]InjectFSSeek begin is failed!\n\r");
		return CSUDI_FALSE;
	}

	*file = pFileTemp;
	return CSUDI_TRUE;
}

static void TS_injectTask(void * TSStreamInfo)
{
	void * ppvBuffer = NULL;

	unsigned  int  uLength = 0;

	TS_StreamInfo_S *pTSStreamInfo = (TS_StreamInfo_S*)TSStreamInfo;

	CSUDI_HANDLE hInjecter = pTSStreamInfo->m_hInjecter;

	int nBlockNum = 0;
	long nFileAllLength = 0;  //文件总长度
	int nFileCurrentPos = 0; //文件当前位置
	int nFileLeftLength = 0; //剩余未读文件长度
	int nInjectedLength = 0; //可注入的大小
	int nReadLength = 0;

	void* file = CSUDI_NULL;

	if(!getInjecterFile((const char*)pTSStreamInfo->m_pcName, &file, &nFileAllLength))
	{
       		CSTCPrint("getInjecterFile failed!\n\r");
       		pTSStreamInfo->m_bRun = CSUDI_TRUE;
     		pTSStreamInfo->m_bOpenFileSuccess = CSUDI_FALSE;
       		return;
	}

	if (pTSStreamInfo->m_uBufferLen<10240||pTSStreamInfo->m_uBufferLen>1024*1024)
	{
		pTSStreamInfo->m_uBufferLen = 10*1024;
	}
	pTSStreamInfo->m_bOpenFileSuccess = CSUDI_TRUE;
	pTSStreamInfo->m_bRun = CSUDI_TRUE;

	while(pTSStreamInfo->m_bRun)
	{
		//求出当前剩余未读文件的长度
		nFileCurrentPos=InjectFSTell(file);	//当前已读文件的长度

		nFileLeftLength=nFileAllLength-nFileCurrentPos; //剩余未读文件的长度

                uLength = 0;

		CSUDIINJECTERGetFreeBuffer (hInjecter, &ppvBuffer,&uLength);

		if(uLength != 0)
		{
			nReadLength = pTSStreamInfo->m_uBufferLen < nFileLeftLength ? pTSStreamInfo->m_uBufferLen:nFileLeftLength;

			if(nReadLength < uLength) //剩余文件长度小于可注入空间的长度
			{
				nInjectedLength = nReadLength;
			}
			else  //剩余文件长度大于等于可注入空间的长度
			{
				nInjectedLength = uLength;
			}

			nBlockNum= InjectFSRead(file,(void*)ppvBuffer, nInjectedLength); //读取相应长度的文件

			CSUDIINJECTERWriteComplete(hInjecter,nInjectedLength);

			if(nFileLeftLength == 0)
			{
				InjectFSSeek(file,0,INJECT_SEEK_SET);
			}
		}
		else
	        {
			CSUDIOSThreadSleep(10);
	        }
	}

	if(!pTSStreamInfo->m_bRun )
	{
		InjectFSClose(file);
	}

}

//查找支持nDemxType类型的demx
//nDemxType参见CSUDIDEMUXWorkType_E
static int searchDemuxID_Y_InInject(int nDemxType)
{
	int i = 0;
	int  nDemuxCount = 0;
	int nDemuxID = -1;
	CSUDIDEMUXCapability_S   sDemuxCapabilityInfo;

	if (CSUDI_SUCCESS == CSUDIDEMUXGetCount(&nDemuxCount))            //先取得demux的数量
	{
		for (i=0; i<nDemuxCount; i++)
		{
			if (CSUDI_SUCCESS == CSUDIDEMUXGetCapability(i, &sDemuxCapabilityInfo))    //取相对应ID=i 的demux的能力
			{
				if((sDemuxCapabilityInfo.m_dwWorkTypeMask & nDemxType ) != 0)
				{
					nDemuxID=i;
					break;
				}
			}
		}
	}
	CSTCPrint("nDemuxID nDemuxID=%d\n\r",nDemuxID);
	return  nDemuxID;
}

//查找支持nAudioType类型的Audio Decoder
//nAudioType参见CSUDIAUDStreamType_E, 类型为EM_UDI_AUD_STREAM_AC3特殊处理
static int searchAudioID_Y_InInject(int nAudioType)
{
	CSUDIAUDStreamType_E  eAudStreamTypeNum=EM_UDI_AUD_STREAMTYPE_NUM;
	CSUDIAUDIOCapability_S   sAudioCapabilityInfo;
	int nAudioCount = 0;
	int nAudioID = -1;
	int i = 0;
	int j = 0;

	if (CSUDI_SUCCESS == CSUDIAUDIOGetCount(&nAudioCount))           //先取得音频解码器的数量
	{
		for (i=0; i<nAudioCount; i++)
		{

			if(CSUDI_SUCCESS == CSUDIAUDIOGetCapability(i, &sAudioCapabilityInfo))           //到相关ID的音频解码器的能力
			{
				if(nAudioType == EM_UDI_AUD_STREAM_AC3)
				{
					for (j=0; j<eAudStreamTypeNum; j++)                                //其中能力包含两方面：支持解码的能力与支持直接输出的能力
					{
						if (sAudioCapabilityInfo.m_sSupportByPass[j] == EM_UDI_AUD_STREAM_UNKNOWN )
						{
							break;
						}
						if (sAudioCapabilityInfo.m_sSupportByPass[j] == nAudioType)
						{
							nAudioID=i;
							break;
						}
					}
				}
				else
				{
					for (j=0; j<eAudStreamTypeNum; j++)                                //其中能力包含两方面：支持解码的能力与支持直接输出的能力
					{
						if (sAudioCapabilityInfo.m_sSupportDecode[j] == EM_UDI_AUD_STREAM_UNKNOWN )
						{
                                                 //CSTKPrint("eAudStreamTypeNum=%d\n",sAudioCapabilityInfo.m_sSupportDecode[j]);
							break;
						}
						if (sAudioCapabilityInfo.m_sSupportDecode[j] == nAudioType)
						{
							nAudioID=i;
							break;
						}
					}
				}

				if (nAudioID != -1)
					break;
			}
		}
	}

	return nAudioID;
}

static int searchVideoID_Y_InInject(int type_index)
{
	int i;
	int j;
	CSUDIVIDStreamType_E  eVidStreamTypeNum=EM_UDI_VID_STREAMTYPE_NUM ;
	int nVideoCount;
	int nVidioID=-1;
	CSUDIVIDEOCapability_S   sVideoCapabilityInfo;

	CSUDIVIDEOGetCount(&nVideoCount);                                      //先取数量

	for ( i=0;i < nVideoCount; i ++)
	{
		CSUDIVIDEOGetCapability( i , &sVideoCapabilityInfo);	//再取相关能力，与音频不同的是视频只有支持的视频类型一项

		for (j=0; j < eVidStreamTypeNum; j++)
		{
			if (sVideoCapabilityInfo.m_eStreamType[j] == EM_UDI_VID_STREAM_UNKNOWN )
			{
				break;
			}
			if (sVideoCapabilityInfo.m_eStreamType[j]==type_index)
			{
				nVidioID=i;
				break;
			}
		}

		if (nVidioID!=-1)
		{
			break;
		}
	}

	return nVidioID;

}

static int  getTSInjectInfo(CSUDIINJECTERChnl_S  *psInjecterChnl,CSUDIPlayerChnl_S  *psPlayerChnl,CSUDIINJECTEROpenParam_S *pOpenParam ,CSUDIStreamInfo_S  sStreamInfo[])
{
	int nAudioId = -1;
	int nVideoId = -1;
	int nDemuxID = -1;
	int i = 0;
	int nStreamCnt = 0;
	TS_StreamInfo_S *pTSStreamInfo = &g_TS_StreamInfo[LivePlayer];

//注入的音视频解码器
	psInjecterChnl->m_nAudioDecoder=-1;//nAudioId;
	psInjecterChnl->m_nVideoDecoder=-1;
	psInjecterChnl->m_nDemux=-1;

	nDemuxID=searchDemuxID_Y_InInject(EM_UDI_DEMUX_INJECT);
	if(nDemuxID == -1)
	{
		CSTCPrint( "没有支持注入的demux! \n\r");
        	CSTCPrint("There's no demux device supporting inject \n");
       	        return -1;
	}

	psPlayerChnl->m_nDemux = nDemuxID;

	for (i=0;i<MAX_PESES_COUNT_IN_TS;i++)
	{
	    if (pTSStreamInfo->m_TSContentInfo[i].m_nPid == INJECT_INVALID_PID)
		{
			continue;
		}

		if (pTSStreamInfo->m_TSContentInfo[i].m_eContentType == EM_UDI_CONTENT_VIDEO)
		{
			nVideoId = searchVideoID_Y_InInject(pTSStreamInfo->m_TSContentInfo[i].m_uStreamType.m_eVideoType);
			if(nVideoId ==-1)
			{
				CSTCPrint( "没有支持指定类型视频解码器!\n\r");
                                CSTCPrint("There's no video decode supporting stream type %d \n",\
                                pTSStreamInfo->m_TSContentInfo[i].m_uStreamType.m_eVideoType);
                                return -1;
			}
			psPlayerChnl->m_nVideoDecoder = nVideoId;

		}
		else if (pTSStreamInfo->m_TSContentInfo[i].m_eContentType == EM_UDI_CONTENT_AUDIO)
		{
			nAudioId = searchAudioID_Y_InInject(pTSStreamInfo->m_TSContentInfo[i].m_uStreamType.m_eAudioType);
			if(nAudioId ==-1)
			{
				CSTCPrint( "没有支持指定类型音频解码器!\n\r");
                		CSTCPrint("There's no audio decode supporting stream type %d \n",\
                    		pTSStreamInfo->m_TSContentInfo[i].m_uStreamType.m_eAudioType);
                		return -1;
			}
			psPlayerChnl->m_nAudioDecoder = nAudioId;
		}

		memcpy(&sStreamInfo[i],&(pTSStreamInfo->m_TSContentInfo[i]),sizeof(CSUDIStreamInfo_S));
        	nStreamCnt++;

	}

	if(nStreamCnt<1)//至少注入一路类型(video,audio,pcr...)
	{
        	CSTCPrint("没有配置注入类型!!\n");
        	CSTCPrint("There's no stream type configed !!\n");
        	return -1;
	}

//播放通道
	psInjecterChnl->m_nDemux=psPlayerChnl->m_nDemux;

	pOpenParam->m_eInjecterType = EM_INJECTER_TS;
	pOpenParam->m_eContentType = EM_UDIINJECTER_CONTENT_DEFAULT;

	return nStreamCnt;
}

//@CASEGROUP:CSUDIPLAYERProbe
//@DESCRIPTION:测试参数的有效性，传入参数非法，查看是否返回CSUDIPLAYER_ERROR_BAD_PARAMETER
//@PRECONDITION:
//@INPUT:1、参数为CSUDI_NULL
//@EXPECTATION:返回CSUDIPLAYER_ERROR_BAD_PARAMETER
//@REMARK:
//@EXECUTIONFLOW:1、传入非法参数CSUDI_NULL，调用CSUDIPLAYERProbe返回CSUDIPLAYER_ERROR_BAD_PARAMETER
BOOL CSTC_FILE_TEST_IT_CSUDIPLAYERProbe_0001(void)
{
		CSTK_ASSERT_TRUE_FATAL(CSUDIPLAYER_ERROR_BAD_PARAMETER == CSUDIPLAYERProbe(CSUDI_NULL), "传入非法参数，应该返回CSUDIPLAYER_ERROR_BAD_PARAMETER");

		CSTK_FATAL_POINT;

		return CSUDI_TRUE;
}

//@CASEGROUP:CSUDIPLAYERAddFilePlayerCallback & CSUDIPLAYERDelFilePlayerCallback
//@DESCRIPTION:测试参数或应用场景在不符合接口设计要求，CSUDIPLAYERAddFilePlayerCallback & CSUDIPLAYERDelFilePlayerCallback函数的执行效果
//@PRECONDITION:PLAYER模块被正常初始化
//@INPUT:1、参数不符合设计要求
//@EXPECTATION:非CSUDI_SUCCESS
//@REMARK:CSUDIPLAYERAddFilePlayerCallback & CSUDIPLAYERDelFilePlayerCallback参数要求请参考UDI移植指南
//@EXECUTIONFLOW:1、打开一个本地文件播放器
//@EXECUTIONFLOW:2、hPlayer为CSUDI_NULL时，调用CSUDIPLAYERAddFilePlayerCallback，期望返回CSUDIPLAYER_ERROR_INVALID_HANDLE
//@EXECUTIONFLOW:3、fnPlayerCallback为CSUDI_NULL 时，调用CSUDIPLAYERAddFilePlayerCallback，期望返回非CSUDI_SUCCESS
//@EXECUTIONFLOW:4、hPlayer为CSUDI_NULL时，调用CSUDIPLAYERDelFilePlayerCallback，期望返回CSUDIPLAYER_ERROR_INVALID_HANDLE
//@EXECUTIONFLOW:5、fnPlayerCallback为CSUDI_NULL时，调用CSUDIPLAYERDelFilePlayerCallback，期望返回非CSUDI_SUCCESS
//@EXECUTIONFLOW:6、重复步骤2-步骤5指定次数
//@EXECUTIONFLOW:7、调用CSUDIPLAYERClose删除播放器
CSUDI_BOOL CSTC_FILE_TEST_IT_Add_DelPlayerCallback_0001(void)
{
	CSUDI_HANDLE   hPlayer = CSUDI_NULL;
	int  nUserData=NUSERDATA;
	int i=0;
	CSUDIPLAYERType_E ePlayerType=EM_UDIPLAYER_FILE;

	//创建播放器
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYEROpen (NULL,ePlayerType,&hPlayer), "步骤1失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_NULL != hPlayer, "步骤1失败");

	for (i=0;i<REPEAT_TIMES;i++)
	{
		//testing CSUDIPLAYERAddFilePlayerCallback
		CSTK_ASSERT_TRUE_FATAL(CSUDIPLAYER_ERROR_INVALID_HANDLE == CSUDIPLAYERAddFilePlayerCallback(CSUDI_NULL,FilePlayEventCallback,&nUserData), "步骤2 失败");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS != CSUDIPLAYERAddFilePlayerCallback(hPlayer,CSUDI_NULL,&nUserData), "步骤3 失败");

		//testing CSUDIPLAYERDelFilePlayerCallback
		CSTK_ASSERT_TRUE_FATAL(CSUDIPLAYER_ERROR_INVALID_HANDLE == CSUDIPLAYERDelFilePlayerCallback (CSUDI_NULL,FilePlayEventCallback,&nUserData), "步骤4 失败");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS != CSUDIPLAYERDelFilePlayerCallback(hPlayer,CSUDI_NULL,&nUserData), "步骤5 失败");
	}

	CSTK_FATAL_POINT;

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERClose (hPlayer), "步骤14 失败");

	return CSUDI_TRUE;
}

//@CASEGROUP:CSUDIPLAYERAddFilePlayerCallback & CSUDIPLAYERDelFilePlayerCallback
//@DESCRIPTION:测试所有参数并且应用场景均符合接口设计要求，CSUDIPLAYERAddFilePlayerCallback & CSUDIPLAYERDelFilePlayerCallback函数的执行效果
//@PRECONDITION:PLAYER模块被正常初始化
//@INPUT:1、所有参数均符合接口设计要求
//@EXPECTATION:返回CSUDI_SUCCESS
//@REMARK:CSUDIPLAYERAddFilePlayerCallback & CSUDIPLAYERDelFilePlayerCallback参数要求请参考UDI移植指南
//@EXECUTIONFLOW:1、打开一个本地文件播放器，得到播放器句柄
//@EXECUTIONFLOW:2、调用CSUDIPLAYERAddFilePlayerCallback添加回调函数的三个不同事例(pnUserData为CSUDI_NULL,0x1234以及合任意合法地址值)，期望返回CSUDI_SUCCESS
//@EXECUTIONFLOW:3、调用CSUDIPLAYERDelFilePlayerCallback删除步骤2中添加的三个回调函数事例，期望返回CSUDI_SUCCESS
//@EXECUTIONFLOW:4、重复步骤2-步骤3指定次数
//@EXECUTIONFLOW:5、调用CSUDIPLAYERClose删除播放器
CSUDI_BOOL CSTC_FILE_TEST_IT_Add_DelPlayerCallback_0002(void)
{
	CSUDI_HANDLE   hPlayer = CSUDI_NULL;
	int  nUserData=NUSERDATA;
	int i=0;
	//创建播放器参数
	CSUDIPLAYERType_E ePlayerType=EM_UDIPLAYER_FILE;

	//创建播放器
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYEROpen (NULL,ePlayerType,&hPlayer), "步骤1失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_NULL != hPlayer, "步骤1 失败");

	for (i=0;i < REPEAT_TIMES; i++)
	{
		//testing CSUDIPLAYERAddPlayerCallback
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERAddFilePlayerCallback(hPlayer,FilePlayEventCallback,&nUserData), "步骤2-1 失败");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERAddFilePlayerCallback(hPlayer,FilePlayEventCallback,CSUDI_NULL), "步骤2-2 失败");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERAddFilePlayerCallback(hPlayer,FilePlayEventCallback,(void *)0x1234), "步骤2-3 失败");

		//testing CSUDIPLAYERDelPlayerCallback
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS ==  CSUDIPLAYERDelFilePlayerCallback(hPlayer,FilePlayEventCallback, &nUserData), "步骤3-1 失败");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS ==  CSUDIPLAYERDelFilePlayerCallback(hPlayer,FilePlayEventCallback, CSUDI_NULL), "步骤3-2 失败");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS ==  CSUDIPLAYERDelFilePlayerCallback(hPlayer,FilePlayEventCallback, (void *)0x1234), "步骤3-3 失败");
	}

	CSTK_FATAL_POINT;

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERClose (hPlayer), "步骤5 失败");

	return CSUDI_TRUE;
}

//@CASEGROUP:CSUDIPLAYERAddFilePlayerCallback & CSUDIPLAYERDelFilePlayerCallback
//@DESCRIPTION:测试利用CSUDIPLAYERAddFilePlayerCallback 最少能成功添加32个回调函数
//@PRECONDITION:PLAYER模块被正常初始化
//@INPUT:1、所有参数均符合接口设计要求
//@EXPECTATION:返回CSUDI_SUCCESS
//@REMARK:CSUDIPLAYERAddFilePlayerCallback & CSUDIPLAYERDelFilePlayerCallback参数要求请参考UDI移植指南
//@EXECUTIONFLOW:1、打开一个本地文件播放器，得到播放器句柄
//@EXECUTIONFLOW:2、调用CSUDIPLAYERAddFilePlayerCallback添加32个回调函数事例，期望均返回CSUDI_SUCCESS
//@EXECUTIONFLOW:3、调用CSUDIPLAYERDelFilePlayerCallback删除步骤2中添加的32个回调函数事例，期望均返回CSUDI_SUCCESS
//@EXECUTIONFLOW:4、重复步骤2-步骤3指定次数
//@EXECUTIONFLOW:5、调用CSUDIPLAYERClose删除播放器
CSUDI_BOOL CSTC_FILE_TEST_IT_Add_DelPlayerCallback_0003(void)
{
	CSUDI_HANDLE   hPlayer = CSUDI_NULL;
	int i=0, j=0;
	int  anUserData[33] = {0};
	//创建播放器参数
	CSUDIPLAYERType_E ePlayerType=EM_UDIPLAYER_FILE;

	//创建播放器
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYEROpen (NULL,ePlayerType,&hPlayer), "步骤1失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_NULL  != hPlayer, "步骤1 失败");

	for (i=0;i<REPEAT_TIMES;i++)
	{
		//添加32个回调
		for (j=0; j<32; j++)
		{
			anUserData[j] = j;
			CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERAddFilePlayerCallback(hPlayer,FilePlayEventCallback,&anUserData[j]), "步骤2失败");
		}

		//删除32个回调
		for (j=0; j<32; j++)
		{
			anUserData[j] = j;
			CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERDelFilePlayerCallback (hPlayer,FilePlayEventCallback,&anUserData[j]), "步骤3失败");
		}
	}

	CSTK_FATAL_POINT;

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERClose (hPlayer), "步骤4 失败");

	return CSUDI_TRUE;
}

//@CASEGROUP:CSUDIPLAYEROpen&CSUDIPLAYERClose
//@DESCRIPTION:测试参数或应用场景在不符合接口设计要求，CSUDIPLAYEROpen&CSUDIPLAYERClose函数的执行效果
//@PRECONDITION:PLAYER模块被正常初始化
//@INPUT:1、参数组合不符合设计要求
//@EXPECTATION:返回非CSUDI_SUCCESS
//@REMARK:
//@EXECUTIONFLOW:1、sPlayerChnl不为NULL，调用CSUDIPLAYEROpen，期望返回CSUDIPLAYER_ERROR_INVALID_DEVICE_ID
//@EXECUTIONFLOW:2、ePlayerType 为无效值(如:EM_UDIPLAYER_MAXPLAYERTYPE)，调用CSUDIPLAYEROpen，期望返回非CSUDI_SUCCESS
//@EXECUTIONFLOW:3、phPlayer 为CSUDI_NULL，调用CSUDIPLAYEROpen，期望返回非CSUDI_SUCCESS
//@EXECUTIONFLOW:4、hPlayer 为CSUDI_NULL，调用CSUDIPLAYERClose，期望返回非CSUDI_SUCCESS
//@EXECUTIONFLOW:5、重复步骤1-步骤4指定次数
CSUDI_BOOL CSTC_FILE_IT_CSUDIPLAYEROpenClose_0001(void)
{
	CSUDIPLAYERType_E  ePlayerType=EM_UDIPLAYER_FILE;
	CSUDI_HANDLE   hPlayer = CSUDI_NULL;
	CSUDIPlayerChnl_S  sPlayerChnl;
	int i = 0;

	for (i=0;i<REPEAT_TIMES;i++)
	{
		sPlayerChnl.m_nAudioDecoder = 0;
		sPlayerChnl.m_nVideoDecoder = 0;
		sPlayerChnl.m_nDemux = 0;
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS != CSUDIPLAYEROpen (&sPlayerChnl,ePlayerType,&hPlayer), "步骤1失败");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS != CSUDIPLAYEROpen (NULL,EM_UDIPLAYER_MAXPLAYERTYPE,&hPlayer), "步骤2失败");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS != CSUDIPLAYEROpen (NULL,ePlayerType,CSUDI_NULL), "步骤3失败");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS != CSUDIPLAYERClose (CSUDI_NULL), "步骤4失败");
	}

	CSTK_FATAL_POINT;

	return CSUDI_TRUE;
}

//@CASEGROUP:CSUDIPLAYEROpen&CSUDIPLAYERClose
//@DESCRIPTION:测试正常情况下，CSUDIPLAYEROpen&CSUDIPLAYERClose的执行情况
//@PRECONDITION:PLAYER模块被正常初始化
//@INPUT:1、能按文件播放方式正常打开播放器
//@EXPECTATION:返回CSUDI_SUCCESS
//@REMARK:
//@EXECUTIONFLOW:1、调用CSUDIPLAYEROpen，以文件播放方式打开播放器，期望返回CSUDI_SUCCESS
//@EXECUTIONFLOW:2、调用CSUDIPLAYERClose，关闭步骤2打开的播放器，期望返回CSUDI_SUCCESS
//@EXECUTIONFLOW:3、重复步骤1-步骤2指定次数
CSUDI_BOOL CSTC_FILE_IT_CSUDIPLAYEROpenClose_0002(void)
{
	CSUDIPLAYERType_E  ePlayerType=EM_UDIPLAYER_FILE;
	CSUDI_HANDLE   hPlayer = CSUDI_NULL;
	int i = 0;

	for (i=0;i<REPEAT_TIMES;i++)
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYEROpen (NULL,ePlayerType,&hPlayer), "步骤1失败");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERClose (hPlayer), "步骤2失败");
	}

	CSTK_FATAL_POINT;

	return CSUDI_TRUE;
}

//@CASEGROUP:CSUDIPLAYEROpen&CSUDIPLAYERClose
//@DESCRIPTION:测试正常参数情况下，连续多次调用CSUDIPLAYEROpen&CSUDIPLAYERClose的情况
//@PRECONDITION:PLAYER模块被正常初始化
//@INPUT:1、能按文件播放方式正常打开播放器
//@EXPECTATION:返回CSUDI_SUCCESS
//@REMARK:
//@EXECUTIONFLOW:1、调用CSUDIPLAYEROpen，以文件播放方式打开播放器，期望返回CSUDI_SUCCESS
//@EXECUTIONFLOW:2、参数不变，再次调用CSUDIPLAYEROpen，以文件播放方式打开播放器，期望返回非CSUDI_SUCCESS
//@EXECUTIONFLOW:3、调用CSUDIPLAYERClose，关闭步骤1打开的播放器，期望返回CSUDI_SUCCESS
//@EXECUTIONFLOW:4、参数不变，再次调用CSUDIPLAYERClose，关闭步骤1打开的播放器，期望返回非CSUDI_SUCCESS
//@EXECUTIONFLOW:5、重复步骤1-步骤4指定次数
CSUDI_BOOL CSTC_FILE_IT_CSUDIPLAYEROpenClose_0003(void)
{
	CSUDIPLAYERType_E  ePlayerType=EM_UDIPLAYER_FILE;
	CSUDI_HANDLE   hPlayer = CSUDI_NULL;
	CSUDI_HANDLE   hHiPlayer = CSUDI_NULL;
	int i = 0;

	for (i=0;i<REPEAT_TIMES;i++)
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYEROpen (NULL,ePlayerType,&hPlayer), "步骤1失败");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS != CSUDIPLAYEROpen (NULL,ePlayerType,&hHiPlayer), "步骤2失败");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERClose (hPlayer), "步骤3失败");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS != CSUDIPLAYERClose (hPlayer), "步骤4失败");
	}

	CSTK_FATAL_POINT;

	return CSUDI_TRUE;
}

//@CASEGROUP:CSUDIPLAYERSetStream&CSUDIPLAYERGetPlaybackParam
//@DESCRIPTION:测试参数或应用场景在不符合接口设计要求的情况下，调用CSUDIPLAYERSetStream&CSUDIPLAYERSetStream&CSUDIPLAYERGetPlaybackParam函数的情况
//@PRECONDITION:PLAYER模块被正常初始化
//@INPUT:1、参数组合不符合设计要求
//@EXPECTATION:返回非CSUDI_SUCCESS
//@REMARK:
//@EXECUTIONFLOW:1、打开一个本地文件播放器
//@EXECUTIONFLOW:2、测试hPlayer为CSUDI_NULL的情况下，调用CSUDIPLAYERSetStream的情况，期望返回非CSUDI_SUCCESS
//@EXECUTIONFLOW:3、测试psStreamInfo不为CSUDI_NULL的情况下，调用CSUDIPLAYERSetStream的情况，期望返回非CSUDI_SUCCESS
//@EXECUTIONFLOW:4、测试psStreamInfo不为CSUDI_NULL并且nStreamCnt不为0的情况下，调用CSUDIPLAYERSetStream的情况，期望返回非CSUDI_SUCCESS
//@EXECUTIONFLOW:5、测试nStreamCnt不为0的情况下，调用CSUDIPLAYERSetStream的情况，期望返回非CSUDI_SUCCESS
//@EXECUTIONFLOW:6、测试stPlaybackParam为NULL的情况下，调用CSUDIPLAYERSetStream的情况，期望返回非CSUDI_SUCCESS
//@EXECUTIONFLOW:7、测试hPlayer为CSUDI_NULL的情况下，调用CSUDIPLAYERGetPlaybackParam的情况，期望返回非CSUDI_SUCCESS
//@EXECUTIONFLOW:8、测试stPlaybackParam为NULL的情况下，调用CSUDIPLAYERGetPlaybackParam的情况，期望返回非CSUDI_SUCCESS
//@EXECUTIONFLOW:9、重复步骤2-步骤8 指定次数
CSUDI_BOOL CSTC_FILE_TEST_IT_SetStream_GetPlaybackParam_0001(void)
{
	int nStreamCnt = 1;
	CSUDIStreamInfo_S asStreamInfo[10];
	CSUDIPlaybackParam_S stPlaybackParam;
	int  i= 0;
	CSUDI_HANDLE   hPlayer = CSUDI_NULL;
	CSUDIPLAYERType_E	ePlayerType=EM_UDIPLAYER_FILE;

	memset(asStreamInfo,0,sizeof(asStreamInfo));
	memset(&stPlaybackParam,0,sizeof(stPlaybackParam));

	stPlaybackParam.m_eSpeed = EM_UDIPLAYER_SPEED_NORMAL;
	stPlaybackParam.m_nSecondPos = 0;

	strncpy(stPlaybackParam.m_szFileName, stFilePath[EM_UDI_FILENAME_MP4_SD].filename, CSUDI_MAX_FILE_NAME_LEN);

	//创建播放器
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYEROpen (NULL,ePlayerType,&hPlayer), "步骤1失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_NULL  != hPlayer, "步骤1失败");

	for (i=0;i<REPEAT_TIMES;i++)
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDIPLAYER_ERROR_INVALID_HANDLE == CSUDIPLAYERSetStream (CSUDI_NULL,(void *)0, 0,&stPlaybackParam), "步骤2失败");

		CSTK_ASSERT_TRUE_FATAL(CSUDIPLAYER_ERROR_BAD_PARAMETER == CSUDIPLAYERSetStream (hPlayer, asStreamInfo,0,&stPlaybackParam), "步骤3失败");

		CSTK_ASSERT_TRUE_FATAL(CSUDIPLAYER_ERROR_BAD_PARAMETER == CSUDIPLAYERSetStream (hPlayer,asStreamInfo,nStreamCnt,&stPlaybackParam), "步骤4失败");

		CSTK_ASSERT_TRUE_FATAL(CSUDIPLAYER_ERROR_BAD_PARAMETER == CSUDIPLAYERSetStream (hPlayer,(void *)0,nStreamCnt,&stPlaybackParam), "步骤5失败");

		CSTK_ASSERT_TRUE_FATAL(CSUDIPLAYER_ERROR_BAD_PARAMETER == CSUDIPLAYERSetStream (hPlayer,(void *)0,nStreamCnt,CSUDI_NULL), "步骤6失败");

		memset(&stPlaybackParam,0,sizeof(stPlaybackParam));

		CSTK_ASSERT_TRUE_FATAL(CSUDIPLAYER_ERROR_BAD_PARAMETER == CSUDIPLAYERGetPlaybackParam (CSUDI_NULL,&stPlaybackParam), "步骤7失败");

		memset(&stPlaybackParam,0,sizeof(stPlaybackParam));

		CSTK_ASSERT_TRUE_FATAL(CSUDIPLAYER_ERROR_BAD_PARAMETER == CSUDIPLAYERGetPlaybackParam (hPlayer,(void *)0), "步骤8失败");

	}

	CSTK_FATAL_POINT;

	if (hPlayer != CSUDI_NULL)
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERClose (hPlayer), "恢复现场失败");
	}
	return CSUDI_TRUE;
}

//@CASEGROUP:CSUDIPLAYERSetStream&CSUDIPLAYERGetPlaybackParam
//@DESCRIPTION:测试参数或应用场景在符合接口设计要求，CSUDIPLAYERSetStream&CSUDIPLAYERGetPlaybackParam函数的执行效果
//@PRECONDITION:PLAYER模块被正常初始化
//@INPUT:1、参数组合符合设计要求
//@EXPECTATION:返回CSUDI_SUCCESS
//@REMARK:
//@EXECUTIONFLOW:1、打开一个本地文件播放器
//@EXECUTIONFLOW:2、测试在本地文件播放状态下，CSUDIPLAYERSetStream函数的执行情况
//@EXECUTIONFLOW:3、测试在本地文件播放状态下，CSUDIPLAYERGetPlaybackParam函数的执行情况
//@EXECUTIONFLOW:4、重复步骤2-步骤3  指定次数
CSUDI_BOOL CSTC_FILE_TEST_IT_SetStream_GetPlaybackParam_0002(void)
{
	CSUDI_HANDLE   hPlayer = CSUDI_NULL;
	int  i= 0;
	CSUDIPlaybackParam_S stPlaybackParam;
	CSUDIPLAYERType_E	ePlayerType=EM_UDIPLAYER_FILE;

	memset(&stPlaybackParam,0,sizeof(stPlaybackParam));
	stPlaybackParam.m_eSpeed = EM_UDIPLAYER_SPEED_NORMAL;
	stPlaybackParam.m_nSecondPos = 0;

	strncpy(stPlaybackParam.m_szFileName, stFilePath[EM_UDI_FILENAME_MP4_SD].filename, CSUDI_MAX_FILE_NAME_LEN);

	//创建播放器
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYEROpen (NULL,ePlayerType,&hPlayer), "步骤1失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_NULL  != hPlayer, "步骤1失败");

	for (i=0;i<REPEAT_TIMES;i++)
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERSetStream (hPlayer,(void *)0, 0,&stPlaybackParam), "步骤2失败");

		memset(&stPlaybackParam,0,sizeof(stPlaybackParam));

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERGetPlaybackParam (hPlayer,&stPlaybackParam), "步骤3失败");
	}

	CSTK_FATAL_POINT;

	if (hPlayer != CSUDI_NULL)
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERClose (hPlayer), "恢复现场失败");
	}
	return CSUDI_TRUE;
}

//@CASEGROUP:CSUDIPLAYERStart   & CSUDIPLAYERStop
//@DESCRIPTION:测试播放器指针非法的情况
//@PRECONDITION:PLAYER模块被正常初始化
//@INPUT:1、hPlayer=CSUDI_NULL
//@EXPECTATION:CSUDIPLAYER_ERROR_INVALID_HANDLE
//@REMARK:
//@EXECUTIONFLOW:1、调用CSUDIPLAYERStart，使hPlayer=CSUDI_NULL，期望返回CSUDIPLAYER_ERROR_INVALID_HANDLE
//@EXECUTIONFLOW:2、调用CSUDIPLAYERStop，使hPlayer=CSUDI_NULL，期望返回CSUDIPLAYER_ERROR_INVALID_HANDLE
CSUDI_BOOL CSTC_FILE_TEST_IT_StartStop_0001(void)
{
	CSTK_ASSERT_TRUE_FATAL(CSUDIPLAYER_ERROR_INVALID_HANDLE  == CSUDIPLAYERStart(CSUDI_NULL), "步骤1失败");
	CSTK_ASSERT_TRUE_FATAL(CSUDIPLAYER_ERROR_INVALID_HANDLE  == CSUDIPLAYERStop(CSUDI_NULL), "步骤2失败");

	CSTK_FATAL_POINT;

	return CSUDI_TRUE;
}

//@CASEGROUP:CSUDIPLAYERStart   & CSUDIPLAYERStop
//@DESCRIPTION:测试正常启动/停止一个播放器的情况
//@PRECONDITION:PLAYER模块被正常初始化
//@INPUT:1、hPlayer有效
//@EXPECTATION:CSUDI_SUCCESS
//@REMARK:
//@EXECUTIONFLOW:1、打开一个本地文件播放器，得到播放器句柄
//@EXECUTIONFLOW:2、调用CSUDIPLAYERSetStream,设置文件属性信息，期望返回CSUDI_SUCCESS
//@EXECUTIONFLOW:3、调用CSUDIPLAYERStart，启动播放器，期望返回CSUDI_SUCCESS且正常播放
//@EXECUTIONFLOW:4、调用CSUDIPLAYERStop，停止播放器，期望返回CSUDI_SUCCESS且正常停止
//@EXECUTIONFLOW:5、重复步骤3-步骤4指定次数
//@EXECUTIONFLOW:6、恢复现场
CSUDI_BOOL CSTC_FILE_TEST_IT_StartStop_0002(void)
{
	int i = 0;
	CSUDI_HANDLE   hPlayer = CSUDI_NULL;
	CSUDIPlaybackParam_S stPlaybackParam;
	CSUDIPLAYERType_E	ePlayerType=EM_UDIPLAYER_FILE;

	memset(&stPlaybackParam,0,sizeof(stPlaybackParam));
	stPlaybackParam.m_eSpeed = EM_UDIPLAYER_SPEED_NORMAL;
	stPlaybackParam.m_nSecondPos = 0;

	strncpy(stPlaybackParam.m_szFileName, stFilePath[EM_UDI_FILENAME_MP4_SD].filename, CSUDI_MAX_FILE_NAME_LEN);

	CSTCPrint("本用例测试文件播放的正常启动和停止，测试文件为: \"%s\"\n", stPlaybackParam.m_szFileName);

	//创建播放器
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYEROpen (NULL,ePlayerType,&hPlayer), "步骤1失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_NULL  != hPlayer, "步骤1失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERSetStream (hPlayer,NULL,0,&stPlaybackParam), "步骤2失败");
	
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERAddFilePlayerCallback(hPlayer,FilePlayCallback,NULL), "注册回调失败");

	for (i=0;i<REPEAT_TIMES;i++)
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERStart(hPlayer), "步骤3失败");

		CSUDIOSThreadSleep(SLEEP_TIME);

		CSTCPrint("音视频是否能够从头开始正常播放？\n");
    	CSTCPrint("Is A/V output normal ?\n");
		CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes(),"步骤3失败：播放不正常");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERStop(hPlayer), "步骤4失败");

		CSTCPrint("音视频是否停止播放了？\n");
  		CSTCPrint("Does the player stop ?\n");
		CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes(),"步骤4失败：停止播放不正常");
	}

	CSTK_FATAL_POINT;

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERDelFilePlayerCallback(hPlayer,FilePlayCallback,NULL), "删除回调失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERClose (hPlayer), "恢复现场");

	return CSUDI_TRUE;
}

//@CASEGROUP:CSUDIPLAYERStart
//@DESCRIPTION:在应用场景不符合设计要求的情况下，调用CSUDIPLAYERStart/ CSUDIPLAYERStop,期望返回非CSUDI_SUCCESS
//@PRECONDITION:PLAYER模块被正常初始化
//@INPUT:1、hPlayer有效
//@EXPECTATION:CSUDIPLAYER_ERROR_INVALID_STATUS
//@REMARK:CSUDIPLAYERStart只能在设置文件信息之后与停止播放器之后调用，CSUDIPLAYERStop只能在启动播放器之后，停止播放器之前才能调用
//@EXECUTIONFLOW:1、打开一个本地文件播放器，得到播放器句柄
//@EXECUTIONFLOW:2、CSUDIPLAYEROpen之后立即调用CSUDIPLAYERStop，停止播放器，期望返回非CSUDI_SUCCESS
//@EXECUTIONFLOW:3、CSUDIPLAYEROpen之后立即调用CSUDIPLAYERStart，启动播放器，期望返回非CSUDI_SUCCESS
//@EXECUTIONFLOW:4、CSUDIPLAYEROpen之后调用CSUDIPLAYERSetStream,设置流属性信息，期望返回CSUDI_SUCCESS
//@EXECUTIONFLOW:5、CSUDIPLAYERSetStream之后、CSUDIPLAYERStart之前，调用CSUDIPLAYERStop，停止播放器，期望返回非CSUDI_SUCCESS
//@EXECUTIONFLOW:6、调用CSUDIPLAYERStart，启动播放器，期望返回CSUDI_SUCCESS
//@EXECUTIONFLOW:7、再次调用CSUDIPLAYERStart，启动播放器，期望返回非CSUDI_SUCCESS
//@EXECUTIONFLOW:8、调用CSUDIPLAYERStop，停止播放器，期望返回CSUDI_SUCCESS
//@EXECUTIONFLOW:9、再次调用CSUDIPLAYERStop，停止播放器，期望返回非CSUDI_SUCCESS
//@EXECUTIONFLOW:10、重复步骤5-步骤9指定次数
//@EXECUTIONFLOW:11、恢复现场
CSUDI_BOOL CSTC_FILE_TEST_IT_StartStop_0003(void)
{
	CSUDI_HANDLE   hPlayer = CSUDI_NULL;
	int i=0;
	CSUDIPlaybackParam_S stPlaybackParam;
	CSUDIPLAYERType_E	ePlayerType=EM_UDIPLAYER_FILE;

	memset(&stPlaybackParam,0,sizeof(stPlaybackParam));
	stPlaybackParam.m_eSpeed = EM_UDIPLAYER_SPEED_NORMAL;
	stPlaybackParam.m_nSecondPos = 0;

	strncpy(stPlaybackParam.m_szFileName, stFilePath[EM_UDI_FILENAME_MP4_SD].filename, CSUDI_MAX_FILE_NAME_LEN);

	//创建播放器
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYEROpen (NULL,ePlayerType,&hPlayer), "步骤1失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_NULL  != hPlayer, "步骤1失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS != CSUDIPLAYERStop(hPlayer), "步骤2失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS != CSUDIPLAYERStart(hPlayer), "步骤3失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERSetStream (hPlayer,NULL,0,&stPlaybackParam), "步骤4失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERAddFilePlayerCallback(hPlayer,FilePlayCallback,NULL), "注册回调失败");

	for (i=0;i<REPEAT_TIMES;i++)
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS != CSUDIPLAYERStop(hPlayer), "步骤5失败");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERStart(hPlayer), "步骤6失败");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS != CSUDIPLAYERStart(hPlayer), "步骤7失败");

		CSUDIOSThreadSleep(SLEEP_TIME);

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERStop(hPlayer), "步骤8失败");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS != CSUDIPLAYERStop(hPlayer), "步骤9失败");
	}

	CSTK_FATAL_POINT;

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERDelFilePlayerCallback(hPlayer,FilePlayCallback,NULL), "删除回调失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERClose (hPlayer), "恢复现场");

	return CSUDI_TRUE;
}

//@CASEGROUP:CSUDIPLAYERPause & CSUDIPLAYERResume
//@DESCRIPTION:测试播放器句柄为CSUDI_NULL时，CSUDIPLAYERPause & CSUDIPLAYERResume的调用情况
//@PRECONDITION:PLAYER模块被正常初始化
//@INPUT:1、hPlayer=CSUDI_NULL
//@EXPECTATION:CSUDIPLAYER_ERROR_INVALID_HANDLE
//@REMARK:
//@EXECUTIONFLOW:1、调用CSUDIPLAYERPause，使hPlayer=CSUDI_NULL，期望返回CSUDIPLAYER_ERROR_INVALID_HANDLE
//@EXECUTIONFLOW:2、调用CSUDIPLAYERResume，使hPlayer=CSUDI_NULL，期望返回CSUDIPLAYER_ERROR_INVALID_HANDLE
CSUDI_BOOL CSTC_FILE_TEST_IT_PauseResume_0001(void)
{
	CSTK_ASSERT_TRUE_FATAL(CSUDIPLAYER_ERROR_INVALID_HANDLE  == CSUDIPLAYERPause(CSUDI_NULL), "步骤1失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDIPLAYER_ERROR_INVALID_HANDLE  == CSUDIPLAYERResume(CSUDI_NULL), "步骤2失败");

	CSTK_FATAL_POINT;

	return CSUDI_TRUE;
}

//@CASEGROUP:CSUDIPLAYERPause & CSUDIPLAYERResume
//@DESCRIPTION:测试应用场景不符合设计要求时，CSUDIPLAYERPause & CSUDIPLAYERResume函数的执行情况
//@PRECONDITION:PLAYER模块被正常初始化
//@INPUT:
//@EXPECTATION:直播播放器可以不支持CSUDIPLAYERPause & CSUDIPLAYERResume
//@REMARK:
//@EXECUTIONFLOW:1、打开一个本地文件播放器，得到播放器句柄
//@EXECUTIONFLOW:2、Open播放器后立即调用CSUDIPLAYERPause，期望返回非CSUDI_SUCCESS
//@EXECUTIONFLOW:3、Open播放器后立即调用CSUDIPLAYERResume，期望返回非CSUDI_SUCCESS
//@EXECUTIONFLOW:4、Open播放器后调用CSUDIPLAYERSetStream设置流属性信息，期望返回CSUDI_SUCCESS
//@EXECUTIONFLOW:5、Start播放之前，调用CSUDIPLAYERPause，期望返回非CSUDI_SUCCESS
//@EXECUTIONFLOW:6、Start播放之前，调用CSUDIPLAYERResume，期望返回非CSUDI_SUCCESS
//@EXECUTIONFLOW:7、调用CSUDIPLAYERStart，Start播放器，期望返回CSUDI_SUCCESS
//@EXECUTIONFLOW:8、Start之后、Pause播放之前，调用CSUDIPLAYERResume，期望返回非CSUDI_SUCCESS
//@EXECUTIONFLOW:9、调用CSUDIPLAYERPause暂停播放器，期望返回CSUDI_SUCCESS 或CSUDIPLAYER_ERROR_FEATURE_NOT_SUPPORTED
//@EXECUTIONFLOW:10、如果CSUDIPLAYERPause成功，则调用CSUDIPLAYERStart，期望返回非CSUDI_SUCCESS
//@EXECUTIONFLOW:11、如果CSUDIPLAYERPause成功，则调用CSUDIPLAYERSetStream，期望返回非CSUDI_SUCCESS
//@EXECUTIONFLOW:12、如果CSUDIPLAYERPause成功，则再次调用CSUDIPLAYERPause，期望返回非CSUDI_SUCCESS
//@EXECUTIONFLOW:13、如果CSUDIPLAYERPause成功，则调用CSUDIPLAYERResume，期望返回CSUDI_SUCCESS
//@EXECUTIONFLOW:14、如果CSUDIPLAYERPause成功，则再次调用CSUDIPLAYERResume，期望返回非CSUDI_SUCCESS
//@EXECUTIONFLOW:15、如果CSUDIPLAYERPause成功，则调用CSUDIPLAYERPause，期望返回CSUDI_SUCCESS
//@EXECUTIONFLOW:16、调用CSUDIPLAYERStop，停止播放器，期望返回CSUDI_SUCCESS
//@EXECUTIONFLOW:17、重复步骤5-步骤16指定次数
//@EXECUTIONFLOW:18、恢复现场
CSUDI_BOOL CSTC_FILE_TEST_IT_PauseResume_0002(void)
{
	int i = 0;
	CSUDI_HANDLE   hPlayer = CSUDI_NULL;
	CSUDI_Error_Code udiRe = CSUDI_SUCCESS;
	CSUDIPlaybackParam_S stPlaybackParam;
	CSUDIPLAYERType_E	ePlayerType=EM_UDIPLAYER_FILE;

	memset(&stPlaybackParam,0,sizeof(stPlaybackParam));
	stPlaybackParam.m_eSpeed = EM_UDIPLAYER_SPEED_NORMAL;
	stPlaybackParam.m_nSecondPos = 0;

	strncpy(stPlaybackParam.m_szFileName, stFilePath[EM_UDI_FILENAME_MP4_SD].filename, CSUDI_MAX_FILE_NAME_LEN);

	//创建播放器
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYEROpen (NULL,ePlayerType,&hPlayer), "步骤1失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_NULL  != hPlayer, "步骤1失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  != CSUDIPLAYERPause (hPlayer), "步骤2失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  != CSUDIPLAYERResume(hPlayer), "步骤3失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERSetStream (hPlayer,NULL,0,&stPlaybackParam), "步骤4失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERAddFilePlayerCallback(hPlayer,FilePlayCallback,NULL), "注册回调失败");

	for (i=0;i<REPEAT_TIMES;i++)
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  != CSUDIPLAYERPause (hPlayer), "步骤5失败");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  != CSUDIPLAYERResume(hPlayer), "步骤6失败");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERStart(hPlayer), "步骤7失败");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  != CSUDIPLAYERResume(hPlayer), "步骤8失败");

		CSUDIOSThreadSleep(SLEEP_TIME);

		udiRe = CSUDIPLAYERPause(hPlayer);
		CSTK_ASSERT_TRUE_FATAL(((CSUDI_SUCCESS == udiRe)||(CSUDIPLAYER_ERROR_FEATURE_NOT_SUPPORTED==udiRe)),"步骤9失败");

		if (CSUDI_SUCCESS == udiRe)
		{
			CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  != CSUDIPLAYERStart(hPlayer), "步骤10失败");

			CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  != CSUDIPLAYERSetStream (hPlayer,NULL,0,&stPlaybackParam), "步骤11失败");

			CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  != CSUDIPLAYERPause(hPlayer), "步骤12失败");

			CSUDIOSThreadSleep(SLEEP_TIME);

			CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERResume(hPlayer), "步骤13失败");

			CSUDIOSThreadSleep(SLEEP_TIME);

			CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  != CSUDIPLAYERResume(hPlayer), "步骤14失败");

			CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERPause(hPlayer), "步骤15失败");

			CSUDIOSThreadSleep(SLEEP_TIME);
		}

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERStop(hPlayer), "步骤16失败");
	}

	CSTK_FATAL_POINT;

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERDelFilePlayerCallback(hPlayer,FilePlayCallback,NULL), "删除回调失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERClose (hPlayer), "恢复现场失败");

	return CSUDI_TRUE;
}

//@CASEGROUP:CSUDIPLAYERPause & CSUDIPLAYERResume
//@DESCRIPTION:测试正常暂停/恢复一个播放器的情况
//@PRECONDITION:PLAYER模块被正常初始化
//@INPUT:1、hPlayer有效
//@EXPECTATION:CSUDI_SUCCESS
//@REMARK:
//@EXECUTIONFLOW:1、打开一个本地文件播放器，得到播放器句柄
//@EXECUTIONFLOW:2、调用CSUDIPLAYERSetStream,设置文件属性信息，期望返回CSUDI_SUCCESS
//@EXECUTIONFLOW:3、调用CSUDIPLAYERStart，启动播放器，期望返回CSUDI_SUCCESS且正常播放
//@EXECUTIONFLOW:4、调用CSUDIPLAYERPause，暂停播放器，期望返回CSUDI_SUCCESS且正常暂停
//@EXECUTIONFLOW:5、调用CSUDIPLAYERResume，恢复播放，期望返回CSUDI_SUCCESS且正常恢复播放
//@EXECUTIONFLOW:6、重复步骤4-步骤5指定次数
//@EXECUTIONFLOW:7、调用CSUDIPLAYERStop，停止播放器，期望返回CSUDI_SUCCESS且正常停止
//@EXECUTIONFLOW:8、恢复现场
CSUDI_BOOL CSTC_FILE_TEST_IT_PauseResume_0003(void)
{
	int i = 0;
	CSUDI_HANDLE   hPlayer = CSUDI_NULL;
	CSUDIPlaybackParam_S stPlaybackParam;
	CSUDIPLAYERType_E	ePlayerType=EM_UDIPLAYER_FILE;

	memset(&stPlaybackParam,0,sizeof(stPlaybackParam));
	stPlaybackParam.m_eSpeed = EM_UDIPLAYER_SPEED_NORMAL;
	stPlaybackParam.m_nSecondPos = 0;

	strncpy(stPlaybackParam.m_szFileName, stFilePath[EM_UDI_FILENAME_MP4_SD].filename, CSUDI_MAX_FILE_NAME_LEN);

	CSTCPrint("本用例测试文件播放的正常暂停和恢复，测试文件为: \"%s\"\n", stPlaybackParam.m_szFileName);

	//创建播放器
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYEROpen (NULL,ePlayerType,&hPlayer), "步骤1失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_NULL != hPlayer, "步骤1失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERSetStream (hPlayer,NULL,0,&stPlaybackParam), "步骤2失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERStart(hPlayer), "步骤3失败");

	CSUDIOSThreadSleep(SLEEP_TIME);

	CSTCPrint("音视频是否能够正常播放？\n");
	CSTCPrint("Is A/V output normal ?\n");
	CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes(),"步骤3失败：播放不正常");

	for (i=0;i<REPEAT_TIMES;i++)
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERPause(hPlayer), "步骤4失败");

		CSTCPrint("音视频是否暂停播放了？\n");
  		CSTCPrint("Does the player pause ?\n");
		CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes(),"步骤4失败：暂停播放不正常");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERResume(hPlayer), "步骤5失败");

		CSTCPrint("音视频是否从暂停点恢复播放了？\n");
  		CSTCPrint("Does the player resume ?\n");
		CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes(),"步骤5失败：恢复播放不正常");
	}

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERStop(hPlayer), "步骤7失败");

	CSTCPrint("音视频是否停止播放了？\n");
	CSTCPrint("Does the player stop ?\n");
	CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes(),"步骤7失败：停止播放不正常");

	CSTK_FATAL_POINT;

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERClose (hPlayer), "恢复现场");

	return CSUDI_TRUE;
}

//@CASEGROUP:CSUDIPLAYERSetSpeed&CSUDIPLAYERGetSpeed
//@DESCRIPTION:1、测试本地文件播放情况下，CSUDIPLAYERSetSpeed函数的执行情况
//@DESCRIPTION:2、测试本地文件播放情况下，CSUDIPLAYERGetSpeed函数的执行情况
//@PRECONDITION:PLAYER模块被正常初始化
//@INPUT:
//@EXPECTATION:
//@REMARK:
//@EXECUTIONFLOW:1、打开一个本地文件播放器，得到播放器句柄
//@EXECUTIONFLOW:2、调用CSUDIPLAYERSetStream，期望返回CSUDI_SUCCESS
//@EXECUTIONFLOW:3、hPlayer为CSUDI_NULL，调用 CSUDIPLAYERSetSpeed，期望返回非CSUDI_SUCCESS
//@EXECUTIONFLOW:4、eSpeed为无效值，调用 CSUDIPLAYERSetSpeed，期望返回非CSUDI_SUCCESS
//@EXECUTIONFLOW:5、hPlayer为CSUDI_NULL，调用 CSUDIPLAYERGetSpeed，期望返回非CSUDI_SUCCESS
//@EXECUTIONFLOW:6、eSpeed为无效值，调用 CSUDIPLAYERGetSpeed，期望返回非CSUDI_SUCCESS
//@EXECUTIONFLOW:7、参数正确的情况下，Start播放器之前调用CSUDIPLAYERSetSpeed，期望返回的不为CSUDI_SUCCESS或为CSUDIPLAYER_ERROR_FEATURE_NOT_SUPPORTED
//@EXECUTIONFLOW:8、参数正确的情况下，Start播放器之前调用CSUDIPLAYERGetSpeed，期望返回的不为CSUDI_SUCCESS或为CSUDIPLAYER_ERROR_FEATURE_NOT_SUPPORTED
//@EXECUTIONFLOW:9、调用CSUDIPLAYERStart，Start播放器，期望返回CSUDI_SUCCESS
//@EXECUTIONFLOW:10、参数正确的情况下，Start之后调用CSUDIPLAYERSetSpeed，期望返回CSUDI_SUCCESS或CSUDIPLAYER_ERROR_FEATURE_NOT_SUPPORTED
//@EXECUTIONFLOW:11、参数正确的情况下，Start之后调用CSUDIPLAYERGetSpeed，期望返回CSUDI_SUCCESS或CSUDIPLAYER_ERROR_FEATURE_NOT_SUPPORTED
//@EXECUTIONFLOW:12、调用CSUDIPLAYERStop，停止播放器，期望返回CSUDI_SUCCESS
//@EXECUTIONFLOW:13、重复步骤3-步骤11指定次数
//@EXECUTIONFLOW:14、恢复现场
CSUDI_BOOL CSTC_FILE_TEST_IT_SetGetSpeed_0001(void)
{
	int i = 0;
	CSUDI_HANDLE   hPlayer = CSUDI_NULL;
	CSUDI_Error_Code errorCode = CSUDI_SUCCESS;
	CSUDIPlayerSpeed_E eSpeed;
	CSUDIPlaybackParam_S stPlaybackParam;
	CSUDIPLAYERType_E	ePlayerType=EM_UDIPLAYER_FILE;

	memset(&stPlaybackParam,0,sizeof(stPlaybackParam));
	stPlaybackParam.m_eSpeed = EM_UDIPLAYER_SPEED_NORMAL;
	stPlaybackParam.m_nSecondPos = 0;

	strncpy(stPlaybackParam.m_szFileName, stFilePath[EM_UDI_FILENAME_MP4_SD].filename, CSUDI_MAX_FILE_NAME_LEN);

	//创建播放器
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYEROpen (NULL,ePlayerType,&hPlayer), "步骤1失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_NULL  != hPlayer, "步骤1失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERSetStream(hPlayer,NULL,0,&stPlaybackParam), "步骤2失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERAddFilePlayerCallback(hPlayer,FilePlayCallback,NULL), "注册回调失败");

	for (i=0;i<REPEAT_TIMES;i++)
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDIPLAYER_ERROR_INVALID_HANDLE  == CSUDIPLAYERSetSpeed(CSUDI_NULL,eSpeed), "步骤3失败");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  != CSUDIPLAYERSetSpeed(CSUDI_NULL,EM_UDIPLAYER_SPEED_FASTFORWARD_32+10), "步骤4失败");

		CSTK_ASSERT_TRUE_FATAL(CSUDIPLAYER_ERROR_INVALID_HANDLE  == CSUDIPLAYERGetSpeed(CSUDI_NULL,&eSpeed), "步骤5失败");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  != CSUDIPLAYERGetSpeed(hPlayer,CSUDI_NULL), "步骤6失败");

		errorCode = CSUDIPLAYERSetSpeed(hPlayer,eSpeed);

		//in live mode, it's return CSUDIPLAYER_ERROR_FEATURE_NOT_SUPPORTED
		CSTK_ASSERT_TRUE_FATAL((CSUDIPLAYER_ERROR_FEATURE_NOT_SUPPORTED == errorCode)||(CSUDI_SUCCESS != errorCode), "步骤7失败");

		errorCode = CSUDIPLAYERGetSpeed(hPlayer,&eSpeed);

		CSTK_ASSERT_TRUE_FATAL((CSUDIPLAYER_ERROR_FEATURE_NOT_SUPPORTED == errorCode)||(CSUDI_SUCCESS == errorCode), "步骤8失败");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERStart(hPlayer), "步骤9失败");

		CSUDIOSThreadSleep(SLEEP_TIME);

		errorCode = CSUDIPLAYERSetSpeed(hPlayer, EM_UDIPLAYER_SPEED_FASTFORWARD_32);

		//in live mode, it's return CSUDIPLAYER_ERROR_FEATURE_NOT_SUPPORTED
		CSTK_ASSERT_TRUE_FATAL((CSUDIPLAYER_ERROR_FEATURE_NOT_SUPPORTED == errorCode)||(CSUDI_SUCCESS == errorCode), "步骤10失败");

		CSUDIOSThreadSleep(SLEEP_TIME);

		errorCode = CSUDIPLAYERGetSpeed(hPlayer,&eSpeed);

        CSTK_ASSERT_TRUE_FATAL((CSUDIPLAYER_ERROR_FEATURE_NOT_SUPPORTED == errorCode)||(CSUDI_SUCCESS == errorCode), "步骤11失败");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERStop(hPlayer), "步骤12失败");
	}

	CSTK_FATAL_POINT;

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERDelFilePlayerCallback(hPlayer, FilePlayCallback, NULL), "删除回调失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERClose (hPlayer), "恢复现场");

	return CSUDI_TRUE;
}

//@CASEGROUP:CSUDIPLAYERSetSpeed&CSUDIPLAYERGetSpeed
//@DESCRIPTION:1、测试本地播放情况下，CSUDIPLAYERSetSpeed函数的执行情况
//@DESCRIPTION:2、测试本地播放情况下，CSUDIPLAYERGetSpeed函数的执行情况
//@PRECONDITION:PLAYER模块被正常初始化
//@INPUT:
//@EXPECTATION:
//@REMARK:
//@EXECUTIONFLOW:1、打开一个本地文件播放器，得到播放器句柄
//@EXECUTIONFLOW:2、调用CSUDIPLAYERSetStream，期望返回CSUDI_SUCCESS
//@EXECUTIONFLOW:3、hPlayer为CSUDI_NULL，调用 CSUDIPLAYERSetSpeed，期望返回非CSUDI_SUCCESS
//@EXECUTIONFLOW:4、eSpeed为无效值，调用 CSUDIPLAYERSetSpeed，期望返回非CSUDI_SUCCESS
//@EXECUTIONFLOW:5、hPlayer为CSUDI_NULL，调用 CSUDIPLAYERGetSpeed，期望返回非CSUDI_SUCCESS
//@EXECUTIONFLOW:6、eSpeed为无效值，调用 CSUDIPLAYERGetSpeed，期望返回非CSUDI_SUCCESS
//@EXECUTIONFLOW:7、参数正确的情况下，Start播放器之前调用CSUDIPLAYERSetSpeed，期望返回的不为CSUDI_SUCCESS或为CSUDIPLAYER_ERROR_FEATURE_NOT_SUPPORTED
//@EXECUTIONFLOW:8、参数正确的情况下，Start播放器之前调用CSUDIPLAYERGetSpeed，期望返回的不为CSUDI_SUCCESS或为CSUDIPLAYER_ERROR_FEATURE_NOT_SUPPORTED
//@EXECUTIONFLOW:9、调用CSUDIPLAYERStart，Start播放器，期望返回CSUDI_SUCCESS
//@EXECUTIONFLOW:10、参数正确的情况下，Start之后调用CSUDIPLAYERSetSpeed，期望返回CSUDI_SUCCESS或CSUDIPLAYER_ERROR_FEATURE_NOT_SUPPORTED
//@EXECUTIONFLOW:11、参数正确的情况下，Start之后调用CSUDIPLAYERGetSpeed，期望返回CSUDI_SUCCESS或CSUDIPLAYER_ERROR_FEATURE_NOT_SUPPORTED
//@EXECUTIONFLOW:12、调用CSUDIPLAYERStop，停止播放器，期望返回CSUDI_SUCCESS
//@EXECUTIONFLOW:13、重复步骤3-步骤11指定次数
//@EXECUTIONFLOW:14、恢复现场
CSUDI_BOOL CSTC_FILE_TEST_IT_SetGetSpeed_0002(void)
{
	int i = 0;
	unsigned char moveToend = 0;
	CSUDI_HANDLE   hPlayer = CSUDI_NULL;
	CSUDI_Error_Code errorCode = CSUDI_SUCCESS;
	CSUDIPlayerSpeed_E eSpeed;
	CSUDIPlaybackParam_S stPlaybackParam;
	CSUDIPLAYERType_E ePlayerType=EM_UDIPLAYER_FILE;
	CSUDI_BOOL bPlaying = CSUDI_FALSE;
	CSUDI_BOOL bAddCallback = CSUDI_FALSE;

	memset(&stPlaybackParam, 0, sizeof(stPlaybackParam));
	stPlaybackParam.m_eSpeed = EM_UDIPLAYER_SPEED_NORMAL;
	stPlaybackParam.m_nSecondPos = 0;

	strncpy(stPlaybackParam.m_szFileName, stFilePath[EM_UDI_FILENAME_MP4_SD].filename, CSUDI_MAX_FILE_NAME_LEN);

	//创建播放器
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYEROpen(NULL, ePlayerType, &hPlayer), "步骤1失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_NULL   != hPlayer, "步骤1失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERSetStream(hPlayer, NULL, 0, &stPlaybackParam), "步骤2失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERAddFilePlayerCallback(hPlayer, FilePlayCallback, &moveToend), "注册回调失败");

	bAddCallback = CSUDI_TRUE;

	for (i = 0;i < REPEAT_TIMES;i++)
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDIPLAYER_ERROR_INVALID_HANDLE   == CSUDIPLAYERSetSpeed(CSUDI_NULL, eSpeed), "步骤3失败");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS   != CSUDIPLAYERSetSpeed(CSUDI_NULL, EM_UDIPLAYER_SPEED_FASTFORWARD_32+10), "步骤4失败");

		CSTK_ASSERT_TRUE_FATAL(CSUDIPLAYER_ERROR_INVALID_HANDLE   == CSUDIPLAYERGetSpeed(CSUDI_NULL, &eSpeed), "步骤5失败");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS   != CSUDIPLAYERGetSpeed(hPlayer,CSUDI_NULL), "步骤6失败");

		errorCode = CSUDIPLAYERSetSpeed(hPlayer, eSpeed);

		//in live mode, it's return CSUDIPLAYER_ERROR_FEATURE_NOT_SUPPORTED
		CSTK_ASSERT_TRUE_FATAL((CSUDIPLAYER_ERROR_FEATURE_NOT_SUPPORTED == errorCode) || (CSUDI_SUCCESS != errorCode), "步骤7失败");

		errorCode = CSUDIPLAYERGetSpeed(hPlayer, &eSpeed);

		CSTK_ASSERT_TRUE_FATAL((CSUDIPLAYER_ERROR_FEATURE_NOT_SUPPORTED == errorCode) || (CSUDI_SUCCESS == errorCode), "步骤8失败");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERStart(hPlayer), "步骤9失败");

		bPlaying = CSUDI_TRUE;

		CSUDIOSThreadSleep(SLEEP_TIME);

		CSTCPrint("音视频是否能正常播放？\n");
		CSTCPrint("Is A/V output normal ?\n");
		CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes(), "步骤9失败：播放不正常");

		moveToend = 0;
		errorCode = CSUDIPLAYERSetSpeed(hPlayer, EM_UDIPLAYER_SPEED_FASTFORWARD_32);

		//in live mode, it's return CSUDIPLAYER_ERROR_FEATURE_NOT_SUPPORTED
		CSTK_ASSERT_TRUE_FATAL((CSUDIPLAYER_ERROR_FEATURE_NOT_SUPPORTED == errorCode) || (CSUDI_SUCCESS == errorCode), "步骤10失败");

		CSTCPrint("视频是否快进播放？ \n");
		CSTCPrint("Is A/V output fast forward?");
		CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes(), "步骤10失败：快进不正常");

		if (FORWARD_TO_END == moveToend)
		{
			CSTCPrint("视频已经快进到末尾！\n");
			break;
		}

		errorCode = CSUDIPLAYERGetSpeed(hPlayer, &eSpeed);

        CSTK_ASSERT_TRUE_FATAL((CSUDIPLAYER_ERROR_FEATURE_NOT_SUPPORTED == errorCode) || (CSUDI_SUCCESS == errorCode), "步骤11失败");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS   == CSUDIPLAYERStop(hPlayer), "步骤12失败");

		bPlaying = CSUDI_FALSE;

		CSTCPrint("视频是否停止播放？ \n");
		CSTCPrint("Is A/V output stop?");
		CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes(), "步骤10失败：停止不正常");
	}

	CSTK_FATAL_POINT;

	if (bPlaying)
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS   == CSUDIPLAYERStop(hPlayer), "停止播放失败");
	}

	if (bAddCallback)
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERDelFilePlayerCallback(hPlayer, FilePlayCallback, &moveToend), "删除回调失败");
	}

	if (CSUDI_NULL != hPlayer)
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS   == CSUDIPLAYERClose(hPlayer), "恢复现场");
	}
	
	return CSUDI_TRUE;
}

//@CASEGROUP:CSUDIPLAYERSeek
//@DESCRIPTION:1、测试本地文件播放情况下，接口函数CSUDIPLAYERSeek的执行情况
//@DESCRIPTION:2、CSUDIPLAYERSeek分三种情况进行验证
//@DESCRIPTION:3、CSUDIPLAYERSeek CSUDIPLAYERGetCurPosInSec组合使用情况
//@PRECONDITION:PLAYER模块被正常初始化
//@INPUT:1、有效的hPlayer
//@INPUT:2、有效的nPosInSec
//@INPUT:3、ePlayPosFlag=EM_UDIPLAYER_POSITION_FROM_CURRENT
//@EXPECTATION:直播模式下不支持  CSUDIPLAYERSeek
//@REMARK:
//@EXECUTIONFLOW:1、打开一个本地文件播放器，得到播放器句柄
//@EXECUTIONFLOW:2、调用CSUDIPLAYERSetStream，期望返回CSUDI_SUCCESS
//@EXECUTIONFLOW:3、调用CSUDIPLAYERStart，期望返回CSUDI_SUCCESS
//@EXECUTIONFLOW:4、hPlayer为CSUDI_NULL，调用 CSUDIPLAYERSeek，期望返回非CSUDI_SUCCESS
//@EXECUTIONFLOW:5、以正常合法参数调用 CSUDIPLAYERSeek，期望返回CSUDI_SUCCESS
//@EXECUTIONFLOW:6、重复步骤3-步骤4指定次数
//@EXECUTIONFLOW:7、调用CSUDIPLAYERStop，期望返回CSUDI_SUCCESS
//@EXECUTIONFLOW:8、恢复现场
CSUDI_BOOL CSTC_FILE_TEST_IT_Seek_0001(void)
{
	CSUDIPlayPosition_E   ePlayPosFlag=EM_UDIPLAYER_POSITION_FROM_CURRENT;
	CSUDIPlaybackParam_S stPlaybackParam;
	CSUDI_HANDLE   hPlayer = CSUDI_NULL;
	CSUDIPLAYERType_E	ePlayerType=EM_UDIPLAYER_FILE;

	memset(&stPlaybackParam,0,sizeof(stPlaybackParam));
	stPlaybackParam.m_eSpeed = EM_UDIPLAYER_SPEED_NORMAL;
	stPlaybackParam.m_nSecondPos = 0;

	strncpy(stPlaybackParam.m_szFileName, stFilePath[EM_UDI_FILENAME_MP4_SD].filename, CSUDI_MAX_FILE_NAME_LEN);

	//创建播放器
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYEROpen (NULL,ePlayerType,&hPlayer), "步骤1失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_NULL  != hPlayer, "步骤1失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERSetStream (hPlayer,NULL,0,&stPlaybackParam), "步骤2失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERAddFilePlayerCallback(hPlayer,FilePlayCallback, NULL), "注册回调失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERStart(hPlayer), "步骤3失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  != CSUDIPLAYERSeek(CSUDI_NULL,100,ePlayPosFlag), "步骤4失败");

	CSUDIOSThreadSleep(SLEEP_TIME);

	for (ePlayPosFlag=EM_UDIPLAYER_POSITION_FROM_HEAD;ePlayPosFlag<=EM_UDIPLAYER_POSITION_FROM_END;ePlayPosFlag++)
	{
		if (ePlayPosFlag == EM_UDIPLAYER_POSITION_FROM_END)
		{
			CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERSeek(hPlayer,-100 ,ePlayPosFlag), "步骤5失败");
		}
		else
		{
			CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERSeek(hPlayer, 50, ePlayPosFlag), "步骤5失败");
		}

		CSUDIOSThreadSleep(SLEEP_TIME);
	}

	CSTK_FATAL_POINT;

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERStop(hPlayer), "步骤7失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERDelFilePlayerCallback(hPlayer,FilePlayCallback, NULL), "删除回调失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERClose (hPlayer), "恢复现场");

	return CSUDI_TRUE;
}

//@CASEGROUP:CSUDIPLAYERGetCurPosInSec
//@DESCRIPTION:1、测试本地文件播放情况下，接口函数CSUDIPLAYERGetCurPosInSec的执行情况
//@DESCRIPTION:2、CSUDIPLAYERGetCurPosInSec  CSUDIPLAYERSetSpeed  CSUDIPLAYERSeek组合使用情况
//@PRECONDITION:PLAYER模块被正常初始化
//@INPUT:
//@EXPECTATION:
//@REMARK:
//@EXECUTIONFLOW:1、打开一个本地文件播放器，得到播放器句柄
//@EXECUTIONFLOW:2、调用CSUDIPLAYERSetStream，期望返回CSUDI_SUCCESS
//@EXECUTIONFLOW:3、hPlayer为CSUDI_NULL，调用 CSUDIPLAYERGetCurPosInSec，期望返回非CSUDI_SUCCESS
//@EXECUTIONFLOW:4、pnPosInSec为CSUDI_NULL，调用 CSUDIPLAYERGetCurPosInSec，期望返回非CSUDI_SUCCESS
//@EXECUTIONFLOW:5、以正常合法参数调用 CSUDIPLAYERStart，期望返回CSUDI_SUCCESS
//@EXECUTIONFLOW:6、以正常合法参数调用 CSUDIPLAYERSeek，期望返回CSUDI_SUCCESS
//@EXECUTIONFLOW:7、以正常合法参数调用 CSUDIPLAYERResume，期望返回CSUDI_SUCCESS
//@EXECUTIONFLOW:7、以正常合法参数调用 CSUDIPLAYERGetCurPosInSec，期望返回CSUDI_SUCCESS
//@EXECUTIONFLOW:8、以正常合法参数调用 CSUDIPLAYERStop，期望返回CSUDI_SUCCESS
//@EXECUTIONFLOW:9、重复步骤3-步骤5指定次数
//@EXECUTIONFLOW:10、恢复现场
CSUDI_BOOL CSTC_FILE_TEST_IT_GetCurPosInSec_0001(void)
{
	int  nPosInSec = 0;
	CSUDIPlaybackParam_S stPlaybackParam;
	int i= 0;
	CSUDI_HANDLE   hPlayer = CSUDI_NULL;
    CSUDIPLAYERType_E	ePlayerType=EM_UDIPLAYER_FILE;

	memset(&stPlaybackParam,0,sizeof(stPlaybackParam));
	stPlaybackParam.m_eSpeed = EM_UDIPLAYER_SPEED_NORMAL;
	stPlaybackParam.m_nSecondPos = 0;

	strncpy(stPlaybackParam.m_szFileName, stFilePath[EM_UDI_FILENAME_MP4_SD].filename, CSUDI_MAX_FILE_NAME_LEN);

	//创建播放器
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYEROpen (NULL,ePlayerType,&hPlayer), "步骤1失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_NULL  != hPlayer, "步骤1失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERSetStream (hPlayer,NULL,0,&stPlaybackParam), "步骤2失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERAddFilePlayerCallback(hPlayer,FilePlayCallback,NULL), "注册回调失败");

    for (i=0;i<REPEAT_TIMES;i++)
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  != CSUDIPLAYERGetCurPosInSec(CSUDI_NULL,&nPosInSec), "步骤3失败");

		//check porting code pls,it have not check pnPosInSec
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  != CSUDIPLAYERGetCurPosInSec(hPlayer,CSUDI_NULL), "步骤4失败");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERStart(hPlayer), "步骤5失败");

		CSUDIOSThreadSleep(SLEEP_TIME);

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERSeek(hPlayer, 100, EM_UDIPLAYER_POSITION_FROM_CURRENT), "步骤6失败");

		//CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERResume(hPlayer), "恢复播放失败");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERGetCurPosInSec(hPlayer,&nPosInSec), "步骤7失败");

		CSUDIOSThreadSleep(SLEEP_TIME);

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERStop(hPlayer), "步骤8失败");

		CSUDIOSThreadSleep(300);//sleep 300ms 是为了stop之后能完全清空视频缓存
	}

	CSTK_FATAL_POINT;

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERDelFilePlayerCallback(hPlayer,FilePlayCallback,NULL), "删除回调失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERClose (hPlayer), "恢复现场失败");

	return CSUDI_TRUE;
}

//@CASEGROUP:CSUDIPLAYERGetDuration
//@DESCRIPTION:1、测试本地文件播放情况下，接口函数CSUDIPLAYERGetDuration的执行情况
//@DESCRIPTION:2、这里允许获得的总时间和实际情况误差1秒
//@PRECONDITION:PLAYER模块被正常初始化
//@INPUT:
//@EXPECTATION:
//@REMARK:
//@EXECUTIONFLOW:1、打开一个本地文件播放器，得到播放器句柄
//@EXECUTIONFLOW:2、调用CSUDIPLAYERSetStream，期望返回CSUDI_SUCCESS
//@EXECUTIONFLOW:3、调用CSUDIPLAYERStart，期望返回CSUDI_SUCCESS
//@EXECUTIONFLOW:4、hPlayer为CSUDI_NULL，调用 CSUDIPLAYERGetDuration，期望返回非CSUDI_SUCCESS
//@EXECUTIONFLOW:5、pnPosInSec为CSUDI_NULL，调用 CSUDIPLAYERGetDuration，期望返回非CSUDI_SUCCESS
//@EXECUTIONFLOW:6、以正常合法参数调用 CSUDIPLAYERGetDuration，期望返回CSUDI_SUCCESS
//@EXECUTIONFLOW:7、调用CSUDIPLAYERStop，期望返回CSUDI_SUCCESS
//@EXECUTIONFLOW:8、重复步骤3-步骤5指定次数
//@EXECUTIONFLOW:9、恢复现场
CSUDI_BOOL CSTC_FILE_TEST_IT_GetDuration_0001(void)
{
	int  nPosInSec = 0;
	CSUDIPlaybackParam_S stPlaybackParam;
	int i = 0;
	int j = EM_UDI_FILENAME_WMV;
	CSUDI_Error_Code eCode = CSUDI_SUCCESS;
	CSUDI_HANDLE   hPlayer = CSUDI_NULL;
	CSUDIPLAYERType_E	ePlayerType=EM_UDIPLAYER_FILE;

	memset(&stPlaybackParam,0,sizeof(stPlaybackParam));
	stPlaybackParam.m_eSpeed = EM_UDIPLAYER_SPEED_NORMAL;
	stPlaybackParam.m_nSecondPos = 0;

	strncpy(stPlaybackParam.m_szFileName, stFilePath[EM_UDI_FILENAME_MP4_SD].filename, CSUDI_MAX_FILE_NAME_LEN);

	//创建播放器
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYEROpen (NULL,ePlayerType,&hPlayer), "步骤1失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_NULL  != hPlayer, "步骤1失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERSetStream (hPlayer,NULL,0,&stPlaybackParam), "步骤2失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERAddFilePlayerCallback(hPlayer,FilePlayCallback,NULL), "注册回调失败");

	for (i=0; i<REPEAT_TIMES; i++)
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERStart (hPlayer), "步骤3失败");

		CSUDIOSThreadSleep(SLEEP_TIME);

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  != CSUDIPLAYERGetDuration(CSUDI_NULL,&nPosInSec), "步骤4失败");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  != CSUDIPLAYERGetDuration(hPlayer,NULL), "步骤5失败");

		eCode = CSUDIPLAYERGetDuration(hPlayer,&nPosInSec);

		int value = 0;
		value = nPosInSec - s_svrfiletime[i];

		if (nPosInSec >= s_svrfiletime[i])
		{
			CSTK_ASSERT_TRUE_FATAL( value >= 0 && value <= 1, "获取文件总时间失败");
		}
		else
		{
			CSTK_ASSERT_TRUE_FATAL(value <= -1, "获取文件总时间失败");
		}

		CSTK_ASSERT_TRUE_FATAL((CSUDIPLAYER_ERROR_FEATURE_NOT_SUPPORTED == eCode)||(CSUDI_SUCCESS == eCode), "步骤6失败");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERStop(hPlayer), "步骤7失败");

		memset(&stPlaybackParam,0,sizeof(stPlaybackParam));
		stPlaybackParam.m_eSpeed = EM_UDIPLAYER_SPEED_NORMAL;
		stPlaybackParam.m_nSecondPos = 0;

		j++;

		strncpy(stPlaybackParam.m_szFileName, stFilePath[j].filename, CSUDI_MAX_FILE_NAME_LEN);

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERSetStream (hPlayer,NULL,0,&stPlaybackParam), "重新设置播放属性失败");

	}

	CSTK_FATAL_POINT;

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERDelFilePlayerCallback(hPlayer,FilePlayCallback,NULL), "删除回调失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERClose (hPlayer), "恢复现场失败");

	return CSUDI_TRUE;
}


static CSUDI_BOOL FILE_TEST_IT_Play_Base(int formatType)
{
	CSUDI_HANDLE hPlayer = CSUDI_NULL;
	CSUDI_Error_Code udiRe = CSUDI_SUCCESS;
	CSUDIPlaybackParam_S stPlaybackParam;
	CSUDIPLAYERType_E ePlayerType = EM_UDIPLAYER_FILE;
	CSUDI_BOOL bPlaying = CSUDI_FALSE;
	CSUDI_BOOL bAddCallback = CSUDI_FALSE;

	memset(&stPlaybackParam, 0, sizeof(stPlaybackParam));
	stPlaybackParam.m_eSpeed = EM_UDIPLAYER_SPEED_NORMAL;
	stPlaybackParam.m_nSecondPos = 0;

	strncpy(stPlaybackParam.m_szFileName, stFilePath[formatType].filename, CSUDI_MAX_FILE_NAME_LEN);

	CSTCPrint("本用例测试 \"%s\" 的播放\n", stPlaybackParam.m_szFileName);

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == (udiRe = CSUDIPLAYERProbe(stPlaybackParam.m_szFileName)), "步骤1失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYEROpen(NULL, ePlayerType, &hPlayer), "步骤2失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_NULL != hPlayer, "步骤2失败：打开文件异常");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERSetStream(hPlayer, NULL, 0, &stPlaybackParam), "步骤3设置流信息失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERAddFilePlayerCallback(hPlayer, FilePlayCallback,  CSUDI_NULL), "步骤4添加回调失败");

	bAddCallback = CSUDI_TRUE;

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERStart(hPlayer), "步骤5失败");

	bPlaying = CSUDI_TRUE;

	CSUDIOSThreadSleep(SLEEP_TIME);

	CSTCPrint("音视频是否能够正常播放？\n");
	CSTCPrint("Is A/V output normal ?\n");
	
	CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes(), "步骤6失败：播放不正常");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERStop(hPlayer), "步骤7失败");

	bPlaying = CSUDI_FALSE;

	CSTCPrint("音视频是否停止播放了\n");
	CSTCPrint("Does the player stop ?\n");
	
	CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes(), "步骤8失败：停止播放不正常");
	
	CSTK_FATAL_POINT;

	if (bPlaying)
	{
		CSTK_ASSERT_TRUE(CSUDI_SUCCESS == CSUDIPLAYERStop(hPlayer), "停止播放失败");
	}

	if (bAddCallback)
	{
		CSTK_ASSERT_TRUE(CSUDI_SUCCESS == CSUDIPLAYERDelFilePlayerCallback(hPlayer, FilePlayCallback, CSUDI_NULL), "删除回调失败");
	}
			
	if (CSUDI_NULL != hPlayer)
	{
		CSTK_ASSERT_TRUE(CSUDI_SUCCESS == CSUDIPLAYERClose(hPlayer), "关闭文件失败");
		
		hPlayer = CSUDI_NULL;
	}
	
	return CSUDI_TRUE;
}

//@CASEGROUP:FILEPLAYER
//@DESCRIPTION:测试本地文件是否支持播放标清TS 流
//@PRECONDITION:PLAYER模块，音视频解码器被正常初始化
//@INPUT:
//@EXECUTIONFLOW:1、调用CSUDIPLAYERProbe，期待返回CSUDI_SUCCESS
//@EXECUTIONFLOW:2、调用CSUDIPLAYEROpen，打开一个本地文件播放器
//@EXECUTIONFLOW:3、调用CSUDIPLAYERSetStream，设置播放器相应属性
//@EXECUTIONFLOW:4、调用CSUDIPLAYERAddFilePlayerCallback，添加文件开始播放的回调事件
//@EXECUTIONFLOW:5、调用CSUDIPLAYERStart  打开播放器
//@EXECUTIONFLOW:6、提示测试人员观察，是否有音视频播放出来
//@EXECUTIONFLOW:7、调用CSUDIPLAYERStop  停止播放器
//@EXECUTIONFLOW:8、调用CSUDIPLAYERDelFilePlayerCallback，删除文件开始播放的回调事件
//@EXECUTIONFLOW:9、提示测试人员观察，是否音视频停止播放
//@EXECUTIONFLOW:10、调用CSUDIPLAYERClose删除播放器
CSUDI_BOOL CSTC_FILE_TEST_IT_0001(void)
{
	FILE_TEST_IT_Play_Base(EM_UDI_FILENAME_TS_SD);

	return CSUDI_TRUE;
}


//@CASEGROUP:FILEPLAYER
//@DESCRIPTION:测试本地文件是否支持播放高清TS流
//@PRECONDITION:PLAYER模块，音视频解码器被正常初始化
//@INPUT:
//@EXECUTIONFLOW:1、调用CSUDIPLAYERProbe，期待返回CSUDI_SUCCESS
//@EXECUTIONFLOW:2、调用CSUDIPLAYEROpen，打开一个本地文件播放器
//@EXECUTIONFLOW:3、调用CSUDIPLAYERSetStream，设置播放器相应属性
//@EXECUTIONFLOW:4、调用CSUDIPLAYERAddFilePlayerCallback，添加播放到文件尾的回调事件
//@EXECUTIONFLOW:5、调用CSUDIPLAYERStart  打开播放器
//@EXECUTIONFLOW:6、提示测试人员观察，是否有音视频播放出来
//@EXECUTIONFLOW:7、调用CSUDIPLAYERStop  停止播放器
//@EXECUTIONFLOW:8、调用CSUDIPLAYERDelFilePlayerCallback，删除回调事件
//@EXECUTIONFLOW:9、提示测试人员观察，是否音视频停止播放
//@EXECUTIONFLOW:10、调用CSUDIPLAYERClose删除播放器
CSUDI_BOOL CSTC_FILE_TEST_IT_0002(void)
{
	FILE_TEST_IT_Play_Base(EM_UDI_FILENAME_TS_HD);

	return CSUDI_TRUE;
}

//@CASEGROUP:FILEPLAYER
//@DESCRIPTION:测试本地文件是否支持播放WMV格式的数据文件
//@PRECONDITION:PLAYER模块，音视频解码器被正常初始化
//@INPUT:
//@EXECUTIONFLOW:1、调用CSUDIPLAYERProbe，期待返回CSUDI_SUCCESS
//@EXECUTIONFLOW:2、调用CSUDIPLAYEROpen，打开一个本地文件播放器
//@EXECUTIONFLOW:3、调用CSUDIPLAYERSetStream，设置播放器相应属性
//@EXECUTIONFLOW:4、调用CSUDIPLAYERAddFilePlayerCallback，添加播放到文件尾的回调事件
//@EXECUTIONFLOW:5、调用CSUDIPLAYERStart  打开播放器
//@EXECUTIONFLOW:6、提示测试人员观察，是否有音视频播放出来
//@EXECUTIONFLOW:7、调用CSUDIPLAYERStop  停止播放器
//@EXECUTIONFLOW:8、调用CSUDIPLAYERDelFilePlayerCallback，删除回调事件
//@EXECUTIONFLOW:9、提示测试人员观察，是否音视频停止播放
//@EXECUTIONFLOW:10    调用CSUDIPLAYERClose删除播放器
CSUDI_BOOL CSTC_FILE_TEST_IT_0003(void)
{
	FILE_TEST_IT_Play_Base(EM_UDI_FILENAME_WMV);

	return CSUDI_TRUE;
}

//@CASEGROUP:FILEPLAYER
//@DESCRIPTION:测试本地文件是否支持播放AVI格式标清文件
//@PRECONDITION:PLAYER模块，音视频解码器被正常初始化
//@INPUT:
//@EXECUTIONFLOW:1、调用CSUDIPLAYERProbe，期待返回CSUDI_SUCCESS
//@EXECUTIONFLOW:2、调用CSUDIPLAYEROpen，打开一个本地文件播放器
//@EXECUTIONFLOW:3、调用CSUDIPLAYERSetStream，设置播放器相应属性
//@EXECUTIONFLOW:4、调用CSUDIPLAYERAddFilePlayerCallback，添加文件开始播放的回调事件
//@EXECUTIONFLOW:5、调用CSUDIPLAYERStart  打开播放器
//@EXECUTIONFLOW:6、提示测试人员观察，是否有音视频播放出来
//@EXECUTIONFLOW:7、调用CSUDIPLAYERStop  停止播放器
//@EXECUTIONFLOW:8、调用CSUDIPLAYERDelFilePlayerCallback，删除回调事件
//@EXECUTIONFLOW:9、提示测试人员观察，是否音视频停止播放
//@EXECUTIONFLOW:10    调用CSUDIPLAYERClose删除播放器
CSUDI_BOOL CSTC_FILE_TEST_IT_0004(void)
{
	FILE_TEST_IT_Play_Base(EM_UDI_FILENAME_AVI_SD);

	return CSUDI_TRUE;
}

//@CASEGROUP:FILEPLAYER
//@DESCRIPTION:测试本地文件是否支持播放AVI格式高清文件
//@PRECONDITION:PLAYER模块，音视频解码器被正常初始化
//@INPUT:
//@EXECUTIONFLOW:1、调用CSUDIPLAYERProbe，期待返回CSUDI_SUCCESS
//@EXECUTIONFLOW:2、调用CSUDIPLAYEROpen，打开一个本地文件播放器
//@EXECUTIONFLOW:3、调用CSUDIPLAYERSetStream，设置播放器相应属性
//@EXECUTIONFLOW:4、调用CSUDIPLAYERAddFilePlayerCallback，添加文件开始播放的回调事件
//@EXECUTIONFLOW:5、调用CSUDIPLAYERStart  打开播放器
//@EXECUTIONFLOW:6、提示测试人员观察，是否有音视频播放出来
//@EXECUTIONFLOW:7、调用CSUDIPLAYERStop  停止播放器
//@EXECUTIONFLOW:8、调用CSUDIPLAYERDelFilePlayerCallback，删除回调事件
//@EXECUTIONFLOW:9、提示测试人员观察，是否音视频停止播放
//@EXECUTIONFLOW:10    调用CSUDIPLAYERClose删除播放器
CSUDI_BOOL CSTC_FILE_TEST_IT_0005(void)
{
	FILE_TEST_IT_Play_Base(EM_UDI_FILENAME_AVI_HD);

	return CSUDI_TRUE;
}

//@CASEGROUP:FILEPLAYER
//@DESCRIPTION:测试本地文件是否支持播放RMVB格式标清文件
//@PRECONDITION:PLAYER模块，音视频解码器被正常初始化
//@INPUT:
//@EXECUTIONFLOW:1、调用CSUDIPLAYERProbe，期待返回CSUDI_SUCCESS
//@EXECUTIONFLOW:2、调用CSUDIPLAYEROpen，打开一个本地文件播放器
//@EXECUTIONFLOW:3、调用CSUDIPLAYERSetStream，设置播放器相应属性
//@EXECUTIONFLOW:4、调用CSUDIPLAYERAddFilePlayerCallback，添加文件开始播放的回调事件
//@EXECUTIONFLOW:5、调用CSUDIPLAYERStart  打开播放器
//@EXECUTIONFLOW:6、提示测试人员观察，是否有音视频播放出来
//@EXECUTIONFLOW:7、调用CSUDIPLAYERStop  停止播放器
//@EXECUTIONFLOW:8、调用CSUDIPLAYERDelFilePlayerCallback，删除回调事件
//@EXECUTIONFLOW:9、提示测试人员观察，是否音视频停止播放
//@EXECUTIONFLOW:10    调用CSUDIPLAYERClose删除播放器
CSUDI_BOOL CSTC_FILE_TEST_IT_0006(void)
{
	FILE_TEST_IT_Play_Base(EM_UDI_FILENAME_RMVB_SD);

	return CSUDI_TRUE;
}

//@CASEGROUP:FILEPLAYER
//@DESCRIPTION:测试本地文件是否支持播放RMVB格式高清文件
//@PRECONDITION:PLAYER模块，音视频解码器被正常初始化
//@INPUT:
//@EXECUTIONFLOW:1、调用CSUDIPLAYERProbe，期待返回CSUDI_SUCCESS
//@EXECUTIONFLOW:2、调用CSUDIPLAYEROpen，打开一个本地文件播放器
//@EXECUTIONFLOW:3、调用CSUDIPLAYERSetStream，设置播放器相应属性
//@EXECUTIONFLOW:4、调用CSUDIPLAYERAddFilePlayerCallback，添加文件开始播放的回调事件
//@EXECUTIONFLOW:5、调用CSUDIPLAYERStart  打开播放器
//@EXECUTIONFLOW:6、提示测试人员观察，是否有音视频播放出来
//@EXECUTIONFLOW:7、调用CSUDIPLAYERStop  停止播放器
//@EXECUTIONFLOW:8、调用CSUDIPLAYERDelFilePlayerCallback，删除回调事件
//@EXECUTIONFLOW:9、提示测试人员观察，是否音视频停止播放
//@EXECUTIONFLOW:10    调用CSUDIPLAYERClose删除播放器
CSUDI_BOOL CSTC_FILE_TEST_IT_0007(void)
{
	FILE_TEST_IT_Play_Base(EM_UDI_FILENAME_RMVB_HD);

	return CSUDI_TRUE;
}

//@CASEGROUP:FILEPLAYER
//@DESCRIPTION:测试本地文件是否支持播放FLV格式文件
//@PRECONDITION:PLAYER模块，音视频解码器被正常初始化
//@INPUT:
//@EXECUTIONFLOW:1、调用CSUDIPLAYERProbe，期待返回CSUDI_SUCCESS
//@EXECUTIONFLOW:2、调用CSUDIPLAYEROpen，打开一个本地文件播放器
//@EXECUTIONFLOW:3、调用CSUDIPLAYERSetStream，设置播放器相应属性
//@EXECUTIONFLOW:4、调用CSUDIPLAYERAddFilePlayerCallback，添加文件开始播放的回调事件
//@EXECUTIONFLOW:5、调用CSUDIPLAYERStart  打开播放器
//@EXECUTIONFLOW:6、提示测试人员观察，是否有音视频播放出来
//@EXECUTIONFLOW:7、调用CSUDIPLAYERStop  停止播放器
//@EXECUTIONFLOW:8、调用CSUDIPLAYERDelFilePlayerCallback，删除回调事件
//@EXECUTIONFLOW:9、提示测试人员观察，是否音视频停止播放
//@EXECUTIONFLOW:10    调用CSUDIPLAYERClose删除播放器
CSUDI_BOOL CSTC_FILE_TEST_IT_0008(void)
{
	FILE_TEST_IT_Play_Base(EM_UDI_FILENAME_FLV);

	return CSUDI_TRUE;
}

//@CASEGROUP:FILEPLAYER
//@DESCRIPTION:测试本地文件是否支持播放MP3格式文件
//@PRECONDITION:PLAYER模块，音视频解码器被正常初始化
//@INPUT:
//@EXECUTIONFLOW:1、调用CSUDIPLAYERProbe，期待返回CSUDI_SUCCESS
//@EXECUTIONFLOW:2、调用CSUDIPLAYEROpen，打开一个本地文件播放器
//@EXECUTIONFLOW:3、调用CSUDIPLAYERSetStream，设置播放器相应属性
//@EXECUTIONFLOW:4、调用CSUDIPLAYERAddFilePlayerCallback，添加文件开始播放的回调事件
//@EXECUTIONFLOW:5、调用CSUDIPLAYERStart  打开播放器
//@EXECUTIONFLOW:6、提示测试人员观察，是否有音视频播放出来
//@EXECUTIONFLOW:7、调用CSUDIPLAYERStop  停止播放器
//@EXECUTIONFLOW:8、调用CSUDIPLAYERDelFilePlayerCallback，删除回调事件
//@EXECUTIONFLOW:9、提示测试人员观察，是否音音频停止播放
//@EXECUTIONFLOW:10    调用CSUDIPLAYERClose删除播放器
CSUDI_BOOL CSTC_FILE_TEST_IT_0009(void)
{
	FILE_TEST_IT_Play_Base(EM_UDI_FILENAME_MP3);

	return CSUDI_TRUE;
}

//@CASEGROUP:FILEPLAYER
//@DESCRIPTION:测试本地文件是否支持播放M4A格式文件
//@PRECONDITION:PLAYER模块，音视频解码器被正常初始化
//@INPUT:
//@EXECUTIONFLOW:1、调用CSUDIPLAYERProbe，期待返回CSUDI_SUCCESS
//@EXECUTIONFLOW:2、调用CSUDIPLAYEROpen，打开一个本地文件播放器
//@EXECUTIONFLOW:3、调用CSUDIPLAYERSetStream，设置播放器相应属性
//@EXECUTIONFLOW:4、调用CSUDIPLAYERAddFilePlayerCallback，添加文件开始播放的回调事件
//@EXECUTIONFLOW:5、调用CSUDIPLAYERStart  打开播放器
//@EXECUTIONFLOW:6、提示测试人员观察，是否有音视频播放出来
//@EXECUTIONFLOW:7、调用CSUDIPLAYERStop  停止播放器
//@EXECUTIONFLOW:8、调用CSUDIPLAYERDelFilePlayerCallback，删除回调事件
//@EXECUTIONFLOW:9、提示测试人员观察，是否音音频停止播放
//@EXECUTIONFLOW:10    调用CSUDIPLAYERClose删除播放器
CSUDI_BOOL CSTC_FILE_TEST_IT_0010(void)
{
	FILE_TEST_IT_Play_Base(EM_UDI_FILENAME_M4A);

	return CSUDI_TRUE;
}

//@CASEGROUP:FILEPLAYER
//@DESCRIPTION:测试本地文件是否支持播放AC3格式文件
//@PRECONDITION:PLAYER模块，音视频解码器被正常初始化
//@INPUT:
//@EXECUTIONFLOW:1、调用CSUDIPLAYERProbe，期待返回CSUDI_SUCCESS
//@EXECUTIONFLOW:2、调用CSUDIPLAYEROpen，打开一个本地文件播放器
//@EXECUTIONFLOW:3、调用CSUDIPLAYERSetStream，设置播放器相应属性
//@EXECUTIONFLOW:4、调用CSUDIPLAYERAddFilePlayerCallback，添加文件开始播放的回调事件
//@EXECUTIONFLOW:5、调用CSUDIPLAYERStart  打开播放器
//@EXECUTIONFLOW:6、提示测试人员观察，是否有音视频播放出来
//@EXECUTIONFLOW:7、调用CSUDIPLAYERStop  停止播放器
//@EXECUTIONFLOW:8、调用CSUDIPLAYERDelFilePlayerCallback，删除回调事件
//@EXECUTIONFLOW:9、提示测试人员观察，是否音音频停止播放
//@EXECUTIONFLOW:10    调用CSUDIPLAYERClose删除播放器
CSUDI_BOOL CSTC_FILE_TEST_IT_0011(void)
{
	FILE_TEST_IT_Play_Base(EM_UDI_FILENAME_AC3);

	return CSUDI_TRUE;
}

//@CASEGROUP:FILEPLAYER
//@DESCRIPTION:测试本地文件是否支持播放MP3格式文件
//@PRECONDITION:PLAYER模块，音视频解码器被正常初始化
//@INPUT:
//@EXECUTIONFLOW:1、调用CSUDIPLAYERProbe，期待返回CSUDI_SUCCESS
//@EXECUTIONFLOW:2、调用CSUDIPLAYEROpen，打开一个本地文件播放器
//@EXECUTIONFLOW:3、调用CSUDIPLAYERSetStream，设置播放器相应属性
//@EXECUTIONFLOW:4、调用CSUDIPLAYERAddFilePlayerCallback，添加文件开始播放的回调事件
//@EXECUTIONFLOW:5、调用CSUDIPLAYERStart  打开播放器
//@EXECUTIONFLOW:6、提示测试人员观察，是否有音视频播放出来
//@EXECUTIONFLOW:7、调用CSUDIPLAYERStop  停止播放器
//@EXECUTIONFLOW:8、调用CSUDIPLAYERDelFilePlayerCallback，删除回调事件
//@EXECUTIONFLOW:9、提示测试人员观察，是否音音频停止播放
//@EXECUTIONFLOW:10    调用CSUDIPLAYERClose删除播放器
CSUDI_BOOL CSTC_FILE_TEST_IT_0012(void)
{
	FILE_TEST_IT_Play_Base(EM_UDI_FILENAME_AAC);

	return CSUDI_TRUE;
}

//@CASEGROUP:FILEPLAYER
//@DESCRIPTION:测试本地文件是否支持播放MP4格式标清文件
//@PRECONDITION:PLAYER模块，音视频解码器被正常初始化
//@INPUT:
//@EXECUTIONFLOW:1、调用CSUDIPLAYERProbe，期待返回CSUDI_SUCCESS
//@EXECUTIONFLOW:2、调用CSUDIPLAYEROpen，打开一个本地文件播放器
//@EXECUTIONFLOW:3、调用CSUDIPLAYERSetStream，设置播放器相应属性
//@EXECUTIONFLOW:4、调用CSUDIPLAYERAddFilePlayerCallback，添加文件开始播放的回调事件
//@EXECUTIONFLOW:5、调用CSUDIPLAYERStart  打开播放器
//@EXECUTIONFLOW:6、提示测试人员观察，是否有音视频播放出来
//@EXECUTIONFLOW:7、调用CSUDIPLAYERStop  停止播放器
//@EXECUTIONFLOW:8、调用CSUDIPLAYERDelFilePlayerCallback，删除回调事件
//@EXECUTIONFLOW:9、提示测试人员观察，是否音视频停止播放
//@EXECUTIONFLOW:10    调用CSUDIPLAYERClose删除播放器
CSUDI_BOOL CSTC_FILE_TEST_IT_0013(void)
{
	FILE_TEST_IT_Play_Base(EM_UDI_FILENAME_MP4_SD);

	return CSUDI_TRUE;
}

//@CASEGROUP:FILEPLAYER
//@DESCRIPTION:测试本地文件是否支持播放MP4格式高清文件
//@PRECONDITION:PLAYER模块，音视频解码器被正常初始化
//@INPUT:
//@EXECUTIONFLOW:1、调用CSUDIPLAYERProbe，期待返回CSUDI_SUCCESS
//@EXECUTIONFLOW:2、调用CSUDIPLAYEROpen，打开一个本地文件播放器
//@EXECUTIONFLOW:3、调用CSUDIPLAYERSetStream，设置播放器相应属性
//@EXECUTIONFLOW:4、调用CSUDIPLAYERAddFilePlayerCallback，添加文件开始播放的回调事件
//@EXECUTIONFLOW:5、调用CSUDIPLAYERStart  打开播放器
//@EXECUTIONFLOW:6、提示测试人员观察，是否有音视频播放出来
//@EXECUTIONFLOW:7、调用CSUDIPLAYERStop  停止播放器
//@EXECUTIONFLOW:8、调用CSUDIPLAYERDelFilePlayerCallback，删除回调事件
//@EXECUTIONFLOW:9、提示测试人员观察，是否音视频停止播放
//@EXECUTIONFLOW:10    调用CSUDIPLAYERClose删除播放器
CSUDI_BOOL CSTC_FILE_TEST_IT_0014(void)
{
	FILE_TEST_IT_Play_Base(EM_UDI_FILENAME_MP4_HD);

	return CSUDI_TRUE;
}

//@CASEGROUP:FILEPLAYER
//@DESCRIPTION:测试本地文件是否支持播放3GP格式文件
//@PRECONDITION:PLAYER模块，音视频解码器被正常初始化
//@INPUT:
//@EXECUTIONFLOW:1、调用CSUDIPLAYERProbe，期待返回CSUDI_SUCCESS
//@EXECUTIONFLOW:2、调用CSUDIPLAYEROpen，打开一个本地文件播放器
//@EXECUTIONFLOW:3、调用CSUDIPLAYERSetStream，设置播放器相应属性
//@EXECUTIONFLOW:4、调用CSUDIPLAYERAddFilePlayerCallback，添加文件开始播放的事件
//@EXECUTIONFLOW:5、调用CSUDIPLAYERStart  打开播放器
//@EXECUTIONFLOW:6、提示测试人员观察，是否有音视频播放出来
//@EXECUTIONFLOW:7、调用CSUDIPLAYERStop  停止播放器
//@EXECUTIONFLOW:8、调用CSUDIPLAYERDelFilePlayerCallback，删除回调事件
//@EXECUTIONFLOW:9、提示测试人员观察，是否音视频停止播放
//@EXECUTIONFLOW:10    调用CSUDIPLAYERClose删除播放器
CSUDI_BOOL CSTC_FILE_TEST_IT_0015(void)
{
	FILE_TEST_IT_Play_Base(EM_UDI_FILENAME_3GP);

	return CSUDI_TRUE;
}

//@CASEGROUP:FILEPLAYER
//@DESCRIPTION:测试本地文件是否支持播放MOV格式文件
//@PRECONDITION:PLAYER模块，音视频解码器被正常初始化
//@INPUT:
//@EXECUTIONFLOW:1、调用CSUDIPLAYERProbe，期待返回CSUDI_SUCCESS
//@EXECUTIONFLOW:2、调用CSUDIPLAYEROpen，打开一个本地文件播放器
//@EXECUTIONFLOW:3、调用CSUDIPLAYERSetStream，设置播放器相应属性
//@EXECUTIONFLOW:4、调用CSUDIPLAYERAddFilePlayerCallback，添加文件开始播放的事件
//@EXECUTIONFLOW:5、调用CSUDIPLAYERStart  打开播放器
//@EXECUTIONFLOW:6、提示测试人员观察，是否有音视频播放出来
//@EXECUTIONFLOW:7、调用CSUDIPLAYERStop  停止播放器
//@EXECUTIONFLOW:8、调用CSUDIPLAYERDelFilePlayerCallback，删除回调事件
//@EXECUTIONFLOW:9、提示测试人员观察，是否音视频停止播放
//@EXECUTIONFLOW:10    调用CSUDIPLAYERClose删除播放器
CSUDI_BOOL CSTC_FILE_TEST_IT_0016(void)
{
	FILE_TEST_IT_Play_Base(EM_UDI_FILENAME_MOV);

	return CSUDI_TRUE;
}

//@CASEGROUP:FILEPLAYER
//@DESCRIPTION:测试本地文件是否支持播放MKV格式标清文件
//@PRECONDITION:PLAYER模块，音视频解码器被正常初始化
//@INPUT:
//@EXECUTIONFLOW:1、调用CSUDIPLAYERProbe，期待返回CSUDI_SUCCESS
//@EXECUTIONFLOW:2、调用CSUDIPLAYEROpen，打开一个本地文件播放器
//@EXECUTIONFLOW:3、调用CSUDIPLAYERSetStream，设置播放器相应属性
//@EXECUTIONFLOW:4、调用CSUDIPLAYERAddFilePlayerCallback，添加文件开始播放的事件
//@EXECUTIONFLOW:5、调用CSUDIPLAYERStart  打开播放器
//@EXECUTIONFLOW:6、提示测试人员观察，是否有音视频播放出来
//@EXECUTIONFLOW:7、调用CSUDIPLAYERStop  停止播放器
//@EXECUTIONFLOW:8、调用CSUDIPLAYERDelFilePlayerCallback，删除回调事件
//@EXECUTIONFLOW:9、提示测试人员观察，是否音视频停止播放
//@EXECUTIONFLOW:10    调用CSUDIPLAYERClose删除播放器
CSUDI_BOOL CSTC_FILE_TEST_IT_0017(void)
{
	FILE_TEST_IT_Play_Base(EM_UDI_FILENAME_MKV_SD);

	return CSUDI_TRUE;
}

//@CASEGROUP:FILEPLAYER
//@DESCRIPTION:测试本地文件是否支持播放MKV格式高清文件
//@PRECONDITION:PLAYER模块，音视频解码器被正常初始化
//@INPUT:
//@EXECUTIONFLOW:1、调用CSUDIPLAYERProbe，期待返回CSUDI_SUCCESS
//@EXECUTIONFLOW:2、调用CSUDIPLAYEROpen，打开一个本地文件播放器
//@EXECUTIONFLOW:3、调用CSUDIPLAYERSetStream，设置播放器相应属性
//@EXECUTIONFLOW:4、调用CSUDIPLAYERAddFilePlayerCallback，添加文件开始播放的事件
//@EXECUTIONFLOW:5、调用CSUDIPLAYERStart  打开播放器
//@EXECUTIONFLOW:6、提示测试人员观察，是否有音视频播放出来
//@EXECUTIONFLOW:7、调用CSUDIPLAYERStop  停止播放器
//@EXECUTIONFLOW:8、调用CSUDIPLAYERDelFilePlayerCallback，删除回调事件
//@EXECUTIONFLOW:9、提示测试人员观察，是否音视频停止播放
//@EXECUTIONFLOW:10    调用CSUDIPLAYERClose删除播放器
CSUDI_BOOL CSTC_FILE_TEST_IT_0018(void)
{
	FILE_TEST_IT_Play_Base(EM_UDI_FILENAME_MKV_HD);

	return CSUDI_TRUE;
}

//@CASEGROUP:FILEPLAYER
//@DESCRIPTION:测试本地文件是否支持播放MKV格式高清大文件，大小在10GB以上
//@PRECONDITION:PLAYER模块，音视频解码器被正常初始化
//@INPUT:
//@EXECUTIONFLOW:1、调用CSUDIPLAYERProbe，期待返回CSUDI_SUCCESS
//@EXECUTIONFLOW:2、调用CSUDIPLAYEROpen，打开一个本地文件播放器
//@EXECUTIONFLOW:3、调用CSUDIPLAYERSetStream，设置播放器相应属性
//@EXECUTIONFLOW:4、调用CSUDIPLAYERAddFilePlayerCallback，添加文件开始播放的事件
//@EXECUTIONFLOW:5、调用CSUDIPLAYERStart  打开播放器
//@EXECUTIONFLOW:6、提示测试人员观察，是否有音视频播放出来
//@EXECUTIONFLOW:7、调用CSUDIPLAYERStop  停止播放器
//@EXECUTIONFLOW:8、调用CSUDIPLAYERDelFilePlayerCallback，删除回调事件
//@EXECUTIONFLOW:9、提示测试人员观察，是否音视频停止播放
//@EXECUTIONFLOW:10    调用CSUDIPLAYERClose删除播放器
CSUDI_BOOL CSTC_FILE_TEST_IT_0019(void)
{
	CSTCPrint("本用例用于测试播放大文件，请确保测试数据大于10GB!\n");
	FILE_TEST_IT_Play_Base(EM_UDI_FILENAME_MKV_HD_BSIZE);

	return CSUDI_TRUE;
}

//@CASEGROUP:FILEPLAYER
//@DESCRIPTION:测试本地文件播放是否支持不同文件之间切换
//@PRECONDITION:PLAYER模块，音视频解码器被正常初始化
//@INPUT:
//@EXECUTIONFLOW:1、调用CSUDIPLAYEROpen，打开一个本地文件播放器
//@EXECUTIONFLOW:2、调用CSUDIPLAYERSetStream，设置播放器相应属性
//@EXECUTIONFLOW:3、调用CSUDIPLAYERAddFilePlayerCallback，添加音频资源改变的回调事件
//@EXECUTIONFLOW:4、调用CSUDIPLAYERStart  打开播放器
//@EXECUTIONFLOW:5、提示测试人员观察，是否有音视频播放出来
//@EXECUTIONFLOW:6、调用CSUDIPLAYERStop  停止播放器
//@EXECUTIONFLOW:7、再次调用CSUDIPLAYERSetStream，设置新文件的播放器相应属性
//@EXECUTIONFLOW:8、调用CSUDIPLAYERStart  打开播放器
//@EXECUTIONFLOW:9、提示测试人员观察，音视频是否开始播放
//@EXECUTIONFLOW:10、调用CSUDIPLAYERStop  停止播放器
//@EXECUTIONFLOW:11、提示测试人员观察，音视频是否停止播放
//@EXECUTIONFLOW:12、调用CSUDIPLAYERDelFilePlayerCallback，删除回调事件
//@EXECUTIONFLOW:13、重复步骤2-步骤10
//@EXECUTIONFLOW:14    调用CSUDIPLAYERClose删除播放器
CSUDI_BOOL CSTC_FILE_TEST_IT_0020(void)
{
	CSUDI_HANDLE   hPlayer=CSUDI_NULL;
	CSUDIPlaybackParam_S stPlaybackParam;
	int i = 0;
	CSUDIPLAYERType_E ePlayerType = EM_UDIPLAYER_FILE;

	memset(&stPlaybackParam,0,sizeof(stPlaybackParam));

	CSTCPrint("本用例测试本地文件播放不同文件之间切换\n");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYEROpen (NULL,ePlayerType,&hPlayer), "步骤1失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_NULL  != hPlayer, "步骤1失败");

	for(i = 0; i < REPEAT_TIMES; i++)
	{
		stPlaybackParam.m_eSpeed = EM_UDIPLAYER_SPEED_NORMAL;
		stPlaybackParam.m_nSecondPos = 0;
		strncpy(stPlaybackParam.m_szFileName, stFilePath[EM_UDI_FILENAME_MKV_SD].filename, CSUDI_MAX_FILE_NAME_LEN);

		CSTCPrint("第 %d 次测试 \"%s\" 的播放\n", i+1, stPlaybackParam.m_szFileName);

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERSetStream (hPlayer, NULL, 0, &stPlaybackParam), "步骤2设置流信息失败");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERAddFilePlayerCallback(hPlayer, FilePlayCallback,  CSUDI_NULL), "步骤3添加回调失败");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERStart(hPlayer), "步骤4失败");

		CSUDIOSThreadSleep(SLEEP_TIME);

		CSTCPrint("音视频是否能够正常播放？\n");
		CSTCPrint("Is A/V output normal ?\n");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes(),"步骤5失败：播放不正常");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERStop(hPlayer), "步骤6失败");

		CSUDIOSThreadSleep(SLEEP_TIME);

		CSTCPrint("音视频是否停止播放了\n");
  		CSTCPrint("Does the player stop ?\n");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes(),"步骤7失败：停止播放失败");

		stPlaybackParam.m_eSpeed = EM_UDIPLAYER_SPEED_NORMAL;
		stPlaybackParam.m_nSecondPos = 0;
		strncpy(stPlaybackParam.m_szFileName, stFilePath[EM_UDI_FILENAME_MP4_SD].filename, CSUDI_MAX_FILE_NAME_LEN);

		CSTCPrint("第 %d 次测试 \"%s\" 的播放\n", i+1, stPlaybackParam.m_szFileName);

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERSetStream (hPlayer, NULL, 0, &stPlaybackParam), "步骤7设置流信息失败");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERStart(hPlayer), "步骤8失败");

		CSTCPrint("音视频是否能够正常播放？\n");
		CSTCPrint("Is A/V output normal ?\n");

		CSUDIOSThreadSleep(SLEEP_TIME);

		CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes(),"步骤9失败：播放不正常");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERStop(hPlayer), "步骤10失败");

		CSTCPrint("音视频是否停止播放了\n");
  		CSTCPrint("Does the player stop ?\n");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes(),"步骤11失败：停止播放不正常");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERDelFilePlayerCallback(hPlayer, FilePlayCallback,  CSUDI_NULL), "步骤12删除回调失败");
	}

	CSTK_FATAL_POINT;

	if (hPlayer != CSUDI_NULL)
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERClose (hPlayer), "步骤14失败");
		hPlayer=CSUDI_NULL;
	}

	return CSUDI_TRUE;
}


//@CASEGROUP:FILEPLAYER
//@DESCRIPTION:测试本地文件播放与直播能够正常切换
//@PRECONDITION:PLAYER模块，音视频解码器被正常初始化
//@INPUT:
//@EXECUTIONFLOW:1、调用CSUDIPLAYEROpen，打开一个本地文件播放器
//@EXECUTIONFLOW:2、调用CSUDIPLAYERSetStream，设置播放器相应属性
//@EXECUTIONFLOW:3、调用CSUDIPLAYERAddFilePlayerCallback，添加视频资源改变的回调事件
//@EXECUTIONFLOW:4、调用CSUDIPLAYERStart  打开播放器
//@EXECUTIONFLOW:5、提示测试人员观察，是否有音视频播放出来
//@EXECUTIONFLOW:6、调用CSUDIPLAYERStop  停止播放器
//@EXECUTIONFLOW:7、调用CSUDIPLAYERDelFilePlayerCallback，删除回调事件
//@EXECUTIONFLOW:8、提示测试人员观察，是否音视频停止播放
//@EXECUTIONFLOW:9    调用CSUDIPLAYERClose删除播放器
//@EXECUTIONFLOW:10、调用CSUDIPLAYEROpen，打开一个直播播放器
//@EXECUTIONFLOW:11、调用CSUDIPLAYERSetStream，设置播放器相应属性
//@EXECUTIONFLOW:12、调用CSUDIPLAYERAddFilePlayerCallback，添加视频资源改变的回调事件
//@EXECUTIONFLOW:13、调用CSUDIPLAYERStart  打开播放器
//@EXECUTIONFLOW:14、提示测试人员观察，是否有音视频播放出来
//@EXECUTIONFLOW:15、调用CSUDIPLAYERStop  停止播放器
//@EXECUTIONFLOW:16、调用CSUDIPLAYERDelFilePlayerCallback，删除回调事件
//@EXECUTIONFLOW:17、提示测试人员观察，是否音视频停止播放
//@EXECUTIONFLOW:18    调用CSUDIPLAYERClose删除播放器
//@EXECUTIONFLOW:19、重复步骤1-步骤18指定次数
//@EXECUTIONFLOW:20、恢复现场
CSUDI_BOOL CSTC_FILE_TEST_IT_0021(void)
{
	CSUDI_HANDLE   hPlayer = CSUDI_NULL;
	CSUDI_HANDLE   hLivePlayer = CSUDI_NULL;
	CSUDIPlayerChnl_S stPlayerChnl;
	CSUDIStreamInfo_S  sStreamInfo[3];  //不会超过10个
	int nStreamCnt = 0;
	CSUDIPlaybackParam_S stPlaybackParam;
	int i = 0;
	CSUDIPLAYERType_E ePlayerType = EM_UDIPLAYER_FILE;

	CSTCPrint("本用例测试本地文件播放与直播的切换\n");

	for(i = 0; i < REPEAT_TIMES; i++)
	{
		memset(&stPlaybackParam,0,sizeof(stPlaybackParam));

		ePlayerType = EM_UDIPLAYER_FILE;

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYEROpen (NULL,ePlayerType,&hPlayer), "步骤1失败");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_NULL  != hPlayer, "步骤1失败");

		stPlaybackParam.m_eSpeed = EM_UDIPLAYER_SPEED_NORMAL;
		stPlaybackParam.m_nSecondPos = 0;

		strncpy(stPlaybackParam.m_szFileName, stFilePath[EM_UDI_FILENAME_MKV_SD].filename, CSUDI_MAX_FILE_NAME_LEN);

		CSTCPrint("第 %d 次测试 \"%s\" 的文件播放\n", i+1, stPlaybackParam.m_szFileName);

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERSetStream (hPlayer, NULL, 0, &stPlaybackParam), "步骤2设置流信息失败");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERAddFilePlayerCallback(hPlayer, FilePlayCallback, CSUDI_NULL), "步骤3添加回调失败");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERStart(hPlayer), "步骤4失败");

		CSUDIOSThreadSleep(SLEEP_TIME);

		CSTCPrint("音视频是否能够正常播放？\n");
		CSTCPrint("Is A/V output normal ?\n");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes(),"步骤5失败：播放不正常");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERStop(hPlayer), "步骤6失败");

		CSTCPrint("音视频是否停止播放了\n");
  		CSTCPrint("Does the player stop ?\n");

  		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERDelFilePlayerCallback(hPlayer, FilePlayCallback,  CSUDI_NULL), "步骤7删除回调失败");

  		CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes(),"步骤8失败：停止播放不正常");

  		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERClose (hPlayer), "步骤9失败");

  		hPlayer=CSUDI_NULL;

		CSUDIOSThreadSleep(SLEEP_TIME);

		ePlayerType = EM_UDIPLAYER_LIVE;

		stPlayerChnl.m_nDemux = 0;
		stPlayerChnl.m_nAudioDecoder = 0;
		stPlayerChnl.m_nVideoDecoder = 0;

		CSTCPrint("第 %d 次测试 \"%s\" 的直播播放\n", i+1, g_sPlayer_SeviceInfo[LivePlayer].m_pcTsFilename);

		CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE== PLAYER_iLockTuner (), "锁频失败");

		CSUDIOSThreadSleep(SLEEP_TIME);

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYEROpen (&stPlayerChnl,ePlayerType,&hLivePlayer), "步骤10失败");

		sStreamInfo[0].m_nPid = g_sPlayer_SeviceInfo[LivePlayer].m_nVideoPid;
		sStreamInfo[0].m_eContentType = EM_UDI_CONTENT_VIDEO;
		sStreamInfo[0].m_uStreamType.m_eVideoType = g_sPlayer_SeviceInfo[LivePlayer].m_eVidStreamType;

		sStreamInfo[1].m_nPid = g_sPlayer_SeviceInfo[LivePlayer].m_nAudioPid;
		sStreamInfo[1].m_eContentType = EM_UDI_CONTENT_AUDIO;
		sStreamInfo[1].m_uStreamType.m_eAudioType = g_sPlayer_SeviceInfo[LivePlayer].m_eAudStreamType;
		if (g_sPlayer_SeviceInfo[LivePlayer].m_nPcrPid <= 0)
		{
			nStreamCnt = 2;
		}
		else
		{
			sStreamInfo[2].m_nPid = g_sPlayer_SeviceInfo[LivePlayer].m_nPcrPid;
			sStreamInfo[2].m_eContentType = EM_UDI_CONTENT_PCR;
			nStreamCnt = 3;
		}

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERSetStream (hLivePlayer, sStreamInfo, nStreamCnt, NULL), "步骤11设置流信息失败");

        CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERAddPlayerCallback(hLivePlayer, PlayCallback , EM_UDIPLAYER_VIDEO_FRAME_COMING, CSUDI_NULL), "步骤12添加回调失败");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERStart(hLivePlayer), "步骤13失败");

		CSUDIOSThreadSleep(SLEEP_TIME);

		CSTCPrint("音视频是否能够正常播放？\n");
		CSTCPrint("Is A/V output normal ?\n");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes(),"步骤14失败：播放不正常");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERStop(hLivePlayer), "步骤15失败");
		CSTCPrint("音视频是否停止播放了\n");
  		CSTCPrint("Does the player stop ?\n");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes(),"步骤16失败：停止播放不正常");

        CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERDelPlayerCallback(hLivePlayer, PlayCallback , EM_UDIPLAYER_VIDEO_FRAME_COMING, CSUDI_NULL), "步骤17删除回调失败");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERClose (hLivePlayer), "步骤18失败");

		hLivePlayer=CSUDI_NULL;
	}

	CSTK_FATAL_POINT;

	if (hPlayer != CSUDI_NULL)
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERClose (hPlayer), "步骤20失败");
		hPlayer=CSUDI_NULL;
	}

	if (hLivePlayer != CSUDI_NULL)
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERClose (hLivePlayer), "步骤21失败");
		hLivePlayer=CSUDI_NULL;
	}

	return CSUDI_TRUE;
}

//@CASEGROUP:FILEPLAYER
//@DESCRIPTION:测试本地文件播放与注入播放能够正常切换
//@PRECONDITION:PLAYER模块，音视频解码器被正常初始化
//@INPUT:
//@EXECUTIONFLOW:1、调用CSUDIPLAYEROpen，打开一个本地文件播放器
//@EXECUTIONFLOW:2、调用CSUDIPLAYERSetStream，设置播放器相应属性
//@EXECUTIONFLOW:3、调用CSUDIPLAYERAddFilePlayerCallback，添加视频资源改变的回调事件
//@EXECUTIONFLOW:4、调用CSUDIPLAYERStart  打开播放器
//@EXECUTIONFLOW:5、提示测试人员观察，是否有音视频播放出来
//@EXECUTIONFLOW:6、调用CSUDIPLAYERStop  停止播放器
//@EXECUTIONFLOW:7、调用CSUDIPLAYERDelFilePlayerCallback，删除回调事件
//@EXECUTIONFLOW:8、提示测试人员观察，是否音视频停止播放
//@EXECUTIONFLOW:9    调用CSUDIPLAYERClose删除播放器
//@EXECUTIONFLOW:10、调用CSUDIINJECTEROpen，创建一个注入TS  码流的注入实例，期望返回CSUDI_SUCCESS
//@EXECUTIONFLOW:11、调用CSUDIPLAYEROpen ，创建一个注入的播放器，期望返回CSUDI_SUCCESS
//@EXECUTIONFLOW:12、调用CSUDIINJECTERSetAttribute ，设置"同步"属性
//@EXECUTIONFLOW:13、调用CSUDIPLAYERSetStream ，设置好TS码流信息，期望返回CSUDI_SUCCESS
//@EXECUTIONFLOW:14、调用CSUDIINJECTERStart，开始数据注入，期望返回CSUDI_SUCCESS
//@EXECUTIONFLOW:15、创建一个线程,循环读取TS码流文件，并进行TS注入操作
//@EXECUTIONFLOW:16、调用CSUDIPLAYERStart ，开始播放数据，期望能够正常播放音视频
//@EXECUTIONFLOW:17、提示用户确认，音视频节目内容能不能播放出来
//@EXECUTIONFLOW:18、通知注入任务停止注入数据，等待注入任务正常结束返回
//@EXECUTIONFLOW:19      调用CSUDIINJECTERClear清除Inecter已经注入到目标缓存区中的尚没有播放完的全部数据
//@EXECUTIONFLOW:20、调用CSUDIPLAYERStop ，停止播放器，期望返回CSUDI_SUCCESS
//@EXECUTIONFLOW:21、调用CSUDIINJECTERStop ，停止注入器，期望返回CSUDI_SUCCESS
//@EXECUTIONFLOW:22、调用CSUDIPLAYERClose ，关闭播放器，期望返回CSUDI_SUCCESS
//@EXECUTIONFLOW:23、调用CSUDIINJECTERClose ，关闭注入器，期望返回CSUDI_SUCCESS
//@EXECUTIONFLOW:24、调用CSUDIOSThreadDestroy ，删除注入任务，期望返回CSUDI_SUCCESS
//@EXECUTIONFLOW:25、重复步骤1-步骤24
//@EXECUTIONFLOW:26、恢复现场
CSUDI_BOOL CSTC_FILE_TEST_IT_0022(void)
{
	CSUDI_HANDLE   hPlayer = CSUDI_NULL;
	CSUDI_HANDLE   hInjectPlayer = CSUDI_NULL;
	CSUDI_HANDLE   hInject= CSUDI_NULL;
	CSUDI_HANDLE hInjectThread = CSUDI_NULL;
	CSUDIPESSYNCMode_S 	stSyncMode;
	CSUDIINJECTEROpenParam_S  stOpenParams;
	CSUDIINJECTERChnl_S stInjecterChnl;
	CSUDIPlayerChnl_S stPlayerChnl;
	CSUDIStreamInfo_S  stStreamInfo[3];  //不会超过10个
	int nStreamCnt = 0;
	CSUDIPlaybackParam_S stPlaybackParam;
	int i = 0;
	CSUDIPLAYERType_E ePlayerType = EM_UDIPLAYER_FILE;

	memset(&stInjecterChnl,-1,sizeof(stInjecterChnl));
	memset(&stPlayerChnl,-1,sizeof(stPlayerChnl));
	memset(stStreamInfo,0,sizeof(stStreamInfo));
	memset(&stOpenParams,0,sizeof(stOpenParams));

	stSyncMode.m_eSyncMode = EM_UDIINJECTER_SYNC_AUDIO_FIRST;
	stSyncMode.m_hSyncHandle = CSUDI_NULL;

	CSTCPrint("本用例测试本地文件播放与直播的切换\n");

	nStreamCnt = getTSInjectInfo(&stInjecterChnl,&stPlayerChnl,&stOpenParams,stStreamInfo);

	CSTK_ASSERT_TRUE_FATAL(-1 != nStreamCnt, "获取注入TS 流信息失败")

	for(i = 0; i < REPEAT_TIMES; i++)
	{
		memset(&stPlaybackParam,0,sizeof(stPlaybackParam));

		ePlayerType = EM_UDIPLAYER_FILE;

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYEROpen (NULL,ePlayerType,&hPlayer), "步骤1失败");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_NULL  != hPlayer, "步骤1失败");

		stPlaybackParam.m_eSpeed = EM_UDIPLAYER_SPEED_NORMAL;
		stPlaybackParam.m_nSecondPos = 0;

		strncpy(stPlaybackParam.m_szFileName, stFilePath[EM_UDI_FILENAME_MKV_HD].filename, CSUDI_MAX_FILE_NAME_LEN);

		CSTCPrint("第 %d 次测试 \"%s\" 的文件播放\n", i+1, stPlaybackParam.m_szFileName);

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERSetStream (hPlayer, NULL, 0, &stPlaybackParam), "步骤2设置流信息失败");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERAddFilePlayerCallback(hPlayer, FilePlayCallback, CSUDI_NULL), "步骤3添加回调失败");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERStart(hPlayer), "步骤4失败");

	    	CSUDIOSThreadSleep(SLEEP_TIME);

		CSTCPrint("音视频是否能够正常播放？\n");
	    	CSTCPrint("Is A/V output normal ?\n");

	    	CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes(),"步骤5失败：播放不正常");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERStop(hPlayer), "步骤6失败");
    		CSTCPrint("音视频是否停止播放了\n");
  		CSTCPrint("Does the player stop ?\n");

  		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERDelFilePlayerCallback(hPlayer, FilePlayCallback, CSUDI_NULL), "步骤7删除回调失败");

  		CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes(),"步骤8失败：播放不正常");

  		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERClose (hPlayer), "步骤9失败");

  		hPlayer=CSUDI_NULL;

		CSUDIOSThreadSleep(SLEEP_TIME);

		CSTCPrint("第 %d 次测试 \"%s\" 的注入播放\n", i+1, g_TS_StreamInfo[LivePlayer].m_pcName);

		ePlayerType = EM_UDIPLAYER_INJECT;

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIINJECTEROpen (&stInjecterChnl,&stOpenParams,&hInject), "步骤10失败");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYEROpen (&stPlayerChnl,ePlayerType,&hInjectPlayer), "步骤11失败");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIINJECTERSetAttribute(hInject,EM_UDIINJECTER_PES_SYNC,&stSyncMode), "步骤12失败");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERSetStream (hInjectPlayer, stStreamInfo, nStreamCnt, NULL), "步骤13设置流信息失败");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIINJECTERStart (hInject), "步骤14失败");

		g_TS_StreamInfo[LivePlayer].m_hInjecter = hInject;
		g_TS_StreamInfo[LivePlayer].m_bRun = CSUDI_FALSE;

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIOSThreadCreate("TS Inject", INJECTTHREADPRO, INJECTTHREADBUF, TS_injectTask, ( void* ) &g_TS_StreamInfo[LivePlayer], &hInjectThread), "步骤15失败");

		while(g_TS_StreamInfo[LivePlayer].m_bRun == CSUDI_FALSE)
		{
			CSUDIOSThreadSleep(500);
		}

		 CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == g_TS_StreamInfo[LivePlayer].m_bOpenFileSuccess, "文件打开失败");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERStart(hInjectPlayer), "步骤16失败");

	    	CSUDIOSThreadSleep(SLEEP_TIME);

		CSTCPrint("音视频是否能够正常播放？\n");
	    	CSTCPrint("Is A/V output normal ?\n");

	    	CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes(),"步骤17失败：播放不正常");

	    	g_TS_StreamInfo[LivePlayer].m_bRun = CSUDI_FALSE;

	    	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIOSThreadJoin(hInjectThread),"步骤18失败");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIINJECTERClear(hInject),"步骤19失败");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERStop(hInjectPlayer), "步骤20失败");
    		CSTCPrint("音视频是否停止播放了\n");
  		CSTCPrint("Does the player stop ?\n");


		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIINJECTERStop(hInject), "步骤21失败");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERClose (hInjectPlayer), "步骤22失败");

		hInjectPlayer = CSUDI_NULL;

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIINJECTERClose (hInject), "步骤23失败");

		hInject = CSUDI_NULL;

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIOSThreadDestroy(hInjectThread), "步骤24失败");

		hInjectThread = CSUDI_NULL;
	}

	CSTK_FATAL_POINT;

	if (hPlayer != CSUDI_NULL)
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERClose (hPlayer), "步骤26失败");
		hPlayer=CSUDI_NULL;
	}

	if (hInjectPlayer != CSUDI_NULL)
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERClose (hInjectPlayer), "步骤26失败");
		hInjectPlayer=CSUDI_NULL;
	}

	if (hInject != CSUDI_NULL)
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIINJECTERClose (hInject), "步骤26失败");
		hInject = CSUDI_NULL;
	}

	if(CSUDI_NULL != hInjectThread)
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIOSThreadDestroy(hInjectThread), "步骤26失败");
		hInjectThread = CSUDI_NULL;
	}

	return CSUDI_TRUE;
}


static CSUDI_BOOL FILE_TEST_IT_PlayToEnd_Base(int formatType)
{
	CSUDIPlayerFileInfo_S stFileInfo;
	unsigned char ucPlayToend = 0;
	CSUDI_HANDLE hPlayer = CSUDI_NULL;
	CSUDI_Error_Code udiRe = CSUDI_SUCCESS;
	CSUDIPLAYERType_E ePlayerType = EM_UDIPLAYER_FILE;
	CSUDIPlaybackParam_S stPlaybackParam;
	CSUDIOSTimeVal_S sStartTime;
	CSUDIOSTimeVal_S sEndTime;
	int nSpanTime = 0;
	int nPosInSec = 0;
	CSUDI_BOOL bPlaying = CSUDI_FALSE;
	CSUDI_BOOL bAddCallback = CSUDI_FALSE;

	memset(&stPlaybackParam, 0, sizeof(stPlaybackParam));
	stPlaybackParam.m_eSpeed = EM_UDIPLAYER_SPEED_NORMAL;
	stPlaybackParam.m_nSecondPos = 0;

	memset(&stFileInfo, 0, sizeof(CSUDIPlayerFileInfo_S));

	strncpy(stPlaybackParam.m_szFileName, stFilePath[formatType].filename, CSUDI_MAX_FILE_NAME_LEN);

	CSTCPrint("本用例测试 \"%s\" 的播放(播放到末尾)\n", stPlaybackParam.m_szFileName);

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == (udiRe = CSUDIPLAYERProbe(stPlaybackParam.m_szFileName)), "不支持的格式");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYEROpen(NULL, ePlayerType, &hPlayer), "创建播放句柄失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_NULL != hPlayer, "创建的播放句柄为空");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERSetStream(hPlayer, NULL, 0, &stPlaybackParam), "设置流信息失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERAddFilePlayerCallback(hPlayer, FilePlayCallback, (void *)&ucPlayToend), "注册回调失败");

	bAddCallback = CSUDI_TRUE;
	
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERStart(hPlayer), "播放文件失败");
	
	bPlaying = CSUDI_TRUE;

	// 获取开始播放的时间
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIOSGetTime(&sStartTime), "获取开始时间失败");
	
	CSUDIOSThreadSleep(SLEEP_TIME);

	// 清除播放状态标记
	ucPlayToend = 0;

	CSTCPrint("音视频是否能正常播放？\n");
	CSTCPrint("Is A/V output normal? \n");
	
	CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes(), "音视频播放失败");

	CSTCPrint("正在播放...\n");
	
	// 等待播放完成
	while (PLAY_TO_END != ucPlayToend)
	{
		CSUDIOSThreadSleep(50);
	}

	bPlaying = CSUDI_FALSE;
	
	CSTCPrint("播放完毕\n");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIOSGetTime(&sEndTime), "获取结束时间失败");

	nSpanTime = sEndTime.m_nSecond - sStartTime.m_nSecond;

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERGetDuration(hPlayer, &nPosInSec), "获取节目总长度失败");

	// 误差2s以内
	CSTK_ASSERT_TRUE_FATAL(((nPosInSec>nSpanTime) ? (2>=(nPosInSec-nSpanTime)) : (2>=(nSpanTime-nPosInSec))), "播放时长异常");

	CSTK_FATAL_POINT;

	if (bPlaying)
	{
		bPlaying = CSUDI_FALSE;
		
		CSTK_ASSERT_TRUE(CSUDI_SUCCESS == CSUDIPLAYERStop(hPlayer), "停止播放失败");
	}

	if (bAddCallback)
	{
		bAddCallback = CSUDI_FALSE;
		
		CSTK_ASSERT_TRUE(CSUDI_SUCCESS == CSUDIPLAYERDelFilePlayerCallback(hPlayer, FilePlayCallback, (void *)&ucPlayToend), "删除回调失败");	
	}

	if (CSUDI_NULL != hPlayer)
	{
		CSTK_ASSERT_TRUE(CSUDI_SUCCESS == CSUDIPLAYERClose(hPlayer), "恢复现场");

		hPlayer = CSUDI_NULL;
	}

	return CSUDI_TRUE;
}

//@CASEGROUP:FILEPLAYER
//@DESCRIPTION:测试本地文件是否能完整正常播放MP3音频
//@PRECONDITION:PLAYER模块，音视频解码器被正常初始化
//@INPUT: void
//@EXPECTATION: 期望播放正常
//@REMARK:
//@EXECUTIONFLOW:1、调用CSUDIPLAYERProbe，期待返回CSUDI_SUCCESS
//@EXECUTIONFLOW:2、调用CSUDIPLAYEROpen，打开一个本地文件播放器
//@EXECUTIONFLOW:3、调用CSUDIPLAYERSetStream，设置播放器相应属性
//@EXECUTIONFLOW:4、调用CSUDIPLAYERAddFilePlayerCallback，添加文件开始播放的回调事件
//@EXECUTIONFLOW:5、调用CSUDIPLAYERStart  打开播放器
//@EXECUTIONFLOW:6、提示测试人员观察，是否有音视频播放出来
//@EXECUTIONFLOW:7、等待播放完毕
//@EXECUTIONFLOW:8、判断播放总时长与实际播放时间是否相等
//@EXECUTIONFLOW:9、调用CSUDIPLAYERDelFilePlayerCallback，删除文件开始播放的回调事件
//@EXECUTIONFLOW:10、调用CSUDIPLAYERClose删除播放器
CSUDI_BOOL CSTC_FILE_TEST_IT_PlayToEnd_0001(void)
{
	FILE_TEST_IT_PlayToEnd_Base(EM_UDI_FILENAME_MP3);

	return CSUDI_TRUE;
}

//@CASEGROUP:FILEPLAYER
//@DESCRIPTION:测试本地文件是否能完整正常播放大容量MKV视频
//@PRECONDITION:PLAYER模块，音视频解码器被正常初始化
//@INPUT: void
//@EXPECTATION: 期望播放正常
//@REMARK:
//@EXECUTIONFLOW:1、调用CSUDIPLAYERProbe，期待返回CSUDI_SUCCESS
//@EXECUTIONFLOW:2、调用CSUDIPLAYEROpen，打开一个本地文件播放器
//@EXECUTIONFLOW:3、调用CSUDIPLAYERSetStream，设置播放器相应属性
//@EXECUTIONFLOW:4、调用CSUDIPLAYERAddFilePlayerCallback，添加文件开始播放的回调事件
//@EXECUTIONFLOW:5、调用CSUDIPLAYERStart  打开播放器
//@EXECUTIONFLOW:6、提示测试人员观察，是否有音视频播放出来
//@EXECUTIONFLOW:7、等待播放完毕
//@EXECUTIONFLOW:8、判断播放总时长与实际播放时间是否相等
//@EXECUTIONFLOW:9、调用CSUDIPLAYERDelFilePlayerCallback，删除文件开始播放的回调事件
//@EXECUTIONFLOW:10、调用CSUDIPLAYERClose删除播放器
CSUDI_BOOL CSTC_FILE_TEST_IT_PlayToEnd_0002(void)
{
	FILE_TEST_IT_PlayToEnd_Base(EM_UDI_FILENAME_MKV_HD_BSIZE);

	return CSUDI_TRUE;
}

//@CASEGROUP:FILEPLAYER CALLBACK
//@DESCRIPTION:测试本地文件非法文件路径回调的返回值
//@PRECONDITION:PLAYER模块正常初始化
//@INPUT:
//@EXECUTIONFLOW:1、调用CSUDIPLAYEROpen，打开一个本地文件播放器
//@EXECUTIONFLOW:2、调用CSUDIPLAYERAddPlayerCallback，添加文件开始播放的回调事件
//@EXECUTIONFLOW:3、调用CSUDIPLAYERSetStream，设置播放器相应属性，FileName为非法路径"./testdata/error.ts"
//@EXECUTIONFLOW:4、调用CSUDIPLAYERStart  打开播放器
//@EXECUTIONFLOW:5、等待3秒，期望有EM_UDIFILEPLAYER_ERR_FORMAT的回调事件到来
//@EXECUTIONFLOW:6、调用CSUDIPLAYERStop  停止播放器
//@EXECUTIONFLOW:7、调用CSUDIPLAYERDelPlayerCallback，删除文件开始播放的回调事件
//@EXECUTIONFLOW:8、调用CSUDIPLAYERClose删除播放器
CSUDI_BOOL CSTC_FILE_TEST_CALLBACK_IT_0001(void)
{
	CSUDI_HANDLE   hPlayer=CSUDI_NULL;
	CSUDIPlaybackParam_S stPlaybackParam;
	CSUDIPLAYERType_E	ePlayerType=EM_UDIPLAYER_FILE;

	memset(&stPlaybackParam,0,sizeof(stPlaybackParam));

	stPlaybackParam.m_eSpeed = EM_UDIPLAYER_SPEED_NORMAL;
	stPlaybackParam.m_nSecondPos = 0;
	strncpy(stPlaybackParam.m_szFileName, "./testdata/error.ts", CSUDI_MAX_FILE_NAME_LEN);

	s_ePlayerEvent = EM_UDIFILEPLAYER_MAXEVENTTYPE;

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYEROpen (NULL,ePlayerType,&hPlayer), "步骤1失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_NULL  != hPlayer, "步骤1失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERAddFilePlayerCallback(hPlayer, FilePlayEventCallback, CSUDI_NULL), "步骤2添加回调失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERSetStream (hPlayer, NULL, 0, &stPlaybackParam), "步骤3设置流信息失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERStart(hPlayer), "步骤4失败");

	CSTCPrint("请等待2秒\n");

	CSUDIOSThreadSleep(SLEEP_TIME);

	CSTK_ASSERT_TRUE_FATAL(EM_UDIFILEPLAYER_ERR_FORMAT == s_ePlayerEvent,"步骤5失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERStop(hPlayer), "步骤6失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERDelFilePlayerCallback(hPlayer, FilePlayEventCallback, CSUDI_NULL), "步骤7删除回调失败");

	CSTK_FATAL_POINT;

	if (hPlayer != CSUDI_NULL)
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERClose (hPlayer), "步骤8失败");
		hPlayer = CSUDI_NULL;
	}

	return CSUDI_TRUE;
}

//@CASEGROUP:FILEPLAYER CALLBACK
//@DESCRIPTION:测试本地文件播放定位错误回调的返回值
//@PRECONDITION:PLAYER模块正常初始化
//@INPUT:
//@EXECUTIONFLOW:1、调用CSUDIPLAYEROpen，打开一个本地文件播放器
//@EXECUTIONFLOW:2、调用CSUDIPLAYERAddPlayerCallback，添加文件开始播放的回调事件
//@EXECUTIONFLOW:3、调用CSUDIPLAYERSetStream，设置播放器相应属性，FileName为非法路径"./testdata/error.ts"
//@EXECUTIONFLOW:4、调用CSUDIPLAYERStart  打开播放器
//@EXECUTIONFLOW:5、调用CSUDIPLAYERSeek  设置错误位置，为文件开头的-1
//@EXECUTIONFLOW:6、等待3秒，期望有EM_UDIFILEPLAYER_ERR_SEEK的回调事件到来
//@EXECUTIONFLOW:7、调用CSUDIPLAYERStop  停止播放器
//@EXECUTIONFLOW:8、调用CSUDIPLAYERDelPlayerCallback，删除文件开始播放的回调事件
//@EXECUTIONFLOW:9、 调用CSUDIPLAYERClose删除播放器
CSUDI_BOOL CSTC_FILE_TEST_CALLBACK_IT_0002(void)
{
	CSUDI_HANDLE   hPlayer=CSUDI_NULL;
	CSUDIPlaybackParam_S stPlaybackParam;
	CSUDIPLAYERType_E	ePlayerType=EM_UDIPLAYER_FILE;

	memset(&stPlaybackParam,0,sizeof(stPlaybackParam));

	stPlaybackParam.m_eSpeed = EM_UDIPLAYER_SPEED_NORMAL;
	stPlaybackParam.m_nSecondPos = 0;
	strncpy(stPlaybackParam.m_szFileName, stFilePath[EM_UDI_FILENAME_TS_SD].filename, CSUDI_MAX_FILE_NAME_LEN);

	s_ePlayerEvent = EM_UDIFILEPLAYER_MAXEVENTTYPE;

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYEROpen (NULL,ePlayerType,&hPlayer), "步骤1失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_NULL  != hPlayer, "步骤1失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERAddFilePlayerCallback(hPlayer, FilePlayEventCallback, CSUDI_NULL), "步骤2添加回调失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERSetStream (hPlayer, NULL, 0, &stPlaybackParam), "步骤3设置流信息失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERStart(hPlayer), "步骤4失败");

	CSUDIOSThreadSleep(SLEEP_TIME);

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS != CSUDIPLAYERSeek(hPlayer, -1, EM_UDIPLAYER_POSITION_FROM_HEAD), "步骤5失败");

	CSTCPrint("请等待2秒\n");

	CSUDIOSThreadSleep(SLEEP_TIME);

	CSTK_ASSERT_TRUE_FATAL(EM_UDIFILEPLAYER_ERR_SEEK == s_ePlayerEvent,"步骤6失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERStop(hPlayer), "步骤7失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERDelFilePlayerCallback(hPlayer, FilePlayEventCallback, CSUDI_NULL), "步骤8删除回调失败");

	CSTK_FATAL_POINT;

	if (hPlayer != CSUDI_NULL)
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERClose (hPlayer), "步骤9失败");
		hPlayer = CSUDI_NULL;
	}

	return CSUDI_TRUE;
}

//@CASEGROUP:FILEPLAYER CALLBACK
//@DESCRIPTION:测试本地文件设置错误速率回调的返回值
//@PRECONDITION:PLAYER模块正常初始化
//@INPUT:
//@EXECUTIONFLOW:1、调用CSUDIPLAYEROpen，打开一个本地文件播放器
//@EXECUTIONFLOW:2、调用CSUDIPLAYERAddPlayerCallback，添加文件开始播放的回调事件
//@EXECUTIONFLOW:3、调用CSUDIPLAYERSetStream，设置播放器相应属性
//@EXECUTIONFLOW:4、调用CSUDIPLAYERStart  打开播放器
//@EXECUTIONFLOW:5、调用CSUDIPLAYERSetSpeed  设置错误速率，分别为(EM_UDIPLAYER_SPEED_NORMAL-1)和(EM_UDIPLAYER_SPEED_MAX)
//@EXECUTIONFLOW:6、等待3秒，期望有EM_UDIFILEPLAYER_ERR_SETSPEED的回调事件到来
//@EXECUTIONFLOW:7、调用CSUDIPLAYERStop  停止播放器
//@EXECUTIONFLOW:8、调用CSUDIPLAYERDelPlayerCallback，删除文件开始播放的回调事件
//@EXECUTIONFLOW:9、调用CSUDIPLAYERClose删除播放器
CSUDI_BOOL CSTC_FILE_TEST_CALLBACK_IT_0003(void)
{
	CSUDI_HANDLE   hPlayer=CSUDI_NULL;
	CSUDIPlaybackParam_S stPlaybackParam;
	CSUDIPLAYERType_E	ePlayerType=EM_UDIPLAYER_FILE;

	memset(&stPlaybackParam,0,sizeof(stPlaybackParam));

	stPlaybackParam.m_eSpeed = EM_UDIPLAYER_SPEED_NORMAL;
	stPlaybackParam.m_nSecondPos = 0;
	strncpy(stPlaybackParam.m_szFileName, stFilePath[EM_UDI_FILENAME_TS_SD].filename, CSUDI_MAX_FILE_NAME_LEN);

	s_ePlayerEvent = EM_UDIFILEPLAYER_MAXEVENTTYPE;

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYEROpen (NULL,ePlayerType,&hPlayer), "步骤1失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_NULL  != hPlayer, "步骤1失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERAddFilePlayerCallback(hPlayer, FilePlayEventCallback, CSUDI_NULL), "步骤2添加回调失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERSetStream (hPlayer, NULL, 0, &stPlaybackParam), "步骤3设置流信息失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERStart(hPlayer), "步骤4失败");

	CSUDIOSThreadSleep(SLEEP_TIME);

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS != CSUDIPLAYERSetSpeed(hPlayer, EM_UDIPLAYER_SPEED_NORMAL-1), "步骤5失败");

	CSTCPrint("请等待2秒\n");

	CSUDIOSThreadSleep(SLEEP_TIME);

	CSTK_ASSERT_TRUE_FATAL(EM_UDIFILEPLAYER_ERR_SETSPEED == s_ePlayerEvent,"步骤6失败");

	s_ePlayerEvent = EM_UDIFILEPLAYER_MAXEVENTTYPE;

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS != CSUDIPLAYERSetSpeed(hPlayer, EM_UDIPLAYER_SPEED_MAX), "步骤5失败");

	CSTCPrint("请等待2秒\n");

	CSUDIOSThreadSleep(SLEEP_TIME);

	CSTK_ASSERT_TRUE_FATAL(EM_UDIFILEPLAYER_ERR_SETSPEED == s_ePlayerEvent,"步骤6失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERStop(hPlayer), "步骤7失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERDelFilePlayerCallback(hPlayer, FilePlayEventCallback, CSUDI_NULL), "步骤8删除回调失败");

	CSTK_FATAL_POINT;

	if (hPlayer != CSUDI_NULL)
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERClose (hPlayer), "步骤9失败");
		hPlayer = CSUDI_NULL;
	}

	return CSUDI_TRUE;
}

//@CASEGROUP:FILEPLAYER CALLBACK
//@DESCRIPTION:测试本地文件正常播放\选时\暂停\恢复\停止流程的回调事件
//@PRECONDITION:PLAYER模块正常初始化
//@INPUT:
//@EXECUTIONFLOW:1、调用CSUDIPLAYEROpen，打开一个本地文件播放器
//@EXECUTIONFLOW:2、调用CSUDIPLAYERAddPlayerCallback，添加文件开始播放的回调事件
//@EXECUTIONFLOW:3、调用CSUDIPLAYERSetStream，设置播放器相应属性
//@EXECUTIONFLOW:4、调用CSUDIPLAYERStart  打开播放器
//@EXECUTIONFLOW:5、期望3秒内有EM_UDIFILEPLAYER_STATE_START的回调事件到来
//@EXECUTIONFLOW:6、调用CSUDIPLAYERSeek 设置播放器位置
//@EXECUTIONFLOW:7、期望3秒内有EM_UDIFILEPLAYER_SEEK_FINISH的回调事件到来
//@EXECUTIONFLOW:8、调用CSUDIPLAYERPause 暂停播放器
//@EXECUTIONFLOW:9、期望3秒内有EM_UDIFILEPLAYER_STATE_PAUSE的回调事件到来
//@EXECUTIONFLOW:10、调用CSUDIPLAYERResume 恢复播放器
//@EXECUTIONFLOW:11、调用CSUDIPLAYERStop  停止播放器
//@EXECUTIONFLOW:12、期望3秒内有EM_UDIFILEPLAYER_STATE_STOP的回调事件到来
//@EXECUTIONFLOW:13、调用CSUDIPLAYERDelPlayerCallback，删除文件开始播放的回调事件
//@EXECUTIONFLOW:14、调用CSUDIPLAYERClose删除播放器
CSUDI_BOOL CSTC_FILE_TEST_CALLBACK_IT_0004(void)
{
	CSUDI_HANDLE   hPlayer=CSUDI_NULL;
	CSUDIPlaybackParam_S stPlaybackParam;
	CSUDIPLAYERType_E	ePlayerType=EM_UDIPLAYER_FILE;

	int i;

	memset(&stPlaybackParam,0,sizeof(stPlaybackParam));

	stPlaybackParam.m_eSpeed = EM_UDIPLAYER_SPEED_NORMAL;
	stPlaybackParam.m_nSecondPos = 0;
	strncpy(stPlaybackParam.m_szFileName, stFilePath[EM_UDI_FILENAME_TS_SD].filename, CSUDI_MAX_FILE_NAME_LEN);

	s_ePlayerEvent = EM_UDIFILEPLAYER_MAXEVENTTYPE;

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYEROpen (NULL,ePlayerType,&hPlayer), "步骤1失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_NULL != hPlayer, "步骤1失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERAddFilePlayerCallback(hPlayer, FilePlayEventCallback, CSUDI_NULL), "步骤2添加回调失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERSetStream (hPlayer, NULL, 0, &stPlaybackParam), "步骤3设置流信息失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERStart(hPlayer), "步骤4失败");

	for(i=0;i<MAX_SLEEP_NUM;i++)
	{
		CSUDIOSThreadSleep(100);
		if(EM_UDIFILEPLAYER_STATE_START == s_ePlayerEvent)
			break;
	}

	if(i==MAX_SLEEP_NUM)
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDI_FALSE, "步骤5失败");
	}

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERSeek(hPlayer, 1, EM_UDIPLAYER_POSITION_FROM_HEAD), "步骤6失败");

	for(i=0;i<MAX_SLEEP_NUM;i++)
	{
		CSUDIOSThreadSleep(100);
		if(EM_UDIFILEPLAYER_SEEK_FINISH == s_ePlayerEvent)
			break;
	}

	if(i==MAX_SLEEP_NUM)
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDI_FALSE, "步骤7失败");
	}

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERPause(hPlayer), "步骤8失败");

	for(i=0;i<MAX_SLEEP_NUM;i++)
	{
		CSUDIOSThreadSleep(100);
		if(EM_UDIFILEPLAYER_STATE_PAUSE == s_ePlayerEvent)
			break;
	}

	if(i==MAX_SLEEP_NUM)
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDI_FALSE, "步骤9失败");
	}

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERResume(hPlayer), "步骤10失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERStop(hPlayer), "步骤11失败");

	for(i=0;i<MAX_SLEEP_NUM;i++)
	{
		CSUDIOSThreadSleep(100);
		if(EM_UDIFILEPLAYER_STATE_STOP == s_ePlayerEvent)
			break;
	}

	if(i==MAX_SLEEP_NUM)
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDI_FALSE, "步骤12失败");
	}

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERDelFilePlayerCallback(hPlayer, FilePlayEventCallback, CSUDI_NULL), "步骤13删除回调失败");

	CSTK_FATAL_POINT;

	if (hPlayer != CSUDI_NULL)
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERClose (hPlayer), "步骤14失败");
		hPlayer = CSUDI_NULL;
	}

	return CSUDI_TRUE;
}

//@CASEGROUP:CSUDIPLAYERGetFileInfo
//@DESCRIPTION:测试输入参数非法的情况下，是否返回CSUDIPLAYER_ERROR_BAD_PARAMETER
//@PRECONDITION:PLAYER模块被正常初始化，file格式的文件存在
//@INPUT:1、参数组合不符合设计要求
//@EXPECTATION:返回非CSUDI_SUCCESS
//@REMARK:
//@EXECUTIONFLOW:1、调用CSUDIPLAYEROpen，打开一个本地文件播放器，得到播放器句柄
//@EXECUTIONFLOW:2、调用CSUDIPLAYERSetStream，设置播放器相应属性
//@EXECUTIONFLOW:3、调用CSUDIPLAYERGetFileInfo，第一个参数设置为空，期望返回CSUDIPLAYER_ERROR_BAD_PARAMETER
//@EXECUTIONFLOW:3、调用CSUDIPLAYERGetFileInfo，第二个参数设置为空，期望返回CSUDIPLAYER_ERROR_BAD_PARAMETER
//@EXECUTIONFLOW:3、调用CSUDIPLAYERGetFileInfo，两个参数都设置为空，期望返回CSUDIPLAYER_ERROR_BAD_PARAMETER
//@EXECUTIONFLOW:4、调用CSUDIPLAYERClose关闭播放句柄
//@EXECUTIONFLOW:5、恢复现场
CSUDI_BOOL CSTC_FILE_TEST_IT_GetFileInfo_0001(void)
{
	CSUDI_HANDLE hPlayer = CSUDI_NULL;
	CSUDI_Error_Code udiRe = CSUDI_SUCCESS;
	CSUDIPLAYERType_E ePlayerType = EM_UDIPLAYER_FILE;
	CSUDIPlayerFileInfo_S  stFileInfo;
	memset(&stFileInfo, 0, sizeof(CSUDIPlayerFileInfo_S));

	CSUDIPlaybackParam_S stPlaybackParam;

	memset(&stPlaybackParam,0,sizeof(stPlaybackParam));
	stPlaybackParam.m_eSpeed = EM_UDIPLAYER_SPEED_NORMAL;
	stPlaybackParam.m_nSecondPos = 0;

	strncpy(stPlaybackParam.m_szFileName, stFilePath[EM_UDI_FILENAME_MA].filename, CSUDI_MAX_FILE_NAME_LEN);

	udiRe = CSUDIPLAYERProbe(stPlaybackParam.m_szFileName);

	if(udiRe == CSUDI_SUCCESS)
	{
		CSTCPrint("本用例测试 \"%s\" 的播放\n", stPlaybackParam.m_szFileName);

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYEROpen (NULL,ePlayerType,&hPlayer), "步骤1:创建播放句柄失败");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_NULL != hPlayer, "步骤1:创建的播放句柄为空");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERSetStream (hPlayer, NULL, 0, &stPlaybackParam), "步骤2:设置流信息失败");

		CSTK_ASSERT_TRUE_FATAL(CSUDIPLAYER_ERROR_BAD_PARAMETER == CSUDIPLAYERGetFileInfo(CSUDI_NULL, &stFileInfo), "步骤3:传入非法句柄，应该返回CSUDIPLAYER_ERROR_BAD_PARAMETER");

		CSTK_ASSERT_TRUE_FATAL(CSUDIPLAYER_ERROR_BAD_PARAMETER == CSUDIPLAYERGetFileInfo(hPlayer, CSUDI_NULL), "步骤4:传入非法参数，应该返回CSUDIPLAYER_ERROR_BAD_PARAMETER");

		CSTK_ASSERT_TRUE_FATAL(CSUDIPLAYER_ERROR_BAD_PARAMETER == CSUDIPLAYERGetFileInfo(CSUDI_NULL, CSUDI_NULL), "步骤5:传入非法参数，应该返回CSUDIPLAYER_ERROR_BAD_PARAMETER");
	}
	else
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDI_FALSE, "不能够直接支持本地媒体播放");
	}

	CSTK_FATAL_POINT;

	if(hPlayer != CSUDI_NULL)
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERClose (hPlayer), "步骤6:关闭播放句柄失败");
	}

	return CSUDI_TRUE;
}

//@CASEGROUP:CSUDIPLAYERGetFileInfo
//@DESCRIPTION:测试获取 ts流文件的信息是否正确情况
//@PRECONDITION:PLAYER模块被正常初始化，file格式的文件存在
///@INPUT:1、有效的hPlayer
//@INPUT:2、有效的stFileStreamId
//@EXPECTATION:不支持  CSUDIPLAYERGetFileInfo获取普通文件信息
//@REMARK:码流必须为allts.ts
//@EXECUTIONFLOW:1、调用CSUDIPLAYEROpen，打开一个本地文件播放器，得到播放器句柄
//@EXECUTIONFLOW:2、调用CSUDIPLAYERSetStream，设置播放器相应属性
//@EXECUTIONFLOW:3、调用CSUDIPLAYERGetFileInfo，获取文件的信息
//@EXECUTIONFLOW:4、比较文件信息和获取到的文件信息是否相等，如果全部相等则证明是正确的
//@EXECUTIONFLOW:5、调用CSUDIPLAYERClose关闭播放句柄
//@EXECUTIONFLOW:6、恢复现场
CSUDI_BOOL CSTC_FILE_TEST_IT_GetFileInfo_0002(void)
{
	CSUDIPlayerFileInfo_S  stFileInfo;
	CSUDI_HANDLE hPlayer = CSUDI_NULL;
	CSUDI_Error_Code   udiRe = CSUDI_SUCCESS;
  	CSUDIPLAYERType_E ePlayerType = EM_UDIPLAYER_FILE;
	CSUDIPlaybackParam_S stPlaybackParam;

	memset(&stPlaybackParam,0,sizeof(stPlaybackParam));
	stPlaybackParam.m_eSpeed = EM_UDIPLAYER_SPEED_NORMAL;
	stPlaybackParam.m_nSecondPos = 0;


	memset(&stFileInfo, 0, sizeof(CSUDIPlayerFileInfo_S));

	strncpy(stPlaybackParam.m_szFileName, stFilePath[EM_UDI_FILENAME_MA].filename, CSUDI_MAX_FILE_NAME_LEN);

	CSTCPrint("本用例测试 \"%s\" 的播放\n", stPlaybackParam.m_szFileName);

	udiRe = CSUDIPLAYERProbe(stPlaybackParam.m_szFileName);

	if(udiRe == CSUDI_SUCCESS)
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYEROpen(NULL, ePlayerType, &hPlayer), "创建播放句柄失败");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_NULL != hPlayer, "创建的播放句柄为空");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERSetStream (hPlayer, NULL, 0, &stPlaybackParam), "设置流信息失败");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERGetFileInfo(hPlayer, &stFileInfo), "获取信息失败");

		CSTK_ASSERT_TRUE_FATAL(s_sStreamInfo[EM_UDIFILEPLAYER_STREAM_ES].m_eStreamType == EM_UDIFILEPLAYER_STREAM_ES, "文件类型错误");
		CSTK_ASSERT_TRUE_FATAL(s_sStreamInfo[EM_UDIFILEPLAYER_STREAM_ES].m_n64FileSize== stFileInfo.m_n64FileSize, "文件大小不一致");
		CSTK_ASSERT_TRUE_FATAL(s_sStreamInfo[EM_UDIFILEPLAYER_STREAM_ES].m_n64StartTime== stFileInfo.m_n64StartTime, "文件播放总时长错误");
		CSTK_ASSERT_TRUE_FATAL(s_sStreamInfo[EM_UDIFILEPLAYER_STREAM_ES].m_n64Duration== stFileInfo.m_n64Duration, "文件总时长错误");
		CSTK_ASSERT_TRUE_FATAL(s_sStreamInfo[EM_UDIFILEPLAYER_STREAM_ES].m_u32Bps == stFileInfo.m_u32Bps, "文件码率错误");
		CSTK_ASSERT_TRUE_FATAL(!strcmp(s_sStreamInfo[EM_UDIFILEPLAYER_STREAM_ES].m_acFileName ,stFileInfo.m_acFileName) , "文件名错误");
		CSTK_ASSERT_TRUE_FATAL(s_sStreamInfo[EM_UDIFILEPLAYER_STREAM_ES].m_u32ProgramNum == stFileInfo.m_u32ProgramNum, "节目个数错误");
		CSTK_ASSERT_TRUE_FATAL(s_sStreamInfo[EM_UDIFILEPLAYER_STREAM_ES].m_u32AudStreamNum == stFileInfo.m_astProgramInfo[0].m_u32AudStreamNum, "音频个数错误");
		CSTK_ASSERT_TRUE_FATAL(s_sStreamInfo[EM_UDIFILEPLAYER_STREAM_ES].m_u32SubTitleNum == stFileInfo.m_astProgramInfo[0].m_u32SubTitleNum, "字幕个数错误");
		CSTK_ASSERT_TRUE_FATAL(s_sStreamInfo[EM_UDIFILEPLAYER_STREAM_ES].m_u32VideoFormat == stFileInfo.m_astProgramInfo[0].m_stVidStream.m_u32Format, "视频格式错误");
		CSTK_ASSERT_TRUE_FATAL(s_sStreamInfo[EM_UDIFILEPLAYER_STREAM_ES].m_u16Width == stFileInfo.m_astProgramInfo[0].m_stVidStream.m_u16Width, "视频宽度错误");
		CSTK_ASSERT_TRUE_FATAL(s_sStreamInfo[EM_UDIFILEPLAYER_STREAM_ES].m_u16Height == stFileInfo.m_astProgramInfo[0].m_stVidStream.m_u16Height, "视频高度错误");
		CSTK_ASSERT_TRUE_FATAL(s_sStreamInfo[EM_UDIFILEPLAYER_STREAM_ES].m_u16Channels == stFileInfo.m_astProgramInfo[0].m_astAudStream[0].m_u16Channels, "音频声道错误");
		CSTK_ASSERT_TRUE_FATAL(s_sStreamInfo[EM_UDIFILEPLAYER_STREAM_ES].m_u32SampleRate == stFileInfo.m_astProgramInfo[0].m_astAudStream[0].m_u32SampleRate, "音频码率错误");
		CSTK_ASSERT_TRUE_FATAL(s_sStreamInfo[EM_UDIFILEPLAYER_STREAM_ES].m_u32AudioFormat == stFileInfo.m_astProgramInfo[0].m_astAudStream[0].m_u32Format, "音频格式错误");

	}
	else
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDI_FALSE, "不能够直接支持本地媒体播放");
	}

	CSTK_FATAL_POINT;

	if(hPlayer != CSUDI_NULL)
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERClose (hPlayer), "关闭播放句柄失败");
	}

	return CSUDI_TRUE;
}

//@CASEGROUP:CSUDIPLAYERGetFileInfo
//@DESCRIPTION:测试获取es流文件的信息是否正确情况
//@PRECONDITION:PLAYER模块被正常初始化，file格式的文件存在
///@INPUT:1、有效的hPlayer
//@INPUT:2、有效的stFileStreamId
//@EXPECTATION:不支持  CSUDIPLAYERGetFileInfo获取普通文件信息
//@REMARK:没有找到相关的码流，
//@EXECUTIONFLOW:1、调用CSUDIPLAYEROpen，打开一个本地文件播放器，得到播放器句柄
//@EXECUTIONFLOW:2、调用CSUDIPLAYERSetStream，设置播放器相应属性
//@EXECUTIONFLOW:3、调用CSUDIPLAYERGetFileInfo，获取文件的信息
//@EXECUTIONFLOW:4、比较文件信息和获取到的文件信息是否相等，如果全部相等则证明是正确的
//@EXECUTIONFLOW:5、调用CSUDIPLAYERClose关闭播放句柄
//@EXECUTIONFLOW:6、恢复现场
CSUDI_BOOL CSTC_FILE_TEST_IT_GetFileInfo_0003(void)
{
	CSUDIPlayerFileInfo_S  stFileInfo;
	CSUDI_HANDLE hPlayer = CSUDI_NULL;
	CSUDIPLAYERType_E ePlayerType = EM_UDIPLAYER_FILE;
	CSUDIPlaybackParam_S stPlaybackParam;

	memset(&stPlaybackParam,0,sizeof(stPlaybackParam));
	stPlaybackParam.m_eSpeed = EM_UDIPLAYER_SPEED_NORMAL;
	stPlaybackParam.m_nSecondPos = 0;


	memset(&stFileInfo, 0, sizeof(CSUDIPlayerFileInfo_S));

	strncpy(stPlaybackParam.m_szFileName, stFilePath[EM_UDI_FILENAME_MV].filename, CSUDI_MAX_FILE_NAME_LEN);

	CSTCPrint("本用例测试 \"%s\" 的播放\n", stPlaybackParam.m_szFileName);

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYEROpen(NULL, ePlayerType, &hPlayer), "创建播放句柄失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_NULL != hPlayer, "创建的播放句柄为空");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERSetStream (hPlayer, NULL, 0, &stPlaybackParam), "设置流信息失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERGetFileInfo(hPlayer, &stFileInfo), "获取信息失败");

	CSTCPrint("---------------m_eStreamType == %d\r\n", stFileInfo.m_eStreamType);
	CSTCPrint("---------------m_acFileName == %s\r\n", stFileInfo.m_acFileName);
	CSTCPrint("---------------m_n64FileSize == %lld\r\n", stFileInfo.m_n64FileSize);
	CSTCPrint("---------------m_n64StartTime == %lld\r\n", stFileInfo.m_n64StartTime);
	CSTCPrint("---------------m_n64Duration == %lld\r\n", stFileInfo.m_n64Duration);
	CSTCPrint("---------------m_u32Bps == %d\r\n", (int)stFileInfo.m_u32Bps);
	CSTCPrint("---------------m_u32ProgramNum == %d\r\n", (int)stFileInfo.m_u32ProgramNum);

	CSTCPrint("---------------m_u32AudStreamNum == %d\r\n", (int)stFileInfo.m_astProgramInfo[0].m_u32AudStreamNum);
	CSTCPrint("---------------m_u32SubTitleNum == %d\r\n", (int)stFileInfo.m_astProgramInfo[0].m_u32SubTitleNum);
	CSTCPrint("---------------m_u16Height == %d\r\n", (int)stFileInfo.m_astProgramInfo[0].m_stVidStream.m_u16Height);
	CSTCPrint("---------------m_u16Width == %d\r\n", (int)stFileInfo.m_astProgramInfo[0].m_stVidStream.m_u16Width);
	CSTCPrint("---------------m_u32Format == %d\r\n", (int)stFileInfo.m_astProgramInfo[0].m_stVidStream.m_u32Format);

	CSTCPrint("---------------m_acAudLang == %s\r\n", (int)stFileInfo.m_astProgramInfo[0].m_astAudStream[0].m_acAudLang);
	CSTCPrint("---------------m_u16Channels == %d\r\n", (int)stFileInfo.m_astProgramInfo[0].m_astAudStream[0].m_u16Channels);
	CSTCPrint("---------------m_u32Format == %d\r\n", (int)stFileInfo.m_astProgramInfo[0].m_astAudStream[0].m_u32Format);
	CSTCPrint("---------------m_u32SampleRate == %d\r\n", (int)stFileInfo.m_astProgramInfo[0].m_astAudStream[0].m_u32SampleRate);

	CSTK_FATAL_POINT;

	if(hPlayer != CSUDI_NULL)
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERClose (hPlayer), "关闭播放句柄失败");
	}

	return CSUDI_TRUE;
}

//@CASEGROUP:CSUDIPLAYERGetFileInfo
//@DESCRIPTION:测试获取普通文件的信息是否正确情况
//@PRECONDITION:PLAYER模块被正常初始化，file格式的文件存在
///@INPUT:1、有效的hPlayer
//@INPUT:2、有效的stFileStreamId
//@EXPECTATION:不支持  CSUDIPLAYERGetFileInfo获取普通文件信息
//@REMARK:没有找到相关的码流，
//@EXECUTIONFLOW:1、调用CSUDIPLAYEROpen，打开一个本地文件播放器，得到播放器句柄
//@EXECUTIONFLOW:2、调用CSUDIPLAYERSetStream，设置播放器相应属性
//@EXECUTIONFLOW:3、调用CSUDIPLAYERGetFileInfo，获取文件的信息
//@EXECUTIONFLOW:4、比较文件信息和获取到的文件信息是否相等，如果全部相等则证明是正确的
//@EXECUTIONFLOW:5、调用CSUDIPLAYERClose关闭播放句柄
//@EXECUTIONFLOW:6、恢复现场
CSUDI_BOOL CSTC_FILE_TEST_IT_GetFileInfo_0004(void)
{
	CSUDIPlayerFileInfo_S  stFileInfo;
	CSUDI_HANDLE hPlayer = CSUDI_NULL;
	CSUDIPLAYERType_E ePlayerType = EM_UDIPLAYER_FILE;
	CSUDIPlaybackParam_S stPlaybackParam;

	memset(&stPlaybackParam,0,sizeof(stPlaybackParam));
	stPlaybackParam.m_eSpeed = EM_UDIPLAYER_SPEED_NORMAL;
	stPlaybackParam.m_nSecondPos = 0;

	memset(&stFileInfo, 0, sizeof(CSUDIPlayerFileInfo_S));

	strncpy(stPlaybackParam.m_szFileName, stFilePath[EM_UDI_FILENAME_MP3].filename, CSUDI_MAX_FILE_NAME_LEN);

	CSTCPrint("本用例测试 \"%s\" 的播放\n", stPlaybackParam.m_szFileName);

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYEROpen(NULL, ePlayerType, &hPlayer), "创建播放句柄失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_NULL != hPlayer, "创建的播放句柄为空");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERSetStream (hPlayer, NULL, 0, &stPlaybackParam), "设置流信息失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERGetFileInfo(hPlayer, &stFileInfo), "获取信息失败");

	CSTCPrint("---------------m_eStreamType == %d\r\n", stFileInfo.m_eStreamType);
	CSTCPrint("---------------m_acFileName == %s\r\n", stFileInfo.m_acFileName);
	CSTCPrint("---------------m_n64FileSize == %lld\r\n", stFileInfo.m_n64FileSize);
	CSTCPrint("---------------m_n64StartTime == %lld\r\n", stFileInfo.m_n64StartTime);
	CSTCPrint("---------------m_n64Duration == %lld\r\n", stFileInfo.m_n64Duration);
	CSTCPrint("---------------m_u32Bps == %d\r\n", (int)stFileInfo.m_u32Bps);
	CSTCPrint("---------------m_u32ProgramNum == %d\r\n", (int)stFileInfo.m_u32ProgramNum);

	CSTCPrint("---------------m_u32AudStreamNum == %d\r\n", (int)stFileInfo.m_astProgramInfo[0].m_u32AudStreamNum);
	CSTCPrint("---------------m_u32SubTitleNum == %d\r\n", (int)stFileInfo.m_astProgramInfo[0].m_u32SubTitleNum);
	CSTCPrint("---------------m_u16Height == %d\r\n", (int)stFileInfo.m_astProgramInfo[0].m_stVidStream.m_u16Height);
	CSTCPrint("---------------m_u16Width == %d\r\n", (int)stFileInfo.m_astProgramInfo[0].m_stVidStream.m_u16Width);
	CSTCPrint("---------------m_u32Format == %d\r\n", (int)stFileInfo.m_astProgramInfo[0].m_stVidStream.m_u32Format);

	CSTCPrint("---------------m_acAudLang == %s\r\n", (int)stFileInfo.m_astProgramInfo[0].m_astAudStream[0].m_acAudLang);
	CSTCPrint("---------------m_u16Channels == %d\r\n", (int)stFileInfo.m_astProgramInfo[0].m_astAudStream[0].m_u16Channels);
	CSTCPrint("---------------m_u32Format == %d\r\n", (int)stFileInfo.m_astProgramInfo[0].m_astAudStream[0].m_u32Format);
	CSTCPrint("---------------m_u32SampleRate == %d\r\n", (int)stFileInfo.m_astProgramInfo[0].m_astAudStream[0].m_u32SampleRate);

	CSTK_FATAL_POINT;

	if(hPlayer != CSUDI_NULL)
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERClose (hPlayer), "关闭播放句柄失败");
	}

	return CSUDI_TRUE;
}

//@CASEGROUP:CSUDIPLAYERGetFileInfo
//@DESCRIPTION:测试获取网络文件的信息是否正确情况
//@PRECONDITION:PLAYER模块被正常初始化，file格式的文件存在
///@INPUT:1、有效的hPlayer
//@INPUT:2、有效的stFileStreamId
//@EXPECTATION:不支持  CSUDIPLAYERGetFileInfo获取网络文件信息
//@REMARK:没有找到相关的码流，
//@EXECUTIONFLOW:1、调用CSUDIPLAYEROpen，打开一个本地文件播放器，得到播放器句柄
//@EXECUTIONFLOW:2、调用CSUDIPLAYERSetStream，设置播放器相应属性
//@EXECUTIONFLOW:3、调用CSUDIPLAYERGetFileInfo，获取文件的信息
//@EXECUTIONFLOW:4、比较文件信息和获取到的文件信息是否相等，如果全部相等则证明是正确的
//@EXECUTIONFLOW:5、调用CSUDIPLAYERClose关闭播放句柄
//@EXECUTIONFLOW:6、恢复现场
CSUDI_BOOL CSTC_FILE_TEST_IT_GetFileInfo_0005(void)
{
	CSUDIPlayerFileInfo_S  stFileInfo;
	CSUDI_HANDLE hPlayer = CSUDI_NULL;
	CSUDIPLAYERType_E ePlayerType = EM_UDIPLAYER_FILE;
	CSUDIPlaybackParam_S stPlaybackParam;

	memset(&stPlaybackParam,0,sizeof(stPlaybackParam));
	stPlaybackParam.m_eSpeed = EM_UDIPLAYER_SPEED_NORMAL;
	stPlaybackParam.m_nSecondPos = 0;

	memset(&stFileInfo, 0, sizeof(CSUDIPlayerFileInfo_S));

	strncpy(stPlaybackParam.m_szFileName, stFilePath[EM_UDI_FILENAME_MA].filename, CSUDI_MAX_FILE_NAME_LEN);

	CSTCPrint("本用例测试 \"%s\" 的播放\n", stPlaybackParam.m_szFileName);

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYEROpen(NULL, ePlayerType, &hPlayer), "创建播放句柄失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_NULL != hPlayer, "创建的播放句柄为空");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS  == CSUDIPLAYERSetStream (hPlayer, NULL, 0, &stPlaybackParam), "设置流信息失败");

	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERGetFileInfo(hPlayer, &stFileInfo), "获取信息失败");

	CSTCPrint("---------------m_eStreamType == %d\r\n", stFileInfo.m_eStreamType);
	CSTCPrint("---------------m_acFileName == %s\r\n", stFileInfo.m_acFileName);
	CSTCPrint("---------------m_n64FileSize == %lld\r\n", stFileInfo.m_n64FileSize);
	CSTCPrint("---------------m_n64StartTime == %lld\r\n", stFileInfo.m_n64StartTime);
	CSTCPrint("---------------m_n64Duration == %lld\r\n", stFileInfo.m_n64Duration);
	CSTCPrint("---------------m_u32Bps == %d\r\n", (int)stFileInfo.m_u32Bps);
	CSTCPrint("---------------m_u32ProgramNum == %d\r\n", (int)stFileInfo.m_u32ProgramNum);

	CSTCPrint("---------------m_u32AudStreamNum == %d\r\n", (int)stFileInfo.m_astProgramInfo[0].m_u32AudStreamNum);
	CSTCPrint("---------------m_u32SubTitleNum == %d\r\n", (int)stFileInfo.m_astProgramInfo[0].m_u32SubTitleNum);
	CSTCPrint("---------------m_u16Height == %d\r\n", (int)stFileInfo.m_astProgramInfo[0].m_stVidStream.m_u16Height);
	CSTCPrint("---------------m_u16Width == %d\r\n", (int)stFileInfo.m_astProgramInfo[0].m_stVidStream.m_u16Width);
	CSTCPrint("---------------m_u32Format == %d\r\n", (int)stFileInfo.m_astProgramInfo[0].m_stVidStream.m_u32Format);

	CSTCPrint("---------------m_acAudLang == %s\r\n", (int)stFileInfo.m_astProgramInfo[0].m_astAudStream[0].m_acAudLang);
	CSTCPrint("---------------m_u16Channels == %d\r\n", (int)stFileInfo.m_astProgramInfo[0].m_astAudStream[0].m_u16Channels);
	CSTCPrint("---------------m_u32Format == %d\r\n", (int)stFileInfo.m_astProgramInfo[0].m_astAudStream[0].m_u32Format);
	CSTCPrint("---------------m_u32SampleRate == %d\r\n", (int)stFileInfo.m_astProgramInfo[0].m_astAudStream[0].m_u32SampleRate);

	CSTK_FATAL_POINT;

	if(hPlayer != CSUDI_NULL)
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERClose (hPlayer), "关闭播放句柄失败");
	}

	return CSUDI_TRUE;
}

//@CASEGROUP:CSUDIPLAYERSetFilePlayStream
//@DESCRIPTION:测试参数非法的情况下，是否返回CSUDIPLAYER_ERROR_BAD_PARAMETER
//@PRECONDITION:PLAYER模块被正常初始化，file格式的文件存在
//@INPUT:1、参数组合不符合设计要求
//@EXPECTATION:返回非CSUDI_SUCCESS
//@REMARK:
//@EXECUTIONFLOW:1、调用CSUDIPLAYEROpen，打开一个本地文件播放器，得到播放器句柄
//@EXECUTIONFLOW:2、调用CSUDIPLAYERSetStream，设置播放器相应属性
//@EXECUTIONFLOW:3、调用CSUDIPLAYERGetFileInfo，获取播文件的信息
//@EXECUTIONFLOW:4、调用CSUDIPLAYERStart，开始播放文件
//@EXECUTIONFLOW:5、调用CSUDIPLAYERSetFilePlayStream，第一个参数设置为空，期望返回CSUDIPLAYER_ERROR_BAD_PARAMETER
//@EXECUTIONFLOW:6、调用CSUDIPLAYERSetFilePlayStream，第二个参数设置为空，期望返回CSUDIPLAYER_ERROR_BAD_PARAMETER
//@EXECUTIONFLOW:7、调用CSUDIPLAYERSetFilePlayStream，两个参数都设置为空，期望返回CSUDIPLAYER_ERROR_BAD_PARAMETER
//@EXECUTIONFLOW:8、调用CSUDIPLAYERStop，停止播放
//@EXECUTIONFLOW:9、恢复现场
CSUDI_BOOL CSTC_FILE_TEST_IT_SetFilePlayStream_0001(void)
{
	CSUDI_HANDLE  hPlayer = CSUDI_NULL;
	CSUDIPLAYERType_E ePlayerType = EM_UDIPLAYER_FILE;
	CSUDIPlayerFileStreamId_S  stFileStreamId;
	CSUDI_Error_Code  udiRe = CSUDI_SUCCESS;
	CSUDIPlaybackParam_S stPlaybackParam;
	CSUDIPlayerFileInfo_S  stFileInfo;

	memset(&stFileStreamId, 0, sizeof(CSUDIPlayerFileStreamId_S));
	memset(&stFileInfo, 0, sizeof(CSUDIPlayerFileInfo_S));

	memset(&stPlaybackParam,0,sizeof(stPlaybackParam));
	stPlaybackParam.m_eSpeed = EM_UDIPLAYER_SPEED_NORMAL;
	stPlaybackParam.m_nSecondPos = 0;

	strncpy(stPlaybackParam.m_szFileName, stFilePath[EM_UDI_FILENAME_MA].filename, CSUDI_MAX_FILE_NAME_LEN);

	udiRe = CSUDIPLAYERProbe(stPlaybackParam.m_szFileName);

	if(udiRe == CSUDI_SUCCESS)
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYEROpen (NULL,ePlayerType,&hPlayer), "步骤1、创建播放句柄失败");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_NULL != hPlayer, "步骤1、创建的播放句柄为空");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERSetStream(hPlayer, NULL, 0, &stPlaybackParam), "步骤2、设置用户节目信息失败")

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERGetFileInfo(hPlayer, &stFileInfo), "步骤3、获取文件信息失败");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERStart(hPlayer), "步骤4、播放失败");

		CSTK_ASSERT_TRUE_FATAL(CSUDIPLAYER_ERROR_BAD_PARAMETER == CSUDIPLAYERSetFilePlayStream(CSUDI_NULL, &stFileStreamId), "步骤5、传入非法参数，应该返回CSUDIPLAYER_ERROR_BAD_PARAMETER");

		CSTK_ASSERT_TRUE_FATAL(CSUDIPLAYER_ERROR_BAD_PARAMETER == CSUDIPLAYERSetFilePlayStream(hPlayer, CSUDI_NULL), "步骤6、传入非法参数，应该返回CSUDIPLAYER_ERROR_BAD_PARAMETER");

		CSTK_ASSERT_TRUE_FATAL(CSUDIPLAYER_ERROR_BAD_PARAMETER == CSUDIPLAYERSetFilePlayStream(CSUDI_NULL, CSUDI_NULL), "步骤7、传入非法参数，应该返回CSUDIPLAYER_ERROR_BAD_PARAMETER");

		CSUDIOSThreadSleep(2*1000);

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERStop(hPlayer),"步骤8、停止播放失败");
	}
	else
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDI_FALSE, "不能够直接支持本地媒体播放");
	}

	CSTK_FATAL_POINT;

	if(hPlayer != CSUDI_NULL)
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERClose (hPlayer), "步骤9、关闭播放句柄失败");
	}

	return CSUDI_TRUE;
}

//@CASEGROUP:CSUDIPLAYERSetFilePlayStream
//@DESCRIPTION:测试多音轨情况下，改变音频的pid值音频是否改变
//@PRECONDITION:PLAYER模块被正常初始化，file格式的文件存在
//@INPUT:1、有效的hPlayer
//@INPUT:2、有效的stFileStreamId
//@EXPECTATION:不支持  CSUDIPLAYERSetFilePlayStream设置多音频
//@REMARK:必须有多音频的码流
//@EXECUTIONFLOW:1、调用CSUDIPLAYEROpen，打开一个本地文件播放器，得到播放器句柄
//@EXECUTIONFLOW:2、调用CSUDIPLAYERSetStream，设置播放器相应属性
//@EXECUTIONFLOW:3、调用CSUDIPLAYERGetFileInfo，获取播文件的信息
//@EXECUTIONFLOW:4、比较音频的个数是否超过最大个数
//@EXECUTIONFLOW:5、调用CSUDIPLAYERSetFilePlayStream，设置文件的播放属性
//@EXECUTIONFLOW:6、调用CSUDIPLAYERStart，开始播放文件
//@EXECUTIONFLOW:7、调用CSTKWaitYes，等待测试是否有音频切换
//@EXECUTIONFLOW:8、调用CSUDIPLAYERStop，停止播放
//@EXECUTIONFLOW:9、调用CSUDIPLAYERClose，关闭播放句柄
//@EXECUTIONFLOW:10、恢复现场
CSUDI_BOOL CSTC_FILE_TEST_IT_SetFilePlayStream_0002(void)
{
	CSUDI_HANDLE  hPlayer = CSUDI_NULL;
	CSUDIPLAYERType_E ePlayerType = EM_UDIPLAYER_FILE;
	CSUDIPlayerFileStreamId_S  stFileStreamId;
	CSUDIPlaybackParam_S   stPlaybackParam;
	CSUDIPlayerFileInfo_S  stFileInfo;
	CSUDI_Error_Code   udiRe = CSUDI_SUCCESS;

	memset(&stFileStreamId, 0, sizeof(CSUDIPlayerFileStreamId_S));

	memset(&stPlaybackParam,0,sizeof(stPlaybackParam));
	stPlaybackParam.m_eSpeed = EM_UDIPLAYER_SPEED_NORMAL;
	stPlaybackParam.m_nSecondPos = 0;

	memset(&stFileInfo, 0, sizeof(CSUDIPlayerFileInfo_S));

	strncpy(stPlaybackParam.m_szFileName, stFilePath[EM_UDI_FILENAME_MA].filename, CSUDI_MAX_FILE_NAME_LEN);

	udiRe = CSUDIPLAYERProbe(stPlaybackParam.m_szFileName);

	if(udiRe == CSUDI_SUCCESS)
	{
		CSTCPrint("本用例测试 \"%s\" 的播放\n", stPlaybackParam.m_szFileName);

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYEROpen (NULL,ePlayerType,&hPlayer), "步骤1失败、创建播放句柄失败");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_NULL != hPlayer, "步骤1失败、创建的播放句柄为空");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERSetStream(hPlayer, NULL, 0, &stPlaybackParam), "步骤2失败、设置用户节目信息失败");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERGetFileInfo(hPlayer, &stFileInfo), "步骤3失败、获取文件信息失败");

		CSTK_ASSERT_TRUE_FATAL(stFileInfo.m_astProgramInfo[0].m_u32AudStreamNum <= CSUDI_PLAYER_MAX_STREAM_NUM, "步骤4失败、音频的个数超过了最大数五个");

		int i = 0;
		for(i=0; i<stFileInfo.m_astProgramInfo[0].m_u32AudStreamNum; i++)
		{
			stFileStreamId.m_u32AStreamId = i;

			CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERSetFilePlayStream(hPlayer, &stFileStreamId), "步骤5失败、设置文件的播放属性");

			CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERStart(hPlayer), "步骤6失败、开始播放不正常");

			if(i != 0)
			{
				CSTCPrint("是否和上一个音频不同\n");
	        	CSTCPrint("Does the player AUDIO are different to the former ?\n");
			}
			else
			{
				CSTCPrint("音频是否播放正常?\n");
	        	CSTCPrint("Does the player AUDIO has play OK ?\n");
			}

			CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes(),"步骤7失败：是否与上一个音频不同");

			CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERStop(hPlayer),"步骤8失败、停止播放失败");

			CSUDIOSThreadSleep(1000);

		}


	}
	else
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDI_FALSE, "不能够直接支持本地媒体播放");
	}

	CSTK_FATAL_POINT;

	if(hPlayer != CSUDI_NULL)
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERClose(hPlayer), "步骤9失败：关闭播放失败");
	}

	return CSUDI_TRUE;
}

//@CASEGROUP:CSUDIPLAYERSetFilePlayStream
//@DESCRIPTION:测试多视频情况下，改变视频的pid值视频是否改变
//@PRECONDITION:PLAYER模块被正常初始化，file格式的文件存在
//@INPUT:1、有效的hPlayer
//@INPUT:2、有效的stFileStreamId
//@EXPECTATION:不支持  CSUDIPLAYERSetFilePlayStream设置多视频
//@REMARK:必须有多视频的码流
//@EXECUTIONFLOW:1、调用CSUDIPLAYEROpen，打开一个本地文件播放器，得到播放器句柄
//@EXECUTIONFLOW:2、调用CSUDIPLAYERSetStream，设置播放器相应属性
//@EXECUTIONFLOW:3、调用CSUDIPLAYERGetFileInfo，获取播文件的信息
//@EXECUTIONFLOW:4、比较视频的个数是否超过最大个数
//@EXECUTIONFLOW:5、调用CSUDIPLAYERSetFilePlayStream，设置文件的播放属性
//@EXECUTIONFLOW:6、调用CSUDIPLAYERStart，开始播放文件
//@EXECUTIONFLOW:7、调用CSTKWaitYes，等待测试是否有视频切换
//@EXECUTIONFLOW:8、调用CSUDIPLAYERStop，停止播放
//@EXECUTIONFLOW:9、调用CSUDIPLAYERClose，关闭播放句柄
//@EXECUTIONFLOW:10、恢复现场
CSUDI_BOOL CSTC_FILE_TEST_IT_SetFilePlayStream_0003(void)
{
	CSUDI_HANDLE  hPlayer = CSUDI_NULL;
	CSUDIPLAYERType_E ePlayerType = EM_UDIPLAYER_FILE;
	CSUDIPlayerFileStreamId_S  stFileStreamId;
	CSUDIPlaybackParam_S   stPlaybackParam;
	CSUDIPlayerFileInfo_S  stFileInfo;
	CSUDI_Error_Code udiRe = CSUDI_SUCCESS;

	memset(&stFileStreamId, 0, sizeof(CSUDIPlayerFileStreamId_S));

	memset(&stPlaybackParam,0,sizeof(stPlaybackParam));
	stPlaybackParam.m_eSpeed = EM_UDIPLAYER_SPEED_NORMAL;
	stPlaybackParam.m_nSecondPos = 0;

	memset(&stFileInfo, 0, sizeof(CSUDIPlayerFileInfo_S));

	strncpy(stPlaybackParam.m_szFileName, stFilePath[EM_UDI_FILENAME_MV].filename, CSUDI_MAX_FILE_NAME_LEN);

	udiRe = CSUDIPLAYERProbe(stPlaybackParam.m_szFileName);

	if(udiRe == CSUDI_SUCCESS)
	{
		CSTCPrint("本用例测试 \"%s\" 的播放\n", stPlaybackParam.m_szFileName);

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYEROpen (NULL,ePlayerType,&hPlayer), "步骤1、创建播放句柄失败");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_NULL != hPlayer, "步骤1、创建的播放句柄为空");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERSetStream(hPlayer, NULL, 0, &stPlaybackParam), "步骤2、设置用户节目信息失败")

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERGetFileInfo(hPlayer, &stFileInfo), "步骤3、获取文件信息失败");

		CSTK_ASSERT_TRUE_FATAL(stFileInfo.m_u32ProgramNum <= CSUDI_PLAYER_MAX_PROGRAM_NUM, "步骤4、视频的个数超过了最大数五个");

		int i = 0;
		for(i=0; i<stFileInfo.m_u32ProgramNum; i++)
		{
			stFileStreamId.m_u32ProgramId = i;

			CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERSetFilePlayStream(hPlayer, &stFileStreamId), "步骤5、设置应该返回");

			CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERStart(hPlayer), "步骤6、开始播放不正常");

			if(i != 0)
			{
				CSTCPrint("视频是否切换 � \n");
	        	CSTCPrint("Does the player VUDIO are different to the former ?\n");
			}
			else
			{
				CSTCPrint("视频是播放成功 � \n");
	        	CSTCPrint("Does the player VUDIO has play OK ?\n");
			}

			CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes(),"步骤7、开始播放不正常");

			CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERStop(hPlayer),"步骤8、停止播放失败");

			CSUDIOSThreadSleep(2*1000);
		}
	}
	else
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDI_FALSE, "不能够直接支持本地媒体播放");
	}

	CSTK_FATAL_POINT;

	if(hPlayer != CSUDI_NULL)
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERClose(hPlayer), "步骤9、关闭播放失败");
	}

	return CSUDI_TRUE;
}



//@CASEGROUP:CSUDIPLAYERSetFilePlayStream
//@DESCRIPTION:测试多字幕情况下，改变字幕的pid值音频是否改变
//@PRECONDITION:PLAYER模块被正常初始化，file格式的文件存在
//@INPUT:1、有效的hPlayer
//@INPUT:2、有效的stFileStreamId
//@EXPECTATION:不支持  CSUDIPLAYERSetFilePlayStream设置字幕
//@REMARK:必须有多字幕的码流
//@EXECUTIONFLOW:1、调用CSUDIPLAYEROpen，打开一个本地文件播放器，得到播放器句柄
//@EXECUTIONFLOW:2、调用CSUDIPLAYERSetStream，设置播放器相应属性
//@EXECUTIONFLOW:3、调用CSUDIPLAYERGetFileInfo，获取播文件的信息
//@EXECUTIONFLOW:4、比较字幕的个数是否超过最大个数，是否有字幕
//@EXECUTIONFLOW:5、调用CSUDIPLAYERSetFilePlayStream，设置文件的播放属性
//@EXECUTIONFLOW:6、调用CSUDIPLAYERStart，开始播放文件
//@EXECUTIONFLOW:7、调用CSTKWaitYes，等待测试是否有字幕切换
//@EXECUTIONFLOW:8、调用CSUDIPLAYERStop，停止播放
//@EXECUTIONFLOW:9、调用CSUDIPLAYERClose，关闭播放句柄
//@EXECUTIONFLOW:10、恢复现场
CSUDI_BOOL CSTC_FILE_TEST_IT_SetFilePlayStream_0004(void)
{
	CSUDI_HANDLE  hPlayer = CSUDI_NULL;
	CSUDIPLAYERType_E ePlayerType = EM_UDIPLAYER_FILE;
	CSUDIPlayerFileStreamId_S  stFileStreamId;
	CSUDIPlaybackParam_S   stPlaybackParam;
	CSUDIPlayerFileInfo_S  stFileInfo;
	CSUDI_Error_Code  udiRe = CSUDI_SUCCESS;

	memset(&stFileStreamId, 0, sizeof(CSUDIPlayerFileStreamId_S));
	memset(&stFileInfo, 0, sizeof(CSUDIPlayerFileInfo_S));
	memset(&stPlaybackParam,0,sizeof(stPlaybackParam));
	stPlaybackParam.m_eSpeed = EM_UDIPLAYER_SPEED_NORMAL;
	stPlaybackParam.m_nSecondPos = 0;
	strncpy(stPlaybackParam.m_szFileName, stFilePath[EM_UDI_FILENAME_MS].filename, CSUDI_MAX_FILE_NAME_LEN);

	udiRe = CSUDIPLAYERProbe(stPlaybackParam.m_szFileName);

	if(udiRe == CSUDI_SUCCESS)
	{
		CSTCPrint("本用例测试 \"%s\" 的播放\n", stPlaybackParam.m_szFileName);

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYEROpen (NULL,ePlayerType,&hPlayer), "步骤1、创建播放句柄失败");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_NULL != hPlayer, "步骤1、创建的播放句柄为空");

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERSetStream(hPlayer, NULL, 0, &stPlaybackParam), "步骤2、设置用户节目信息失败")

		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERGetFileInfo(hPlayer, &stFileInfo), "步骤3、获取文件信息失败");

		CSTK_ASSERT_TRUE_FATAL(stFileInfo.m_astProgramInfo[0].m_u32SubTitleNum <= CSUDI_PLAYER_MAX_LANG_NUM, "步骤4、字幕的个数超过了最大数五个");

		CSTK_ASSERT_TRUE_FATAL(stFileInfo.m_astProgramInfo[0].m_u32SubTitleNum > 0, "步骤4、字幕的个数为零，请加字幕文件");

		int i = 0;
		for(i=0; i<stFileInfo.m_astProgramInfo[0].m_u32SubTitleNum; i++)
		{
			stFileStreamId.m_u32SubTitleId = i;
			CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERSetFilePlayStream(hPlayer, &stFileStreamId), "步骤5、设置应该返回");

			CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERStart(hPlayer), "步骤6、开始播放不正常");

			if(i != 0)
			{
				CSTCPrint("the language is %s\n", stFileInfo.m_astProgramInfo[0].m_astSubTitle[i].m_acSubTitleName);
				CSTCPrint(" 字幕的标题是否和前一个不同\n");
	        	CSTCPrint("Does the player subtitle has differnt to the former ?\n");
			}
			else
			{
				CSTCPrint(" 是否显示字幕标题\n");
				CSTCPrint("the language is %s\n", stFileInfo.m_astProgramInfo[0].m_astSubTitle[i].m_acSubTitleName);
	        	CSTCPrint("Does the player subtitle has normal output    ?\n");
			}

			CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes(),"步骤7、字幕播放失败!");

			CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERStop(hPlayer),"步骤8、停止播放失败");

			CSUDIOSThreadSleep(1000);
		}
	}
	else
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDI_FALSE, "不能够直接支持本地媒体播放");
	}

	CSTK_FATAL_POINT;

	if(hPlayer != CSUDI_NULL)
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIPLAYERClose(hPlayer), "步骤9、关闭播放失败");
	}

	return CSUDI_TRUE;
}



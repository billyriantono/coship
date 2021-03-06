/* --------------------------------------------------------------------
注意：
1.在需要与用户交互的测试用例中，可以：
	a. 使用CSTKWaitAnyKey等待用户输入任意按键
	b. 使用CSTKWaitYes等待用户输入YES
2.测试用例函数命名：测试用例ID，"测试用例ID"定义在测试用例文档中
-----------------------------------------------------------------------*/
#include <string.h>
#include <stdio.h>
#include "../../../include/porting/udi2/udi2_EW200Service.h"
#include "../../../include/porting/udi2/udi2_tuner.h"
#include "cs_ew200servicetestcase.h"
#include "../cs_udi2testcase.h" 
#include "udi2_error.h"
#include "udi2_typedef.h"
#include "udi2_public.h"
#include "udi2_os.h"
#include "cs_testkit.h"
#include "udi2_player.h"
#include "CSEWVod.h"
#include "udiplus_debug.h"

static int s_nDeliverCnt = 1;     //频点个数
static int s_nRunCnt = 10;

static char* s_pcTestDvbcUrl = "dvbc://659000000.6875.64.67";     
static int s_nUserData = 12345;          //tunercallback
static int s_nTestData = -1; 
static int s_nCallbackUserData = 0x1234;
static int s_anTestData[4501];            //test addcallback function

BOOL CSTC_EW200SERVICE_Init(void)
{
	//在本测试用例集执行前调用
	return TRUE;
}

BOOL CSTC_EW200SERVICE_UnInit(void)
{
	//在本测试用例集执行后调用
	return TRUE;
}

//@CASEGROUP:CSUDIEW200SReadClientInfo 
//@DESCRIPTION: 测试函数CSUDIEW200SReadClientInfo在参数都正确时的执行效果(主要对第三方提供的EM_UDIEW200S_CARDID, \
//EM_UDIEW200S_SN,EM_UDIEW200S_PROVIDER,EM_UDIEW200S_ALIAS,EM_UDIEW200S_MODULE,EM_UDIEW200S_LOCAL_IP,EM_UDIEW200S_LOCAL_MAC, \
//EM_UDIEW200S_CITV_PSD,EM_UDIEW200S_CITV_SERVERIP,EM_UDIEW200S_CITV_SERVERPORT,EM_UDIEW200S_CITV_VIDEOTYPE,EM_UDIEW200S_CITV_BROWSERVER测试)
//@PRECONDITION:第三方提供配置信息
//@INPUT:1、分别输入CSUDIEW200SClientInfoType_E枚举中的值
//@INPUT:2、要读取出的CSUDIEW200SClientInfoType_E中各枚举类型对应的信息acValue
//@INPUT:3、读取出的信息长度为sizeof(acValue)
//@EXPECTATION:期望读取出的值与从配置中获得对应的值相等
//@EXECUTIONFLOW:1、从配置文件中分别获得EM_UDIEW200S_CARDID, \
//@EXECUTIONFLOW:EM_UDIEW200S_SN,EM_UDIEW200S_PROVIDER,EM_UDIEW200S_ALIAS,EM_UDIEW200S_MODULE,EM_UDIEW200S_LOCAL_IP,EM_UDIEW200S_LOCAL_MAC, \
//@EXECUTIONFLOW:EM_UDIEW200S_CITV_PSD,EM_UDIEW200S_CITV_SERVERIP,EM_UDIEW200S_CITV_SERVERPORT,EM_UDIEW200S_CITV_VIDEOTYPE,EM_UDIEW200S_CITV_BROWSERVER对应的值
//@EXECUTIONFLOW:2、调用函数CSUDIEW200SReadClientInfo分别读取EM_UDIEW200S_CARDID, \
//@EXECUTIONFLOW:EM_UDIEW200S_SN,EM_UDIEW200S_PROVIDER,EM_UDIEW200S_ALIAS,EM_UDIEW200S_MODULE,EM_UDIEW200S_LOCAL_IP,EM_UDIEW200S_LOCAL_MAC, \
//@EXECUTIONFLOW:EM_UDIEW200S_CITV_PSD,EM_UDIEW200S_CITV_SERVERIP,EM_UDIEW200S_CITV_SERVERPORT,EM_UDIEW200S_CITV_VIDEOTYPE,EM_UDIEW200S_CITV_BROWSERVER的认证信息
//@EXECUTIONFLOW:3、比较从配置文件中获得的值与调用函数CSUDIEW200SReadClientInfo读取出的值，期望两者相等
BOOL CSTC_EW200SERVICE_MT_0001(void)
{
	char acCardID[32] = {0}; 
	char acSn[32] = {0};
	char acProvider[32] = {0};
	char acAlias[32] = {0};
	char acModule[32] = {0};
	char acLocalIp[32] = {0};
	char acLocalMac[32] = {0};
	char acPsd[32] = {0};
	char acServerIp[32] = {0};
	int nServerPort = 0;
	char acVideoType[32] = {0};
	char acBrowserver[32] = {0};
	char acTemp[32] = {0};
	char acValue[32] = {0};
	int nValue;
	CSUDIEW200SClientInfoType_E eType;
	
	CSTK_ASSERT_TRUE(CS_TK_CONFIG_SUCCESS == CSTKGetConfigInfo("EW200Service","CSUDICFG_CARDID", acCardID, sizeof(acCardID)), "从配置文件读取只能卡号失败\n");
	
	CSTK_ASSERT_TRUE(CS_TK_CONFIG_SUCCESS == CSTKGetConfigInfo("EW200Service","CSUDICFG_SN", acSn, sizeof(acSn)), "从配置文件读取序列号失败\n");
	
	CSTK_ASSERT_TRUE(CS_TK_CONFIG_SUCCESS == CSTKGetConfigInfo("EW200Service","CSUDICFG_PROVIDER", acProvider, sizeof(acProvider)), "从配置文件读取终端提供商信息失败\n");
	
	CSTK_ASSERT_TRUE(CS_TK_CONFIG_SUCCESS == CSTKGetConfigInfo("EW200Service","CSUDICFG_ALIAS", acAlias, sizeof(acAlias)), "从配置文件读取终端别名失败\n");
	
	CSTK_ASSERT_TRUE(CS_TK_CONFIG_SUCCESS == CSTKGetConfigInfo("EW200Service","CSUDICFG_MODULE", acModule, sizeof(acModule)), "从配置文件读取终端模型失败\n")
	
	CSTK_ASSERT_TRUE(CS_TK_CONFIG_SUCCESS == CSTKGetConfigInfo("EW200Service","CSUDICFG_LOCAL_IP", acLocalIp, sizeof(acLocalIp)), "从配置文件读取本地IP地址失败\n");
	
	CSTK_ASSERT_TRUE(CS_TK_CONFIG_SUCCESS == CSTKGetConfigInfo("EW200Service","CSUDICFG_LOCAL_MAC", acLocalMac, sizeof(acLocalMac)), "从配置文件读取本地MAC地址失败\n");
	
	CSTK_ASSERT_TRUE(CS_TK_CONFIG_SUCCESS == CSTKGetConfigInfo("EW200Service","CSUDICFG_CITV_PSD", acPsd, sizeof(acPsd)), "从配置文件读取用户密码失败\n");
	
	CSTK_ASSERT_TRUE(CS_TK_CONFIG_SUCCESS == CSTKGetConfigInfo("EW200Service","CSUDICFG_CITV_SERVERIP", acServerIp, sizeof(acServerIp)), "从配置文件读取服务器IP失败\n");
	
	CSTK_ASSERT_TRUE(CS_TK_CONFIG_SUCCESS == CSTKGetConfigInfo("EW200Service","CSUDICFG_CITV_SERVERPORT", acTemp, sizeof(acTemp)), "从配置文件读取服务器端口号失败\n");
	nServerPort = CSTKGetIntFromStr(acTemp, 10);
		
	CSTK_ASSERT_TRUE(CS_TK_CONFIG_SUCCESS == CSTKGetConfigInfo("EW200Service","CSUDICFG_CITV_VIDEOTYPE", acVideoType, sizeof(acVideoType)), "从配置文件读取视频类型失败\n")
	
	CSTK_ASSERT_TRUE(CS_TK_CONFIG_SUCCESS == CSTKGetConfigInfo("EW200Service","CSUDICFG_CITV_BROWSERVER", acBrowserver, sizeof(acBrowserver)), "从配置文件读取浏览器版本号失败\n");

	eType = EM_UDIEW200S_CARDID;
	CSTK_ASSERT_TRUE_FATAL(-1 != CSUDIEW200SReadClientInfo(eType, (void*)acValue, sizeof(acValue)), "读取智能卡信息失败\n"	);
	CSTK_ASSERT_TRUE_FATAL(0 == strcmp(acValue, acCardID), "读取出的序列号信息错误\n");

	eType = EM_UDIEW200S_SN;
	memset(acValue, 0, sizeof(acValue));
	CSTK_ASSERT_TRUE_FATAL(-1 != CSUDIEW200SReadClientInfo(eType, (void*)acValue, sizeof(acValue)), "读取序列号信息失败\n"	);
	CSTK_ASSERT_TRUE_FATAL(0 == strcmp(acValue, acSn), "读取出的智能卡信息错误\n");

	eType = EM_UDIEW200S_PROVIDER;
	memset(acValue, 0, sizeof(acValue));
	CSTK_ASSERT_TRUE_FATAL(-1 != CSUDIEW200SReadClientInfo(eType, (void*)acValue, sizeof(acValue)), "读取供应商信息失败\n"	);
	CSTK_ASSERT_TRUE_FATAL(0 == strcmp(acValue,  acProvider), "读取出的供应商信息错误\n");

	eType = EM_UDIEW200S_ALIAS;
	memset(acValue, 0, sizeof(acValue));
	CSTK_ASSERT_TRUE_FATAL(-1 != CSUDIEW200SReadClientInfo(eType, (void*)acValue, sizeof(acValue)), "读取终端别名信息失败\n"	);
	CSTK_ASSERT_TRUE_FATAL(0 == strcmp(acValue,  acAlias), "读取出的终端别名信息错误\n");

	eType = EM_UDIEW200S_MODULE;
	memset(acValue, 0, sizeof(acValue));
	CSTK_ASSERT_TRUE_FATAL(-1 != CSUDIEW200SReadClientInfo(eType, (void*)acValue, sizeof(acValue)), "读取终端型号信息失败\n"	);
	CSTK_ASSERT_TRUE_FATAL(0 == strcmp(acValue,  acModule), "读取出的终端型号信息错误\n");

	eType = EM_UDIEW200S_LOCAL_IP;
	memset(acValue, 0, sizeof(acValue));
	CSTK_ASSERT_TRUE_FATAL(-1 != CSUDIEW200SReadClientInfo(eType, (void*)acValue, sizeof(acValue)), "读取本机IP地址失败\n"	);
	CSTK_ASSERT_TRUE_FATAL(0 == strcmp(acValue,  acLocalIp), "读取出的本机IP地址错误\n");

	eType = EM_UDIEW200S_LOCAL_MAC;
	memset(acValue, 0, sizeof(acValue));
	CSTK_ASSERT_TRUE_FATAL(-1 != CSUDIEW200SReadClientInfo(eType, (void*)acValue, sizeof(acValue)), "读取本机MAC地址失败\n"	);
	CSTK_ASSERT_TRUE_FATAL(0 == strcmp(acValue,  acLocalMac), "读取出的本机MAC地址错误\n");

	eType = EM_UDIEW200S_CITV_PSD;
	memset(acValue, 0, sizeof(acValue));
	CSTK_ASSERT_TRUE_FATAL(-1 != CSUDIEW200SReadClientInfo(eType, (void*)acValue, sizeof(acValue)), "读取用户密码失败\n"	);
	CSTK_ASSERT_TRUE_FATAL(0 == strcmp(acValue,  acPsd), "读取出的用户密码错误\n");

	eType = EM_UDIEW200S_CITV_SERVERIP;
	memset(acValue, 0, sizeof(acValue));
	CSTK_ASSERT_TRUE_FATAL(-1 != CSUDIEW200SReadClientInfo(eType, (void*)acValue, sizeof(acValue)), "读取服务器IP失败\n"	);
	CSTK_ASSERT_TRUE_FATAL(0 == strcmp(acValue,  acServerIp), "读取出的服务器IP错误\n");

	eType = EM_UDIEW200S_CITV_SERVERPORT;
	CSTK_ASSERT_TRUE_FATAL(-1 != CSUDIEW200SReadClientInfo(eType, (void*)&nValue, sizeof(int)), "读取服务器端口号失败\n"	);
	CSTK_ASSERT_TRUE_FATAL(nValue == nServerPort, "读取出的服务器端口号错误\n");

	eType = EM_UDIEW200S_CITV_VIDEOTYPE;
	memset(acValue, 0, sizeof(acValue));
	CSTK_ASSERT_TRUE_FATAL(-1 != CSUDIEW200SReadClientInfo(eType, (void*)acValue, sizeof(acValue)), "读取视频解码格式失败\n"	);
	CSTK_ASSERT_TRUE_FATAL(0 == strcmp(acValue,  acVideoType), "读取出的视频解码格式错误\n");
	
	eType = EM_UDIEW200S_CITV_BROWSERVER;
	memset(acValue, 0, sizeof(acValue));
	CSTK_ASSERT_TRUE_FATAL(-1 != CSUDIEW200SReadClientInfo(eType, (void*)acValue, sizeof(acValue)), "读取浏览器版本号失败\n"	);
	CSTK_ASSERT_TRUE_FATAL(0 == strcmp(acValue,  acBrowserver), "读取出的浏览器版本号错误\n");

	CSTK_FATAL_POINT
		
	return TRUE;
}

//@CASEGROUP:CSUDIEW200SReadClientInfo 
//@DESCRIPTION: 测试函数CSUDIEW200SReadClientInfo与CSUDIEW200SWriteClientInfo在参数都正确时的执行效果(针对userid和token的测试)
//@PRECONDITION:
//@INPUT:1、EM_UDIEW200S_CITV_USERID, acValue,sizeof(acValue)
//@INPUT:2、EM_UDIEW200S_CITV_TOKEN,acValue,sizeof(acValue)
//@INPUT:3、读取出的信息长度为sizeof(acValue)
//@EXPECTATION:期望写入的值与读取出的值相等
//@EXECUTIONFLOW:1、调用CSUDIEW200SWriteClientInfo写入USERID，然后调用CSUDIEW200SReadClientInfo读取USERID，期望写入的信息与读取出的信息相等
//@EXECUTIONFLOW:2、调用CSUDIEW200SWriteClientInfo写入TOKEN，然后调用CSUDIEW200SReadClientInfo读取TOKEN，期望写入的信息与读取出的信息相等
BOOL CSTC_EW200SERVICE_MT_0002(void)
{
	char* pcUserID = "123456";
	char* pcToken = "555555";
	char acValue[32] = {0};
	
	CSTK_ASSERT_TRUE_FATAL(-1 != CSUDIEW200SWriteClientInfo(EM_UDIEW200S_CITV_USERID, (void*)pcUserID, strlen(pcUserID)), "写入用户ID失败\n");
	CSTK_ASSERT_TRUE_FATAL(-1 != CSUDIEW200SReadClientInfo(EM_UDIEW200S_CITV_USERID, (void*)acValue, sizeof(acValue)), "读取用户ID失败\n"	);
	CSTK_ASSERT_TRUE_FATAL(0 == strcmp(pcUserID, acValue), "写入的用户ID错误");

	memset(acValue, 0 , sizeof(acValue));
	CSTK_ASSERT_TRUE_FATAL(-1 != CSUDIEW200SWriteClientInfo(EM_UDIEW200S_CITV_TOKEN, (void*)pcToken, strlen(pcToken)), "写入令牌信息失败\n");
	CSTK_ASSERT_TRUE_FATAL(-1 != CSUDIEW200SReadClientInfo(EM_UDIEW200S_CITV_TOKEN, (void*)acValue, sizeof(acValue)), "读取令牌信息失败\n");
	CSTK_ASSERT_TRUE_FATAL(0 == strcmp(pcToken, acValue), "写入的令牌错误");

	CSTK_FATAL_POINT
		
	return TRUE;
}

//@CASEGROUP:CSUDIEW200SReadClientInfo 
//@DESCRIPTION: 测试函数CSUDIEW200SReadClientInfo在输入参数不正确时的执行效果
//@PRECONDITION:None
//@INPUT:输入不符合接口设计要求的参数组合
//@INPUT:1、eType=EM_UDIEW200S_CARDID-1, acValue,sizeof(acValue)
//@INPUT:2、eType=EM_UDIEW200S_PROGRAMEINFO+1, acValue,sizeof(acValue)
//@INPUT:3、eType=EM_UDIEW200S_CARDID,acValue,length=0
//@EXPECTATION:期望上述三种情况下函数CSUDIEW200SReadClientInfo的返回都为-1
//@EXECUTIONFLOW: 1、输入参数eType=EM_UDIEW200S_CARDID-1、acValue、sizeof(acValue)，调用CSUDIEW200SReadClientInfo期望函数返回值为-1
//@EXECUTIONFLOW: 2、输入参数eType=EM_UDIEW200S_PROGRAMEINFO+1、acValue、sizeof(acValue)，调用CSUDIEW200SReadClientInfo期望函数返回值为-1
//@EXECUTIONFLOW: 3、输入参数EM_UDIEW200S_CARDID、acValue、0，调用CSUDIEW200SReadClientInfo期望函数返回值为-1
BOOL CSTC_EW200SERVICE_MT_0003(void)
{
	char acValue[32] = {0};
	
	CSTK_ASSERT_TRUE_FATAL(-1 == CSUDIEW200SReadClientInfo(EM_UDIEW200S_CARDID-1, (void*)acValue, sizeof(acValue)), "步骤1失败");
	
	CSTK_ASSERT_TRUE_FATAL(-1 == CSUDIEW200SReadClientInfo(EM_UDIEW200S_PROGRAMEINFO+1, (void*)acValue, sizeof(acValue)), "步骤2失败");
	
	CSTK_ASSERT_TRUE_FATAL(-1 == CSUDIEW200SReadClientInfo(EM_UDIEW200S_CARDID, (void*)acValue, 0), "步骤3失败");

	CSTK_FATAL_POINT
		
	return TRUE;
}

//@CASEGROUP:CSUDIEW200SWriteClientInfo
//@DESCRIPTION: 测试函数CSUDIEW200SWriteClientInfo在参数都符合设计要求时的执行效果
//@PRECONDITION:信号线正常连接，供测试的码流正常播放
//@INPUT:输入参数均符合设计要求的参数组合
//@INPUT:1、输入CSUDIEW200SClientInfoType_E的值为EM_UDIEW200S_ISMUTE
//@INPUT:2、要写入的eType的信息分别为CSUDI_TRUE和CSUDI_FALSE
//@INPUT:3、写入的信息长度为sizeof(int)
//@EXPECTATION:成功写入信息，且写入的信息生效，测试人员能看到写入信息的相应效果
//@EXECUTIONFLOW: 1、调用函数CSUDIEW200SWriteClientInfo，写入静音消息CSUDI_TRUE
//@EXECUTIONFLOW: 2、调用CSUDIEW200SPlayStream播放节目,期望测试人员听到的音频效果为静音
//@EXECUTIONFLOW: 3、调用CSTKWaitYes,等待测试人员对写入CSUDI_TRUE执行效果的反馈
//@EXECUTIONFLOW: 4、调用函数CSUDIEW200SWriteClientInfo，写入静音消息CSUDI_FALSE，期望测试人员看到的视频效果为非静音
//@EXECUTIONFLOW: 5、调用CSTKWaitYes,等待测试人员对执行效果的反馈
BOOL CSTC_EW200SERVICE_MT_0004(void)
{
	CSUDI_BOOL bIsmute = CSUDI_TRUE;
	CSUDIEW200SClientInfoType_E eType = EM_UDIEW200S_ISMUTE;
	
	CSTK_ASSERT_TRUE_FATAL(-1 != CSUDIEW200SWriteClientInfo(eType, (void*)&bIsmute, sizeof(int)), "步骤1失败");
	CSUDIEW200SPlayStream(s_pcTestDvbcUrl, 0);
	CSTCPrint("音频播放为静音吗？\n");
	CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes(), "设置静音无效");
	
	bIsmute = CSUDI_FALSE;
	CSTK_ASSERT_TRUE_FATAL(-1 != CSUDIEW200SWriteClientInfo(eType, (void*)&bIsmute, sizeof(int)), "步骤4失败");
	CSTCPrint("音频播放有声音吗？\n");
	CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes(), "设置非静音无效");                      
	CSUDIEW200SStopPlay(0);
	
	CSTK_FATAL_POINT

	CSUDIEW200SStopPlay(0);
		
	return TRUE;
}

//@CASEGROUP:CSUDIEW200SWriteClientInfo
//@DESCRIPTION: 测试函数CSUDIEW200SWriteClientInfo在参数都符合设计要求时的执行效果
//@PRECONDITION:信号线正常连接，供测试的码流正常播放
//@INPUT:输入参数均符合设计要求的参数组合
//@INPUT:1、输入CSUDIEW200SClientInfoType_E的值为EM_UDIEW200S_VOLUME
//@INPUT:2、要写入的eType的信息value为0-31之间的值依次增大
//@INPUT:3、写入的信息长度为sizeof(int)
//@EXPECTATION:成功写入信息，且写入的信息生效，测试人员能看到写入信息的相应效果
//@EXECUTIONFLOW: 1、调用CSUDIEW200SPlayStream播放视频
//@EXECUTIONFLOW: 2、调用CSUDIEW200SWriteClientInfo从0-31每一步加5循环写入音量值，期待测试人员听到声音渐强
//@EXECUTIONFLOW: 3、每次改变值后调用CSUDIOSThreadSleep等待5s，让测试人员看效果
//@EXECUTIONFLOW: 4、调用CSTKWaitYes,等待测试人员反馈写入音量的效果
//@EXECUTIONFLOW: 5、调用CSUDIEW200SStopPlay停止视频的播放
BOOL CSTC_EW200SERVICE_MT_0005(void)
{
	CSUDIEW200SClientInfoType_E eType  = EM_UDIEW200S_VOLUME;
	int  nVolume = 0;

	CSUDIEW200SPlayStream(s_pcTestDvbcUrl, 0);
	while(nVolume <= 31)
	{
		CSTK_ASSERT_TRUE_FATAL(-1 != CSUDIEW200SWriteClientInfo(eType, (void*)&nVolume, sizeof(int)), "步骤2失败");
		nVolume += 5;
		CSUDIOSThreadSleep(5000);
	}

	CSTCPrint("音频的声音出现声音渐强的情况了吗？\n");
	CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes(), "写入音量无效");                      			  
	CSUDIEW200SStopPlay(0);
	
	CSTK_FATAL_POINT

	CSUDIEW200SStopPlay(0);
		
	return TRUE;    
}

//@CASEGROUP:CSUDIEW200SWriteClientInfo
//@DESCRIPTION: 测试函数CSUDIEW200SWriteClientInfo在参数都符合设计要求时的执行效果
//@PRECONDITION:信号线正常连接，供测试的码流正常播放
//@INPUT:输入参数均符合设计要求的参数组合
//@INPUT:1、输入CSUDIEW200SClientInfoType_E的值为EM_UDIEW200S_VIDEOALPHA
//@INPUT:2、要写入的eType的信息value
//@INPUT:3、写入的信息长度为sizeof(int)
//@EXPECTATION:成功写入信息，且写入的信息生效，测试人员能看到写入信息的相应效果
//@EXECUTIONFLOW: 1、调用CSUDIEW200SPlayStream播放视频
//@EXECUTIONFLOW: 2、调用CSUDIEW200SWriteClientInfo从0-100每一次加10循环写入透明度的值，期待测试人员看到视频由黑变亮的效果
//@EXECUTIONFLOW: 3、每次改变值后调用CSUDIOSThreadSleep等待5s，让测试人员看效果
//@EXECUTIONFLOW: 4、调用CSTKWaitYes,等待测试人员反馈写入透明度的效果
//@EXECUTIONFLOW: 5、调用CSUDIEW200SWriteClientInfo,写入透明度值为50，使视频正常
//@EXECUTIONFLOW: 6、调用CSUDIEW200SStopPlay停止视频的播放
BOOL CSTC_EW200SERVICE_MT_0006(void)
{
	
	int  nVideoAlpha = 0;
	CSUDIEW200SClientInfoType_E eType = EM_UDIEW200S_VIDEOALPHA;

	CSUDIEW200SPlayStream(s_pcTestDvbcUrl, 0);	
	
	while(nVideoAlpha <= 100)
	{			
		CSTK_ASSERT_TRUE_FATAL(-1 != CSUDIEW200SWriteClientInfo(eType, (void*)&nVideoAlpha, sizeof(int)), "步骤2失败");
		nVideoAlpha += 10;	
		CSUDIOSThreadSleep(5000);
	}
	
	CSTCPrint("开始播放视频后，视频由全黑逐渐变得透明吗？\n");
	CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes(), "写入透明度无效");        

	nVideoAlpha = 50;
	CSTK_ASSERT_TRUE_FATAL(-1 != CSUDIEW200SWriteClientInfo(eType, (void*)&nVideoAlpha, sizeof(int)), "写入透明度失败");
	CSUDIEW200SStopPlay(0);
	
	CSTK_FATAL_POINT	
	
	CSUDIEW200SStopPlay(0);	
	
	return TRUE;    
}

//@CASEGROUP:CSUDIEW200SWriteClientInfo
//@DESCRIPTION: 测试函数CSUDIEW200SWriteClientInfo在参数都符合设计要求时的执行效果
//@PRECONDITION:信号线正常连接，供测试的码流正常播放
//@INPUT:输入参数均符合设计要求的参数组合
//@INPUT:1、输入CSUDIEW200SClientInfoType_E的值为EM_UDIEW200S_BRIGHTNESS
//@INPUT:2、要写入的eType的信息value
//@INPUT:3、写入的信息长度为sizeof(int)
//@EXPECTATION:成功写入信息，且写入的信息生效，测试人员能看到写入信息的相应效果
//@EXECUTIONFLOW: 1、调用CSUDIEW200SPlayStream播放视频
//@EXECUTIONFLOW: 2、调用CSUDIEW200SWriteClientInfo从0-100每一次加10循环写入亮度的值，期待测试人员看到视频由黑变清晰，再变成全白的效果
//@EXECUTIONFLOW: 3、每次改变值后调用CSUDIOSThreadSleep等待5s，让测试人员看效果
//@EXECUTIONFLOW: 4、调用CSTKWaitYes,等待测试人员反馈写入亮度的效果
//@EXECUTIONFLOW: 5、调用CSUDIEW200SWriteClientInfo,写入亮度值为50，使视频正常
//@EXECUTIONFLOW: 6、调用CSUDIEW200SStopPlay停止视频的播放
BOOL CSTC_EW200SERVICE_MT_0007(void)
{
	int  nBrightness = 0;
	CSUDIEW200SClientInfoType_E eType = EM_UDIEW200S_BRIGHTNESS;

	CSUDIEW200SPlayStream(s_pcTestDvbcUrl, 0);
	while(nBrightness <= 100)
	{
		CSTK_ASSERT_TRUE_FATAL(-1 != CSUDIEW200SWriteClientInfo(eType, (void*)&nBrightness, sizeof(int)), "步骤2失败");
		nBrightness += 10;
		CSUDIOSThreadSleep(5000);
	}
	CSTCPrint("视频开始播放后，视频出现由黑变清晰，再变成全白吗？\n");
	CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes(), "写入视频亮度无效");                      			  

	nBrightness = 50;
	CSTK_ASSERT_TRUE_FATAL(-1 != CSUDIEW200SWriteClientInfo(eType, (void*)&nBrightness, sizeof(int)), "步骤5失败");
	CSUDIEW200SStopPlay(0);
	
	CSTK_FATAL_POINT

	CSUDIEW200SStopPlay(0);
		
	return TRUE;    
}

//@CASEGROUP:CSUDIEW200SWriteClientInfo
//@DESCRIPTION: 测试函数CSUDIEW200SWriteClientInfo在参数都符合设计要求时的执行效果
//@PRECONDITION:信号线正常连接，供测试的码流正常播放
//@INPUT:输入参数均符合设计要求的参数组合
//@INPUT:1、输入CSUDIEW200SClientInfoType_E的值为EM_UDIEW200S_CONTRAST
//@INPUT:2、要写入的eType的信息value
//@INPUT:3、写入的信息长度为sizeof(int)
//@EXPECTATION:成功写入信息，且写入的信息生效，测试人员能看到写入信息的相应效果
//@EXECUTIONFLOW: 1、调用CSUDIEW200SPlayStream播放视频
//@EXECUTIONFLOW: 2、调用CSUDIEW200SWriteClientInfo从0-100每一次加10写入对比度的值，期待测试人员看到视频由不清晰变清晰的效果
//@EXECUTIONFLOW: 3、每次改变值后调用CSUDIOSThreadSleep等待5s，让测试人员看效果
//@EXECUTIONFLOW: 4、调用CSTKWaitYes,等待测试人员反馈写入对比度的效果
//@EXECUTIONFLOW: 5、调用CSUDIEW200SWriteClientInfo,写入对比度值为50，使视频正常
//@EXECUTIONFLOW: 6、调用CSUDIEW200SStopPlay停止视频的播放
BOOL CSTC_EW200SERVICE_MT_0008(void)
{
	int  nContrast = 0;
	CSUDIEW200SClientInfoType_E eType = EM_UDIEW200S_CONTRAST;

	CSUDIEW200SPlayStream(s_pcTestDvbcUrl, 0);
	while(nContrast <= 100)
	{
		CSTK_ASSERT_TRUE_FATAL(-1 != CSUDIEW200SWriteClientInfo(eType, (void*)&nContrast, sizeof(int)), "步骤2失败");
		 nContrast += 10;
		 CSUDIOSThreadSleep(5000);
	}
	CSTCPrint("开始播放视频后，视频由不清晰变得逐渐清晰吗？\n");
	CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes(), "写入视频对比度无效");                      			  

	nContrast = 50;
	CSTK_ASSERT_TRUE_FATAL(-1 != CSUDIEW200SWriteClientInfo(eType, (void*)&nContrast, sizeof(int)), "步骤5失败");
	CSUDIEW200SStopPlay(0);
	
	CSTK_FATAL_POINT

	CSUDIEW200SStopPlay(0);
		
	return TRUE;    
}

//@CASEGROUP:CSUDIEW200SWriteClientInfo
//@DESCRIPTION: 测试函数CSUDIEW200SWriteClientInfo在参数都符合设计要求时的执行效果
//@PRECONDITION:信号线正常连接，供测试的码流正常播放
//@INPUT:输入参数均符合设计要求的参数组合
//@INPUT:1、输入CSUDIEW200SClientInfoType_E的值为EM_UDIEW200S_SATURATION
//@INPUT:2、要写入的eType的信息value
//@INPUT:3、写入的信息长度为sizeof(int)
//@EXPECTATION:成功写入信息，且写入的信息生效，测试人员能看到写入信息的相应效果
//@EXECUTIONFLOW: 1、调用CSUDIEW200SPlayStream播放视频
//@EXECUTIONFLOW: 2、调用CSUDIEW200SWriteClientInfo从0-100每一次加10写入饱和度的值，期待测试人员看到视频由灰变得逐渐清晰的效果
//@EXECUTIONFLOW: 3、每次改变值后调用CSUDIOSThreadSleep等待5s，让测试人员看效果
//@EXECUTIONFLOW: 4、调用CSTKWaitYes,等待测试人员反馈写入饱和度的效果
//@EXECUTIONFLOW: 5、调用CSUDIEW200SWriteClientInfo,写入饱和度值为50，使得视频能看见
//@EXECUTIONFLOW: 6、调用CSUDIEW200SStopPlay停止视频的播放
BOOL CSTC_EW200SERVICE_MT_0009(void)
{
	int  nSaturation = 0;
	CSUDIEW200SClientInfoType_E eType = EM_UDIEW200S_SATURATION;

	CSUDIEW200SPlayStream(s_pcTestDvbcUrl, 0);
	while(nSaturation <= 100)
	{
		CSTK_ASSERT_TRUE_FATAL(-1 != CSUDIEW200SWriteClientInfo(eType, (void*)&nSaturation, sizeof(int)), "步骤2失败");
		nSaturation += 10;
		CSUDIOSThreadSleep(5000);
	}
	CSTCPrint("开始播放视频后，视频颜色对比度由灰变得逐渐清晰吗？\n");
	CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes(), "写入视频饱和度无效\n");                      			  

	nSaturation = 50;
	CSTK_ASSERT_TRUE_FATAL(-1 != CSUDIEW200SWriteClientInfo(eType, (void*)&nSaturation, sizeof(int)), "步骤5失败\n");
	CSUDIEW200SStopPlay(0);
	
	CSTK_FATAL_POINT

	CSUDIEW200SStopPlay(0);
		
	return TRUE;    
}
	
//@CASEGROUP:CSUDIEW200SWriteClientInfo
//@DESCRIPTION: 测试函数CSUDIEW200SWriteClientInfo在参数都符合设计要求时的执行效果
//@PRECONDITION:信号线正常连接，供测试的码流正常播放
//@INPUT:输入参数均符合设计要求的参数组合
//@INPUT:1、输入CSUDIEW200SClientInfoType_E的值为EM_UDIEW200S_ASPECTRATIO
//@INPUT:2、要写入的eType的信息value
//@INPUT:3、写入的信息长度为sizeof(int)
//@EXPECTATION:成功写入信息，且写入的信息生效，测试人员能看到写入信息的相应效果
//@EXECUTIONFLOW: 1、调用CSUDIEW200SWriteClientInfo写入信息EM_UDIEW200S_WIN_ASPECT_RATIO_4_3
//@EXECUTIONFLOW: 2、调用CSUDIEW200SPlayStream,期望测试人员看到的效果为屏幕宽高比为4:3
//@EXECUTIONFLOW: 3、调用CSUDIEW200SWriteClientInfo写入信息EM_UDIEW200S_WIN_ASPECT_RATIO_16_9
//@EXECUTIONFLOW: 4、调用CSUDIEW200SPlayStream,期望测试人员看到的效果为屏幕宽高比为16:9
//@EXECUTIONFLOW: 5、调用CSTKWaitYes,等待测试人员反馈写入不同视频的效果
//@EXECUTIONFLOW: 6、调用CSUDIEW200SStopPlay停止播放视频
BOOL CSTC_EW200SERVICE_MT_0010(void)
{
	int  nCSUDI_ASPECTRATIO = EM_UDIEW200S_WIN_ASPECT_RATIO_4_3;
	CSUDIEW200SClientInfoType_E eType = EM_UDIEW200S_ASPECTRATIO;
	CSTK_ASSERT_TRUE_FATAL(-1 != CSUDIEW200SWriteClientInfo(eType, (void*)&nCSUDI_ASPECTRATIO, sizeof(int)), "步骤1失败");
	CSUDIEW200SPlayStream(s_pcTestDvbcUrl, 0);
	CSTCPrint("视频宽高比为4:3吗？\n");
	CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes(), "写入的视频宽高比4:3无效");  
	
	nCSUDI_ASPECTRATIO = EM_UDIEW200S_WIN_ASPECT_RATIO_16_9;
	CSTK_ASSERT_TRUE_FATAL(-1 != CSUDIEW200SWriteClientInfo(eType, (void*)&nCSUDI_ASPECTRATIO, sizeof(int)), "步骤3失败");
	CSTCPrint("视频宽高比为16:9吗？\n");
	CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes(), "写入视频宽高比16:9无效");       
	CSUDIEW200SStopPlay(0);
	
	CSTK_FATAL_POINT
		
	return TRUE;    
}

//@CASEGROUP:CSUDIEW200SWriteClientInfo
//@DESCRIPTION: 测试函数CSUDIEW200SWriteClientInfo在参数都符合设计要求时的执行效果
//@PRECONDITION:信号线正常连接，供测试的码流正常播放
//@INPUT:输入参数均符合设计要求的参数组合
//@INPUT:1、输入CSUDIEW200SClientInfoType_E枚举中的值EM_UDIEW200S_STOPMODE
//@INPUT:2、要写入的eType的信息value
//@INPUT:3、写入的信息长度为sizeof(int)
//@EXPECTATION:成功写入信息，且写入的信息生效，测试人员能看到写入信息的相应效果
//@EXECUTIONFLOW: 1、调用CSUDIEW200SWriteClientInfo写入eType=EM_UDIEW200S_STOPMODE时的信息EM_UDIEW200S_STOPMODE_BLACK
//@EXECUTIONFLOW: 2、调用CSUDIEW200SPlayStream播放视频 
//@EXECUTIONFLOW: 3、调用CSUDIOSThreadSleep等待5s
//@EXECUTIONFLOW: 4、调用CSUDIEW200SStopPlay停止视频播放，期望测试人员看到视频停止播放时的效果为黑屏
//@EXECUTIONFLOW: 5、调用CSTKWaitYes,等待测试人员反馈看到的视频停止时的效果为黑屏
//@EXECUTIONFLOW: 6、调用CSUDIEW200SWriteClientInfo写入eType=EM_UDIEW200S_STOPMODE时的信息EM_UDIEW200S_STOPMODE_FREEZE
//@EXECUTIONFLOW: 7、调用CSUDIEW200SPlayStream播放视频
//@EXECUTIONFLOW: 8、调用CSUDIOSThreadSleep等待5s
//@EXECUTIONFLOW: 9、调用CSUDIEW200SStopPlay停止视频播放，期望测试人员看到视频停止播放时的效果为静帧
//@EXECUTIONFLOW: 10、调用CSTKWaitYes,等待测试人员反馈看到的视频停止时的效果为静帧
BOOL CSTC_EW200SERVICE_MT_0011(void)
{
	int  nCSUDI_STOPMODE = EM_UDIEW200S_STOPMODE_BLACK;
	CSUDIEW200SClientInfoType_E eType = EM_UDIEW200S_STOPMODE;
	CSTK_ASSERT_TRUE_FATAL(-1 != CSUDIEW200SWriteClientInfo(eType, (void*)&nCSUDI_STOPMODE, sizeof(int)), "步骤1失败");
	CSUDIEW200SPlayStream(s_pcTestDvbcUrl, 0);
	CSUDIOSThreadSleep(5000);
	CSUDIEW200SStopPlay(0);
	CSTCPrint("视频停止时为黑屏吗？\n");
	CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes(), "写入的停止播放时视频类型无效"); 
	
	nCSUDI_STOPMODE = EM_UDIEW200S_STOPMODE_FREEZE;
	CSTK_ASSERT_TRUE_FATAL(-1 != CSUDIEW200SWriteClientInfo(eType, (void*)&nCSUDI_STOPMODE, sizeof(int)), "步骤6失败");
	CSUDIEW200SPlayStream(s_pcTestDvbcUrl, 0);
	CSUDIOSThreadSleep(5000);
	CSUDIEW200SStopPlay(0);
	CSTCPrint("视频停止时为静帧吗？\n");
	CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes(), "写入停止播放时视频类型无效"); 
	
	CSTK_FATAL_POINT
		
	return TRUE;    
}

//@CASEGROUP:CSUDIEW200SWriteClientInfo 
//@DESCRIPTION: 测试函数CSUDIEW200SWriteClientInfo在输入参数不正确时的执行效果
//@PRECONDITION:None
//@INPUT:输入不符合接口设计要求的参数组合
//@INPUT:1、eType=EM_UDIEW200S_CARDID-1，其他参数正确
//@INPUT:2、eType=EM_UDIEW200S_PROGRAMEINFO+1，其他参数正确
//@INPUT:3、length=0，其他参数正确
//@INPUT:4、value非法时，eType=EM_UDIEW200S_AUDIOPID,value=-1,length=sizeof(int)
//@EXPECTATION:函数CSUDIEW200SWriteClientInfo返回-1
//@EXECUTIONFLOW: 1、输入参数eType=EM_UDIEW200S_CARDID-1，value = 700092200002, length = 32,调用函数CSUDIEW200SWriteClientInfo写入信息，期望函数返回值为-1
//@EXECUTIONFLOW: 2、输入参数eType=EM_UDIEW200S_PROGRAMEINFO+1，value = 700092200002, length = 32,调用函数CSUDIEW200SWriteClientInfo写入信息，期望函数返回值为-1
//@EXECUTIONFLOW: 3、输入参数eType=EM_UDIEW200S_CARDID，value = 700092200002, length = 0，调用CSUDIEW200SWriteClientInfo写入信息，期望函数返回值为-1
//@EXECUTIONFLOW: 4、输入参数eType=EM_UDIEW200S_CARDID，value = NULL, length = 32，调用CSUDIEW200SWriteClientInfo写入信息，期望函数返回值为-1
BOOL CSTC_EW200SERVICE_MT_0012(void)
{
	CSTK_ASSERT_TRUE_FATAL(-1 == CSUDIEW200SWriteClientInfo(EM_UDIEW200S_CARDID-1, "700092200002", 32), "步骤1失败");

	CSTK_ASSERT_TRUE_FATAL(-1 == CSUDIEW200SWriteClientInfo(EM_UDIEW200S_PROGRAMEINFO+1, "700092200002", 32), "步骤2失败");
	
	CSTK_ASSERT_TRUE_FATAL(-1 == CSUDIEW200SWriteClientInfo(EM_UDIEW200S_CARDID, "700092200002", 0), "步骤3失败");
	
	CSTK_ASSERT_TRUE_FATAL(-1 == CSUDIEW200SWriteClientInfo(EM_UDIEW200S_AUDIOPID, NULL, 32), "步骤4失败");

	CSTK_FATAL_POINT
		
	return TRUE;
}

static void  TunerCallback ( CSUDITunerType_E eType, CSUDITunerCallbackMessage_S* psMessage)
{
	if (psMessage == NULL )
		return ;

	if(psMessage->m_pvUserData == (void*)s_nUserData)
	{
		
		switch (psMessage->m_eEvent)
		{
		case EM_UDITUNER_SIGNAL_CONNECTED:
			{
				s_nTestData = 0;                        //锁频成功
			}
			break;
		case EM_UDITUNER_SIGNAL_LOST:
			{
				s_nTestData = 1;                       //锁定转为失锁
			}
			break;
		default:
			break;
		}
	}
}

//@CASEGROUP:CSUDIEW200SGetTuneParam
//@DESCRIPTION: 测试函数CSUDIEW200SGetTuneParam在输入正确参数时的执行结果
//@PRECONDITION:信号线正常连接，且有信号
//@INPUT:1、 nOrNetId、 nTsId、nServiceId
//@INPUT:2、CSUDITunerAddCallback
//@INPUT:3、CSUDITunerConnect
//@INPUT:4、回调消息响应测试值s_ntestData
//@EXPECTATION:调用CSUDIEW200SGetTuneParam，期望返回CSUDI_TRUE
//@EXECUTIONFLOW: 1、调用CSUDIEW200SGetTuneParam，获得频点信息
//@EXECUTIONFLOW: 2、调用 CSUDITunerAddCallback与CSUDITunerConnect，利用获取到的信息进行锁频，锁频成功则获取的数据源信息有效
//@EXECUTIONFLOW: 3、调用 CSUDIOSThreadSleep等待2s
//@EXECUTIONFLOW: 4、查看回调消息响应的信息s_ntestData的值与期望值相等
BOOL CSTC_EW200SERVICE_MT_0013(void)
{
	int nTsId =1;
	int nOrNetId = 100;
	int nServiceId = 103;
	CSUDI_Error_Code ret;
	CSUDIEW200SSourceParam_S sSourceParam;
	memset(&sSourceParam, 0, sizeof( sSourceParam));
	
	ret = CSUDIEW200SGetTuneParam(nOrNetId, nTsId, nServiceId, &sSourceParam);
	CSTK_ASSERT_TRUE_FATAL(ret == CSUDI_SUCCESS, "步骤1失败\n");
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDITunerAddCallback( sSourceParam.m_dwTunerId, TunerCallback, (void *)s_nUserData), "注册回调失败\n");
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDITunerConnect(sSourceParam.m_dwTunerId, &sSourceParam.m_punDeliver), "锁频失败\n");
	CSUDIOSThreadSleep(2000);
	CSTK_ASSERT_TRUE_FATAL((s_nTestData == 0 || s_nTestData == 1), "锁频失败\n");

	CSTK_FATAL_POINT	
		
	return TRUE;
}


//@CASEGROUP:CSUDIEW200SGetTuneParam
//@DESCRIPTION: 测试函数CSUDIEW200SGetTuneParam在输入错误参数时的执行结果
//@PRECONDITION:None
//@INPUT:输入参数不符合接口设计的要求
//@INPUT:1、输入参数nOrNetId=-1,nTsId、nServiceId分别从配置文件中读取其值
//@INPUT:2、输入参数nTsId=-1，nOrNetId、nServiceId分别从配置文件中读取其值
//@INPUT:3、输入参数nServiceId=-1，nOrNetId、nTsId分别从配置文件中读取其值
//@EXPECTATION:返回CSUDIEW200S_ERROR_BAD_PARAMETER
//@EXECUTIONFLOW: 1、输入参数nOrNetId=-1时,调用CSUDIEW200SGetTuneParam,期望函数返回值不为CSUDI_SUCCESS
//@EXECUTIONFLOW: 2、输入参数nTsId=-1时,调用CSUDIEW200SGetTuneParam,期望函数返回值不为CSUDI_SUCCESS
//@EXECUTIONFLOW: 3、输入参数nServiceId=-1时,调用CSUDIEW200SGetTuneParam,期望函数返回值不为CSUDI_SUCCESS
BOOL CSTC_EW200SERVICE_MT_0014(void)
{
	CSUDI_Error_Code ret = CSUDI_SUCCESS;
	CSUDIEW200SSourceParam_S sSourceParam;
	int nTsId = 10;
	int nOrNetId = 1;
	int nServiceId = 103;
	
	memset(&sSourceParam , 0 , sizeof( sSourceParam));
	ret = CSUDIEW200SGetTuneParam(-1, nTsId, nServiceId, &sSourceParam);
	CSTK_ASSERT_TRUE_FATAL(ret != CSUDI_SUCCESS, "步骤1失败");
	
	ret = CSUDIEW200SGetTuneParam(nOrNetId, -1, nServiceId, &sSourceParam);
	CSTK_ASSERT_TRUE_FATAL(ret != CSUDI_SUCCESS, "步骤2失败");
	
	ret = CSUDIEW200SGetTuneParam(nOrNetId, nTsId, -1, &sSourceParam);
	CSTK_ASSERT_TRUE_FATAL(ret != CSUDI_SUCCESS, "步骤3失败");

	CSTK_FATAL_POINT
		
	return TRUE;
}

CSUDI_BOOL initChInfo(CSUDITunerSRCDeliver_U *auDeliverList)
{
	char acTemp[32] = {0};
	
	CSTK_ASSERT_TRUE(CS_TK_CONFIG_SUCCESS == CSTKGetConfigInfo("UDI2","CAB_TS1_FREQ",acTemp, sizeof(acTemp)), "从配置文件读取频率失败\n");
	auDeliverList[0].m_sCableDeliver.m_uFrequency = CSTKGetIntFromStr(acTemp, 10);
		
	memset(acTemp, 0 , sizeof(acTemp));
	CSTK_ASSERT_TRUE(CS_TK_CONFIG_SUCCESS == CSTKGetConfigInfo("UDI2","CAB_TS1_SR",acTemp, sizeof(acTemp)), "从配置文件读取符号率失败\n");
	auDeliverList[0].m_sCableDeliver.m_uSymbolRate_24 = CSTKGetIntFromStr(acTemp, 10);
		
	memset(acTemp, 0 , sizeof(acTemp));
	CSTK_ASSERT_TRUE(CS_TK_CONFIG_SUCCESS == CSTKGetConfigInfo("UDI2","CAB_TS1_MODE",acTemp, sizeof(acTemp)), "从配置文件读取调制方式失败\n");
	auDeliverList[0].m_sCableDeliver.m_uModulation_8 = CSTKGetIntFromStr(acTemp, 10);

	CSTK_FATAL_POINT
		
	return CSUDI_TRUE;
}

//@CASEGROUP:CSUDIEW200SGetIPQamDomainInfo
//@DESCRIPTION: 测试函数CSUDIEW200SGetIPQamDomainInfo在输入正确参数时的执行结果
//@PRECONDITION:供测试的码流正常播放
//@INPUT:1、频点信息数组auDeliverList
//@INPUT:2、频点个数
//@INPUT:3、nPid、ucTableId、char caIpQamDataBuffer[16]、nBufferLength = sizeof(caIpQamDataBuffer)、pvReserved=NULL
//@EXPECTATION:函数返回CSUDI_TRUE, 返回值与期望值相等
//@EXECUTIONFLOW: 1、调用CSUDIEW200SGetIPQamDomainInfo获得IPQam
//@EXECUTIONFLOW: 2、比较获取到的值与预期的值，期望相等
BOOL CSTC_EW200SERVICE_MT_0015(void)
{
	CSUDITunerSRCDeliver_U auDeliverList[s_nDeliverCnt];
	memset(&auDeliverList, 0, sizeof(auDeliverList));
	initChInfo(auDeliverList);
	char acIpQamDataBuffer[16] = {0};
	char acIPQam[16]={0};
	memset(acIpQamDataBuffer, 0, sizeof(acIpQamDataBuffer));
	CSUDI_BOOL bRet = CSUDI_FALSE;
	
	bRet = CSUDIEW200SGetIPQamDomainInfo(auDeliverList, s_nDeliverCnt, 0, 0, acIpQamDataBuffer, sizeof(acIpQamDataBuffer), NULL);
	CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == bRet, "获取IPQam信息失败\n");
	CSTK_ASSERT_TRUE(CS_TK_CONFIG_SUCCESS == CSTKGetConfigInfo("EW200Service","CSUDICFG_IPQAM", acIPQam, sizeof(acIPQam)), "从配置文件读取IPQAM的信息失败\n");
	CSTK_ASSERT_TRUE_FATAL(0 == strcmp(acIpQamDataBuffer, acIPQam), "获取的IPQAM错误");

	CSTK_FATAL_POINT
		
	return TRUE;
}


//@CASEGROUP:CSUDIEW200SGetIPQamDomainInfo
//@DESCRIPTION: 测试函数CSUDIEW200SGetIPQamDomainInfo在输入错误参数时的执行效果
//@PRECONDITION:None
//@INPUT:输入不符合接口设计的参数组合
///@INPUT:1、auDeliverList为空,nDeliverCount=0,nPid=0,ucTableId=0,pIpQamDataBuffer,sizeof(pIpQamDataBuffer),NULL
//@INPUT:2、auDeliverList不存在，nDeliverCount=s_nDeliverCnt,nPid=0,ucTableId=0,pIpQamDataBuffer,sizeof(pIpQamDataBuffer),NULL
//@INPUT:3、auDeliverList正确,nDeliverCount=0,nPid=0,ucTableId=0,pIpQamDataBuffer,sizeof(pIpQamDataBuffer),NULL
//@INPUT:4、auDeliverList正确,nDeliverCount=s_nDeliverCnt,nPid=-1,ucTableId=0,pIpQamDataBuffer,sizeof(pIpQamDataBuffer),NULL
//@INPUT:5、auDeliverList正确,nDeliverCount=s_nDeliverCnt,nPid=0,ucTableId=-1,pIpQamDataBuffer,sizeof(pIpQamDataBuffer),NULL
//@INPUT:6、auDeliverList正确,nDeliverCount=s_nDeliverCnt,nPid=0,ucTableId=0,NULL,sizeof(pIpQamDataBuffer),NULL
//@INPUT:7、auDeliverList正确,nDeliverCount=s_nDeliverCnt,nPid=0,ucTableId=0,pIpQamDataBuffer,0,NULL
//@EXPECTATION:上述情况下函数返回CSUDI_FALSE
//@EXECUTIONFLOW: 1、测试在CSUDIEW200SGetIPQamDomainInfo中输入空的频点信息数组的情况,期望返回值为CSUDI_FALSE
//@EXECUTIONFLOW: 2、测试在CSUDIEW200SGetIPQamDomainInfo中输入不存在的频点信息数组的情况,期望返回值为CSUDI_FALSE
//@EXECUTIONFLOW: 3、测试在CSUDIEW200SGetIPQamDomainInfo中输入错误的nDeliverCount的情况,期望返回值为CSUDI_FALSE
//@EXECUTIONFLOW: 4、测试在CSUDIEW200SGetIPQamDomainInfo中输入错误的nPid的情况,期望返回值为CSUDI_FALSE
//@EXECUTIONFLOW: 5、测试在CSUDIEW200SGetIPQamDomainInfo中输入错误的ucTableId的情况,期望返回值为CSUDI_FALSE
//@EXECUTIONFLOW: 6、测试在CSUDIEW200SGetIPQamDomainInfo中输入大小为0的用于保存IPQam信息的数组acIpQamDataBuffer的情况,期望返回值为CSUDI_FALSE
//@EXECUTIONFLOW: 7、测试在CSUDIEW200SGetIPQamDomainInfo中输入nBufferLength=0的情况,期望返回值为CSUDI_FALSE
BOOL CSTC_EW200SERVICE_MT_0016(void)
{
	CSUDITunerSRCDeliver_U auDeliverList[s_nDeliverCnt];
	memset(&auDeliverList, 0, sizeof(auDeliverList));
	char acIpQamDataBuffer[16] = {0};
	memset(acIpQamDataBuffer, 0, sizeof(acIpQamDataBuffer));
	CSUDI_BOOL bRet = CSUDI_TRUE;
	
	bRet = CSUDIEW200SGetIPQamDomainInfo(auDeliverList, s_nDeliverCnt, 0, 0, acIpQamDataBuffer, sizeof(acIpQamDataBuffer), NULL);
	CSTK_ASSERT_TRUE_FATAL(CSUDI_FALSE == bRet, "步骤1失败");

	auDeliverList[0].m_sCableDeliver.m_uFrequency = 307111;
	auDeliverList[0].m_sCableDeliver.m_uSymbolRate_24 = 6875;
	auDeliverList[0].m_sCableDeliver.m_uModulation_8 = 3;
	bRet = CSUDIEW200SGetIPQamDomainInfo(auDeliverList, s_nDeliverCnt, 0, 0, acIpQamDataBuffer, sizeof(acIpQamDataBuffer), NULL);
	CSTK_ASSERT_TRUE_FATAL(CSUDI_FALSE == bRet, "步骤2失败");

	initChInfo(auDeliverList);
	bRet = CSUDIEW200SGetIPQamDomainInfo(auDeliverList, 0, 0, 0, acIpQamDataBuffer, sizeof(acIpQamDataBuffer), NULL);
	CSTK_ASSERT_TRUE_FATAL(CSUDI_FALSE == bRet, "步骤3失败");

	bRet = CSUDIEW200SGetIPQamDomainInfo(auDeliverList, s_nDeliverCnt, -1, 0, acIpQamDataBuffer, sizeof(acIpQamDataBuffer), NULL);
	CSTK_ASSERT_TRUE_FATAL(CSUDI_FALSE == bRet, "步骤4失败");

	bRet = CSUDIEW200SGetIPQamDomainInfo(auDeliverList, s_nDeliverCnt, 0, -1, acIpQamDataBuffer, sizeof(acIpQamDataBuffer), NULL);
	CSTK_ASSERT_TRUE_FATAL(CSUDI_FALSE == bRet, "步骤5失败");

	bRet = CSUDIEW200SGetIPQamDomainInfo(auDeliverList, s_nDeliverCnt, 0, 0, NULL, sizeof(acIpQamDataBuffer), NULL);
	CSTK_ASSERT_TRUE_FATAL(CSUDI_FALSE == bRet, "步骤6失败");

	bRet = CSUDIEW200SGetIPQamDomainInfo(auDeliverList, s_nDeliverCnt, 0, 0, acIpQamDataBuffer, 0, NULL);
	CSTK_ASSERT_TRUE_FATAL(CSUDI_FALSE == bRet, "步骤7失败");

	CSTK_FATAL_POINT
		
	return TRUE;
}


//@CASEGROUP:CSUDIEW200SPlayStream & CSUDIEW200SSetPlayRect & CSUDIEW200SStopPlay
//@DESCRIPTION: 测试函数CSUDIEW200SPlayStream、CSUDIEW200SSetPlayRect和CSUDIEW200SStopPlay在输入正确的参数时的执行效果
//@PRECONDITION:存在指定三要素的测试流，且供测试的码流正常播放
//@INPUT:输入正确的参数
//@INPUT:1、dvb格式的URL，nplayId=0
//@INPUT:2、dvbc格式的URL，nplayId=0
//@INPUT:3、avpid格式的URL，nplayId=0
//@EXPECTATION:按照指定窗口大小正常播放节目和停止节目播放
//@EXECUTIONFLOW: 1、调用CSUDIEW200SSetPlayRect，设置播放窗口为全屏
//@EXECUTIONFLOW: 2、调用CSUDIEW200SPlayStream，对指定的dvbc格式的流进行播放
//@EXECUTIONFLOW: 3、调用CSTKWaitYes，测试人员确认视频是否已成功全屏播放
//@EXECUTIONFLOW: 4、调用CSUDIEW200SSetPlayRect，设置播放窗口的信息为0,0,500,500
//@EXECUTIONFLOW: 5、调用CSTKWaitYes，测试人员视频按照0,0,500,500的窗口格式正常播出
//@EXECUTIONFLOW: 6、调用CSUDIEW200SStopPlay停止视频播放
//@EXECUTIONFLOW: 7、调用CSTKWaitYes，测试人员反馈节目正常停止播放
BOOL CSTC_EW200SERVICE_MT_0017(void)
{ 
	CSUDIWinRect_S sRect;
	memset(&sRect, 0, sizeof(sRect));
	sRect.m_nX = 0;
	sRect.m_nY = 0;
	sRect.m_nWidth = 500;
	sRect.m_nHeight = 500;
	
	CSUDIEW200SSetPlayRect(NULL,0); 
	CSUDIEW200SPlayStream(s_pcTestDvbcUrl,0);
	CSTCPrint("视频全屏播放吗？\n");
	CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes(), "步骤2失败");    

	CSUDIEW200SSetPlayRect(&sRect,0); 
	CSTCPrint("视频窗口以0,0,500,500的位置显示吗？\n");
	CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes(), "步骤4失败");    

	CSTCPrint("视频节目正常停止播放吗吗？\n");
	CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes(), "步骤6失败");
	CSUDIEW200SStopPlay(0);
	
	CSTK_FATAL_POINT

	CSUDIEW200SStopPlay(0);
		
	return TRUE;
}


//@CASEGROUP:CSUDIEW200SPlayStream & CSUDIEW200SSetPlayRect & CSUDIEW200SStopPlay
//@DESCRIPTION: 测试函数CSUDIEW200SPlayStream在输入错误参数时的执行效果
//@PRECONDITION:供测试的码流正常播放
//@INPUT:输入不符合设计的参数组合
//@INPUT:1、dvbc格式的URL头错误，形式为http://OriginalNetworkId.TS_Id.ServiceId
//@INPUT:2、dvbc格式的URL中ServiceId出错,形式为dvbc://490000000:6875:64:-1
//@INPUT:3、URL为空
//@EXPECTATION:不能正常播放节目
//@EXECUTIONFLOW: 1、输入参数为错误的URL地址
//@EXECUTIONFLOW: 2、调用CSUDIEW200SSetPlayRect，设置播放窗口为全屏
//@EXECUTIONFLOW: 3、调用CSUDIEW200SPlayStream，对空URL进行播放，期望节目不能正常播放
//@EXECUTIONFLOW: 4、调用CSTKWaitYes，测试人员确认节目是否播放
//@EXECUTIONFLOW: 5、调用CSUDIEW200SStopPlay，停止视频播放
//@EXECUTIONFLOW: 6、调用CSUDIEW200SPlayStream，对http://403000000:6875:64:103格式的URL进行播放，期望节目不能正常播放
//@EXECUTIONFLOW: 7、调用CSTKWaitYes，测试人员确认节目是否播放
//@EXECUTIONFLOW: 8、调用CSUDIEW200SStopPlay，停止视频播放
//@EXECUTIONFLOW: 9、调用CSUDIEW200SPlayStream，对dvbc://403:6875:64:103格式的URL进行播放，期望节目不能正常播放
//@EXECUTIONFLOW: 10、调用CSTKWaitYes，测试人员确认节目是否播放
//@EXECUTIONFLOW: 11、调用CSUDIEW200SStopPlay，停止视频播放
BOOL CSTC_EW200SERVICE_MT_0018(void)
{ 
	char* pcTesturl_wrongHead = "http://403000000:6875:64:103";
	char* pcTesturl_wrongItem = "dvbc://403:6875:64:103";
	char* pcTestwrongurl = "";
	
	CSUDIEW200SSetPlayRect(NULL,0);
	CSUDIEW200SPlayStream(pcTesturl_wrongHead, 0);
	CSTCPrint("节目不能播放吗？\n");
	CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes(), "步骤3失败");    
	CSUDIEW200SStopPlay(0);
	
	CSUDIEW200SPlayStream(pcTesturl_wrongItem, 0);
	CSTCPrint("节目不能播放吗？\n");
	CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes(), "步骤6失败");    
	CSUDIEW200SStopPlay(0);
	
	CSUDIEW200SPlayStream(pcTestwrongurl,0);
	CSTCPrint("节目不能播放吗？\n");
	CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes(), "步骤9失败");    
	CSUDIEW200SStopPlay(0);
	
	CSTK_FATAL_POINT

	CSUDIEW200SStopPlay(0);
		
	return TRUE;
}


//@CASEGROUP:CSUDIEW200SPlayStream & CSUDIEW200SSetPlayRect & CSUDIEW200SStopPlay
//@DESCRIPTION: 测试函数CSUDIEW200SSetPlayRect在输入错误参数时的执行效果
//@PRECONDITION:供测试的码流正常播放
//@INPUT:输入不符合设计的的参数组合
//@INPUT:1、CSUDIStreamInfo_S psRect, psRect->m_nX=0， psRect->m_nY=0， psRect->m_nWidth=0， psRect->m_nHeight=500
//@INPUT:2、CSUDIStreamInfo_S psRect, psRect->m_nX=0， psRect->m_nY=0， psRect->m_nWidth=500， psRect->m_nHeight=0
//@EXPECTATION:节目不能按照指定的窗口大小播放
//@EXECUTIONFLOW: 1、调用CSUDIEW200SSetPlayRect，设置播放窗口坐标为0,0,0,500
//@EXECUTIONFLOW: 2、调用CSUDIEW200SPlayStream,对dvbc格式的URL进行播放，期望节目播放但视频窗口显示不为0,0,0,500
//@EXECUTIONFLOW: 3、调用CSTKWaitYes，测试人员确认是否节目播放窗口的坐标为0,0,0,500
//@EXECUTIONFLOW: 4、调用CSUDIEW200SSetPlayRect，设置播放窗口的坐标为0,0,500,0
//@EXECUTIONFLOW: 5、调用CSTKWaitYes，测试人员确认是否节目播放窗口的坐标为0,0,500,0
//@EXECUTIONFLOW: 6、调用CSUDIEW200SStopPlay，停止节目播放
BOOL CSTC_EW200SERVICE_MT_0019(void)
{
	CSUDIWinRect_S sRect_wrong;
	memset(&sRect_wrong, 0, sizeof(sRect_wrong));
	sRect_wrong.m_nX = 0;
	sRect_wrong.m_nY = 0;
	sRect_wrong.m_nWidth = 0;
	sRect_wrong.m_nHeight = 500; 
	
	CSUDIEW200SSetPlayRect(&sRect_wrong, 0); 
	CSUDIEW200SPlayStream(s_pcTestDvbcUrl, 0);
	CSTCPrint("视频节目播放窗口的矩形区坐标为0,0,0,500吗？\n");
	CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes(), "步骤1失败");    
	
	sRect_wrong.m_nWidth = 500;
	sRect_wrong.m_nHeight = 0;
	CSUDIEW200SSetPlayRect(&sRect_wrong, 0); 
	CSTCPrint("视频节目播放窗口的坐标为0,0,500,0吗？\n");
	CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes(), "步骤4失败");    
	CSUDIEW200SStopPlay(0);
	
	CSTK_FATAL_POINT

	CSUDIEW200SStopPlay(0);
		
	return TRUE;
}

//@CASEGROUP:CSUDIEW200SPlayStream & CSUDIEW200SSetPlayRect & CSUDIEW200SStopPlay          
//@DESCRIPTION:测试在一个节目正常播放的时候对接口进行多次调用的情况
//@PRECONDITION:信号线正常连接，供测试的码流正常播放
//@INPUT: 待播放流的URL为dvbc://403000000.6875.64.67
//@EXPECTATION: 节目的播放状态不受影响
//@REMARK: 主要是测试一个节目正常播放的时候不能受其他错误调用的影响
//@EXECUTIONFLOW:1、CSUDIEW200SPlayStream播放url为dvbc://403000000.6875.64.67的流
//@EXECUTIONFLOW:2、调用CSTKWaitYes测试人员反馈节目正常播放
//@EXECUTIONFLOW:3、循环调用CSUDIEW200SPlayStream至少10次，期望节目不受影响仍然正常播放
//@EXECUTIONFLOW:4、调用CSTKWaitYes测试人员反馈节目仍然正常播放
//@EXECUTIONFLOW:5、调用CSUDIEW200SSetPlayRect设置视频播放窗口为全屏
//@EXECUTIONFLOW:6、调用CSTKWaitYes测试人员反馈节目正常播放且窗口为全屏
//@EXECUTIONFLOW:7、调用CSUDIEW200SSetPlayRect至少10次，期望节目仍正常播放不受影响
//@EXECUTIONFLOW:8、调用CSTKWaitYes测试人员反馈节目仍正常播放
//@EXECUTIONFLOW:9、调用CSUDIEW200SSetPlayRect设置视频播放窗口坐标为0,0,500,500
//@EXECUTIONFLOW:10、调用CSTKWaitYes测试人员反馈节目正常播放且窗口坐标为0,0,500,500
//@EXECUTIONFLOW:11、调用CSUDIEW200SSetPlayRect至少10次，期望节目仍正常播放不受影响
//@EXECUTIONFLOW:12、调用CSTKWaitYes测试人员反馈节目仍正常播放
//@EXECUTIONFLOW:13、调用CSTKWaitYes测试人员确定节目已停止播�
//@EXECUTIONFLOW:14、调用CSUDIEW200SStopPlay，停止节目播放�
BOOL CSTC_EW200SERVICE_MT_0024(void)
{
	int i = 0;
	CSUDIWinRect_S sRect;
	memset(&sRect, 0, sizeof(sRect));
	sRect.m_nX = 0;
	sRect.m_nY = 0;
	sRect.m_nWidth = 500;
	sRect.m_nHeight = 500;
	
	CSUDIEW200SPlayStream(s_pcTestDvbcUrl, 0);
	CSTCPrint("视频节目正常播放吗？\n");
	CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes(), "步骤2失败\n");    
	while(i < s_nRunCnt)
	{
		CSUDIEW200SPlayStream(s_pcTestDvbcUrl, 0);
		i++;
	}
	CSTCPrint("视频节目仍正常播放吗？\n");
	CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes(), "步骤4失败\n");    
	
	CSUDIEW200SSetPlayRect(NULL, 0);
	CSTCPrint("视频节目全屏播放吗？\n");
	CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes(), "步骤6失败\n");  

	i = 0;
	while(i < s_nRunCnt)
	{
		CSUDIEW200SSetPlayRect(NULL, 0);
		i++;
	}
	CSTCPrint("视频节目全屏正常播放吗？\n");
	CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes(), "步骤8失败\n"); 

	CSUDIEW200SSetPlayRect(&sRect,0); 
	CSTCPrint("视频窗口以0,0,500,500的位置显示吗？\n");
	CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes(), "步骤10失败"); 

	i = 0;
	while(i < s_nRunCnt)
	{
		CSUDIEW200SSetPlayRect(&sRect, 0);
		i++;
	}
	CSTCPrint("视频窗口仍以0,0,500,500的位置显示吗？\n");
	CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes(), "步骤12失败");

	CSTCPrint("视频节目正常停止播放吗吗？\n");
	CSTK_ASSERT_TRUE_FATAL(CSUDI_TRUE == CSTKWaitYes(), "步骤14失败");
	CSUDIEW200SStopPlay(0);
	
	CSTK_FATAL_POINT

	CSUDIEW200SStopPlay(0);
	
	return TRUE;
}

static int fnCallback(unsigned int dwMsg, CSUDIEW200SParam_S *psParam, void* pUserData)
{
	
	if( pUserData!=NULL)
	{
		switch(dwMsg)
	{
		case EM_EW_MSG_VOD_PLAYSTATE_PAUSESUCC:
		{
			s_anTestData[0] = 0;
			break;
		}
		case EM_EW_MSG_VOD_PLAYSTATE_PAUSEFAILED:
		{
			s_anTestData[1] = 1;
			break;
		}
		case EM_EW_MSG_VOD_PLAYSTATE_RESUMESUCC:
		{
			s_anTestData[2] = 2;
			break;
		}
		case EM_EW_MSG_VOD_PLAYSTATE_RESUMEFAILED:
		{
			s_anTestData[3] = 3;
			break;
		}
		case EM_EW_MSG_VOD_PLAYSTATE_SETRATESUCC:
		{
			s_anTestData[4] = 4;
			break;
		}
		case EM_EW_MSG_VOD_PLAYSTATE_SETRATEFAILED:
		{
			s_anTestData[5] = 5;
			break;
		}
		case EM_EW_MSG_VOD_PLAYSTATE_SEEKSUCC:
		{
			s_anTestData[6] = 6;
			break;
		}
		case EM_EW_MSG_VOD_PLAYSTATE_SEEKFAILED:
		{
			s_anTestData[7] = 7;
			break;
		}
		case EM_EW_MSG_VOD_PLAYSTATE_SERVERERROR:
		{
			s_anTestData[10] = 10;
			break;
		}
		case EM_EW_MSG_VOD_PLAYSTATE_TOSTART:
		{
			s_anTestData[11] = 11;
			break;
		}
		case EM_EW_MSG_VOD_PLAYSTATE_TOEND:
		{
			s_anTestData[12] = 12;
			break;
		}
		case EM_EW_MSG_VOD_PLAYSTATE_SERVER_REDIRECT:
		{
			s_anTestData[13] = 13;
			break;
		}
		case EM_EW_MSG_VOD_PLAYSTATE_SERVER_OPTIONS:
		{
			s_anTestData[14] = 14;
			break;
		}
		case EM_EW_MSG_VOD_PLAYSTATE_SERVER_GETPARAMETER:
		{
			s_anTestData[15] = 15;
			break;
		}
		case EM_EW_MSG_VOD_PLAYSTATE_SERVER_SETPARAMETER:
		{
			s_anTestData[16] = 16;
			break;
		}
		case EM_EW_MSG_VOD_PLAYSTATE_DESCRIBE_SUCC:
		{
			s_anTestData[17] = 17;
			break;
		}
		case EM_EW_MSG_VOD_PLAYSTATE_DESCRIBE_FAIL:
		{
			s_anTestData[18] = 18;
			break;
		}
		case EM_EW_MSG_VOD_PLAYSTATE_SETUP_SUCC:
		{
			s_anTestData[19] = 19;
			break;
		}
		case EM_EW_MSG_VOD_PLAYSTATE_SETUP_FAIL:
		{
			s_anTestData[20] = 20;
			break;
		}
		case EM_EW_MSG_VOD_PLAYSTATE_AUTH_SUCC:
		{
			s_anTestData[21] = 21;
			break;
		}
		case EM_EW_MSG_VOD_PLAYSTATE_AUTH_FAIL:
		{
			s_anTestData[22] = 22;
			break;
		}
		case EM_EW_MSG_VOD_PLAYSTATE_SERVER_NODATA:
		{
			s_anTestData[23] = 23;
			break;
		}
		case EM_EW_MSG_VOD_PLAYSTATE_LIVE_TO_TIMESHIFT:
		{
			s_anTestData[24] = 24;
			break;
		}
		case EM_EW_MSG_VOD_PLAYSTATE_TIMESHIFT_TO_LIVE:
		{
			s_anTestData[25] = 25;
			break;
		}
		case EM_EW_MSG_VOD_PLAYSTATE_SAVE_BOOK_MARK:
		{
			s_anTestData[26] = 26;
			break;
		}
		case EM_EW_MSG_VOD_PLAYSTATE_GET_START_TIME:
		{
			s_anTestData[27] = 27;
			break;
		}
		case EM_EW_MSG_VOD_PLAYSTATE_PLAYSUCC:
		{
			s_anTestData[28] = 28;
			break;
		}
		case EM_EW_MSG_VOD_PLAYSTATE_SHAKEHAND_TIMEOUT:
		{
			s_anTestData[30] = 30;
			break;
		}
		case EM_EW_MSG_VOD_PLAYSTATE_INVALID:
		{
			s_anTestData[31] = 31;
			break;
		}
		case EM_EW_MSG_VOD_CONNECT_SUCCESS:
		{
			s_anTestData[90] = 90;
			break;
		}
		case EM_EW_MSG_VOD_CONNECT_FAILED:
		{
			s_anTestData[91] = 91;
			break;
		}
		case EM_EW_MSG_VOD_NO_PROGRAME:
		{
			s_anTestData[92] = 92;
			break;
		}
		case EM_EW_MSG_VOD_LOAD_SUCCESS:
		{
			s_anTestData[93] = 93;
			break;
		}
		case EM_EW_MSG_VOD_NOT_SUPPORT:
		{
			s_anTestData[94] = 94;
			break;
		}case EM_EW_MSG_VOD_FRONTTS_STOP:
		{
			s_anTestData[95] = 95;
			break;
		}
		case EM_EW_MSG_VOD_FRONTTS_RUN:
		{
			s_anTestData[96] = 96;
			break;
		}
		case EM_EW_MSG_VOD_PLAYSTATE_READY:
		{
			s_anTestData[4096] = 4096;
			break;
		}
		case EM_EW_MSG_VOD_PLAYSTATE_PLAYFAILED:
		{
			s_anTestData[4097] = 4097;
			break;
		}
		 case EM_EW_MSG_VOD_CARD_VALID:
		 {
		 	s_anTestData[4352] = 4352;
			break;
		 }
   		case EM_EW_MSG_VOD_CARD_INVALID:
		{
			s_anTestData[4353] = 4353;
			break;
   		}
    		case EM_EW_MSG_VOD_CARD_PREPARING:
		{
			s_anTestData[4354] = 4354;
			break;
    		}
			
		default:
		s_anTestData[4500] = -1;	
		break;
	}
	}
	return 0;
	
}


//@CASEGROUP:CSUDIEW200SAddCallback
//@DESCRIPTION: 测试函数CSUDIEW200SAddCallback在输入正确的参数时的执行效果
//@PRECONDITION:信号线正常连接，供测试的码流正常播放
//@INPUT:所有参数均符合接口设计的要求
//@INPUT:1、nPlayId=0
//@INPUT:2、s_nCallbackUserData = 0x1234;
//@INPUT:3、fncallback
//@INPUT:4、s_anTestData[4501];   
//@EXPECTATION:返回CSUDI_SUCCESS
//@EXECUTIONFLOW: 1、设置ncallbackTestdata为测试回调函数是否响应消息的值
//@EXECUTIONFLOW: 2、调用CSUDIEW200SAddCallback注册回调函数，期望返回值为CSUDI_SUCCESS
//@EXECUTIONFLOW: 3、调用CSUDIEW200SPlayStream触发消息，期望节目正常播放
//@EXECUTIONFLOW: 4、调用CSUDIOSThreadSleep等待2s后判断回调函数中消息响应分别为EM_EW_MSG_VOD_CONNECT_SUCCESS，\
//@EXECUTIONFLOW:EM_EW_MSG_VOD_LOAD_SUCCESS，EM_EW_MSG_VOD_PLAYSTATE_READY
//@EXECUTIONFLOW: 5、调用CSUDIEW200SDelCallback删除注册的回调，期望返回CSUDI_SUCCESS
BOOL CSTC_EW200SERVICE_MT_0020(void)
{
	memset(s_anTestData, 0, sizeof(s_anTestData));
	
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIEW200SAddCallback(0, fnCallback, (void*)s_nCallbackUserData), "步骤2失败\n");
	CSUDIEW200SPlayStream(s_pcTestDvbcUrl,0);
	CSUDIOSThreadSleep(2000);
	CSTK_ASSERT_TRUE_FATAL((s_anTestData[90] == 90) && (s_anTestData[93] == 93) && (s_anTestData[4096] == 4096), "步骤3失败\n");
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIEW200SDelCallback(0, fnCallback, (void*)s_nCallbackUserData), "步骤5失败\n"); 
	CSUDIEW200SStopPlay(0);
	
	CSTK_FATAL_POINT

	CSUDIEW200SStopPlay(0);
		
	return TRUE;
}

//@CASEGROUP:CSUDIEW200SAddCallback
//@DESCRIPTION: 测试函数CSUDIEW200SAddCallback在输入fncallback = CSUDI_NULL时的执行效果
//@PRECONDITION:信号线正常连接，供测试的码流正常播放
//@INPUT:参数不符合接口设计的要求
//@INPUT:1、nPlayId=0
//@INPUT:2、s_nCallbackUserData = 0x1234;
//@INPUT:3、fncallback = CSUDI_NULL
//@INPUT:4、s_anTestData[4501];   
//@EXPECTATION:回调函数注册不成功，返回非CSUDI_SUCCESS，测试的值不变
//@EXECUTIONFLOW: 1、设置ncallbackTestdata为测试回调函数是否被调用的值
//@EXECUTIONFLOW: 2、调用CSUDIEW200SAddCallback添加(pnUserData 为0x1234或任意合法地址值)，期望返回值为CSUDI_SUCCESS
//@EXECUTIONFLOW: 3、调用CSUDIOSThreadSleep等待2s后期望ncallbackTestdata的值不变
//@EXECUTIONFLOW: 4、调用CSUDIEW200SDelCallback删除注册的回调，期望返回值为CSUDIEW200S_ERROR_NO_CALLBACK
BOOL CSTC_EW200SERVICE_MT_0021(void)
{
	memset(s_anTestData, 0, sizeof(s_anTestData));
	
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS != CSUDIEW200SAddCallback(0, CSUDI_NULL, (void*)s_nCallbackUserData), "步骤2失败\n");
	CSUDIEW200SPlayStream(s_pcTestDvbcUrl,0);
	CSUDIOSThreadSleep(2000);
	CSTK_ASSERT_TRUE_FATAL(s_anTestData[4500] == 0, "步骤2失败");
	CSUDIEW200SStopPlay(0);
	CSTK_ASSERT_TRUE_FATAL(CSUDIEW200S_ERROR_NO_CALLBACK == CSUDIEW200SDelCallback(0, fnCallback, (void*)s_nCallbackUserData), "步骤4失败\n");
	
	CSTK_FATAL_POINT
		
	return TRUE;
}

//@CASEGROUP:CSUDIEW200SAddCallback
//@DESCRIPTION: 测试函数CSUDIEW200SAddCallback在输入s_nCallbackUserData=CSUDI_NULL时的执行效果
//@PRECONDITION:信号线正常连接，供测试的码流正常播放
//@INPUT:参数不符合接口设计的要求
//@INPUT:1、nPlayId=0
//@INPUT:2、s_nCallbackUserData = CSUDI_NULL
//@INPUT:3、fncallback
//@INPUT:4、s_anTestData[4501];   
//@EXPECTATION:回调函数注册成功，返回CSUDI_SUCCESS，测试相应消息的值不变
//@EXECUTIONFLOW: 1、设置ncallbackTestdata为测试回调函数是否被调用的值
//@EXECUTIONFLOW: 2、调用CSUDIEW200SAddCallback添加(pnUserData 为为CSUDI_NULL)回调函数，期望返回值为CSUDI_SUCCESS
//@EXECUTIONFLOW: 3、调用CSUDIEW200SPlayStream触发消息，期望节目正常播放
//@EXECUTIONFLOW: 4、调用CSUDIOSThreadSleep等待2s后期望ncallbackTestdata的值不变
//@EXECUTIONFLOW: 5、调用CSUDIEW200SDelCallback删除注册的回调，期望返回值为CSUDIEW200S_ERROR_NO_CALLBACK
BOOL CSTC_EW200SERVICE_MT_0022(void)
{
	memset(s_anTestData, 0, sizeof(s_anTestData));
	
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIEW200SAddCallback(0, fnCallback, CSUDI_NULL), "步骤2失败\n");
	CSUDIEW200SPlayStream(s_pcTestDvbcUrl,0);
	CSUDIOSThreadSleep(2000);
	CSTK_ASSERT_TRUE_FATAL(s_anTestData[4500] == 0, "步骤3失败\n");
	CSUDIEW200SStopPlay(0);
	CSTK_ASSERT_TRUE_FATAL(CSUDIEW200S_ERROR_NO_CALLBACK == CSUDIEW200SDelCallback(0, fnCallback, (void*)s_nCallbackUserData), "步骤5失败\n\n"); 
	
	CSTK_FATAL_POINT
		
	return TRUE;
}

//@CASEGROUP:CSUDIEW200SAddCallback & CSUDIEW200SDelCallback
//@DESCRIPTION: 测试函数CSUDIEW200SAddCallback在连续注册两次删除两次相同的回调时的执行效果
//@PRECONDITION:无
//@INPUT:所有参数均符合接口设计的要求
//@INPUT:1、nPlayId=0
//@INPUT:2、s_nCallbackUserData = 0x1234
//@INPUT:3、fncallback
//@INPUT:4、s_anTestData[4501];   
//@EXPECTATION:第一次注册返回CSUDI_SUCCESS，第二次注册返回CSUDIEW200S_ERROR_ALREADY_ADDED
//@EXECUTIONFLOW: 1、设置ncallbackTestdata为测试回调函数是否被调用的值
//@EXECUTIONFLOW: 2、调用CSUDIEW200SAddCallback注册回调函数，期望返回值为CSUDI_SUCCESS
//@EXECUTIONFLOW: 3、调用CSUDIEW200SAddCallback注册与步骤2相同的回调函数，期望返回值不为CSUDI_SUCCESS
//@EXECUTIONFLOW: 4、调用CSUDIEW200SDelCallback删除注册的回调,期望返回CSUDI_SUCCESS
//@EXECUTIONFLOW: 5、调用CSUDIEW200SDelCallback删除注册的回调,期望返回CSUDIEW200S_ERROR_NO_CALLBACK
BOOL CSTC_EW200SERVICE_MT_0023(void)
{
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIEW200SAddCallback(0, fnCallback, (void*)s_nCallbackUserData), "步骤2失败\n");
	CSTK_ASSERT_TRUE_FATAL(CSUDIEW200S_ERROR_ALREADY_ADDED == CSUDIEW200SAddCallback(0, fnCallback, (void*)s_nCallbackUserData), "步骤3失败\n");
	CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIEW200SDelCallback(0, fnCallback, (void*)s_nCallbackUserData), "步骤4失败\n");
	CSTK_ASSERT_TRUE_FATAL(CSUDIEW200S_ERROR_NO_CALLBACK == CSUDIEW200SDelCallback(0, fnCallback, (void*)s_nCallbackUserData), "步骤5失败\n");

	CSTK_FATAL_POINT
		
	return TRUE;
}

//@CASEGROUP:CSUDIEW200SAddCallback  & CSUDIEW200SDelCallback
//@DESCRIPTION:测试参数全部合法时注册5个回调函数能否注册回调成功
//@PRECONDITION:设备初始化成功
//@INPUT:1、nPlayId = 0
//@INPUT:2、fnCallback = fncallback
//@INPUT:3、UserData = anUserData[i]
//@EXPECTATION:全部注册成功
//@EXECUTIONFLOW:1、调用CSUDIEW200SAddCallback连续注册5个回调函数，注册的函数句柄相同但UserData不同，要求全部返回成功，否则测试用例失败
//@EXECUTIONFLOW:2、调用CSUDIEW200SDelCallback删除注册成功的回调函数，要求返回成功
BOOL CSTC_EW200SERVICE_MT_0025(void)
{
	int anUserData[5] = {0};
	int nAddCallbackCount = 5;
	int i;

	for(i=0 ; i<nAddCallbackCount ; i++)
	{			
		anUserData[i] = i*10+i;
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIEW200SAddCallback(0,fnCallback,&anUserData[i]), "步骤1失败");
	}
			
	for(i=0; i<nAddCallbackCount; i++)
	{
		CSTK_ASSERT_TRUE_FATAL(CSUDI_SUCCESS == CSUDIEW200SDelCallback(0, fnCallback, &anUserData[i]),"步骤2失败");
	}
			
	CSTK_FATAL_POINT
	
	return TRUE;
}





#ifndef CSSHELL_ComvenientCA_PRIVATE_H
#define CSSHELL_ComvenientCA_PRIVATE_H

#include "thirdparty/ca/comvenient/CS_COMVENIENT_Interface.h"

#ifdef __cplusplus
extern "C"
{
#endif
		
/**
@brief 获取Comvenient CA实现接口
@return 成功返回之前设置的Comvenientca实现接口
@see CSShellComvenientCaGetDefaultParam
*/
const CSShellComvenientCaInitParam_S* CSShellComvenientCaGetParam(void);

/**
@brief 获取ComvenientCA的相关配置信息
@return 成功返回配置信息，失败返回NULL
@note 当应用没有调用CSShellComvenientCaSetConfig()设置配置信息时，返回NULL
@see CSShellComvenientCaSetConfig
*/
const CSShellComvenientCaConfig_S* CSShellComvenientCaGetConfig(void);


/******************************************************************************
@breif 根据ca的类型和消息，设置智能卡状态
@param dwType--dwParam3,对应cs_cainterface_prvext.h中 F_CBMsg中的参数
@return 成功返回TRUE,否则返回FALSE
@note 在接收到ca发出的，智能卡错误，请插入智能卡，加密节目，节目未授权等要停止音视频输出的消息时，需要将该原因告诉给mediaplayer。如果mediaplayer不知道
节目停播的原因，就会误发送节目停播的消息
*******************************************************************************/
BOOL CSShellComvenientCaSetSmartCardState(DWORD dwType, DWORD dwParm1, DWORD dwParm2);


#ifdef __cplusplus
}
#endif
/** @} */
#endif



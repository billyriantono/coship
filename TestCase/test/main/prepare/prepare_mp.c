
#include "setup.h"
#include "stdlib.h"
#include "string.h"
#include "stdio.h"
#include "irdecode.h"
#include "CS_OS.h"
#include "udi_init.h"
#include "cs_testkit.h"
 
void PrepareTestMediaplayer(void)
{
	//SetupPortingDriver();
	udi_init();
	CSWMInit();
	SetupFS();
	SetupDTV();
}


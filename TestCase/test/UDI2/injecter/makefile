include $(MMCP_HOME)/build/$(MMCP_PLATFORM)/Makefile.defs
include $(MMCP_HOME)/build/$(MMCP_PLATFORM)/Include.rules

# You can modify MYFLAGS_LOCAL,VPATH,INCLUDES_LOCAL,SUBDIRS,SOURCES,LIBS_LOCAL, don't use +=
# �����������-D����-I�Ĳ���д��INCLUDES_LOCAL��
MYFLAGS_LOCAL  :=

# Դ������·�������ڵ�ǰĿ¼������
VPATH	       := 

# ͷ�ļ�����·��
INCLUDES_LOCAL := 

# ����Ҫ���ӵĿ⣬�ո�ֿ�������չ��".$(LIBSUFFIX)"������"trunk\lib\ƽ̨Ŀ¼"��
LIBS_LOCAL :=

# �¼���������Ŀ¼, �ո�ֿ�
SUBDIRS := pcm_injecter  peses_injecter ts_injecter iframe_injecter

# ���������õ���Դ����,$(wildcard *.c *.cpp)��ʾ�����ļ�
SOURCES := $(wildcard *.c *.cpp)

CFLAGS_LOCAL   := $(MYFLAGS_LOCAL)
CXXFLAGS_LOCAL := $(MYFLAGS_LOCAL)
MMCP_COMPILE_RULES:=TESTKIT
include $(MMCP_HOME)/build/$(MMCP_PLATFORM)/Compile.rules

EXPORT_PORT_TEST_LIB := $(MMCP_HOME)/test/udi2/libs/$(MMCP_PLATFORM)/$(MMCP_COMPILING_MODE)/testlib/$(TARGET)/lib/$(MMCP_PLATFORM)_$(TARGET)_test.$(LIBSUFFIX)

INPUT_PORT_TEST_LIB := $(MMCP_HOME)/test/testkit/libs/$(MMCP_PLATFORM)/$(MMCP_COMPILING_MODE)/libtestkittest.$(LIBSUFFIX) \
	      ./libs/$(MMCP_PLATFORM)/$(MMCP_COMPILING_MODE)/lib$(TARGET)test.$(LIBSUFFIX)  \
	      $(MMCP_HOME)/lib/$(MMCP_PLATFORM)/$(MMCP_COMPILING_MODE)/libUDIPlus.$(LIBSUFFIX) \
	      $(LIBS_LOCAL)
	   


$(EXPORT_PORT_TEST_LIB) : $(INPUT_PORT_TEST_LIB)
	@echo build $(EXPORT_PORT_TEST_LIB) $^...
	$(MMCP_BUILD_LIBRARY)
	@echo build $(EXPORT_PORT_TEST_LIB) over
	
doc:
	@mktestdb .
	@mktestdoc testdb.lua
	@cp $(MMCP_HOME)/test/udi2/$(TARGET)/document  $(MMCP_HOME)/test/udi2/libs/$(MMCP_PLATFORM)/$(MMCP_COMPILING_MODE)/testlib/$(TARGET)/  -R
	@rm $(MMCP_HOME)/test/udi2/libs/$(MMCP_PLATFORM)/$(MMCP_COMPILING_MODE)/testlib/$(TARGET)/include -rf
	@mkdir -p $(MMCP_HOME)/test/udi2/libs/$(MMCP_PLATFORM)/$(MMCP_COMPILING_MODE)/testlib/$(TARGET)/include
	@cp $(MMCP_HOME)/test/udi2/$(TARGET)/cs_$(TARGET)testentry.h  $(MMCP_HOME)/test/udi2/libs/$(MMCP_PLATFORM)/$(MMCP_COMPILING_MODE)/testlib/$(TARGET)/include/
	@cp $(MMCP_HOME)/include/porting/udi2/udi2_inject.h  $(MMCP_HOME)/test/udi2/libs/$(MMCP_PLATFORM)/$(MMCP_COMPILING_MODE)/testlib/$(TARGET)/include/
	
testlib: $(EXPORT_PORT_TEST_LIB)
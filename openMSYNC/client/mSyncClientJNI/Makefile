OPENML_HOME = ../../../openML
SYNC_HOME = ../..
SAMPLE_JAVA_DIR = $(SYNC_HOME)/sample/Java
SAMPLE_ANDROID_SRC_DIR = $(SYNC_HOME)/sample/android/JSyncDemo/src

CC = $(CCBIN)
LD = $(LDBIN)
#make static library
#LIBS = $(SYSLIBS) $(SYNC_HOME)/lib/lib$(MSYNC_LIB_NAME).a
#make shared library
#LIBS = -L$(SYNC_HOME)/lib $(SYSLIBS) -l$(MSYNC_LIB_NAME)
LIBS = -L$(SYNC_HOME)/lib $(SYSLIBS) 

COPT = $(CMODE) -fvisibility=default $(CMODE_PIC)

COMPILE = $(CC) -c $(COPT) $(INCLUDE)

INCLUDE = -I. -I$(SYNC_HOME)/client/include -I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/linux

JAVAC = javac
JAVAH = javah
JAVAC_ENC = -encoding UTF-8

#JNI_LIB = lib$(MSYNC_LIB_NAME)mSyncClientJNI.so
JNI_LIB = libmSyncClientJNI.so
#JNI_SRC = com_openMSYNC_mSyncClientJNI.c ConvertString.c 
JNI_SRC = com_openMSYNC_mSyncClientJNI.c ConvertString.c 
SYNC_SRC = ../src/export.c  ../src/state.c

JNI_OBJ = $(JNI_SRC:.c=.o) $(SYNC_SRC: .c=.o)

all : jni $(JNI_LIB) jdemoapps
release : jni $(JNI_LIB) jdemoapps

jni:
ifeq ($(JAVA_HOME),)
	@echo "Not set JAVA_HOME"
else
	$(JAVAC) $(JAVAC_ENC) com/openMSYNC/mSyncClientJNI.java
	$(JAVAH) -jni com.openMSYNC.mSyncClientJNI
endif

$(JNI_LIB) : $(JNI_OBJ)	
ifneq ($(JAVA_HOME),)
	$(LD) $(COPT) -o $@ $(JNI_OBJ) $(LIBS)
	@cp $(JNI_LIB) $(SYNC_HOME)/lib/.
	@cp com/openMSYNC/mSyncClientJNI.java $(SYNC_HOME)/include/.	
endif

jdemoapps:	
ifeq ($(OSNAME),SunOS)
	-@mkdir $(SAMPLE_JAVA_DIR)/com/openml	
	-@mkdir $(SAMPLE_ANDROID_SRC_DIR)/com/openml
	-@mkdir $(SAMPLE_ANDROID_SRC_DIR)/com/openMSYNC
endif
ifeq ($(OSNAME),Linux)	
	@if [ ! -e $(SAMPLE_JAVA_DIR)/com/openml ]; then (mkdir $(SAMPLE_JAVA_DIR)/com/openml); fi	
	@if [ ! -e $(SAMPLE_ANDROID_SRC_DIR)/com/openml ]; then (mkdir $(SAMPLE_ANDROID_SRC_DIR)/com/openml); fi	
	@if [ ! -e $(SAMPLE_ANDROID_SRC_DIR)/com/openMSYNC ]; then (mkdir $(SAMPLE_ANDROID_SRC_DIR)/com/openMSYNC); fi	
endif
ifneq ($(JAVA_HOME),)
	@cp $(OPENML_HOME)/src/jni/com/openml/JEDB.java $(SAMPLE_JAVA_DIR)/com/openml/.
	@cp ./com/openMSYNC/mSyncClientJNI.java  $(SAMPLE_JAVA_DIR)/com/openMSYNC/.
	@cp $(OPENML_HOME)/src/jni/com/openml/JEDB.java  $(SAMPLE_ANDROID_SRC_DIR)/com/openml/.
	@cp ./com/openMSYNC/mSyncClientJNI.java  $(SAMPLE_ANDROID_SRC_DIR)/com/openMSYNC/.
	@$(JAVAC) $(JAVAC_ENC) $(SAMPLE_JAVA_DIR)/com/openml/JEDB.java $(SAMPLE_JAVA_DIR)/com/openMSYNC/mSyncClientJNI.java $(SAMPLE_JAVA_DIR)/com/openMSYNC/app/JDemoApp.java $(SAMPLE_JAVA_DIR)/com/openMSYNC/app/JDemoLocalDB.java 
	@jar -cvmf $(SAMPLE_JAVA_DIR)/manifest.txt jdemoapp.jar $(SAMPLE_JAVA_DIR)/com/openml/*.class $(SAMPLE_JAVA_DIR)/com/openMSYNC/*.class $(SAMPLE_JAVA_DIR)/com/openMSYNC/app/*.class
	@cp *.jar $(SYNC_HOME)/bin/.	
	@cp jdemoapp $(SYNC_HOME)/bin/.
endif

.c.o:
	$(COMPILE) $<

clean :
	rm -rf *.o *.so *.jar mapfile
	rm -rf ./com/openMSYNC/*.class
	rm -rf $(SAMPLE_JAVA_DIR)/com/openml/*.class
	rm -rf $(SAMPLE_JAVA_DIR)/com/openMSYNC/*.class
	rm -rf $(SAMPLE_JAVA_DIR)/com/openMSYNC/app/*.class

###########################################
# Makefile for SPEED openMSYNC 
###########################################

.KEEP_STATE:

###########################################
# Environment setting
###########################################
-include config.mk

MSYNC_LIB_NAME=mSyncClient
export MSYNC_LIB_NAME

ifeq ($(COMPILE_MODE),mode64)
    B=64
    OPTION += -D_64BIT
else
    B=32
endif

# for gnu make
OSNAME := $(shell /bin/uname)
NODENAME := $(shell /bin/uname -n)

ifeq ($(OSNAME),SunOS)
    WHOAMI := $(shell /usr/ucb/whoami)
else
    WHOAMI := $(shell /usr/bin/whoami)
endif

ifeq ($(OSNAME),Linux)
    ifeq ($(COMPILER),gprof)
        CCBIN = "gcc -m$B -D_GCC -pg"
        LDBIN = "gcc -m$B -shared -pg"
        ARBIN = "ar"
        CCOPT =
    else
        ifeq ($(COMPILER),ppc860)
            CCBIN = "ppc_8xx-g++ -D_GCC"
            LDBIN = "ppc_8xx-g++ -shared"
            ARBIN = "ar"
            CCOPT = -x c
        else
            CCBIN = "gcc -m$B -D_GCC"
            LDBIN = "gcc -m$B -shared -Wl,-Map=mapfile"
            ARBIN = "ar"
            CCOPT = -Wuninitialized -Wunused
        endif
    endif
   
    ifeq ($(COMPILER),gcc)
        OPTION += -Wdeclaration-after-statement
        CCOPT += -fsigned-char
    endif
    ifeq ($(COMPILER),arm-gcc-android)
        CCBIN = "$(ANDROID_TOOLCHAIN)/bin/arm-linux-androideabi-gcc -D_GCC -mabi=aapcs-linux -fsigned-char"
        LDBIN = "$(ANDROID_TOOLCHAIN)/bin/arm-linux-androideabi-gcc -shared -Wl,-Map=mapfile"
        ARBIN = "$(ANDROID_TOOLCHAIN)/bin/arm-linux-androideabi-ar"
        OPTION += -DANDROID_OS
    endif
    ifeq ($(COMPILER),purecov)
        CCBIN = "purecov gcc -m$B -D_GCC"
        LDBIN = "gcc -m$B -shared -Wl,-Map=mapfile"
        CCOPT = 
    endif

    CMODE_PIC =-fPIC
    ifeq ($(PTHREAD),shared)
        ifeq ($(COMPILER),arm-gcc-android)
            SYSLIBS = "-lm -ldl"
            CMODE_DBG = "-g -Wall -Werror-implicit-function-declaration -Wextra -DMDB_DEBUG $(OPTION) $(CCOPT)" 
            CMODE_REL = "-g -O2 -UMDB_DEBUG -Werror-implicit-function-declaration -Wextra $(OPTION) $(CCOPT)" 
        else
            SYSLIBS = "-lpthread -lnsl -lm"
            CMODE_DBG = "-g -Wall -Werror-implicit-function-declaration -Wextra -DMDB_DEBUG $(OPTION) $(CCOPT) -fvisibility=hidden"
            CMODE_REL = "-g -O2 -UMDB_DEBUG -Werror-implicit-function-declaration -Wextra $(OPTION) $(CCOPT) -fvisibility=hidden"
            CMODE_REL_G = "-g -UMDB_DEBUG -Werror-implicit-function-declaration -Wextra $(OPTION) $(CCOPT) -fvisibility=hidden"
        endif
    else
        SYSLIBS = "-L/usr/lib/nptl -lnsl -lm /usr/lib/nptl/libpthread.a /usr/lib/nptl/libc.a"
        CMODE_DBG = "-g -Wall -DMDB_DEBUG -I/usr/include/nptl $(OPTION) $(CCOPT)"
        CMODE_REL = "-g -O2 -UMDB_DEBUG -I/usr/include/nptl $(OPTION) $(CCOPT) -fvisibility=hidden"
        CMODE_REL_G = "-g -UMDB_DEBUG -I/usr/include/nptl $(OPTION) $(CCOPT) -fvisibility=hidden"
    endif    
endif


MAKEOPT_DBG = CC=$(CCBIN) LD=$(LDBIN) AR=$(ARBIN) CMODE=$(CMODE_DBG) \
              CMODE_PIC=$(CMODE_PIC) OSNAME=$(OSNAME) SYSLIBS=$(SYSLIBS) \
              ENABLE_YACC=$(ENABLE_YACC) JNI=$(JNI) \
              COMPILER=$(COMPILER) ARBIN=$(ARBIN) MSYNC_LIB_NAME=$(MSYNC_LIB_NAME) \
              HAVE_READLINE=$(HAVE_READLINE) 

MAKEOPT_REL = CC=$(CCBIN) LD=$(LDBIN) AR=$(ARBIN) CMODE=$(CMODE_REL) \
              CMODE_PIC=$(CMODE_PIC) OSNAME=$(OSNAME) SYSLIBS=$(SYSLIBS) \
              ENABLE_YACC=$(ENABLE_YACC) JNI=$(JNI) \
              COMPILER=$(COMPILER) ARBIN=$(ARBIN) MSYNC_LIB_NAME=$(MSYNC_LIB_NAME) \
              HAVE_READLINE=$(HAVE_READLINE)

MAKEOPT_REL_G = CC=$(CCBIN) LD=$(LDBIN) AR=$(ARBIN) CMODE=$(CMODE_REL_G) \
              CMODE_PIC=$(CMODE_PIC) OSNAME=$(OSNAME) SYSLIBS=$(SYSLIBS) \
              ENABLE_YACC=$(ENABLE_YACC) JNI=$(JNI) \
              COMPILER=$(COMPILER) ARBIN=$(ARBIN) MSYNC_LIB_NAME=$(MSYNC_LIB_NAME) \
              HAVE_READLINE=$(HAVE_READLINE)



###########################################
# General setting
###########################################
		
all: mobile_debug
release: mobile_release
release_g: mobile_release_g

mobile: mobile_debug

check_dir:
	-@if [ ! -d ./bin ]; then mkdir ./bin; fi
	-@if [ ! -d ./lib ]; then mkdir ./lib; fi
	-@if [ -d ./include ]; then rm -rf ./include; fi
	-@if [ ! -d ./include ]; then mkdir ./include; fi
#	-@cp ./src/include/*.h ./include		

mode32:
	@rm -f config.tmp
	@sed "/COMPILE_MODE/d" config.mk > config.tmp
	@mv config.tmp config.mk
	@echo "COMPILE_MODE = mode32" 1>> config.mk

mode64:
	@rm -f config.tmp
	@sed "/COMPILE_MODE/d" config.mk > config.tmp
	@mv config.tmp config.mk
	@echo "COMPILE_MODE = mode64" 1>> config.mk

cc:
	@rm -f config.tmp
	@sed "/COMPILER/d" config.mk > config.tmp
	@mv config.tmp config.mk
	@echo "COMPILER = cc" 1>> config.mk

gcc:
	@rm -f config.tmp
	@sed "/COMPILER/d" config.mk > config.tmp
	@mv config.tmp config.mk
	@echo "COMPILER = gcc" 1>> config.mk

arm-gcc-android:
	@rm -f config.tmp
	@sed "/COMPILER/d" config.mk > config.tmp
	@mv config.tmp config.mk
	@echo "COMPILER = arm-gcc-android" 1>> config.mk
								
spthread:
	@rm -f config.tmp
	@sed "/PTHREAD /d" config.mk > config.tmp
	@mv config.tmp config.mk
	@echo "PTHREAD = shared" 1>> config.mk
apthread:
	@rm -f config.tmp
	@sed "/PTHREAD /d" config.mk > config.tmp
	@mv config.tmp config.mk
	@echo "PTHREAD = static" 1>> config.mk

purecov:
	@rm -f config.tmp
	@sed "/COMPILE/d" config.mk > config.tmp
	@mv config.tmp config.mk
	@echo "COMPILE_MODE = mode32" 1>> config.mk
	@echo "COMPILER = purecov" 1>> config.mk

BRAND=openMSYNC

ifeq ($(JNI),1)
    JNI_FEATURE+=-jni
endif

#PKGNAME=$(BRAND)-$M.$m.$S.$P-$(OSNAME)-$B-$(COMPILER)$(JNI_FEATURE).tar.gz
PKGNAME=$(BRAND)-$(OSNAME)-$B-$(COMPILER)$(JNI_FEATURE).tar.gz

pkg:
	-@rm -rf ./$(BRAND) $(BRAND).tar $(BRAND).tar.gz
	-@mkdir ./$(BRAND)
	-@mkdir ./$(BRAND)/include
	-@(tar cf - ./include ./lib ./bin) | (cd $(BRAND); tar xf -)
	-@tar cf $(BRAND).tar $(BRAND)
	-@gzip -9 -v $(BRAND).tar
	-@mv $(BRAND).tar.gz $(PKGNAME)
	-@echo "$(PKGNAME) done..."

distclean: clean
	@if [ -d ./client ]; then (cd ./client; make distclean) ; fi
	-@echo -n "ccache... "
	-@ccache -C

clean:		
	@if [ -d ./client/mSyncClientJNI ]; then (cd ./client/mSyncClientJNI; make clean) ; fi
	@if [ -d ./client ]; then (cd ./client; make clean) ; fi
	-rm -rf ./bin/* ./lib/* ./include/*


machdep:
	echo $(OSNAME)_$(COMPILE_MODE)_$(COMPILER).mk

mobile_debug: check_dir libmsync_d  mobile_jni_d notice_debug

mobile_release: check_dir  libmsync_r  mobile_jni_r notice_release
	-@rm -f ./bin/logdump

mobile_release_g: check_dir libmsync_rg  mobile_jni_rg notice_release_g
	-@rm -f ./bin/logdump

libmsync_d:
ifeq ($(OSNAME),Linux)
	@if [ -d ./client ]; then (cd ./client; make all $(MAKEOPT_DBG)) ; fi
else
    ifeq ($(OSNAME),SunOS)
	@if [ -d ./client ]; then (cd ./client; make all $(MAKEOPT_DBG)) ; fi
  else
    ifeq ($(OSNAME),Cygwin)
	  @if [ -d ./client ]; then (cd ./client; make all $(MAKEOPT_DBG)) ; fi
    else
	  echo "Not supported mobiledb on $(OSNAME)"
    endif
  endif
endif

libmsync_r:
ifeq ($(OSNAME),Linux)
	@if [ -d ./client ]; then (cd ./client; make all $(MAKEOPT_REL)) ; fi
else
  ifeq ($(OSNAME),SunOS)
	@if [ -d ./client ]; then (cd ./client; make all $(MAKEOPT_REL)) ; fi
  else
    ifeq ($(OSNAME),Cygwin)
	  @if [ -d ./client ]; then (cd ./client; make all $(MAKEOPT_REL)) ; fi
    else
	  	echo "Not supported mobiledb on $(OSNAME)"
    endif
  endif
endif

libmsync_rg:
ifeq ($(OSNAME),Linux)
	@if [ -d ./client ]; then (cd ./client; make all $(MAKEOPT_REL_G)) ; fi
else
ifeq ($(OSNAME),SunOS)
	@if [ -d ./client ]; then (cd ./client; make all $(MAKEOPT_REL_G)) ; fi
else
	echo "Not supported mobiledb on $(OSNAME)"
endif
endif


mobile_jni_d:		
ifeq ($(JNI),1)
	@if [ -d ./client/mSyncClientJNI ]; then (cd ./client/mSyncClientJNI; make all $(MAKEOPT_DBG)) ; fi
endif

mobile_jni_r:
ifeq ($(JNI),1)
	@if [ -d ./client/mSyncClientJNI ]; then (cd ./client/mSyncClientJNI; make all $(MAKEOPT_REL)) ; fi
endif

mobile_jni_rg:
ifeq ($(JNI),1)
	@if [ -d ./client/mSyncClientJNI ]; then (cd ./client/mSyncClientJNI; make all $(MAKEOPT_REL_G)) ; fi
endif

tee:
	make -f Makefile 2>&1 | sed -u -e '/warning: null argument where non-null required/d' | tee make.log

release_tee:
	make -f Makefile release 2>&1 | sed -u -e '/warning: null argument where non-null required/d' | tee make.log

notice_debug: notice
	@echo "  >> End of Debug Mode Compilation"

notice_release: notice
	@echo "  >> End of Release Mode Compilation"

notice_release_g: notice
	@echo "  >> End of Release-G Mode Compilation"

notice:
	@echo "    OSNAME:            $(OSNAME)"
	@echo "    COMPILER:          $(COMPILER)"
	@echo "    COMPILE_MODE:      $(COMPILE_MODE)"
	@echo "    OPTION:            $(OPTION)"
	@echo "    ENABLE_YACC:       $(ENABLE_YACC)"
	@echo "    JNI:               $(JNI)"


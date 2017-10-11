BUILD				= debug

HOME_PATH			= ../../..

OSTYPE				= $(shell /bin/uname)

ifeq ($(CCBIN),)
CC					= gcc
else
CC					= $(CCBIN)
endif
AR					= ar
ARFLAGS				= cru
RANLIB				= ranlib
ifeq ($(LDBIN),)
LD					= ld
else
LD					= $(LDBIN)
endif
LDFLAGS				= 

SUFFIX				= c

DEBUGFLAGS			=
RELEASEFLAGS		=
INCLUDE				= -I$(HOME_PATH)/src/include -I../include -I./ -I$(HOME_PATH)/src/server/include 
LIBRARY				= -L$(HOME_PATH)/lib -laladdin
CFLAGS				= $(CMODE) -D_REENTRANT $(INCLUDE)
SHAREDCFLAGS		= $(CMODE_PIC)

TARGET_LIB_DIR		= $(HOME_PATH)/lib
TARGET_LIB_PREFIX	= libsql
ifeq ($(_LIB_SUFFIX),)
TARGET_LIB_SUFFIX	= a
else
TARGET_LIB_SUFFIX	= $(_LIB_SUFFIX)
endif
TARGET_LIB_OBJS		= sql_main.o sql_parser.o sql_norm.o sql_plan.o sql_execute.o sql_util.o sql_calc_expression.o sql_calc_function.o sql_calc_timefunc.o sql_func_timeutil.o sql_worktable.o sql_drop.o sql_create.o sql_insert.o sql_update.o sql_delete.o sql_select.o sql_mclient.o sql_interface.o sql_result.o sql_decimal.o

TARGET_BIN_DIR		=
TARGET_BIN_PREFIX	=
TARGET_BIN_SUFFIX	=
TARGET_BIN_OBJS		=
TARGET_BIN_LIBS		=

OBJS				= $(TARGET_BIN_OBJS) $(TARGET_LIB_OBJS)

OBJTARGETDIR		= .
DEBUG_PREFIX		=
RELEASE_PREFIX		=
RELEASEOBJDIR		= release-objs
DEBUGOBJDIR			= debug-objs
RELEASETARGETDIR	= $(addprefix $(OBJTARGETDIR)/,$(RELEASEOBJDIR))
DEBUGTARGETDIR		= $(addprefix $(OBJTARGETDIR)/,$(DEBUGOBJDIR))
RELEASEOBJS			= $(addprefix $(RELEASETARGETDIR)/,$(OBJS))
DEBUGOBJS			= $(addprefix $(DEBUGTARGETDIR)/,$(OBJS))

ifneq ($(DEBUG_PREFIX),)
DEBUG_TARGET_INFIX	= _$(DEBUG_PREFIX)
endif
ifneq ($(RELEASE_PREFIX),)
RELEASE_TARGET_INFIX= _$(RELEASE_PREFIX)
endif

#LEX_LIBRARY		= -ll
PARSER_PREFIX		= parser
LEX_OBJS_INFIX		= ll
YACC_OBJS_INFIX		= yy
LEX_FILE_SUFFIX		= l
YACC_FILE_SUFFIX	= y
LEX_OBJS			= $(addprefix $(PARSER_PREFIX).,$(addsuffix .o,$(LEX_OBJS_INFIX)))
YACC_OBJS			= $(addprefix $(PARSER_PREFIX).,$(addsuffix .o,$(YACC_OBJS_INFIX)))


.SUFFIXES: .c .o .h



all: $(BUILD)

release: $(RELEASEOBJS) yacc_release
	-@rm -f $(addprefix $(TARGET_LIB_DIR)/,$(addprefix $(addsuffix $(RELEASE_TARGET_INFIX),$(TARGET_LIB_PREFIX)),.$(TARGET_LIB_SUFFIX)))
ifneq ($(TARGET_LIB_OBJS),)
	-@if [ ! -d $(TARGET_LIB_DIR) ]; then mkdir $(TARGET_LIB_DIR); fi
endif
ifneq ($(TARGET_BIN_OBJS),)
	-@if [ ! -d $(TARGET_BIN_DIR) ]; then mkdir $(TARGET_BIN_DIR); fi
endif

ifneq ($(TARGET_LIB_OBJS),)
ifeq ($(TARGET_LIB_SUFFIX),a)
	$(AR) $(ARFLAGS) $(addprefix $(TARGET_LIB_DIR)/,$(addprefix $(addsuffix $(RELEASE_TARGET_INFIX),$(TARGET_LIB_PREFIX)),.$(TARGET_LIB_SUFFIX))) $(addprefix $(RELEASETARGETDIR)/,$(TARGET_LIB_OBJS)) $(addprefix $(RELEASETARGETDIR)/,$(LEX_OBJS)) $(addprefix $(RELEASETARGETDIR)/,$(YACC_OBJS))
#	$(RANLIB) $(addprefix $(TARGET_LIB_DIR)/,$(addprefix $(addsuffix $(RELEASE_TARGET_INFIX),$(TARGET_LIB_PREFIX)),.$(TARGET_LIB_SUFFIX)))
else
	$(LD) $(LDFLAGS) -o $(addprefix $(TARGET_LIB_DIR)/,$(addprefix $(addsuffix $(RELEASE_TARGET_INFIX),$(TARGET_LIB_PREFIX)),.$(TARGET_LIB_SUFFIX))) $(addprefix $(RELEASETARGETDIR)/,$(TARGET_LIB_OBJS)) $(addprefix $(RELEASETARGETDIR)/,$(LEX_OBJS)) $(addprefix $(RELEASETARGETDIR)/,$(YACC_OBJS))
endif
endif

ifneq ($(TARGET_BIN_OBJS),)
ifeq ($(TARGET_BIN_SUFFIX),)
	$(CC) -D_RELEASE $(CFLAGS) $(RELEASEFLAGS) -o $(addprefix $(TARGET_BIN_DIR)/,$(addsuffix $(RELEASE_TARGET_INFIX),$(TARGET_BIN_PREFIX))) $(addprefix $(RELEASETARGETDIR)/,$(TARGET_BIN_OBJS)) $(addprefix $(TARGET_LIB_DIR)/,$(addprefix $(addsuffix $(RELEASE_TARGET_INFIX),$(TARGET_LIB_PREFIX)),.$(TARGET_LIB_SUFFIX))) $(TARGET_BIN_LIBS)
else
	$(CC) -D_RELEASE $(CFLAGS) $(RELEASEFLAGS) -o $(addprefix $(TARGET_BIN_DIR)/,$(addprefix $(addsuffix $(RELEASE_TARGET_INFIX),$(TARGET_BIN_PREFIX)),.$(TARGET_BIN_SUFFIX))) $(addprefix $(RELEASETARGETDIR)/,$(TARGET_BIN_OBJS)) $(addprefix $(TARGET_LIB_DIR)/,$(addprefix $(addsuffix $(RELEASE_TARGET_INFIX),$(TARGET_LIB_PREFIX)),.$(TARGET_LIB_SUFFIX))) $(TARGET_BIN_LIBS)
endif
endif

yacc_release: 
ifneq ($(TARGET_LIB_OBJS),)
ifeq ($(TARGET_LIB_SUFFIX),a)
	@$(MAKE) -f ./Makefile.yacc BUILD=release OBJS_TYPE=static CMODE="$(CFLAGS)" INCLUDE="$(INCLUDE)" RELEASETARGETDIR="$(RELEASETARGETDIR)" PARSER_PREFIX="$(PARSER_PREFIX)" YACC_OBJS_INFIX="$(YACC_OBJS_INFIX)" LEX_OBJS_INFIX="$(LEX_OBJS_INFIX)" YACC_FILE_SUFFIX="$(YACC_FILE_SUFFIX)" LEX_FILE_SUFFIX="$(LEX_FILE_SUFFIX)" CMODE_PIC="$(CMODE_PIC)" ENABLE_YACC=$(ENABLE_YACC)
#	$(RANLIB) $(addprefix $(TARGET_LIB_DIR)/,$(addprefix $(addsuffix $(RELEASE_TARGET_INFIX),$(TARGET_LIB_PREFIX)),.$(TARGET_LIB_SUFFIX)))
else
	@$(MAKE) -f ./Makefile.yacc BUILD=release OBJS_TYPE=dynamic CMODE="$(CFLAGS)" INCLUDE="$(INCLUDE)" RELEASETARGETDIR="$(RELEASETARGETDIR)" PARSER_PREFIX="$(PARSER_PREFIX)" YACC_OBJS_INFIX="$(YACC_OBJS_INFIX)" LEX_OBJS_INFIX="$(LEX_OBJS_INFIX)" YACC_FILE_SUFFIX="$(YACC_FILE_SUFFIX)" LEX_FILE_SUFFIX="$(LEX_FILE_SUFFIX)" CMODE_PIC="$(CMODE_PIC)" ENABLE_YACC=$(ENABLE_YACC)
endif
endif

debug: $(DEBUGOBJS) yacc_debug
	-@rm -f $(addprefix $(TARGET_LIB_DIR)/,$(addprefix $(addsuffix $(DEBUG_TARGET_INFIX),$(TARGET_LIB_PREFIX)),.$(TARGET_LIB_SUFFIX)))
ifneq ($(TARGET_LIB_OBJS),)
	-@if [ ! -d $(TARGET_LIB_DIR) ]; then mkdir $(TARGET_LIB_DIR); fi
endif
ifneq ($(TARGET_BIN_OBJS),)
	-@if [ ! -d $(TARGET_BIN_DIR) ]; then mkdir $(TARGET_BIN_DIR); fi
endif

ifneq ($(TARGET_LIB_OBJS),)
ifeq ($(TARGET_LIB_SUFFIX),a)
	$(AR) $(ARFLAGS) $(addprefix $(TARGET_LIB_DIR)/,$(addprefix $(addsuffix $(DEBUG_TARGET_INFIX),$(TARGET_LIB_PREFIX)),.$(TARGET_LIB_SUFFIX))) $(addprefix $(DEBUGTARGETDIR)/,$(TARGET_LIB_OBJS)) $(addprefix $(DEBUGTARGETDIR)/,$(LEX_OBJS)) $(addprefix $(DEBUGTARGETDIR)/,$(YACC_OBJS))
#	$(RANLIB) $(addprefix $(TARGET_LIB_DIR)/,$(addprefix $(addsuffix $(DEBUG_TARGET_INFIX),$(TARGET_LIB_PREFIX)),.$(TARGET_LIB_SUFFIX)))
else
	$(LD) $(LDFLAGS) -o $(addprefix $(TARGET_LIB_DIR)/,$(addprefix $(addsuffix $(DEBUG_TARGET_INFIX),$(TARGET_LIB_PREFIX)),.$(TARGET_LIB_SUFFIX))) $(addprefix $(DEBUGTARGETDIR)/,$(TARGET_LIB_OBJS)) $(addprefix $(DEBUGTARGETDIR)/,$(LEX_OBJS)) $(addprefix $(DEBUGTARGETDIR)/,$(YACC_OBJS))
endif
endif

ifneq ($(TARGET_BIN_OBJS),)
ifeq ($(TARGET_BIN_SUFFIX),)
# SWKIM 2007.06.14 compilation message cleanup
#	$(CC) -DMDB_DEBUG $(CFLAGS) $(DEBUGFLAGS) -o $(addprefix $(TARGET_BIN_DIR)/,$(addsuffix $(DEBUG_TARGET_INFIX),$(TARGET_BIN_PREFIX))) $(addprefix $(DEBUGTARGETDIR)/,$(TARGET_BIN_OBJS)) $(addprefix $(TARGET_LIB_DIR)/,$(addprefix $(addsuffix $(DEBUG_TARGET_INFIX),$(TARGET_LIB_PREFIX)),.$(TARGET_LIB_SUFFIX))) $(TARGET_BIN_LIBS)
	$(CC) $(CFLAGS) $(DEBUGFLAGS) -o $(addprefix $(TARGET_BIN_DIR)/,$(addsuffix $(DEBUG_TARGET_INFIX),$(TARGET_BIN_PREFIX))) $(addprefix $(DEBUGTARGETDIR)/,$(TARGET_BIN_OBJS)) $(addprefix $(TARGET_LIB_DIR)/,$(addprefix $(addsuffix $(DEBUG_TARGET_INFIX),$(TARGET_LIB_PREFIX)),.$(TARGET_LIB_SUFFIX))) $(TARGET_BIN_LIBS)
else
# SWKIM 2007.06.14 compilation message cleanup
#	$(CC) -DMDB_DEBUG $(CFLAGS) $(DEBUGFLAGS) -o $(addprefix $(TARGET_BIN_DIR)/,$(addprefix $(addsuffix $(DEBUG_TARGET_INFIX),$(TARGET_BIN_PREFIX)),.$(TARGET_BIN_SUFFIX))) $(addprefix $(DEBUGTARGETDIR)/,$(TARGET_BIN_OBJS)) $(addprefix $(TARGET_LIB_DIR)/,$(addprefix $(addsuffix $(DEBUG_TARGET_INFIX),$(TARGET_LIB_PREFIX)),.$(TARGET_LIB_SUFFIX)))  $(TARGET_BIN_LIBS)
	$(CC) $(CFLAGS) $(DEBUGFLAGS) -o $(addprefix $(TARGET_BIN_DIR)/,$(addprefix $(addsuffix $(DEBUG_TARGET_INFIX),$(TARGET_BIN_PREFIX)),.$(TARGET_BIN_SUFFIX))) $(addprefix $(DEBUGTARGETDIR)/,$(TARGET_BIN_OBJS)) $(addprefix $(TARGET_LIB_DIR)/,$(addprefix $(addsuffix $(DEBUG_TARGET_INFIX),$(TARGET_LIB_PREFIX)),.$(TARGET_LIB_SUFFIX)))  $(TARGET_BIN_LIBS)
endif
endif

yacc_debug: 
ifneq ($(TARGET_LIB_OBJS),)
ifeq ($(TARGET_LIB_SUFFIX),a)
	@$(MAKE) -f ./Makefile.yacc BUILD=debug OBJS_TYPE=static CMODE="$(CFLAGS)" INCLUDE="$(INCLUDE)" DEBUGTARGETDIR="$(DEBUGTARGETDIR)" PARSER_PREFIX="$(PARSER_PREFIX)" YACC_OBJS_INFIX="$(YACC_OBJS_INFIX)" LEX_OBJS_INFIX="$(LEX_OBJS_INFIX)" YACC_FILE_SUFFIX="$(YACC_FILE_SUFFIX)" LEX_FILE_SUFFIX="$(LEX_FILE_SUFFIX)" CMODE_PIC="$(CMODE_PIC)"  ENABLE_YACC=$(ENABLE_YACC)
#	$(RANLIB) $(addprefix $(TARGET_LIB_DIR)/,$(addprefix $(addsuffix $(DEBUG_TARGET_INFIX),$(TARGET_LIB_PREFIX)),.$(TARGET_LIB_SUFFIX)))
else
	@$(MAKE) -f ./Makefile.yacc BUILD=debug OBJS_TYPE=dynamic CMODE="$(CFLAGS)" INCLUDE="$(INCLUDE)" DEBUGTARGETDIR="$(DEBUGTARGETDIR)" PARSER_PREFIX="$(PARSER_PREFIX)" YACC_OBJS_INFIX="$(YACC_OBJS_INFIX)" LEX_OBJS_INFIX="$(LEX_OBJS_INFIX)" YACC_FILE_SUFFIX="$(YACC_FILE_SUFFIX)" LEX_FILE_SUFFIX="$(LEX_FILE_SUFFIX)" CMODE_PIC="$(CMODE_PIC)" ENABLE_YACC=$(ENABLE_YACC)
endif
endif

$(RELEASEOBJS):
	-@if [ ! -d $(RELEASETARGETDIR) ]; then mkdir $(RELEASETARGETDIR); fi
ifeq ($(TARGET_LIB_SUFFIX),a)
	$(CC) -D_RELEASE $(CFLAGS) $(RELEASEFLAGS) $(SHAREDCFLAGS) -c $(notdir $(patsubst %.o,%.c,$@)) -o $@
else
	$(CC) -D_RELEASE $(CFLAGS) $(RELEASEFLAGS) $(SHAREDCFLAGS) -c $(notdir $(patsubst %.o,%.c,$@)) -o $@
endif

$(DEBUGOBJS):
	-@if [ ! -d $(DEBUGTARGETDIR) ]; then mkdir $(DEBUGTARGETDIR); fi
ifeq ($(cc),lint)
# SWKIM 2007.06.14 compilation message cleanup
#	$(CC) -DMDB_DEBUG $(CFLAGS) $(DEBUGFLAGS) $(SHAREDCFLAGS) $(notdir $(patsubst %.o,%.c,$@))
	$(CC) $(CFLAGS) $(DEBUGFLAGS) $(SHAREDCFLAGS) $(notdir $(patsubst %.o,%.c,$@))
else
ifeq ($(TARGET_LIB_SUFFIX),a)
# SWKIM 2007.06.14 compilation message cleanup
#	$(CC) -DMDB_DEBUG $(CFLAGS) $(DEBUGFLAGS) $(SHAREDCFLAGS) -c $(notdir $(patsubst %.o,%.c,$@)) -o $@
	$(CC) $(CFLAGS) $(DEBUGFLAGS) $(SHAREDCFLAGS) -c $(notdir $(patsubst %.o,%.c,$@)) -o $@
else
# SWKIM 2007.06.14 compilation message cleanup
#	$(CC) -DMDB_DEBUG $(CFLAGS) $(DEBUGFLAGS) $(SHAREDCFLAGS) -c $(notdir $(patsubst %.o,%.c,$@)) -o $@
	$(CC) $(CFLAGS) $(DEBUGFLAGS) $(SHAREDCFLAGS) -c $(notdir $(patsubst %.o,%.c,$@)) -o $@
endif
endif

clean: $(addsuffix -clean,$(BUILD))
ifeq ($(BUILD),release)
	@$(MAKE) -f Makefile.yacc RELEASETARGETDIR="$(RELEASETARGETDIR)" LEX_OBJS="$(LEX_OBJS)" YACC_OBJS="$(YACC_OBJS)" CMODE="$(CFLAGS)" release-clean
else
	@$(MAKE) -f Makefile.yacc DEBUGTARGETDIR="$(DEBUGTARGETDIR)" LEX_OBJS="$(LEX_OBJS)" YACC_OBJS="$(YACC_OBJS)" CMODE="$(CFLAGS)" debug-clean
endif

distclean: release-clean debug-clean
	@$(MAKE) -f Makefile.yacc RELEASETARGETDIR="$(RELEASETARGETDIR)"DEBUGTARGETDIR="$(DEBUGTARGETDIR)" LEX_OBJS="$(LEX_OBJS)" YACC_OBJS="$(YACC_OBJS)" CMODE="$(CFLAGS)" distclean
	@rm -rf $(RELEASETARGETDIR) $(DEBUGTARGETDIR)
	@rm -f tags

release-clean:
	@rm -f *.o $(addprefix $(TARGET_LIB_DIR)/,$(addprefix $(addsuffix $(RELEASE_TARGET_INFIX),$(TARGET_LIB_PREFIX)),.$(TARGET_LIB_SUFFIX)))
ifeq ($(TARGET_BIN_SUFFIX),)
	@rm -f $(addprefix $(TARGET_BIN_DIR)/,$(addsuffix $(RELEASE_TARGET_INFIX),$(TARGET_BIN_PREFIX)))
else
	@rm -f $(addprefix $(TARGET_BIN_DIR)/,$(addprefix $(addsuffix $(RELEASE_TARGET_INFIX),$(TARGET_BIN_PREFIX)),.$(TARGET_BIN_SUFFIX)))
endif
	@rm -f $(RELEASEOBJS)

debug-clean:
	@rm -f *.o $(addprefix $(TARGET_LIB_DIR)/,$(addprefix $(addsuffix $(DEBUG_TARGET_INFIX),$(TARGET_LIB_PREFIX)),.$(TARGET_LIB_SUFFIX)))
ifeq ($(TARGET_BIN_SUFFIX),)
	@rm -f $(addprefix $(TARGET_BIN_DIR)/,$(addsuffix $(DEBUG_TARGET_INFIX),$(TARGET_BIN_PREFIX)))
else
	@rm -f $(addprefix $(TARGET_BIN_DIR)/,$(addprefix $(addsuffix $(DEBUG_TARGET_INFIX),$(TARGET_BIN_PREFIX)),.$(TARGET_BIN_SUFFIX)))
endif
	@rm -f $(DEBUGOBJS)

tags: Makefile $(patsubst %.o,%.c,$(OBJS)) $(addprefix $(PARSER_PREFIX).,$(YACC_FILE_SUFFIX)) $(addprefix $(PARSER_PREFIX).,$(LEX_FILE_SUFFIX))
	@/usr/bin/ctags $(patsubst %.o,%.c,$(OBJS)) $(addprefix $(PARSER_PREFIX).,$(YACC_FILE_SUFFIX)) $(addprefix $(PARSER_PREFIX).,$(LEX_FILE_SUFFIX))

depend: __depend
	@$(shell \
		bef=""; \
		for lines in `cat .depend`; do \
			now=`echo $$lines | cut -d: -f1`; \
			if [ "$$bef" != "$$now" ]; then \
				bef=$$now; \
				echo >> ._depend; \
				echo "$$bef:`basename $$bef | cut -d. -f1`.$(SUFFIX)" >> \
					._depend; \
			fi; \
			echo $$lines >> ._depend; \
		done \
	)
	@sed "s// /g" ._depend > .depend
	@cat .depend >> .makefile
	@rm -f .depend ._depend

ifneq ($(TARGET_LIB_OBJS),)
	@touch ._depend
	@cat $(patsubst %.o,%.c,$(TARGET_LIB_OBJS)) | grep \#include > .include_depend.c
	@makedepend -f._depend -Y -- $(INCLUDE) .include_depend.c 2> /dev/null
	@sed -n -e :a -e "1,2!{P;N;D;};N;ba" ._depend > .depend
	@rm -f .include_depend.c ._depend ._depend.bak
	@echo "\n$$(TARGET_LIB_DIR)/$(addsuffix .$(TARGET_LIB_SUFFIX),$(addsuffix $(RELEASE_TARGET_INFIX),$(TARGET_LIB_PREFIX))): $(patsubst %.o,%.c,$(TARGET_LIB_OBJS)) $(LIBRARY) $(addprefix $(PARSER_PREFIX).,$(YACC_FILE_SUFFIX)) $(addprefix $(PARSER_PREFIX).,$(LEX_FILE_SUFFIX))" >> .makefile
	@sed "s/^\.include_depend\.o/$$(TARGET_LIB_DIR)\/$(addsuffix .$(TARGET_LIB_SUFFIX),$(addsuffix $(RELEASE_TARGET_INFIX),$(TARGET_LIB_PREFIX)))/g" .depend >> .makefile
ifneq ($(RELEASE_TARGET_INFIX), $(DEBUG_TARGET_INFIX))
	@echo "\n$$(TARGET_LIB_DIR)/$(addsuffix .$(TARGET_LIB_SUFFIX),$(addsuffix $(DEBUG_TARGET_INFIX),$(TARGET_LIB_PREFIX))): $(patsubst %.o,%.c,$(TARGET_LIB_OBJS)) $(LIBRARY)" >> .makefile
	@sed "s/^\.include_depend\.o/$$(TARGET_LIB_DIR)\/$(addsuffix .$(TARGET_LIB_SUFFIX),$(addsuffix $(DEBUG_TARGET_INFIX),$(TARGET_LIB_PREFIX)))/g" .depend >> .makefile
endif
	@rm -f .depend
endif

ifneq ($(TARGET_BIN_OBJS),)
	@touch ._depend
	@cat $(patsubst %.o,%.c,$(TARGET_BIN_OBJS)) | grep \#include > .include_depend.c
	@makedepend -f._depend -Y -- $(INCLUDE) .include_depend.c 2> /dev/null
#@sed -n -e :a -e "1,2!{P;N;D;};N;ba" ._depend > .depend
	@sed -n "1,2p" ._depend > .depend
	@rm -f .include_depend.c ._depend ._depend.bak


ifneq ($(TARGET_BIN_SUFFIX),)
	@echo "\n$$(TARGET_BIN_DIR)/$(addsuffix .$(TARGET_BIN_SUFFIX),$(addsuffix $(RELEASE_TARGET_INFIX),$(TARGET_BIN_PREFIX))): $(patsubst %.o,%.c,$(TARGET_BIN_OBJS)) $(addprefix $(TARGET_LIB_DIR)/,$(addprefix $(addsuffix $(RELEASE_TARGET_INFIX),$(TARGET_LIB_PREFIX)),.$(TARGET_LIB_SUFFIX)))" >> .makefile
	@sed "s/^\.include_depend\.o/$$(TARGET_BIN_DIR)\/$(addsuffix .$(TARGET_BIN_SUFFIX),$(addsuffix $(RELEASE_TARGET_INFIX),$(TARGET_BIN_PREFIX)))/g" .depend >> .makefile
ifneq ($(RELEASE_TARGET_INFIX), $(DEBUG_TARGET_INFIX))
	@echo "\n$$(TARGET_BIN_DIR)/$(addsuffix .$(TARGET_BIN_SUFFIX),$(addsuffix $(DEBUG_TARGET_INFIX),$(TARGET_BIN_PREFIX))): $(patsubst %.o,%.c,$(TARGET_BIN_OBJS)) $(addprefix $(TARGET_LIB_DIR)/,$(addprefix $(addsuffix $(DEBUG_TARGET_INFIX),$(TARGET_LIB_PREFIX)),.$(TARGET_LIB_SUFFIX)))" >> .makefile
	@sed "s/^\.include_depend\.o/$$(TARGET_BIN_DIR)\/$(addsuffix .$(TARGET_BIN_SUFFIX),$(addsuffix $(DEBUG_TARGET_INFIX),$(TARGET_BIN_PREFIX)))/g" .depend >> .makefile
endif
else
	@echo "\n$$(TARGET_BIN_DIR)/$(addsuffix $(RELEASE_TARGET_INFIX),$(TARGET_BIN_PREFIX)): $(patsubst %.o,%.c,$(TARGET_BIN_OBJS)) $(addprefix $(TARGET_LIB_DIR)/,$(addprefix $(addsuffix $(RELEASE_TARGET_INFIX),$(TARGET_LIB_PREFIX)),.$(TARGET_LIB_SUFFIX)))" >> .makefile
	@sed "s/^\.include_depend\.o/$$(TARGET_BIN_DIR)\/$(addsuffix $(RELEASE_TARGET_INFIX),$(TARGET_BIN_PREFIX))/g" .depend >> .makefile
ifneq ($(RELEASE_TARGET_INFIX), $(DEBUG_TARGET_INFIX))
	@echo "\n$$(TARGET_BIN_DIR)/$(addsuffix $(DEBUG_TARGET_INFIX),$(TARGET_BIN_PREFIX)): $(patsubst %.o,%.c,$(TARGET_BIN_OBJS)) $(addprefix $(TARGET_LIB_DIR)/,$(addprefix $(addsuffix $(DEBUG_TARGET_INFIX),$(TARGET_LIB_PREFIX)),.$(TARGET_LIB_SUFFIX)))" >> .makefile
	@sed "s/^\.include_depend\.o/$$(TARGET_BIN_DIR)\/$(addsuffix $(DEBUG_TARGET_INFIX),$(TARGET_BIN_PREFIX))/g" .depend >> .makefile
endif
endif
	@rm -f .depend
endif
	@mv -f .makefile Makefile
	@$(MAKE) -f ./Makefile.yacc NOECHO=1 INCLUDE="$(INCLUDE)" DEBUGTARGETDIR="$(DEBUGTARGETDIR)" PARSER_PREFIX="$(PARSER_PREFIX)" YACC_OBJS_INFIX="$(YACC_OBJS_INFIX)" LEX_OBJS_INFIX="$(LEX_OBJS_INFIX)" YACC_FILE_SUFFIX="$(YACC_FILE_SUFFIX)" LEX_FILE_SUFFIX="$(LEX_FILE_SUFFIX)" ENABLE_YACC=$(ENABLE_YACC) CMODE="$(CFLAGS)" depend > /dev/null


__depend:
	@echo "Create or update source and target file dependencies..."
	@makedepend -s"# FILE DEPENDENCIES BELOW; DO NOT EDIT OR DELETE IT" -p "$$(RELEASETARGETDIR)/" -Y -- $(INCLUDE) $(patsubst %.o,%.c,$(OBJS)) 2> /dev/null
	@makedepend -s"# FILE DEPENDENCIES BELOW; DO NOT EDIT OR DELETE IT" -a -p "$$(DEBUGTARGETDIR)/" -Y -- $(INCLUDE) $(patsubst %.o,%.c,$(OBJS)) 2> /dev/null
	@rm -f Makefile.bak
	@head -`grep -n "^# FILE DEPENDENCIES BELOW; DO NOT EDIT OR DELETE IT" Makefile | cut -d: -f 1` Makefile > .makefile
	@sed -n -e :a -e "1,`grep -n \"^# FILE DEPENDENCIES BELOW; DO NOT EDIT OR DELETE IT\" Makefile | cut -d: -f1`!{P;N;D;};N;ba" Makefile > .depend
	@sed "s/ //g" .depend > ._depend
	@mv -f ._depend .depend

# FILE DEPENDENCIES BELOW; DO NOT EDIT OR DELETE IT

$(RELEASETARGETDIR)/sql_main.o: sql_main.c
$(RELEASETARGETDIR)/sql_main.o: ../../../src/include/mdb_config.h
$(RELEASETARGETDIR)/sql_main.o: ../../../src/include/sys_inc.h
$(RELEASETARGETDIR)/sql_main.o: ../../../src/server/include/mdb_compat.h
$(RELEASETARGETDIR)/sql_main.o: ../../../src/include/mdb_typedef.h
$(RELEASETARGETDIR)/sql_main.o: ../../../src/include/dbport.h
$(RELEASETARGETDIR)/sql_main.o: ../include/sql_main.h
$(RELEASETARGETDIR)/sql_main.o: ../../../src/server/include/mdb_dbi.h
$(RELEASETARGETDIR)/sql_main.o: ../../../src/include/systable.h
$(RELEASETARGETDIR)/sql_main.o: ../../../src/include/mdb_define.h
$(RELEASETARGETDIR)/sql_main.o: ../../../src/include/ErrorCode.h
$(RELEASETARGETDIR)/sql_main.o: ../../../src/server/include/mdb_FieldDesc.h
$(RELEASETARGETDIR)/sql_main.o: ../../../src/server/include/mdb_FieldVal.h
$(RELEASETARGETDIR)/sql_main.o: ../../../src/server/include/mdb_KeyDesc.h
$(RELEASETARGETDIR)/sql_main.o: ../../../src/server/include/mdb_inner_define.h
$(RELEASETARGETDIR)/sql_main.o: ../../../src/include/db_api.h
$(RELEASETARGETDIR)/sql_main.o: ../../../src/server/include/mdb_KeyValue.h
$(RELEASETARGETDIR)/sql_main.o: ../../../src/server/include/mdb_Filter.h
$(RELEASETARGETDIR)/sql_main.o: ../../../src/server/include/mdb_schema_cache.h
$(RELEASETARGETDIR)/sql_main.o: ../../../src/server/include/mdb_Trans.h
$(RELEASETARGETDIR)/sql_main.o: ../../../src/server/include/mdb_LSN.h
$(RELEASETARGETDIR)/sql_main.o: ../../../src/server/include/mdb_LockMgrPI.h
$(RELEASETARGETDIR)/sql_main.o: ../../../src/server/include/mdb_ppthread.h
$(RELEASETARGETDIR)/sql_main.o: ../../../src/server/include/mdb_Csr.h
$(RELEASETARGETDIR)/sql_main.o: ../../../src/server/include/mdb_ContIter.h
$(RELEASETARGETDIR)/sql_main.o: ../../../src/server/include/mdb_Iterator.h
$(RELEASETARGETDIR)/sql_main.o: ../../../src/server/include/mdb_Cont.h
$(RELEASETARGETDIR)/sql_main.o: ../../../src/server/include/mdb_BaseCont.h
$(RELEASETARGETDIR)/sql_main.o: ../../../src/server/include/mdb_Collect.h
$(RELEASETARGETDIR)/sql_main.o: ../../../src/server/include/mdb_PageList.h
$(RELEASETARGETDIR)/sql_main.o: ../../../src/server/include/mdb_Page.h
$(RELEASETARGETDIR)/sql_main.o: ../../../src/server/include/mdb_TTree.h
$(RELEASETARGETDIR)/sql_main.o: ../../../src/server/include/mdb_TTreeNode.h
$(RELEASETARGETDIR)/sql_main.o: ../../../src/server/include/mdb_TTreeIter.h
$(RELEASETARGETDIR)/sql_main.o: ../../../src/include/isql.h
$(RELEASETARGETDIR)/sql_main.o: ../include/sql_const.h
$(RELEASETARGETDIR)/sql_main.o: ../include/sql_datast.h ../include/sql_util.h
$(RELEASETARGETDIR)/sql_main.o: ../include/sql_packet_format.h
$(RELEASETARGETDIR)/sql_main.o: ../../../src/server/include/mdb_PMEM.h
$(RELEASETARGETDIR)/sql_main.o: ../../../src/server/include/mdb_er.h
$(RELEASETARGETDIR)/sql_main.o: ../../../src/server/include/mdb_charset.h
$(RELEASETARGETDIR)/sql_main.o: ../include/sql_parser.h ../include/sql_plan.h
$(RELEASETARGETDIR)/sql_main.o: ../include/sql_execute.h
$(RELEASETARGETDIR)/sql_main.o: ../include/sql_norm.h sql_func_timeutil.h
$(RELEASETARGETDIR)/sql_main.o: sql_mclient.h

$(RELEASETARGETDIR)/sql_parser.o: sql_parser.c
$(RELEASETARGETDIR)/sql_parser.o: ../../../src/include/mdb_config.h
$(RELEASETARGETDIR)/sql_parser.o: ../../../src/include/sys_inc.h
$(RELEASETARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_compat.h
$(RELEASETARGETDIR)/sql_parser.o: ../../../src/include/mdb_typedef.h
$(RELEASETARGETDIR)/sql_parser.o: ../../../src/include/dbport.h
$(RELEASETARGETDIR)/sql_parser.o: ../include/sql_parser.h
$(RELEASETARGETDIR)/sql_parser.o: ../include/sql_datast.h
$(RELEASETARGETDIR)/sql_parser.o: ../../../src/include/systable.h
$(RELEASETARGETDIR)/sql_parser.o: ../../../src/include/mdb_define.h
$(RELEASETARGETDIR)/sql_parser.o: ../../../src/include/ErrorCode.h
$(RELEASETARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_FieldDesc.h
$(RELEASETARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_FieldVal.h
$(RELEASETARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_KeyDesc.h
$(RELEASETARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_Csr.h
$(RELEASETARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_ContIter.h
$(RELEASETARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_Iterator.h
$(RELEASETARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_Cont.h
$(RELEASETARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_BaseCont.h
$(RELEASETARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_inner_define.h
$(RELEASETARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_Collect.h
$(RELEASETARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_PageList.h
$(RELEASETARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_Page.h
$(RELEASETARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_TTree.h
$(RELEASETARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_TTreeNode.h
$(RELEASETARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_KeyValue.h
$(RELEASETARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_LockMgrPI.h
$(RELEASETARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_ppthread.h
$(RELEASETARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_Trans.h
$(RELEASETARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_LSN.h
$(RELEASETARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_Filter.h
$(RELEASETARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_TTreeIter.h
$(RELEASETARGETDIR)/sql_parser.o: ../include/sql_const.h
$(RELEASETARGETDIR)/sql_parser.o: ../../../src/include/isql.h
$(RELEASETARGETDIR)/sql_parser.o: ../../../src/include/db_api.h
$(RELEASETARGETDIR)/sql_parser.o: ../include/sql_util.h
$(RELEASETARGETDIR)/sql_parser.o: ../include/sql_packet_format.h
$(RELEASETARGETDIR)/sql_parser.o: ../include/sql_main.h
$(RELEASETARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_dbi.h
$(RELEASETARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_schema_cache.h
$(RELEASETARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_er.h
$(RELEASETARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_charset.h
$(RELEASETARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_PMEM.h
$(RELEASETARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_Server.h
$(RELEASETARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_ContCat.h
$(RELEASETARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_IndexCat.h
$(RELEASETARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_LogMgr.h
$(RELEASETARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_LogAnchor.h
$(RELEASETARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_Recovery.h
$(RELEASETARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_LogBuffer.h
$(RELEASETARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_LogFile.h
$(RELEASETARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_LogRec.h
$(RELEASETARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_Mem_Mgr.h
$(RELEASETARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_MemBase.h
$(RELEASETARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_Segment.h
$(RELEASETARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_DBFile.h
$(RELEASETARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_syslog.h
$(RELEASETARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_Thread.h
$(RELEASETARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_comm_stub.h
$(RELEASETARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_memmgr.h
$(RELEASETARGETDIR)/sql_parser.o: sql_mclient.h
$(RELEASETARGETDIR)/sql_parser.o: sql_func_timeutil.h

$(RELEASETARGETDIR)/sql_norm.o: sql_norm.c
$(RELEASETARGETDIR)/sql_norm.o: ../../../src/include/mdb_config.h
$(RELEASETARGETDIR)/sql_norm.o: ../../../src/include/sys_inc.h
$(RELEASETARGETDIR)/sql_norm.o: ../../../src/server/include/mdb_compat.h
$(RELEASETARGETDIR)/sql_norm.o: ../../../src/include/mdb_typedef.h
$(RELEASETARGETDIR)/sql_norm.o: ../../../src/include/dbport.h
$(RELEASETARGETDIR)/sql_norm.o: ../include/sql_norm.h ../include/sql_datast.h
$(RELEASETARGETDIR)/sql_norm.o: ../../../src/include/systable.h
$(RELEASETARGETDIR)/sql_norm.o: ../../../src/include/mdb_define.h
$(RELEASETARGETDIR)/sql_norm.o: ../../../src/include/ErrorCode.h
$(RELEASETARGETDIR)/sql_norm.o: ../../../src/server/include/mdb_FieldDesc.h
$(RELEASETARGETDIR)/sql_norm.o: ../../../src/server/include/mdb_FieldVal.h
$(RELEASETARGETDIR)/sql_norm.o: ../../../src/server/include/mdb_KeyDesc.h
$(RELEASETARGETDIR)/sql_norm.o: ../../../src/server/include/mdb_Csr.h
$(RELEASETARGETDIR)/sql_norm.o: ../../../src/server/include/mdb_ContIter.h
$(RELEASETARGETDIR)/sql_norm.o: ../../../src/server/include/mdb_Iterator.h
$(RELEASETARGETDIR)/sql_norm.o: ../../../src/server/include/mdb_Cont.h
$(RELEASETARGETDIR)/sql_norm.o: ../../../src/server/include/mdb_BaseCont.h
$(RELEASETARGETDIR)/sql_norm.o: ../../../src/server/include/mdb_inner_define.h
$(RELEASETARGETDIR)/sql_norm.o: ../../../src/server/include/mdb_Collect.h
$(RELEASETARGETDIR)/sql_norm.o: ../../../src/server/include/mdb_PageList.h
$(RELEASETARGETDIR)/sql_norm.o: ../../../src/server/include/mdb_Page.h
$(RELEASETARGETDIR)/sql_norm.o: ../../../src/server/include/mdb_TTree.h
$(RELEASETARGETDIR)/sql_norm.o: ../../../src/server/include/mdb_TTreeNode.h
$(RELEASETARGETDIR)/sql_norm.o: ../../../src/server/include/mdb_KeyValue.h
$(RELEASETARGETDIR)/sql_norm.o: ../../../src/server/include/mdb_LockMgrPI.h
$(RELEASETARGETDIR)/sql_norm.o: ../../../src/server/include/mdb_ppthread.h
$(RELEASETARGETDIR)/sql_norm.o: ../../../src/server/include/mdb_Trans.h
$(RELEASETARGETDIR)/sql_norm.o: ../../../src/server/include/mdb_LSN.h
$(RELEASETARGETDIR)/sql_norm.o: ../../../src/server/include/mdb_Filter.h
$(RELEASETARGETDIR)/sql_norm.o: ../../../src/server/include/mdb_TTreeIter.h
$(RELEASETARGETDIR)/sql_norm.o: ../include/sql_const.h
$(RELEASETARGETDIR)/sql_norm.o: ../../../src/include/isql.h
$(RELEASETARGETDIR)/sql_norm.o: ../../../src/include/db_api.h
$(RELEASETARGETDIR)/sql_norm.o: ../include/sql_util.h
$(RELEASETARGETDIR)/sql_norm.o: ../include/sql_packet_format.h
$(RELEASETARGETDIR)/sql_norm.o: ../include/sql_main.h
$(RELEASETARGETDIR)/sql_norm.o: ../../../src/server/include/mdb_dbi.h
$(RELEASETARGETDIR)/sql_norm.o: ../../../src/server/include/mdb_schema_cache.h
$(RELEASETARGETDIR)/sql_norm.o: ../../../src/server/include/mdb_er.h
$(RELEASETARGETDIR)/sql_norm.o: ../../../src/server/include/mdb_charset.h
$(RELEASETARGETDIR)/sql_norm.o: ../../../src/server/include/mdb_PMEM.h
$(RELEASETARGETDIR)/sql_norm.o: ../include/sql_parser.h sql_mclient.h

$(RELEASETARGETDIR)/sql_plan.o: sql_plan.c
$(RELEASETARGETDIR)/sql_plan.o: ../../../src/include/mdb_config.h
$(RELEASETARGETDIR)/sql_plan.o: ../../../src/include/sys_inc.h
$(RELEASETARGETDIR)/sql_plan.o: ../../../src/server/include/mdb_compat.h
$(RELEASETARGETDIR)/sql_plan.o: ../../../src/include/mdb_typedef.h
$(RELEASETARGETDIR)/sql_plan.o: ../../../src/include/dbport.h
$(RELEASETARGETDIR)/sql_plan.o: ../include/sql_plan.h ../include/sql_datast.h
$(RELEASETARGETDIR)/sql_plan.o: ../../../src/include/systable.h
$(RELEASETARGETDIR)/sql_plan.o: ../../../src/include/mdb_define.h
$(RELEASETARGETDIR)/sql_plan.o: ../../../src/include/ErrorCode.h
$(RELEASETARGETDIR)/sql_plan.o: ../../../src/server/include/mdb_FieldDesc.h
$(RELEASETARGETDIR)/sql_plan.o: ../../../src/server/include/mdb_FieldVal.h
$(RELEASETARGETDIR)/sql_plan.o: ../../../src/server/include/mdb_KeyDesc.h
$(RELEASETARGETDIR)/sql_plan.o: ../../../src/server/include/mdb_Csr.h
$(RELEASETARGETDIR)/sql_plan.o: ../../../src/server/include/mdb_ContIter.h
$(RELEASETARGETDIR)/sql_plan.o: ../../../src/server/include/mdb_Iterator.h
$(RELEASETARGETDIR)/sql_plan.o: ../../../src/server/include/mdb_Cont.h
$(RELEASETARGETDIR)/sql_plan.o: ../../../src/server/include/mdb_BaseCont.h
$(RELEASETARGETDIR)/sql_plan.o: ../../../src/server/include/mdb_inner_define.h
$(RELEASETARGETDIR)/sql_plan.o: ../../../src/server/include/mdb_Collect.h
$(RELEASETARGETDIR)/sql_plan.o: ../../../src/server/include/mdb_PageList.h
$(RELEASETARGETDIR)/sql_plan.o: ../../../src/server/include/mdb_Page.h
$(RELEASETARGETDIR)/sql_plan.o: ../../../src/server/include/mdb_TTree.h
$(RELEASETARGETDIR)/sql_plan.o: ../../../src/server/include/mdb_TTreeNode.h
$(RELEASETARGETDIR)/sql_plan.o: ../../../src/server/include/mdb_KeyValue.h
$(RELEASETARGETDIR)/sql_plan.o: ../../../src/server/include/mdb_LockMgrPI.h
$(RELEASETARGETDIR)/sql_plan.o: ../../../src/server/include/mdb_ppthread.h
$(RELEASETARGETDIR)/sql_plan.o: ../../../src/server/include/mdb_Trans.h
$(RELEASETARGETDIR)/sql_plan.o: ../../../src/server/include/mdb_LSN.h
$(RELEASETARGETDIR)/sql_plan.o: ../../../src/server/include/mdb_Filter.h
$(RELEASETARGETDIR)/sql_plan.o: ../../../src/server/include/mdb_TTreeIter.h
$(RELEASETARGETDIR)/sql_plan.o: ../include/sql_const.h
$(RELEASETARGETDIR)/sql_plan.o: ../../../src/include/isql.h
$(RELEASETARGETDIR)/sql_plan.o: ../../../src/include/db_api.h
$(RELEASETARGETDIR)/sql_plan.o: ../include/sql_execute.h
$(RELEASETARGETDIR)/sql_plan.o: ../include/sql_main.h
$(RELEASETARGETDIR)/sql_plan.o: ../../../src/server/include/mdb_dbi.h
$(RELEASETARGETDIR)/sql_plan.o: ../../../src/server/include/mdb_schema_cache.h
$(RELEASETARGETDIR)/sql_plan.o: ../include/sql_util.h
$(RELEASETARGETDIR)/sql_plan.o: ../include/sql_packet_format.h
$(RELEASETARGETDIR)/sql_plan.o: ../../../src/server/include/mdb_PMEM.h
$(RELEASETARGETDIR)/sql_plan.o: ../../../src/server/include/mdb_er.h
$(RELEASETARGETDIR)/sql_plan.o: ../../../src/server/include/mdb_charset.h
$(RELEASETARGETDIR)/sql_plan.o: ../include/sql_parser.h sql_mclient.h

$(RELEASETARGETDIR)/sql_execute.o: sql_execute.c
$(RELEASETARGETDIR)/sql_execute.o: ../../../src/include/mdb_config.h
$(RELEASETARGETDIR)/sql_execute.o: ../../../src/include/sys_inc.h
$(RELEASETARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_compat.h
$(RELEASETARGETDIR)/sql_execute.o: ../../../src/include/mdb_typedef.h
$(RELEASETARGETDIR)/sql_execute.o: ../../../src/include/dbport.h
$(RELEASETARGETDIR)/sql_execute.o: ../include/sql_main.h
$(RELEASETARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_dbi.h
$(RELEASETARGETDIR)/sql_execute.o: ../../../src/include/systable.h
$(RELEASETARGETDIR)/sql_execute.o: ../../../src/include/mdb_define.h
$(RELEASETARGETDIR)/sql_execute.o: ../../../src/include/ErrorCode.h
$(RELEASETARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_FieldDesc.h
$(RELEASETARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_FieldVal.h
$(RELEASETARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_KeyDesc.h
$(RELEASETARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_inner_define.h
$(RELEASETARGETDIR)/sql_execute.o: ../../../src/include/db_api.h
$(RELEASETARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_KeyValue.h
$(RELEASETARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_Filter.h
$(RELEASETARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_schema_cache.h
$(RELEASETARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_Trans.h
$(RELEASETARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_LSN.h
$(RELEASETARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_LockMgrPI.h
$(RELEASETARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_ppthread.h
$(RELEASETARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_Csr.h
$(RELEASETARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_ContIter.h
$(RELEASETARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_Iterator.h
$(RELEASETARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_Cont.h
$(RELEASETARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_BaseCont.h
$(RELEASETARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_Collect.h
$(RELEASETARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_PageList.h
$(RELEASETARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_Page.h
$(RELEASETARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_TTree.h
$(RELEASETARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_TTreeNode.h
$(RELEASETARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_TTreeIter.h
$(RELEASETARGETDIR)/sql_execute.o: ../../../src/include/isql.h
$(RELEASETARGETDIR)/sql_execute.o: ../include/sql_const.h
$(RELEASETARGETDIR)/sql_execute.o: ../include/sql_datast.h
$(RELEASETARGETDIR)/sql_execute.o: ../include/sql_util.h
$(RELEASETARGETDIR)/sql_execute.o: ../include/sql_packet_format.h
$(RELEASETARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_PMEM.h
$(RELEASETARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_er.h
$(RELEASETARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_charset.h
$(RELEASETARGETDIR)/sql_execute.o: ../include/sql_execute.h sql_decimal.h
$(RELEASETARGETDIR)/sql_execute.o: sql_func_timeutil.h
$(RELEASETARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_comm_stub.h
$(RELEASETARGETDIR)/sql_execute.o: ../include/sql_parser.h sql_mclient.h
$(RELEASETARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_Server.h
$(RELEASETARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_ContCat.h
$(RELEASETARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_IndexCat.h
$(RELEASETARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_LogMgr.h
$(RELEASETARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_LogAnchor.h
$(RELEASETARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_Recovery.h
$(RELEASETARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_LogBuffer.h
$(RELEASETARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_LogFile.h
$(RELEASETARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_LogRec.h
$(RELEASETARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_Mem_Mgr.h
$(RELEASETARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_MemBase.h
$(RELEASETARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_Segment.h
$(RELEASETARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_DBFile.h
$(RELEASETARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_syslog.h
$(RELEASETARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_Thread.h
$(RELEASETARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_memmgr.h

$(RELEASETARGETDIR)/sql_util.o: sql_util.c
$(RELEASETARGETDIR)/sql_util.o: ../../../src/include/mdb_config.h
$(RELEASETARGETDIR)/sql_util.o: ../../../src/include/sys_inc.h
$(RELEASETARGETDIR)/sql_util.o: ../../../src/server/include/mdb_compat.h
$(RELEASETARGETDIR)/sql_util.o: ../../../src/include/mdb_typedef.h
$(RELEASETARGETDIR)/sql_util.o: ../../../src/include/dbport.h
$(RELEASETARGETDIR)/sql_util.o: ../include/sql_main.h
$(RELEASETARGETDIR)/sql_util.o: ../../../src/server/include/mdb_dbi.h
$(RELEASETARGETDIR)/sql_util.o: ../../../src/include/systable.h
$(RELEASETARGETDIR)/sql_util.o: ../../../src/include/mdb_define.h
$(RELEASETARGETDIR)/sql_util.o: ../../../src/include/ErrorCode.h
$(RELEASETARGETDIR)/sql_util.o: ../../../src/server/include/mdb_FieldDesc.h
$(RELEASETARGETDIR)/sql_util.o: ../../../src/server/include/mdb_FieldVal.h
$(RELEASETARGETDIR)/sql_util.o: ../../../src/server/include/mdb_KeyDesc.h
$(RELEASETARGETDIR)/sql_util.o: ../../../src/server/include/mdb_inner_define.h
$(RELEASETARGETDIR)/sql_util.o: ../../../src/include/db_api.h
$(RELEASETARGETDIR)/sql_util.o: ../../../src/server/include/mdb_KeyValue.h
$(RELEASETARGETDIR)/sql_util.o: ../../../src/server/include/mdb_Filter.h
$(RELEASETARGETDIR)/sql_util.o: ../../../src/server/include/mdb_schema_cache.h
$(RELEASETARGETDIR)/sql_util.o: ../../../src/server/include/mdb_Trans.h
$(RELEASETARGETDIR)/sql_util.o: ../../../src/server/include/mdb_LSN.h
$(RELEASETARGETDIR)/sql_util.o: ../../../src/server/include/mdb_LockMgrPI.h
$(RELEASETARGETDIR)/sql_util.o: ../../../src/server/include/mdb_ppthread.h
$(RELEASETARGETDIR)/sql_util.o: ../../../src/server/include/mdb_Csr.h
$(RELEASETARGETDIR)/sql_util.o: ../../../src/server/include/mdb_ContIter.h
$(RELEASETARGETDIR)/sql_util.o: ../../../src/server/include/mdb_Iterator.h
$(RELEASETARGETDIR)/sql_util.o: ../../../src/server/include/mdb_Cont.h
$(RELEASETARGETDIR)/sql_util.o: ../../../src/server/include/mdb_BaseCont.h
$(RELEASETARGETDIR)/sql_util.o: ../../../src/server/include/mdb_Collect.h
$(RELEASETARGETDIR)/sql_util.o: ../../../src/server/include/mdb_PageList.h
$(RELEASETARGETDIR)/sql_util.o: ../../../src/server/include/mdb_Page.h
$(RELEASETARGETDIR)/sql_util.o: ../../../src/server/include/mdb_TTree.h
$(RELEASETARGETDIR)/sql_util.o: ../../../src/server/include/mdb_TTreeNode.h
$(RELEASETARGETDIR)/sql_util.o: ../../../src/server/include/mdb_TTreeIter.h
$(RELEASETARGETDIR)/sql_util.o: ../../../src/include/isql.h
$(RELEASETARGETDIR)/sql_util.o: ../include/sql_const.h
$(RELEASETARGETDIR)/sql_util.o: ../include/sql_datast.h ../include/sql_util.h
$(RELEASETARGETDIR)/sql_util.o: ../include/sql_packet_format.h
$(RELEASETARGETDIR)/sql_util.o: ../../../src/server/include/mdb_PMEM.h
$(RELEASETARGETDIR)/sql_util.o: ../../../src/server/include/mdb_er.h
$(RELEASETARGETDIR)/sql_util.o: ../../../src/server/include/mdb_charset.h
$(RELEASETARGETDIR)/sql_util.o: sql_decimal.h sql_func_timeutil.h

$(RELEASETARGETDIR)/sql_calc_expression.o: sql_calc_expression.c
$(RELEASETARGETDIR)/sql_calc_expression.o: ../../../src/include/mdb_config.h
$(RELEASETARGETDIR)/sql_calc_expression.o: ../../../src/server/include/mdb_compat.h
$(RELEASETARGETDIR)/sql_calc_expression.o: ../../../src/include/sys_inc.h
$(RELEASETARGETDIR)/sql_calc_expression.o: ../../../src/include/mdb_typedef.h
$(RELEASETARGETDIR)/sql_calc_expression.o: ../../../src/include/dbport.h
$(RELEASETARGETDIR)/sql_calc_expression.o: ../include/sql_main.h
$(RELEASETARGETDIR)/sql_calc_expression.o: ../../../src/server/include/mdb_dbi.h
$(RELEASETARGETDIR)/sql_calc_expression.o: ../../../src/include/systable.h
$(RELEASETARGETDIR)/sql_calc_expression.o: ../../../src/include/mdb_define.h
$(RELEASETARGETDIR)/sql_calc_expression.o: ../../../src/include/ErrorCode.h
$(RELEASETARGETDIR)/sql_calc_expression.o: ../../../src/server/include/mdb_FieldDesc.h
$(RELEASETARGETDIR)/sql_calc_expression.o: ../../../src/server/include/mdb_FieldVal.h
$(RELEASETARGETDIR)/sql_calc_expression.o: ../../../src/server/include/mdb_KeyDesc.h
$(RELEASETARGETDIR)/sql_calc_expression.o: ../../../src/server/include/mdb_inner_define.h
$(RELEASETARGETDIR)/sql_calc_expression.o: ../../../src/include/db_api.h
$(RELEASETARGETDIR)/sql_calc_expression.o: ../../../src/server/include/mdb_KeyValue.h
$(RELEASETARGETDIR)/sql_calc_expression.o: ../../../src/server/include/mdb_Filter.h
$(RELEASETARGETDIR)/sql_calc_expression.o: ../../../src/server/include/mdb_schema_cache.h
$(RELEASETARGETDIR)/sql_calc_expression.o: ../../../src/server/include/mdb_Trans.h
$(RELEASETARGETDIR)/sql_calc_expression.o: ../../../src/server/include/mdb_LSN.h
$(RELEASETARGETDIR)/sql_calc_expression.o: ../../../src/server/include/mdb_LockMgrPI.h
$(RELEASETARGETDIR)/sql_calc_expression.o: ../../../src/server/include/mdb_ppthread.h
$(RELEASETARGETDIR)/sql_calc_expression.o: ../../../src/server/include/mdb_Csr.h
$(RELEASETARGETDIR)/sql_calc_expression.o: ../../../src/server/include/mdb_ContIter.h
$(RELEASETARGETDIR)/sql_calc_expression.o: ../../../src/server/include/mdb_Iterator.h
$(RELEASETARGETDIR)/sql_calc_expression.o: ../../../src/server/include/mdb_Cont.h
$(RELEASETARGETDIR)/sql_calc_expression.o: ../../../src/server/include/mdb_BaseCont.h
$(RELEASETARGETDIR)/sql_calc_expression.o: ../../../src/server/include/mdb_Collect.h
$(RELEASETARGETDIR)/sql_calc_expression.o: ../../../src/server/include/mdb_PageList.h
$(RELEASETARGETDIR)/sql_calc_expression.o: ../../../src/server/include/mdb_Page.h
$(RELEASETARGETDIR)/sql_calc_expression.o: ../../../src/server/include/mdb_TTree.h
$(RELEASETARGETDIR)/sql_calc_expression.o: ../../../src/server/include/mdb_TTreeNode.h
$(RELEASETARGETDIR)/sql_calc_expression.o: ../../../src/server/include/mdb_TTreeIter.h
$(RELEASETARGETDIR)/sql_calc_expression.o: ../../../src/include/isql.h
$(RELEASETARGETDIR)/sql_calc_expression.o: ../include/sql_const.h
$(RELEASETARGETDIR)/sql_calc_expression.o: ../include/sql_datast.h
$(RELEASETARGETDIR)/sql_calc_expression.o: ../include/sql_util.h
$(RELEASETARGETDIR)/sql_calc_expression.o: ../include/sql_packet_format.h
$(RELEASETARGETDIR)/sql_calc_expression.o: ../../../src/server/include/mdb_PMEM.h
$(RELEASETARGETDIR)/sql_calc_expression.o: ../../../src/server/include/mdb_er.h
$(RELEASETARGETDIR)/sql_calc_expression.o: ../../../src/server/include/mdb_charset.h
$(RELEASETARGETDIR)/sql_calc_expression.o: sql_func_timeutil.h sql_mclient.h
$(RELEASETARGETDIR)/sql_calc_expression.o: ../include/sql_parser.h

$(RELEASETARGETDIR)/sql_calc_function.o: sql_calc_function.c
$(RELEASETARGETDIR)/sql_calc_function.o: ../../../src/include/mdb_config.h
$(RELEASETARGETDIR)/sql_calc_function.o: ../../../src/include/sys_inc.h
$(RELEASETARGETDIR)/sql_calc_function.o: ../../../src/server/include/mdb_compat.h
$(RELEASETARGETDIR)/sql_calc_function.o: ../../../src/include/mdb_typedef.h
$(RELEASETARGETDIR)/sql_calc_function.o: ../../../src/include/dbport.h
$(RELEASETARGETDIR)/sql_calc_function.o: ../../../src/include/isql.h
$(RELEASETARGETDIR)/sql_calc_function.o: ../../../src/include/mdb_define.h
$(RELEASETARGETDIR)/sql_calc_function.o: ../../../src/include/ErrorCode.h
$(RELEASETARGETDIR)/sql_calc_function.o: ../../../src/server/include/mdb_FieldDesc.h
$(RELEASETARGETDIR)/sql_calc_function.o: ../../../src/server/include/mdb_FieldVal.h
$(RELEASETARGETDIR)/sql_calc_function.o: ../../../src/include/db_api.h
$(RELEASETARGETDIR)/sql_calc_function.o: ../../../src/server/include/mdb_KeyValue.h
$(RELEASETARGETDIR)/sql_calc_function.o: ../../../src/server/include/mdb_KeyDesc.h
$(RELEASETARGETDIR)/sql_calc_function.o: ../../../src/server/include/mdb_Filter.h
$(RELEASETARGETDIR)/sql_calc_function.o: ../../../src/include/systable.h
$(RELEASETARGETDIR)/sql_calc_function.o: ../include/sql_datast.h
$(RELEASETARGETDIR)/sql_calc_function.o: ../../../src/server/include/mdb_Csr.h
$(RELEASETARGETDIR)/sql_calc_function.o: ../../../src/server/include/mdb_ContIter.h
$(RELEASETARGETDIR)/sql_calc_function.o: ../../../src/server/include/mdb_Iterator.h
$(RELEASETARGETDIR)/sql_calc_function.o: ../../../src/server/include/mdb_Cont.h
$(RELEASETARGETDIR)/sql_calc_function.o: ../../../src/server/include/mdb_BaseCont.h
$(RELEASETARGETDIR)/sql_calc_function.o: ../../../src/server/include/mdb_inner_define.h
$(RELEASETARGETDIR)/sql_calc_function.o: ../../../src/server/include/mdb_Collect.h
$(RELEASETARGETDIR)/sql_calc_function.o: ../../../src/server/include/mdb_PageList.h
$(RELEASETARGETDIR)/sql_calc_function.o: ../../../src/server/include/mdb_Page.h
$(RELEASETARGETDIR)/sql_calc_function.o: ../../../src/server/include/mdb_TTree.h
$(RELEASETARGETDIR)/sql_calc_function.o: ../../../src/server/include/mdb_TTreeNode.h
$(RELEASETARGETDIR)/sql_calc_function.o: ../../../src/server/include/mdb_LockMgrPI.h
$(RELEASETARGETDIR)/sql_calc_function.o: ../../../src/server/include/mdb_ppthread.h
$(RELEASETARGETDIR)/sql_calc_function.o: ../../../src/server/include/mdb_Trans.h
$(RELEASETARGETDIR)/sql_calc_function.o: ../../../src/server/include/mdb_LSN.h
$(RELEASETARGETDIR)/sql_calc_function.o: ../../../src/server/include/mdb_TTreeIter.h
$(RELEASETARGETDIR)/sql_calc_function.o: ../include/sql_const.h
$(RELEASETARGETDIR)/sql_calc_function.o: ../include/sql_util.h
$(RELEASETARGETDIR)/sql_calc_function.o: ../include/sql_packet_format.h
$(RELEASETARGETDIR)/sql_calc_function.o: ../include/sql_main.h
$(RELEASETARGETDIR)/sql_calc_function.o: ../../../src/server/include/mdb_dbi.h
$(RELEASETARGETDIR)/sql_calc_function.o: ../../../src/server/include/mdb_schema_cache.h
$(RELEASETARGETDIR)/sql_calc_function.o: ../../../src/server/include/mdb_er.h
$(RELEASETARGETDIR)/sql_calc_function.o: ../../../src/server/include/mdb_charset.h
$(RELEASETARGETDIR)/sql_calc_function.o: ../../../src/server/include/mdb_PMEM.h
$(RELEASETARGETDIR)/sql_calc_function.o: sql_calc_timefunc.h sql_func_timeutil.h

$(RELEASETARGETDIR)/sql_calc_timefunc.o: sql_calc_timefunc.c
$(RELEASETARGETDIR)/sql_calc_timefunc.o: ../../../src/include/mdb_config.h
$(RELEASETARGETDIR)/sql_calc_timefunc.o: ../../../src/include/sys_inc.h
$(RELEASETARGETDIR)/sql_calc_timefunc.o: ../../../src/server/include/mdb_compat.h
$(RELEASETARGETDIR)/sql_calc_timefunc.o: ../../../src/include/mdb_typedef.h
$(RELEASETARGETDIR)/sql_calc_timefunc.o: ../../../src/include/dbport.h
$(RELEASETARGETDIR)/sql_calc_timefunc.o: ../../../src/include/isql.h
$(RELEASETARGETDIR)/sql_calc_timefunc.o: ../../../src/include/mdb_define.h
$(RELEASETARGETDIR)/sql_calc_timefunc.o: ../../../src/include/ErrorCode.h
$(RELEASETARGETDIR)/sql_calc_timefunc.o: ../../../src/server/include/mdb_FieldDesc.h
$(RELEASETARGETDIR)/sql_calc_timefunc.o: ../../../src/server/include/mdb_FieldVal.h
$(RELEASETARGETDIR)/sql_calc_timefunc.o: ../../../src/include/db_api.h
$(RELEASETARGETDIR)/sql_calc_timefunc.o: ../../../src/server/include/mdb_KeyValue.h
$(RELEASETARGETDIR)/sql_calc_timefunc.o: ../../../src/server/include/mdb_KeyDesc.h
$(RELEASETARGETDIR)/sql_calc_timefunc.o: ../../../src/server/include/mdb_Filter.h
$(RELEASETARGETDIR)/sql_calc_timefunc.o: ../../../src/include/systable.h
$(RELEASETARGETDIR)/sql_calc_timefunc.o: ../include/sql_datast.h
$(RELEASETARGETDIR)/sql_calc_timefunc.o: ../../../src/server/include/mdb_Csr.h
$(RELEASETARGETDIR)/sql_calc_timefunc.o: ../../../src/server/include/mdb_ContIter.h
$(RELEASETARGETDIR)/sql_calc_timefunc.o: ../../../src/server/include/mdb_Iterator.h
$(RELEASETARGETDIR)/sql_calc_timefunc.o: ../../../src/server/include/mdb_Cont.h
$(RELEASETARGETDIR)/sql_calc_timefunc.o: ../../../src/server/include/mdb_BaseCont.h
$(RELEASETARGETDIR)/sql_calc_timefunc.o: ../../../src/server/include/mdb_inner_define.h
$(RELEASETARGETDIR)/sql_calc_timefunc.o: ../../../src/server/include/mdb_Collect.h
$(RELEASETARGETDIR)/sql_calc_timefunc.o: ../../../src/server/include/mdb_PageList.h
$(RELEASETARGETDIR)/sql_calc_timefunc.o: ../../../src/server/include/mdb_Page.h
$(RELEASETARGETDIR)/sql_calc_timefunc.o: ../../../src/server/include/mdb_TTree.h
$(RELEASETARGETDIR)/sql_calc_timefunc.o: ../../../src/server/include/mdb_TTreeNode.h
$(RELEASETARGETDIR)/sql_calc_timefunc.o: ../../../src/server/include/mdb_LockMgrPI.h
$(RELEASETARGETDIR)/sql_calc_timefunc.o: ../../../src/server/include/mdb_ppthread.h
$(RELEASETARGETDIR)/sql_calc_timefunc.o: ../../../src/server/include/mdb_Trans.h
$(RELEASETARGETDIR)/sql_calc_timefunc.o: ../../../src/server/include/mdb_LSN.h
$(RELEASETARGETDIR)/sql_calc_timefunc.o: ../../../src/server/include/mdb_TTreeIter.h
$(RELEASETARGETDIR)/sql_calc_timefunc.o: ../include/sql_const.h
$(RELEASETARGETDIR)/sql_calc_timefunc.o: ../include/sql_util.h
$(RELEASETARGETDIR)/sql_calc_timefunc.o: ../include/sql_packet_format.h
$(RELEASETARGETDIR)/sql_calc_timefunc.o: ../include/sql_main.h
$(RELEASETARGETDIR)/sql_calc_timefunc.o: ../../../src/server/include/mdb_dbi.h
$(RELEASETARGETDIR)/sql_calc_timefunc.o: ../../../src/server/include/mdb_schema_cache.h
$(RELEASETARGETDIR)/sql_calc_timefunc.o: ../../../src/server/include/mdb_er.h
$(RELEASETARGETDIR)/sql_calc_timefunc.o: ../../../src/server/include/mdb_charset.h
$(RELEASETARGETDIR)/sql_calc_timefunc.o: ../../../src/server/include/mdb_PMEM.h
$(RELEASETARGETDIR)/sql_calc_timefunc.o: sql_func_timeutil.h

$(RELEASETARGETDIR)/sql_func_timeutil.o: sql_func_timeutil.c
$(RELEASETARGETDIR)/sql_func_timeutil.o: ../../../src/include/mdb_config.h
$(RELEASETARGETDIR)/sql_func_timeutil.o: ../../../src/include/sys_inc.h
$(RELEASETARGETDIR)/sql_func_timeutil.o: ../../../src/server/include/mdb_compat.h
$(RELEASETARGETDIR)/sql_func_timeutil.o: ../../../src/include/mdb_typedef.h
$(RELEASETARGETDIR)/sql_func_timeutil.o: ../../../src/include/dbport.h
$(RELEASETARGETDIR)/sql_func_timeutil.o: ../include/sql_datast.h
$(RELEASETARGETDIR)/sql_func_timeutil.o: ../../../src/include/systable.h
$(RELEASETARGETDIR)/sql_func_timeutil.o: ../../../src/include/mdb_define.h
$(RELEASETARGETDIR)/sql_func_timeutil.o: ../../../src/include/ErrorCode.h
$(RELEASETARGETDIR)/sql_func_timeutil.o: ../../../src/server/include/mdb_FieldDesc.h
$(RELEASETARGETDIR)/sql_func_timeutil.o: ../../../src/server/include/mdb_FieldVal.h
$(RELEASETARGETDIR)/sql_func_timeutil.o: ../../../src/server/include/mdb_KeyDesc.h
$(RELEASETARGETDIR)/sql_func_timeutil.o: ../../../src/server/include/mdb_Csr.h
$(RELEASETARGETDIR)/sql_func_timeutil.o: ../../../src/server/include/mdb_ContIter.h
$(RELEASETARGETDIR)/sql_func_timeutil.o: ../../../src/server/include/mdb_Iterator.h
$(RELEASETARGETDIR)/sql_func_timeutil.o: ../../../src/server/include/mdb_Cont.h
$(RELEASETARGETDIR)/sql_func_timeutil.o: ../../../src/server/include/mdb_BaseCont.h
$(RELEASETARGETDIR)/sql_func_timeutil.o: ../../../src/server/include/mdb_inner_define.h
$(RELEASETARGETDIR)/sql_func_timeutil.o: ../../../src/server/include/mdb_Collect.h
$(RELEASETARGETDIR)/sql_func_timeutil.o: ../../../src/server/include/mdb_PageList.h
$(RELEASETARGETDIR)/sql_func_timeutil.o: ../../../src/server/include/mdb_Page.h
$(RELEASETARGETDIR)/sql_func_timeutil.o: ../../../src/server/include/mdb_TTree.h
$(RELEASETARGETDIR)/sql_func_timeutil.o: ../../../src/server/include/mdb_TTreeNode.h
$(RELEASETARGETDIR)/sql_func_timeutil.o: ../../../src/server/include/mdb_KeyValue.h
$(RELEASETARGETDIR)/sql_func_timeutil.o: ../../../src/server/include/mdb_LockMgrPI.h
$(RELEASETARGETDIR)/sql_func_timeutil.o: ../../../src/server/include/mdb_ppthread.h
$(RELEASETARGETDIR)/sql_func_timeutil.o: ../../../src/server/include/mdb_Trans.h
$(RELEASETARGETDIR)/sql_func_timeutil.o: ../../../src/server/include/mdb_LSN.h
$(RELEASETARGETDIR)/sql_func_timeutil.o: ../../../src/server/include/mdb_Filter.h
$(RELEASETARGETDIR)/sql_func_timeutil.o: ../../../src/server/include/mdb_TTreeIter.h
$(RELEASETARGETDIR)/sql_func_timeutil.o: ../include/sql_const.h
$(RELEASETARGETDIR)/sql_func_timeutil.o: ../../../src/include/isql.h
$(RELEASETARGETDIR)/sql_func_timeutil.o: ../../../src/include/db_api.h
$(RELEASETARGETDIR)/sql_func_timeutil.o: ../include/sql_util.h
$(RELEASETARGETDIR)/sql_func_timeutil.o: ../include/sql_packet_format.h
$(RELEASETARGETDIR)/sql_func_timeutil.o: ../include/sql_main.h
$(RELEASETARGETDIR)/sql_func_timeutil.o: ../../../src/server/include/mdb_dbi.h
$(RELEASETARGETDIR)/sql_func_timeutil.o: ../../../src/server/include/mdb_schema_cache.h
$(RELEASETARGETDIR)/sql_func_timeutil.o: ../../../src/server/include/mdb_er.h
$(RELEASETARGETDIR)/sql_func_timeutil.o: ../../../src/server/include/mdb_charset.h
$(RELEASETARGETDIR)/sql_func_timeutil.o: ../../../src/server/include/mdb_PMEM.h
$(RELEASETARGETDIR)/sql_func_timeutil.o: sql_func_timeutil.h

$(RELEASETARGETDIR)/sql_worktable.o: sql_worktable.c
$(RELEASETARGETDIR)/sql_worktable.o: ../../../src/include/mdb_config.h
$(RELEASETARGETDIR)/sql_worktable.o: ../../../src/include/sys_inc.h
$(RELEASETARGETDIR)/sql_worktable.o: ../include/sql_main.h
$(RELEASETARGETDIR)/sql_worktable.o: ../../../src/server/include/mdb_dbi.h
$(RELEASETARGETDIR)/sql_worktable.o: ../../../src/include/systable.h
$(RELEASETARGETDIR)/sql_worktable.o: ../../../src/include/mdb_define.h
$(RELEASETARGETDIR)/sql_worktable.o: ../../../src/include/ErrorCode.h
$(RELEASETARGETDIR)/sql_worktable.o: ../../../src/include/mdb_typedef.h
$(RELEASETARGETDIR)/sql_worktable.o: ../../../src/include/dbport.h
$(RELEASETARGETDIR)/sql_worktable.o: ../../../src/server/include/mdb_FieldDesc.h
$(RELEASETARGETDIR)/sql_worktable.o: ../../../src/server/include/mdb_FieldVal.h
$(RELEASETARGETDIR)/sql_worktable.o: ../../../src/server/include/mdb_KeyDesc.h
$(RELEASETARGETDIR)/sql_worktable.o: ../../../src/server/include/mdb_inner_define.h
$(RELEASETARGETDIR)/sql_worktable.o: ../../../src/include/db_api.h
$(RELEASETARGETDIR)/sql_worktable.o: ../../../src/server/include/mdb_KeyValue.h
$(RELEASETARGETDIR)/sql_worktable.o: ../../../src/server/include/mdb_Filter.h
$(RELEASETARGETDIR)/sql_worktable.o: ../../../src/server/include/mdb_schema_cache.h
$(RELEASETARGETDIR)/sql_worktable.o: ../../../src/server/include/mdb_Trans.h
$(RELEASETARGETDIR)/sql_worktable.o: ../../../src/server/include/mdb_LSN.h
$(RELEASETARGETDIR)/sql_worktable.o: ../../../src/server/include/mdb_LockMgrPI.h
$(RELEASETARGETDIR)/sql_worktable.o: ../../../src/server/include/mdb_ppthread.h
$(RELEASETARGETDIR)/sql_worktable.o: ../../../src/server/include/mdb_compat.h
$(RELEASETARGETDIR)/sql_worktable.o: ../../../src/server/include/mdb_Csr.h
$(RELEASETARGETDIR)/sql_worktable.o: ../../../src/server/include/mdb_ContIter.h
$(RELEASETARGETDIR)/sql_worktable.o: ../../../src/server/include/mdb_Iterator.h
$(RELEASETARGETDIR)/sql_worktable.o: ../../../src/server/include/mdb_Cont.h
$(RELEASETARGETDIR)/sql_worktable.o: ../../../src/server/include/mdb_BaseCont.h
$(RELEASETARGETDIR)/sql_worktable.o: ../../../src/server/include/mdb_Collect.h
$(RELEASETARGETDIR)/sql_worktable.o: ../../../src/server/include/mdb_PageList.h
$(RELEASETARGETDIR)/sql_worktable.o: ../../../src/server/include/mdb_Page.h
$(RELEASETARGETDIR)/sql_worktable.o: ../../../src/server/include/mdb_TTree.h
$(RELEASETARGETDIR)/sql_worktable.o: ../../../src/server/include/mdb_TTreeNode.h
$(RELEASETARGETDIR)/sql_worktable.o: ../../../src/server/include/mdb_TTreeIter.h
$(RELEASETARGETDIR)/sql_worktable.o: ../../../src/include/isql.h
$(RELEASETARGETDIR)/sql_worktable.o: ../include/sql_const.h
$(RELEASETARGETDIR)/sql_worktable.o: ../include/sql_datast.h
$(RELEASETARGETDIR)/sql_worktable.o: ../include/sql_util.h
$(RELEASETARGETDIR)/sql_worktable.o: ../include/sql_packet_format.h
$(RELEASETARGETDIR)/sql_worktable.o: ../../../src/server/include/mdb_PMEM.h
$(RELEASETARGETDIR)/sql_worktable.o: ../../../src/server/include/mdb_er.h
$(RELEASETARGETDIR)/sql_worktable.o: ../../../src/server/include/mdb_charset.h
$(RELEASETARGETDIR)/sql_worktable.o: sql_decimal.h ../include/sql_execute.h

$(RELEASETARGETDIR)/sql_drop.o: sql_drop.c
$(RELEASETARGETDIR)/sql_drop.o: ../include/sql_main.h
$(RELEASETARGETDIR)/sql_drop.o: ../../../src/include/mdb_config.h
$(RELEASETARGETDIR)/sql_drop.o: ../../../src/server/include/mdb_dbi.h
$(RELEASETARGETDIR)/sql_drop.o: ../../../src/include/sys_inc.h
$(RELEASETARGETDIR)/sql_drop.o: ../../../src/include/systable.h
$(RELEASETARGETDIR)/sql_drop.o: ../../../src/include/mdb_define.h
$(RELEASETARGETDIR)/sql_drop.o: ../../../src/include/ErrorCode.h
$(RELEASETARGETDIR)/sql_drop.o: ../../../src/include/mdb_typedef.h
$(RELEASETARGETDIR)/sql_drop.o: ../../../src/include/dbport.h
$(RELEASETARGETDIR)/sql_drop.o: ../../../src/server/include/mdb_FieldDesc.h
$(RELEASETARGETDIR)/sql_drop.o: ../../../src/server/include/mdb_FieldVal.h
$(RELEASETARGETDIR)/sql_drop.o: ../../../src/server/include/mdb_KeyDesc.h
$(RELEASETARGETDIR)/sql_drop.o: ../../../src/server/include/mdb_inner_define.h
$(RELEASETARGETDIR)/sql_drop.o: ../../../src/include/db_api.h
$(RELEASETARGETDIR)/sql_drop.o: ../../../src/server/include/mdb_KeyValue.h
$(RELEASETARGETDIR)/sql_drop.o: ../../../src/server/include/mdb_Filter.h
$(RELEASETARGETDIR)/sql_drop.o: ../../../src/server/include/mdb_schema_cache.h
$(RELEASETARGETDIR)/sql_drop.o: ../../../src/server/include/mdb_Trans.h
$(RELEASETARGETDIR)/sql_drop.o: ../../../src/server/include/mdb_LSN.h
$(RELEASETARGETDIR)/sql_drop.o: ../../../src/server/include/mdb_LockMgrPI.h
$(RELEASETARGETDIR)/sql_drop.o: ../../../src/server/include/mdb_ppthread.h
$(RELEASETARGETDIR)/sql_drop.o: ../../../src/server/include/mdb_compat.h
$(RELEASETARGETDIR)/sql_drop.o: ../../../src/server/include/mdb_Csr.h
$(RELEASETARGETDIR)/sql_drop.o: ../../../src/server/include/mdb_ContIter.h
$(RELEASETARGETDIR)/sql_drop.o: ../../../src/server/include/mdb_Iterator.h
$(RELEASETARGETDIR)/sql_drop.o: ../../../src/server/include/mdb_Cont.h
$(RELEASETARGETDIR)/sql_drop.o: ../../../src/server/include/mdb_BaseCont.h
$(RELEASETARGETDIR)/sql_drop.o: ../../../src/server/include/mdb_Collect.h
$(RELEASETARGETDIR)/sql_drop.o: ../../../src/server/include/mdb_PageList.h
$(RELEASETARGETDIR)/sql_drop.o: ../../../src/server/include/mdb_Page.h
$(RELEASETARGETDIR)/sql_drop.o: ../../../src/server/include/mdb_TTree.h
$(RELEASETARGETDIR)/sql_drop.o: ../../../src/server/include/mdb_TTreeNode.h
$(RELEASETARGETDIR)/sql_drop.o: ../../../src/server/include/mdb_TTreeIter.h
$(RELEASETARGETDIR)/sql_drop.o: ../../../src/include/isql.h
$(RELEASETARGETDIR)/sql_drop.o: ../include/sql_const.h
$(RELEASETARGETDIR)/sql_drop.o: ../include/sql_datast.h ../include/sql_util.h
$(RELEASETARGETDIR)/sql_drop.o: ../include/sql_packet_format.h
$(RELEASETARGETDIR)/sql_drop.o: ../../../src/server/include/mdb_PMEM.h
$(RELEASETARGETDIR)/sql_drop.o: ../../../src/server/include/mdb_er.h
$(RELEASETARGETDIR)/sql_drop.o: ../../../src/server/include/mdb_charset.h
$(RELEASETARGETDIR)/sql_drop.o: sql_decimal.h ../include/sql_parser.h
$(RELEASETARGETDIR)/sql_drop.o: sql_mclient.h

$(RELEASETARGETDIR)/sql_create.o: sql_create.c
$(RELEASETARGETDIR)/sql_create.o: ../../../src/include/mdb_config.h
$(RELEASETARGETDIR)/sql_create.o: ../../../src/include/sys_inc.h
$(RELEASETARGETDIR)/sql_create.o: ../../../src/server/include/mdb_compat.h
$(RELEASETARGETDIR)/sql_create.o: ../../../src/include/mdb_typedef.h
$(RELEASETARGETDIR)/sql_create.o: ../../../src/include/dbport.h
$(RELEASETARGETDIR)/sql_create.o: ../include/sql_datast.h
$(RELEASETARGETDIR)/sql_create.o: ../../../src/include/systable.h
$(RELEASETARGETDIR)/sql_create.o: ../../../src/include/mdb_define.h
$(RELEASETARGETDIR)/sql_create.o: ../../../src/include/ErrorCode.h
$(RELEASETARGETDIR)/sql_create.o: ../../../src/server/include/mdb_FieldDesc.h
$(RELEASETARGETDIR)/sql_create.o: ../../../src/server/include/mdb_FieldVal.h
$(RELEASETARGETDIR)/sql_create.o: ../../../src/server/include/mdb_KeyDesc.h
$(RELEASETARGETDIR)/sql_create.o: ../../../src/server/include/mdb_Csr.h
$(RELEASETARGETDIR)/sql_create.o: ../../../src/server/include/mdb_ContIter.h
$(RELEASETARGETDIR)/sql_create.o: ../../../src/server/include/mdb_Iterator.h
$(RELEASETARGETDIR)/sql_create.o: ../../../src/server/include/mdb_Cont.h
$(RELEASETARGETDIR)/sql_create.o: ../../../src/server/include/mdb_BaseCont.h
$(RELEASETARGETDIR)/sql_create.o: ../../../src/server/include/mdb_inner_define.h
$(RELEASETARGETDIR)/sql_create.o: ../../../src/server/include/mdb_Collect.h
$(RELEASETARGETDIR)/sql_create.o: ../../../src/server/include/mdb_PageList.h
$(RELEASETARGETDIR)/sql_create.o: ../../../src/server/include/mdb_Page.h
$(RELEASETARGETDIR)/sql_create.o: ../../../src/server/include/mdb_TTree.h
$(RELEASETARGETDIR)/sql_create.o: ../../../src/server/include/mdb_TTreeNode.h
$(RELEASETARGETDIR)/sql_create.o: ../../../src/server/include/mdb_KeyValue.h
$(RELEASETARGETDIR)/sql_create.o: ../../../src/server/include/mdb_LockMgrPI.h
$(RELEASETARGETDIR)/sql_create.o: ../../../src/server/include/mdb_ppthread.h
$(RELEASETARGETDIR)/sql_create.o: ../../../src/server/include/mdb_Trans.h
$(RELEASETARGETDIR)/sql_create.o: ../../../src/server/include/mdb_LSN.h
$(RELEASETARGETDIR)/sql_create.o: ../../../src/server/include/mdb_Filter.h
$(RELEASETARGETDIR)/sql_create.o: ../../../src/server/include/mdb_TTreeIter.h
$(RELEASETARGETDIR)/sql_create.o: ../include/sql_const.h
$(RELEASETARGETDIR)/sql_create.o: ../../../src/include/isql.h
$(RELEASETARGETDIR)/sql_create.o: ../../../src/include/db_api.h
$(RELEASETARGETDIR)/sql_create.o: ../include/sql_main.h
$(RELEASETARGETDIR)/sql_create.o: ../../../src/server/include/mdb_dbi.h
$(RELEASETARGETDIR)/sql_create.o: ../../../src/server/include/mdb_schema_cache.h
$(RELEASETARGETDIR)/sql_create.o: ../include/sql_util.h
$(RELEASETARGETDIR)/sql_create.o: ../include/sql_packet_format.h
$(RELEASETARGETDIR)/sql_create.o: ../../../src/server/include/mdb_PMEM.h
$(RELEASETARGETDIR)/sql_create.o: ../../../src/server/include/mdb_er.h
$(RELEASETARGETDIR)/sql_create.o: ../../../src/server/include/mdb_charset.h
$(RELEASETARGETDIR)/sql_create.o: sql_decimal.h
$(RELEASETARGETDIR)/sql_create.o: ../include/sql_parser.h sql_mclient.h

$(RELEASETARGETDIR)/sql_insert.o: sql_insert.c
$(RELEASETARGETDIR)/sql_insert.o: ../../../src/include/mdb_config.h
$(RELEASETARGETDIR)/sql_insert.o: ../../../src/include/sys_inc.h
$(RELEASETARGETDIR)/sql_insert.o: ../../../src/server/include/mdb_compat.h
$(RELEASETARGETDIR)/sql_insert.o: ../../../src/include/mdb_typedef.h
$(RELEASETARGETDIR)/sql_insert.o: ../../../src/include/dbport.h
$(RELEASETARGETDIR)/sql_insert.o: ../include/sql_main.h
$(RELEASETARGETDIR)/sql_insert.o: ../../../src/server/include/mdb_dbi.h
$(RELEASETARGETDIR)/sql_insert.o: ../../../src/include/systable.h
$(RELEASETARGETDIR)/sql_insert.o: ../../../src/include/mdb_define.h
$(RELEASETARGETDIR)/sql_insert.o: ../../../src/include/ErrorCode.h
$(RELEASETARGETDIR)/sql_insert.o: ../../../src/server/include/mdb_FieldDesc.h
$(RELEASETARGETDIR)/sql_insert.o: ../../../src/server/include/mdb_FieldVal.h
$(RELEASETARGETDIR)/sql_insert.o: ../../../src/server/include/mdb_KeyDesc.h
$(RELEASETARGETDIR)/sql_insert.o: ../../../src/server/include/mdb_inner_define.h
$(RELEASETARGETDIR)/sql_insert.o: ../../../src/include/db_api.h
$(RELEASETARGETDIR)/sql_insert.o: ../../../src/server/include/mdb_KeyValue.h
$(RELEASETARGETDIR)/sql_insert.o: ../../../src/server/include/mdb_Filter.h
$(RELEASETARGETDIR)/sql_insert.o: ../../../src/server/include/mdb_schema_cache.h
$(RELEASETARGETDIR)/sql_insert.o: ../../../src/server/include/mdb_Trans.h
$(RELEASETARGETDIR)/sql_insert.o: ../../../src/server/include/mdb_LSN.h
$(RELEASETARGETDIR)/sql_insert.o: ../../../src/server/include/mdb_LockMgrPI.h
$(RELEASETARGETDIR)/sql_insert.o: ../../../src/server/include/mdb_ppthread.h
$(RELEASETARGETDIR)/sql_insert.o: ../../../src/server/include/mdb_Csr.h
$(RELEASETARGETDIR)/sql_insert.o: ../../../src/server/include/mdb_ContIter.h
$(RELEASETARGETDIR)/sql_insert.o: ../../../src/server/include/mdb_Iterator.h
$(RELEASETARGETDIR)/sql_insert.o: ../../../src/server/include/mdb_Cont.h
$(RELEASETARGETDIR)/sql_insert.o: ../../../src/server/include/mdb_BaseCont.h
$(RELEASETARGETDIR)/sql_insert.o: ../../../src/server/include/mdb_Collect.h
$(RELEASETARGETDIR)/sql_insert.o: ../../../src/server/include/mdb_PageList.h
$(RELEASETARGETDIR)/sql_insert.o: ../../../src/server/include/mdb_Page.h
$(RELEASETARGETDIR)/sql_insert.o: ../../../src/server/include/mdb_TTree.h
$(RELEASETARGETDIR)/sql_insert.o: ../../../src/server/include/mdb_TTreeNode.h
$(RELEASETARGETDIR)/sql_insert.o: ../../../src/server/include/mdb_TTreeIter.h
$(RELEASETARGETDIR)/sql_insert.o: ../../../src/include/isql.h
$(RELEASETARGETDIR)/sql_insert.o: ../include/sql_const.h
$(RELEASETARGETDIR)/sql_insert.o: ../include/sql_datast.h
$(RELEASETARGETDIR)/sql_insert.o: ../include/sql_util.h
$(RELEASETARGETDIR)/sql_insert.o: ../include/sql_packet_format.h
$(RELEASETARGETDIR)/sql_insert.o: ../../../src/server/include/mdb_PMEM.h
$(RELEASETARGETDIR)/sql_insert.o: ../../../src/server/include/mdb_er.h
$(RELEASETARGETDIR)/sql_insert.o: ../../../src/server/include/mdb_charset.h
$(RELEASETARGETDIR)/sql_insert.o: sql_decimal.h

$(RELEASETARGETDIR)/sql_update.o: sql_update.c
$(RELEASETARGETDIR)/sql_update.o: ../../../src/include/mdb_config.h
$(RELEASETARGETDIR)/sql_update.o: ../../../src/include/sys_inc.h
$(RELEASETARGETDIR)/sql_update.o: ../../../src/server/include/mdb_compat.h
$(RELEASETARGETDIR)/sql_update.o: ../../../src/include/mdb_typedef.h
$(RELEASETARGETDIR)/sql_update.o: ../../../src/include/dbport.h
$(RELEASETARGETDIR)/sql_update.o: ../include/sql_datast.h
$(RELEASETARGETDIR)/sql_update.o: ../../../src/include/systable.h
$(RELEASETARGETDIR)/sql_update.o: ../../../src/include/mdb_define.h
$(RELEASETARGETDIR)/sql_update.o: ../../../src/include/ErrorCode.h
$(RELEASETARGETDIR)/sql_update.o: ../../../src/server/include/mdb_FieldDesc.h
$(RELEASETARGETDIR)/sql_update.o: ../../../src/server/include/mdb_FieldVal.h
$(RELEASETARGETDIR)/sql_update.o: ../../../src/server/include/mdb_KeyDesc.h
$(RELEASETARGETDIR)/sql_update.o: ../../../src/server/include/mdb_Csr.h
$(RELEASETARGETDIR)/sql_update.o: ../../../src/server/include/mdb_ContIter.h
$(RELEASETARGETDIR)/sql_update.o: ../../../src/server/include/mdb_Iterator.h
$(RELEASETARGETDIR)/sql_update.o: ../../../src/server/include/mdb_Cont.h
$(RELEASETARGETDIR)/sql_update.o: ../../../src/server/include/mdb_BaseCont.h
$(RELEASETARGETDIR)/sql_update.o: ../../../src/server/include/mdb_inner_define.h
$(RELEASETARGETDIR)/sql_update.o: ../../../src/server/include/mdb_Collect.h
$(RELEASETARGETDIR)/sql_update.o: ../../../src/server/include/mdb_PageList.h
$(RELEASETARGETDIR)/sql_update.o: ../../../src/server/include/mdb_Page.h
$(RELEASETARGETDIR)/sql_update.o: ../../../src/server/include/mdb_TTree.h
$(RELEASETARGETDIR)/sql_update.o: ../../../src/server/include/mdb_TTreeNode.h
$(RELEASETARGETDIR)/sql_update.o: ../../../src/server/include/mdb_KeyValue.h
$(RELEASETARGETDIR)/sql_update.o: ../../../src/server/include/mdb_LockMgrPI.h
$(RELEASETARGETDIR)/sql_update.o: ../../../src/server/include/mdb_ppthread.h
$(RELEASETARGETDIR)/sql_update.o: ../../../src/server/include/mdb_Trans.h
$(RELEASETARGETDIR)/sql_update.o: ../../../src/server/include/mdb_LSN.h
$(RELEASETARGETDIR)/sql_update.o: ../../../src/server/include/mdb_Filter.h
$(RELEASETARGETDIR)/sql_update.o: ../../../src/server/include/mdb_TTreeIter.h
$(RELEASETARGETDIR)/sql_update.o: ../include/sql_const.h
$(RELEASETARGETDIR)/sql_update.o: ../../../src/include/isql.h
$(RELEASETARGETDIR)/sql_update.o: ../../../src/include/db_api.h sql_decimal.h
$(RELEASETARGETDIR)/sql_update.o: ../../../src/server/include/mdb_er.h
$(RELEASETARGETDIR)/sql_update.o: ../include/sql_main.h
$(RELEASETARGETDIR)/sql_update.o: ../../../src/server/include/mdb_dbi.h
$(RELEASETARGETDIR)/sql_update.o: ../../../src/server/include/mdb_schema_cache.h
$(RELEASETARGETDIR)/sql_update.o: ../include/sql_util.h
$(RELEASETARGETDIR)/sql_update.o: ../include/sql_packet_format.h
$(RELEASETARGETDIR)/sql_update.o: ../../../src/server/include/mdb_PMEM.h
$(RELEASETARGETDIR)/sql_update.o: ../../../src/server/include/mdb_charset.h

$(RELEASETARGETDIR)/sql_delete.o: sql_delete.c
$(RELEASETARGETDIR)/sql_delete.o: ../../../src/include/mdb_config.h
$(RELEASETARGETDIR)/sql_delete.o: ../../../src/include/sys_inc.h
$(RELEASETARGETDIR)/sql_delete.o: ../../../src/server/include/mdb_compat.h
$(RELEASETARGETDIR)/sql_delete.o: ../../../src/include/mdb_typedef.h
$(RELEASETARGETDIR)/sql_delete.o: ../../../src/include/dbport.h
$(RELEASETARGETDIR)/sql_delete.o: ../include/sql_datast.h
$(RELEASETARGETDIR)/sql_delete.o: ../../../src/include/systable.h
$(RELEASETARGETDIR)/sql_delete.o: ../../../src/include/mdb_define.h
$(RELEASETARGETDIR)/sql_delete.o: ../../../src/include/ErrorCode.h
$(RELEASETARGETDIR)/sql_delete.o: ../../../src/server/include/mdb_FieldDesc.h
$(RELEASETARGETDIR)/sql_delete.o: ../../../src/server/include/mdb_FieldVal.h
$(RELEASETARGETDIR)/sql_delete.o: ../../../src/server/include/mdb_KeyDesc.h
$(RELEASETARGETDIR)/sql_delete.o: ../../../src/server/include/mdb_Csr.h
$(RELEASETARGETDIR)/sql_delete.o: ../../../src/server/include/mdb_ContIter.h
$(RELEASETARGETDIR)/sql_delete.o: ../../../src/server/include/mdb_Iterator.h
$(RELEASETARGETDIR)/sql_delete.o: ../../../src/server/include/mdb_Cont.h
$(RELEASETARGETDIR)/sql_delete.o: ../../../src/server/include/mdb_BaseCont.h
$(RELEASETARGETDIR)/sql_delete.o: ../../../src/server/include/mdb_inner_define.h
$(RELEASETARGETDIR)/sql_delete.o: ../../../src/server/include/mdb_Collect.h
$(RELEASETARGETDIR)/sql_delete.o: ../../../src/server/include/mdb_PageList.h
$(RELEASETARGETDIR)/sql_delete.o: ../../../src/server/include/mdb_Page.h
$(RELEASETARGETDIR)/sql_delete.o: ../../../src/server/include/mdb_TTree.h
$(RELEASETARGETDIR)/sql_delete.o: ../../../src/server/include/mdb_TTreeNode.h
$(RELEASETARGETDIR)/sql_delete.o: ../../../src/server/include/mdb_KeyValue.h
$(RELEASETARGETDIR)/sql_delete.o: ../../../src/server/include/mdb_LockMgrPI.h
$(RELEASETARGETDIR)/sql_delete.o: ../../../src/server/include/mdb_ppthread.h
$(RELEASETARGETDIR)/sql_delete.o: ../../../src/server/include/mdb_Trans.h
$(RELEASETARGETDIR)/sql_delete.o: ../../../src/server/include/mdb_LSN.h
$(RELEASETARGETDIR)/sql_delete.o: ../../../src/server/include/mdb_Filter.h
$(RELEASETARGETDIR)/sql_delete.o: ../../../src/server/include/mdb_TTreeIter.h
$(RELEASETARGETDIR)/sql_delete.o: ../include/sql_const.h
$(RELEASETARGETDIR)/sql_delete.o: ../../../src/include/isql.h
$(RELEASETARGETDIR)/sql_delete.o: ../../../src/include/db_api.h
$(RELEASETARGETDIR)/sql_delete.o: ../include/sql_main.h
$(RELEASETARGETDIR)/sql_delete.o: ../../../src/server/include/mdb_dbi.h
$(RELEASETARGETDIR)/sql_delete.o: ../../../src/server/include/mdb_schema_cache.h
$(RELEASETARGETDIR)/sql_delete.o: ../include/sql_util.h
$(RELEASETARGETDIR)/sql_delete.o: ../include/sql_packet_format.h
$(RELEASETARGETDIR)/sql_delete.o: ../../../src/server/include/mdb_PMEM.h
$(RELEASETARGETDIR)/sql_delete.o: ../../../src/server/include/mdb_er.h
$(RELEASETARGETDIR)/sql_delete.o: ../../../src/server/include/mdb_charset.h

$(RELEASETARGETDIR)/sql_select.o: sql_select.c
$(RELEASETARGETDIR)/sql_select.o: ../../../src/include/mdb_config.h
$(RELEASETARGETDIR)/sql_select.o: ../../../src/include/sys_inc.h
$(RELEASETARGETDIR)/sql_select.o: ../../../src/server/include/mdb_compat.h
$(RELEASETARGETDIR)/sql_select.o: ../../../src/include/mdb_typedef.h
$(RELEASETARGETDIR)/sql_select.o: ../../../src/include/dbport.h
$(RELEASETARGETDIR)/sql_select.o: ../include/sql_main.h
$(RELEASETARGETDIR)/sql_select.o: ../../../src/server/include/mdb_dbi.h
$(RELEASETARGETDIR)/sql_select.o: ../../../src/include/systable.h
$(RELEASETARGETDIR)/sql_select.o: ../../../src/include/mdb_define.h
$(RELEASETARGETDIR)/sql_select.o: ../../../src/include/ErrorCode.h
$(RELEASETARGETDIR)/sql_select.o: ../../../src/server/include/mdb_FieldDesc.h
$(RELEASETARGETDIR)/sql_select.o: ../../../src/server/include/mdb_FieldVal.h
$(RELEASETARGETDIR)/sql_select.o: ../../../src/server/include/mdb_KeyDesc.h
$(RELEASETARGETDIR)/sql_select.o: ../../../src/server/include/mdb_inner_define.h
$(RELEASETARGETDIR)/sql_select.o: ../../../src/include/db_api.h
$(RELEASETARGETDIR)/sql_select.o: ../../../src/server/include/mdb_KeyValue.h
$(RELEASETARGETDIR)/sql_select.o: ../../../src/server/include/mdb_Filter.h
$(RELEASETARGETDIR)/sql_select.o: ../../../src/server/include/mdb_schema_cache.h
$(RELEASETARGETDIR)/sql_select.o: ../../../src/server/include/mdb_Trans.h
$(RELEASETARGETDIR)/sql_select.o: ../../../src/server/include/mdb_LSN.h
$(RELEASETARGETDIR)/sql_select.o: ../../../src/server/include/mdb_LockMgrPI.h
$(RELEASETARGETDIR)/sql_select.o: ../../../src/server/include/mdb_ppthread.h
$(RELEASETARGETDIR)/sql_select.o: ../../../src/server/include/mdb_Csr.h
$(RELEASETARGETDIR)/sql_select.o: ../../../src/server/include/mdb_ContIter.h
$(RELEASETARGETDIR)/sql_select.o: ../../../src/server/include/mdb_Iterator.h
$(RELEASETARGETDIR)/sql_select.o: ../../../src/server/include/mdb_Cont.h
$(RELEASETARGETDIR)/sql_select.o: ../../../src/server/include/mdb_BaseCont.h
$(RELEASETARGETDIR)/sql_select.o: ../../../src/server/include/mdb_Collect.h
$(RELEASETARGETDIR)/sql_select.o: ../../../src/server/include/mdb_PageList.h
$(RELEASETARGETDIR)/sql_select.o: ../../../src/server/include/mdb_Page.h
$(RELEASETARGETDIR)/sql_select.o: ../../../src/server/include/mdb_TTree.h
$(RELEASETARGETDIR)/sql_select.o: ../../../src/server/include/mdb_TTreeNode.h
$(RELEASETARGETDIR)/sql_select.o: ../../../src/server/include/mdb_TTreeIter.h
$(RELEASETARGETDIR)/sql_select.o: ../../../src/include/isql.h
$(RELEASETARGETDIR)/sql_select.o: ../include/sql_const.h
$(RELEASETARGETDIR)/sql_select.o: ../include/sql_datast.h
$(RELEASETARGETDIR)/sql_select.o: ../include/sql_util.h
$(RELEASETARGETDIR)/sql_select.o: ../include/sql_packet_format.h
$(RELEASETARGETDIR)/sql_select.o: ../../../src/server/include/mdb_PMEM.h
$(RELEASETARGETDIR)/sql_select.o: ../../../src/server/include/mdb_er.h
$(RELEASETARGETDIR)/sql_select.o: ../../../src/server/include/mdb_charset.h
$(RELEASETARGETDIR)/sql_select.o: ../include/sql_execute.h sql_decimal.h
$(RELEASETARGETDIR)/sql_select.o: sql_func_timeutil.h

$(RELEASETARGETDIR)/sql_mclient.o: sql_mclient.c
$(RELEASETARGETDIR)/sql_mclient.o: ../../../src/include/mdb_config.h
$(RELEASETARGETDIR)/sql_mclient.o: ../../../src/include/sys_inc.h
$(RELEASETARGETDIR)/sql_mclient.o: ../../../src/server/include/mdb_compat.h
$(RELEASETARGETDIR)/sql_mclient.o: ../../../src/include/mdb_typedef.h
$(RELEASETARGETDIR)/sql_mclient.o: ../../../src/include/dbport.h
$(RELEASETARGETDIR)/sql_mclient.o: ../../../src/server/include/mdb_profile.h
$(RELEASETARGETDIR)/sql_mclient.o: sql_mclient.h ../include/sql_main.h
$(RELEASETARGETDIR)/sql_mclient.o: ../../../src/server/include/mdb_dbi.h
$(RELEASETARGETDIR)/sql_mclient.o: ../../../src/include/systable.h
$(RELEASETARGETDIR)/sql_mclient.o: ../../../src/include/mdb_define.h
$(RELEASETARGETDIR)/sql_mclient.o: ../../../src/include/ErrorCode.h
$(RELEASETARGETDIR)/sql_mclient.o: ../../../src/server/include/mdb_FieldDesc.h
$(RELEASETARGETDIR)/sql_mclient.o: ../../../src/server/include/mdb_FieldVal.h
$(RELEASETARGETDIR)/sql_mclient.o: ../../../src/server/include/mdb_KeyDesc.h
$(RELEASETARGETDIR)/sql_mclient.o: ../../../src/server/include/mdb_inner_define.h
$(RELEASETARGETDIR)/sql_mclient.o: ../../../src/include/db_api.h
$(RELEASETARGETDIR)/sql_mclient.o: ../../../src/server/include/mdb_KeyValue.h
$(RELEASETARGETDIR)/sql_mclient.o: ../../../src/server/include/mdb_Filter.h
$(RELEASETARGETDIR)/sql_mclient.o: ../../../src/server/include/mdb_schema_cache.h
$(RELEASETARGETDIR)/sql_mclient.o: ../../../src/server/include/mdb_Trans.h
$(RELEASETARGETDIR)/sql_mclient.o: ../../../src/server/include/mdb_LSN.h
$(RELEASETARGETDIR)/sql_mclient.o: ../../../src/server/include/mdb_LockMgrPI.h
$(RELEASETARGETDIR)/sql_mclient.o: ../../../src/server/include/mdb_ppthread.h
$(RELEASETARGETDIR)/sql_mclient.o: ../../../src/server/include/mdb_Csr.h
$(RELEASETARGETDIR)/sql_mclient.o: ../../../src/server/include/mdb_ContIter.h
$(RELEASETARGETDIR)/sql_mclient.o: ../../../src/server/include/mdb_Iterator.h
$(RELEASETARGETDIR)/sql_mclient.o: ../../../src/server/include/mdb_Cont.h
$(RELEASETARGETDIR)/sql_mclient.o: ../../../src/server/include/mdb_BaseCont.h
$(RELEASETARGETDIR)/sql_mclient.o: ../../../src/server/include/mdb_Collect.h
$(RELEASETARGETDIR)/sql_mclient.o: ../../../src/server/include/mdb_PageList.h
$(RELEASETARGETDIR)/sql_mclient.o: ../../../src/server/include/mdb_Page.h
$(RELEASETARGETDIR)/sql_mclient.o: ../../../src/server/include/mdb_TTree.h
$(RELEASETARGETDIR)/sql_mclient.o: ../../../src/server/include/mdb_TTreeNode.h
$(RELEASETARGETDIR)/sql_mclient.o: ../../../src/server/include/mdb_TTreeIter.h
$(RELEASETARGETDIR)/sql_mclient.o: ../../../src/include/isql.h
$(RELEASETARGETDIR)/sql_mclient.o: ../include/sql_const.h ../include/sql_datast.h
$(RELEASETARGETDIR)/sql_mclient.o: ../include/sql_util.h
$(RELEASETARGETDIR)/sql_mclient.o: ../include/sql_packet_format.h
$(RELEASETARGETDIR)/sql_mclient.o: ../../../src/server/include/mdb_PMEM.h
$(RELEASETARGETDIR)/sql_mclient.o: ../../../src/server/include/mdb_er.h
$(RELEASETARGETDIR)/sql_mclient.o: ../../../src/server/include/mdb_charset.h
$(RELEASETARGETDIR)/sql_mclient.o: ../include/sql_parser.h

$(RELEASETARGETDIR)/sql_interface.o: sql_interface.c
$(RELEASETARGETDIR)/sql_interface.o: ../../../src/include/mdb_config.h
$(RELEASETARGETDIR)/sql_interface.o: ../../../src/include/sys_inc.h
$(RELEASETARGETDIR)/sql_interface.o: ../../../src/server/include/mdb_comm_stub.h
$(RELEASETARGETDIR)/sql_interface.o: ../../../src/include/mdb_typedef.h
$(RELEASETARGETDIR)/sql_interface.o: ../../../src/include/dbport.h
$(RELEASETARGETDIR)/sql_interface.o: ../../../src/server/include/mdb_ppthread.h
$(RELEASETARGETDIR)/sql_interface.o: ../../../src/server/include/mdb_compat.h
$(RELEASETARGETDIR)/sql_interface.o: ../../../src/include/ErrorCode.h
$(RELEASETARGETDIR)/sql_interface.o: ../include/sql_packet_format.h
$(RELEASETARGETDIR)/sql_interface.o: ../include/sql_main.h
$(RELEASETARGETDIR)/sql_interface.o: ../../../src/server/include/mdb_dbi.h
$(RELEASETARGETDIR)/sql_interface.o: ../../../src/include/systable.h
$(RELEASETARGETDIR)/sql_interface.o: ../../../src/include/mdb_define.h
$(RELEASETARGETDIR)/sql_interface.o: ../../../src/server/include/mdb_FieldDesc.h
$(RELEASETARGETDIR)/sql_interface.o: ../../../src/server/include/mdb_FieldVal.h
$(RELEASETARGETDIR)/sql_interface.o: ../../../src/server/include/mdb_KeyDesc.h
$(RELEASETARGETDIR)/sql_interface.o: ../../../src/server/include/mdb_inner_define.h
$(RELEASETARGETDIR)/sql_interface.o: ../../../src/include/db_api.h
$(RELEASETARGETDIR)/sql_interface.o: ../../../src/server/include/mdb_KeyValue.h
$(RELEASETARGETDIR)/sql_interface.o: ../../../src/server/include/mdb_Filter.h
$(RELEASETARGETDIR)/sql_interface.o: ../../../src/server/include/mdb_schema_cache.h
$(RELEASETARGETDIR)/sql_interface.o: ../../../src/server/include/mdb_Trans.h
$(RELEASETARGETDIR)/sql_interface.o: ../../../src/server/include/mdb_LSN.h
$(RELEASETARGETDIR)/sql_interface.o: ../../../src/server/include/mdb_LockMgrPI.h
$(RELEASETARGETDIR)/sql_interface.o: ../../../src/server/include/mdb_Csr.h
$(RELEASETARGETDIR)/sql_interface.o: ../../../src/server/include/mdb_ContIter.h
$(RELEASETARGETDIR)/sql_interface.o: ../../../src/server/include/mdb_Iterator.h
$(RELEASETARGETDIR)/sql_interface.o: ../../../src/server/include/mdb_Cont.h
$(RELEASETARGETDIR)/sql_interface.o: ../../../src/server/include/mdb_BaseCont.h
$(RELEASETARGETDIR)/sql_interface.o: ../../../src/server/include/mdb_Collect.h
$(RELEASETARGETDIR)/sql_interface.o: ../../../src/server/include/mdb_PageList.h
$(RELEASETARGETDIR)/sql_interface.o: ../../../src/server/include/mdb_Page.h
$(RELEASETARGETDIR)/sql_interface.o: ../../../src/server/include/mdb_TTree.h
$(RELEASETARGETDIR)/sql_interface.o: ../../../src/server/include/mdb_TTreeNode.h
$(RELEASETARGETDIR)/sql_interface.o: ../../../src/server/include/mdb_TTreeIter.h
$(RELEASETARGETDIR)/sql_interface.o: ../../../src/include/isql.h
$(RELEASETARGETDIR)/sql_interface.o: ../include/sql_const.h
$(RELEASETARGETDIR)/sql_interface.o: ../include/sql_datast.h
$(RELEASETARGETDIR)/sql_interface.o: ../include/sql_util.h
$(RELEASETARGETDIR)/sql_interface.o: ../../../src/server/include/mdb_PMEM.h
$(RELEASETARGETDIR)/sql_interface.o: ../../../src/server/include/mdb_er.h
$(RELEASETARGETDIR)/sql_interface.o: ../../../src/server/include/mdb_charset.h
$(RELEASETARGETDIR)/sql_interface.o: sql_func_timeutil.h sql_mclient.h
$(RELEASETARGETDIR)/sql_interface.o: ../include/sql_parser.h

$(RELEASETARGETDIR)/sql_result.o: sql_result.c
$(RELEASETARGETDIR)/sql_result.o: ../../../src/include/mdb_config.h
$(RELEASETARGETDIR)/sql_result.o: ../../../src/include/sys_inc.h
$(RELEASETARGETDIR)/sql_result.o: ../include/sql_datast.h
$(RELEASETARGETDIR)/sql_result.o: ../../../src/server/include/mdb_compat.h
$(RELEASETARGETDIR)/sql_result.o: ../../../src/include/mdb_typedef.h
$(RELEASETARGETDIR)/sql_result.o: ../../../src/include/dbport.h
$(RELEASETARGETDIR)/sql_result.o: ../../../src/include/systable.h
$(RELEASETARGETDIR)/sql_result.o: ../../../src/include/mdb_define.h
$(RELEASETARGETDIR)/sql_result.o: ../../../src/include/ErrorCode.h
$(RELEASETARGETDIR)/sql_result.o: ../../../src/server/include/mdb_FieldDesc.h
$(RELEASETARGETDIR)/sql_result.o: ../../../src/server/include/mdb_FieldVal.h
$(RELEASETARGETDIR)/sql_result.o: ../../../src/server/include/mdb_KeyDesc.h
$(RELEASETARGETDIR)/sql_result.o: ../../../src/server/include/mdb_Csr.h
$(RELEASETARGETDIR)/sql_result.o: ../../../src/server/include/mdb_ContIter.h
$(RELEASETARGETDIR)/sql_result.o: ../../../src/server/include/mdb_Iterator.h
$(RELEASETARGETDIR)/sql_result.o: ../../../src/server/include/mdb_Cont.h
$(RELEASETARGETDIR)/sql_result.o: ../../../src/server/include/mdb_BaseCont.h
$(RELEASETARGETDIR)/sql_result.o: ../../../src/server/include/mdb_inner_define.h
$(RELEASETARGETDIR)/sql_result.o: ../../../src/server/include/mdb_Collect.h
$(RELEASETARGETDIR)/sql_result.o: ../../../src/server/include/mdb_PageList.h
$(RELEASETARGETDIR)/sql_result.o: ../../../src/server/include/mdb_Page.h
$(RELEASETARGETDIR)/sql_result.o: ../../../src/server/include/mdb_TTree.h
$(RELEASETARGETDIR)/sql_result.o: ../../../src/server/include/mdb_TTreeNode.h
$(RELEASETARGETDIR)/sql_result.o: ../../../src/server/include/mdb_KeyValue.h
$(RELEASETARGETDIR)/sql_result.o: ../../../src/server/include/mdb_LockMgrPI.h
$(RELEASETARGETDIR)/sql_result.o: ../../../src/server/include/mdb_ppthread.h
$(RELEASETARGETDIR)/sql_result.o: ../../../src/server/include/mdb_Trans.h
$(RELEASETARGETDIR)/sql_result.o: ../../../src/server/include/mdb_LSN.h
$(RELEASETARGETDIR)/sql_result.o: ../../../src/server/include/mdb_Filter.h
$(RELEASETARGETDIR)/sql_result.o: ../../../src/server/include/mdb_TTreeIter.h
$(RELEASETARGETDIR)/sql_result.o: ../include/sql_const.h
$(RELEASETARGETDIR)/sql_result.o: ../../../src/include/isql.h
$(RELEASETARGETDIR)/sql_result.o: ../../../src/include/db_api.h
$(RELEASETARGETDIR)/sql_result.o: ../include/sql_main.h
$(RELEASETARGETDIR)/sql_result.o: ../../../src/server/include/mdb_dbi.h
$(RELEASETARGETDIR)/sql_result.o: ../../../src/server/include/mdb_schema_cache.h
$(RELEASETARGETDIR)/sql_result.o: ../include/sql_util.h
$(RELEASETARGETDIR)/sql_result.o: ../include/sql_packet_format.h
$(RELEASETARGETDIR)/sql_result.o: ../../../src/server/include/mdb_PMEM.h
$(RELEASETARGETDIR)/sql_result.o: ../../../src/server/include/mdb_er.h
$(RELEASETARGETDIR)/sql_result.o: ../../../src/server/include/mdb_charset.h
$(RELEASETARGETDIR)/sql_result.o: sql_func_timeutil.h sql_mclient.h
$(RELEASETARGETDIR)/sql_result.o: ../include/sql_parser.h

$(RELEASETARGETDIR)/sql_decimal.o: sql_decimal.c
$(RELEASETARGETDIR)/sql_decimal.o: ../../../src/include/mdb_config.h
$(RELEASETARGETDIR)/sql_decimal.o: ../../../src/include/sys_inc.h
$(RELEASETARGETDIR)/sql_decimal.o: ../../../src/server/include/mdb_compat.h
$(RELEASETARGETDIR)/sql_decimal.o: ../../../src/include/mdb_typedef.h
$(RELEASETARGETDIR)/sql_decimal.o: ../../../src/include/dbport.h
$(RELEASETARGETDIR)/sql_decimal.o: ../include/sql_datast.h
$(RELEASETARGETDIR)/sql_decimal.o: ../../../src/include/systable.h
$(RELEASETARGETDIR)/sql_decimal.o: ../../../src/include/mdb_define.h
$(RELEASETARGETDIR)/sql_decimal.o: ../../../src/include/ErrorCode.h
$(RELEASETARGETDIR)/sql_decimal.o: ../../../src/server/include/mdb_FieldDesc.h
$(RELEASETARGETDIR)/sql_decimal.o: ../../../src/server/include/mdb_FieldVal.h
$(RELEASETARGETDIR)/sql_decimal.o: ../../../src/server/include/mdb_KeyDesc.h
$(RELEASETARGETDIR)/sql_decimal.o: ../../../src/server/include/mdb_Csr.h
$(RELEASETARGETDIR)/sql_decimal.o: ../../../src/server/include/mdb_ContIter.h
$(RELEASETARGETDIR)/sql_decimal.o: ../../../src/server/include/mdb_Iterator.h
$(RELEASETARGETDIR)/sql_decimal.o: ../../../src/server/include/mdb_Cont.h
$(RELEASETARGETDIR)/sql_decimal.o: ../../../src/server/include/mdb_BaseCont.h
$(RELEASETARGETDIR)/sql_decimal.o: ../../../src/server/include/mdb_inner_define.h
$(RELEASETARGETDIR)/sql_decimal.o: ../../../src/server/include/mdb_Collect.h
$(RELEASETARGETDIR)/sql_decimal.o: ../../../src/server/include/mdb_PageList.h
$(RELEASETARGETDIR)/sql_decimal.o: ../../../src/server/include/mdb_Page.h
$(RELEASETARGETDIR)/sql_decimal.o: ../../../src/server/include/mdb_TTree.h
$(RELEASETARGETDIR)/sql_decimal.o: ../../../src/server/include/mdb_TTreeNode.h
$(RELEASETARGETDIR)/sql_decimal.o: ../../../src/server/include/mdb_KeyValue.h
$(RELEASETARGETDIR)/sql_decimal.o: ../../../src/server/include/mdb_LockMgrPI.h
$(RELEASETARGETDIR)/sql_decimal.o: ../../../src/server/include/mdb_ppthread.h
$(RELEASETARGETDIR)/sql_decimal.o: ../../../src/server/include/mdb_Trans.h
$(RELEASETARGETDIR)/sql_decimal.o: ../../../src/server/include/mdb_LSN.h
$(RELEASETARGETDIR)/sql_decimal.o: ../../../src/server/include/mdb_Filter.h
$(RELEASETARGETDIR)/sql_decimal.o: ../../../src/server/include/mdb_TTreeIter.h
$(RELEASETARGETDIR)/sql_decimal.o: ../include/sql_const.h
$(RELEASETARGETDIR)/sql_decimal.o: ../../../src/include/isql.h
$(RELEASETARGETDIR)/sql_decimal.o: ../../../src/include/db_api.h
$(RELEASETARGETDIR)/sql_decimal.o: ../include/sql_util.h
$(RELEASETARGETDIR)/sql_decimal.o: ../include/sql_packet_format.h
$(RELEASETARGETDIR)/sql_decimal.o: ../include/sql_main.h
$(RELEASETARGETDIR)/sql_decimal.o: ../../../src/server/include/mdb_dbi.h
$(RELEASETARGETDIR)/sql_decimal.o: ../../../src/server/include/mdb_schema_cache.h
$(RELEASETARGETDIR)/sql_decimal.o: ../../../src/server/include/mdb_er.h
$(RELEASETARGETDIR)/sql_decimal.o: ../../../src/server/include/mdb_charset.h
$(RELEASETARGETDIR)/sql_decimal.o: ../../../src/server/include/mdb_PMEM.h

$(DEBUGTARGETDIR)/sql_main.o: sql_main.c
$(DEBUGTARGETDIR)/sql_main.o: ../../../src/include/mdb_config.h
$(DEBUGTARGETDIR)/sql_main.o: ../../../src/include/sys_inc.h
$(DEBUGTARGETDIR)/sql_main.o: ../../../src/server/include/mdb_compat.h
$(DEBUGTARGETDIR)/sql_main.o: ../../../src/include/mdb_typedef.h
$(DEBUGTARGETDIR)/sql_main.o: ../../../src/include/dbport.h
$(DEBUGTARGETDIR)/sql_main.o: ../include/sql_main.h
$(DEBUGTARGETDIR)/sql_main.o: ../../../src/server/include/mdb_dbi.h
$(DEBUGTARGETDIR)/sql_main.o: ../../../src/include/systable.h
$(DEBUGTARGETDIR)/sql_main.o: ../../../src/include/mdb_define.h
$(DEBUGTARGETDIR)/sql_main.o: ../../../src/include/ErrorCode.h
$(DEBUGTARGETDIR)/sql_main.o: ../../../src/server/include/mdb_FieldDesc.h
$(DEBUGTARGETDIR)/sql_main.o: ../../../src/server/include/mdb_FieldVal.h
$(DEBUGTARGETDIR)/sql_main.o: ../../../src/server/include/mdb_KeyDesc.h
$(DEBUGTARGETDIR)/sql_main.o: ../../../src/server/include/mdb_inner_define.h
$(DEBUGTARGETDIR)/sql_main.o: ../../../src/include/db_api.h
$(DEBUGTARGETDIR)/sql_main.o: ../../../src/server/include/mdb_KeyValue.h
$(DEBUGTARGETDIR)/sql_main.o: ../../../src/server/include/mdb_Filter.h
$(DEBUGTARGETDIR)/sql_main.o: ../../../src/server/include/mdb_schema_cache.h
$(DEBUGTARGETDIR)/sql_main.o: ../../../src/server/include/mdb_Trans.h
$(DEBUGTARGETDIR)/sql_main.o: ../../../src/server/include/mdb_LSN.h
$(DEBUGTARGETDIR)/sql_main.o: ../../../src/server/include/mdb_LockMgrPI.h
$(DEBUGTARGETDIR)/sql_main.o: ../../../src/server/include/mdb_ppthread.h
$(DEBUGTARGETDIR)/sql_main.o: ../../../src/server/include/mdb_Csr.h
$(DEBUGTARGETDIR)/sql_main.o: ../../../src/server/include/mdb_ContIter.h
$(DEBUGTARGETDIR)/sql_main.o: ../../../src/server/include/mdb_Iterator.h
$(DEBUGTARGETDIR)/sql_main.o: ../../../src/server/include/mdb_Cont.h
$(DEBUGTARGETDIR)/sql_main.o: ../../../src/server/include/mdb_BaseCont.h
$(DEBUGTARGETDIR)/sql_main.o: ../../../src/server/include/mdb_Collect.h
$(DEBUGTARGETDIR)/sql_main.o: ../../../src/server/include/mdb_PageList.h
$(DEBUGTARGETDIR)/sql_main.o: ../../../src/server/include/mdb_Page.h
$(DEBUGTARGETDIR)/sql_main.o: ../../../src/server/include/mdb_TTree.h
$(DEBUGTARGETDIR)/sql_main.o: ../../../src/server/include/mdb_TTreeNode.h
$(DEBUGTARGETDIR)/sql_main.o: ../../../src/server/include/mdb_TTreeIter.h
$(DEBUGTARGETDIR)/sql_main.o: ../../../src/include/isql.h
$(DEBUGTARGETDIR)/sql_main.o: ../include/sql_const.h ../include/sql_datast.h
$(DEBUGTARGETDIR)/sql_main.o: ../include/sql_util.h
$(DEBUGTARGETDIR)/sql_main.o: ../include/sql_packet_format.h
$(DEBUGTARGETDIR)/sql_main.o: ../../../src/server/include/mdb_PMEM.h
$(DEBUGTARGETDIR)/sql_main.o: ../../../src/server/include/mdb_er.h
$(DEBUGTARGETDIR)/sql_main.o: ../../../src/server/include/mdb_charset.h
$(DEBUGTARGETDIR)/sql_main.o: ../include/sql_parser.h ../include/sql_plan.h
$(DEBUGTARGETDIR)/sql_main.o: ../include/sql_execute.h ../include/sql_norm.h
$(DEBUGTARGETDIR)/sql_main.o: sql_func_timeutil.h sql_mclient.h

$(DEBUGTARGETDIR)/sql_parser.o: sql_parser.c
$(DEBUGTARGETDIR)/sql_parser.o: ../../../src/include/mdb_config.h
$(DEBUGTARGETDIR)/sql_parser.o: ../../../src/include/sys_inc.h
$(DEBUGTARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_compat.h
$(DEBUGTARGETDIR)/sql_parser.o: ../../../src/include/mdb_typedef.h
$(DEBUGTARGETDIR)/sql_parser.o: ../../../src/include/dbport.h
$(DEBUGTARGETDIR)/sql_parser.o: ../include/sql_parser.h
$(DEBUGTARGETDIR)/sql_parser.o: ../include/sql_datast.h
$(DEBUGTARGETDIR)/sql_parser.o: ../../../src/include/systable.h
$(DEBUGTARGETDIR)/sql_parser.o: ../../../src/include/mdb_define.h
$(DEBUGTARGETDIR)/sql_parser.o: ../../../src/include/ErrorCode.h
$(DEBUGTARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_FieldDesc.h
$(DEBUGTARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_FieldVal.h
$(DEBUGTARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_KeyDesc.h
$(DEBUGTARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_Csr.h
$(DEBUGTARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_ContIter.h
$(DEBUGTARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_Iterator.h
$(DEBUGTARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_Cont.h
$(DEBUGTARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_BaseCont.h
$(DEBUGTARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_inner_define.h
$(DEBUGTARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_Collect.h
$(DEBUGTARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_PageList.h
$(DEBUGTARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_Page.h
$(DEBUGTARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_TTree.h
$(DEBUGTARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_TTreeNode.h
$(DEBUGTARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_KeyValue.h
$(DEBUGTARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_LockMgrPI.h
$(DEBUGTARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_ppthread.h
$(DEBUGTARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_Trans.h
$(DEBUGTARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_LSN.h
$(DEBUGTARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_Filter.h
$(DEBUGTARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_TTreeIter.h
$(DEBUGTARGETDIR)/sql_parser.o: ../include/sql_const.h
$(DEBUGTARGETDIR)/sql_parser.o: ../../../src/include/isql.h
$(DEBUGTARGETDIR)/sql_parser.o: ../../../src/include/db_api.h
$(DEBUGTARGETDIR)/sql_parser.o: ../include/sql_util.h
$(DEBUGTARGETDIR)/sql_parser.o: ../include/sql_packet_format.h
$(DEBUGTARGETDIR)/sql_parser.o: ../include/sql_main.h
$(DEBUGTARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_dbi.h
$(DEBUGTARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_schema_cache.h
$(DEBUGTARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_er.h
$(DEBUGTARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_charset.h
$(DEBUGTARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_PMEM.h
$(DEBUGTARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_Server.h
$(DEBUGTARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_ContCat.h
$(DEBUGTARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_IndexCat.h
$(DEBUGTARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_LogMgr.h
$(DEBUGTARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_LogAnchor.h
$(DEBUGTARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_Recovery.h
$(DEBUGTARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_LogBuffer.h
$(DEBUGTARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_LogFile.h
$(DEBUGTARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_LogRec.h
$(DEBUGTARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_Mem_Mgr.h
$(DEBUGTARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_MemBase.h
$(DEBUGTARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_Segment.h
$(DEBUGTARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_DBFile.h
$(DEBUGTARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_syslog.h
$(DEBUGTARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_Thread.h
$(DEBUGTARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_comm_stub.h
$(DEBUGTARGETDIR)/sql_parser.o: ../../../src/server/include/mdb_memmgr.h
$(DEBUGTARGETDIR)/sql_parser.o: sql_mclient.h

$(DEBUGTARGETDIR)/sql_norm.o: sql_norm.c
$(DEBUGTARGETDIR)/sql_norm.o: ../../../src/include/mdb_config.h
$(DEBUGTARGETDIR)/sql_norm.o: ../../../src/include/sys_inc.h
$(DEBUGTARGETDIR)/sql_norm.o: ../../../src/server/include/mdb_compat.h
$(DEBUGTARGETDIR)/sql_norm.o: ../../../src/include/mdb_typedef.h
$(DEBUGTARGETDIR)/sql_norm.o: ../../../src/include/dbport.h
$(DEBUGTARGETDIR)/sql_norm.o: ../include/sql_norm.h ../include/sql_datast.h
$(DEBUGTARGETDIR)/sql_norm.o: ../../../src/include/systable.h
$(DEBUGTARGETDIR)/sql_norm.o: ../../../src/include/mdb_define.h
$(DEBUGTARGETDIR)/sql_norm.o: ../../../src/include/ErrorCode.h
$(DEBUGTARGETDIR)/sql_norm.o: ../../../src/server/include/mdb_FieldDesc.h
$(DEBUGTARGETDIR)/sql_norm.o: ../../../src/server/include/mdb_FieldVal.h
$(DEBUGTARGETDIR)/sql_norm.o: ../../../src/server/include/mdb_KeyDesc.h
$(DEBUGTARGETDIR)/sql_norm.o: ../../../src/server/include/mdb_Csr.h
$(DEBUGTARGETDIR)/sql_norm.o: ../../../src/server/include/mdb_ContIter.h
$(DEBUGTARGETDIR)/sql_norm.o: ../../../src/server/include/mdb_Iterator.h
$(DEBUGTARGETDIR)/sql_norm.o: ../../../src/server/include/mdb_Cont.h
$(DEBUGTARGETDIR)/sql_norm.o: ../../../src/server/include/mdb_BaseCont.h
$(DEBUGTARGETDIR)/sql_norm.o: ../../../src/server/include/mdb_inner_define.h
$(DEBUGTARGETDIR)/sql_norm.o: ../../../src/server/include/mdb_Collect.h
$(DEBUGTARGETDIR)/sql_norm.o: ../../../src/server/include/mdb_PageList.h
$(DEBUGTARGETDIR)/sql_norm.o: ../../../src/server/include/mdb_Page.h
$(DEBUGTARGETDIR)/sql_norm.o: ../../../src/server/include/mdb_TTree.h
$(DEBUGTARGETDIR)/sql_norm.o: ../../../src/server/include/mdb_TTreeNode.h
$(DEBUGTARGETDIR)/sql_norm.o: ../../../src/server/include/mdb_KeyValue.h
$(DEBUGTARGETDIR)/sql_norm.o: ../../../src/server/include/mdb_LockMgrPI.h
$(DEBUGTARGETDIR)/sql_norm.o: ../../../src/server/include/mdb_ppthread.h
$(DEBUGTARGETDIR)/sql_norm.o: ../../../src/server/include/mdb_Trans.h
$(DEBUGTARGETDIR)/sql_norm.o: ../../../src/server/include/mdb_LSN.h
$(DEBUGTARGETDIR)/sql_norm.o: ../../../src/server/include/mdb_Filter.h
$(DEBUGTARGETDIR)/sql_norm.o: ../../../src/server/include/mdb_TTreeIter.h
$(DEBUGTARGETDIR)/sql_norm.o: ../include/sql_const.h
$(DEBUGTARGETDIR)/sql_norm.o: ../../../src/include/isql.h
$(DEBUGTARGETDIR)/sql_norm.o: ../../../src/include/db_api.h
$(DEBUGTARGETDIR)/sql_norm.o: ../include/sql_util.h
$(DEBUGTARGETDIR)/sql_norm.o: ../include/sql_packet_format.h
$(DEBUGTARGETDIR)/sql_norm.o: ../include/sql_main.h
$(DEBUGTARGETDIR)/sql_norm.o: ../../../src/server/include/mdb_dbi.h
$(DEBUGTARGETDIR)/sql_norm.o: ../../../src/server/include/mdb_schema_cache.h
$(DEBUGTARGETDIR)/sql_norm.o: ../../../src/server/include/mdb_er.h
$(DEBUGTARGETDIR)/sql_norm.o: ../../../src/server/include/mdb_charset.h
$(DEBUGTARGETDIR)/sql_norm.o: ../../../src/server/include/mdb_PMEM.h
$(DEBUGTARGETDIR)/sql_norm.o: ../include/sql_parser.h sql_mclient.h

$(DEBUGTARGETDIR)/sql_plan.o: sql_plan.c
$(DEBUGTARGETDIR)/sql_plan.o: ../../../src/include/mdb_config.h
$(DEBUGTARGETDIR)/sql_plan.o: ../../../src/include/sys_inc.h
$(DEBUGTARGETDIR)/sql_plan.o: ../../../src/server/include/mdb_compat.h
$(DEBUGTARGETDIR)/sql_plan.o: ../../../src/include/mdb_typedef.h
$(DEBUGTARGETDIR)/sql_plan.o: ../../../src/include/dbport.h
$(DEBUGTARGETDIR)/sql_plan.o: ../include/sql_plan.h ../include/sql_datast.h
$(DEBUGTARGETDIR)/sql_plan.o: ../../../src/include/systable.h
$(DEBUGTARGETDIR)/sql_plan.o: ../../../src/include/mdb_define.h
$(DEBUGTARGETDIR)/sql_plan.o: ../../../src/include/ErrorCode.h
$(DEBUGTARGETDIR)/sql_plan.o: ../../../src/server/include/mdb_FieldDesc.h
$(DEBUGTARGETDIR)/sql_plan.o: ../../../src/server/include/mdb_FieldVal.h
$(DEBUGTARGETDIR)/sql_plan.o: ../../../src/server/include/mdb_KeyDesc.h
$(DEBUGTARGETDIR)/sql_plan.o: ../../../src/server/include/mdb_Csr.h
$(DEBUGTARGETDIR)/sql_plan.o: ../../../src/server/include/mdb_ContIter.h
$(DEBUGTARGETDIR)/sql_plan.o: ../../../src/server/include/mdb_Iterator.h
$(DEBUGTARGETDIR)/sql_plan.o: ../../../src/server/include/mdb_Cont.h
$(DEBUGTARGETDIR)/sql_plan.o: ../../../src/server/include/mdb_BaseCont.h
$(DEBUGTARGETDIR)/sql_plan.o: ../../../src/server/include/mdb_inner_define.h
$(DEBUGTARGETDIR)/sql_plan.o: ../../../src/server/include/mdb_Collect.h
$(DEBUGTARGETDIR)/sql_plan.o: ../../../src/server/include/mdb_PageList.h
$(DEBUGTARGETDIR)/sql_plan.o: ../../../src/server/include/mdb_Page.h
$(DEBUGTARGETDIR)/sql_plan.o: ../../../src/server/include/mdb_TTree.h
$(DEBUGTARGETDIR)/sql_plan.o: ../../../src/server/include/mdb_TTreeNode.h
$(DEBUGTARGETDIR)/sql_plan.o: ../../../src/server/include/mdb_KeyValue.h
$(DEBUGTARGETDIR)/sql_plan.o: ../../../src/server/include/mdb_LockMgrPI.h
$(DEBUGTARGETDIR)/sql_plan.o: ../../../src/server/include/mdb_ppthread.h
$(DEBUGTARGETDIR)/sql_plan.o: ../../../src/server/include/mdb_Trans.h
$(DEBUGTARGETDIR)/sql_plan.o: ../../../src/server/include/mdb_LSN.h
$(DEBUGTARGETDIR)/sql_plan.o: ../../../src/server/include/mdb_Filter.h
$(DEBUGTARGETDIR)/sql_plan.o: ../../../src/server/include/mdb_TTreeIter.h
$(DEBUGTARGETDIR)/sql_plan.o: ../include/sql_const.h
$(DEBUGTARGETDIR)/sql_plan.o: ../../../src/include/isql.h
$(DEBUGTARGETDIR)/sql_plan.o: ../../../src/include/db_api.h
$(DEBUGTARGETDIR)/sql_plan.o: ../include/sql_execute.h ../include/sql_main.h
$(DEBUGTARGETDIR)/sql_plan.o: ../../../src/server/include/mdb_dbi.h
$(DEBUGTARGETDIR)/sql_plan.o: ../../../src/server/include/mdb_schema_cache.h
$(DEBUGTARGETDIR)/sql_plan.o: ../include/sql_util.h
$(DEBUGTARGETDIR)/sql_plan.o: ../include/sql_packet_format.h
$(DEBUGTARGETDIR)/sql_plan.o: ../../../src/server/include/mdb_PMEM.h
$(DEBUGTARGETDIR)/sql_plan.o: ../../../src/server/include/mdb_er.h
$(DEBUGTARGETDIR)/sql_plan.o: ../../../src/server/include/mdb_charset.h
$(DEBUGTARGETDIR)/sql_plan.o: ../include/sql_parser.h sql_mclient.h

$(DEBUGTARGETDIR)/sql_execute.o: sql_execute.c
$(DEBUGTARGETDIR)/sql_execute.o: ../../../src/include/mdb_config.h
$(DEBUGTARGETDIR)/sql_execute.o: ../../../src/include/sys_inc.h
$(DEBUGTARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_compat.h
$(DEBUGTARGETDIR)/sql_execute.o: ../../../src/include/mdb_typedef.h
$(DEBUGTARGETDIR)/sql_execute.o: ../../../src/include/dbport.h
$(DEBUGTARGETDIR)/sql_execute.o: ../include/sql_main.h
$(DEBUGTARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_dbi.h
$(DEBUGTARGETDIR)/sql_execute.o: ../../../src/include/systable.h
$(DEBUGTARGETDIR)/sql_execute.o: ../../../src/include/mdb_define.h
$(DEBUGTARGETDIR)/sql_execute.o: ../../../src/include/ErrorCode.h
$(DEBUGTARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_FieldDesc.h
$(DEBUGTARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_FieldVal.h
$(DEBUGTARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_KeyDesc.h
$(DEBUGTARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_inner_define.h
$(DEBUGTARGETDIR)/sql_execute.o: ../../../src/include/db_api.h
$(DEBUGTARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_KeyValue.h
$(DEBUGTARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_Filter.h
$(DEBUGTARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_schema_cache.h
$(DEBUGTARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_Trans.h
$(DEBUGTARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_LSN.h
$(DEBUGTARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_LockMgrPI.h
$(DEBUGTARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_ppthread.h
$(DEBUGTARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_Csr.h
$(DEBUGTARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_ContIter.h
$(DEBUGTARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_Iterator.h
$(DEBUGTARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_Cont.h
$(DEBUGTARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_BaseCont.h
$(DEBUGTARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_Collect.h
$(DEBUGTARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_PageList.h
$(DEBUGTARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_Page.h
$(DEBUGTARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_TTree.h
$(DEBUGTARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_TTreeNode.h
$(DEBUGTARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_TTreeIter.h
$(DEBUGTARGETDIR)/sql_execute.o: ../../../src/include/isql.h
$(DEBUGTARGETDIR)/sql_execute.o: ../include/sql_const.h
$(DEBUGTARGETDIR)/sql_execute.o: ../include/sql_datast.h
$(DEBUGTARGETDIR)/sql_execute.o: ../include/sql_util.h
$(DEBUGTARGETDIR)/sql_execute.o: ../include/sql_packet_format.h
$(DEBUGTARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_PMEM.h
$(DEBUGTARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_er.h
$(DEBUGTARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_charset.h
$(DEBUGTARGETDIR)/sql_execute.o: ../include/sql_execute.h sql_decimal.h
$(DEBUGTARGETDIR)/sql_execute.o: sql_func_timeutil.h
$(DEBUGTARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_comm_stub.h
$(DEBUGTARGETDIR)/sql_execute.o: ../include/sql_parser.h sql_mclient.h
$(DEBUGTARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_Server.h
$(DEBUGTARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_ContCat.h
$(DEBUGTARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_IndexCat.h
$(DEBUGTARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_LogMgr.h
$(DEBUGTARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_LogAnchor.h
$(DEBUGTARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_Recovery.h
$(DEBUGTARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_LogBuffer.h
$(DEBUGTARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_LogFile.h
$(DEBUGTARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_LogRec.h
$(DEBUGTARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_Mem_Mgr.h
$(DEBUGTARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_MemBase.h
$(DEBUGTARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_Segment.h
$(DEBUGTARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_DBFile.h
$(DEBUGTARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_syslog.h
$(DEBUGTARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_Thread.h
$(DEBUGTARGETDIR)/sql_execute.o: ../../../src/server/include/mdb_memmgr.h

$(DEBUGTARGETDIR)/sql_util.o: sql_util.c
$(DEBUGTARGETDIR)/sql_util.o: ../../../src/include/mdb_config.h
$(DEBUGTARGETDIR)/sql_util.o: ../../../src/include/sys_inc.h
$(DEBUGTARGETDIR)/sql_util.o: ../../../src/server/include/mdb_compat.h
$(DEBUGTARGETDIR)/sql_util.o: ../../../src/include/mdb_typedef.h
$(DEBUGTARGETDIR)/sql_util.o: ../../../src/include/dbport.h
$(DEBUGTARGETDIR)/sql_util.o: ../include/sql_main.h
$(DEBUGTARGETDIR)/sql_util.o: ../../../src/server/include/mdb_dbi.h
$(DEBUGTARGETDIR)/sql_util.o: ../../../src/include/systable.h
$(DEBUGTARGETDIR)/sql_util.o: ../../../src/include/mdb_define.h
$(DEBUGTARGETDIR)/sql_util.o: ../../../src/include/ErrorCode.h
$(DEBUGTARGETDIR)/sql_util.o: ../../../src/server/include/mdb_FieldDesc.h
$(DEBUGTARGETDIR)/sql_util.o: ../../../src/server/include/mdb_FieldVal.h
$(DEBUGTARGETDIR)/sql_util.o: ../../../src/server/include/mdb_KeyDesc.h
$(DEBUGTARGETDIR)/sql_util.o: ../../../src/server/include/mdb_inner_define.h
$(DEBUGTARGETDIR)/sql_util.o: ../../../src/include/db_api.h
$(DEBUGTARGETDIR)/sql_util.o: ../../../src/server/include/mdb_KeyValue.h
$(DEBUGTARGETDIR)/sql_util.o: ../../../src/server/include/mdb_Filter.h
$(DEBUGTARGETDIR)/sql_util.o: ../../../src/server/include/mdb_schema_cache.h
$(DEBUGTARGETDIR)/sql_util.o: ../../../src/server/include/mdb_Trans.h
$(DEBUGTARGETDIR)/sql_util.o: ../../../src/server/include/mdb_LSN.h
$(DEBUGTARGETDIR)/sql_util.o: ../../../src/server/include/mdb_LockMgrPI.h
$(DEBUGTARGETDIR)/sql_util.o: ../../../src/server/include/mdb_ppthread.h
$(DEBUGTARGETDIR)/sql_util.o: ../../../src/server/include/mdb_Csr.h
$(DEBUGTARGETDIR)/sql_util.o: ../../../src/server/include/mdb_ContIter.h
$(DEBUGTARGETDIR)/sql_util.o: ../../../src/server/include/mdb_Iterator.h
$(DEBUGTARGETDIR)/sql_util.o: ../../../src/server/include/mdb_Cont.h
$(DEBUGTARGETDIR)/sql_util.o: ../../../src/server/include/mdb_BaseCont.h
$(DEBUGTARGETDIR)/sql_util.o: ../../../src/server/include/mdb_Collect.h
$(DEBUGTARGETDIR)/sql_util.o: ../../../src/server/include/mdb_PageList.h
$(DEBUGTARGETDIR)/sql_util.o: ../../../src/server/include/mdb_Page.h
$(DEBUGTARGETDIR)/sql_util.o: ../../../src/server/include/mdb_TTree.h
$(DEBUGTARGETDIR)/sql_util.o: ../../../src/server/include/mdb_TTreeNode.h
$(DEBUGTARGETDIR)/sql_util.o: ../../../src/server/include/mdb_TTreeIter.h
$(DEBUGTARGETDIR)/sql_util.o: ../../../src/include/isql.h
$(DEBUGTARGETDIR)/sql_util.o: ../include/sql_const.h ../include/sql_datast.h
$(DEBUGTARGETDIR)/sql_util.o: ../include/sql_util.h
$(DEBUGTARGETDIR)/sql_util.o: ../include/sql_packet_format.h
$(DEBUGTARGETDIR)/sql_util.o: ../../../src/server/include/mdb_PMEM.h
$(DEBUGTARGETDIR)/sql_util.o: ../../../src/server/include/mdb_er.h
$(DEBUGTARGETDIR)/sql_util.o: ../../../src/server/include/mdb_charset.h sql_decimal.h
$(DEBUGTARGETDIR)/sql_util.o: sql_func_timeutil.h

$(DEBUGTARGETDIR)/sql_calc_expression.o: sql_calc_expression.c
$(DEBUGTARGETDIR)/sql_calc_expression.o: ../../../src/include/mdb_config.h
$(DEBUGTARGETDIR)/sql_calc_expression.o: ../../../src/server/include/mdb_compat.h
$(DEBUGTARGETDIR)/sql_calc_expression.o: ../../../src/include/sys_inc.h
$(DEBUGTARGETDIR)/sql_calc_expression.o: ../../../src/include/mdb_typedef.h
$(DEBUGTARGETDIR)/sql_calc_expression.o: ../../../src/include/dbport.h
$(DEBUGTARGETDIR)/sql_calc_expression.o: ../include/sql_main.h
$(DEBUGTARGETDIR)/sql_calc_expression.o: ../../../src/server/include/mdb_dbi.h
$(DEBUGTARGETDIR)/sql_calc_expression.o: ../../../src/include/systable.h
$(DEBUGTARGETDIR)/sql_calc_expression.o: ../../../src/include/mdb_define.h
$(DEBUGTARGETDIR)/sql_calc_expression.o: ../../../src/include/ErrorCode.h
$(DEBUGTARGETDIR)/sql_calc_expression.o: ../../../src/server/include/mdb_FieldDesc.h
$(DEBUGTARGETDIR)/sql_calc_expression.o: ../../../src/server/include/mdb_FieldVal.h
$(DEBUGTARGETDIR)/sql_calc_expression.o: ../../../src/server/include/mdb_KeyDesc.h
$(DEBUGTARGETDIR)/sql_calc_expression.o: ../../../src/server/include/mdb_inner_define.h
$(DEBUGTARGETDIR)/sql_calc_expression.o: ../../../src/include/db_api.h
$(DEBUGTARGETDIR)/sql_calc_expression.o: ../../../src/server/include/mdb_KeyValue.h
$(DEBUGTARGETDIR)/sql_calc_expression.o: ../../../src/server/include/mdb_Filter.h
$(DEBUGTARGETDIR)/sql_calc_expression.o: ../../../src/server/include/mdb_schema_cache.h
$(DEBUGTARGETDIR)/sql_calc_expression.o: ../../../src/server/include/mdb_Trans.h
$(DEBUGTARGETDIR)/sql_calc_expression.o: ../../../src/server/include/mdb_LSN.h
$(DEBUGTARGETDIR)/sql_calc_expression.o: ../../../src/server/include/mdb_LockMgrPI.h
$(DEBUGTARGETDIR)/sql_calc_expression.o: ../../../src/server/include/mdb_ppthread.h
$(DEBUGTARGETDIR)/sql_calc_expression.o: ../../../src/server/include/mdb_Csr.h
$(DEBUGTARGETDIR)/sql_calc_expression.o: ../../../src/server/include/mdb_ContIter.h
$(DEBUGTARGETDIR)/sql_calc_expression.o: ../../../src/server/include/mdb_Iterator.h
$(DEBUGTARGETDIR)/sql_calc_expression.o: ../../../src/server/include/mdb_Cont.h
$(DEBUGTARGETDIR)/sql_calc_expression.o: ../../../src/server/include/mdb_BaseCont.h
$(DEBUGTARGETDIR)/sql_calc_expression.o: ../../../src/server/include/mdb_Collect.h
$(DEBUGTARGETDIR)/sql_calc_expression.o: ../../../src/server/include/mdb_PageList.h
$(DEBUGTARGETDIR)/sql_calc_expression.o: ../../../src/server/include/mdb_Page.h
$(DEBUGTARGETDIR)/sql_calc_expression.o: ../../../src/server/include/mdb_TTree.h
$(DEBUGTARGETDIR)/sql_calc_expression.o: ../../../src/server/include/mdb_TTreeNode.h
$(DEBUGTARGETDIR)/sql_calc_expression.o: ../../../src/server/include/mdb_TTreeIter.h
$(DEBUGTARGETDIR)/sql_calc_expression.o: ../../../src/include/isql.h
$(DEBUGTARGETDIR)/sql_calc_expression.o: ../include/sql_const.h
$(DEBUGTARGETDIR)/sql_calc_expression.o: ../include/sql_datast.h
$(DEBUGTARGETDIR)/sql_calc_expression.o: ../include/sql_util.h
$(DEBUGTARGETDIR)/sql_calc_expression.o: ../include/sql_packet_format.h
$(DEBUGTARGETDIR)/sql_calc_expression.o: ../../../src/server/include/mdb_PMEM.h
$(DEBUGTARGETDIR)/sql_calc_expression.o: ../../../src/server/include/mdb_er.h
$(DEBUGTARGETDIR)/sql_calc_expression.o: ../../../src/server/include/mdb_charset.h
$(DEBUGTARGETDIR)/sql_calc_expression.o: sql_func_timeutil.h sql_mclient.h
$(DEBUGTARGETDIR)/sql_calc_expression.o: ../include/sql_parser.h

$(DEBUGTARGETDIR)/sql_calc_function.o: sql_calc_function.c
$(DEBUGTARGETDIR)/sql_calc_function.o: ../../../src/include/mdb_config.h
$(DEBUGTARGETDIR)/sql_calc_function.o: ../../../src/include/sys_inc.h
$(DEBUGTARGETDIR)/sql_calc_function.o: ../../../src/server/include/mdb_compat.h
$(DEBUGTARGETDIR)/sql_calc_function.o: ../../../src/include/mdb_typedef.h
$(DEBUGTARGETDIR)/sql_calc_function.o: ../../../src/include/dbport.h
$(DEBUGTARGETDIR)/sql_calc_function.o: ../../../src/include/isql.h
$(DEBUGTARGETDIR)/sql_calc_function.o: ../../../src/include/mdb_define.h
$(DEBUGTARGETDIR)/sql_calc_function.o: ../../../src/include/ErrorCode.h
$(DEBUGTARGETDIR)/sql_calc_function.o: ../../../src/server/include/mdb_FieldDesc.h
$(DEBUGTARGETDIR)/sql_calc_function.o: ../../../src/server/include/mdb_FieldVal.h
$(DEBUGTARGETDIR)/sql_calc_function.o: ../../../src/include/db_api.h
$(DEBUGTARGETDIR)/sql_calc_function.o: ../../../src/server/include/mdb_KeyValue.h
$(DEBUGTARGETDIR)/sql_calc_function.o: ../../../src/server/include/mdb_KeyDesc.h
$(DEBUGTARGETDIR)/sql_calc_function.o: ../../../src/server/include/mdb_Filter.h
$(DEBUGTARGETDIR)/sql_calc_function.o: ../../../src/include/systable.h
$(DEBUGTARGETDIR)/sql_calc_function.o: ../include/sql_datast.h
$(DEBUGTARGETDIR)/sql_calc_function.o: ../../../src/server/include/mdb_Csr.h
$(DEBUGTARGETDIR)/sql_calc_function.o: ../../../src/server/include/mdb_ContIter.h
$(DEBUGTARGETDIR)/sql_calc_function.o: ../../../src/server/include/mdb_Iterator.h
$(DEBUGTARGETDIR)/sql_calc_function.o: ../../../src/server/include/mdb_Cont.h
$(DEBUGTARGETDIR)/sql_calc_function.o: ../../../src/server/include/mdb_BaseCont.h
$(DEBUGTARGETDIR)/sql_calc_function.o: ../../../src/server/include/mdb_inner_define.h
$(DEBUGTARGETDIR)/sql_calc_function.o: ../../../src/server/include/mdb_Collect.h
$(DEBUGTARGETDIR)/sql_calc_function.o: ../../../src/server/include/mdb_PageList.h
$(DEBUGTARGETDIR)/sql_calc_function.o: ../../../src/server/include/mdb_Page.h
$(DEBUGTARGETDIR)/sql_calc_function.o: ../../../src/server/include/mdb_TTree.h
$(DEBUGTARGETDIR)/sql_calc_function.o: ../../../src/server/include/mdb_TTreeNode.h
$(DEBUGTARGETDIR)/sql_calc_function.o: ../../../src/server/include/mdb_LockMgrPI.h
$(DEBUGTARGETDIR)/sql_calc_function.o: ../../../src/server/include/mdb_ppthread.h
$(DEBUGTARGETDIR)/sql_calc_function.o: ../../../src/server/include/mdb_Trans.h
$(DEBUGTARGETDIR)/sql_calc_function.o: ../../../src/server/include/mdb_LSN.h
$(DEBUGTARGETDIR)/sql_calc_function.o: ../../../src/server/include/mdb_TTreeIter.h
$(DEBUGTARGETDIR)/sql_calc_function.o: ../include/sql_const.h
$(DEBUGTARGETDIR)/sql_calc_function.o: ../include/sql_util.h
$(DEBUGTARGETDIR)/sql_calc_function.o: ../include/sql_packet_format.h
$(DEBUGTARGETDIR)/sql_calc_function.o: ../include/sql_main.h
$(DEBUGTARGETDIR)/sql_calc_function.o: ../../../src/server/include/mdb_dbi.h
$(DEBUGTARGETDIR)/sql_calc_function.o: ../../../src/server/include/mdb_schema_cache.h
$(DEBUGTARGETDIR)/sql_calc_function.o: ../../../src/server/include/mdb_er.h
$(DEBUGTARGETDIR)/sql_calc_function.o: ../../../src/server/include/mdb_charset.h
$(DEBUGTARGETDIR)/sql_calc_function.o: ../../../src/server/include/mdb_PMEM.h
$(DEBUGTARGETDIR)/sql_calc_function.o: sql_calc_timefunc.h sql_func_timeutil.h

$(DEBUGTARGETDIR)/sql_calc_timefunc.o: sql_calc_timefunc.c
$(DEBUGTARGETDIR)/sql_calc_timefunc.o: ../../../src/include/mdb_config.h
$(DEBUGTARGETDIR)/sql_calc_timefunc.o: ../../../src/include/sys_inc.h
$(DEBUGTARGETDIR)/sql_calc_timefunc.o: ../../../src/server/include/mdb_compat.h
$(DEBUGTARGETDIR)/sql_calc_timefunc.o: ../../../src/include/mdb_typedef.h
$(DEBUGTARGETDIR)/sql_calc_timefunc.o: ../../../src/include/dbport.h
$(DEBUGTARGETDIR)/sql_calc_timefunc.o: ../../../src/include/isql.h
$(DEBUGTARGETDIR)/sql_calc_timefunc.o: ../../../src/include/mdb_define.h
$(DEBUGTARGETDIR)/sql_calc_timefunc.o: ../../../src/include/ErrorCode.h
$(DEBUGTARGETDIR)/sql_calc_timefunc.o: ../../../src/server/include/mdb_FieldDesc.h
$(DEBUGTARGETDIR)/sql_calc_timefunc.o: ../../../src/server/include/mdb_FieldVal.h
$(DEBUGTARGETDIR)/sql_calc_timefunc.o: ../../../src/include/db_api.h
$(DEBUGTARGETDIR)/sql_calc_timefunc.o: ../../../src/server/include/mdb_KeyValue.h
$(DEBUGTARGETDIR)/sql_calc_timefunc.o: ../../../src/server/include/mdb_KeyDesc.h
$(DEBUGTARGETDIR)/sql_calc_timefunc.o: ../../../src/server/include/mdb_Filter.h
$(DEBUGTARGETDIR)/sql_calc_timefunc.o: ../../../src/include/systable.h
$(DEBUGTARGETDIR)/sql_calc_timefunc.o: ../include/sql_datast.h
$(DEBUGTARGETDIR)/sql_calc_timefunc.o: ../../../src/server/include/mdb_Csr.h
$(DEBUGTARGETDIR)/sql_calc_timefunc.o: ../../../src/server/include/mdb_ContIter.h
$(DEBUGTARGETDIR)/sql_calc_timefunc.o: ../../../src/server/include/mdb_Iterator.h
$(DEBUGTARGETDIR)/sql_calc_timefunc.o: ../../../src/server/include/mdb_Cont.h
$(DEBUGTARGETDIR)/sql_calc_timefunc.o: ../../../src/server/include/mdb_BaseCont.h
$(DEBUGTARGETDIR)/sql_calc_timefunc.o: ../../../src/server/include/mdb_inner_define.h
$(DEBUGTARGETDIR)/sql_calc_timefunc.o: ../../../src/server/include/mdb_Collect.h
$(DEBUGTARGETDIR)/sql_calc_timefunc.o: ../../../src/server/include/mdb_PageList.h
$(DEBUGTARGETDIR)/sql_calc_timefunc.o: ../../../src/server/include/mdb_Page.h
$(DEBUGTARGETDIR)/sql_calc_timefunc.o: ../../../src/server/include/mdb_TTree.h
$(DEBUGTARGETDIR)/sql_calc_timefunc.o: ../../../src/server/include/mdb_TTreeNode.h
$(DEBUGTARGETDIR)/sql_calc_timefunc.o: ../../../src/server/include/mdb_LockMgrPI.h
$(DEBUGTARGETDIR)/sql_calc_timefunc.o: ../../../src/server/include/mdb_ppthread.h
$(DEBUGTARGETDIR)/sql_calc_timefunc.o: ../../../src/server/include/mdb_Trans.h
$(DEBUGTARGETDIR)/sql_calc_timefunc.o: ../../../src/server/include/mdb_LSN.h
$(DEBUGTARGETDIR)/sql_calc_timefunc.o: ../../../src/server/include/mdb_TTreeIter.h
$(DEBUGTARGETDIR)/sql_calc_timefunc.o: ../include/sql_const.h
$(DEBUGTARGETDIR)/sql_calc_timefunc.o: ../include/sql_util.h
$(DEBUGTARGETDIR)/sql_calc_timefunc.o: ../include/sql_packet_format.h
$(DEBUGTARGETDIR)/sql_calc_timefunc.o: ../include/sql_main.h
$(DEBUGTARGETDIR)/sql_calc_timefunc.o: ../../../src/server/include/mdb_dbi.h
$(DEBUGTARGETDIR)/sql_calc_timefunc.o: ../../../src/server/include/mdb_schema_cache.h
$(DEBUGTARGETDIR)/sql_calc_timefunc.o: ../../../src/server/include/mdb_er.h
$(DEBUGTARGETDIR)/sql_calc_timefunc.o: ../../../src/server/include/mdb_charset.h
$(DEBUGTARGETDIR)/sql_calc_timefunc.o: ../../../src/server/include/mdb_PMEM.h
$(DEBUGTARGETDIR)/sql_calc_timefunc.o: sql_func_timeutil.h

$(DEBUGTARGETDIR)/sql_func_timeutil.o: sql_func_timeutil.c
$(DEBUGTARGETDIR)/sql_func_timeutil.o: ../../../src/include/mdb_config.h
$(DEBUGTARGETDIR)/sql_func_timeutil.o: ../../../src/include/sys_inc.h
$(DEBUGTARGETDIR)/sql_func_timeutil.o: ../../../src/server/include/mdb_compat.h
$(DEBUGTARGETDIR)/sql_func_timeutil.o: ../../../src/include/mdb_typedef.h
$(DEBUGTARGETDIR)/sql_func_timeutil.o: ../../../src/include/dbport.h
$(DEBUGTARGETDIR)/sql_func_timeutil.o: ../include/sql_datast.h
$(DEBUGTARGETDIR)/sql_func_timeutil.o: ../../../src/include/systable.h
$(DEBUGTARGETDIR)/sql_func_timeutil.o: ../../../src/include/mdb_define.h
$(DEBUGTARGETDIR)/sql_func_timeutil.o: ../../../src/include/ErrorCode.h
$(DEBUGTARGETDIR)/sql_func_timeutil.o: ../../../src/server/include/mdb_FieldDesc.h
$(DEBUGTARGETDIR)/sql_func_timeutil.o: ../../../src/server/include/mdb_FieldVal.h
$(DEBUGTARGETDIR)/sql_func_timeutil.o: ../../../src/server/include/mdb_KeyDesc.h
$(DEBUGTARGETDIR)/sql_func_timeutil.o: ../../../src/server/include/mdb_Csr.h
$(DEBUGTARGETDIR)/sql_func_timeutil.o: ../../../src/server/include/mdb_ContIter.h
$(DEBUGTARGETDIR)/sql_func_timeutil.o: ../../../src/server/include/mdb_Iterator.h
$(DEBUGTARGETDIR)/sql_func_timeutil.o: ../../../src/server/include/mdb_Cont.h
$(DEBUGTARGETDIR)/sql_func_timeutil.o: ../../../src/server/include/mdb_BaseCont.h
$(DEBUGTARGETDIR)/sql_func_timeutil.o: ../../../src/server/include/mdb_inner_define.h
$(DEBUGTARGETDIR)/sql_func_timeutil.o: ../../../src/server/include/mdb_Collect.h
$(DEBUGTARGETDIR)/sql_func_timeutil.o: ../../../src/server/include/mdb_PageList.h
$(DEBUGTARGETDIR)/sql_func_timeutil.o: ../../../src/server/include/mdb_Page.h
$(DEBUGTARGETDIR)/sql_func_timeutil.o: ../../../src/server/include/mdb_TTree.h
$(DEBUGTARGETDIR)/sql_func_timeutil.o: ../../../src/server/include/mdb_TTreeNode.h
$(DEBUGTARGETDIR)/sql_func_timeutil.o: ../../../src/server/include/mdb_KeyValue.h
$(DEBUGTARGETDIR)/sql_func_timeutil.o: ../../../src/server/include/mdb_LockMgrPI.h
$(DEBUGTARGETDIR)/sql_func_timeutil.o: ../../../src/server/include/mdb_ppthread.h
$(DEBUGTARGETDIR)/sql_func_timeutil.o: ../../../src/server/include/mdb_Trans.h
$(DEBUGTARGETDIR)/sql_func_timeutil.o: ../../../src/server/include/mdb_LSN.h
$(DEBUGTARGETDIR)/sql_func_timeutil.o: ../../../src/server/include/mdb_Filter.h
$(DEBUGTARGETDIR)/sql_func_timeutil.o: ../../../src/server/include/mdb_TTreeIter.h
$(DEBUGTARGETDIR)/sql_func_timeutil.o: ../include/sql_const.h
$(DEBUGTARGETDIR)/sql_func_timeutil.o: ../../../src/include/isql.h
$(DEBUGTARGETDIR)/sql_func_timeutil.o: ../../../src/include/db_api.h
$(DEBUGTARGETDIR)/sql_func_timeutil.o: ../include/sql_util.h
$(DEBUGTARGETDIR)/sql_func_timeutil.o: ../include/sql_packet_format.h
$(DEBUGTARGETDIR)/sql_func_timeutil.o: ../include/sql_main.h
$(DEBUGTARGETDIR)/sql_func_timeutil.o: ../../../src/server/include/mdb_dbi.h
$(DEBUGTARGETDIR)/sql_func_timeutil.o: ../../../src/server/include/mdb_schema_cache.h
$(DEBUGTARGETDIR)/sql_func_timeutil.o: ../../../src/server/include/mdb_er.h
$(DEBUGTARGETDIR)/sql_func_timeutil.o: ../../../src/server/include/mdb_charset.h
$(DEBUGTARGETDIR)/sql_func_timeutil.o: ../../../src/server/include/mdb_PMEM.h
$(DEBUGTARGETDIR)/sql_func_timeutil.o: sql_func_timeutil.h

$(DEBUGTARGETDIR)/sql_worktable.o: sql_worktable.c
$(DEBUGTARGETDIR)/sql_worktable.o: ../../../src/include/mdb_config.h
$(DEBUGTARGETDIR)/sql_worktable.o: ../../../src/include/sys_inc.h
$(DEBUGTARGETDIR)/sql_worktable.o: ../include/sql_main.h
$(DEBUGTARGETDIR)/sql_worktable.o: ../../../src/server/include/mdb_dbi.h
$(DEBUGTARGETDIR)/sql_worktable.o: ../../../src/include/systable.h
$(DEBUGTARGETDIR)/sql_worktable.o: ../../../src/include/mdb_define.h
$(DEBUGTARGETDIR)/sql_worktable.o: ../../../src/include/ErrorCode.h
$(DEBUGTARGETDIR)/sql_worktable.o: ../../../src/include/mdb_typedef.h
$(DEBUGTARGETDIR)/sql_worktable.o: ../../../src/include/dbport.h
$(DEBUGTARGETDIR)/sql_worktable.o: ../../../src/server/include/mdb_FieldDesc.h
$(DEBUGTARGETDIR)/sql_worktable.o: ../../../src/server/include/mdb_FieldVal.h
$(DEBUGTARGETDIR)/sql_worktable.o: ../../../src/server/include/mdb_KeyDesc.h
$(DEBUGTARGETDIR)/sql_worktable.o: ../../../src/server/include/mdb_inner_define.h
$(DEBUGTARGETDIR)/sql_worktable.o: ../../../src/include/db_api.h
$(DEBUGTARGETDIR)/sql_worktable.o: ../../../src/server/include/mdb_KeyValue.h
$(DEBUGTARGETDIR)/sql_worktable.o: ../../../src/server/include/mdb_Filter.h
$(DEBUGTARGETDIR)/sql_worktable.o: ../../../src/server/include/mdb_schema_cache.h
$(DEBUGTARGETDIR)/sql_worktable.o: ../../../src/server/include/mdb_Trans.h
$(DEBUGTARGETDIR)/sql_worktable.o: ../../../src/server/include/mdb_LSN.h
$(DEBUGTARGETDIR)/sql_worktable.o: ../../../src/server/include/mdb_LockMgrPI.h
$(DEBUGTARGETDIR)/sql_worktable.o: ../../../src/server/include/mdb_ppthread.h
$(DEBUGTARGETDIR)/sql_worktable.o: ../../../src/server/include/mdb_compat.h
$(DEBUGTARGETDIR)/sql_worktable.o: ../../../src/server/include/mdb_Csr.h
$(DEBUGTARGETDIR)/sql_worktable.o: ../../../src/server/include/mdb_ContIter.h
$(DEBUGTARGETDIR)/sql_worktable.o: ../../../src/server/include/mdb_Iterator.h
$(DEBUGTARGETDIR)/sql_worktable.o: ../../../src/server/include/mdb_Cont.h
$(DEBUGTARGETDIR)/sql_worktable.o: ../../../src/server/include/mdb_BaseCont.h
$(DEBUGTARGETDIR)/sql_worktable.o: ../../../src/server/include/mdb_Collect.h
$(DEBUGTARGETDIR)/sql_worktable.o: ../../../src/server/include/mdb_PageList.h
$(DEBUGTARGETDIR)/sql_worktable.o: ../../../src/server/include/mdb_Page.h
$(DEBUGTARGETDIR)/sql_worktable.o: ../../../src/server/include/mdb_TTree.h
$(DEBUGTARGETDIR)/sql_worktable.o: ../../../src/server/include/mdb_TTreeNode.h
$(DEBUGTARGETDIR)/sql_worktable.o: ../../../src/server/include/mdb_TTreeIter.h
$(DEBUGTARGETDIR)/sql_worktable.o: ../../../src/include/isql.h
$(DEBUGTARGETDIR)/sql_worktable.o: ../include/sql_const.h
$(DEBUGTARGETDIR)/sql_worktable.o: ../include/sql_datast.h ../include/sql_util.h
$(DEBUGTARGETDIR)/sql_worktable.o: ../include/sql_packet_format.h
$(DEBUGTARGETDIR)/sql_worktable.o: ../../../src/server/include/mdb_PMEM.h
$(DEBUGTARGETDIR)/sql_worktable.o: ../../../src/server/include/mdb_er.h
$(DEBUGTARGETDIR)/sql_worktable.o: ../../../src/server/include/mdb_charset.h
$(DEBUGTARGETDIR)/sql_worktable.o: sql_decimal.h ../include/sql_execute.h

$(DEBUGTARGETDIR)/sql_drop.o: sql_drop.c
$(DEBUGTARGETDIR)/sql_drop.o: ../include/sql_main.h
$(DEBUGTARGETDIR)/sql_drop.o: ../../../src/include/mdb_config.h
$(DEBUGTARGETDIR)/sql_drop.o: ../../../src/server/include/mdb_dbi.h
$(DEBUGTARGETDIR)/sql_drop.o: ../../../src/include/sys_inc.h
$(DEBUGTARGETDIR)/sql_drop.o: ../../../src/include/systable.h
$(DEBUGTARGETDIR)/sql_drop.o: ../../../src/include/mdb_define.h
$(DEBUGTARGETDIR)/sql_drop.o: ../../../src/include/ErrorCode.h
$(DEBUGTARGETDIR)/sql_drop.o: ../../../src/include/mdb_typedef.h
$(DEBUGTARGETDIR)/sql_drop.o: ../../../src/include/dbport.h
$(DEBUGTARGETDIR)/sql_drop.o: ../../../src/server/include/mdb_FieldDesc.h
$(DEBUGTARGETDIR)/sql_drop.o: ../../../src/server/include/mdb_FieldVal.h
$(DEBUGTARGETDIR)/sql_drop.o: ../../../src/server/include/mdb_KeyDesc.h
$(DEBUGTARGETDIR)/sql_drop.o: ../../../src/server/include/mdb_inner_define.h
$(DEBUGTARGETDIR)/sql_drop.o: ../../../src/include/db_api.h
$(DEBUGTARGETDIR)/sql_drop.o: ../../../src/server/include/mdb_KeyValue.h
$(DEBUGTARGETDIR)/sql_drop.o: ../../../src/server/include/mdb_Filter.h
$(DEBUGTARGETDIR)/sql_drop.o: ../../../src/server/include/mdb_schema_cache.h
$(DEBUGTARGETDIR)/sql_drop.o: ../../../src/server/include/mdb_Trans.h
$(DEBUGTARGETDIR)/sql_drop.o: ../../../src/server/include/mdb_LSN.h
$(DEBUGTARGETDIR)/sql_drop.o: ../../../src/server/include/mdb_LockMgrPI.h
$(DEBUGTARGETDIR)/sql_drop.o: ../../../src/server/include/mdb_ppthread.h
$(DEBUGTARGETDIR)/sql_drop.o: ../../../src/server/include/mdb_compat.h
$(DEBUGTARGETDIR)/sql_drop.o: ../../../src/server/include/mdb_Csr.h
$(DEBUGTARGETDIR)/sql_drop.o: ../../../src/server/include/mdb_ContIter.h
$(DEBUGTARGETDIR)/sql_drop.o: ../../../src/server/include/mdb_Iterator.h
$(DEBUGTARGETDIR)/sql_drop.o: ../../../src/server/include/mdb_Cont.h
$(DEBUGTARGETDIR)/sql_drop.o: ../../../src/server/include/mdb_BaseCont.h
$(DEBUGTARGETDIR)/sql_drop.o: ../../../src/server/include/mdb_Collect.h
$(DEBUGTARGETDIR)/sql_drop.o: ../../../src/server/include/mdb_PageList.h
$(DEBUGTARGETDIR)/sql_drop.o: ../../../src/server/include/mdb_Page.h
$(DEBUGTARGETDIR)/sql_drop.o: ../../../src/server/include/mdb_TTree.h
$(DEBUGTARGETDIR)/sql_drop.o: ../../../src/server/include/mdb_TTreeNode.h
$(DEBUGTARGETDIR)/sql_drop.o: ../../../src/server/include/mdb_TTreeIter.h
$(DEBUGTARGETDIR)/sql_drop.o: ../../../src/include/isql.h
$(DEBUGTARGETDIR)/sql_drop.o: ../include/sql_const.h ../include/sql_datast.h
$(DEBUGTARGETDIR)/sql_drop.o: ../include/sql_util.h
$(DEBUGTARGETDIR)/sql_drop.o: ../include/sql_packet_format.h
$(DEBUGTARGETDIR)/sql_drop.o: ../../../src/server/include/mdb_PMEM.h
$(DEBUGTARGETDIR)/sql_drop.o: ../../../src/server/include/mdb_er.h
$(DEBUGTARGETDIR)/sql_drop.o: ../../../src/server/include/mdb_charset.h sql_decimal.h
$(DEBUGTARGETDIR)/sql_drop.o: ../include/sql_parser.h
$(DEBUGTARGETDIR)/sql_drop.o: sql_mclient.h

$(DEBUGTARGETDIR)/sql_create.o: sql_create.c
$(DEBUGTARGETDIR)/sql_create.o: ../../../src/include/mdb_config.h
$(DEBUGTARGETDIR)/sql_create.o: ../../../src/include/sys_inc.h
$(DEBUGTARGETDIR)/sql_create.o: ../../../src/server/include/mdb_compat.h
$(DEBUGTARGETDIR)/sql_create.o: ../../../src/include/mdb_typedef.h
$(DEBUGTARGETDIR)/sql_create.o: ../../../src/include/dbport.h
$(DEBUGTARGETDIR)/sql_create.o: ../include/sql_datast.h
$(DEBUGTARGETDIR)/sql_create.o: ../../../src/include/systable.h
$(DEBUGTARGETDIR)/sql_create.o: ../../../src/include/mdb_define.h
$(DEBUGTARGETDIR)/sql_create.o: ../../../src/include/ErrorCode.h
$(DEBUGTARGETDIR)/sql_create.o: ../../../src/server/include/mdb_FieldDesc.h
$(DEBUGTARGETDIR)/sql_create.o: ../../../src/server/include/mdb_FieldVal.h
$(DEBUGTARGETDIR)/sql_create.o: ../../../src/server/include/mdb_KeyDesc.h
$(DEBUGTARGETDIR)/sql_create.o: ../../../src/server/include/mdb_Csr.h
$(DEBUGTARGETDIR)/sql_create.o: ../../../src/server/include/mdb_ContIter.h
$(DEBUGTARGETDIR)/sql_create.o: ../../../src/server/include/mdb_Iterator.h
$(DEBUGTARGETDIR)/sql_create.o: ../../../src/server/include/mdb_Cont.h
$(DEBUGTARGETDIR)/sql_create.o: ../../../src/server/include/mdb_BaseCont.h
$(DEBUGTARGETDIR)/sql_create.o: ../../../src/server/include/mdb_inner_define.h
$(DEBUGTARGETDIR)/sql_create.o: ../../../src/server/include/mdb_Collect.h
$(DEBUGTARGETDIR)/sql_create.o: ../../../src/server/include/mdb_PageList.h
$(DEBUGTARGETDIR)/sql_create.o: ../../../src/server/include/mdb_Page.h
$(DEBUGTARGETDIR)/sql_create.o: ../../../src/server/include/mdb_TTree.h
$(DEBUGTARGETDIR)/sql_create.o: ../../../src/server/include/mdb_TTreeNode.h
$(DEBUGTARGETDIR)/sql_create.o: ../../../src/server/include/mdb_KeyValue.h
$(DEBUGTARGETDIR)/sql_create.o: ../../../src/server/include/mdb_LockMgrPI.h
$(DEBUGTARGETDIR)/sql_create.o: ../../../src/server/include/mdb_ppthread.h
$(DEBUGTARGETDIR)/sql_create.o: ../../../src/server/include/mdb_Trans.h
$(DEBUGTARGETDIR)/sql_create.o: ../../../src/server/include/mdb_LSN.h
$(DEBUGTARGETDIR)/sql_create.o: ../../../src/server/include/mdb_Filter.h
$(DEBUGTARGETDIR)/sql_create.o: ../../../src/server/include/mdb_TTreeIter.h
$(DEBUGTARGETDIR)/sql_create.o: ../include/sql_const.h
$(DEBUGTARGETDIR)/sql_create.o: ../../../src/include/isql.h
$(DEBUGTARGETDIR)/sql_create.o: ../../../src/include/db_api.h
$(DEBUGTARGETDIR)/sql_create.o: ../include/sql_main.h
$(DEBUGTARGETDIR)/sql_create.o: ../../../src/server/include/mdb_dbi.h
$(DEBUGTARGETDIR)/sql_create.o: ../../../src/server/include/mdb_schema_cache.h
$(DEBUGTARGETDIR)/sql_create.o: ../include/sql_util.h
$(DEBUGTARGETDIR)/sql_create.o: ../include/sql_packet_format.h
$(DEBUGTARGETDIR)/sql_create.o: ../../../src/server/include/mdb_PMEM.h
$(DEBUGTARGETDIR)/sql_create.o: ../../../src/server/include/mdb_er.h
$(DEBUGTARGETDIR)/sql_create.o: ../../../src/server/include/mdb_charset.h
$(DEBUGTARGETDIR)/sql_create.o: sql_decimal.h
$(DEBUGTARGETDIR)/sql_create.o: ../include/sql_parser.h sql_mclient.h

$(DEBUGTARGETDIR)/sql_insert.o: sql_insert.c
$(DEBUGTARGETDIR)/sql_insert.o: ../../../src/include/mdb_config.h
$(DEBUGTARGETDIR)/sql_insert.o: ../../../src/include/sys_inc.h
$(DEBUGTARGETDIR)/sql_insert.o: ../../../src/server/include/mdb_compat.h
$(DEBUGTARGETDIR)/sql_insert.o: ../../../src/include/mdb_typedef.h
$(DEBUGTARGETDIR)/sql_insert.o: ../../../src/include/dbport.h
$(DEBUGTARGETDIR)/sql_insert.o: ../include/sql_main.h
$(DEBUGTARGETDIR)/sql_insert.o: ../../../src/server/include/mdb_dbi.h
$(DEBUGTARGETDIR)/sql_insert.o: ../../../src/include/systable.h
$(DEBUGTARGETDIR)/sql_insert.o: ../../../src/include/mdb_define.h
$(DEBUGTARGETDIR)/sql_insert.o: ../../../src/include/ErrorCode.h
$(DEBUGTARGETDIR)/sql_insert.o: ../../../src/server/include/mdb_FieldDesc.h
$(DEBUGTARGETDIR)/sql_insert.o: ../../../src/server/include/mdb_FieldVal.h
$(DEBUGTARGETDIR)/sql_insert.o: ../../../src/server/include/mdb_KeyDesc.h
$(DEBUGTARGETDIR)/sql_insert.o: ../../../src/server/include/mdb_inner_define.h
$(DEBUGTARGETDIR)/sql_insert.o: ../../../src/include/db_api.h
$(DEBUGTARGETDIR)/sql_insert.o: ../../../src/server/include/mdb_KeyValue.h
$(DEBUGTARGETDIR)/sql_insert.o: ../../../src/server/include/mdb_Filter.h
$(DEBUGTARGETDIR)/sql_insert.o: ../../../src/server/include/mdb_schema_cache.h
$(DEBUGTARGETDIR)/sql_insert.o: ../../../src/server/include/mdb_Trans.h
$(DEBUGTARGETDIR)/sql_insert.o: ../../../src/server/include/mdb_LSN.h
$(DEBUGTARGETDIR)/sql_insert.o: ../../../src/server/include/mdb_LockMgrPI.h
$(DEBUGTARGETDIR)/sql_insert.o: ../../../src/server/include/mdb_ppthread.h
$(DEBUGTARGETDIR)/sql_insert.o: ../../../src/server/include/mdb_Csr.h
$(DEBUGTARGETDIR)/sql_insert.o: ../../../src/server/include/mdb_ContIter.h
$(DEBUGTARGETDIR)/sql_insert.o: ../../../src/server/include/mdb_Iterator.h
$(DEBUGTARGETDIR)/sql_insert.o: ../../../src/server/include/mdb_Cont.h
$(DEBUGTARGETDIR)/sql_insert.o: ../../../src/server/include/mdb_BaseCont.h
$(DEBUGTARGETDIR)/sql_insert.o: ../../../src/server/include/mdb_Collect.h
$(DEBUGTARGETDIR)/sql_insert.o: ../../../src/server/include/mdb_PageList.h
$(DEBUGTARGETDIR)/sql_insert.o: ../../../src/server/include/mdb_Page.h
$(DEBUGTARGETDIR)/sql_insert.o: ../../../src/server/include/mdb_TTree.h
$(DEBUGTARGETDIR)/sql_insert.o: ../../../src/server/include/mdb_TTreeNode.h
$(DEBUGTARGETDIR)/sql_insert.o: ../../../src/server/include/mdb_TTreeIter.h
$(DEBUGTARGETDIR)/sql_insert.o: ../../../src/include/isql.h
$(DEBUGTARGETDIR)/sql_insert.o: ../include/sql_const.h
$(DEBUGTARGETDIR)/sql_insert.o: ../include/sql_datast.h ../include/sql_util.h
$(DEBUGTARGETDIR)/sql_insert.o: ../include/sql_packet_format.h
$(DEBUGTARGETDIR)/sql_insert.o: ../../../src/server/include/mdb_PMEM.h
$(DEBUGTARGETDIR)/sql_insert.o: ../../../src/server/include/mdb_er.h
$(DEBUGTARGETDIR)/sql_insert.o: ../../../src/server/include/mdb_charset.h
$(DEBUGTARGETDIR)/sql_insert.o: sql_decimal.h

$(DEBUGTARGETDIR)/sql_update.o: sql_update.c
$(DEBUGTARGETDIR)/sql_update.o: ../../../src/include/mdb_config.h
$(DEBUGTARGETDIR)/sql_update.o: ../../../src/include/sys_inc.h
$(DEBUGTARGETDIR)/sql_update.o: ../../../src/server/include/mdb_compat.h
$(DEBUGTARGETDIR)/sql_update.o: ../../../src/include/mdb_typedef.h
$(DEBUGTARGETDIR)/sql_update.o: ../../../src/include/dbport.h
$(DEBUGTARGETDIR)/sql_update.o: ../include/sql_datast.h
$(DEBUGTARGETDIR)/sql_update.o: ../../../src/include/systable.h
$(DEBUGTARGETDIR)/sql_update.o: ../../../src/include/mdb_define.h
$(DEBUGTARGETDIR)/sql_update.o: ../../../src/include/ErrorCode.h
$(DEBUGTARGETDIR)/sql_update.o: ../../../src/server/include/mdb_FieldDesc.h
$(DEBUGTARGETDIR)/sql_update.o: ../../../src/server/include/mdb_FieldVal.h
$(DEBUGTARGETDIR)/sql_update.o: ../../../src/server/include/mdb_KeyDesc.h
$(DEBUGTARGETDIR)/sql_update.o: ../../../src/server/include/mdb_Csr.h
$(DEBUGTARGETDIR)/sql_update.o: ../../../src/server/include/mdb_ContIter.h
$(DEBUGTARGETDIR)/sql_update.o: ../../../src/server/include/mdb_Iterator.h
$(DEBUGTARGETDIR)/sql_update.o: ../../../src/server/include/mdb_Cont.h
$(DEBUGTARGETDIR)/sql_update.o: ../../../src/server/include/mdb_BaseCont.h
$(DEBUGTARGETDIR)/sql_update.o: ../../../src/server/include/mdb_inner_define.h
$(DEBUGTARGETDIR)/sql_update.o: ../../../src/server/include/mdb_Collect.h
$(DEBUGTARGETDIR)/sql_update.o: ../../../src/server/include/mdb_PageList.h
$(DEBUGTARGETDIR)/sql_update.o: ../../../src/server/include/mdb_Page.h
$(DEBUGTARGETDIR)/sql_update.o: ../../../src/server/include/mdb_TTree.h
$(DEBUGTARGETDIR)/sql_update.o: ../../../src/server/include/mdb_TTreeNode.h
$(DEBUGTARGETDIR)/sql_update.o: ../../../src/server/include/mdb_KeyValue.h
$(DEBUGTARGETDIR)/sql_update.o: ../../../src/server/include/mdb_LockMgrPI.h
$(DEBUGTARGETDIR)/sql_update.o: ../../../src/server/include/mdb_ppthread.h
$(DEBUGTARGETDIR)/sql_update.o: ../../../src/server/include/mdb_Trans.h
$(DEBUGTARGETDIR)/sql_update.o: ../../../src/server/include/mdb_LSN.h
$(DEBUGTARGETDIR)/sql_update.o: ../../../src/server/include/mdb_Filter.h
$(DEBUGTARGETDIR)/sql_update.o: ../../../src/server/include/mdb_TTreeIter.h
$(DEBUGTARGETDIR)/sql_update.o: ../include/sql_const.h
$(DEBUGTARGETDIR)/sql_update.o: ../../../src/include/isql.h
$(DEBUGTARGETDIR)/sql_update.o: ../../../src/include/db_api.h sql_decimal.h
$(DEBUGTARGETDIR)/sql_update.o: ../../../src/server/include/mdb_er.h
$(DEBUGTARGETDIR)/sql_update.o: ../include/sql_main.h
$(DEBUGTARGETDIR)/sql_update.o: ../../../src/server/include/mdb_dbi.h
$(DEBUGTARGETDIR)/sql_update.o: ../../../src/server/include/mdb_schema_cache.h
$(DEBUGTARGETDIR)/sql_update.o: ../include/sql_util.h
$(DEBUGTARGETDIR)/sql_update.o: ../include/sql_packet_format.h
$(DEBUGTARGETDIR)/sql_update.o: ../../../src/server/include/mdb_PMEM.h
$(DEBUGTARGETDIR)/sql_update.o: ../../../src/server/include/mdb_charset.h

$(DEBUGTARGETDIR)/sql_delete.o: sql_delete.c
$(DEBUGTARGETDIR)/sql_delete.o: ../../../src/include/mdb_config.h
$(DEBUGTARGETDIR)/sql_delete.o: ../../../src/include/sys_inc.h
$(DEBUGTARGETDIR)/sql_delete.o: ../../../src/server/include/mdb_compat.h
$(DEBUGTARGETDIR)/sql_delete.o: ../../../src/include/mdb_typedef.h
$(DEBUGTARGETDIR)/sql_delete.o: ../../../src/include/dbport.h
$(DEBUGTARGETDIR)/sql_delete.o: ../include/sql_datast.h
$(DEBUGTARGETDIR)/sql_delete.o: ../../../src/include/systable.h
$(DEBUGTARGETDIR)/sql_delete.o: ../../../src/include/mdb_define.h
$(DEBUGTARGETDIR)/sql_delete.o: ../../../src/include/ErrorCode.h
$(DEBUGTARGETDIR)/sql_delete.o: ../../../src/server/include/mdb_FieldDesc.h
$(DEBUGTARGETDIR)/sql_delete.o: ../../../src/server/include/mdb_FieldVal.h
$(DEBUGTARGETDIR)/sql_delete.o: ../../../src/server/include/mdb_KeyDesc.h
$(DEBUGTARGETDIR)/sql_delete.o: ../../../src/server/include/mdb_Csr.h
$(DEBUGTARGETDIR)/sql_delete.o: ../../../src/server/include/mdb_ContIter.h
$(DEBUGTARGETDIR)/sql_delete.o: ../../../src/server/include/mdb_Iterator.h
$(DEBUGTARGETDIR)/sql_delete.o: ../../../src/server/include/mdb_Cont.h
$(DEBUGTARGETDIR)/sql_delete.o: ../../../src/server/include/mdb_BaseCont.h
$(DEBUGTARGETDIR)/sql_delete.o: ../../../src/server/include/mdb_inner_define.h
$(DEBUGTARGETDIR)/sql_delete.o: ../../../src/server/include/mdb_Collect.h
$(DEBUGTARGETDIR)/sql_delete.o: ../../../src/server/include/mdb_PageList.h
$(DEBUGTARGETDIR)/sql_delete.o: ../../../src/server/include/mdb_Page.h
$(DEBUGTARGETDIR)/sql_delete.o: ../../../src/server/include/mdb_TTree.h
$(DEBUGTARGETDIR)/sql_delete.o: ../../../src/server/include/mdb_TTreeNode.h
$(DEBUGTARGETDIR)/sql_delete.o: ../../../src/server/include/mdb_KeyValue.h
$(DEBUGTARGETDIR)/sql_delete.o: ../../../src/server/include/mdb_LockMgrPI.h
$(DEBUGTARGETDIR)/sql_delete.o: ../../../src/server/include/mdb_ppthread.h
$(DEBUGTARGETDIR)/sql_delete.o: ../../../src/server/include/mdb_Trans.h
$(DEBUGTARGETDIR)/sql_delete.o: ../../../src/server/include/mdb_LSN.h
$(DEBUGTARGETDIR)/sql_delete.o: ../../../src/server/include/mdb_Filter.h
$(DEBUGTARGETDIR)/sql_delete.o: ../../../src/server/include/mdb_TTreeIter.h
$(DEBUGTARGETDIR)/sql_delete.o: ../include/sql_const.h
$(DEBUGTARGETDIR)/sql_delete.o: ../../../src/include/isql.h
$(DEBUGTARGETDIR)/sql_delete.o: ../../../src/include/db_api.h
$(DEBUGTARGETDIR)/sql_delete.o: ../include/sql_main.h
$(DEBUGTARGETDIR)/sql_delete.o: ../../../src/server/include/mdb_dbi.h
$(DEBUGTARGETDIR)/sql_delete.o: ../../../src/server/include/mdb_schema_cache.h
$(DEBUGTARGETDIR)/sql_delete.o: ../include/sql_util.h
$(DEBUGTARGETDIR)/sql_delete.o: ../include/sql_packet_format.h
$(DEBUGTARGETDIR)/sql_delete.o: ../../../src/server/include/mdb_PMEM.h
$(DEBUGTARGETDIR)/sql_delete.o: ../../../src/server/include/mdb_er.h
$(DEBUGTARGETDIR)/sql_delete.o: ../../../src/server/include/mdb_charset.h

$(DEBUGTARGETDIR)/sql_select.o: sql_select.c
$(DEBUGTARGETDIR)/sql_select.o: ../../../src/include/mdb_config.h
$(DEBUGTARGETDIR)/sql_select.o: ../../../src/include/sys_inc.h
$(DEBUGTARGETDIR)/sql_select.o: ../../../src/server/include/mdb_compat.h
$(DEBUGTARGETDIR)/sql_select.o: ../../../src/include/mdb_typedef.h
$(DEBUGTARGETDIR)/sql_select.o: ../../../src/include/dbport.h
$(DEBUGTARGETDIR)/sql_select.o: ../include/sql_main.h
$(DEBUGTARGETDIR)/sql_select.o: ../../../src/server/include/mdb_dbi.h
$(DEBUGTARGETDIR)/sql_select.o: ../../../src/include/systable.h
$(DEBUGTARGETDIR)/sql_select.o: ../../../src/include/mdb_define.h
$(DEBUGTARGETDIR)/sql_select.o: ../../../src/include/ErrorCode.h
$(DEBUGTARGETDIR)/sql_select.o: ../../../src/server/include/mdb_FieldDesc.h
$(DEBUGTARGETDIR)/sql_select.o: ../../../src/server/include/mdb_FieldVal.h
$(DEBUGTARGETDIR)/sql_select.o: ../../../src/server/include/mdb_KeyDesc.h
$(DEBUGTARGETDIR)/sql_select.o: ../../../src/server/include/mdb_inner_define.h
$(DEBUGTARGETDIR)/sql_select.o: ../../../src/server/include/mdb_Msg.h
$(DEBUGTARGETDIR)/sql_select.o: ../../../src/include/db_api.h
$(DEBUGTARGETDIR)/sql_select.o: ../../../src/server/include/mdb_KeyValue.h
$(DEBUGTARGETDIR)/sql_select.o: ../../../src/server/include/mdb_Filter.h
$(DEBUGTARGETDIR)/sql_select.o: ../../../src/server/include/mdb_schema_cache.h
$(DEBUGTARGETDIR)/sql_select.o: ../../../src/server/include/mdb_Trans.h
$(DEBUGTARGETDIR)/sql_select.o: ../../../src/server/include/mdb_LSN.h
$(DEBUGTARGETDIR)/sql_select.o: ../../../src/server/include/mdb_LockMgrPI.h
$(DEBUGTARGETDIR)/sql_select.o: ../../../src/server/include/mdb_ppthread.h
$(DEBUGTARGETDIR)/sql_select.o: ../../../src/server/include/mdb_Csr.h
$(DEBUGTARGETDIR)/sql_select.o: ../../../src/server/include/mdb_ContIter.h
$(DEBUGTARGETDIR)/sql_select.o: ../../../src/server/include/mdb_Iterator.h
$(DEBUGTARGETDIR)/sql_select.o: ../../../src/server/include/mdb_Cont.h
$(DEBUGTARGETDIR)/sql_select.o: ../../../src/server/include/mdb_BaseCont.h
$(DEBUGTARGETDIR)/sql_select.o: ../../../src/server/include/mdb_Collect.h
$(DEBUGTARGETDIR)/sql_select.o: ../../../src/server/include/mdb_PageList.h
$(DEBUGTARGETDIR)/sql_select.o: ../../../src/server/include/mdb_Page.h
$(DEBUGTARGETDIR)/sql_select.o: ../../../src/server/include/mdb_TTree.h
$(DEBUGTARGETDIR)/sql_select.o: ../../../src/server/include/mdb_TTreeNode.h
$(DEBUGTARGETDIR)/sql_select.o: ../../../src/server/include/mdb_TTreeIter.h
$(DEBUGTARGETDIR)/sql_select.o: ../../../src/include/isql.h
$(DEBUGTARGETDIR)/sql_select.o: ../include/sql_const.h
$(DEBUGTARGETDIR)/sql_select.o: ../include/sql_datast.h ../include/sql_util.h
$(DEBUGTARGETDIR)/sql_select.o: ../include/sql_packet_format.h
$(DEBUGTARGETDIR)/sql_select.o: ../../../src/server/include/mdb_PMEM.h
$(DEBUGTARGETDIR)/sql_select.o: ../../../src/server/include/mdb_er.h
$(DEBUGTARGETDIR)/sql_select.o: ../../../src/server/include/mdb_charset.h
$(DEBUGTARGETDIR)/sql_select.o: ../include/sql_execute.h sql_decimal.h
$(DEBUGTARGETDIR)/sql_select.o: sql_func_timeutil.h

$(DEBUGTARGETDIR)/sql_mclient.o: sql_mclient.c
$(DEBUGTARGETDIR)/sql_mclient.o: ../../../src/include/mdb_config.h
$(DEBUGTARGETDIR)/sql_mclient.o: ../../../src/include/sys_inc.h
$(DEBUGTARGETDIR)/sql_mclient.o: ../../../src/server/include/mdb_compat.h
$(DEBUGTARGETDIR)/sql_mclient.o: ../../../src/include/mdb_typedef.h
$(DEBUGTARGETDIR)/sql_mclient.o: ../../../src/include/dbport.h
$(DEBUGTARGETDIR)/sql_mclient.o: ../../../src/server/include/mdb_profile.h sql_mclient.h
$(DEBUGTARGETDIR)/sql_mclient.o: ../include/sql_main.h
$(DEBUGTARGETDIR)/sql_mclient.o: ../../../src/server/include/mdb_dbi.h
$(DEBUGTARGETDIR)/sql_mclient.o: ../../../src/include/systable.h
$(DEBUGTARGETDIR)/sql_mclient.o: ../../../src/include/mdb_define.h
$(DEBUGTARGETDIR)/sql_mclient.o: ../../../src/include/ErrorCode.h
$(DEBUGTARGETDIR)/sql_mclient.o: ../../../src/server/include/mdb_FieldDesc.h
$(DEBUGTARGETDIR)/sql_mclient.o: ../../../src/server/include/mdb_FieldVal.h
$(DEBUGTARGETDIR)/sql_mclient.o: ../../../src/server/include/mdb_KeyDesc.h
$(DEBUGTARGETDIR)/sql_mclient.o: ../../../src/server/include/mdb_inner_define.h
$(DEBUGTARGETDIR)/sql_mclient.o: ../../../src/server/include/mdb_Msg.h
$(DEBUGTARGETDIR)/sql_mclient.o: ../../../src/include/db_api.h
$(DEBUGTARGETDIR)/sql_mclient.o: ../../../src/server/include/mdb_KeyValue.h
$(DEBUGTARGETDIR)/sql_mclient.o: ../../../src/server/include/mdb_Filter.h
$(DEBUGTARGETDIR)/sql_mclient.o: ../../../src/server/include/mdb_schema_cache.h
$(DEBUGTARGETDIR)/sql_mclient.o: ../../../src/server/include/mdb_Trans.h
$(DEBUGTARGETDIR)/sql_mclient.o: ../../../src/server/include/mdb_LSN.h
$(DEBUGTARGETDIR)/sql_mclient.o: ../../../src/server/include/mdb_LockMgrPI.h
$(DEBUGTARGETDIR)/sql_mclient.o: ../../../src/server/include/mdb_ppthread.h
$(DEBUGTARGETDIR)/sql_mclient.o: ../../../src/server/include/mdb_Csr.h
$(DEBUGTARGETDIR)/sql_mclient.o: ../../../src/server/include/mdb_ContIter.h
$(DEBUGTARGETDIR)/sql_mclient.o: ../../../src/server/include/mdb_Iterator.h
$(DEBUGTARGETDIR)/sql_mclient.o: ../../../src/server/include/mdb_Cont.h
$(DEBUGTARGETDIR)/sql_mclient.o: ../../../src/server/include/mdb_BaseCont.h
$(DEBUGTARGETDIR)/sql_mclient.o: ../../../src/server/include/mdb_Collect.h
$(DEBUGTARGETDIR)/sql_mclient.o: ../../../src/server/include/mdb_PageList.h
$(DEBUGTARGETDIR)/sql_mclient.o: ../../../src/server/include/mdb_Page.h
$(DEBUGTARGETDIR)/sql_mclient.o: ../../../src/server/include/mdb_TTree.h
$(DEBUGTARGETDIR)/sql_mclient.o: ../../../src/server/include/mdb_TTreeNode.h
$(DEBUGTARGETDIR)/sql_mclient.o: ../../../src/server/include/mdb_TTreeIter.h
$(DEBUGTARGETDIR)/sql_mclient.o: ../../../src/include/isql.h
$(DEBUGTARGETDIR)/sql_mclient.o: ../include/sql_const.h ../include/sql_datast.h
$(DEBUGTARGETDIR)/sql_mclient.o: ../include/sql_util.h ../include/sql_packet_format.h
$(DEBUGTARGETDIR)/sql_mclient.o: ../../../src/server/include/mdb_PMEM.h
$(DEBUGTARGETDIR)/sql_mclient.o: ../../../src/server/include/mdb_er.h
$(DEBUGTARGETDIR)/sql_mclient.o: ../../../src/server/include/mdb_charset.h
$(DEBUGTARGETDIR)/sql_mclient.o: ../include/sql_parser.h

$(DEBUGTARGETDIR)/sql_interface.o: sql_interface.c
$(DEBUGTARGETDIR)/sql_interface.o: ../../../src/include/mdb_config.h
$(DEBUGTARGETDIR)/sql_interface.o: ../../../src/include/sys_inc.h
$(DEBUGTARGETDIR)/sql_interface.o: ../../../src/server/include/mdb_comm_stub.h
$(DEBUGTARGETDIR)/sql_interface.o: ../../../src/include/mdb_typedef.h
$(DEBUGTARGETDIR)/sql_interface.o: ../../../src/include/dbport.h
$(DEBUGTARGETDIR)/sql_interface.o: ../../../src/server/include/mdb_ppthread.h
$(DEBUGTARGETDIR)/sql_interface.o: ../../../src/server/include/mdb_compat.h
$(DEBUGTARGETDIR)/sql_interface.o: ../../../src/include/ErrorCode.h
$(DEBUGTARGETDIR)/sql_interface.o: ../include/sql_packet_format.h
$(DEBUGTARGETDIR)/sql_interface.o: ../include/sql_main.h
$(DEBUGTARGETDIR)/sql_interface.o: ../../../src/server/include/mdb_dbi.h
$(DEBUGTARGETDIR)/sql_interface.o: ../../../src/include/systable.h
$(DEBUGTARGETDIR)/sql_interface.o: ../../../src/include/mdb_define.h
$(DEBUGTARGETDIR)/sql_interface.o: ../../../src/server/include/mdb_FieldDesc.h
$(DEBUGTARGETDIR)/sql_interface.o: ../../../src/server/include/mdb_FieldVal.h
$(DEBUGTARGETDIR)/sql_interface.o: ../../../src/server/include/mdb_KeyDesc.h
$(DEBUGTARGETDIR)/sql_interface.o: ../../../src/server/include/mdb_inner_define.h
$(DEBUGTARGETDIR)/sql_interface.o: ../../../src/server/include/mdb_Msg.h
$(DEBUGTARGETDIR)/sql_interface.o: ../../../src/include/db_api.h
$(DEBUGTARGETDIR)/sql_interface.o: ../../../src/server/include/mdb_KeyValue.h
$(DEBUGTARGETDIR)/sql_interface.o: ../../../src/server/include/mdb_Filter.h
$(DEBUGTARGETDIR)/sql_interface.o: ../../../src/server/include/mdb_schema_cache.h
$(DEBUGTARGETDIR)/sql_interface.o: ../../../src/server/include/mdb_Trans.h
$(DEBUGTARGETDIR)/sql_interface.o: ../../../src/server/include/mdb_LSN.h
$(DEBUGTARGETDIR)/sql_interface.o: ../../../src/server/include/mdb_LockMgrPI.h
$(DEBUGTARGETDIR)/sql_interface.o: ../../../src/server/include/mdb_Csr.h
$(DEBUGTARGETDIR)/sql_interface.o: ../../../src/server/include/mdb_ContIter.h
$(DEBUGTARGETDIR)/sql_interface.o: ../../../src/server/include/mdb_Iterator.h
$(DEBUGTARGETDIR)/sql_interface.o: ../../../src/server/include/mdb_Cont.h
$(DEBUGTARGETDIR)/sql_interface.o: ../../../src/server/include/mdb_BaseCont.h
$(DEBUGTARGETDIR)/sql_interface.o: ../../../src/server/include/mdb_Collect.h
$(DEBUGTARGETDIR)/sql_interface.o: ../../../src/server/include/mdb_PageList.h
$(DEBUGTARGETDIR)/sql_interface.o: ../../../src/server/include/mdb_Page.h
$(DEBUGTARGETDIR)/sql_interface.o: ../../../src/server/include/mdb_TTree.h
$(DEBUGTARGETDIR)/sql_interface.o: ../../../src/server/include/mdb_TTreeNode.h
$(DEBUGTARGETDIR)/sql_interface.o: ../../../src/server/include/mdb_TTreeIter.h
$(DEBUGTARGETDIR)/sql_interface.o: ../../../src/include/isql.h
$(DEBUGTARGETDIR)/sql_interface.o: ../include/sql_const.h
$(DEBUGTARGETDIR)/sql_interface.o: ../include/sql_datast.h ../include/sql_util.h
$(DEBUGTARGETDIR)/sql_interface.o: ../../../src/server/include/mdb_PMEM.h
$(DEBUGTARGETDIR)/sql_interface.o: ../../../src/server/include/mdb_er.h
$(DEBUGTARGETDIR)/sql_interface.o: ../../../src/server/include/mdb_charset.h
$(DEBUGTARGETDIR)/sql_interface.o: sql_func_timeutil.h sql_mclient.h
$(DEBUGTARGETDIR)/sql_interface.o: ../include/sql_parser.h

$(DEBUGTARGETDIR)/sql_result.o: sql_result.c
$(DEBUGTARGETDIR)/sql_result.o: ../../../src/include/mdb_config.h
$(DEBUGTARGETDIR)/sql_result.o: ../../../src/include/sys_inc.h
$(DEBUGTARGETDIR)/sql_result.o: ../include/sql_datast.h
$(DEBUGTARGETDIR)/sql_result.o: ../../../src/server/include/mdb_compat.h
$(DEBUGTARGETDIR)/sql_result.o: ../../../src/include/mdb_typedef.h
$(DEBUGTARGETDIR)/sql_result.o: ../../../src/include/dbport.h
$(DEBUGTARGETDIR)/sql_result.o: ../../../src/include/systable.h
$(DEBUGTARGETDIR)/sql_result.o: ../../../src/include/mdb_define.h
$(DEBUGTARGETDIR)/sql_result.o: ../../../src/include/ErrorCode.h
$(DEBUGTARGETDIR)/sql_result.o: ../../../src/server/include/mdb_FieldDesc.h
$(DEBUGTARGETDIR)/sql_result.o: ../../../src/server/include/mdb_FieldVal.h
$(DEBUGTARGETDIR)/sql_result.o: ../../../src/server/include/mdb_KeyDesc.h
$(DEBUGTARGETDIR)/sql_result.o: ../../../src/server/include/mdb_Csr.h
$(DEBUGTARGETDIR)/sql_result.o: ../../../src/server/include/mdb_ContIter.h
$(DEBUGTARGETDIR)/sql_result.o: ../../../src/server/include/mdb_Iterator.h
$(DEBUGTARGETDIR)/sql_result.o: ../../../src/server/include/mdb_Cont.h
$(DEBUGTARGETDIR)/sql_result.o: ../../../src/server/include/mdb_BaseCont.h
$(DEBUGTARGETDIR)/sql_result.o: ../../../src/server/include/mdb_inner_define.h
$(DEBUGTARGETDIR)/sql_result.o: ../../../src/server/include/mdb_Collect.h
$(DEBUGTARGETDIR)/sql_result.o: ../../../src/server/include/mdb_PageList.h
$(DEBUGTARGETDIR)/sql_result.o: ../../../src/server/include/mdb_Page.h
$(DEBUGTARGETDIR)/sql_result.o: ../../../src/server/include/mdb_TTree.h
$(DEBUGTARGETDIR)/sql_result.o: ../../../src/server/include/mdb_TTreeNode.h
$(DEBUGTARGETDIR)/sql_result.o: ../../../src/server/include/mdb_KeyValue.h
$(DEBUGTARGETDIR)/sql_result.o: ../../../src/server/include/mdb_LockMgrPI.h
$(DEBUGTARGETDIR)/sql_result.o: ../../../src/server/include/mdb_ppthread.h
$(DEBUGTARGETDIR)/sql_result.o: ../../../src/server/include/mdb_Trans.h
$(DEBUGTARGETDIR)/sql_result.o: ../../../src/server/include/mdb_LSN.h
$(DEBUGTARGETDIR)/sql_result.o: ../../../src/server/include/mdb_Filter.h
$(DEBUGTARGETDIR)/sql_result.o: ../../../src/server/include/mdb_TTreeIter.h
$(DEBUGTARGETDIR)/sql_result.o: ../include/sql_const.h
$(DEBUGTARGETDIR)/sql_result.o: ../../../src/include/isql.h
$(DEBUGTARGETDIR)/sql_result.o: ../../../src/include/db_api.h
$(DEBUGTARGETDIR)/sql_result.o: ../include/sql_main.h
$(DEBUGTARGETDIR)/sql_result.o: ../../../src/server/include/mdb_dbi.h
$(DEBUGTARGETDIR)/sql_result.o: ../../../src/server/include/mdb_Msg.h
$(DEBUGTARGETDIR)/sql_result.o: ../../../src/server/include/mdb_schema_cache.h
$(DEBUGTARGETDIR)/sql_result.o: ../include/sql_util.h
$(DEBUGTARGETDIR)/sql_result.o: ../include/sql_packet_format.h
$(DEBUGTARGETDIR)/sql_result.o: ../../../src/server/include/mdb_PMEM.h
$(DEBUGTARGETDIR)/sql_result.o: ../../../src/server/include/mdb_er.h
$(DEBUGTARGETDIR)/sql_result.o: ../../../src/server/include/mdb_charset.h
$(DEBUGTARGETDIR)/sql_result.o: sql_func_timeutil.h sql_mclient.h
$(DEBUGTARGETDIR)/sql_result.o: ../include/sql_parser.h

$(DEBUGTARGETDIR)/sql_decimal.o: sql_decimal.c
$(DEBUGTARGETDIR)/sql_decimal.o: ../../../src/include/mdb_config.h
$(DEBUGTARGETDIR)/sql_decimal.o: ../../../src/include/sys_inc.h
$(DEBUGTARGETDIR)/sql_decimal.o: ../../../src/server/include/mdb_compat.h
$(DEBUGTARGETDIR)/sql_decimal.o: ../../../src/include/mdb_typedef.h
$(DEBUGTARGETDIR)/sql_decimal.o: ../../../src/include/dbport.h
$(DEBUGTARGETDIR)/sql_decimal.o: ../include/sql_datast.h
$(DEBUGTARGETDIR)/sql_decimal.o: ../../../src/include/systable.h
$(DEBUGTARGETDIR)/sql_decimal.o: ../../../src/include/mdb_define.h
$(DEBUGTARGETDIR)/sql_decimal.o: ../../../src/include/ErrorCode.h
$(DEBUGTARGETDIR)/sql_decimal.o: ../../../src/server/include/mdb_FieldDesc.h
$(DEBUGTARGETDIR)/sql_decimal.o: ../../../src/server/include/mdb_FieldVal.h
$(DEBUGTARGETDIR)/sql_decimal.o: ../../../src/server/include/mdb_KeyDesc.h
$(DEBUGTARGETDIR)/sql_decimal.o: ../../../src/server/include/mdb_Csr.h
$(DEBUGTARGETDIR)/sql_decimal.o: ../../../src/server/include/mdb_ContIter.h
$(DEBUGTARGETDIR)/sql_decimal.o: ../../../src/server/include/mdb_Iterator.h
$(DEBUGTARGETDIR)/sql_decimal.o: ../../../src/server/include/mdb_Cont.h
$(DEBUGTARGETDIR)/sql_decimal.o: ../../../src/server/include/mdb_BaseCont.h
$(DEBUGTARGETDIR)/sql_decimal.o: ../../../src/server/include/mdb_inner_define.h
$(DEBUGTARGETDIR)/sql_decimal.o: ../../../src/server/include/mdb_Collect.h
$(DEBUGTARGETDIR)/sql_decimal.o: ../../../src/server/include/mdb_PageList.h
$(DEBUGTARGETDIR)/sql_decimal.o: ../../../src/server/include/mdb_Page.h
$(DEBUGTARGETDIR)/sql_decimal.o: ../../../src/server/include/mdb_TTree.h
$(DEBUGTARGETDIR)/sql_decimal.o: ../../../src/server/include/mdb_TTreeNode.h
$(DEBUGTARGETDIR)/sql_decimal.o: ../../../src/server/include/mdb_KeyValue.h
$(DEBUGTARGETDIR)/sql_decimal.o: ../../../src/server/include/mdb_LockMgrPI.h
$(DEBUGTARGETDIR)/sql_decimal.o: ../../../src/server/include/mdb_ppthread.h
$(DEBUGTARGETDIR)/sql_decimal.o: ../../../src/server/include/mdb_Trans.h
$(DEBUGTARGETDIR)/sql_decimal.o: ../../../src/server/include/mdb_LSN.h
$(DEBUGTARGETDIR)/sql_decimal.o: ../../../src/server/include/mdb_Filter.h
$(DEBUGTARGETDIR)/sql_decimal.o: ../../../src/server/include/mdb_TTreeIter.h
$(DEBUGTARGETDIR)/sql_decimal.o: ../include/sql_const.h
$(DEBUGTARGETDIR)/sql_decimal.o: ../../../src/include/isql.h
$(DEBUGTARGETDIR)/sql_decimal.o: ../../../src/include/db_api.h
$(DEBUGTARGETDIR)/sql_decimal.o: ../include/sql_util.h ../include/sql_packet_format.h
$(DEBUGTARGETDIR)/sql_decimal.o: ../include/sql_main.h
$(DEBUGTARGETDIR)/sql_decimal.o: ../../../src/server/include/mdb_dbi.h
$(DEBUGTARGETDIR)/sql_decimal.o: ../../../src/server/include/mdb_Msg.h
$(DEBUGTARGETDIR)/sql_decimal.o: ../../../src/server/include/mdb_schema_cache.h
$(DEBUGTARGETDIR)/sql_decimal.o: ../../../src/server/include/mdb_er.h
$(DEBUGTARGETDIR)/sql_decimal.o: ../../../src/server/include/mdb_charset.h
$(DEBUGTARGETDIR)/sql_decimal.o: ../../../src/server/include/mdb_PMEM.h

$(TARGET_LIB_DIR)/libsql.a: sql_main.c sql_parser.c sql_norm.c sql_plan.c sql_execute.c sql_util.c sql_calc_expression.c sql_calc_function.c sql_calc_timefunc.c sql_func_timeutil.c sql_worktable.c sql_drop.c sql_create.c sql_insert.c sql_update.c sql_delete.c sql_select.c sql_mclient.c sql_interface.c sql_result.c sql_decimal.c -L../../../lib -laladdin parser.y parser.l
$(TARGET_LIB_DIR)/libsql.a: ../../../src/include/mdb_config.h
$(TARGET_LIB_DIR)/libsql.a: ../../../src/include/sys_inc.h
$(TARGET_LIB_DIR)/libsql.a: ../../../src/server/include/mdb_compat.h
$(TARGET_LIB_DIR)/libsql.a: ../../../src/include/mdb_typedef.h
$(TARGET_LIB_DIR)/libsql.a: ../../../src/include/dbport.h ../include/sql_main.h
$(TARGET_LIB_DIR)/libsql.a: ../../../src/server/include/mdb_dbi.h
$(TARGET_LIB_DIR)/libsql.a: ../../../src/include/systable.h
$(TARGET_LIB_DIR)/libsql.a: ../../../src/include/mdb_define.h
$(TARGET_LIB_DIR)/libsql.a: ../../../src/include/ErrorCode.h
$(TARGET_LIB_DIR)/libsql.a: ../../../src/server/include/mdb_FieldDesc.h
$(TARGET_LIB_DIR)/libsql.a: ../../../src/server/include/mdb_FieldVal.h
$(TARGET_LIB_DIR)/libsql.a: ../../../src/server/include/mdb_KeyDesc.h
$(TARGET_LIB_DIR)/libsql.a: ../../../src/server/include/mdb_inner_define.h
$(TARGET_LIB_DIR)/libsql.a: ../../../src/server/include/mdb_Msg.h
$(TARGET_LIB_DIR)/libsql.a: ../../../src/include/db_api.h
$(TARGET_LIB_DIR)/libsql.a: ../../../src/server/include/mdb_KeyValue.h
$(TARGET_LIB_DIR)/libsql.a: ../../../src/server/include/mdb_Filter.h
$(TARGET_LIB_DIR)/libsql.a: ../../../src/server/include/mdb_schema_cache.h
$(TARGET_LIB_DIR)/libsql.a: ../../../src/server/include/mdb_Trans.h
$(TARGET_LIB_DIR)/libsql.a: ../../../src/server/include/mdb_LSN.h
$(TARGET_LIB_DIR)/libsql.a: ../../../src/server/include/mdb_LockMgrPI.h
$(TARGET_LIB_DIR)/libsql.a: ../../../src/server/include/mdb_ppthread.h
$(TARGET_LIB_DIR)/libsql.a: ../../../src/server/include/mdb_Csr.h
$(TARGET_LIB_DIR)/libsql.a: ../../../src/server/include/mdb_ContIter.h
$(TARGET_LIB_DIR)/libsql.a: ../../../src/server/include/mdb_Iterator.h
$(TARGET_LIB_DIR)/libsql.a: ../../../src/server/include/mdb_Cont.h
$(TARGET_LIB_DIR)/libsql.a: ../../../src/server/include/mdb_BaseCont.h
$(TARGET_LIB_DIR)/libsql.a: ../../../src/server/include/mdb_Collect.h
$(TARGET_LIB_DIR)/libsql.a: ../../../src/server/include/mdb_PageList.h
$(TARGET_LIB_DIR)/libsql.a: ../../../src/server/include/mdb_Page.h
$(TARGET_LIB_DIR)/libsql.a: ../../../src/server/include/mdb_TTree.h
$(TARGET_LIB_DIR)/libsql.a: ../../../src/server/include/mdb_TTreeNode.h
$(TARGET_LIB_DIR)/libsql.a: ../../../src/server/include/mdb_TTreeIter.h
$(TARGET_LIB_DIR)/libsql.a: ../../../src/include/isql.h ../include/sql_const.h
$(TARGET_LIB_DIR)/libsql.a: ../include/sql_datast.h ../include/sql_util.h
$(TARGET_LIB_DIR)/libsql.a: ../include/sql_packet_format.h
$(TARGET_LIB_DIR)/libsql.a: ../../../src/server/include/mdb_PMEM.h
$(TARGET_LIB_DIR)/libsql.a: ../../../src/server/include/mdb_er.h
$(TARGET_LIB_DIR)/libsql.a: ../../../src/server/include/mdb_charset.h
$(TARGET_LIB_DIR)/libsql.a: ../include/sql_parser.h ../include/sql_plan.h
$(TARGET_LIB_DIR)/libsql.a: ../include/sql_execute.h ../include/sql_norm.h
$(TARGET_LIB_DIR)/libsql.a: sql_func_timeutil.h sql_mclient.h
$(TARGET_LIB_DIR)/libsql.a: ../../../src/server/include/mdb_Server.h
$(TARGET_LIB_DIR)/libsql.a: ../../../src/server/include/mdb_ContCat.h
$(TARGET_LIB_DIR)/libsql.a: ../../../src/server/include/mdb_IndexCat.h
$(TARGET_LIB_DIR)/libsql.a: ../../../src/server/include/mdb_LogMgr.h
$(TARGET_LIB_DIR)/libsql.a: ../../../src/server/include/mdb_LogAnchor.h
$(TARGET_LIB_DIR)/libsql.a: ../../../src/server/include/mdb_Recovery.h
$(TARGET_LIB_DIR)/libsql.a: ../../../src/server/include/mdb_LogBuffer.h
$(TARGET_LIB_DIR)/libsql.a: ../../../src/server/include/mdb_LogFile.h
$(TARGET_LIB_DIR)/libsql.a: ../../../src/server/include/mdb_LogRec.h
$(TARGET_LIB_DIR)/libsql.a: ../../../src/server/include/mdb_Mem_Mgr.h
$(TARGET_LIB_DIR)/libsql.a: ../../../src/server/include/mdb_MemBase.h
$(TARGET_LIB_DIR)/libsql.a: ../../../src/server/include/mdb_Segment.h
$(TARGET_LIB_DIR)/libsql.a: ../../../src/server/include/mdb_DBFile.h
$(TARGET_LIB_DIR)/libsql.a: ../../../src/server/include/mdb_syslog.h
$(TARGET_LIB_DIR)/libsql.a: ../../../src/server/include/mdb_Thread.h
$(TARGET_LIB_DIR)/libsql.a: ../../../src/server/include/mdb_comm_stub.h
$(TARGET_LIB_DIR)/libsql.a: ../../../src/server/include/mdb_memmgr.h
$(TARGET_LIB_DIR)/libsql.a: sql_decimal.h sql_calc_timefunc.h
$(TARGET_LIB_DIR)/libsql.a: ../../../src/server/include/mdb_profile.h

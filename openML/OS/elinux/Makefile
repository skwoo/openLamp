#################################################
#       EMWARE
#################################################
#
# @autohr
#   Kimyoung, Jin(jinki@emware.co.kr)
#
#################################################


#################################################
# General Setting
#################################################
CC  = $(CCBIN)
LD  = $(LDBIN)
ifeq ($(ARBIN),)
    AR	= ar
else
    AR	= $(ARBIN)
endif

ifeq ($(COMPILER),arm-gcc-android)
	PTHREAD =
else
	PTHREAD = -lpthread
endif

HOME_PATH = ../..
MMDB_SRC_PATH = $(HOME_PATH)/src
LIB_PATH = $(HOME_PATH)/lib

#################################################
# Complie Option
#################################################
DEBUGFLAGS  =

CFLAGS      = $(CMODE) $(CMODE_PIC) -D_REENTRANT

COPT = $(DEBUGFLAGS) $(CFLAGS)

SHAREDLIB            = lib$(DB_LIB_NAME).so
STATICLIB            = lib$(DB_LIB_NAME).a

#################################################
# Source and header file directory
#################################################

COMM_STUB_SRCDIR     = $(MMDB_SRC_PATH)/comm_stub/
CLIENT_SRCDIR        = $(MMDB_SRC_PATH)/client/
UTILITY_SRCDIR       = $(MMDB_SRC_PATH)/utils/
API_SRCDIR           = $(MMDB_SRC_PATH)/server/API/
COMM_SRCDIR          = $(MMDB_SRC_PATH)/server/common/
LOCK_SRCDIR          = $(MMDB_SRC_PATH)/server/lock/
RECOVERY_SRCDIR      = $(MMDB_SRC_PATH)/server/recovery/
STORAGE_SRCDIR       = $(MMDB_SRC_PATH)/server/storage/
USE_LOCK_SRCDIR      = $(MMDB_SRC_PATH)/server/use_lock/
SQL_SRCDIR           = $(MMDB_SRC_PATH)/server/sql/
SRC_INCDIR           = $(MMDB_SRC_PATH)/include
SERVER_INCDIR        = $(MMDB_SRC_PATH)/server/include
SQL_INCDIR           = $(MMDB_SRC_PATH)/server/sql


ifeq ($(COMPILER),arm-gcc)
INCOPT = -I/usr/local/arm/3.4/include 
endif

INCOPT += -I$(SRC_INCDIR) -I$(SERVER_INCDIR) -I$(SQL_INCDIR)

#################################################
#   Pattern-matching rules : c -> o
#################################################
ifeq ($(CC),lint)
	_C_OPTION=
else
	_C_OPTION= -c
endif

objs/%.o: $(COMM_STUB_SRCDIR)%.c
	$(CC) $(COPT) $(_C_OPTION) $(INCOPT) $< -o $@

objs/%.o: $(CLIENT_SRCDIR)%.c
	$(CC) $(COPT) $(_C_OPTION) $(INCOPT) $< -o $@

objs/%.o: $(UTILITY_SRCDIR)%.c
	$(CC) $(COPT) $(_C_OPTION) $(INCOPT) $< -o $@

objs/%.o: $(API_SRCDIR)%.c
	$(CC) $(COPT) $(_C_OPTION) $(INCOPT) $< -o $@

objs/%.o: $(COMM_SRCDIR)%.c
	$(CC) $(COPT) $(_C_OPTION) $(INCOPT) $< -o $@

objs/%.o: $(LOCK_SRCDIR)%.c
	$(CC) $(COPT) $(_C_OPTION) $(INCOPT) $< -o $@

objs/%.o: $(RECOVERY_SRCDIR)%.c
	$(CC) $(COPT) $(_C_OPTION) $(INCOPT) $< -o $@

objs/%.o: $(STORAGE_SRCDIR)%.c
	$(CC) $(COPT) $(_C_OPTION) $(INCOPT) $< -o $@

objs/%.o: $(USE_LOCK_SRCDIR)%.c
	$(CC) $(COPT) $(_C_OPTION) $(INCOPT) $< -o $@

objs/%.o: $(SQL_SRCDIR)%.c
	$(CC) $(COPT) $(_C_OPTION) $(INCOPT) $< -o $@

#################################################
#   SOURCE LIST
#################################################
# jinki : 컴파일할 파일 리스트를 기술
#################################################
COMM_STUB_SRC    = mdb_commstub.c

CLIENT_SRC       = sql_iSQL.c

UTILITY_SRC      = mdb_createdb.c 

API_SRC          = mdb_dbi.c mdb_Insert.c mdb_Read.c mdb_Remove.c \
                   mdb_Schema.c mdb_Update.c

COMM_SRC         = mdb_compat.c mdb_configure.o mdb_memmgr.c mdb_er.c \
                   mdb_ErrorCode.c mdb_charset.c mdb_PMEM.c mdb_ppthread.c \
                   mdb_Speed.c sc_default.c mdb_SysLog.c \
                   mdb_TransMgr.c mdb_ucs2vsutf.c

LOCK_SRC         = mdb_LockMgr.c

RECOVERY_SRC     = mdb_AfterLog.c mdb_BeforeLog.c mdb_ChkPnt.c mdb_CLR.c \
                   mdb_LogAnchor.c mdb_LogBuffer.c mdb_LogFile.c \
                   mdb_LogMgr.c mdb_LogRec.c mdb_MemAnchorLog.c \
                   mdb_PhysicalLog.c mdb_TransAbortLog.c \
                   mdb_TransBeginLog.c mdb_TransCommitLog.c mdb_UpdateTrans.c

STORAGE_SRC      = mdb_Collect.c mdb_Cont.c mdb_ContCat.c mdb_ContIter.c \
                   mdb_CsrMgr.c mdb_DBFile.c mdb_DbMgr.c mdb_FieldDesc.c \
                   mdb_FieldVal.c mdb_Filter.c mdb_hiddenfield.c \
                   mdb_Index_Collect.c mdb_Index_MemMgr.c mdb_Index_Mgr.c \
                   mdb_Index_OID.c mdb_IndexCat.c mdb_Iterator.c \
                   mdb_KeyDesc.c mdb_KeyValue.c mdb_Mem_Mgr.c mdb_OID.c \
                   mdb_PageID.c mdb_TTree.c mdb_TTree_Create.c \
                   mdb_TTree_Insert.c mdb_TTree_Remove.c mdb_TTreeIter.c \
                   mdb_TTreeNode.c

SQL_SRC          = sql_calc_expression.c sql_calc_function.c \
                   sql_calc_timefunc.c sql_ce_lex.yy.o sql_ce_y.tab.o \
                   sql_decimal.c sql_func_timeutil.o sql_mclient.c \
                   sql_interface.c sql_main.c sql_create.c sql_delete.c \
                   sql_drop.c sql_execute.c sql_insert.c sql_norm.c \
                   sql_parser.c sql_plan.c sql_select.c sql_update.c \
                   sql_util.c sql_result.c sql_worktable.c

MMDB_SRC = $(COMM_STUB_SRC) $(CLIENT_SRC) $(UTILITY_SRC) $(API_SRC) \
			$(COMM_SRC) $(LOCK_SRC) $(RECOVERY_SRC) $(STORAGE_SRC) $(SQL_SRC)

MMDB_OBJ = $(addprefix objs/,$(MMDB_SRC:.c=.o))

LIBRARIES = $(SHAREDLIB) $(STATICLIB)

all: check_dir makelib

check_dir:
ifeq ($(OSNAME),SunOS)
	-@mkdir objs
	-touch objs_backup/*
	-cp objs_backup/*.o objs/
endif
ifeq ($(OSNAME),Linux)
	@if [ -e objs_backup ]; then (touch objs_backup/*); else (mkdir objs_backup); fi
	@if [ ! -e objs ]; then (mkdir objs); fi
	@if [ -e objs_backup/*.o ]; then (cp objs_backup/*.o objs/); fi
endif

makelib: $(LIBRARIES) sharedlib_pass2

$(SHAREDLIB): $(MMDB_OBJ)
	$(LD) -o $(SHAREDLIB) $(MMDB_OBJ) -lm $(PTHREAD) -ldl
	@cp $(SHAREDLIB) $(HOME_PATH)/lib
	
$(STATICLIB): $(MMDB_OBJ)
	$(AR) rc $(STATICLIB) $(MMDB_OBJ)
	@cp $(STATICLIB) $(HOME_PATH)/lib

sharedlib_pass2:
	$(LD) -o $(SHAREDLIB) $(MMDB_OBJ) -L$(HOME_PATH)/lib -lm $(PTHREAD) -ldl
	@cp $(SHAREDLIB) $(HOME_PATH)/lib
#	$(LD) -o $(SHAREDLIB) $(MMDB_OBJ) -lm $(PTHREAD) -ldl
#	@cp $(SHAREDLIB) $(HOME_PATH)/lib

clean:
	rm -rf *.o $(SHAREDLIB) $(STATICLIB) 
	rm -rf *.o $(HOME_PATH)/lib/$(SHAREDLIB) $(HOME_PATH)/lib/$(STATICLIB)
	rm -rf objs/*.o

distclean: clean
#	-rm -rf objs_backup/*.o

objs/sc_default.o: $(COMM_SRCDIR)sc_default*.c
	$(CC) $(COPT) $(_C_OPTION) $(INCOPT) $(COMM_SRCDIR)sc_default.c -o $@

objs/mdb_DBFile.o: $(STORAGE_SRCDIR)mdb_DBFile*.c
	$(CC) $(COPT) $(_C_OPTION) $(INCOPT) $(STORAGE_SRCDIR)mdb_DBFile.c -o $@

objs/sql_calc_function.o: $(SQL_SRCDIR)sql_calc_function.c
	$(CC) $(COPT) $(_C_OPTION) $(INCOPT) $(SQL_SRCDIR)sql_calc_function.c -o $@

objs/mdb_dbi.o: $(API_SRCDIR)mdb_dbi.c $(API_SRCDIR)mdb_like.c
	$(CC) $(COPT) $(_C_OPTION) $(INCOPT) $(API_SRCDIR)mdb_dbi.c -o $@

objs/mdb_compat.o: $(COMM_SRCDIR)mdb_compat.c
	$(CC) $(COPT) $(_C_OPTION) $(INCOPT) $(COMM_SRCDIR)mdb_compat.c -o $@


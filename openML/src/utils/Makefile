###########################################
# Makefile for SPEED openML 0.0.1
###########################################

.KEEP_STATE:

###########################################
# Environment setting
###########################################

###########################################
# General setting
###########################################

CC = $(CCBIN)
LD = $(LDBIN)
LIBS = $(SYSLIBS)

MOBILELITE_HOME = ../..

CFLAGS  = $(CMODE) -c
COPT = -c $(CMODE) -D_REENTRANT -D_UTILITY_ -DCREATE_DB

LIBS_TO_BE_CHECKED = ../../lib/lib$(DB_LIB_NAME).a

MOBILELITELIB = -L../../lib -l$(DB_LIB_NAME) $(CMODE_LIBPATH) $(COMP_LIB)

INCLUDE = -I$(MOBILELITE_HOME)/src/include -I$(MOBILELITE_HOME)/src/server/include

# readline
# interactive prompt 
ifeq ($(HAVE_READLINE),1)
    COPT += -DHAVE_READLINE
    INCLUDE += -I/usr/include/readline
    LIBS += -lreadline -lncurses
endif

###########################################
# Source and header file directory
###########################################

ACTIONSRCDIR	= ../server/action/
COMMONSRCDIR	= ../server/common/
STORAGESRCDIR	= ../server/storage/
RECOVERYSRCDIR	= ../server/recovery/
APISRCDIR		= ../server/API/
USECHECKSRCDIR	= ../server/use_lock/
CLIENTLIBSRCDIR	= ../clientlib/
UTILITYSRCDIR	= ./

VPATH = ${ACTIONSRCDIR}:${COMMONSRCDIR}:${STORAGESRCDIR}:${RECOVERYSRCDIR}:${APISRCDIR}:${USECHECKSRCDIR}:${CLIENTLIBSRCDIR}:${UTILITYSRCDIR}

SERVERINCDIR    = ../server/include/
COMMONINCDIR    = ../include/

###########################################
# Pattern-matching rules: c -> o
###########################################

#%.o: $(USECHECKSRCDIR)%.c
#	$(CC) $(COPT) -I$(SERVERINCDIR) -I$(COMMONINCDIR) $<
#
#%.o: $(UTILITYSRCDIR)%.c
#	$(CC) $(COPT) -I$(SERVERINCDIR) -I$(COMMONINCDIR) $<
#
#%.o: $(COMMONSRCDIR)%.c
#	$(CC) $(COPT) -I$(SERVERINCDIR) -I$(COMMONINCDIR) $<
#
%.o: %.c
	$(CC) $(COPT) -I$(SERVERINCDIR) -I$(COMMONINCDIR) $<


###########################################
# SPEED openML 0.0.1 CREATEDB
###########################################

CREATEDBSRC = mdb_createdb.c
CREATEDBOBJ = $(CREATEDBSRC:.c=.o)

DESTROYDBSRC = mdb_destroydb.c
DESTROYDBOBJ = $(DESTROYDBSRC:.c=.o)
    
LOGDUMPSRC = mdb_logdump.c
LOGDUMPOBJ = $(LOGDUMPSRC:.c=.o)

BINARIES = createdb destroydb isql isql2 isqlucs logdump

all : $(BINARIES)
	-@cp createdb ../../bin/
	-@cp destroydb ../../bin/
	-@cp logdump ../../bin/

createdb: $(CREATEDBOBJ) $(LIBS_TO_BE_CHECKED)
	$(CC) $(LOPT) -o  $@ $(CREATEDBOBJ) $(MOBILELITELIB) $(LIBS)

destroydb: $(DESTROYDBOBJ) $(LIBS_TO_BE_CHECKED)
	$(CC) $(LOPT) -o  $@ $(DESTROYDBOBJ) $(MOBILELITELIB) $(LIBS)

logdump: $(LOGDUMPOBJ) $(LIBS_TO_BE_CHECKED)
	$(CC) $(LOPT) -o  $@ $(LOGDUMPOBJ) $(MOBILELITELIB) $(LIBS)

## in Makefile_sql.mobile
isql: mdb_isql.o $(LIBS_TO_BE_CHECKED)
	$(CC) $(LOPT) -o $@ mdb_isql.o $(MOBILELITELIB) $(LIBS)
	cp $@ $(MOBILELITE_HOME)/bin/

isql2: mdb_isql2.o $(LIBS_TO_BE_CHECKED)
	$(CC) $(LOPT) -o $@ mdb_isql2.o $(MOBILELITELIB) $(LIBS)
	cp $@ $(MOBILELITE_HOME)/bin/

isqlucs: mdb_isql2.c  $(LIBS_TO_BE_CHECKED)
	$(CC) $(COPT) $(LOPT) -c -DISQL_UCS -o isql_ucs.o mdb_isql2.c $(INCLUDE)
	$(CC) $(LOPT) -o $@ isql_ucs.o $(MOBILELITELIB) $(LIBS) 
	cp $@ $(MOBILELITE_HOME)/bin/

###########################################
# SPEED openML 0.0.1 Compile
###########################################

clean:
	rm -rf core *.o *.a createdb destroydb logdump \
           $(MOBILELITE_HOME)/bin/isql $(MOBILELITE_HOME)/bin/isql2 \
           isql isql2 isqlucs

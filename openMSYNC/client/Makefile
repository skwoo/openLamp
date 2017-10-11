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

HOME_PATH = ..
MSYNC_SRC_PATH = $(HOME_PATH)
LIB_PATH = $(HOME_PATH)/lib

#################################################
# Complie Option
#################################################
DEBUGFLAGS  =

CFLAGS      = $(CMODE) $(CMODE_PIC) -D_REENTRANT

COPT = $(DEBUGFLAGS) $(CFLAGS)

SHAREDLIB            = lib$(MSYNC_LIB_NAME).so
STATICLIB            = lib$(MSYNC_LIB_NAME).a

#################################################
# Source and header file directory
#################################################

CLIENT_SRCDIR        = $(MSYNC_SRC_PATH)/client/src/
CLIENT_INCDIR        = $(MSYNC_SRC_PATH)/client/include


ifeq ($(COMPILER),arm-gcc)
INCOPT = -I/usr/local/arm/3.4/include 
endif

INCOPT += -I$(CLIENT_INCDIR) 

#################################################
#   Pattern-matching rules : c -> o
#################################################
ifeq ($(CC),lint)
	_C_OPTION=
else
	_C_OPTION= -c
endif

objs/%.o: $(CLIENT_SRCDIR)%.c
	$(CC) $(COPT) $(_C_OPTION) $(INCOPT) $< -o $@


#################################################
#   SOURCE LIST
#################################################
CLIENT_SRC       = export.c  state.c

MSYNCCLIENT_SRC = $(CLIENT_SRC) 
MSYNCCLIENT_OBJ = $(addprefix objs/,$(MSYNCCLIENT_SRC:.c=.o))

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

$(SHAREDLIB): $(MSYNCCLIENT_OBJ)
	$(LD) -o $(SHAREDLIB) $(MSYNCCLIENT_OBJ) -lm $(PTHREAD) -ldl
	@cp $(SHAREDLIB) $(HOME_PATH)/lib
	
$(STATICLIB): $(MSYNCCLIENT_OBJ)
	$(AR) rc $(STATICLIB) $(MSYNCCLIENT_OBJ)
	@cp $(STATICLIB) $(HOME_PATH)/lib

sharedlib_pass2:
	$(LD) -o $(SHAREDLIB) $(MSYNCCLIENT_OBJ) -L$(HOME_PATH)/lib -lm $(PTHREAD) -ldl
	@cp $(SHAREDLIB) $(HOME_PATH)/lib
#	$(LD) -o $(SHAREDLIB) $(MSYNCCLIENT_OBJ) -lm $(PTHREAD) -ldl
#	@cp $(SHAREDLIB) $(HOME_PATH)/lib

clean:
	rm -rf *.o $(SHAREDLIB) $(STATICLIB) 
	rm -rf *.o $(HOME_PATH)/lib/$(SHAREDLIB) $(HOME_PATH)/lib/$(STATICLIB)
	rm -rf objs/*.o

distclean: clean
#	-rm -rf objs_backup/*.o




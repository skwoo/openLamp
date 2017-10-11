DB_HOME = ../..

CC = $(CCBIN)
LD = $(LDBIN)
#make static library
#LIBS = $(SYSLIBS) $(DB_HOME)/lib/lib$(DB_LIB_NAME).a
#make shared library
LIBS = -L$(DB_HOME)/lib $(SYSLIBS) -l$(DB_LIB_NAME)

COPT = $(CMODE) -fvisibility=default $(CMODE_PIC)

COMPILE = $(CC) -c $(COPT) $(INCLUDE)

INCLUDE = -I. -I$(DB_HOME)/src/include -I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/linux

JAVAC = javac
JAVAH = javah

#JNI_LIB = lib$(DB_LIB_NAME)_jedb.so
JNI_LIB = libjedb.so
JNI_SRC = com_openml_JEDB.c ConvertString.c mdb_MLapi.c
JNI_OBJ = $(JNI_SRC:.c=.o)

all : jni $(JNI_LIB) jsql
release : jni $(JNI_LIB) jsql

jni:
ifeq ($(JAVA_HOME),)
	@echo "Not set JAVA_HOME"
else
	$(JAVAC) com/openml/JEDB.java
	$(JAVAH) -jni com.openml.JEDB
endif

$(JNI_LIB) : $(JNI_OBJ)
ifneq ($(JAVA_HOME),)
	$(LD) $(COPT) -o $@ $(JNI_OBJ) $(LIBS)
	@cp $(JNI_LIB) $(DB_HOME)/lib/.
	@cp com/openml/JEDB.java $(DB_HOME)/include/.
endif

jsql:
ifneq ($(JAVA_HOME),)
	@$(JAVAC) com/openml/app/Jsql.java
	@jar -cvmf manifest.txt Jsql.jar com/openml/*.class com/openml/app/*.class
	@cp *.jar $(DB_HOME)/bin/.
	@cp Jsql $(DB_HOME)/bin/.
endif

.c.o:
	$(COMPILE) $<

clean :
	rm -rf *.o *.so *.jar mapfile
	rm -rf com/openml/*.class
	rm -rf com/openml/app/*.class

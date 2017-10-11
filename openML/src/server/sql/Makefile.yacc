BUILD             = release

CC                = $(CCBIN)
INCLUDE           = -I../include -I../../include
CFLAGS            = -Wall $(INCLUDE) $(CMODE)
#SHAREDCFLAGS      = -fPIC

HOSTNAME          = $(shell /bin/hostname)

OBJS_TYPE         = static

SHAREDCFLAGS	  = $(CMODE_PIC)

PARSER_PREFIX     = parser
YACC_OBJS_INFIX   = yy
YACC_FILE_SUFFIX  = y
YACC_OBJS         = $(addprefix $(PARSER_PREFIX).,$(addsuffix .o,$(YACC_OBJS_INFIX)))
LEX_OBJS_INFIX    = ll
LEX_FILE_SUFFIX   = l
LEX_OBJS          = $(addprefix $(PARSER_PREFIX).,$(addsuffix .o,$(LEX_OBJS_INFIX)))

RELEASEFLAGS      = 
RELEASETARGETDIR  = release-objs
RELEASE_YACC_OBJS = $(addprefix $(RELEASETARGETDIR)/,$(YACC_OBJS))
RELEASE_LEX_OBJS  = $(addprefix $(RELEASETARGETDIR)/,$(LEX_OBJS))
DEBUGFLAGS        = -g -DYYDEBUG=0
DEBUGTARGETDIR    = debug-objs
DEBUG_YACC_OBJS   = $(addprefix $(DEBUGTARGETDIR)/,$(YACC_OBJS))
DEBUG_LEX_OBJS    = $(addprefix $(DEBUGTARGETDIR)/,$(LEX_OBJS))

#LEX               = flex -P __yy -f -i -8 -t --bison-bridge --nounistd
#LEX               = flex -P __yy -I -i -8 -t --bison-bridge --nounistd
#YACC              = bison -y -d -p __yy
LEX     = flex -P __yy -I -i -8 -t --bison-bridge --nounistd -o sql_lex.yy.c
YACC    = bison -y -d -p __yy -o sql_y.tab.c

.SUFFIXES: .c .o .l .y .h

all: $(BUILD)
	
release: $(RELEASE_YACC_OBJS) $(RELEASE_LEX_OBJS)

debug: $(DEBUG_YACC_OBJS) $(DEBUG_LEX_OBJS)

$(RELEASE_YACC_OBJS):
	-@if [ ! -d $(RELEASETARGETDIR) ]; then mkdir $(RELEASETARGETDIR); fi
ifeq ($(OBJS_TYPE),static)
	$(CC) -D_RELEASE $(CFLAGS) $(RELEASEFLAGS) $(SHAREDCFLAGS) -c sql_y.tab.c -o $(addprefix $(RELEASETARGETDIR)/,$(addprefix $(PARSER_PREFIX).,$(addsuffix .o,$(YACC_OBJS_INFIX))))
else
	$(CC) -D_RELEASE $(CFLAGS) $(RELEASEFLAGS) $(SHAREDCFLAGS) -c sql_y.tab.c -o $(addprefix $(RELEASETARGETDIR)/,$(addprefix $(PARSER_PREFIX).,$(addsuffix .o,$(YACC_OBJS_INFIX))))
endif

$(RELEASE_LEX_OBJS):
	-@if [ ! -d $(RELEASETARGETDIR) ]; then mkdir $(RELEASETARGETDIR); fi
ifeq ($(OBJS_TYPE),static)
	$(CC) -D_RELEASE $(CFLAGS) $(RELEASEFLAGS) $(SHAREDCFLAGS) -c sql_lex.yy.c -o $(addprefix $(RELEASETARGETDIR)/,$(addprefix $(PARSER_PREFIX).,$(addsuffix .o,$(LEX_OBJS_INFIX))))
else
	$(CC) -D_RELEASE $(CFLAGS) $(RELEASEFLAGS) $(SHAREDCFLAGS) -c sql_lex.yy.c -o $(addprefix $(RELEASETARGETDIR)/,$(addprefix $(PARSER_PREFIX).,$(addsuffix .o,$(LEX_OBJS_INFIX))))
endif

$(DEBUG_YACC_OBJS):
	-@if [ ! -d $(DEBUGTARGETDIR) ]; then mkdir $(DEBUGTARGETDIR); fi
ifeq ($(CC),lint)
# SWKIM 2007.06.14 compilation message cleanup
#	$(CC) -DMDB_DEBUG $(CFLAGS) $(DEBUGFLAGS) $(SHAREDCFLAGS) sql_y.tab.c
	$(CC) $(CFLAGS) $(DEBUGFLAGS) $(SHAREDCFLAGS) sql_y.tab.c
else
ifeq ($(OBJS_TYPE),static)
# SWKIM 2007.06.14 compilation message cleanup
#	$(CC) -DMDB_DEBUG $(CFLAGS) $(DEBUGFLAGS) $(SHAREDCFLAGS) -c sql_y.tab.c -o $(addprefix $(DEBUGTARGETDIR)/,$(addprefix $(PARSER_PREFIX).,$(addsuffix .o,$(YACC_OBJS_INFIX))))
	$(CC) $(CFLAGS) $(DEBUGFLAGS) $(SHAREDCFLAGS) -c sql_y.tab.c -o $(addprefix $(DEBUGTARGETDIR)/,$(addprefix $(PARSER_PREFIX).,$(addsuffix .o,$(YACC_OBJS_INFIX))))
else
# SWKIM 2007.06.14 compilation message cleanup
#	$(CC) -DMDB_DEBUG $(CFLAGS) $(DEBUGFLAGS) $(SHAREDCFLAGS) -c sql_y.tab.c -o $(addprefix $(DEBUGTARGETDIR)/,$(addprefix $(PARSER_PREFIX).,$(addsuffix .o,$(YACC_OBJS_INFIX))))
	$(CC) $(CFLAGS) $(DEBUGFLAGS) $(SHAREDCFLAGS) -c sql_y.tab.c -o $(addprefix $(DEBUGTARGETDIR)/,$(addprefix $(PARSER_PREFIX).,$(addsuffix .o,$(YACC_OBJS_INFIX))))
endif
endif

$(DEBUG_LEX_OBJS):
	-@if [ ! -d $(DEBUGTARGETDIR) ]; then mkdir $(DEBUGTARGETDIR); fi
ifeq ($(CC),lint)
# SWKIM 2007.06.14 compilation message cleanup
#	$(CC) -DMDB_DEBUG $(CFLAGS) $(DEBUGFLAGS) $(SHAREDCFLAGS) sql_lex.yy.c
	$(CC) $(CFLAGS) $(DEBUGFLAGS) $(SHAREDCFLAGS) sql_lex.yy.c
else
ifeq ($(OBJS_TYPE),static)
# SWKIM 2007.06.14 compilation message cleanup
#	$(CC) -DMDB_DEBUG $(CFLAGS) $(DEBUGFLAGS) $(SHAREDCFLAGS) -c sql_lex.yy.c -o $(addprefix $(DEBUGTARGETDIR)/,$(addprefix $(PARSER_PREFIX).,$(addsuffix .o,$(LEX_OBJS_INFIX))))
	$(CC) $(CFLAGS) $(DEBUGFLAGS) $(SHAREDCFLAGS) -c sql_lex.yy.c -o $(addprefix $(DEBUGTARGETDIR)/,$(addprefix $(PARSER_PREFIX).,$(addsuffix .o,$(LEX_OBJS_INFIX))))
else
# SWKIM 2007.06.14 compilation message cleanup
#	$(CC) -DMDB_DEBUG $(CFLAGS) $(DEBUGFLAGS) $(SHAREDCFLAGS) -c sql_lex.yy.c -o $(addprefix $(DEBUGTARGETDIR)/,$(addprefix $(PARSER_PREFIX).,$(addsuffix .o,$(LEX_OBJS_INFIX))))
	$(CC) $(CFLAGS) $(DEBUGFLAGS) $(SHAREDCFLAGS) -c sql_lex.yy.c -o $(addprefix $(DEBUGTARGETDIR)/,$(addprefix $(PARSER_PREFIX).,$(addsuffix .o,$(LEX_OBJS_INFIX))))
endif
endif

sql_y.tab.h:

sql_y.tab.c: $(addprefix $(PARSER_PREFIX).,$(YACC_FILE_SUFFIX))
ifeq ($(ENABLE_YACC),1)
	$(YACC) $(addprefix $(PARSER_PREFIX).,$(YACC_FILE_SUFFIX))
	-@rm _$@_tmp
	@sed "s/int yychar;/int yychar; T_POSTFIXELEM elem; char buf\[MDB_BUFSIZ\]; T_EXPRESSIONDESC *expr; char *p; int i; char tablename\[REL_NAME_LENG\]; int ret;/g" $@ | \
        sed "/ YYSTACK_ALLOC alloca/d" | \
        sed "/ YYSTACK_ALLOC __builtin_alloca/d" \
        > _$@_tmp
	@mv _$@_tmp $@
	-@rm sql_ce_y.tab.c
	@sed "/^#line/d" $@ | \
     sed "s/# define YYINITDEPTH 200/# define YYINITDEPTH 20/g" | \
     sed "s/YYSTYPE yylval/static YYSTYPE yylval/g" | \
     sed "s/short   yyssa/static short yyssa/g" | \
     sed "s/YYSTYPE yyvsa/static YYSTYPE yyvsa/g" | \
     sed "s/YYSTYPE yyval/static YYSTYPE yyval/g" | \
     sed "s/strcasecmp/sc_strcasecmp/g" | \
	 sed "s/yychar; T_POSTFIXELEM/yychar; static T_POSTFIXELEM/g" | \
	 sed "s/char buf\[MDB_BUFSIZ\]/static char buf\[MDB_BUFSIZ\]/g" | \
     sed "/#[ 	]*include[ 	]*</d" | \
     sed "s/FILE \*/int \*/g" | \
     sed "/ YYSTACK_ALLOC alloca/d" | \
     sed "/ YYSTACK_ALLOC __builtin_alloca/d" \
     > sql_ce_y.tab.c
endif

sql_lex.yy.c lexyy.c: $(addprefix $(PARSER_PREFIX).,$(LEX_FILE_SUFFIX))
ifeq ($(ENABLE_YACC),1)
	$(LEX) $(addprefix $(PARSER_PREFIX).,$(LEX_FILE_SUFFIX)) | \
	sed "s/"\$Header"//g" | sed "s/ malloc( size )/ PMEM_ALLOC( size )/g" | \
    sed "s/ realloc( (char/ PMEM_REALLOC( (char/g" | \
    sed "s/free( (/PMEM_FREENUL( (/g" | \
    sed "s/PMEM_FREENUL( (char \*)/PMEM_FREENUL( /g" > $@
	-@rm sql_ce_lex.yy.c
	@sed "/#include <errno.h>/d" $@ | sed "/#include <unistd.h>/d" | \
	 sed "s/b->yy_is_interactive = file ? (isatty( fileno(file) ) > 0) : 0;/b->yy_is_interactive = 0;/g" | sed "/^#line/d" | \
	 sed "s/exit(/\/\/exit(/g" | \
	 sed "s/#include <stdio.h>/#include \"sql_lex_inc.h\"/g" | \
	 sed "/#include <string.h>/d" | \
	 sed "/#include <stdlib.h>/d" | \
     sed "s/size_t yy_size_t/unsigned int yy_size_t/g" | \
     sed "s/size_t num_to_read =/int num_to_read =/g" | \
     sed "/#[ 	]*include[ 	]*</d" | \
     sed "s/memset/sc_memset/g" | \
     sed "s/strlen/sc_strlen/g" | \
     sed "s/printf/sc_printf/g" | \
     sed "s/FILE \*/int \*/g" | \
     sed "s/ENOMEM/SQL_E_OUTOFMEMORY/g" | \
     sed "s/\<errno\>/lex_errno/g" \
     > sql_ce_lex.yy.c
endif

win: sql_y.tab.c sql_y.tab.h lexyy.c
	-@del parser.ll.c
	-@del parser.yy.c
	@ren sql_y.tab.c parser.yy.c
	@ren lexyy.c parser.ll.c

clean: $(addsuffix -clean,$(BUILD))
ifeq ($(ENABLE_YACC),1)
	@rm -f sql_y.tab.h sql_y.tab.c sql_lex.yy.c sql_ce_y.tab.h sql_ce_y.tab.c sql_ce_lex.yy.c
endif

distclean: release-clean debug-clean
ifeq ($(ENABLE_YACC),1)
	@rm -f sql_y.tab.h sql_y.tab.c sql_lex.yy.c sql_ce_y.tab.h sql_ce_y.tab.c sql_ce_lex.yy.c
endif
	@rm -rf $(RELEASETARGETDIR) $(DEBUGTARGETDIR)
	@rm -f tags

release-clean:
	@rm -f $(addprefix $(RELEASETARGETDIR)/,$(addprefix $(PARSER_PREFIX).,$(addsuffix .o,$(YACC_OBJS_INFIX)))) $(addprefix $(RELEASETARGETDIR)/,$(addprefix $(PARSER_PREFIX).,$(addsuffix .o,$(LEX_OBJS_INFIX))))
ifeq ($(ENABLE_YACC),1)
	@rm -f sql_y.tab.h sql_y.tab.c sql_lex.yy.c
endif

debug-clean:
	@rm -f $(addprefix $(DEBUGTARGETDIR)/,$(addprefix $(PARSER_PREFIX).,$(addsuffix .o,$(YACC_OBJS_INFIX)))) $(addprefix $(DEBUGTARGETDIR)/,$(addprefix $(PARSER_PREFIX).,$(addsuffix .o,$(LEX_OBJS_INFIX))))
ifeq ($(ENABLE_YACC),1)
	@rm -f sql_y.tab.h sql_y.tab.c sql_lex.yy.c
endif

depend:
ifneq ($(NOECHO),1)
	@echo "Create or update source and target file dependencies..."
endif
	@cat $(addprefix $(PARSER_PREFIX).,$(YACC_FILE_SUFFIX)) | grep \#include > .yacc_include.c
	@cat $(addprefix $(PARSER_PREFIX).,$(YACC_FILE_SUFFIX)) | grep \#include > .lex_include.c
	@makedepend -fMakefile.yacc -s"# FILE DEPENDENCIES BELOW; DO NOT EDIT OR DELETE IT" -p "$$(RELEASETARGETDIR)/" -Y -- $(INCLUDE) .yacc_include.c 2> /dev/null
	@echo "$$(RELEASETARGETDIR)/.yacc_include.o: sql_y.tab.c" >> Makefile.yacc
	@makedepend -fMakefile.yacc -s"# FILE DEPENDENCIES BELOW; DO NOT EDIT OR DELETE IT" -a -p "$$(DEBUGTARGETDIR)/" -Y -- $(INCLUDE) .yacc_include.c 2> /dev/null
	@echo "$$(DEBUGTARGETDIR)/.yacc_include.o: sql_y.tab.c" >> Makefile.yacc
	@rm -f .yacc_include.c
	@makedepend -fMakefile.yacc -s"# FILE DEPENDENCIES BELOW; DO NOT EDIT OR DELETE IT" -a -p "$$(RELEASETARGETDIR)/" -Y -- $(INCLUDE) .lex_include.c 2> /dev/null
	@echo "$$(RELEASETARGETDIR)/.lex_include.o: sql_lex.yy.c" >> Makefile.yacc
	@makedepend -fMakefile.yacc -s"# FILE DEPENDENCIES BELOW; DO NOT EDIT OR DELETE IT" -a -p "$$(DEBUGTARGETDIR)/" -Y -- $(INCLUDE) .lex_include.c 2> /dev/null
	@echo "$$(DEBUGTARGETDIR)/.lex_include.o: sql_lex.yy.c" >> Makefile.yacc
	@rm -f .lex_include.c
	@rm -f Makefile.yacc.bak
	@head -`grep -n "^# FILE DEPENDENCIES BELOW; DO NOT EDIT OR DELETE IT" Makefile.yacc | cut -d: -f 1` Makefile.yacc > .makefile.yacc
	@sed -n -e :a -e "1,`grep -n \"^# FILE DEPENDENCIES BELOW; DO NOT EDIT OR DELETE IT\" Makefile.yacc | cut -d: -f1`!{P;N;D;};N;ba" Makefile.yacc > .depend.yacc
	@sed "s/\.yacc_include\.o/$(YACC_OBJS)/g" .depend.yacc | sed "s/\.lex_include\.o/$(LEX_OBJS)/g" >> ._depend.yacc
	@cat ._depend.yacc >> .makefile.yacc
	@mv -f .makefile.yacc Makefile.yacc
	@rm -f ._depend.yacc .depend.yacc

# FILE DEPENDENCIES BELOW; DO NOT EDIT OR DELETE IT

$(RELEASETARGETDIR)/parser.yy.o: ../../../src/include/mdb_config.h
$(RELEASETARGETDIR)/parser.yy.o: ../../../src/include/sys_inc.h
$(RELEASETARGETDIR)/parser.yy.o: ../../../src/include/mdb_define.h
$(RELEASETARGETDIR)/parser.yy.o: ../../../src/include/ErrorCode.h
$(RELEASETARGETDIR)/parser.yy.o: ../../../src/include/isql.h
$(RELEASETARGETDIR)/parser.yy.o: ../../../src/server/include/mdb_FieldDesc.h
$(RELEASETARGETDIR)/parser.yy.o: ../../../src/include/mdb_typedef.h
$(RELEASETARGETDIR)/parser.yy.o: ../../../src/include/dbport.h
$(RELEASETARGETDIR)/parser.yy.o: ../../../src/server/include/mdb_FieldVal.h
$(RELEASETARGETDIR)/parser.yy.o: ../../../src/include/db_api.h
$(RELEASETARGETDIR)/parser.yy.o: ../../../src/server/include/mdb_KeyValue.h
$(RELEASETARGETDIR)/parser.yy.o: ../../../src/server/include/mdb_KeyDesc.h
$(RELEASETARGETDIR)/parser.yy.o: ../../../src/server/include/mdb_Filter.h
$(RELEASETARGETDIR)/parser.yy.o: ../../../src/include/systable.h
$(RELEASETARGETDIR)/parser.yy.o: ../include/sql_datast.h
$(RELEASETARGETDIR)/parser.yy.o: ../../../src/server/include/mdb_compat.h
$(RELEASETARGETDIR)/parser.yy.o: ../../../src/server/include/mdb_Csr.h
$(RELEASETARGETDIR)/parser.yy.o: ../../../src/server/include/mdb_ContIter.h
$(RELEASETARGETDIR)/parser.yy.o: ../../../src/server/include/mdb_Iterator.h
$(RELEASETARGETDIR)/parser.yy.o: ../../../src/server/include/mdb_Cont.h
$(RELEASETARGETDIR)/parser.yy.o: ../../../src/server/include/mdb_BaseCont.h
$(RELEASETARGETDIR)/parser.yy.o: ../../../src/server/include/mdb_inner_define.h
$(RELEASETARGETDIR)/parser.yy.o: ../../../src/server/include/mdb_Collect.h
$(RELEASETARGETDIR)/parser.yy.o: ../../../src/server/include/mdb_PageList.h
$(RELEASETARGETDIR)/parser.yy.o: ../../../src/server/include/mdb_Page.h
$(RELEASETARGETDIR)/parser.yy.o: ../../../src/server/include/mdb_TTree.h
$(RELEASETARGETDIR)/parser.yy.o: ../../../src/server/include/mdb_TTreeNode.h
$(RELEASETARGETDIR)/parser.yy.o: ../../../src/server/include/mdb_LockMgrPI.h
$(RELEASETARGETDIR)/parser.yy.o: ../../../src/server/include/mdb_ppthread.h
$(RELEASETARGETDIR)/parser.yy.o: ../../../src/server/include/mdb_Trans.h
$(RELEASETARGETDIR)/parser.yy.o: ../../../src/server/include/mdb_LSN.h
$(RELEASETARGETDIR)/parser.yy.o: ../../../src/server/include/mdb_TTreeIter.h
$(RELEASETARGETDIR)/parser.yy.o: ../include/sql_const.h
$(RELEASETARGETDIR)/parser.yy.o: ../include/sql_parser.h
$(RELEASETARGETDIR)/parser.yy.o: ../include/sql_util.h
$(RELEASETARGETDIR)/parser.yy.o: ../include/sql_packet_format.h
$(RELEASETARGETDIR)/parser.yy.o: ../include/sql_main.h
$(RELEASETARGETDIR)/parser.yy.o: ../../../src/server/include/mdb_dbi.h
$(RELEASETARGETDIR)/parser.yy.o: ../../../src/server/include/mdb_schema_cache.h
$(RELEASETARGETDIR)/parser.yy.o: ../../../src/server/include/mdb_er.h
$(RELEASETARGETDIR)/parser.yy.o: ../../../src/server/include/mdb_charset.h
$(RELEASETARGETDIR)/parser.yy.o: ../../../src/server/include/mdb_PMEM.h
$(RELEASETARGETDIR)/parser.yy.o: sql_calc_timefunc.h sql_func_timeutil.h
$(RELEASETARGETDIR)/parser.yy.o: sql_mclient.h
$(RELEASETARGETDIR)/parser.yy.o: sql_y.tab.c

$(DEBUGTARGETDIR)/parser.yy.o: ../../../src/include/mdb_config.h
$(DEBUGTARGETDIR)/parser.yy.o: ../../../src/include/sys_inc.h
$(DEBUGTARGETDIR)/parser.yy.o: ../../../src/include/mdb_define.h
$(DEBUGTARGETDIR)/parser.yy.o: ../../../src/include/ErrorCode.h
$(DEBUGTARGETDIR)/parser.yy.o: ../../../src/include/isql.h
$(DEBUGTARGETDIR)/parser.yy.o: ../../../src/server/include/mdb_FieldDesc.h
$(DEBUGTARGETDIR)/parser.yy.o: ../../../src/include/mdb_typedef.h
$(DEBUGTARGETDIR)/parser.yy.o: ../../../src/include/dbport.h
$(DEBUGTARGETDIR)/parser.yy.o: ../../../src/server/include/mdb_FieldVal.h
$(DEBUGTARGETDIR)/parser.yy.o: ../../../src/include/db_api.h
$(DEBUGTARGETDIR)/parser.yy.o: ../../../src/server/include/mdb_KeyValue.h
$(DEBUGTARGETDIR)/parser.yy.o: ../../../src/server/include/mdb_KeyDesc.h
$(DEBUGTARGETDIR)/parser.yy.o: ../../../src/server/include/mdb_Filter.h
$(DEBUGTARGETDIR)/parser.yy.o: ../../../src/include/systable.h
$(DEBUGTARGETDIR)/parser.yy.o: ../include/sql_datast.h
$(DEBUGTARGETDIR)/parser.yy.o: ../../../src/server/include/mdb_compat.h
$(DEBUGTARGETDIR)/parser.yy.o: ../../../src/server/include/mdb_Csr.h
$(DEBUGTARGETDIR)/parser.yy.o: ../../../src/server/include/mdb_ContIter.h
$(DEBUGTARGETDIR)/parser.yy.o: ../../../src/server/include/mdb_Iterator.h
$(DEBUGTARGETDIR)/parser.yy.o: ../../../src/server/include/mdb_Cont.h
$(DEBUGTARGETDIR)/parser.yy.o: ../../../src/server/include/mdb_BaseCont.h
$(DEBUGTARGETDIR)/parser.yy.o: ../../../src/server/include/mdb_inner_define.h
$(DEBUGTARGETDIR)/parser.yy.o: ../../../src/server/include/mdb_Collect.h
$(DEBUGTARGETDIR)/parser.yy.o: ../../../src/server/include/mdb_PageList.h
$(DEBUGTARGETDIR)/parser.yy.o: ../../../src/server/include/mdb_Page.h
$(DEBUGTARGETDIR)/parser.yy.o: ../../../src/server/include/mdb_TTree.h
$(DEBUGTARGETDIR)/parser.yy.o: ../../../src/server/include/mdb_TTreeNode.h
$(DEBUGTARGETDIR)/parser.yy.o: ../../../src/server/include/mdb_LockMgrPI.h
$(DEBUGTARGETDIR)/parser.yy.o: ../../../src/server/include/mdb_ppthread.h
$(DEBUGTARGETDIR)/parser.yy.o: ../../../src/server/include/mdb_Trans.h
$(DEBUGTARGETDIR)/parser.yy.o: ../../../src/server/include/mdb_LSN.h
$(DEBUGTARGETDIR)/parser.yy.o: ../../../src/server/include/mdb_TTreeIter.h
$(DEBUGTARGETDIR)/parser.yy.o: ../include/sql_const.h
$(DEBUGTARGETDIR)/parser.yy.o: ../include/sql_parser.h
$(DEBUGTARGETDIR)/parser.yy.o: ../include/sql_util.h
$(DEBUGTARGETDIR)/parser.yy.o: ../include/sql_packet_format.h
$(DEBUGTARGETDIR)/parser.yy.o: ../include/sql_main.h
$(DEBUGTARGETDIR)/parser.yy.o: ../../../src/server/include/mdb_dbi.h
$(DEBUGTARGETDIR)/parser.yy.o: ../../../src/server/include/mdb_schema_cache.h
$(DEBUGTARGETDIR)/parser.yy.o: ../../../src/server/include/mdb_er.h
$(DEBUGTARGETDIR)/parser.yy.o: ../../../src/server/include/mdb_charset.h
$(DEBUGTARGETDIR)/parser.yy.o: ../../../src/server/include/mdb_PMEM.h
$(DEBUGTARGETDIR)/parser.yy.o: sql_calc_timefunc.h sql_func_timeutil.h
$(DEBUGTARGETDIR)/parser.yy.o: sql_mclient.h
$(DEBUGTARGETDIR)/parser.yy.o: sql_y.tab.c

$(RELEASETARGETDIR)/parser.ll.o: ../../../src/include/mdb_config.h
$(RELEASETARGETDIR)/parser.ll.o: ../../../src/include/sys_inc.h
$(RELEASETARGETDIR)/parser.ll.o: ../../../src/include/mdb_define.h
$(RELEASETARGETDIR)/parser.ll.o: ../../../src/include/ErrorCode.h
$(RELEASETARGETDIR)/parser.ll.o: ../../../src/include/isql.h
$(RELEASETARGETDIR)/parser.ll.o: ../../../src/server/include/mdb_FieldDesc.h
$(RELEASETARGETDIR)/parser.ll.o: ../../../src/include/mdb_typedef.h
$(RELEASETARGETDIR)/parser.ll.o: ../../../src/include/dbport.h
$(RELEASETARGETDIR)/parser.ll.o: ../../../src/server/include/mdb_FieldVal.h
$(RELEASETARGETDIR)/parser.ll.o: ../../../src/include/db_api.h
$(RELEASETARGETDIR)/parser.ll.o: ../../../src/server/include/mdb_KeyValue.h
$(RELEASETARGETDIR)/parser.ll.o: ../../../src/server/include/mdb_KeyDesc.h
$(RELEASETARGETDIR)/parser.ll.o: ../../../src/server/include/mdb_Filter.h
$(RELEASETARGETDIR)/parser.ll.o: ../../../src/include/systable.h
$(RELEASETARGETDIR)/parser.ll.o: ../include/sql_datast.h
$(RELEASETARGETDIR)/parser.ll.o: ../../../src/server/include/mdb_compat.h
$(RELEASETARGETDIR)/parser.ll.o: ../../../src/server/include/mdb_Csr.h
$(RELEASETARGETDIR)/parser.ll.o: ../../../src/server/include/mdb_ContIter.h
$(RELEASETARGETDIR)/parser.ll.o: ../../../src/server/include/mdb_Iterator.h
$(RELEASETARGETDIR)/parser.ll.o: ../../../src/server/include/mdb_Cont.h
$(RELEASETARGETDIR)/parser.ll.o: ../../../src/server/include/mdb_BaseCont.h
$(RELEASETARGETDIR)/parser.ll.o: ../../../src/server/include/mdb_inner_define.h
$(RELEASETARGETDIR)/parser.ll.o: ../../../src/server/include/mdb_Collect.h
$(RELEASETARGETDIR)/parser.ll.o: ../../../src/server/include/mdb_PageList.h
$(RELEASETARGETDIR)/parser.ll.o: ../../../src/server/include/mdb_Page.h
$(RELEASETARGETDIR)/parser.ll.o: ../../../src/server/include/mdb_TTree.h
$(RELEASETARGETDIR)/parser.ll.o: ../../../src/server/include/mdb_TTreeNode.h
$(RELEASETARGETDIR)/parser.ll.o: ../../../src/server/include/mdb_LockMgrPI.h
$(RELEASETARGETDIR)/parser.ll.o: ../../../src/server/include/mdb_ppthread.h
$(RELEASETARGETDIR)/parser.ll.o: ../../../src/server/include/mdb_Trans.h
$(RELEASETARGETDIR)/parser.ll.o: ../../../src/server/include/mdb_LSN.h
$(RELEASETARGETDIR)/parser.ll.o: ../../../src/server/include/mdb_TTreeIter.h
$(RELEASETARGETDIR)/parser.ll.o: ../include/sql_const.h
$(RELEASETARGETDIR)/parser.ll.o: ../include/sql_parser.h
$(RELEASETARGETDIR)/parser.ll.o: ../include/sql_util.h
$(RELEASETARGETDIR)/parser.ll.o: ../include/sql_packet_format.h
$(RELEASETARGETDIR)/parser.ll.o: ../include/sql_main.h
$(RELEASETARGETDIR)/parser.ll.o: ../../../src/server/include/mdb_dbi.h
$(RELEASETARGETDIR)/parser.ll.o: ../../../src/server/include/mdb_schema_cache.h
$(RELEASETARGETDIR)/parser.ll.o: ../../../src/server/include/mdb_er.h
$(RELEASETARGETDIR)/parser.ll.o: ../../../src/server/include/mdb_charset.h
$(RELEASETARGETDIR)/parser.ll.o: ../../../src/server/include/mdb_PMEM.h
$(RELEASETARGETDIR)/parser.ll.o: sql_calc_timefunc.h sql_func_timeutil.h
$(RELEASETARGETDIR)/parser.ll.o: sql_mclient.h
$(RELEASETARGETDIR)/parser.ll.o: sql_lex.yy.c

$(DEBUGTARGETDIR)/parser.ll.o: ../../../src/include/mdb_config.h
$(DEBUGTARGETDIR)/parser.ll.o: ../../../src/include/sys_inc.h
$(DEBUGTARGETDIR)/parser.ll.o: ../../../src/include/mdb_define.h
$(DEBUGTARGETDIR)/parser.ll.o: ../../../src/include/ErrorCode.h
$(DEBUGTARGETDIR)/parser.ll.o: ../../../src/include/isql.h
$(DEBUGTARGETDIR)/parser.ll.o: ../../../src/server/include/mdb_FieldDesc.h
$(DEBUGTARGETDIR)/parser.ll.o: ../../../src/include/mdb_typedef.h
$(DEBUGTARGETDIR)/parser.ll.o: ../../../src/include/dbport.h
$(DEBUGTARGETDIR)/parser.ll.o: ../../../src/server/include/mdb_FieldVal.h
$(DEBUGTARGETDIR)/parser.ll.o: ../../../src/include/db_api.h
$(DEBUGTARGETDIR)/parser.ll.o: ../../../src/server/include/mdb_KeyValue.h
$(DEBUGTARGETDIR)/parser.ll.o: ../../../src/server/include/mdb_KeyDesc.h
$(DEBUGTARGETDIR)/parser.ll.o: ../../../src/server/include/mdb_Filter.h
$(DEBUGTARGETDIR)/parser.ll.o: ../../../src/include/systable.h
$(DEBUGTARGETDIR)/parser.ll.o: ../include/sql_datast.h
$(DEBUGTARGETDIR)/parser.ll.o: ../../../src/server/include/mdb_compat.h
$(DEBUGTARGETDIR)/parser.ll.o: ../../../src/server/include/mdb_Csr.h
$(DEBUGTARGETDIR)/parser.ll.o: ../../../src/server/include/mdb_ContIter.h
$(DEBUGTARGETDIR)/parser.ll.o: ../../../src/server/include/mdb_Iterator.h
$(DEBUGTARGETDIR)/parser.ll.o: ../../../src/server/include/mdb_Cont.h
$(DEBUGTARGETDIR)/parser.ll.o: ../../../src/server/include/mdb_BaseCont.h
$(DEBUGTARGETDIR)/parser.ll.o: ../../../src/server/include/mdb_inner_define.h
$(DEBUGTARGETDIR)/parser.ll.o: ../../../src/server/include/mdb_Collect.h
$(DEBUGTARGETDIR)/parser.ll.o: ../../../src/server/include/mdb_PageList.h
$(DEBUGTARGETDIR)/parser.ll.o: ../../../src/server/include/mdb_Page.h
$(DEBUGTARGETDIR)/parser.ll.o: ../../../src/server/include/mdb_TTree.h
$(DEBUGTARGETDIR)/parser.ll.o: ../../../src/server/include/mdb_TTreeNode.h
$(DEBUGTARGETDIR)/parser.ll.o: ../../../src/server/include/mdb_LockMgrPI.h
$(DEBUGTARGETDIR)/parser.ll.o: ../../../src/server/include/mdb_ppthread.h
$(DEBUGTARGETDIR)/parser.ll.o: ../../../src/server/include/mdb_Trans.h
$(DEBUGTARGETDIR)/parser.ll.o: ../../../src/server/include/mdb_LSN.h
$(DEBUGTARGETDIR)/parser.ll.o: ../../../src/server/include/mdb_TTreeIter.h
$(DEBUGTARGETDIR)/parser.ll.o: ../include/sql_const.h
$(DEBUGTARGETDIR)/parser.ll.o: ../include/sql_parser.h
$(DEBUGTARGETDIR)/parser.ll.o: ../include/sql_util.h
$(DEBUGTARGETDIR)/parser.ll.o: ../include/sql_packet_format.h
$(DEBUGTARGETDIR)/parser.ll.o: ../include/sql_main.h
$(DEBUGTARGETDIR)/parser.ll.o: ../../../src/server/include/mdb_dbi.h
$(DEBUGTARGETDIR)/parser.ll.o: ../../../src/server/include/mdb_schema_cache.h
$(DEBUGTARGETDIR)/parser.ll.o: ../../../src/server/include/mdb_er.h
$(DEBUGTARGETDIR)/parser.ll.o: ../../../src/server/include/mdb_charset.h
$(DEBUGTARGETDIR)/parser.ll.o: ../../../src/server/include/mdb_PMEM.h
$(DEBUGTARGETDIR)/parser.ll.o: sql_calc_timefunc.h sql_func_timeutil.h
$(DEBUGTARGETDIR)/parser.ll.o: sql_mclient.h
$(DEBUGTARGETDIR)/parser.ll.o: sql_lex.yy.c

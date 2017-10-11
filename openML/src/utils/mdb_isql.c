/* 
   This file is part of openML, mobile and embedded DBMS.

   Copyright (C) 2012 Inervit Co., Ltd.
   support@inervit.com

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifndef WIN32
#include <unistd.h>
#include <signal.h>
#endif

#include "mdb_compat.h"
#include "isql.h"
#include "mdb_inner_define.h"

#include "../../version_mmdb.h"

#define MAX_DBHOSTNAME        32
#define MAX_DBNAME            MAX_DB_NAME
#define MAX_USERNAME          32
#define MAX_PASSWD            16
#define MAX_HEADLINE          80

#define SINGLE_QUOTE        200
#define DOUBLE_QUOTE        250

#if defined(sun) || defined(linux)

#include <errno.h>
#include <termios.h>
#include <sys/time.h>

static char *_def_editor = "/bin/ed";

#else

#include <windows.h>
#include <conio.h>      // For getch()
#include <io.h> // For _access()
#include <time.h>

#ifndef PATH_MAX
#define PATH_MAX            64
#endif

#ifndef PASS_MAX
#define PASS_MAX            16
#endif

#ifndef LOGNAME_MAX
#define LOGNAME_MAX        32
#endif

#ifndef F_OK
#define F_OK                0
#endif

#define access(A,B)        _access(A,B)

static char *_def_editor = "%WINDIR%\\notepad.exe";

#endif // defined(sun) || defined(linux)

#define MAX_LINE 1024

#if defined(linux) && !defined(ANDROID_OS)
extern __sighandler_t sigset(int __sig, __sighandler_t __disp);
#elif defined(linux) && defined(ANDROID_OS)
#define sigset signal
#endif

#if defined(HAVE_READLINE) && (HAVE_READLINE==1)
#include <readline/readline.h>
#include <readline/history.h>
#else /* HAVE_READLINE */
#define readline
#define add_history
#define add_history_query
#define read_history
#define write_history
#define stifle_history
#endif /* HAVE_READLINE */

static int _s_i_prompt_is_interactive = 0;
static char _s_sz_history_home[256];

#if !defined(sun) && !defined(linux)
static char *getpass(const char *str);
static char *getlogin(void);
#endif

static char *__isql_usage = "Usage: %s [OPTIONS] database";


static int parse_options(int ac, char **av,
        char *dbhost, char *dbname, char *dbuser,
        char *dbpasswd, char *start_sql_file);
static int xstrlcpy(char *dst, const char *src, unsigned long int dstsize);

static void print_resultset(iSQL * isql);
static int read_querystr(char *query);

static int find_command(char *query,
        int len, int start, char *old_query, int *action);
static int display_helpmessage(char *query, char *dummy);
static int exit_isql(char *query, char *dummy);
static int checkpoint(char *query, char *dummy);
static int save_querystr(char *query, char *file);
static int edit_querystr(char *query, char *dummy);
static int print_querystr(char *query, char *dummy);
static int run_shellcmd(char *dummy, char *shellcmd);
static int do_query_in_file(char *query, char *file);
static int init_query_in_file(char *query_file);

#if defined(HAVE_READLINE) && (HAVE_READLINE==1)
static int print_history(char *dummy, char *shellcmd);
static int add_history_query(char *query);
#endif
static int start_time(char *query, char *dummy);
static int end_time(char *query, char *dummy);

static void isql_end(int signo);

static int is_procedure_in_query(char ch, int step);

void set_timeinterval(T_DB_TIMEVAL *);
void get_timeinterval(T_DB_TIMEVAL *, char *);
char *set_next_query_str(char *query);

#ifdef CHECK_MALLOC_FREE
int print_mallocfree(char *dummy, char *size);
int sc_uplevel_mallocfree(void);
int sc_print_mallocfree(int level);
#endif

static struct
{
    const char *name;
    const char *short_name;
    int (*func) (char *, char *);
    int ret_val;
    int take_args;
    const char *description;
} isql_commands[] =
{
    {
    "edit", "ed", edit_querystr, 0, 0, "Edit the last SQL statement."},
    {
    "exit", NULL, NULL, -1, 0, "Exit isql."},
    {
    "quit", NULL, NULL, -1, 0, "Exit isql."},
    {
    "ctrl-c", NULL, exit_isql, 0, 0, "terminate isql."},
    {
    "checkpoint", NULL, checkpoint, 0, 0, "checkpointing."},
    {
    "help", NULL, display_helpmessage, 0, 0, "Display this help."},
    {
        "run", "r", print_querystr, 1,
#if defined(HAVE_READLINE) && (HAVE_READLINE==1)
                1,
#else
                0,
#endif
    "Run the last SQL statement."},
    {
    "save", "sa", save_querystr, 0, 1,
                "Append the last SQL statement to " "the specified file."},
    {
    "!", NULL, run_shellcmd, 0, 1,
                "Execute the specified system shell command."},
    {
    "@", NULL, do_query_in_file, 0, 1,
                "Runs the SQL statements in the specified " "command file."},
#ifdef CHECK_MALLOC_FREE
    {
    "printmem", "pm", print_mallocfree, 0, 1,
                "Print information of doing file."},
#endif
#if defined(HAVE_READLINE) && (HAVE_READLINE==1)
    {
    "history", "hs", print_history, 0, 0,
                "show query history (Only Linux)."},
#endif
    {
    "start-time", "st", start_time, 0, 0, "set time"},
    {
    "end-time", "et", end_time, 0, 0, "end time"},
    {
    NULL, NULL, NULL, 0, 0, NULL}
};

static char *pn;

static char __first_prompt[] = "isql>";
static char __second_prompt[] = "   ->";
static char __comment[] = "--";

static char query_filename[MAX_LINE];
static char __editor[PATH_MAX];

static iSQL isql;

static int use_query_in_file;

static int queryexec_flag;
static int interrupt_flag;
static FILE *script_fptr;
static char script_filename[PATH_MAX];

static int fflag = 0;
static int is_autocommit = 0;
static int is_use_result = 0;

static T_DB_TIMEVAL _time4cmd;

/*****************************************************************************/
//! xstrlcpy

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param dst(in/out) :
 * \param src(in) : 
 * \param dstsize(in):
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static int
xstrlcpy(char *dst, const char *src, unsigned long int dstsize)
{
    sc_strncpy(dst, src, dstsize);      /* do nothing */
    dst[dstsize - 1] = '\0';
    return sc_strlen(src);
}

/*****************************************************************************/
//! set_timeinterval 

/*! \breif set current time
 ************************************
 * \param tv(in/out) :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
void
set_timeinterval(T_DB_TIMEVAL * tv)
{
    sc_gettimeofday(tv, NULL);
}

/*****************************************************************************/
//! get_timeinterval 

/*! \breif  calculate the time-interval 
 ************************************
 * \param btv(in) : before time
 * \param disp(in) : messge 
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
void
get_timeinterval(T_DB_TIMEVAL * btv, char *disp)
{
    T_DB_TIMEVAL ctv;
    void display_timeinterval(T_DB_TIMEVAL *, char *);

    sc_gettimeofday(&ctv, NULL);

    if (ctv.tv_usec < btv->tv_usec)
    {
        btv->tv_usec = ctv.tv_usec + (1000000 - btv->tv_usec);
        btv->tv_sec = ctv.tv_sec - btv->tv_sec - 1;
    }
    else
    {
        btv->tv_usec = ctv.tv_usec - btv->tv_usec;
        btv->tv_sec = ctv.tv_sec - btv->tv_sec;
    }

    display_timeinterval(btv, disp);
}

/*****************************************************************************/
//! display_timeinterval 

/*! \breif  print the time-interval and message
 ************************************
 * \param tv(in) :
 * \param str(int) : 
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
void
display_timeinterval(T_DB_TIMEVAL * tv, char *str)
{
    if (str)
    {
        fprintf(stdout, "%s: ", str);
    }
    else
    {
        fprintf(stdout, "Elapsed: ");
    }

    fprintf(stdout, "%02ld:%02ld:%02ld.%06ld\n",
            tv->tv_sec / 3600, (tv->tv_sec / 60) % 60, tv->tv_sec % 60,
            tv->tv_usec);
}

#if defined(sun) || defined(linux)
static int modified = 0;
static struct termios oterm;
#endif


/*****************************************************************************
  *                 QUERY 입력 처리                                          *    
  ****************************************************************************/

/*****************************************************************************/
//! usage

/*! \breif  iSQL의 option을 출력하는 함수
 ************************************
 * \param pn(out) :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static void
usage(char *pn)
{
    fprintf(stdout, __isql_usage, pn);
    fprintf(stdout, "\n");
    fprintf(stdout, "-?: Display this help.\n");
    fprintf(stdout, "-h: Connect to host.\n");
    fprintf(stdout, "-u: User for login if not current user.\n");
    fprintf(stdout, "-p: Password to use on connecting to server.\n");
    fprintf(stdout, "-i: read and excute the query file.\n");
    fprintf(stdout, "-c: Off the AUTO COMMIT MODE\n");
    fprintf(stdout, "-r: use iSQL_use_result\n");
}

/*****************************************************************************/
//! parse_options 

/*! \breif  isql utility의 option을 처리하는 함수임
 ************************************
 * \param param_name :
 * \param param_name : 
 * \param param_name :
 * \param param_name :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘 및 기타 설명
 *  ISQL UTILITY'S OPTION
 *      -?: Display this help
 *      -h: Connect to host
 *      -u: User for login if not current user.
 *      -p: Password to use on connecting to server
 *      -s: Use shared memory
 *      -i: read and excute the query file.
 *      -k: encryption key
 *      -c: Off the AUTO COMMIT MODE
 *
 *  - AUTOCOMMIT을 OFF 시킬 수 있는 OPTION을 추가한다
 *****************************************************************************/
static int
parse_options(int ac, char **av,
        char *dbhost, char *dbname, char *dbuser,
        char *dbpasswd, char *start_sql_file)
{
#if !defined(WIN32_MOBILE)
    int c;
    extern int optind;
    extern char *optarg;
    char *p;
#else
    char *parg;
    int len;
#endif

    dbhost[0] = '\0';
    dbuser[0] = '\0';
    dbpasswd[0] = '\0';
    start_sql_file[0] = '\0';

#if defined(WIN32_MOBILE) || defined(WIN32)

    dbname[0] = 0x00;
    av++;
    ac--;
    while (ac > 0)
    {
        parg = *av;
        av++;
        ac--;

        if (*parg == '-')
        {
            parg++;
            if (*(parg + 1) != 0x00)
            {
                fprintf(stdout, "%s: illegal option -- %s\n", pn, parg);
                usage(pn);
                return 0;
            }

            if (*parg == '?')   // help
            {
                usage(pn);
                return 0;
            }
            else if (*parg == 'f')
                fflag++;
            else if (*parg == 'c')
                is_autocommit = 1;
            else if (*parg == 'r')
                is_use_result = 1;
            /* 옵션에 따른 파라메터가 필요한 경우 */
            else if (ac > 0)
            {   /*  여기서 ac == 0 인 경우는 옵션뒤에 데이타가 없다는 의미이다.  */
                len = 0;
                if (*parg == 'h')       // host
                {
                    len = MAX_DBHOSTNAME;
                    xstrlcpy(dbhost, *av, len);

                }
                else if (*parg == 'u')  // user
                {
                    len = MAX_USERNAME;
                    xstrlcpy(dbuser, *av, len);
                }
                else if (*parg == 'p')  // password
                {
                    len = MAX_PASSWD;
                    xstrlcpy(dbpasswd, *av, len);
                }
                else if (*parg == 'i')
                {
                    len = PATH_MAX;
                    xstrlcpy(start_sql_file, *av, len);
                }
                else
                {
                    fprintf(stdout, "%s: illegal option -- %c\n", pn, *parg);
                    usage(pn);
                    return 0;
                }


                if (sc_strlen(*av) >= len)
                {
                    fprintf(stdout, "%s: illegal option size -- %s\n", pn,
                            *av);
                    usage(pn);
                    return 0;
                }

                av++;
                ac--;
            }
            else
            {
                fprintf(stdout, "%s: illegal option -- %c\n", pn, *parg);
                usage(pn);
                return 0;
            }
        }   //if( *parg == '-' )
        else if (ac == 0)
            xstrlcpy(dbname, parg, MAX_DBNAME);
        else
        {
            usage(pn);
            return 0;
        }
    }

#else //#if defined(WIN32_MOBILE) || defined(WIN32)
    while ((c = getopt(ac, av, "i:h:u:p:f?cr")) != -1)
        switch (c)
        {
        case 'h':      // host
            sc_strcpy(dbhost, optarg);
            break;
        case 'u':      // user
            sc_strcpy(dbuser, optarg);
            break;
        case 'p':      // password
            sc_strcpy(dbpasswd, optarg);
            break;
        case '?':      // help
            usage(pn);
            return 0;
        case 'i':
            sc_strcpy(start_sql_file, optarg);
            break;
        case 'f':
            fflag++;
            break;
        case 'c':
            is_autocommit = 1;
            break;
        case 'r':
            is_use_result = 1;
            break;
        default:
            fprintf(stdout, "%s: illegal option -- %c\n", pn, c);
            usage(pn);
            return 0;
        }

    if (ac != optind + 1)
    {
        usage(pn);
        return 0;
    }
    else
        xstrlcpy(dbname, av[optind], MAX_DBNAME);
#endif

    if (dbuser[0] == '\0')
#if defined(WIN32_MOBILE)
        xstrlcpy(dbuser, "guest", MAX_USERNAME);
#else
        xstrlcpy(dbuser, (((p = getlogin()))? p : "guest"), MAX_USERNAME);
#endif
    if (dbpasswd[0] == '\0')
#if defined(WIN32_MOBILE) || defined(ANDROID_OS)
        xstrlcpy(dbpasswd, "guest", MAX_PASSWD);
#else
    {
        if (!(p = getpass("Enter password:")))
        {
            perror("ERROR");
            return 0;
        }
        else
        {
            xstrlcpy(dbpasswd, p, MAX_PASSWD);
#ifdef CHECK_MALLOC_FREE
            free(p);
#else
            sc_free(p);
#endif
        }
    }
#endif

    if (dbhost[0] == '\0')
        xstrlcpy(dbhost, "127.0.0.1", MAX_DBHOSTNAME);


    return 1;
}



/*****************************************************************************/
//! find_command

/*! \breif  iSQL 수행중 command가 들어간 경우 이를 처리함 
 ************************************
 * \param query() :
 * \param len() : 
 * \param start():
 * \param old_query() :
 * \param action() :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static int
find_command(char *query, int len, int start, char *old_query, int *action)
{
    int i, j;
    int cmd_len, ret = 1;

    *action = 0xFF;

    for (i = 0; isql_commands[i].name != NULL; i++)
    {
        cmd_len = 0;
        if (!sc_strncasecmp(isql_commands[i].name,
                        query + start, sc_strlen(isql_commands[i].name)))
            cmd_len = sc_strlen(isql_commands[i].name);
        else if (isql_commands[i].short_name != NULL &&
                !sc_strncasecmp(isql_commands[i].short_name,
                        query + start, sc_strlen(isql_commands[i].short_name)))
            cmd_len = sc_strlen(isql_commands[i].short_name);

        if (cmd_len)
        {
            for (j = start + cmd_len; j < len; j++)
                if (query[j] != ' ' && query[j] != '\t'
                        && query[j] != '\r' && query[j] != '\n')
                    break;
            if (query[j] == ';')
            {
                ++j;
                *action = -1;   // means "break"
            }


            if (isql_commands[i].take_args)
            {
#if defined(HAVE_READLINE) && (HAVE_READLINE==1)
                if (isql_commands[i].short_name != NULL
                        && sc_strncmp("r", isql_commands[i].short_name,
                                cmd_len) == 0 && cmd_len != len)
                {
                    if (query[start + cmd_len] != ' ')
                    {
                        break;
                    }
                }

                *action = 1;    // means "return"
#else
                *action = 1;    // means "return"
#endif
                if (isql_commands[i].func)
#if defined(HAVE_READLINE) && (HAVE_READLINE==1)
                {
                    ret = isql_commands[i].func(old_query, &query[j]);
                    break;
                }
#else
                    ret = isql_commands[i].func(old_query, &query[j]);
#endif
                else
                    ret = isql_commands[i].ret_val;
            }
            else
            {
                if (j < len)
                {
                    if (*action == 0xFF)
                        *action = 0;    // means "continue"
                }
                else
                {
                    *action = 1;        // means "return"
                    if (isql_commands[i].func)
#if defined(HAVE_READLINE) && (HAVE_READLINE==1)
                    {
                        ret = isql_commands[i].func(old_query, NULL);
                        break;
                    }
#else
                        ret = isql_commands[i].func(old_query, NULL);
#endif
                    else
                        ret = isql_commands[i].ret_val;
                }
            }
        }
    }
    if (*action == 0xFF)
        *action = 0;    // means "continue"

    return ret;
}

#if !defined(sun) && !defined(linux)
/*****************************************************************************/
//! getpass 

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param str():
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static char *
getpass(const char *str)
{
    int i, ch;
    static char pwd[PASS_MAX];

    fprintf(stdout, "%s", str);

    for (i = 0;
            i < PASS_MAX - 1 &&
            (ch = getch()) != EOF && ch != '\r' && ch != '\n'; i++)
    {
        fprintf(stdout, "*");
        pwd[i] = (char) ch;
    }
    pwd[i] = '\0';
    fprintf(stdout, "\n");

    return pwd;
}

/*****************************************************************************/
//! getlogin 

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param void :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static char *
getlogin(void)
{
    static char username[LOGNAME_MAX];
    DWORD dwsize;

    dwsize = LOGNAME_MAX;
    ZeroMemory(username, LOGNAME_MAX);
    GetUserName(username, &dwsize);

    return username;
}
#endif // !defined(sun) && !defined(linux)

/*****************************************************************************/
//! function_name 

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param param_name :
 * \param param_name : 
 * \param param_name :
 * \param param_name :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static int
save_querystr(char *query, char *query_file)
{
    FILE *fptr;
    char filename[PATH_MAX];
    int i, ret = -1;
    int len = sc_strlen(query_file);

    if (!query[0])
    {
        fprintf(stdout, "\nERROR: Nothing to save\n\n");
        return 0;
    }

    for (i = 0; i < len; i++)
        if (query_file[i] != ' ' && query_file[i] != '\t'
                && query_file[i] != '\r' && query_file[i] != '\n')
            break;
    if (i == len || query_file[i] == ';')
        sc_strcpy(filename, "mobilelite.sql");
    else
    {
        xstrlcpy(filename, query_file, PATH_MAX);
        for (i = 0; i < PATH_MAX && filename[i]; i++)
            if (filename[i] == ' ' || filename[i] == '\t' ||
                    filename[i] == '\r' || filename[i] == '\n' ||
                    filename[i] == ';')
            {
                filename[i] = '\0';
                break;
            }
    }

    if (access(filename, F_OK) < 0)
        fprintf(stdout, "Created query file %s\n", filename);
    else
        fprintf(stdout, "Appended query file %s\n", filename);

    fptr = fopen(filename, "a");
    if (fptr == NULL)
    {
        fprintf(stdout, "\nERROR: %s\n\n", sc_strerror(errno));
        goto RETURN_LABEL;
    }

    fprintf(fptr, "%s;\n", query);

    ret = 0;

  RETURN_LABEL:
    if (fptr)
        fclose(fptr);

    return ret;
}

/*****************************************************************************/
//! print_querystr

/*! \breif  QUERY 문장을 출력하는 함수 
 ************************************
 * \param query(in) :
 * \param dummy(): 
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static int
print_querystr(char *query, char *dummy)
{
    int i, pos;


#if defined(HAVE_READLINE) && (HAVE_READLINE==1)
    if (dummy)
    {
        int offset = 0;

        offset = sc_atoi(dummy);

        if (offset > 0)
        {
            HIST_ENTRY *his;

            his = history_get(offset - 1 + history_base);

            if (his)
            {
                sc_strcpy(query, his->line);
            }
        }
    }
    else if (!query[0] && !dummy)
    {
        return 1;
    }
#else
    if (!query[0])
        return 1;
#endif

    for (i = 0, pos = 0; query[pos]; i++)
    {
        fprintf(stdout, "%4d> ", i + 1);
        --pos;
        do
            fputc(query[++pos], stdout);
        while (query[pos] && query[pos] != '\r' && query[pos] != '\n');
        if (query[pos] == '\r')
            ++pos;      // For windows
        if (query[pos] == '\n')
            ++pos;
    }
    if (query[pos] == '\n')
        query[pos - 1] = '\0';  // cut last '\n'
    else
        fputc('\n', stdout);

    add_history_query(query);

    return 1;
}

/*****************************************************************************/
//! display_helpmessage 

/*! \breif  help message를 화면에 출력
 ************************************
 * \param dummy1() :
 * \param dummy2() : 
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static int
display_helpmessage(char *dummy1, char *dummy2)
{
    int i, dcnt;

    fprintf(stdout, "\nList of all commands:\n");
    for (i = 0; isql_commands[i].name != NULL; i++)
    {
        if (isql_commands[i].short_name)
        {
            dcnt = fprintf(stdout, "%s(%s)",
                    isql_commands[i].name, isql_commands[i].short_name);
            fprintf(stdout, "%c\t%s",
                    (dcnt < 8) ? '\t' : '\0', isql_commands[i].description);
        }
        else
        {
            dcnt = fprintf(stdout, "%s", isql_commands[i].name);
            fprintf(stdout, "%c\t%s",
                    (dcnt < 8) ? '\t' : '\0', isql_commands[i].description);
        }
        fprintf(stdout, "\n");
    }
    fprintf(stdout, "\n");

    return 0;
}

static int
exit_isql(char *dummy1, char *dummy2)
{
    fprintf(stdout, "Program is terminated~~!\n");
    exit(0);

    return 0;
}

static int
checkpoint(char *dummy1, char *dummy2)
{
    T_DB_TIMEVAL tv = { 0, 0 };

    fprintf(stdout, "Checkpointing...\n");
    if (isql.flags & OPT_TIME)
        set_timeinterval(&tv);

    iSQL_checkpoint(&isql);

    if (isql.flags & OPT_TIME)
    {
        get_timeinterval(&tv, NULL);
        fprintf(stdout, "\n");
    }

    return 0;
}

/*****************************************************************************/
//! edit_querystr 

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param query() :
 * \param dummy() : 
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static int
edit_querystr(char *query, char *dummy)
{
    FILE *fptr;

    char *p = NULL, sh_cmd[128];
    int i, ret, len, orig_len = sc_strlen(query);
    int insert_newline = 0;

    // replaced tempnam with mktemp to resolve
    // tempnam warning
    if (strlen(query_filename) == 0)
    {
        strcpy(query_filename, "isql_XXXXXX");
        if (mktemp(query_filename) == NULL)
        {
            fprintf(stdout,
                    "\nERROR: in edit_querystr(): mktemp(query_filename=\"isql_XXXXXX\") failed.\n\n");
            return -1;
        }

#if !defined(WIN32_MOBILE)
        p = sc_getenv("EDITOR");
#endif
        if (!p)
            p = _def_editor;
        xstrlcpy(__editor, p, PATH_MAX);
    }

    fptr = fopen(query_filename, "w");
    if (!fptr)
    {
        fprintf(stdout, "\nERROR: %s\n\n", sc_strerror(errno));
        memset(query_filename, 0, sizeof(query_filename));
        return -1;
    }

    if (orig_len && !fwrite(query, 1, orig_len, fptr))
    {
        fprintf(stdout, "\nERROR: %s\n\n", sc_strerror(errno));
        ret = -1;
        goto RETURN_LABEL;
    }

    if (orig_len && query[orig_len - 1] != '\n')
    {       // For "/bin/ed"
        insert_newline = 1;
        if (!fwrite("\n", 1, 1, fptr))
        {
            fprintf(stdout, "\nERROR: %s\n\n", sc_strerror(errno));
            ret = -1;
            goto RETURN_LABEL;
        }
    }

    fclose(fptr);

    sc_snprintf(sh_cmd, sizeof(sh_cmd), "%s %s", __editor, query_filename);

    system(sh_cmd);

    fptr = fopen(query_filename, "r");
    if (!fptr)
    {
        fprintf(stdout, "\nERROR: %s\n\n", sc_strerror(errno));
        return -1;
    }

    len = fread(query, 1, iSQL_QUERY_MAX - 1, fptr);
    if (insert_newline && len > 0)
        --len;
    query[len] = '\0';

    for (i = 0; i < len; i++)
        if (query[i] != ' ' && query[i] != '\t' &&
                query[i] != '\r' && query[i] != '\n')
            break;
    for (i = len - 1; i >= 0; --i)
        if (query[i] != ' ' && query[i] != '\t' &&
                query[i] != '\r' && query[i] != '\n')
            break;
    if (query[i] == ';' || query[i] == '/')
        query[i] = '\0';

    fprintf(stdout, "\n");
    print_querystr(query, dummy);

    ret = 0;

  RETURN_LABEL:

    fclose(fptr);

    return ret;
}

/*****************************************************************************/
//! run_shellcmd

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param dummy() :
 * \param shellcmd(): 
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static int
run_shellcmd(char *dummy, char *shellcmd)
{
    int ret;

    ret = system(shellcmd);
    if (ret < 0)
        fprintf(stdout, "\nERROR: %s\n\n", sc_strerror(errno));
    else
        ret = 0;

    return ret;
}

#if defined(HAVE_READLINE) && (HAVE_READLINE==1)
static int
print_history(char *dummy, char *shellcmd)
{
    if (_s_i_prompt_is_interactive)
    {
        register HIST_ENTRY **list;
        register int i;

        list = history_list();

        if (list)
        {
            for (i = 0; list[i]; i++)
            {
                printf(" %d  %s\n", i + 1, list[i]->line);
            }
        }
    }

    return 0;
}

static int
add_history_query(char *sz_query)
{
    if (_s_i_prompt_is_interactive)
    {
        /* for one line string */
        int n_len, i;
        char sz_history[iSQL_QUERY_MAX + 1];

        sc_memset(sz_history, 0x00, sizeof(sz_history));

        n_len = sc_strlen(sz_query);
        sc_strncpy(sz_history, sz_query, n_len);

        for (i = 0; i < n_len; i++)
        {
            if (sz_history[i] == '\n' || sz_history[i] == '\t'
                    || sz_history[i] == '\r')
            {
                sz_history[i] = ' ';
            }
        }
        add_history(sz_history);
    }

    return 1;
}
#endif

static int
start_time(char *query, char *dummy)
{
    query[0] = '\0';
    set_timeinterval(&_time4cmd);

    return 0;
}

static int
end_time(char *query, char *dummy)
{
    T_DB_TIMEVAL _tv;

    sc_memcpy(&_tv, &_time4cmd, sizeof(T_DB_TIMEVAL));
    get_timeinterval(&_tv, "Elapsed Time");
    fprintf(stdout, "\n");
    query[0] = '\0';

    return 0;
}

#ifdef CHECK_MALLOC_FREE
int
print_mallocfree(char *dummy, char *level)
{
    if (sc_strncmp(level, "up", 2) == 0)
    {
        sc_uplevel_mallocfree();
    }
    else if (sc_strlen(level) == 0)
    {
        sc_print_mallocfree(-1);
    }
    else
    {
        sc_print_mallocfree(sc_atoi(level));
    }
    return 0;
}
#endif
/*****************************************************************************/
//! init_query_in_file

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param query_file(in) : query file name
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static int
init_query_in_file(char *query_file)
{
    char buf[PATH_MAX];
    int i;

    if (!use_query_in_file)
    {
        if (query_file[0] == '\0')
            return 0;
        xstrlcpy(script_filename, query_file, PATH_MAX);
        for (i = 0; i < PATH_MAX && script_filename[i]; i++)
            if (script_filename[i] == ' ' || script_filename[i] == '\t' ||
                    script_filename[i] == '\r' || script_filename[i] == '\n' ||
                    script_filename[i] == ';')
            {
                script_filename[i] = '\0';
                break;
            }
        if (!access(script_filename, F_OK))
            script_fptr = fopen(script_filename, "r");
        else
        {
            if (sc_strchr(script_filename, '.'))
            {
                fprintf(stdout, "\nERROR: unable to open file \"%s\"\n\n",
                        script_filename);
                return 0;
            }

            sc_snprintf(buf, PATH_MAX, "%s.sql", script_filename);
            if (!access(buf, F_OK))
                script_fptr = fopen(buf, "r");
            else
            {
                fprintf(stdout, "\nERROR: unable to open file \"%s\"\n\n",
                        buf);
                return 0;
            }
        }
        if (script_fptr == NULL)
        {
            fprintf(stdout, "\nERROR: %s\n\n", sc_strerror(errno));
            return 0;
        }
        use_query_in_file = 1;
    }

    return 1;
}


/*****************************************************************************/
//! isql_end 

/*! \breif  isql이 종료되는 경우 호출되는 함수 
 ************************************
 * \param signo(in) :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static void
isql_end(int signo)
{
    if (!signo)
    {
        fprintf(stdout, "Bye~~!\n");
        iSQL_disconnect(&isql);
    }
    else if (signo > 0)
        fprintf(stdout, "Interrupted!!!\n");

    if (strlen(query_filename) != 0)
    {
        unlink(query_filename);
        memset(query_filename, 0, sizeof(query_filename));
    }

#if defined(sun) || defined(linux)
    if (modified)
        tcsetattr(0, TCSANOW, &oterm);
#endif

    exit(signo ? -1 : 0);
}

/*****************************************************************************
  *                                                   QUERY 입력 처리                                                        *
  ****************************************************************************/

/*****************************************************************************/
//! set_next_query_str

/*! \breif    isql shell에서 입력 받은 여러개의 쿼리문장을 하나씩 
 *            끊어주는 역할을 수행한다
 ************************************
 * \param query(in) :  
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *            '/'나 ';'가 나온 후 다음 쿼리 문장의 앞부분의 쓸때없는 공백을
 *            제거해준다
 *****************************************************************************/
char *
set_next_query_str(char *query)
{
    int pos;
    int step = 0;
    char ch = '\0';

    pos = 0;

    while (1)
    {
        ch = query[pos];
        step = is_procedure_in_query(ch, step);

        if (ch == ';')
        {
            if (step != SINGLE_QUOTE && step != DOUBLE_QUOTE)
            {
                query[pos] = '\0';
                ++pos;
                break;
            }
        }
        else if (ch == '\0')
            break;
        ++pos;
    }

    if (query[pos] == '\0')
        return NULL;

    while (query[pos] == '\n' || query[pos] == ' ')
        ++pos;

    return (query + pos);
}


static int
is_procedure_in_query(char ch, int step)
{
    switch (step)
    {
    case 0:
        if (ch == '\'')
            return SINGLE_QUOTE;
        else if (ch == '"')
            return DOUBLE_QUOTE;
        else
            return 0;
    case SINGLE_QUOTE:
        if (ch == '\'')
            return 0;
        else
            return SINGLE_QUOTE;
    case DOUBLE_QUOTE:
        if (ch == '"')
            return 0;
        else
            return DOUBLE_QUOTE;
    default:
        return 0;
    }
}


/*****************************************************************************/
//! do_query_in_file

/*! \breif  파일에서 query 문을 읽어 오는 함수
 ************************************
 * \param query(out)    : 파일에서 읽은 query
 * \param query_file(in): 쿼리가 저장되어 있는 파일 이름 
 ************************************
 * \return  return_value
 ************************************
 * \note     1. 파일의 이름을 읽어옴                \n
 *            2. 한문자씩 읽어서 적절하게 파싱    \n
 *            3. query에 내용을 넣음                
 *****************************************************************************/
static int
do_query_in_file(char *query, char *query_file)
{
    char buf[PATH_MAX];
    int ch, i;
    int step;
    int is_started;
    int end_of_query;

    if (!use_query_in_file)
    {
        if (query_file[0] == '\0')
            return 0;
        xstrlcpy(script_filename, query_file, PATH_MAX);
        for (i = 0; i < PATH_MAX && script_filename[i]; i++)
            if (script_filename[i] == ' ' || script_filename[i] == '\t' ||
                    script_filename[i] == '\r' || script_filename[i] == '\n' ||
                    script_filename[i] == ';')
            {
                script_filename[i] = '\0';
                break;
            }
        if (!access(script_filename, F_OK))
            script_fptr = fopen(script_filename, "r");
        else
        {
            if (sc_strchr(script_filename, '.'))
            {
                fprintf(stdout, "\nERROR: unable to open file \"%s\"\n\n",
                        script_filename);
                return 0;
            }

            sc_snprintf(buf, PATH_MAX, "%s.sql", script_filename);
            if (!access(buf, F_OK))
                script_fptr = fopen(buf, "r");
            else
            {
                fprintf(stdout, "\nERROR: unable to open file \"%s\"\n\n",
                        buf);
                return 0;
            }
        }
        if (script_fptr == NULL)
        {
            fprintf(stdout, "\nERROR: %s\n\n", sc_strerror(errno));
            return 0;
        }
        use_query_in_file = 1;
    }

    step = 0;
    is_started = 0;
    end_of_query = 0;

    // 한 문자씩 읽어서 파싱
    for (i = 0; (ch = fgetc(script_fptr)) != EOF; i++)
    {
        if (i >= iSQL_QUERY_MAX)
        {
            fprintf(stdout, "\nERROR: A query size is too large.\n\n");
            fprintf(stdout, "[%.*s]", i, query);
            break;
        }

        if (!is_started)
        {
            if (ch == ' ' || ch == '\t' ||
                    ch == '\r' || ch == '\n' || ch == ';')
            {
                --i;
                continue;
            }
            else
                is_started = 1;
        }

        // automata에서 알맞은 step으로 jump
        step = is_procedure_in_query((char) ch, step);

        // 특수 문자일때 처리
        if (ch == ';')
        {   // 일상적인 쿼리일때
            query[i] = ch;
            if (step != SINGLE_QUOTE && step != DOUBLE_QUOTE)
            {
                end_of_query = 1;
                break;
            }
        }
        else if (ch == '/')
        {
            query[i] = ch;
        }
        else if (ch == '-')
        {   // check comment 
            query[i] = ch;
            ch = fgetc(script_fptr);
            if (ch == EOF)
                break;
            if (ch == '-')
            {
                while ((ch = fgetc(script_fptr)) != '\n' && ch != EOF);
                if (ch == EOF)
                    break;
                --i;
            }
            else
            {
                if (++i >= iSQL_QUERY_MAX)
                {
                    fprintf(stdout, "\nERROR: A query size is too large.\n\n");
                    fprintf(stdout, "[%.*s]", i, query);
                    break;
                }
                query[i] = ch;
            }
        }
        else
            query[i] = ch;
    }

    if (end_of_query)
    {
        ++i;
        query[i] = '\0';
    }
    else
    {
        fclose(script_fptr);
        script_fptr = NULL;
        use_query_in_file = 0;
        return 0;
    }

    add_history_query(query);

    if (1)
    {
        char *find_quit = query;

        while (*find_quit != '\0')
        {
            if (sc_isspace((int) (*find_quit)))
                find_quit++;
            else
                break;

        }

        if (sc_strncasecmp(find_quit, "quit", 4) == 0 ||
                sc_strncasecmp(find_quit, "exit", 4) == 0)
        {
            fclose(script_fptr);
            script_fptr = NULL;
            use_query_in_file = 0;
            return -1;
        }
    }

    return 1;
}

/*****************************************************************************/
//! read_querystr

/*! \breif  isql shell에서 쿼리를 입력받을 때 사용하는 함수
 ************************************
 * \param query(out) : query for execute
 ************************************
 * \return  return_value
 ************************************
 * \note isql shell에서 읽어들이는 내용과 파일에서 읽어오는            \n
 *         루틴이 다르게 되어 있었으나, 최대한 비슷하게 만들었다        \n
 *            \n\n이 왔을때에 문장을 무시하도록 만들어준다
 *     len이 0인 경우, query의 값을 없애주어야.. 
 *     찌꺼기 값이 사라진다 
 *****************************************************************************/
static int
read_querystr(char *query)
{
    char *tmp_query;
    int len = 0, blen;
    int chk_cmd = 0, action;
    int i = 0, ret;
    int first_pos = -1;
    int step = 0;               // for automata
    char ch;
    int enter_cnt = 0;

    if (use_query_in_file)
    {
        ret = do_query_in_file(query, NULL);
        if (ret != 0)
        {
            fprintf(stdout, "%s ", __first_prompt);
            return ret;
        }
    }

    tmp_query = (char *) sc_malloc(iSQL_QUERY_MAX + 1);
    if (tmp_query == NULL)
        return -1;

    sc_memset(tmp_query, 0x00, iSQL_QUERY_MAX + 1);

    while (1)
    {
        // print the prompt
      START_QUERY:
        if (first_pos == -1)
        {
            sc_memset(tmp_query, 0, iSQL_QUERY_MAX + 1);
            if (!_s_i_prompt_is_interactive)
            {
                fprintf(stdout, "%s ", __first_prompt);
            }
            step = 0;
            ret = 0;
            blen = 0;
            len = 0;
            ch = '\0';
            first_pos = -1;
            ret = 0;
            chk_cmd = 0;
            enter_cnt = 0;
        }
        else
        {
            if (!_s_i_prompt_is_interactive)
            {
                fprintf(stdout, "%s ", __second_prompt);
            }
        }

        fflush(stdout);

        // read the input
#if defined(HAVE_READLINE) && (HAVE_READLINE==1)
        if (_s_i_prompt_is_interactive)
        {
            char *zResult = NULL;
            char *zPrompt = NULL;

            if (tmp_query && tmp_query[0])
            {
                zPrompt = __second_prompt;
                sc_strncpy(tmp_query + len, "\n", 1);
                len++;
            }
            else
            {
                zPrompt = __first_prompt;
            }

            zResult = readline(zPrompt);

            if (zResult && *zResult)
            {
                sc_strncpy(tmp_query + len, zResult, sc_strlen(zResult));
            }

            if (zResult)
            {
                sc_free(zResult);
                zResult = NULL;
            }
        }
        else
        {
#endif
            if (fgets(tmp_query + len, (iSQL_QUERY_MAX + 1) - len,
                            stdin) == NULL)
            {
                sc_free(tmp_query);
                if (len != 0)
                    return -2;
                else
                    return -1;
            }
#if defined(HAVE_READLINE) && (HAVE_READLINE==1)
        }
#endif

        // "--"가 들어오면 shell이 종료된다
        if (!sc_strncmp(tmp_query + len, __comment, sc_strlen(__comment)))
            break;

        blen = len;
        len += sc_strlen(tmp_query + len);

        if (len == iSQL_QUERY_MAX)
        {
            sc_free(tmp_query);
            return -2;
        }

        if (tmp_query[blen] == '\n')
        {
            if (enter_cnt == 1)
                first_pos = -1;
            else
                ++enter_cnt;
            goto START_QUERY;
        }
        enter_cnt = 0;

        for (i = blen; i < len; ++i)
        {
            ch = *(tmp_query + i);
            if (first_pos < 0)
            {
                if (ch == '\n')
                {
                    first_pos = -1;
                    goto START_QUERY;
                }
                if (ch != ' ' && ch != '\r' && ch != '\n')
                {
                    first_pos = i;
                    chk_cmd = 1;
                }
            }


            step = is_procedure_in_query(ch, step);
            // '/n' '/n'이 중첩되면 끝나게끔 한다.. 
            // is-procedure_in_query()를 지운다
        };

        // 먼저 커맨드 체크를 한다              
        if (chk_cmd)
        {   // command를 check 하는 곳
            chk_cmd = 0;

            ret = find_command(tmp_query, len, first_pos, query, &action);
            if (action == 1)    // means return
            {
                sc_free(tmp_query);
                return ret;
            }
            else if (action != 0)
                break;
        }

        // 문장의 끝 단어부터 비교한다
        for (i = len - 1; i >= blen; --i)
        {
            ch = *(tmp_query + i);
            if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r')
                continue;

            if (ch == ';')
            {
                if (step != SINGLE_QUOTE && step != DOUBLE_QUOTE)
                    goto END_OF_QUERY;
            }
            break;
        }
    }

  END_OF_QUERY:

    if (len == 0)
        query[0] = 0x00;
    else
        sc_memcpy(query, tmp_query, len);

    if (len < iSQL_QUERY_MAX)
        query[len] = 0x00;

    for (; i < len; ++i)
    {
        if (query[i] == '\n' || query[i] == '\t' || query[i] == '\r'
                || query[i] == ' ')
        {
            query[i] = '\0';
        }
    }
    sc_free(tmp_query);

    add_history_query(query);

    return len;
}


/*****************************************************************************
  *     QUERY 결과값 처리                                                    *
  ****************************************************************************/

/*****************************************************************************/
//! print_resultset 

/*! \breif  QUERY의 결과값을 출력하는 함수 
 ************************************
 * \param isql(in) :
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *  - BYTE/VARBYTE TYPE을 받아오도록 수정함
 *****************************************************************************/
static void
print_resultset(iSQL * isql)
{
    iSQL_RES *res;
    iSQL_ROW row;
    iSQL_FIELD **fields = NULL;

    char disp_name[32];
    DB_INT32 i, j, num_fields;
    DB_UINT32 k;

#ifndef WIN32
    char *pager_cmd;
#endif
    FILE *isql_fp;
    char *plan_string = NULL;
    int headline_count;
    int sql_type;
    char upper_name[FIELD_NAME_LENG];

    fflush(stdout);
    if (!(isql->flags & OPT_PLAN_OFF))
    {
        plan_string = iSQL_get_plan_string(isql);
        if (plan_string == NULL)
        {
            fprintf(stdout, "\nERROR: %s\n\n", iSQL_error(isql));
            return;
        }
        else
        {
            fprintf(stdout, "\n=========================================");
            fprintf(stdout, "\n               QUERY PLAN                ");
            fprintf(stdout, "\n=========================================");
            fprintf(stdout, "%s", plan_string);
            fflush(stdout);
            sc_free(plan_string);

            if (isql->flags & OPT_PLAN_ONLY)
            {
                if (iSQL_query_end(isql) < 0)
                    fprintf(stdout, "\nERROR: %s\n\n", iSQL_error(isql));
                return;
            }
        }
    }
    sql_type = iSQL_last_querytype(isql);
    if (sql_type == SQL_STMT_UPDATE || sql_type == SQL_STMT_DELETE)
        return;
    if (is_use_result == 1)
        res = iSQL_use_result(isql);
    else
        res = iSQL_store_result(isql);
    if (!res)
    {
        fprintf(stdout, "\nERROR: %s\n\n", iSQL_error(isql));
        return;
    }

    num_fields = iSQL_num_fields(res);
    fields = sc_malloc(sizeof(iSQL_FIELD *) * num_fields);
    if (!fields)
    {
        perror("ERROR");
        goto RETURN_LABEL;
    }
    for (i = 0; i < num_fields; i++)
    {
        fields[i] = iSQL_fetch_field(res);
        if (!fields[i])
        {
            perror("ERROR");
            goto RETURN_LABEL;
        }
    }

    if (!(isql->flags & OPT_HEADING))
        fprintf(stdout, "\n");

    /* PAGER가 설정 안되어 있는 경우 pager_cmd 사용하지 않게 */
#ifdef WIN32
    isql_fp = stdout;
#else
    pager_cmd = sc_getenv("PAGER");
    if (pager_cmd == NULL)
        isql_fp = stdout;
    else
    {
        pager_cmd = "more";
        isql_fp = popen(pager_cmd, "w");
        if (isql_fp == NULL)
            isql_fp = stdout;
    }
#endif

    for (i = 0; (row = iSQL_fetch_row(res)) != NULL; i++)
    {
        if (isql->flags & OPT_HEADING && !(i % 10))
        {
            fprintf(isql_fp, "\n");
            for (j = 0; j < num_fields; j++)
            {
                switch (iSQL_get_fieldtype(res, j))
                {
                case SQL_DATA_CHAR:    // char[]
                case SQL_DATA_DATE:    // date
                case SQL_DATA_TIME:    // time
                case SQL_DATA_TIMESTAMP:       // timestamp
                case SQL_DATA_DATETIME:        // datetime
                    sc_snprintf(disp_name, 32,
                            "%%-%d.*s", fields[j]->max_length);
                    break;
                case SQL_DATA_VARCHAR:
                    sc_snprintf(disp_name, 32,
                            "%%-%d.*s",
                            (fields[j]->max_length <
                                    MAX_HEADLINE) ? fields[j]->max_length :
                            MAX_HEADLINE);
                    break;
                case SQL_DATA_NCHAR:
                    sc_snprintf(disp_name, 32,
                            "%%-%d.*s",
                            fields[j]->max_length * sizeof(DB_WCHAR));
                    break;
                case SQL_DATA_NVARCHAR:        // varchar
                    sc_snprintf(disp_name, 32,
                            "%%-%d.*s",
                            (sizeof(DB_WCHAR) * fields[j]->max_length <
                                    MAX_HEADLINE) ? (sizeof(DB_WCHAR) *
                                    fields[j]->max_length) : MAX_HEADLINE);
                    break;
                case SQL_DATA_BYTE:
                    sc_snprintf(disp_name, 32,
                            "%%-%d.*s", fields[j]->max_length);
                    break;
                case SQL_DATA_VARBYTE:
                    sc_snprintf(disp_name, 32,
                            "%%-%d.*s",
                            (fields[j]->max_length <
                                    MAX_HEADLINE) ? fields[j]->max_length :
                            MAX_HEADLINE);
                    break;
                default:
                    sc_snprintf(disp_name, 32,
                            "%%%d.*s", fields[j]->max_length);
                    break;
                }

                sc_memset(upper_name, 0, FIELD_NAME_LENG);
                sc_strupper(upper_name, fields[j]->name);
                fprintf(isql_fp, disp_name, fields[j]->max_length, upper_name);
                if (j + 1 < num_fields)
                    fprintf(isql_fp, " ");
            }

            fprintf(isql_fp, "\n");
            for (j = 0; j < num_fields; j++)
            {
                if (iSQL_get_fieldtype(res, j) == SQL_DATA_VARCHAR)
                    headline_count = (fields[j]->max_length < MAX_HEADLINE) ?
                            fields[j]->max_length : MAX_HEADLINE;
                else if (iSQL_get_fieldtype(res, j) == SQL_DATA_NCHAR)
                    headline_count = sizeof(DB_WCHAR) * fields[j]->max_length;
                else if (iSQL_get_fieldtype(res, j) == SQL_DATA_NVARCHAR)
                    headline_count =
                            (sizeof(DB_WCHAR) * fields[j]->max_length <
                            MAX_HEADLINE) ? (sizeof(DB_WCHAR) *
                            fields[j]->max_length) : MAX_HEADLINE;
                else
                    headline_count = fields[j]->max_length;

                for (k = 0; k < (DB_UINT32) headline_count; k++)
                    fputc('-', isql_fp);

                if (j + 1 < num_fields)
                    fprintf(isql_fp, " ");
            }
            fprintf(isql_fp, "\n");
        }   // !(i%10)

        for (j = 0; j < num_fields; j++)
        {
            switch (iSQL_get_fieldtype(res, j))
            {
            case SQL_DATA_CHAR:
            case SQL_DATA_DATE:        // date
            case SQL_DATA_TIME:        // time
            case SQL_DATA_TIMESTAMP:   // timestamp
            case SQL_DATA_DATETIME:    // datetime
            case SQL_DATA_BYTE:
                sc_snprintf(disp_name, 32, "%%-%d.*s", fields[j]->max_length);
                fprintf(isql_fp, disp_name, fields[j]->max_length, row[j]);
                break;
            case SQL_DATA_VARCHAR:
            case SQL_DATA_VARBYTE:
                sc_snprintf(disp_name, 32, "%%-%d.*s",
                        (fields[j]->max_length < MAX_HEADLINE) ?
                        fields[j]->max_length : MAX_HEADLINE);
                fprintf(isql_fp, disp_name, fields[j]->max_length, row[j]);
                break;
            case SQL_DATA_NCHAR:       // char[]
                sc_snprintf(disp_name, 32, "%%-%d.*s",
                        sizeof(DB_WCHAR) * fields[j]->max_length);
                fprintf(isql_fp, disp_name,
                        sizeof(DB_WCHAR) * fields[j]->max_length, row[j]);
                break;
            case SQL_DATA_NVARCHAR:    // varchar
                sc_snprintf(disp_name, 32, "%%-%d.*s",
                        (sizeof(DB_WCHAR) * fields[j]->max_length <
                                MAX_HEADLINE) ? (sizeof(DB_WCHAR) *
                                fields[j]->max_length) : MAX_HEADLINE);
                fprintf(isql_fp, disp_name,
                        sizeof(DB_WCHAR) * fields[j]->max_length, row[j]);
                break;
            default:
                sc_snprintf(disp_name, 32, "%%%d.*s", fields[j]->max_length);
                fprintf(isql_fp, disp_name, fields[j]->max_length, row[j]);
                break;
            }

            if (j + 1 < num_fields)
                fprintf(isql_fp, " ");
        }
        fprintf(isql_fp, "\n");
    }       // for(;;)

#ifndef WIN32
    if (isql_fp != stdout)
        pclose(isql_fp);
#endif

    if (!iSQL_eof(res))
        fprintf(stdout, "\nERROR: %s\n\n", iSQL_error(isql));
    else
    {
        if (isql->flags & OPT_FEEDBACK)
        {
            if (i > 1)
                fprintf(stdout, "\n%d rows selected\n\n", i);
            else if (i <= 0)
                fprintf(stdout, "\nno rows selected\n\n");
            else
                fprintf(stdout, "\n");
        }
        else
        {
            if (i > 0)
                fprintf(stdout, "\n");
        }
    }

  RETURN_LABEL:
    if (fields)
        sc_free(fields);
    iSQL_free_result(res);
}



/*****************************************************************************/
//!  main  
/*! \breif  main function
 ************************************
 * \param ac(in) :
 * \param av(in): 
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘 및 기타 설명
 *  - AUTOCOMMIT을 OFF 시킬 수 있는 OPTION을 추가한다
 *  - ADMIN 문장 추가(EXPORT & IMPORT)
 *****************************************************************************/
int
main(int ac, char **av)
{
    char query[iSQL_QUERY_MAX];
    char dbhost[MAX_DBHOSTNAME], dbname[MAX_DBNAME];
    char dbuser[MAX_USERNAME], dbpasswd[MAX_PASSWD];
    char start_sql_file[PATH_MAX];
    const int opval = 1;
    int ret;
    char *query_start_ptr = NULL;
    char *tmp_ptr;
    int i;
    T_DB_TIMEVAL tv = { 0, 0 };
    int stdin_is_file = 0;

#if defined(sun) || defined(linux)
    //set_control_character(VERASE, '', 0);
    //set_control_character(VWERASE, '', 0);
#endif

#if defined(HAVE_READLINE) && (HAVE_READLINE==1)
    _s_i_prompt_is_interactive = isatty(0);
    sc_sprintf(_s_sz_history_home, "%s/.ml_history", sc_getenv("HOME"));
    using_history();
#endif

    memset(query_filename, 0, sizeof(query_filename));

    do
    {
        int ret;
        struct stat buf;

        ret = fstat(fileno(stdin), &buf);

        if (ret == 0 && buf.st_mode & S_IFREG)
            stdin_is_file = 1;

        /*
           printf("$$$$$ %d %d %d\n", ret, buf.st_rdev, stdin_is_file);
           printf("$$$$$ st_ino %d\n", buf.st_ino);
           printf("$$$$$ st_mode %d\n", buf.st_mode);
           printf("$$$$$ st_mode & S_IFIFO %d\n", buf.st_mode & S_IFIFO);
           printf("$$$$$ st_mode & S_IFCHR %d\n", buf.st_mode & S_IFCHR);
           printf("$$$$$ st_mode & S_IFBLK %d\n", buf.st_mode & S_IFBLK);
           printf("$$$$$ st_mode & S_IFREG %d\n", buf.st_mode & S_IFREG);
         */
    } while (0);

#ifndef WIN32
    signal(SIGPIPE, SIG_IGN);
#endif

    fprintf(stdout, "\nisql for %s %s\n", _DBNAME_, _DBVERSION_);
    fprintf(stdout, "%s\n\n", _COPYRIGHT_);

    pn = av[0];

    if (!parse_options(ac, av,
                    dbhost, dbname, dbuser, dbpasswd, start_sql_file))
        return -1;

    if (fflag)
        createdb(dbname);

    if (!iSQL_connect(&isql, dbhost, dbname, dbuser, dbpasswd))
    {
        fprintf(stdout, "Error: %s\n", iSQL_error(&isql));
        return -1;
    }

    if (is_autocommit == 0)
    {
        if (iSQL_options(&isql, OPT_AUTOCOMMIT, &opval, sizeof(opval)) < 0)
        {
            fprintf(stdout,
                    "isql: an error occured on setting auto-commit mode.\n");
            return -1;
        }
    }

    if (iSQL_options(&isql, OPT_PLAN_OFF, &opval, sizeof(opval)) < 0)
    {
        fprintf(stdout, "isql: an error occured on setting plan off mode.\n");
        return -1;
    }

    query[0] = '\0';

    if (sc_strlen(start_sql_file))
    {
        init_query_in_file(start_sql_file);
    }

    if (_s_i_prompt_is_interactive)
    {
        read_history(_s_sz_history_home);
    }

    while (1)
    {
      READ_QUERY_STRING:
        if (interrupt_flag)
        {
            /* check 1 :
               If SQL script file is currently used, close the scirpt file. 
               Otherwise, the next line will be executed continuously.
             */
            if (use_query_in_file)
            {
                fclose(script_fptr);
                script_fptr = NULL;
                use_query_in_file = 0;
            }

            /* check 2 :
               # isql testdb < query.sql

               Like above command line, if the SQL script file is given 
               using the command line, the device ID of stdin is 0.
               In this case, the interrupt must terminate isql utility.
             */
            if (stdin_is_file)
            {
                isql_end(0);
                return 0;
            }

            interrupt_flag = 0;
        }

        ret = read_querystr(query);
        if (ret == 0)
            continue;
        else if (ret == -1)
        {   // quit..
            if (interrupt_flag)
                continue;
            iSQL_commit(&isql);
            break;
        }
        else if (ret == -2)
        {   // TOO BIG QUERY STRING
            if (interrupt_flag)
                continue;
            fprintf(stdout, "\nERROR: A query size is too large.\n\n");
            continue;
        }
        else if (query[0] == '\0')
            continue;

        if (_s_i_prompt_is_interactive)
        {
            stifle_history(50);
            write_history(_s_sz_history_home);
        }

        if (isql.flags & OPT_TIME)
            set_timeinterval(&tv);

        query_start_ptr = query;

        while (query_start_ptr)
        {
            if (interrupt_flag)
                break;
            tmp_ptr = set_next_query_str(query_start_ptr);
            fprintf(stdout, "%s\n", query_start_ptr);
            queryexec_flag = 1;

            if (iSQL_query(&isql, query_start_ptr))
            {
                queryexec_flag = 0;
                fprintf(stdout, "\nERROR: %s\n\n", iSQL_error(&isql));
                if (isql.flags & OPT_TIME)
                {
                    get_timeinterval(&tv, NULL);
                    fprintf(stdout, "\n");
                }
                if (iSQL_errno(&isql) == SQL_E_NOTCONNECTED)
                    return -1;
                i = 0;
                // shift from this to the first.
                do
                {
                    query[i] = query_start_ptr[i];
                    i++;
                } while (query_start_ptr[i] != '\0');
                // if not last query, shift remained ones
                if (tmp_ptr)
                {
                    // get back ';'
                    while (query_start_ptr[i] == '\0')
                        query[i++] = ';';
                    // shift the remains
                    do
                    {
                        query[i] = query_start_ptr[i];
                        i++;
                    } while (query_start_ptr[i] != '\0');
                }
                query[i] = '\0';
                goto READ_QUERY_STRING;
            }
            queryexec_flag = 0;
            if (tmp_ptr == NULL)
            {
                i = 0;
                do
                {
                    query[i] = query_start_ptr[i];
                    i++;
                } while (query_start_ptr[i] != '\0');
                query[i] = '\0';
            }

            query_start_ptr = tmp_ptr;
            switch (iSQL_last_querytype(&isql))
            {
            case SQL_STMT_SELECT:
                queryexec_flag = 1;
                print_resultset(&isql);
                queryexec_flag = 0;
                break;
            case SQL_STMT_INSERT:
                fprintf(stdout,
                        "\n%u row(s) created.\n\n", iSQL_affected_rows(&isql));
                break;
            case SQL_STMT_UPSERT:
                switch (iSQL_affected_rows(&isql))
                {
                case 0:
                    fprintf(stdout, "\n1 row exists.\n\n");
                    break;
                case 1:
                    fprintf(stdout, "\n1 row inserted.\n\n");
                    break;
                case 2:
                    fprintf(stdout, "\n1 row updated.\n\n");
                    break;
                }

                break;
            case SQL_STMT_UPDATE:
                print_resultset(&isql);
                fprintf(stdout,
                        "\n%d row(s) updated.\n\n", iSQL_affected_rows(&isql));
                break;
            case SQL_STMT_DELETE:
                print_resultset(&isql);
                fprintf(stdout,
                        "\n%d row(s) deleted.\n\n", iSQL_affected_rows(&isql));
                break;
            case SQL_STMT_CREATE:
                fprintf(stdout, "\nObject created.\n\n");
                break;
            case SQL_STMT_DROP:
                fprintf(stdout, "\nObject dropped.\n\n");
                break;
            case SQL_STMT_RENAME:
                fprintf(stdout, "\nObject renamed.\n\n");
                break;
            case SQL_STMT_ALTER:
                fprintf(stdout, "\nObject altered.\n\n");
                break;
            case SQL_STMT_COMMIT:
                fprintf(stdout, "\nCommit complete.\n\n");
                break;
            case SQL_STMT_ROLLBACK:
                fprintf(stdout, "\nRollback complete.\n\n");
                break;
            case SQL_STMT_DESCRIBE:
                queryexec_flag = 1;
                print_resultset(&isql);
                queryexec_flag = 0;
                break;
            case SQL_STMT_SET:
                fprintf(stdout, "\nSet complete.\n\n");
                break;
            case SQL_STMT_TRUNCATE:
                fprintf(stdout, "\nTruncate complete.\n\n");
                break;
            case SQL_STMT_ADMIN:
                fprintf(stdout, "\nAdmin Statement complete.\n\n");
                break;

            default:
                fprintf(stdout, "\nInappropriate input");
                if (iSQL_last_querytype(&isql) == SQL_STMT_NONE)
                    fprintf(stdout, "(no query).\n\n");
                else
                    fprintf(stdout, "(%d).\n\n", iSQL_last_querytype(&isql));
                break;
            }

            if (isql.flags & OPT_TIME
                    && iSQL_last_querytype(&isql) != SQL_STMT_SET)
            {
                get_timeinterval(&tv, NULL);
                fprintf(stdout, "\n");
            }

            if (isql.flags & OPT_TIME && query_start_ptr)
                set_timeinterval(&tv);

        }   // while(query_start_ptr);
    }

    isql_end(0);

    return 0;   // For avoiding compile warning
}

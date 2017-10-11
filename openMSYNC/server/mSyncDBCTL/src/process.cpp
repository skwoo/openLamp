/* 
    This file is part of openMSync Server module, DB synchronization software.

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

#include	<windows.h>
#include	<stdio.h>
#include	<stdarg.h>
#include	<errno.h>
#include	<string.h>	

#include	<direct.h>
#include	<io.h>
#include	<time.h>
#include	<SYS\TIMEB.H>

#include	"Sync.h"

#define		LOGHOME	"ServerLog"

int				logInit = TRUE;
static FILE*	pErrorLog = NULL ;
static char		errorLogFileName[256] ;

char				mSyncPath[200];
CRITICAL_SECTION	crit;

int  Due_Year, Due_Mon;

/*****************************************************************************/
//! SetPath 
/*****************************************************************************/
/*! \breif  Setting the mSync's Path
 ************************************
 * \param	productInfo(in) : 제품정보가 들어 잇는 구조체 
 ************************************
 * \return	void
 ************************************
 * \note	mSync의 path를 setting하는 함수로 현재 실행되는 mSync의 path가\n
 *			무엇인지 알 수 있도록 로그에 남긴다.
 *****************************************************************************/
__DECL_PREFIX_MSYNC void SetPath(char *path)
{
	sprintf(mSyncPath, "%s\\", path);
}


/*****************************************************************************/
//! TimeGet
/*****************************************************************************/
/*! \breif  현재 시간 출력
 ************************************
 * \param	CurrentTime(out) : MON DAY, Year at 24hh:mm:ss 형태로 출력
 ************************************
 * \return  제품의 Due 시간과 비교해서 값을 넣어준다\n
 *			TRUE	: 제품 사용 시간이 남음\n
 *			FALSE	: 제품 사용 시간이 남지 않음 
 ************************************
 * \note	현재 시간을 특정한 형태의 문자열로 변화시켜줌
 *****************************************************************************/
int TimeGet(char *CurrentTime)
{
    char	*MON[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", 
                      "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    struct tm	*tp;	
    time_t		tm;
    int			REMAIN_DAYS = TRUE;
    
    time(&tm);
    tp = localtime(&tm);
    memset(CurrentTime, 0x00, sizeof(CurrentTime));
    
    if(tp->tm_year + 1900 > Due_Year || 
        (tp->tm_year+1900 == Due_Year && tp->tm_mon >= Due_Mon )) 
    {
        REMAIN_DAYS = FALSE;
    }
    
    sprintf(CurrentTime, "%3s %02d, %04d at %02d:%02d:%02d", 
            MON[tp->tm_mon], tp->tm_mday,  tp->tm_year+1900, 
            tp->tm_hour, tp->tm_min, tp->tm_sec);
    
    return	REMAIN_DAYS;
}

/*****************************************************************************/
//! GetYYMMDD
/*****************************************************************************/
/*! \breif  현재 시간을 특정한 형태의 문자열로 변화시켜줌
 ************************************
 * \param	curTime(out) : MMDDHHMMSS 형태로 표현 
 ************************************
 * \return  void
 ************************************
 * \note	현재 시간을 특정한 형태의 문자열로 변화시켜줌
 *****************************************************************************/
void GetYYMMDD(char *curTime)
{
    struct _timeb tstruct;
    struct 	tm      *tp;	
    time_t tm;
    
    time(&tm);
    tp = localtime(&tm);
    _ftime( &tstruct );
    
    sprintf(curTime, "%02d%02d_%02d%02d%02d%03d", 
            tp->tm_mon+1, tp->tm_mday, 
            tp->tm_hour, tp->tm_min, tp->tm_sec, tstruct.millitm );
}

/*****************************************************************************/
//! ErrLog 
/*****************************************************************************/
/*! \breif  에러 로그용 함수
 ************************************
 * \param	char *format,... : 로그에 기록할 내용들
 ************************************
 * \return  void
 ************************************
 * \note	mSync에서 필요한 로그를 log.txt에 남겨주는 함수로 LOG_FILE_SIZE를\n
 *			초과하면 기존 로그 정보는 다른 이름으로 move 해주고 log.txt에\n
 *			계속하여 로그를 남긴다.
 *****************************************************************************/
__DECL_PREFIX_MSYNC void ErrLog(char *format,...)
{
    int     ret = 0;
    char	curTime[DATE_LEN + 1] ;	
    va_list	arg ;
    int		seek_file;
    char	newLogFileName[256] ;
    
    if(logInit)
    {
        memset(errorLogFileName, 0x00, sizeof(errorLogFileName)) ;
        sprintf(errorLogFileName, "%s%s", mSyncPath, LOGHOME);
        if (_access(errorLogFileName, 6) < 0)  // read & write permission
        {
            if (_mkdir(errorLogFileName) < 0) 
            {
                fprintf(stderr, "Log file open error[%s]\n", strerror(errno));
                exit(1) ;
            }
        }
        sprintf(errorLogFileName, "%s\\log.txt", errorLogFileName) ;
        
        GetYYMMDD(curTime);
        sprintf(newLogFileName, 
                "%s%s\\log_%s.txt", mSyncPath, LOGHOME, curTime);
        ret = rename(errorLogFileName, newLogFileName);
        pErrorLog = fopen(errorLogFileName, "w") ;
        
        if (pErrorLog == NULL)	
            exit(1);
        logInit = FALSE;
        fseek(pErrorLog, 0, SEEK_END);
        
        
        InitializeCriticalSection(&crit);
    }
    
    EnterCriticalSection(&crit);
    seek_file = ftell(pErrorLog);
    if(seek_file > LOG_FILE_SIZE)
    {	
        GetYYMMDD(curTime);
        sprintf(newLogFileName, 
                "%s%s\\log_%s.txt", mSyncPath, LOGHOME, curTime);
        fflush(pErrorLog);
        fclose(pErrorLog);
        ret = rename(errorLogFileName, newLogFileName);
        pErrorLog = fopen(errorLogFileName, "w") ;
    }
    
    TimeGet((char *)curTime);
    fprintf(pErrorLog,"%s;", curTime);
    
    va_start(arg, format) ;	
    vfprintf(pErrorLog, format, arg) ;
    
    if (fflush(pErrorLog) == EOF) 
    {
        fprintf(stderr, "fflush error[%s]\n", strerror(errno)) ;
        exit(1) ;
    }
    
    va_end(arg) ;
    
    LeaveCriticalSection(&crit);
}

/*****************************************************************************/
//! MYDeleteCriticalSection
/*****************************************************************************/
/*! \breif  크리티칼 셕션(전역 변수)을 삭제
 ************************************
 * \param	void	
  ************************************
 * \return  void
 ************************************
 * \note	Windows용에서 로그 파일을 공유하기 때문에 Critical section을\n
 *			사용하는데 mSync 종료 시에 이 값을 삭제한다.
 *****************************************************************************/
__DECL_PREFIX_MSYNC void MYDeleteCriticalSection()
{
	DeleteCriticalSection(&crit);
}


/*****************************************************************************/
//! ParseData
/*****************************************************************************/
/*! \breif  현재 시간 출력
 ************************************
 * \param	ptr(in)				: 입력받은 문자열 
 * \param	uid(out)			: 파싱된 id
 * \param	passwd(out)			: 파싱된 pwd
 * \param	applicationID(out)	: 파싱된 application ID
 * \param	version(out)		: 파싱된 version(application)
 ************************************
 * \return  void
 ************************************
 * \note	Client로부터 입력받은 버퍼를 parsing 하여 userid, passwd, \n
 *			application, version 정보를 return 한다. \n \n
 *			-- start jinki --\n
 *			보안을 생각한다면 pwd도 암호화 시켜주고 복호화 시켜줘야 한다\n
 *			(단점 패킷 사이즈가 커질 것이다)
 *****************************************************************************/
int	ParseData(char *ptr, char *uid, char *passwd, 
              char *applicationID, int *version)
{
    int		Idx = 0, s_Idx = 0, length;
    int		dataLength = strlen(ptr);
    char	tmpversion[VERSION_LEN+1];
    
    // UID 파싱
    while(Idx<dataLength && ptr[Idx] != '|') 
        Idx++;
    
    length = Idx-s_Idx;
    if(length==0 || length > USER_ID_LEN) 
        return (-1);
    
    strncpy(uid, ptr+s_Idx, length);
    uid[length] = 0x00;
    if(ptr[Idx] == '|') Idx++;		// delimeter skip
    s_Idx = Idx;	
    
    // PWD 파싱
    while(Idx<dataLength && ptr[Idx] != '|') 
        Idx++;
    
    length = Idx-s_Idx;
    if(length==0 || length > PASSWD_LEN) 
        return (-1);	
    
    strncpy(passwd, ptr+s_Idx, length);
    passwd[length] = 0x00;
    if(ptr[Idx] == '|') 
        Idx++;		// delimeter skip
    
    s_Idx = Idx;	
    
    // applicatonID 파싱
    while(Idx<dataLength && ptr[Idx] != '|')
        Idx++;
    
    length = Idx-s_Idx;
    if(length==0 || length > APPLICATION_ID_LEN) 
        return (-1);	
    
    strncpy(applicationID, ptr+s_Idx, length);
    applicationID[length] = 0x00;
    if(ptr[Idx] == '|') 
        Idx++;		// delimeter skip
    
    s_Idx = Idx;	
    
    // version 파싱
    while(Idx<dataLength && ptr[Idx] != '|') 
        Idx++;
    
    length = Idx-s_Idx;
    if(length==0 || length > VERSION_LEN) 
        return (-1);
    
    strncpy(tmpversion, ptr+s_Idx, length);
    tmpversion[length] = 0x00;
    if(ptr[Idx] == '|') 
        Idx++;		// delimeter skip
    
    s_Idx = Idx;
    *version = atoi(tmpversion);
    
    if(Idx == dataLength && ptr[Idx] != 0x00) 
        return (-1);
    
    return 0;
}

/*****************************************************************************/
//! DisplayError
/*****************************************************************************/
/*! \breif  due date를 setting
 ************************************
 * \param	hstmt(out)			:
 * \param	uid(in)				:
 * \param	order(in)			:
 * \param	returncoude(out)	:
 ************************************
 * \return	void
 ************************************
 * \note	Sync 중에 발생하는 DB 에러의 메시지를 출력하는 함수.
 *****************************************************************************/
void DisplayError(SQLHSTMT hstmt, char *uid, int order, int returncode)
{
    SQLCHAR			SQLSTATE[6], Msg[256];
    SQLINTEGER		NativeError;
    SQLSMALLINT		MsgLen;
    
    Msg[0] = '\0';
    SQLSTATE[0] = '\0';
    SQLGetDiagRec(SQL_HANDLE_STMT, 
                  hstmt, 1, SQLSTATE, &NativeError, Msg, sizeof(Msg), &MsgLen);
    
    if (Msg[strlen((char *)Msg)-1]=='\n')
        Msg[strlen((char *)Msg)-1] = '\0';
    
    ErrLog("%d_%s;DBERROR[%d:%s]:%s;\n", order, uid, returncode, SQLSTATE, Msg);	
}

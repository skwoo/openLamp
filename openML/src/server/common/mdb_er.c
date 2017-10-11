/* 
   This file is part of openML, mobile and embedded DBMS.

   Copyright (C) 2012 Inervit Co., Ltd.
   support@inervit.com

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Less General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Less General Public License for more details.

   You should have received a copy of the GNU Less General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "mdb_config.h"
#include "dbport.h"
#include "mdb_compat.h"
#include "ErrorCode.h"
#include "mdb_er.h"
#include "mdb_comm_stub.h"

/*****************************************************************************/
//! er_set_message 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param er_Msg :
 * \param num_args : 
 * \param ap_ptr :
 * \param err_msg :
 ************************************
 * \return  static ER_MSG * :
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static ER_MSG *
er_set_message(ER_MSG * er_Msg, int num_args, va_list * ap_ptr, char *err_msg)
{
    char *src_msg;
    int msg_size;

    /* ... */

    /* make error message */
    if (err_msg)
        src_msg = err_msg;
    else
        src_msg = (char *) DB_strerror(er_Msg->er_id);

    msg_size = sc_strlen(src_msg) + 1;

    if (msg_size > er_Msg->msg_area_size)
    {
        if (er_Msg->msg_area)
            mdb_free(er_Msg->msg_area);
        er_Msg->msg_area = NULL;
        er_Msg->msg_area_size = 0;
    }

    if (er_Msg->msg_area == NULL)
    {
        er_Msg->msg_area = (char *) mdb_malloc(msg_size);

        if (er_Msg->msg_area == NULL)
        {   /* we need more gentle action for internal error */
            return NULL;
        }
        er_Msg->msg_area_size = msg_size;
    }

    sc_strcpy(er_Msg->msg_area, src_msg);

#ifdef MDB_DEBUG
    if (er_Msg->er_id != SQL_E_STACKUNDERFLOW &&
            er_Msg->er_id != DB_E_DUPUNIQUEKEY)
    {
        __SYSLOG("ERRORINFO [%d] %d : (%s, %d)", er_Msg->er_id,
                er_Msg->severity, er_Msg->file_name, er_Msg->lineno);
        __SYSLOG("%s)\n", er_Msg->msg_area);
    }
#endif

    return er_Msg;
}


__DECL_PREFIX ER_MSG *er_Msg = NULL;
__DECL_PREFIX ER_MSG er_msgarea;

/*****************************************************************************/
//! er_clear 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param void :
 ************************************
 * \return  void
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
void
er_clear(void)
{
    if (er_Msg == NULL)
        return;

    if (er_Msg->msg_area)
    {
        mdb_free(er_Msg->msg_area);
        er_Msg->msg_area = NULL;
    }
    er_Msg->msg_area_size = 0;
    er_Msg->er_id = DB_NOERROR;
}

/*****************************************************************************/
//! er_errid 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param void
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
__DECL_PREFIX int
er_errid(void)
{
    if (er_Msg == NULL)
        return DB_NOERROR;
    else
        return er_Msg->er_id;
}

/*****************************************************************************/
//! er_message 
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
__DECL_PREFIX char *
er_message(void)
{
    if (er_Msg == NULL || er_Msg->er_id == DB_NOERROR)
        return NULL;
    else
        return er_Msg->msg_area;
}

/*****************************************************************************/
//! er_init 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param void 
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static ER_MSG *
er_init(void)
{
    er_Msg = &er_msgarea;

    er_Msg->er_id = DB_NOERROR;
    er_Msg->msg_area = NULL;
    er_Msg->msg_area_size = 0;

    return er_Msg;
}

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
#if defined(MDB_DEBUG) || defined(WIN32)
void
er_set(int severity, const char *file_name, const int lineno,
        int er_id, int num_args, ...)
#else
void
er_set_real(int severity, int er_id, int num_args, ...)
#endif
{

    va_list ap;

    if (er_Msg == NULL)
        er_Msg = er_init();

    if (er_Msg->er_id != DB_NOERROR)
        return;

    /* Initialize the area... */
    er_Msg->er_id = er_id;
    er_Msg->severity = severity;
#if defined(MDB_DEBUG) || defined(WIN32)
    /*er_Msg->file_name  = file_name; */
    sc_strncpy(er_Msg->file_name, file_name, MDB_FILE_NAME - 1);
    er_Msg->lineno = lineno;
#endif

    va_start(ap, num_args);

    if (er_set_message(er_Msg, num_args, &ap, NULL) == NULL)
    {
        /* exit_routin(); */
    }

    va_end(ap);
}


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
/* caller must free err_msg */
#if defined(MDB_DEBUG) || defined(WIN32)
void
er_set_parser(int severity, const char *file_name, const int lineno,
        int er_id, char *err_msg)
#else
void
er_set_parser_real(int severity, int er_id, char *err_msg)
#endif
{
    if (er_Msg == NULL)
        er_Msg = er_init();

    /* Initialize the area... */
    er_Msg->er_id = er_id;
    er_Msg->severity = severity;
#if defined(MDB_DEBUG) || defined(WIN32)
    /*er_Msg->file_name  = file_name; */
    sc_strncpy(er_Msg->file_name, file_name, MDB_FILE_NAME - 1);
    er_Msg->lineno = lineno;
#endif

    if (er_set_message(er_Msg, 0, NULL, err_msg) == NULL)
    {
        /* exit_routin(); */
    }
}

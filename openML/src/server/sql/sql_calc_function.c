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

#include "isql.h"
#include "sql_datast.h"
#include "sql_util.h"

#include "sql_calc_timefunc.h"
#include "sql_func_timeutil.h"
#include "mdb_er.h"
#include "ErrorCode.h"
#include "mdb_charset.h"

#include "mdb_syslog.h"

#include "mdb_Server.h"
extern int (*sc_ncollate_func[]) (const char *s1, const char *s2, int n);

/*****************************************************************************/
//! check_function 

/*! \breif  SQL function들의 인자가 옳바른지를 검사
 ************************************
 * \param func(in)  :
 * \param elem(in)  : 
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘 및 기타 설명 \n
 *  - SEQUENCE 추가
 *  - REPLACE() 추가
 *  - T_EXPRESSIONDESC 자료구조 변경
 *      elem 타입이 (T_POSTFIXELEM*) -> (T_POSTFIXELEM**)
 *  - BYTE/VARBYTE인 경우 사용하지 못하도록 함
 *  - BYTE_SIZE, COPYFROM(), COPYTO() 함수 인자 검사
 *
 *****************************************************************************/
static int
check_function(T_FUNCDESC * func, T_POSTFIXELEM ** elem)
{
    register T_VALUEDESC *t_e0_value = NULL;

    if (elem)
        t_e0_value = &elem[0]->u.value;

    switch (func->type)
    {
    case SFT_DECODE:
        if (IS_BYTE(t_e0_value->valuetype))
            return RET_ERROR;
    case SFT_CONVERT:
    case SFT_IFNULL:
        if ((!t_e0_value->is_null &&
                        t_e0_value->valuetype == DT_NULL_TYPE) ||
                (!elem[1]->u.value.is_null &&
                        elem[1]->u.value.valuetype == DT_NULL_TYPE))
            return RET_ERROR;
        break;
    case SFT_SIGN:
        if (!t_e0_value->is_null && !IS_NUMERIC(t_e0_value->valuetype))
            return RET_ERROR;
        break;
    case SFT_ROUND:
        if (!t_e0_value->is_null && !IS_NUMERIC(t_e0_value->valuetype))
            return RET_ERROR;
        if (!elem[1]->u.value.is_null &&
                !IS_NUMERIC(elem[1]->u.value.valuetype))
            return RET_ERROR;
        break;
    case SFT_TRUNC:
        if (!t_e0_value->is_null && !IS_NUMERIC(t_e0_value->valuetype))
            return RET_ERROR;
        if (!elem[1]->u.value.is_null &&
                !IS_NUMERIC(elem[1]->u.value.valuetype))
            return RET_ERROR;
        break;
    case SFT_SUBSTRING:        /* check at compile time(parser) */
        if (!t_e0_value->is_null && !IS_BS_OR_NBS(t_e0_value->valuetype))
            return RET_ERROR;
        break;
    case SFT_LOWERCASE:
    case SFT_UPPERCASE:
        if (!t_e0_value->is_null && !IS_BS_OR_NBS(t_e0_value->valuetype))
            return RET_ERROR;
        break;
    case SFT_HEXASTRING:
    case SFT_BINARYSTRING:
        if (!t_e0_value->is_null && !IS_BYTE(t_e0_value->valuetype))
            return RET_ERROR;
        break;
    case SFT_LTRIM:
    case SFT_RTRIM:
        if (!t_e0_value->is_null && !IS_BS_OR_NBS(t_e0_value->valuetype))
            return RET_ERROR;
        if (func->argcnt == 2)
        {
            if (!elem[1]->u.value.is_null &&
                    !IS_BS_OR_NBS(elem[1]->u.value.valuetype))
                return RET_ERROR;
            if ((IS_BS(elem[0]->u.value.valuetype)
                            && !IS_BS(elem[1]->u.value.valuetype))
                    || (IS_NBS(elem[0]->u.value.valuetype) &&
                            !IS_NBS(elem[1]->u.value.valuetype)))
                return RET_ERROR;
        }
        break;
    case SFT_CURRENT_DATE:
    case SFT_CURRENT_TIME:
    case SFT_CURRENT_TIMESTAMP:
    case SFT_SYSDATE:
    case SFT_NOW:
        break;
    case SFT_DATE_ADD:
    case SFT_DATE_SUB:
        if (t_e0_value->valueclass != SVC_CONSTANT ||
                (!t_e0_value->is_null && t_e0_value->valuetype != DT_VARCHAR))
            return RET_ERROR;
        if (!elem[1]->u.value.is_null &&
                !IS_INTEGER(elem[1]->u.value.valuetype))
            return RET_ERROR;
        if (!elem[2]->u.value.is_null &&
                !IS_STRING(elem[2]->u.value.valuetype))
            return RET_ERROR;
        break;
    case SFT_DATE_DIFF:
        if (t_e0_value->valueclass != SVC_CONSTANT ||
                (!t_e0_value->is_null && t_e0_value->valuetype != DT_VARCHAR))
            return RET_ERROR;
        if (!elem[1]->u.value.is_null &&
                !IS_STRING(elem[1]->u.value.valuetype))
            return RET_ERROR;
        if (!elem[2]->u.value.is_null &&
                !IS_STRING(elem[2]->u.value.valuetype))
            return RET_ERROR;
        break;
    case SFT_DATE_FORMAT:
        if (!t_e0_value->is_null && t_e0_value->valuetype == DT_NULL_TYPE)
            return RET_ERROR;
        if (!elem[1]->u.value.is_null && !IS_BS(elem[1]->u.value.valuetype))
            return RET_ERROR;
        break;
    case SFT_TIME_ADD:
    case SFT_TIME_SUB:
        if (t_e0_value->valueclass != SVC_CONSTANT ||
                (!t_e0_value->is_null && t_e0_value->valuetype != DT_VARCHAR))
            return RET_ERROR;
        if (!elem[1]->u.value.is_null &&
                !IS_INTEGER(elem[1]->u.value.valuetype))
            return RET_ERROR;
        if (!elem[2]->u.value.is_null &&
                !IS_STRING(elem[2]->u.value.valuetype))
            return RET_ERROR;
        break;
    case SFT_TIME_DIFF:
        if (t_e0_value->valueclass != SVC_CONSTANT ||
                (!t_e0_value->is_null && t_e0_value->valuetype != DT_VARCHAR))
            return RET_ERROR;
        if (!elem[1]->u.value.is_null &&
                !IS_STRING(elem[1]->u.value.valuetype))
            return RET_ERROR;
        if (!elem[2]->u.value.is_null &&
                !IS_STRING(elem[2]->u.value.valuetype))
            return RET_ERROR;
        break;
    case SFT_TIME_FORMAT:
        if (!t_e0_value->is_null && t_e0_value->valuetype == DT_NULL_TYPE)
            return RET_ERROR;
        if (!elem[1]->u.value.is_null && !IS_BS(elem[1]->u.value.valuetype))
            return RET_ERROR;
        break;
    case SFT_CHARTOHEX:
        if (t_e0_value->valueclass != SVC_CONSTANT ||
                (!t_e0_value->is_null && t_e0_value->valuetype != DT_VARCHAR))
            return RET_ERROR;
        break;
    case SFT_HEXCOMP:
    case SFT_HEXNOTCOMP:
        if (t_e0_value->valueclass == SVC_VARIABLE &&
                (!t_e0_value->is_null && t_e0_value->valuetype != DT_CHAR))
            return RET_ERROR;
        if (elem[1]->u.value.valueclass == SVC_VARIABLE &&
                (!elem[1]->u.value.is_null &&
                        elem[1]->u.value.valuetype != DT_CHAR))
            return RET_ERROR;
        break;
    case SFT_RANDOM:
        break;
    case SFT_SRANDOM:
        if (!IS_NUMERIC(t_e0_value->valuetype))
            return RET_ERROR;
        break;
    case SFT_ROWNUM:
        if (func->rownump == NULL)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, RET_ERROR, 0);
            return RET_ERROR;
        }
        break;

    case SFT_RID:
        if (t_e0_value->valueclass == SVC_VARIABLE &&
                t_e0_value->valuetype != DT_OID)
            return RET_ERROR;

        break;

    case SFT_SEQUENCE_CURRVAL:
    case SFT_SEQUENCE_NEXTVAL:
        if (t_e0_value->u.ptr == NULL || t_e0_value->u.ptr[0] == '\0')
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, RET_ERROR, 0);
            return RET_ERROR;
        }
        break;

    case SFT_OBJECTSIZE:
        if (!elem[1]->u.value.is_null && !IS_BS(elem[1]->u.value.valuetype))
            return RET_ERROR;
        break;
    case SFT_BYTESIZE:
        if (!t_e0_value->is_null && !IS_BYTE(t_e0_value->valuetype))
            return RET_ERROR;
        break;
    case SFT_COPYFROM:
        if (!t_e0_value->is_null && !IS_BS(t_e0_value->valuetype))
            return RET_ERROR;
        if (!elem[1]->u.value.is_null && !IS_BS(elem[1]->u.value.valuetype))
            return RET_ERROR;
        break;
    case SFT_COPYTO:
        if (!t_e0_value->is_null &&
                !(IS_BYTE(t_e0_value->valuetype) ||
                        IS_BS(t_e0_value->valuetype)))
            return RET_ERROR;

        if (!elem[1]->u.value.is_null && !IS_BS(elem[1]->u.value.valuetype))
            return RET_ERROR;
        break;

#if defined(MDB_DEBUG)
    case SFT_TABLENAME:
        if (t_e0_value->valueclass == SVC_VARIABLE &&
                t_e0_value->valuetype != DT_OID)
            return RET_ERROR;
        break;
#endif
    default:
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, RET_ERROR, 0);
        return RET_ERROR;
    }
    return RET_SUCCESS;
}

/*****************************************************************************/
//! calc_func_convert 

/*! \breif src를 base의 자료형으로 변환하여\n
            그 결과를 res로 반환한다.
 ************************************
 * \param base(in)      :
 * \param src(in)   : 
 * \param res(in/out)   :
 ************************************
 * \return RET_SUCCESS or some errors
 ************************************
 * \note 
 *  - decimal이 소숫점이 없는 경우, "%g"로 소수점 위에만 출력하도록 수정
 *  - BYTE, VARBYTE 지원
 *      공간은 attrdesc.len 만큼 할당하고, value_len 만큼 복사한다
 *  - base의 collation을 따라 간다
 *****************************************************************************/
int
calc_func_convert(T_VALUEDESC * base, T_VALUEDESC * src, T_VALUEDESC * res,
        MDB_INT32 is_init)
{
    DB_BOOL is_float = 0;
    char datetime[MAX_DATETIME_STRING_LEN];
    char date[MAX_DATE_STRING_LEN];
    char time[MAX_TIME_STRING_LEN];
    char timestamp[MAX_TIMESTAMP_STRING_LEN];
    int ret = RET_ERROR;
    char *ptr;

    if (base->valueclass == SVC_VARIABLE || src->valueclass == SVC_VARIABLE)
    {
        res->valueclass = SVC_VARIABLE;
    }

    res->valuetype = base->valuetype;
    res->attrdesc.collation = base->attrdesc.collation;

    if (IS_NUMERIC(res->valuetype) && res->valuetype != DT_DECIMAL)
    {
        res->attrdesc.len = SizeOfType[res->valuetype];
    }
    else
    {
        res->attrdesc.len = base->attrdesc.len;
        if (res->valuetype == DT_DECIMAL)
        {
            res->attrdesc.dec = base->attrdesc.dec;
        }
    }

    if (src->is_null)
    {
        res->is_null = 1;
        return RET_SUCCESS;
    }

    if (IS_NUMERIC(res->valuetype)
            && (src->valueclass == SVC_CONSTANT || !is_init))
    {
        if (IS_BS(src->valuetype))
        {
            ret = mdb_check_numeric(src->u.ptr, NULL, &is_float);
            if (ret < 0)
                goto RETURN_LABEL;
        }

        ret = check_numeric_bound(src, res->valuetype, res->attrdesc.len);
        if (ret == RET_FALSE)
        {
            ret = SQL_E_TOOLARGEVALUE;
            goto RETURN_LABEL;
        }
    }

    if (is_init)
        return RET_SUCCESS;

    if (IS_NUMERIC(res->valuetype) && !src->is_null &&
            (src->valuetype == DT_CHAR || src->valuetype == DT_VARCHAR))
    {
        ret = mdb_check_numeric(src->u.ptr, NULL, &is_float);
        if (ret < 0)
            goto RETURN_LABEL;
    }

    switch (res->valuetype)
    {
    case DT_TINYINT:
        switch (src->valuetype)
        {
        case DT_TINYINT:
        case DT_SMALLINT:
        case DT_INTEGER:
        case DT_BIGINT:
        case DT_FLOAT:
        case DT_DOUBLE:
        case DT_DECIMAL:
        case DT_CHAR:
        case DT_VARCHAR:
        case DT_DATETIME:
        case DT_DATE:
        case DT_TIME:
        case DT_TIMESTAMP:
            GET_INT_VALUE2(res->u.i8, tinyint, src, is_float);
            break;
        default:
            ret = SQL_E_INCONSISTENTTYPE;
            goto RETURN_LABEL;
        }
        break;
    case DT_SMALLINT:
        switch (src->valuetype)
        {
        case DT_TINYINT:
        case DT_SMALLINT:
        case DT_INTEGER:
        case DT_BIGINT:
        case DT_FLOAT:
        case DT_DOUBLE:
        case DT_DECIMAL:
        case DT_CHAR:
        case DT_VARCHAR:
        case DT_DATETIME:
        case DT_DATE:
        case DT_TIME:
        case DT_TIMESTAMP:
            GET_INT_VALUE2(res->u.i16, smallint, src, is_float);
            break;
        default:
            ret = SQL_E_INCONSISTENTTYPE;
            goto RETURN_LABEL;
        }
        break;
    case DT_INTEGER:
        switch (src->valuetype)
        {
        case DT_TINYINT:
        case DT_SMALLINT:
        case DT_INTEGER:
        case DT_BIGINT:
        case DT_FLOAT:
        case DT_DOUBLE:
        case DT_DECIMAL:
        case DT_CHAR:
        case DT_VARCHAR:
        case DT_DATETIME:
        case DT_DATE:
        case DT_TIME:
        case DT_TIMESTAMP:
            GET_INT_VALUE2(res->u.i32, int, src, is_float);

            break;
            break;
        default:
            ret = SQL_E_INCONSISTENTTYPE;
            goto RETURN_LABEL;
        }
        break;
    case DT_BIGINT:
        switch (src->valuetype)
        {
        case DT_TINYINT:
        case DT_SMALLINT:
        case DT_INTEGER:
        case DT_BIGINT:
        case DT_FLOAT:
        case DT_DOUBLE:
        case DT_DECIMAL:
        case DT_OID:
        case DT_CHAR:
        case DT_VARCHAR:
        case DT_DATETIME:
        case DT_DATE:
        case DT_TIME:
        case DT_TIMESTAMP:
            GET_INT_VALUE2(res->u.i64, bigint, src, is_float);
            break;
        default:
            ret = SQL_E_INCONSISTENTTYPE;
            goto RETURN_LABEL;
        }
        break;
    case DT_OID:
        switch (src->valuetype)
        {
        case DT_TINYINT:
        case DT_SMALLINT:
        case DT_INTEGER:
        case DT_BIGINT:
        case DT_FLOAT:
        case DT_DOUBLE:
        case DT_DECIMAL:
        case DT_OID:
        case DT_CHAR:
        case DT_VARCHAR:
        case DT_DATETIME:
        case DT_DATE:
        case DT_TIME:
        case DT_TIMESTAMP:
            GET_INT_VALUE2(res->u.oid, OID, src, is_float);
            break;
        default:
            ret = SQL_E_INCONSISTENTTYPE;
            goto RETURN_LABEL;
        }
        break;

    case DT_FLOAT:
        switch (src->valuetype)
        {
        case DT_TINYINT:
        case DT_SMALLINT:
        case DT_INTEGER:
        case DT_BIGINT:
        case DT_FLOAT:
        case DT_DOUBLE:
        case DT_DECIMAL:
        case DT_CHAR:
        case DT_VARCHAR:
        case DT_DATETIME:
        case DT_DATE:
        case DT_TIME:
        case DT_TIMESTAMP:
            GET_FLOAT_VALUE2(res->u.f16, float, src, is_float);

            break;
        default:
            ret = SQL_E_INCONSISTENTTYPE;
            goto RETURN_LABEL;
        }
        break;
    case DT_DOUBLE:
        switch (src->valuetype)
        {
        case DT_TINYINT:
        case DT_SMALLINT:
        case DT_INTEGER:
        case DT_BIGINT:
        case DT_FLOAT:
        case DT_DOUBLE:
        case DT_DECIMAL:
        case DT_CHAR:
        case DT_VARCHAR:
        case DT_DATETIME:
        case DT_DATE:
        case DT_TIME:
        case DT_TIMESTAMP:
            GET_FLOAT_VALUE2(res->u.f32, double, src, is_float);

            break;
        default:
            ret = SQL_E_INCONSISTENTTYPE;
            goto RETURN_LABEL;
        }
        break;
    case DT_DECIMAL:
        switch (src->valuetype)
        {
        case DT_TINYINT:
        case DT_SMALLINT:
        case DT_INTEGER:
        case DT_BIGINT:
        case DT_FLOAT:
        case DT_DOUBLE:
        case DT_DECIMAL:
        case DT_CHAR:
        case DT_VARCHAR:
        case DT_DATETIME:
        case DT_DATE:
        case DT_TIME:
        case DT_TIMESTAMP:
            GET_FLOAT_VALUE2(res->u.dec, decimal, src, is_float);
            break;
        default:
            ret = SQL_E_INCONSISTENTTYPE;
            goto RETURN_LABEL;
        }
        break;
    case DT_NCHAR:
        res->attrdesc.len = base->attrdesc.len;
        if (sql_value_ptrXcalloc(res, base->attrdesc.len + 1) < 0)
        {
            ret = SQL_E_OUTOFMEMORY;
            goto RETURN_LABEL;
        }
        switch (src->valuetype)
        {
        case DT_NCHAR:
        case DT_NVARCHAR:
            sc_wcsncpy(res->u.Wptr, src->u.Wptr, base->attrdesc.len);
            res->u.Wptr[base->attrdesc.len] = '\0';
            res->attrdesc.collation = src->attrdesc.collation;
            break;
        default:
            ret = SQL_E_INCONSISTENTTYPE;
            goto RETURN_LABEL;
        }
        break;
    case DT_NVARCHAR:
        switch (src->valuetype)
        {
            // Refer to the SQL & Programming Reference for malloc size;
        case DT_NCHAR:
        case DT_NVARCHAR:
            if (base->attrdesc.len > src->attrdesc.len
                    || base->attrdesc.len == 0)
                res->attrdesc.len = src->attrdesc.len;
            else
                res->attrdesc.len = base->attrdesc.len;

            if (sql_value_ptrXcalloc(res, res->attrdesc.len + 1) < 0)
            {
                ret = SQL_E_OUTOFMEMORY;
                goto RETURN_LABEL;
            }
            sc_wcsncpy(res->u.Wptr, src->u.Wptr, res->attrdesc.len);
            res->u.Wptr[res->attrdesc.len] = '\0';
            res->attrdesc.collation = src->attrdesc.collation;
            break;
        default:
            ret = SQL_E_INCONSISTENTTYPE;
            goto RETURN_LABEL;
        }

        res->value_len = strlen_func[res->valuetype] (res->u.ptr);
        break;
    case DT_CHAR:
    case DT_VARCHAR:
        /* OID convert */
        if (res->valuetype == DT_VARCHAR &&
                (src->valuetype == DT_OID || src->valuetype == DT_BIGINT) &&
                base->attrdesc.fixedlen == -2)
        {
            res->attrdesc.len = 30;
            if (sql_value_ptrXcalloc(res, res->attrdesc.len + 1) < 0)
            {
                ret = SQL_E_OUTOFMEMORY;
                goto RETURN_LABEL;
            }

            sc_sprintf(res->u.ptr, "%d(%d, %d, %d)",
                    src->u.oid,
                    getSegmentNo(src->u.oid),
                    getPageNo(src->u.oid), OID_GetOffSet(src->u.oid));
            break;
        }

        if (IS_BS(src->valuetype) && base->attrdesc.len == 0)
            res->attrdesc.len = src->attrdesc.len;
        else
            res->attrdesc.len = base->attrdesc.len;

        if (sql_value_ptrXcalloc(res, res->attrdesc.len + 1) < 0)
        {
            ret = SQL_E_OUTOFMEMORY;
            goto RETURN_LABEL;
        }
        switch (src->valuetype)
        {
        case DT_TINYINT:
        case DT_SMALLINT:
        case DT_INTEGER:
        case DT_BIGINT:
        case DT_OID:
        case DT_FLOAT:
        case DT_DOUBLE:
        case DT_DECIMAL:
        case DT_DATETIME:
        case DT_DATE:
        case DT_TIME:
        case DT_TIMESTAMP:
            print_value_as_string(src, res->u.ptr, res->attrdesc.len);
            break;
        case DT_CHAR:
        case DT_VARCHAR:
            print_value_as_string(src, res->u.ptr, res->attrdesc.len);
            res->attrdesc.collation = src->attrdesc.collation;
            break;
        default:
            ret = SQL_E_INCONSISTENTTYPE;
            goto RETURN_LABEL;
        }
        break;
    case DT_BYTE:
    case DT_VARBYTE:
        switch (src->valuetype)
        {
            // Refer to the SQL & Programming Reference for malloc size;
        case DT_BYTE:
        case DT_VARBYTE:
            if (base->attrdesc.len > src->attrdesc.len
                    || base->attrdesc.len == 0)
                res->attrdesc.len = src->attrdesc.len;
            else
                res->attrdesc.len = base->attrdesc.len;

            // 저장 공간은 필드 공간 만큼 잡아 놓아야 한다
            if (sql_value_ptrXcalloc(res, res->attrdesc.len) < 0)
            {
                ret = SQL_E_OUTOFMEMORY;
                goto RETURN_LABEL;
            }

            if (res->attrdesc.len < src->value_len)
                res->value_len = res->attrdesc.len;
            else
                res->value_len = src->value_len;

            sc_memcpy(res->u.ptr, src->u.ptr, res->value_len);
            break;

        default:
            ret = SQL_E_INCONSISTENTTYPE;
            goto RETURN_LABEL;
        }
        break;
    case DT_DATETIME:
        sc_memset(datetime, 0x0, sizeof(datetime));
        ptr = datetime;
        switch (src->valuetype)
        {
        case DT_TINYINT:
        case DT_SMALLINT:
        case DT_INTEGER:
        case DT_BIGINT:
        case DT_FLOAT:
        case DT_DOUBLE:
        case DT_DECIMAL:
            add_interval_by_convert(INTERVAL_DAY, src,
                    datetime, MAX_DATETIME_STRING_LEN);
            break;
        case DT_CHAR:
        case DT_VARCHAR:
            ptr = src->u.ptr;
            break;
        case DT_DATETIME:
            sc_memcpy(res->u.datetime, src->u.datetime,
                    MAX_DATETIME_FIELD_LEN);
            break;
        case DT_DATE:
            date2char(date, src->u.date);
            ptr = date;
            break;
        case DT_TIMESTAMP:
            timestamp2char(timestamp, &src->u.timestamp);
            ptr = timestamp;
            break;
        default:
            ret = SQL_E_INCONSISTENTTYPE;
            goto RETURN_LABEL;
        }
        if (src->valuetype != DT_DATETIME)
        {
            ptr = char2datetime(res->u.datetime, ptr, sc_strlen(ptr));
            if (ptr == NULL)
            {
                res->is_null = 1;
            }
        }
        break;
    case DT_DATE:
        /* fix a uninitialised value */
        sc_memset(date, 0x0, sizeof(date));
        ptr = date;
        switch (src->valuetype)
        {
        case DT_TINYINT:
        case DT_SMALLINT:
        case DT_INTEGER:
        case DT_BIGINT:
        case DT_FLOAT:
        case DT_DOUBLE:
        case DT_DECIMAL:
            add_interval_by_convert(INTERVAL_DAY, src,
                    date, MAX_DATE_STRING_LEN);
            break;
        case DT_CHAR:
        case DT_VARCHAR:
            ptr = src->u.ptr;
            break;
        case DT_DATETIME:
            datetime2char(datetime, src->u.datetime);
            ptr = datetime;
            break;
        case DT_DATE:
            sc_memcpy(res->u.date, src->u.date, MAX_DATE_FIELD_LEN);
            break;
        case DT_TIMESTAMP:
            timestamp2char(timestamp, &src->u.timestamp);
            ptr = timestamp;
            break;
        default:
            ret = SQL_E_INCONSISTENTTYPE;
            goto RETURN_LABEL;
        }
        if (src->valuetype != DT_DATE)
        {
            ptr = char2date(res->u.date, ptr, sc_strlen(ptr));
            if (ptr == NULL)
            {
                res->is_null = 1;
            }
        }
        break;
    case DT_TIME:
        /* fix a uninitialised value */
        sc_memset(time, 0x0, sizeof(time));
        ptr = time;
        switch (src->valuetype)
        {
        case DT_TINYINT:
        case DT_SMALLINT:
        case DT_INTEGER:
        case DT_BIGINT:
        case DT_FLOAT:
        case DT_DOUBLE:
        case DT_DECIMAL:
            ret = add_interval_by_convert(INTERVAL_SECOND, src,
                    time, MAX_TIME_STRING_LEN);
            if (ret < 0)
            {
                res->is_null = 1;
            }
            break;
        case DT_CHAR:
        case DT_VARCHAR:
            ptr = src->u.ptr;
            break;
        case DT_DATETIME:
            datetime2char(datetime, src->u.datetime);
            ptr = datetime;
            break;
        case DT_DATE:
            date2char(date, src->u.date);
            break;
        case DT_TIME:
            sc_memcpy(res->u.time, src->u.time, MAX_TIME_FIELD_LEN);
            break;
        case DT_TIMESTAMP:
            timestamp2char(timestamp, &src->u.timestamp);
            ptr = timestamp;
            break;
        default:
            ret = SQL_E_INCONSISTENTTYPE;
            goto RETURN_LABEL;
        }
        if (src->valuetype != DT_TIME)
        {
            ptr = char2time(res->u.time, ptr, sc_strlen(ptr));
            if (ptr == NULL)
            {
                res->is_null = 1;
            }
        }
        break;
    case DT_TIMESTAMP:
        /* fix a uninitialised value */
        sc_memset(timestamp, 0x0, sizeof(timestamp));
        ptr = timestamp;
        switch (src->valuetype)
        {
        case DT_TINYINT:
        case DT_SMALLINT:
        case DT_INTEGER:
        case DT_BIGINT:
        case DT_FLOAT:
        case DT_DOUBLE:
        case DT_DECIMAL:
            add_interval_by_convert(INTERVAL_DAY, src,
                    timestamp, MAX_TIMESTAMP_STRING_LEN);
            break;
        case DT_CHAR:
        case DT_VARCHAR:
            ptr = src->u.ptr;
            break;
        case DT_DATETIME:
            datetime2char(datetime, src->u.datetime);
            ptr = datetime;
            break;
        case DT_DATE:
            date2char(date, src->u.date);
            ptr = date;
            break;
        case DT_TIMESTAMP:
            sc_memcpy(&res->u.timestamp, &src->u.timestamp,
                    MAX_DATETIME_FIELD_LEN);
            break;
        default:
            ret = SQL_E_INCONSISTENTTYPE;
            goto RETURN_LABEL;
        }
        if (src->valuetype != DT_TIMESTAMP)
        {
            ptr = (char *) char2timestamp(&res->u.timestamp, ptr,
                    sc_strlen(ptr));
            if (ptr == NULL)
            {
                res->is_null = 1;
            }
        }
        break;
    default:
        ret = SQL_E_INCONSISTENTTYPE;
        goto RETURN_LABEL;
    }


    ret = RET_SUCCESS;

  RETURN_LABEL:
    if (ret < 0)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
    }

    return ret;
}

int CSizeOfType[] = {
    0,  /*_NULL_TYPE, */
    CSIZE_CHAR,     /*_BOOL, */
    CSIZE_TINYINT,  /*_TINYINT, */
    CSIZE_SMALLINT, /*_SMALLINT, */
    CSIZE_INT,      /*_INTEGER, */
    CSIZE_INT,      /*_LONG, */
    CSIZE_BIGINT,   /*_BIGINT, */
    CSIZE_FLOAT,    /*_FLOAT, */
    CSIZE_DOUBLE,   /*_DOUBLE, */
    CSIZE_DECIMAL,  /*_DECIMAL, */
    CSIZE_CHAR,     /*_CHAR, */
    0,  /*_NCHAR, */
    0,  /*_VARCHAR, */
    0,  /*_NVARCHAR, */
    0,  /*_BYTE, */
    0,  /*_VARBYTE, */
    0,  /*_TEXT, */
    0,  /*_BLOB, */
    MAX_TIMESTAMP_STRING_LEN,  /*_TIMESTAMP, */
    MAX_DATETIME_STRING_LEN,  /*_DATETIME, */
    MAX_DATE_STRING_LEN,  /*_DATE, */
    MAX_TIME_STRING_LEN,  /*_TIME, */
    0 /*HEX */ ,
    CSIZE_BIGINT
};

/* bigint, double에서는 두 수의 차이가 int 범위를 넘는 것을 고려! */
int
compare_values(T_VALUEDESC * val1, T_VALUEDESC * val2, int len)
{
    switch (val1->valuetype)
    {
    case DT_TINYINT:
        return (val1->u.i8 == val2->u.i8) ?
                0 : (val1->u.i8 < val2->u.i8) ? -1 : 1;
        break;
    case DT_SMALLINT:
        return (val1->u.i16 == val2->u.i16) ?
                0 : (val1->u.i16 < val2->u.i16) ? -1 : 1;
        break;
    case DT_INTEGER:
        return (val1->u.i32 == val2->u.i32) ?
                0 : (val1->u.i32 < val2->u.i32) ? -1 : 1;
        break;
    case DT_BIGINT:
        return (val1->u.i64 == val2->u.i64) ?
                0 : (val1->u.i64 < val2->u.i64) ? -1 : 1;
        break;
    case DT_FLOAT:
        return (val1->u.f16 == val2->u.f16) ?
                0 : (val1->u.f16 < val2->u.f16) ? -1 : 1;
        break;
    case DT_DOUBLE:
        return (val1->u.f32 == val2->u.f32) ?
                0 : (val1->u.f32 < val2->u.f32) ? -1 : 1;
        break;
    case DT_DECIMAL:
        return (val1->u.dec == val2->u.dec) ?
                0 : (val1->u.dec < val2->u.dec) ? -1 : 1;
        break;
    case DT_CHAR:
    case DT_NCHAR:
    case DT_VARCHAR:
    case DT_NVARCHAR:
#if defined(MDB_DEBUG)
        if (val1->attrdesc.collation != val1->attrdesc.collation)
            sc_assert(0, __FILE__, __LINE__);
#endif // MDB_DEBUG
        return sc_ncollate_func[val1->attrdesc.collation] (val1->u.ptr,
                val2->u.ptr, len);
    case DT_DATETIME:
        return sc_memcmp(val1->u.datetime, val2->u.datetime,
                MAX_DATETIME_FIELD_LEN - 1);
        break;
    case DT_DATE:
        return sc_memcmp(val1->u.date, val2->u.date, MAX_DATE_FIELD_LEN - 1);
        break;
    case DT_TIME:
        return sc_memcmp(val1->u.time, val2->u.time, MAX_TIME_FIELD_LEN - 1);
        break;
    case DT_TIMESTAMP:
        return (val1->u.timestamp == val2->u.timestamp) ?
                0 : (val1->u.timestamp < val2->u.timestamp) ? -1 : 1;
        break;
    default:
        return -1;
    }
}

/*****************************************************************************/
//! calc_func_decode 

/*! \breif 
 ************************************
 * \param num        :
 * \param vals       : 
 * \param res        :
 ************************************
 * \return RET_SUCCESS or RET_ERROR 
 ************************************
 * \note decode(e, s1, r1, s2, r2, ... , sn, rn, d) 에서\n
 *          e가 s1 ~ sn의 자료형과 호환되는지 확인하고\n
 *          r1 ~ rn과 d의 자료형이 서로 호환하는지 확인한다.
 *          자료형이 모두 호환된다면 e와 si를 비교하고 참이면 ri를\n
 *          반환한다. s1 ~ sn까지 모두 참이 아니면 d를 반환한다.
 *  - collation 미처리된 부분 존재함
 *****************************************************************************/
static int
calc_func_decode(int num, T_VALUEDESC * vals, T_VALUEDESC * res,
        MDB_INT32 is_init)
{
    T_VALUEDESC base, search;
    T_VALUEDESC expr1, expr2;
    ibool found = 0;
    int i, size, comp_result;

    sc_memset(&base, 0, sizeof(T_VALUEDESC));
    sc_memset(&search, 0, sizeof(T_VALUEDESC));

    for (i = 0; i < num; i++)
    {
        if (vals[i].valueclass == SVC_VARIABLE)
        {
            res->valueclass = SVC_VARIABLE;
            break;
        }
    }

    res->valuetype = vals[2].valuetype;
    res->attrdesc.collation = vals[2].attrdesc.collation;
    res->attrdesc.len = vals[2].attrdesc.len;

    for (i = 4; i < num; i += 2)
    {
        if (res->valuetype == DT_NULL_TYPE &&
                vals[i].valuetype != DT_NULL_TYPE)
        {
            res->valuetype = vals[i].valuetype;
            res->attrdesc.collation = vals[i].attrdesc.collation;
        }
        if (IS_ALLOCATED_TYPE(res->valuetype))
        {
            if (IS_ALLOCATED_TYPE(vals[i].valuetype))
                size = vals[i].attrdesc.len;
            else
                size = CSizeOfType[vals[i].valuetype];
            if (res->attrdesc.len < size)
                res->attrdesc.len = size;
        }
    }
    if (!(num & 0x1))
    {
        if (res->valuetype == DT_NULL_TYPE &&
                vals[num - 1].valuetype != DT_NULL_TYPE)
        {
            res->valuetype = vals[num - 1].valuetype;
            res->attrdesc.collation = vals[num - 1].attrdesc.collation;
        }
        if (IS_ALLOCATED_TYPE(res->valuetype))
        {
            if (IS_ALLOCATED_TYPE(vals[num - 1].valuetype))
            {
                size = vals[num - 1].attrdesc.len;
                if (res->attrdesc.len < size)
                    res->attrdesc.len = size;
            }
            else
                size = CSizeOfType[vals[num - 1].valuetype];
            if (res->attrdesc.len < size)
                res->attrdesc.len = size;
        }
    }

    if (res->valuetype == DT_NULL_TYPE)
    {
        res->valuetype = DT_VARCHAR;
        res->attrdesc.collation = DB_Params.default_col_char;
    }

    if (is_init)
    {
        res->is_null = 1;
        return RET_SUCCESS;
    }

    if (vals[0].valuetype != vals[1].valuetype)
    {
        if (vals[0].is_null)
            base.is_null = 1;
        else
        {
            if (vals[1].is_null)
            {
                base.valuetype = DT_VARCHAR;
                base.attrdesc.len = 0;
                base.attrdesc.collation = DB_Params.default_col_char;
                for (i = 3; i < num - 1; i += 2)
                {
                    if (IS_BS_OR_NBS(vals[i].valuetype))
                        size = vals[i].attrdesc.len;
                    else
                        size = CSizeOfType[vals[i].valuetype];
                    if (base.attrdesc.len < size)
                        base.attrdesc.len = size;
                }
                base.call_free = 0;
                base.is_null = 0;
                if (calc_func_convert(&base, &vals[0], &base, MDB_FALSE) < 0)
                    return RET_ERROR;
            }
            else if (IS_NUMERIC(vals[0].valuetype) &&
                    IS_NUMERIC(vals[1].valuetype))
            {
                base.valuetype = DT_DOUBLE;
                base.attrdesc.len = 0;
                base.attrdesc.collation = MDB_COL_NUMERIC;
                base.call_free = 0;
                base.is_null = 0;

                if (calc_func_convert(&base, &vals[0], &base, MDB_FALSE) < 0)
                    return RET_ERROR;
            }
            else
            {
                base.call_free = 0;
                base.is_null = 0;
                base.valuetype = vals[1].valuetype;
                if (IS_BS_OR_NBS(vals[0].valuetype))
                    base.attrdesc.len = vals[0].attrdesc.len;
                else
                    base.attrdesc.len = CSizeOfType[vals[0].valuetype];
                base.attrdesc.collation = vals[1].attrdesc.collation;

                if (calc_func_convert(&base, &vals[0], &base, MDB_FALSE) < 0)
                    return RET_ERROR;
            }
        }
    }
    else
    {
        base = vals[0];
        base.call_free = 0;
    }

    for (i = 1; i < num - 1; i += 2)
    {
        if (base.is_null)
        {
            if (vals[i].is_null)
            {
                if (vals[i + 1].is_null)
                {
                    res->is_null = 1;
                    break;
                }
                else
                {
                    if (calc_func_convert(res, &vals[i + 1], res,
                                    MDB_FALSE) < 0)
                        goto ERROR_LABEL;
                }

                found = 1;
                break;
            }
        }
        else
        {
            if (vals[i].is_null)
                continue;

            if (vals[i].valuetype != base.valuetype)
            {
                search.call_free = 0;
                search.is_null = 0;
                search.valuetype = base.valuetype;
                search.attrdesc.len =
                        base.attrdesc.len == 0 ? 1 : base.attrdesc.len;
                search.attrdesc.collation = base.attrdesc.collation;

                if (calc_func_convert(&search, &vals[i], &search,
                                MDB_FALSE) < 0)
                    goto ERROR_LABEL;
            }
            else
            {
                search = vals[i];
                search.call_free = 0;
            }

            if (base.valuetype == DT_DOUBLE)
            {
                expr1.call_free = 0;
                expr1.is_null = 0;
                expr1.valuetype = DT_VARCHAR;
                expr1.attrdesc.len = CSizeOfType[base.valuetype];
                expr1.attrdesc.collation = DB_Params.default_col_char;
                expr1.valueclass = base.valueclass;

                if (calc_func_convert(&expr1, &base, &expr1, MDB_FALSE) < 0)
                    goto ERROR_LABEL;

                expr2.call_free = 0;
                expr2.is_null = 0;
                expr2.valuetype = DT_VARCHAR;
                expr2.attrdesc.len = CSizeOfType[search.valuetype];
                expr2.attrdesc.collation = DB_Params.default_col_char;
                expr2.valueclass = search.valueclass;

                if (calc_func_convert(&expr2, &search, &expr2, MDB_FALSE) < 0)
                {
                    if (!expr1.is_null)
                        sql_value_ptrFree(&expr1);
                    goto ERROR_LABEL;
                }

                comp_result =
                        compare_values(&expr1, &expr2, expr1.attrdesc.len);

                if (!expr2.is_null)
                    sql_value_ptrFree(&expr2);
                if (!expr1.is_null)
                    sql_value_ptrFree(&expr1);
            }
            else
            {
                if (IS_BYTE(base.valuetype))
                    size = base.value_len > search.value_len ?
                            base.value_len : search.value_len;
                else
                    size = base.attrdesc.len > search.attrdesc.len ?
                            base.attrdesc.len : search.attrdesc.len;
                comp_result =
                        compare_values(&base, &search, size == 0 ? 1 : size);
            }

            if (comp_result == 0)
            {
                found = 1;

                if (vals[i + 1].is_null)
                {
                    res->is_null = 1;
                }
                else
                {
                    if (calc_func_convert(res, &vals[i + 1], res,
                                    MDB_FALSE) < 0)
                        goto ERROR_LABEL;
                }
            }
        }
        if (!search.is_null)
            sql_value_ptrFree(&search);

        if (found)
            break;
    }

    if (!base.is_null)
        sql_value_ptrFree(&base);

    if (!found)
    {
        if (!(num & 0x1))
        {
            if (calc_func_convert(res, &vals[num - 1], res, MDB_FALSE) < 0)
                goto ERROR_LABEL;
        }
        else
            res->is_null = 1;
    }


    if (!res->is_null && IS_N_STRING(res->valuetype))
        res->value_len = strlen_func[res->valuetype] (res->u.ptr);

    return RET_SUCCESS;

  ERROR_LABEL:

    if (!search.is_null)
        sql_value_ptrFree(&search);
    if (!base.is_null)
        sql_value_ptrFree(&base);

    return RET_ERROR;
}

/*****************************************************************************/
//! calc_func_ifnull

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param left(in)      :
 * \param right(in)     : 
 * \param res(in)       :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘 및 기타 설명
 *  - collation 처리됨
 *****************************************************************************/
static int
calc_func_ifnull(T_VALUEDESC * left, T_VALUEDESC * right, T_VALUEDESC * res,
        MDB_INT32 is_init)
{
    int size;

    if (left->valueclass == SVC_VARIABLE || right->valueclass == SVC_VARIABLE)
    {
        res->valueclass = SVC_VARIABLE;
    }

    res->call_free = 0;
    res->is_null = 0;

    if (left->valuetype != DT_NULL_TYPE)
    {
        res->valuetype = left->valuetype;
        res->attrdesc.len = left->attrdesc.len;
        res->attrdesc.collation = left->attrdesc.collation;

        if (IS_ALLOCATED_TYPE(res->valuetype))
        {
            if (IS_ALLOCATED_TYPE(right->valuetype))
                size = right->attrdesc.len;
            else
                size = CSizeOfType[right->valuetype];

            if (res->attrdesc.len < size)
                res->attrdesc.len = size;
        }
        else if (res->valuetype != right->valuetype &&
                IS_NUMERIC(res->valuetype))
        {
            res->valuetype = DT_DOUBLE;
            res->attrdesc.len = CSizeOfType[res->valuetype];
            res->attrdesc.collation = MDB_COL_NUMERIC;
        }
    }
    else if (right->valuetype != DT_NULL_TYPE)
    {
        res->valuetype = right->valuetype;
        res->attrdesc.len = right->attrdesc.len;
        res->attrdesc.collation = right->attrdesc.collation;
    }
    else
    {
        res->valuetype = DT_VARCHAR;
        res->attrdesc.len = right->attrdesc.len;
        res->attrdesc.collation = DB_Params.default_col_char;
    }

    if (is_init)
        return RET_SUCCESS;

    if (left->is_null)
    {
        if (right->is_null)
        {
            res->is_null = 1;
            return RET_SUCCESS;
        }

        if (calc_func_convert(res, right, res, is_init) < 0)
            return RET_ERROR;
    }
    else
    {
        if (calc_func_convert(res, left, res, is_init) < 0)
            return RET_ERROR;
    }

    return RET_SUCCESS;
}

/*****************************************************************************/
//! calc_func_round 

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param fl(in) :
 * \param de(in) : 
 * \param res(in/out) :
 * \param is_init(in) : MDB_TRUE :  res's type check, MDB_FALSE : 실제 수행
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *  - 2번 수행되는 것을 막음
 *  - collation 추가 
 *****************************************************************************/
static int
calc_func_round(T_VALUEDESC * fl, T_VALUEDESC * de,
        T_VALUEDESC * res, MDB_INT32 is_init)
{
    double val;
    int deci;
    unsigned int abs_deci;
    double tmp1, tmp2;

    if (fl->valueclass == SVC_VARIABLE || de->valueclass == SVC_VARIABLE)
    {
        res->valueclass = SVC_VARIABLE;
    }

    res->valuetype = fl->valuetype;
    res->attrdesc.collation = fl->attrdesc.collation;

    if (fl->is_null)
    {
        res->is_null = 1;
        return RET_SUCCESS;
    }

    if (is_init)
    {
        return RET_SUCCESS;
    }

    GET_FLOAT_VALUE(val, double, fl);

    GET_INT_VALUE(deci, int, de);

    abs_deci = sc_abs(deci);

    tmp1 = sc_pow(10.0, abs_deci);
    tmp2 = (deci <
            0 ? sc_rint(val / tmp1) * tmp1 : sc_rint(val * tmp1) / tmp1);

    PUT_FLOAT_VALUE(res, tmp2);

    return RET_SUCCESS;
}

/*****************************************************************************/
//! calc_func_sign 

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param num :
 * \param res : 
 * \param is_init(in) : MDB_TRUE :  res's type check, MDB_FALSE : 실제 수행
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *  - collation 추가
 *****************************************************************************/
static int
calc_func_sign(T_VALUEDESC * num, T_VALUEDESC * res, MDB_INT32 is_init)
{
    res->valueclass = num->valueclass;

    res->valuetype = DT_TINYINT;
    res->attrdesc.type = DT_TINYINT;
    res->attrdesc.collation = MDB_COL_NUMERIC;
    if (is_init)
    {
        return RET_SUCCESS;
    }

    if (num->is_null)
    {
        res->is_null = 1;
        return RET_SUCCESS;
    }

    res->u.i64 = 0;

    switch (num->valuetype)
    {
    case DT_TINYINT:
        if (num->u.i8 < 0)
            res->u.i8 = -1;
        else if (num->u.i8 > 0)
            res->u.i8 = 1;
        break;
    case DT_SMALLINT:
        if (num->u.i16 < 0)
            res->u.i8 = -1;
        else if (num->u.i16 > 0)
            res->u.i8 = 1;
        break;
    case DT_INTEGER:
        if (num->u.i32 < 0)
            res->u.i8 = -1;
        else if (num->u.i32 > 0)
            res->u.i8 = 1;
        break;
    case DT_BIGINT:
        if (num->u.i64 < 0)
            res->u.i8 = -1;
        else if (num->u.i64 > 0)
            res->u.i8 = 1;
        break;
    case DT_FLOAT:
        if (num->u.f16 < 0)
            res->u.i8 = -1;
        else if (num->u.f16 > 0)
            res->u.i8 = 1;
        break;
    case DT_DOUBLE:
        if (num->u.f32 < 0)
            res->u.i8 = -1;
        else if (num->u.f32 > 0)
            res->u.i8 = 1;
        break;
    case DT_DECIMAL:
        if (num->u.dec < 0)
            res->u.i8 = -1;
        else if (num->u.dec > 0)
            res->u.i8 = 1;
        break;
    default:
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, RET_ERROR, 0);
        return RET_ERROR;
    }

    return RET_SUCCESS;
}

/*****************************************************************************/
//! calc_func_trunc 

/*! \breif  SQL FUNCTION 중 TRUNCATE FUNCTION의 루틴
 ************************************
 * \param fl(in) :
 * \param de(in) : 
 * \param res(in/out) :
 * \param is_init(in) : MDB_TRUE :  res's type check, MDB_FALSE : 실제 수행
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *  - is_init 변수 추가
 *  - collation 추가
 *****************************************************************************/
static int
calc_func_trunc(T_VALUEDESC * fl, T_VALUEDESC * de, T_VALUEDESC * res,
        MDB_INT32 is_init)
{
    double val;
    int deci = 0;
    unsigned int abs_deci;
    double tmp1, tmp2;

    if (fl->valueclass == SVC_VARIABLE || de->valueclass == SVC_VARIABLE)
    {
        res->valueclass = SVC_VARIABLE;
    }

    res->valuetype = fl->valuetype;
    res->attrdesc.collation = fl->attrdesc.collation;

    if (fl->is_null)
    {
        res->is_null = 1;
        return RET_SUCCESS;
    }

    if (is_init)
    {
        return RET_SUCCESS;
    }

    GET_FLOAT_VALUE(val, double, fl);

    GET_INT_VALUE(val, double, de);

    abs_deci = sc_abs(deci);

    tmp1 = sc_pow(10.0, abs_deci);
    if (val >= 0)
        tmp2 = (deci <
                0 ? sc_floor(val / tmp1) * tmp1 : sc_floor(val * tmp1) / tmp1);
    else
        tmp2 = (deci <
                0 ? sc_ceil(val / tmp1) * tmp1 : sc_ceil(val * tmp1) / tmp1);

    PUT_FLOAT_VALUE(res, tmp2);

    return RET_SUCCESS;
}

/*****************************************************************************/
//! calc_func_uplowercase 

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param str(in)       :
 * \param res(in/out)   : 
 * \param is_init(in)   : MDB_TRUE :  res's type check, MDB_FALSE : 실제 수행
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *****************************************************************************/
static int
calc_func_uplowercase(int func_type, T_VALUEDESC * str, T_VALUEDESC * res,
        MDB_INT32 is_init)
{
    int ret;

    res->valueclass = str->valueclass;

    res->valuetype = str->valuetype;
    res->attrdesc.collation = str->attrdesc.collation;

    if (func_type != SFT_UPPERCASE && func_type != SFT_LOWERCASE)
        return RET_ERROR;

    if (str->is_null)
    {
        res->is_null = 1;
        return RET_SUCCESS;
    }

    if (IS_BS_OR_NBS(str->valuetype))
        res->attrdesc.len = str->attrdesc.len;

    if (is_init)
        return RET_SUCCESS;

    if (IS_BS_OR_NBS(str->valuetype))
    {
        if (sql_value_ptrXcalloc(res, str->attrdesc.len + 1) < 0)
            return RET_ERROR;

        if (IS_BS(str->valuetype))
        {
            if (func_type == SFT_UPPERCASE)
                ret = sc_strupper(res->u.ptr, str->u.ptr);
            else
                ret = sc_strlower(res->u.ptr, str->u.ptr);
            res->u.ptr[str->attrdesc.len] = '\0';
        }
        else
        {
            if (func_type == SFT_UPPERCASE)
                ret = sc_wcsupper(res->u.Wptr, str->u.Wptr);
            else
                ret = sc_wcslower(res->u.Wptr, str->u.Wptr);
            res->u.Wptr[str->attrdesc.len] = '\0';
        }
        if (ret < 0)
            return RET_ERROR;
        res->value_len = str->value_len;
    }
    else
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_INVALIDTYPE, 0);
        return RET_ERROR;
    }

    return RET_SUCCESS;
}

/*****************************************************************************/
//! calc_func_rltrim 

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param str :
 * \param set : 
 * \param res :
 * \param is_init(in) : MDB_TRUE :  res's type check, MDB_FALSE : 실제 수행
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *  - is_init 추가
 *  - collation 추가
 *****************************************************************************/
static int
calc_func_rltrim(int func_type, T_VALUEDESC * str, T_VALUEDESC * set,
        T_VALUEDESC * res, MDB_INT32 is_init)
{
    int i, j, len, sign, pos;
    T_VALUEDESC temp;
    char a_space[4] = { ' ', 0x0, 0x0, 0x0 };

    if (str->valueclass == SVC_VARIABLE
            || (set && set->valueclass == SVC_VARIABLE))
    {
        res->valueclass = SVC_VARIABLE;
    }

    res->valuetype = str->valuetype;
    res->attrdesc.collation = str->attrdesc.collation;
    if (str->is_null)
    {
        res->is_null = 1;
        return RET_SUCCESS;
    }

    len = str->attrdesc.len;
    if (IS_BS(res->valuetype) && str->u.ptr)
        len = sc_strlen(str->u.ptr);
    else if (IS_NBS(res->valuetype) && str->u.Wptr)
        len = sc_wcslen(str->u.Wptr);

    res->attrdesc.len = len;
    if (is_init)
    {
        if (IS_N_STRING(res->valuetype))
            res->attrdesc.len++;
        return RET_SUCCESS;
    }

    if (!set)
    {
        /* uninitialised value */
        sc_memset(&temp, 0x0, sizeof(T_VALUEDESC));
        temp.u.ptr = a_space;
        set = &temp;
    }

    if (set && set->is_null)
    {
        if (IS_BS(res->valuetype))
        {
            if (sql_value_ptrXcalloc(res, len + 1) < 0)
                return RET_ERROR;
            sc_strncpy(res->u.ptr, str->u.ptr, len);
            res->u.ptr[len] = '\0';
        }
        else if (IS_NBS(res->valuetype))
        {
            if (sql_value_ptrXcalloc(res, len + 1) < 0)
                return RET_ERROR;
            sc_wcsncpy(res->u.Wptr, str->u.Wptr, len);
            res->u.Wptr[len] = '\0';
        }
        else
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_INVALIDTYPE, 0);
            return RET_ERROR;
        }

        res->value_len = strlen_func[res->valuetype] (res->u.ptr);
        return RET_SUCCESS;
    }

    if (func_type == SFT_LTRIM)
    {
        sign = 1;
        pos = 0;
    }
    else if (func_type == SFT_RTRIM)
    {
        sign = -1;
        pos = len - 1;
    }
    else
        return RET_ERROR;

    if (sql_value_ptrXcalloc(res,
                    str->attrdesc.len > 8 ? str->attrdesc.len + 1 : 8 + 1) < 0)
        return RET_ERROR;

    if (IS_NBS(res->valuetype))
    {
        for (i = 0; (pos < len || pos > -1); i++, pos += sign)
        {
            for (j = 0; set->u.Wptr[j]; j++)
                if (str->u.Wptr[pos] == set->u.Wptr[j])
                    break;
            if (!set->u.Wptr[j])
                break;
        }

        if (sign == -1)
        {
            len = pos + 1;
            pos = 0;
        }
        else
            len -= pos;

        sc_wmemcpy(res->u.Wptr, str->u.Wptr + pos, len);
    }
    else
    {
        for (i = 0; (pos < len || pos > -1); i++, pos += sign)
        {
            for (j = 0; set->u.ptr[j]; j++)
                if (str->u.ptr[pos] == set->u.ptr[j])
                    break;
            if (!set->u.ptr[j])
                break;
        }

        if (sign == -1)
        {
            len = pos + 1;
            pos = 0;
        }
        else
        {
            len -= pos;
        }

        sc_memcpy(res->u.ptr, str->u.ptr + pos, len);
    }

    res->value_len = strlen_func[res->valuetype] (res->u.ptr);

    return RET_SUCCESS;
}

/*****************************************************************************/
//! calc_func_hexbinstring 

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param str :
 * \param res : 
 * \param param_name :
 * \param is_init(in) : MDB_TRUE :  res's type check, MDB_FALSE : 실제 수행
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *****************************************************************************/
int
calc_func_hexbinstring(int func_type, T_VALUEDESC * str,
        T_VALUEDESC * res, MDB_INT32 is_init)
{
    int char_size;

    res->valueclass = str->valueclass;

    res->valuetype = DT_VARCHAR;
    res->attrdesc.collation = DB_Params.default_col_char;

    if (func_type == SFT_HEXASTRING)
        char_size = 2;
    else if (func_type == SFT_BINARYSTRING)
        char_size = 8;
    else
        return RET_ERROR;
    res->attrdesc.len = str->attrdesc.len * char_size;

    if (str->is_null)
    {
        res->is_null = 1;
        return RET_SUCCESS;
    }

    if (is_init)
    {
        return RET_SUCCESS;
    }

    if (IS_BYTE(str->valuetype))
    {
        if (sql_value_ptrXcalloc(res, res->attrdesc.len + 1) < 0)
            return RET_ERROR;

        if (func_type == SFT_HEXASTRING)
            mdb_binary2hstring(str->u.ptr, res->u.ptr, str->value_len);
        else
            mdb_binary2bstring(str->u.ptr, res->u.ptr, str->value_len);

        res->u.ptr[str->value_len * char_size] = '\0';
        res->value_len = str->value_len * char_size;
    }
    else
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_INVALIDTYPE, 0);
        return RET_ERROR;
    }

    return RET_SUCCESS;
}

/*****************************************************************************/
//! calc_func_substring 

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param str(in)       :
 * \param fposi(in)     : 
 * \param flen(in)      :
 * \param res(in/out)   :
 * \param is_init(in) : MDB_TRUE :  res's type check, MDB_FALSE : 실제 수행
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *  - is_init 추가
 *  - collation 할당
 *****************************************************************************/
static int
calc_func_substring(T_VALUEDESC * str, T_VALUEDESC * fposi,
        T_VALUEDESC * flen, T_VALUEDESC * res, MDB_INT32 is_init)
{
    char *ptr = NULL;
    char *dest = NULL;

    int posi2cpy, length2cpy;

    bigint val4int = 0;
    double val4flt = 0.0;

    int str_len;
    int ret = RET_SUCCESS;

    if (str->valueclass == SVC_VARIABLE
            || fposi->valueclass == SVC_VARIABLE
            || flen->valueclass == SVC_VARIABLE)
    {
        res->valueclass = SVC_VARIABLE;
    }

    res->valuetype = str->valuetype;
    res->attrdesc.collation = str->attrdesc.collation;

    if (str->is_null)
    {
        res->is_null = 1;
        return RET_SUCCESS;
    }

    if (is_init)
    {       // 초기화인 경우에만.. 이렇게 값을 max로 할당한다
        res->attrdesc.len = str->attrdesc.len;
        return RET_SUCCESS;
    }

    switch (fposi->valuetype)
    {
    case DT_TINYINT:
    case DT_SMALLINT:
    case DT_INTEGER:
    case DT_BIGINT:
        GET_INT_VALUE(val4int, bigint, fposi);
        posi2cpy = (int) val4int;
        break;
    case DT_FLOAT:
    case DT_DOUBLE:
    case DT_DECIMAL:
        GET_FLOAT_VALUE(val4flt, double, fposi);

        posi2cpy = (int) val4flt;
        break;
    default:
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, RET_ERROR, 0);
        return RET_ERROR;
    }
    switch (flen->valuetype)
    {
    case DT_TINYINT:
    case DT_SMALLINT:
    case DT_INTEGER:
    case DT_BIGINT:
        GET_INT_VALUE(val4int, bigint, flen);
        length2cpy = (int) val4int;
        break;
    case DT_FLOAT:
    case DT_DOUBLE:
    case DT_DECIMAL:
        GET_FLOAT_VALUE(val4flt, double, flen);

        length2cpy = (int) val4flt;
        break;
    default:
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, RET_ERROR, 0);
        return RET_ERROR;
    }

    if (IS_BS_OR_NBS(str->valuetype))
    {
        ptr = str->u.ptr;
    }
    else
    {
        if ((ptr = (char *) sql_xcalloc(32, str->valuetype)) == NULL)
            return SQL_E_OUTOFMEMORY;

        print_value_as_string(str, ptr, 32);
    }

    str_len = strlen_func[res->valuetype] (ptr);

    if (posi2cpy)
        posi2cpy--;     // 0부터 시작하도록...

    if (length2cpy < 0)
        length2cpy = str_len - posi2cpy;

    if (str_len < posi2cpy)
    {
        res->is_null = 1;

        if (IS_BS_OR_NBS(res->valuetype))
            res->attrdesc.len = length2cpy;
    }
    else
    {
        int alloc_size = length2cpy;

        if (IS_BS_OR_NBS(str->valuetype))
        {
            if (str->attrdesc.len > length2cpy)
                alloc_size = str->attrdesc.len;

            /*
             * substring(expr, position, length) 
             * substring 하려는 길이(length - position)가 
             * expr의 정의 길이보다 큰 경우에는
             * expr의 정의 길이 + 1 만큼 메모리를 할당하여
             * 메모리의 낭비를 줄인다.
             */
            if (str->attrdesc.len < (length2cpy - (posi2cpy + 1)))
            {
                length2cpy = alloc_size = str->attrdesc.len;
            }
            /* ?COLUMN? 보다는 크게 하기 위해서 */
            if (alloc_size < 8)
                alloc_size = 8;
        }

        dest = sql_xcalloc(alloc_size + 1, res->valuetype);
        if (dest == NULL)
        {
            if (!IS_BS_OR_NBS(str->valuetype))
                PMEM_FREENUL(ptr);

            return SQL_E_OUTOFMEMORY;
        }

        strncpy_func[res->valuetype] (dest,
                ptr + DB_BYTE_LENG(res->valuetype, posi2cpy), length2cpy);

        if (IS_BS_OR_NBS(res->valuetype))
        {
            res->u.ptr = dest;
            res->call_free = 1;
            res->attrdesc.len = strlen_func[res->valuetype] (dest);
            res->value_len = res->attrdesc.len;
        }
        else
        {
            if (make_value_from_string(res, dest, res->valuetype) < 0)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, RET_ERROR, 0);
                ret = RET_ERROR;
            }
        }
    }

    if (!IS_BS_OR_NBS(str->valuetype))
    {
        PMEM_FREENUL(ptr);
        PMEM_FREENUL(dest);
    }

    return ret;
}

/*****************************************************************************/
//! calc_func_chartohex 

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param hex(in)     :
 * \param res(in/out) : 
 * \param is_init(in) : MDB_TRUE :  res's type check, MDB_FALSE : 실제 수행
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *  - is_init 인자 추가
 *  - collation 타입 추가
 *****************************************************************************/
static int
calc_func_chartohex(T_VALUEDESC * hex, T_VALUEDESC * res, MDB_INT32 is_init)
{
    char *src = hex->u.ptr;
    char *dst;
    int len;

    if (hex->is_null)
    {
        res->is_null = 1;
        return RET_SUCCESS;
    }

    /* hex는 항상 constant string이다; parser에서 checking */
    if (src[0] == '0' && (src[1] == 'x' || src[1] == 'X'))      // bypass '0x' prefix
        src += 2;
    len = sc_strlen(src);

    res->valueclass = hex->valueclass;

    res->valuetype = DT_HEX;
    res->attrdesc.len = ((len - 1) >> 1) + 1;
    res->attrdesc.collation = hex->attrdesc.collation;

    if (is_init)
        return RET_SUCCESS;

    if (sql_value_ptrXcalloc(res, (((len - 1) >> 1) + 1)) < 0)
        return SQL_E_OUTOFMEMORY;
    res->call_free = 1;

    dst = res->u.ptr;
    sc_memset(dst, 0, ((len - 1) >> 1) + 1);
    while (*src)
    {
        if ((*src) >= '0' && (*src) <= '9')
            *dst = ((*src) - '0') << 4;
        else
        {
            if ((*src) >= 'a' && (*src) <= 'f')
                *dst = ((*src) - 'a' + 10) << 4;
            else
                *dst = ((*src) - 'A' + 10) << 4;
        }

        if (!*(++src))
            break;

        if ((*src) >= '0' && (*src) <= '9')
            *dst |= ((*src) - '0');
        else
        {
            if ((*src) >= 'a' && (*src) <= 'f')
                *dst |= ((*src) - 'a' + 10);
            else
                *dst |= ((*src) - 'A' + 10);
        }

        ++src;
        ++dst;
    }

    return RET_SUCCESS;
}

/*****************************************************************************/
//! calc_func_hexcomp 

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param arg1(in)      :
 * \param arg2(in)      : 
 * \param res(in/out)   :
 * \param is_init(in)   : MDB_TRUE(res's type check), MDB_FALSE(실제 수행)
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *  - is_init 인자 추가
 *  - collation 추가
 *****************************************************************************/
static int
calc_func_hexcomp(T_VALUEDESC * arg1, T_VALUEDESC * arg2,
        T_VALUEDESC * res, MDB_INT32 is_init)
{
    T_VALUEDESC tmp;
    char *p1, *p2;
    char *p;
    int len1, len2;
    int i, ret;

    if (arg1->valueclass == SVC_VARIABLE || arg2->valueclass == SVC_VARIABLE)
    {
        res->valueclass = SVC_VARIABLE;
    }

    res->valuetype = DT_BOOL;
    res->attrdesc.collation = MDB_COL_NUMERIC;
    if (arg1->is_null || arg2->is_null)
    {       // 이럴리가 없지만 혹시나...
        res->u.bl = 0;
        return RET_SUCCESS;
    }

    if (is_init)
        return RET_SUCCESS;

    sc_memset(&tmp, 0, sizeof(T_VALUEDESC));
    if (arg1->valueclass == SVC_VARIABLE)
    {
        if (arg2->valueclass == SVC_VARIABLE)
        {
            if (arg1->attrdesc.len != arg2->attrdesc.len)
            {
                res->u.bl = 0;
                return RET_SUCCESS;
            }
            if (sc_memcmp(arg1->u.ptr, arg2->u.ptr, arg1->attrdesc.len))
                res->u.bl = 0;
            else
                res->u.bl = 1;
        }
        else if (arg2->valueclass == SVC_CONSTANT)
        {
            if (arg2->u.ptr[0] == '0' &&
                    (arg2->u.ptr[1] == 'x' || arg2->u.ptr[1] == 'X'))
                len2 = sc_strlen(arg2->u.ptr) - 2;
            else
                len2 = sc_strlen(arg2->u.ptr);
            if (arg1->attrdesc.len * 2 > len2)
            {
                int before_memory_free = arg2->call_free;

                p = arg2->u.ptr;

                if (sql_value_ptrXcalloc(arg2, arg1->attrdesc.len * 2 + 1) < 0)
                    return RET_ERROR;
                if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X'))
                    sc_memcpy(arg2->u.ptr, p + 2, len2);
                else
                    sc_memcpy(arg2->u.ptr, p, len2);
                for (i = len2; i < arg1->attrdesc.len * 2; i++)
                    arg2->u.ptr[i] = '0';
                arg2->attrdesc.len = arg1->attrdesc.len * 2;
                if (before_memory_free)
                    PMEM_FREENUL(p);    // 기존 메모리 제거
            }

            ret = calc_func_chartohex(arg2, &tmp, is_init);
            if (ret < 0)
                return ret;
            p2 = tmp.u.ptr;
            len2 = tmp.attrdesc.len;
            if (arg1->attrdesc.len != len2)
            {
                PMEM_FREENUL(p2);
                res->u.bl = 0;
                return RET_SUCCESS;
            }
            if (sc_memcmp(arg1->u.ptr, p2, len2))
                res->u.bl = 0;
            else
                res->u.bl = 1;
            PMEM_FREENUL(p2);
        }
        else
            return RET_ERROR;
    }
    else if (arg1->valueclass == SVC_CONSTANT)
    {
        if (arg1->u.ptr[0] == '0' &&
                (arg1->u.ptr[1] == 'x' || arg1->u.ptr[1] == 'X'))
            len1 = sc_strlen(arg1->u.ptr) - 2;
        else
            len1 = sc_strlen(arg1->u.ptr);
        if (arg2->valueclass == SVC_VARIABLE)
        {
            if (arg2->attrdesc.len * 2 > len1)
            {
                int before_call_free = arg1->call_free;

                p = arg1->u.ptr;
                if (sql_value_ptrXcalloc(arg1, arg2->attrdesc.len * 2 + 1) < 0)
                    return RET_ERROR;

                if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X'))
                    sc_memcpy(arg1->u.ptr, p + 2, len1);
                else
                    sc_memcpy(arg1->u.ptr, p, len1);
                for (i = len1; i < arg2->attrdesc.len * 2; i++)
                    arg1->u.ptr[i] = '0';
                arg1->attrdesc.len = arg2->attrdesc.len * 2;
                if (before_call_free)
                    PMEM_FREENUL(p);
            }
            ret = calc_func_chartohex(arg1, &tmp, is_init);
            if (ret < 0)
                return ret;
            p1 = tmp.u.ptr;
            len1 = tmp.attrdesc.len;

            if (len1 != arg2->attrdesc.len)
            {
                PMEM_FREENUL(p1);
                res->u.bl = 0;
                return RET_SUCCESS;
            }
            if (sc_memcmp(p1, arg2->u.ptr, len1))
                res->u.bl = 0;
            else
                res->u.bl = 1;
            PMEM_FREENUL(p1);
        }
        else if (arg2->valueclass == SVC_CONSTANT)
        {
            ret = calc_func_chartohex(arg1, &tmp, is_init);
            if (ret < 0)
                return ret;
            p1 = tmp.u.ptr;
            len1 = tmp.attrdesc.len;

            ret = calc_func_chartohex(arg2, &tmp, is_init);
            if (ret < 0)
                return ret;
            p2 = tmp.u.ptr;
            len2 = tmp.attrdesc.len;
            if (!len1 != len2)
            {
                PMEM_FREENUL(p1);
                PMEM_FREENUL(p2);
                res->u.bl = 0;
                return RET_SUCCESS;
            }
            if (sc_memcmp(p1, p2, len1))
                res->u.bl = 0;
            else
                res->u.bl = 1;
            PMEM_FREENUL(p1);
            PMEM_FREENUL(p2);
        }
        else
        {
            return RET_ERROR;
        }
    }
    else
        return RET_ERROR;

    return RET_SUCCESS;
}

/*****************************************************************************/
//! calc_func_rownum

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param func(in)      :
 * \param res(in/out)   :
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *  - collation 추가
 *****************************************************************************/
int
calc_func_rownum(T_FUNCDESC * func, T_VALUEDESC * res)
{
    res->valueclass = SVC_CONSTANT;
    res->valuetype = DT_INTEGER;
    res->attrdesc.collation = MDB_COL_NUMERIC;

    if (func->rownump)
    {
        res->u.i32 = *func->rownump;
        return RET_SUCCESS;
    }
    else
        return RET_ERROR;
}

/*****************************************************************************/
//! calc_func_rid

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param func(in)      :
 * \param res(in/out)   :
 * \param is_init(in)   : MDB_TRUE(res's type check), MDB_FALSE(실제 수행)
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *  - collation 추가
 *****************************************************************************/
int
calc_func_rid(T_VALUEDESC * arg, T_VALUEDESC * res, int is_init)
{
    res->valueclass = SVC_CONSTANT;
    res->valuetype = DT_OID;
    res->attrdesc.collation = MDB_COL_NUMERIC;
    if (is_init)
        return RET_SUCCESS;

    res->is_null = arg->is_null;

    if (arg->is_null)
        res->u.oid = NULL_OID;
    else
        res->u.oid = arg->u.oid;

    return RET_SUCCESS;
}

/*****************************************************************************/
//! calc_func_random

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param res(in/out)   :
 * \param is_init(in)   : MDB_TRUE(res's type check), MDB_FALSE(실제 수행)
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *  - collation 추가
 *****************************************************************************/
int
calc_func_random(T_VALUEDESC * res, int is_init)
{
    res->valueclass = SVC_CONSTANT;
    res->valuetype = DT_INTEGER;
    res->attrdesc.collation = MDB_COL_NUMERIC;

    if (is_init)
        return RET_SUCCESS;

    res->u.i32 = sc_rand();

    return RET_SUCCESS;
}

/*****************************************************************************/
//! calc_func_random

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param seed(in)      :
 * \param res(in/out)   :
 * \param is_init(in)   : MDB_TRUE(res's type check), MDB_FALSE(실제 수행)
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *  - collation 추가
 *****************************************************************************/
int
calc_func_srandom(T_VALUEDESC * seed, T_VALUEDESC * res, int is_init)
{
    if (res)
    {
        res->valueclass = SVC_CONSTANT;
        res->valuetype = DT_BIGINT;
        res->attrdesc.collation = MDB_COL_NUMERIC;
        res->u.i64 = seed->u.i64;
    }

    if (is_init)
        return RET_SUCCESS;

    sc_srand((MDB_UINT32) seed->u.i64);

    return RET_SUCCESS;
}

/*****************************************************************************/
//! calc_func_objectsize

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param obj_type(in)  :
 * \param obj(in)       :
 * \param res(in/out)   :
 * \param is_init(in)   : MDB_TRUE(res's type check), MDB_FALSE(실제 수행)
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *  - collation 추가
 *****************************************************************************/
int
calc_func_objectsize(T_VALUEDESC * obj_type, T_VALUEDESC * obj,
        T_VALUEDESC * res, int is_init)
{
    int ret = -1;
    int objsize = 0;

    res->valueclass = SVC_CONSTANT;
    res->valuetype = DT_INTEGER;
    res->attrdesc.collation = MDB_COL_NUMERIC;

    if (obj->is_null)
    {
        res->is_null = 1;
        return RET_SUCCESS;
    }

    if (!IS_BS(obj->valuetype))
        return RET_ERROR;

    if (is_init)
        return RET_SUCCESS;

    switch (obj_type->u.i32)
    {
    case OBJ_TABLE:
        ret = dbi_Table_Size(-1, obj->u.ptr);
        if (ret < 0)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
            return ret;
        }
        objsize = ret;

        ret = dbi_Index_Size(-1, obj->u.ptr);
        if (ret < 0)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
            return ret;
        }
        objsize += ret;

        break;

    case OBJ_TABLEDATA:
        ret = dbi_Table_Size(-1, obj->u.ptr);
        if (ret < 0)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
            return ret;
        }
        objsize = ret;

        break;

    case OBJ_TABLEINDEX:
        ret = dbi_Index_Size(-1, obj->u.ptr);
        if (ret < 0)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
            return ret;
        }
        objsize = ret;

        break;

    default:
        return ret;
    }

    if (res)
    {
        res->valueclass = SVC_CONSTANT;
        res->valuetype = DT_INTEGER;
        res->u.i32 = objsize;
    }

    return RET_SUCCESS;
}

/*****************************************************************************/
//! calc_func_sequence_currval

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param src(in)       :
 * \param res(in/out)   :
 * \param is_init(in)   : MDB_TRUE(res's type check), MDB_FALSE(실제 수행)
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *  - collation 추가
 *****************************************************************************/
int
calc_func_sequence_currval(T_VALUEDESC * str, T_VALUEDESC * res,
        MDB_INT32 is_init)
{
    int currVal = -1;
    int ret = -1;

    if (!IS_BS(str->valuetype))
        return RET_ERROR;

    res->valueclass = SVC_CONSTANT;
    res->valuetype = DT_INTEGER;
    res->attrdesc.collation = MDB_COL_NUMERIC;

    if (is_init)
    {
        return RET_SUCCESS;
    }
    else
    {
        ret = dbi_Sequence_Currval(-1, str->u.ptr, &currVal);
        if (ret < 0)
            return RET_ERROR;

        res->u.i32 = currVal;
    }

    return RET_SUCCESS;
}

/*****************************************************************************/
//! calc_func_sequence_nextval

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param src(in)       :
 * \param res(in/out)   :
 * \param is_init(in)   : MDB_TRUE(res's type check), MDB_FALSE(실제 수행)
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *  - collation 추가
 *****************************************************************************/
int
calc_func_sequence_nextval(T_VALUEDESC * str, T_VALUEDESC * res,
        MDB_INT32 is_init)
{
    int nextVal = -1;
    int ret = -1;

    if (!IS_BS(str->valuetype))
        return RET_ERROR;

    res->valueclass = SVC_CONSTANT;
    res->valuetype = DT_INTEGER;
    res->attrdesc.collation = MDB_COL_NUMERIC;

    if (is_init)
        return RET_SUCCESS;

    ret = dbi_Sequence_Nextval(-1, str->u.ptr, &nextVal);
    if (ret < 0)
        return RET_ERROR;

    if (res)
    {
        res->u.i32 = nextVal;
    }

    return RET_SUCCESS;
}

/*****************************************************************************/
//! calc_func_replace

/*! \breif  replace()가 수행되는 경우 사용되는 함수임
 ************************************
 * \param str(in)          : Source String
 * \param from_str(in)  : 변화시키고 싶은 문자열
 * \param to_str(in)      : 변화시킬 문자열 
 * \param res(out)        : source string이 변환된 문자열
 * \param is_init(in)   : MDB_TRUE(결과값을 계산), MDB_FALSE(실제 계산)
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *  - second week
 *****************************************************************************/
static int
calc_func_replace(T_VALUEDESC * str, T_VALUEDESC * from_str,
        T_VALUEDESC * to_str, T_VALUEDESC * res, MDB_INT32 is_init)
{
    int str_len = 0;
    int from_str_len = 0;
    int to_str_len = 0;
    int res_len = 0;
    int match_cnt = 0;
    char *src = NULL;
    char *old_src = NULL;
    char *dest = NULL;
    int move_len = 0;

    res->valuetype = str->valuetype;
    res->attrdesc.collation = str->attrdesc.collation;

    if (str->is_null)
    {
        res->is_null = 1;
        return RET_SUCCESS;
    }

    if (str->valueclass == SVC_VARIABLE
            || from_str->valueclass == SVC_VARIABLE
            || to_str->valueclass == SVC_VARIABLE)
    {
        res->valueclass = SVC_VARIABLE;
    }
    res->attrdesc.len = str->attrdesc.len * to_str->attrdesc.len;

    if (is_init)
        return RET_SUCCESS;

    str_len = strlen_func[str->valuetype] (str->u.ptr);

    if (!from_str->is_null)
    {
        src = str->u.ptr;
        from_str_len = strlen_func[from_str->valuetype] (from_str->u.ptr);
        if (from_str_len > 0)
        {
            while ((src = strstr_func[str->valuetype] (src, from_str->u.ptr))
                    != NULL)
            {
                ++match_cnt;
                if (IS_NBS(str->valuetype))
                    src += (sizeof(DB_WCHAR) * from_str_len);
                else
                    src += from_str_len;
            }
        }

        if (to_str->is_null)
        {
            if (match_cnt)
            {
                res->is_null = 1;
                return RET_SUCCESS;
            }
        }
        else
            to_str_len = strlen_func[to_str->valuetype] (to_str->u.ptr);
    }

    res_len = str_len + (match_cnt * (to_str_len - from_str_len));

    res->attrdesc.len = res_len;
    if (!sql_value_ptrXcalloc(res, res_len + 1))
        return RET_ERROR;

    old_src = src = str->u.ptr;
    dest = res->u.ptr;
    if (!from_str->is_null)
    {
        if (from_str_len > 0)
        {
            while ((src = strstr_func[str->valuetype] (src, from_str->u.ptr))
                    != NULL)
            {
                move_len = (src - old_src);
                if (IS_NBS(str->valuetype))
                    move_len = move_len / sizeof(DB_WCHAR);
                memcpy_func[str->valuetype] (dest, old_src, move_len);

                if (IS_NBS(str->valuetype))
                    dest += (sizeof(DB_WCHAR) * move_len);
                else
                    dest += move_len;

                memcpy_func[str->valuetype] (dest, to_str->u.ptr, to_str_len);
                if (IS_NBS(str->valuetype))
                    dest += (sizeof(DB_WCHAR) * to_str_len);
                else
                    dest += to_str_len;

                if (IS_NBS(str->valuetype))
                    src += (sizeof(DB_WCHAR) * from_str_len);
                else
                    src += from_str_len;
                old_src = src;
            }
        }
    }

    if (IS_NBS(str->valuetype))
    {
        /*
         * Assumption1:
         *  - old_src is UCS style, "...., 0x00, 0x00"
         * Assumption2:
         *  - the size of dest buffer is enough to copy old_src.
         */
        if (!((old_src == NULL) && ((old_src + 1) == NULL)))
        {
            strcpy_func[str->valuetype] (dest, old_src);
        }
    }
    else
    {
        if (*old_src)   // replace('1234', '1', 'aa')인 경우.. '234' 복사
        {
            strcpy_func[str->valuetype] (dest, old_src);
        }
    }

    res->value_len = strlen_func[res->valuetype] (res->u.ptr);

    return RET_SUCCESS;
}

/*****************************************************************************/
//! calc_func_byte_size

/*! \breif  COLUMNSD의 필드에서 실제 사용 bytes를 리턴함
 *         
 ************************************
 * \param res(in/out)   :
 * \param is_init(in)   : MDB_TRUE :  res's type check, MDB_FALSE : 실제 수행
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *  - function 추가
 *  - collation 할당
 *****************************************************************************/
static int
calc_func_byte_size(T_VALUEDESC * arg1, T_VALUEDESC * res, MDB_INT32 is_init)
{
    res->valueclass = arg1->valueclass;

    res->valuetype = DT_INTEGER;
    res->attrdesc.collation = MDB_COL_NUMERIC;
    if (is_init)
    {
        return RET_SUCCESS;
    }

    // checking routine에서 걸러주지 못함
#ifdef MDB_DEBUG
    if (!IS_BYTE(arg1->valuetype) && !arg1->is_null)
        sc_assert(0, __FILE__, __LINE__);
#endif

    if (arg1->is_null)
        res->is_null = 1;
    else
        res->u.i32 = arg1->value_len;

    return RET_SUCCESS;
}

/*****************************************************************************/
//! calc_func_copyfrom

/*! \breif  column의 값을 file에 write 함
 ************************************
 * \param col(in)       : column's name
 * \param file(in)      : file's name
 * \param is_init(in)   : MDB_TRUE :  res's type check, MDB_FALSE : 실제 수행
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *  - function 추가
 *  - byte의 collation 추가
 *  - DT_VARCHAR도 사용가능 하도록 수정함
 *****************************************************************************/
static int
calc_func_copyfrom(T_VALUEDESC * file, T_VALUEDESC * type, T_VALUEDESC * res,
        MDB_INT32 is_init)
{
    int fd = -1, length;

    if (!mdb_strcmp(type->u.ptr, "char"))
    {
        res->valuetype = DT_CHAR;
        res->attrdesc.collation = DB_Params.default_col_char;
    }
    else if (!mdb_strcmp(type->u.ptr, "varchar"))
    {
        res->valuetype = DT_VARCHAR;
        res->attrdesc.collation = DB_Params.default_col_char;
    }
    else if (!mdb_strcmp(type->u.ptr, "byte"))
    {
        res->valuetype = DT_BYTE;
        res->attrdesc.collation = MDB_COL_DEFAULT_BYTE;
    }
    else if (!mdb_strcmp(type->u.ptr, "varbyte"))
    {
        res->valuetype = DT_VARBYTE;
        res->attrdesc.collation = MDB_COL_DEFAULT_BYTE;
    }
    else
        return RET_ERROR;

    if (file->is_null)
    {
        res->is_null = 1;
        return RET_SUCCESS;
    }

    if (is_init)
        return RET_SUCCESS;

    // path 처리를 해주어야 함
    fd = sc_open(file->u.ptr, O_RDONLY, OPEN_MODE);
    if (fd < 0)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                SQL_E_INVALID_BYTE_FILE_PATH, 0);
        return RET_ERROR;
    }

    // 뒤에 NULL이 포함 되는듯
    length = sc_lseek(fd, 0, SEEK_END);

    // A. length is Zero
    if (length)
    {
        if (sql_value_ptrXcalloc(res, length + 1) < 0)
        {
            sc_close(fd);
            return RET_ERROR;
        }
        sc_lseek(fd, 0, SEEK_SET);
        sc_read(fd, res->u.ptr, length);
        res->value_len = length;
    }
    else
    {
        res->value_len = 0;
        res->u.ptr = NULL;
    }

    sc_close(fd);

    return RET_SUCCESS;
}

/*****************************************************************************/
//! calc_func_copyto

/*! \breif  column의 값을 file에 write 함
 ************************************
 * \param col(in)       : column's name
 * \param file(in)      : file's name
 * \param is_init(in)   : MDB_TRUE :  res's type check, MDB_FALSE : 실제 수행
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *  - function 추가
 *  - DT_VARCHAR도 사용가능 하도록 수정함
 *****************************************************************************/
static int
calc_func_copyto(T_VALUEDESC * col, T_VALUEDESC * file,
        T_VALUEDESC * res, MDB_INT32 is_init)
{
    int fd = -1;

    res->valuetype = DT_INTEGER;
    res->u.i32 = 0;
    res->attrdesc.collation = MDB_COL_NUMERIC;

    if (col->is_null || file->is_null)
    {
        res->is_null = 1;
        return RET_SUCCESS;
    }

    if (is_init)
    {
        if (file->param_idx || file->valuetype == DT_NULL_TYPE)
        {
            file->attrdesc.len = MDB_FILE_NAME;
            file->attrdesc.collation = DB_Params.default_col_char;
        }
        return RET_SUCCESS;
    }

    // path를 바꿔줄것..
    fd = sc_open(file->u.ptr, O_WRONLY | O_CREAT | O_BINARY, OPEN_MODE);
    if (fd < 0)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                SQL_E_INVALID_BYTE_FILE_PATH, 0);
        return RET_ERROR;
    }

    sc_write(fd, col->u.ptr, col->value_len);
    sc_close(fd);

    res->u.i32 = 1;
    return RET_SUCCESS;
}

#if defined(MDB_DEBUG)
/* return tablename of the rid */
int
calc_func_tablename(T_VALUEDESC * arg, T_VALUEDESC * res, int is_init)
{
    res->valueclass = SVC_CONSTANT;
    res->valuetype = DT_VARCHAR;
    res->attrdesc.len = MAX_TABLE_NAME_LEN;
    res->attrdesc.collation = DB_Params.default_col_char;

    if (is_init)
        return RET_SUCCESS;

    if (sql_value_ptrAlloc(res, (MAX_TABLE_NAME_LEN + 1)) < 0)
        return RET_ERROR;

    res->is_null = arg->is_null;

    if (arg->is_null || dbi_Rid_Tablename(-1, arg->u.oid, res->u.ptr))
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, DB_E_INVALID_RID, 0);
        return DB_E_INVALID_RID;
    }

    return RET_SUCCESS;
}
#endif

/*****************************************************************************/
//! calc_function 

/*! \breif  SQL function이 수행되는 부분 
 ************************************
 * \param func(in)  :
 * \param res(in/out)   : 
 * \param args(in)  :
 * \param is_init(in)  : MDB_TRUE or MDB_FALSE
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘 및 기타 설명
 *  - SEQUENCE 추가
 *  - REPLACE() 추가
 *  - arg가 (T_POSTFIXELEM*)에서 (T_POSTFIXELEM**)로 변화
 *  - execution 시에 CURRENT_TIME_STAMP/SYSDATE가 수행되도록 수정
 *  - BYTE_SIZE(), COPYFROM(), COPYTO() 함수 추가
 *****************************************************************************/
int
calc_function(T_FUNCDESC * func, T_POSTFIXELEM * res, T_POSTFIXELEM ** args,
        MDB_INT32 is_init)
{
    T_VALUEDESC *vals, *res_val;
    int ret;
    int i;

    if (check_function(func, args) < 0)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_INVALIDTYPE, 0);
        return SQL_E_INVALIDTYPE;
    }

    res->elemtype = SPT_VALUE;
    res->u.value.valueclass = SVC_CONSTANT;

    res_val = &res->u.value;
    res_val->call_free = 0;
    res_val->is_null = 0;

    switch (func->type)
    {
    case SFT_CONVERT:
        ret = calc_func_convert(&args[0]->u.value, &args[1]->u.value,
                res_val, is_init);
        break;
    case SFT_DECODE:
        vals = (T_VALUEDESC *) PMEM_ALLOC(sizeof(T_VALUEDESC) * func->argcnt);
        if (!vals)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
            return RET_ERROR;
        }

        for (i = 0; i < func->argcnt; i++)
            vals[i] = args[i]->u.value;

        ret = calc_func_decode(func->argcnt, vals, res_val, is_init);
        PMEM_FREENUL(vals);

        break;
    case SFT_IFNULL:
        ret = calc_func_ifnull(&args[0]->u.value, &args[1]->u.value,
                res_val, is_init);
        break;
    case SFT_ROUND:
        ret = calc_func_round(&args[0]->u.value, &args[1]->u.value,
                res_val, is_init);
        break;
    case SFT_SIGN:
        ret = calc_func_sign(&args[0]->u.value, res_val, is_init);
        break;
    case SFT_TRUNC:
        ret = calc_func_trunc(&args[0]->u.value, &args[1]->u.value,
                res_val, is_init);
        break;
    case SFT_UPPERCASE:
    case SFT_LOWERCASE:
        ret = calc_func_uplowercase(func->type, &args[0]->u.value,
                res_val, is_init);
        break;
    case SFT_LTRIM:
    case SFT_RTRIM:
        if (func->argcnt == 1)
            vals = NULL;
        else
            vals = &args[1]->u.value;
        ret = calc_func_rltrim(func->type, &args[0]->u.value, vals,
                res_val, is_init);
        break;
    case SFT_HEXASTRING:
    case SFT_BINARYSTRING:
        ret = calc_func_hexbinstring(func->type, &args[0]->u.value,
                res_val, is_init);
        break;
    case SFT_SUBSTRING:
        ret = calc_func_substring(&args[0]->u.value, &args[1]->u.value,
                &args[2]->u.value, res_val, is_init);
        break;
    case SFT_CURRENT_DATE:
        ret = calc_func_current_date(res_val, is_init);
        break;
    case SFT_CURRENT_TIME:
        ret = calc_func_current_time(res_val, is_init);
        break;
    case SFT_CURRENT_TIMESTAMP:
        ret = calc_func_current_timestamp(res_val, is_init);
        break;
    case SFT_DATE_ADD:
        ret = calc_func_date_add(&args[0]->u.value, &args[1]->u.value,
                &args[2]->u.value, res_val, is_init);
        break;
    case SFT_DATE_DIFF:
        ret = calc_func_date_diff(&args[0]->u.value, &args[1]->u.value,
                &args[2]->u.value, res_val, is_init);
        break;
    case SFT_DATE_FORMAT:
        ret = calc_func_date_format(&args[0]->u.value,
                &args[1]->u.value, res_val, is_init);
        break;
    case SFT_DATE_SUB:
        ret = calc_func_date_sub(&args[0]->u.value, &args[1]->u.value,
                &args[2]->u.value, res_val, is_init);
        break;
    case SFT_TIME_ADD:
        ret = calc_func_time_add(&args[0]->u.value, &args[1]->u.value,
                &args[2]->u.value, res_val, is_init);
        break;
    case SFT_TIME_DIFF:
        ret = calc_func_time_diff(&args[0]->u.value, &args[1]->u.value,
                &args[2]->u.value, res_val, is_init);
        break;
    case SFT_TIME_FORMAT:
        ret = calc_func_time_format(&args[0]->u.value,
                &args[1]->u.value, res_val, is_init);
        break;
    case SFT_TIME_SUB:
        ret = calc_func_time_sub(&args[0]->u.value, &args[1]->u.value,
                &args[2]->u.value, res_val, is_init);
        break;
    case SFT_NOW:
    case SFT_SYSDATE:
        ret = calc_func_sysdate(res_val, is_init);
        break;
    case SFT_CHARTOHEX:
        ret = calc_func_chartohex(&args[0]->u.value, res_val, is_init);
        break;
    case SFT_HEXCOMP:
        ret = calc_func_hexcomp(&args[0]->u.value, &args[1]->u.value,
                res_val, is_init);
        break;
    case SFT_HEXNOTCOMP:
        ret = calc_func_hexcomp(&args[0]->u.value, &args[1]->u.value,
                res_val, is_init);
        res_val->u.bl = !res_val->u.bl;
        break;
    case SFT_RANDOM:
        ret = calc_func_random(res_val, is_init);
        break;
    case SFT_SRANDOM:
        ret = calc_func_srandom(&args[0]->u.value, res_val, is_init);
        break;
    case SFT_OBJECTSIZE:
        ret = calc_func_objectsize(&args[0]->u.value,
                &args[1]->u.value, res_val, is_init);
        break;
    case SFT_ROWNUM:
        ret = calc_func_rownum(func, res_val);
        break;
    case SFT_SEQUENCE_CURRVAL:
        ret = calc_func_sequence_currval(&args[0]->u.value, res_val, is_init);
        break;
    case SFT_SEQUENCE_NEXTVAL:
        ret = calc_func_sequence_nextval(&args[0]->u.value, res_val, is_init);
        break;
    case SFT_RID:
        ret = calc_func_rid(&args[0]->u.value, res_val, is_init);
        break;
    case SFT_REPLACE:
        ret = calc_func_replace(&args[0]->u.value, &args[1]->u.value,
                &args[2]->u.value, res_val, is_init);
        break;
    case SFT_BYTESIZE:
        ret = calc_func_byte_size(&args[0]->u.value, res_val, is_init);
        break;
    case SFT_COPYFROM:
        ret = calc_func_copyfrom(&args[0]->u.value, &args[1]->u.value,
                res_val, is_init);
        break;
    case SFT_COPYTO:
        ret = calc_func_copyto(&args[0]->u.value, &args[1]->u.value,
                res_val, is_init);
        break;
    case SFT_TABLENAME:
#if defined(MDB_DEBUG)
        ret = calc_func_tablename(&args[0]->u.value, res_val, is_init);
        break;
#endif
    default:
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, RET_ERROR, 0);
        return RET_ERROR;
    }

    if (ret < 0)
        return ret;

    return RET_SUCCESS;
}

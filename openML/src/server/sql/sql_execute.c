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
#include "sql_main.h"

#include "sql_execute.h"

#include "sql_decimal.h"
#include "sql_func_timeutil.h"
#include "sql_execute.h"
#include "mdb_er.h"
#include "ErrorCode.h"
#include "mdb_comm_stub.h"
#include "mdb_Server.h"
    /* Dconnector sends query, so increments count here. */

extern int exec_drop(int handle, T_STATEMENT * stmt);
extern int exec_create(int handle, T_STATEMENT * stmt);
extern int exec_delete(int handle, T_STATEMENT * stmt);
extern int exec_insert(int handle, T_STATEMENT * stmt);
extern int exec_select(int handle, T_STATEMENT * stmt, int result_type);
extern int exec_update(int handle, T_STATEMENT * stmt);
extern int exec_rename(int handle, T_STATEMENT * stmt);
extern int exec_alter(int handle, T_STATEMENT * stmt);
extern int exec_truncate(int handle, T_STATEMENT * stmt);
extern int exec_call(int handle, T_STATEMENT * stmt, int result_type);

extern int exec_admin(int handle, T_STATEMENT * stmt);

//////////////////////// Util Functions Start ////////////////////////

extern int calc_equal(T_VALUEDESC *, T_VALUEDESC *, T_VALUEDESC *);
extern int calc_simpleexpr(T_OPDESC * op, T_POSTFIXELEM * res,
        T_POSTFIXELEM ** args, MDB_INT32 is_init);
extern int calc_function(T_FUNCDESC *, T_POSTFIXELEM *, T_POSTFIXELEM **,
        MDB_INT32);
extern int calc_subqexpr(int, T_OPDESC *, T_POSTFIXELEM *, T_POSTFIXELEM **);
extern int calc_func_convert(T_VALUEDESC * base, T_VALUEDESC * src,
        T_VALUEDESC * res, MDB_INT32 is_init);

/* 아래에 존재하던것을 옮김 */
extern int exec_singleton_subq(int handle, T_SELECT * select);
extern int one_binary_column_length(T_RESULTCOLUMN *, int);

extern int (*sc_ncollate_func[]) (const char *s1, const char *s2, int n);

/*****************************************************************************/
//! check_numeric_conversion 

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param value :
 * \param bound : 
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
check_numeric_conversion(T_VALUEDESC * value, MDB_UINT8 * bound)
{
    DataType type, value_type;
    int length;
    bigint val4int = 0;
    double val4flt = 0.0;
    int ret;

    type = value->attrdesc.type;
    length = value->attrdesc.len;
    value_type = value->valuetype;

    if (IS_STRING(type) || IS_STRING(value_type))
        return RET_UNKNOWN;

    if (IS_NBS(type) || IS_NBS(value_type))
        return RET_UNKNOWN;

    switch (value_type)
    {
    case DT_TINYINT:
    case DT_SMALLINT:
    case DT_INTEGER:
    case DT_BIGINT:
        GET_INT_VALUE(val4int, bigint, value);
        CHECK_OVERFLOW(val4int, type, length, ret);
        if (ret == RET_TRUE)
        {
            switch (*bound)
            {
            case MDB_LT:
            case MDB_LE:
                return ((val4int > 0) ? RET_TRUE : RET_FALSE);
            case MDB_GT:
            case MDB_GE:
                return ((val4int < 0) ? RET_TRUE : RET_FALSE);
            case MDB_EQ:
                return RET_FALSE;
            case MDB_NE:
                return RET_TRUE;
            default:
                break;
            }
        }
        break;
    case DT_FLOAT:
    case DT_DOUBLE:
        GET_FLOAT_VALUE(val4flt, double, value);

        CHECK_OVERFLOW(val4flt, type, length, ret);
        if (ret == RET_TRUE)
        {
            switch (*bound)
            {
            case MDB_LT:
            case MDB_LE:
                return ((val4flt > 0) ? RET_TRUE : RET_FALSE);
            case MDB_GT:
            case MDB_GE:
                return ((val4flt < 0) ? RET_TRUE : RET_FALSE);
            case MDB_EQ:
                return RET_FALSE;
            case MDB_NE:
                return RET_TRUE;
            default:
                break;
            }
        }
        CHECK_UNDERFLOW(val4flt, type, length, ret);
        if (IS_INTEGER(type))
        {
            val4int = (bigint) val4flt;
            if (val4flt - val4int)
            {
                switch (*bound)
                {
                case MDB_GT:
                    *bound = MDB_GE;
                    /* fallthrough */
                case MDB_GE:
                    PUT_INT_VALUE(value, val4int + 1);
                    return RET_UNKNOWN;
                case MDB_LT:
                    *bound = MDB_LE;
                    /* fallthrough */
                case MDB_LE:
                    PUT_INT_VALUE(value, val4int);
                    return RET_UNKNOWN;
                case MDB_EQ:
                    return RET_FALSE;
                case MDB_NE:
                    return RET_TRUE;
                default:
                    return RET_FALSE;
                }
            }
        }
        break;
    default:
        break;
    }
    return RET_UNKNOWN;
}

/*****************************************************************************/
//! make_fieldvalue 

/*! \breif  val에 해당하는 FIELDVALUE_T에 값을 할당한다
 ************************************
 * \param val(in)       : T_VALUEDESC(Schema 정보를 담는다)
 * \param fval(in/out)  : FIELDVALUE_T
 * \param bound(in)     :
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - called : make_fieldvalue_for_filter()
 *  - NULL/NOT NULL의 경우를 체크할때.. 이쪽으로 들어올수도 있음
 *  - COLLATION INTERFACE 지원
 *  - CONVERT 할수 없는 TYPE은 ERROR 처리시킴
 *****************************************************************************/
int
make_fieldvalue(T_VALUEDESC * val, FIELDVALUE_T * fval, MDB_UINT8 * bound)
{
    T_VALUEDESC base, result;
    void *resultval;
    char str_dec[MAX_DECIMAL_STRING_LEN + 1];
    int ret;

    // search all column: null + not null
    if (val->valueclass == SVC_NONE && val->valuetype == DT_NULL_TYPE)
    {

        ret = dbi_FieldValue(MDB_AL, val->attrdesc.type,
                val->attrdesc.posi.ordinal,
                val->attrdesc.posi.offset,
                val->attrdesc.len,
                val->attrdesc.dec,
                val->attrdesc.collation, NULL, DT_NULL_TYPE, 0, fval);
        if (ret != DB_SUCCESS)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_INVALIDEXPRESSION,
                    0);
            return RET_ERROR;
        }
        return RET_SUCCESS;
    }

    ret = check_numeric_conversion(val, bound);
    if (ret != RET_UNKNOWN)
        return ret;

    if (val->is_null)
    {
        fval->is_null = 1;
        fval->mode_ = MDB_NN;
        fval->position_ = val->attrdesc.posi.ordinal;
        fval->type_ = val->valuetype;
        fval->offset_ = val->attrdesc.posi.offset;
        fval->length_ = val->attrdesc.len;
        fval->value_length_ = 0;
        fval->fixed_part = -1;
        fval->collation = val->attrdesc.collation;
        fval->scale_ = 0;

        return RET_SUCCESS;
    }
    else
        switch (val->attrdesc.type)
        {
        case DT_NVARCHAR:      // nstring
        case DT_NCHAR: // nbytes
            if (!IS_NBS(val->valuetype))
                goto type_mismatch;
            break;
        case DT_VARCHAR:       // string
        case DT_CHAR:  // bytes
            if (!IS_BS(val->valuetype))
                goto type_mismatch;
            break;
        case DT_TINYINT:       // tinyint
        case DT_SMALLINT:      // smallint
        case DT_INTEGER:       // integer    
        case DT_BIGINT:        // bigint
        case DT_FLOAT: // float
        case DT_DOUBLE:        // double
        case DT_DECIMAL:
            if (!IS_NUMERIC(val->valuetype) && val->valuetype != DT_VARCHAR)
                goto type_mismatch;
            break;
        case DT_DATETIME:      // datetime
        case DT_DATE:
        case DT_TIMESTAMP:     // timestamp
            if (!IS_NUMERIC(val->valuetype) && !IS_BS(val->valuetype)
                    && val->valuetype != DT_DATETIME
                    && val->valuetype != DT_DATE
                    && val->valuetype != DT_TIMESTAMP)
                goto type_mismatch;
            break;
        case DT_TIME:
            /* 
             *  DT_TIME 은 일단 
             *  NUMERIC type 과 DT_CHAR, DT_VARCHAR 로 변환되는 것으로 함.
             *  DT_DATETIME, DT_TIMESTAMP 로 변환안됨.
             */
            if (!IS_NUMERIC(val->valuetype) && !IS_BS(val->valuetype)
                    && val->valuetype != DT_TIME)
                goto type_mismatch;
            break;
        default:       // vchar, blob, time, date
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                    SQL_E_INVALIDEXPRESSION, 0);
            return RET_ERROR;
        }

    if (IS_BS_OR_NBS(val->attrdesc.type))
    {
        resultval = val->u.ptr;
    }
    /* 동일한 type인 경우 convert가 필요 없음 */
    else if (val->attrdesc.type == val->valuetype)
    {
        if (val->attrdesc.type == DT_DECIMAL)
        {
            decimal2char(str_dec, val->u.dec,
                    val->attrdesc.len, val->attrdesc.dec);
            resultval = str_dec;
        }
        else
        {
            resultval = &val->u;
        }
    }
    else
    {
        base.valuetype = val->attrdesc.type;
        base.attrdesc.len = val->attrdesc.len;
        base.valueclass = SVC_NONE;
        if (base.valuetype == DT_DECIMAL)
        {
            base.attrdesc.dec = val->attrdesc.dec;
        }
        if (calc_func_convert(&base, val, &result, MDB_FALSE) != RET_SUCCESS)
            goto type_mismatch;
        if (val->attrdesc.type == DT_DECIMAL)
        {
            decimal2char(str_dec, result.u.dec,
                    val->attrdesc.len, val->attrdesc.dec);
            resultval = str_dec;
        }
        else
            resultval = &result.u;
    }

    ret = dbi_FieldValue(MDB_NN, val->attrdesc.type,
            (MDB_UINT16) val->attrdesc.posi.ordinal,
            val->attrdesc.posi.offset, val->attrdesc.len, 0,
            val->attrdesc.collation, resultval,
            val->attrdesc.type, val->attrdesc.len, fval);
    if (ret != DB_SUCCESS)
        return RET_ERROR;


    return RET_SUCCESS;

  type_mismatch:
    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_INCONSISTENTTYPE, 0);
    return RET_ERROR;
}

/*****************************************************************************/
//! make_minfieldvalue

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param attr(in)      : FIELD's Attribution Info
 * \param is_null(in)   : NULL Flag
 * \param fval(in/out)  : FIELDVALUE_T 구조체
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *  - BYTE/VARBYTE는 사용할 수 없다
 *  - COLLATION INTERFACE 지원
 *****************************************************************************/
static int
make_minfieldvalue(T_ATTRDESC * attr, int is_null, FIELDVALUE_T * fval)
{
    T_VALUEDESC value;
    char str_dec[MAX_DECIMAL_STRING_LEN];
    int ret;
    void *resultval;

    switch (attr->type)
    {
    case DT_CHAR:      // bytes
    case DT_VARCHAR:   // string
    case DT_NCHAR:
    case DT_NVARCHAR:  // string
        if (IS_NBS(attr->type))
            value.u.ptr = PMEM_WALLOC(attr->len * sizeof(DB_WCHAR));
        else
            value.u.ptr = PMEM_WALLOC(attr->len);
        if (!value.u.ptr)
            return RET_ERROR;
        set_minvalue(attr->type, attr->len, &value);
        resultval = value.u.ptr;
        break;
    case DT_SMALLINT:  // smallint
    case DT_INTEGER:   // integer
    case DT_FLOAT:     // float
    case DT_DOUBLE:    // double
    case DT_TINYINT:   // tinyint
    case DT_BIGINT:    // bigint
    case DT_DATETIME:  // datetime
    case DT_DATE:      // date
    case DT_TIME:      // time
    case DT_TIMESTAMP: // timestamp
        set_minvalue(attr->type, attr->len, &value);
        resultval = &value.u;
        break;
    case DT_DECIMAL:   // decimal
        set_minvalue(attr->type, attr->len, &value);
        decimal2char(str_dec, value.u.dec, attr->len, attr->dec);
        resultval = str_dec;
        break;
    case DT_BYTE:
    case DT_VARBYTE:
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_INVALIDEXPRESSION, 0);
        return RET_ERROR;
    default:   // vchar, blob, time, date
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_INVALIDEXPRESSION, 0);
        return RET_ERROR;
    }

    ret = dbi_FieldValue((MDB_INT8) ((is_null) ? MDB_NU : MDB_NN),
            attr->type, (MDB_UINT16) attr->posi.ordinal,
            attr->posi.offset, attr->len, 0,
            attr->collation, resultval, attr->type, attr->len, fval);

    if (IS_BS_OR_NBS(attr->type))
        PMEM_FREENUL(value.u.ptr);

    if (ret != DB_SUCCESS)
        return RET_ERROR;

    return RET_SUCCESS;
}

/*****************************************************************************/
//! make_maxfieldvalue

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param attr(in)      : FIELD's Attribution Info
 * \param is_null(in)   : NULL Flag
 * \param fval(in/out)  : FIELDVALUE_T 구조체
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *  - BYTE/VARBYTE는 사용할 수 없다
 *****************************************************************************/
static int
make_maxfieldvalue(T_ATTRDESC * attr, int is_null, FIELDVALUE_T * fval)
{
    T_VALUEDESC value;
    char str_dec[MAX_DECIMAL_STRING_LEN];
    int ret;
    void *resultval;

    switch (attr->type)
    {
    case DT_CHAR:      // bytes
    case DT_VARCHAR:   // string
    case DT_NCHAR:
    case DT_NVARCHAR:  // string
        if (IS_NBS(attr->type))
            value.u.ptr = PMEM_WALLOC(attr->len * sizeof(DB_WCHAR));
        else
            value.u.ptr = PMEM_WALLOC(attr->len);
        if (!value.u.ptr)
            return RET_ERROR;
        set_maxvalue(attr->type, attr->len, &value);
        resultval = value.u.ptr;
        break;
    case DT_SMALLINT:  // smallint
    case DT_INTEGER:   // integer
    case DT_FLOAT:     // float
    case DT_DOUBLE:    // double
    case DT_TINYINT:   // tinyint
    case DT_BIGINT:    // bigint
    case DT_DATETIME:  // datetime
    case DT_DATE:      // date
    case DT_TIME:      // date
    case DT_TIMESTAMP: // timestamp
        set_maxvalue(attr->type, attr->len, &value);
        resultval = &value.u;
        break;
    case DT_DECIMAL:   // decimal
        set_maxvalue(attr->type, attr->len, &value);
        decimal2char(str_dec, value.u.dec, attr->len, attr->dec);
        resultval = str_dec;
        break;
    case DT_BYTE:
    case DT_VARBYTE:
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_INVALIDEXPRESSION, 0);
        return RET_ERROR;
    default:   // vchar, blob, time, date
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_INVALIDEXPRESSION, 0);
        return RET_ERROR;
    }

    ret = dbi_FieldValue((MDB_INT8) ((is_null) ? MDB_NU : MDB_NN),
            attr->type, (MDB_UINT16) attr->posi.ordinal,
            attr->posi.offset, attr->len, 0,
            attr->collation, resultval, attr->type, attr->len, fval);

    if (IS_BS_OR_NBS(attr->type))
        PMEM_FREENUL(value.u.ptr);

    if (ret != DB_SUCCESS)
        return RET_ERROR;

    return RET_SUCCESS;
}

/*****************************************************************************/
//! get_minvalue 

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param res(in/out)   : result column
 * \param val(in)       : value column
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘 및 기타 설명
 *****************************************************************************/
int
get_minvalue(T_RESULTCOLUMN * res, T_VALUEDESC * val)
{
    int val_len;

    if (val->is_null)
        return 0;

    switch (res->value.valuetype)
    {
    case DT_TINYINT:
        if (res->value.is_null || res->value.u.i8 > val->u.i8)
            res->value.u.i8 = val->u.i8;
        else
            return 0;
        break;
    case DT_SMALLINT:
        if (res->value.is_null || res->value.u.i16 > val->u.i16)
            res->value.u.i16 = val->u.i16;
        else
            return 0;
        break;
    case DT_INTEGER:
        if (res->value.is_null || res->value.u.i32 > val->u.i32)
            res->value.u.i32 = val->u.i32;
        else
            return 0;
        break;
    case DT_BIGINT:
        if (res->value.is_null || res->value.u.i64 > val->u.i64)
            res->value.u.i64 = val->u.i64;
        else
            return 0;
        break;
    case DT_FLOAT:
        if (res->value.is_null || res->value.u.f16 > val->u.f16)
            res->value.u.f16 = val->u.f16;
        else
            return 0;
        break;
    case DT_DOUBLE:
        if (res->value.is_null || res->value.u.f32 > val->u.f32)
            res->value.u.f32 = val->u.f32;
        else
            return 0;
        break;
    case DT_DECIMAL:
        if (res->value.is_null || res->value.u.dec > val->u.dec)
            res->value.u.dec = val->u.dec;
        else
            return 0;
        break;
    case DT_VARCHAR:
    case DT_NVARCHAR:
    case DT_CHAR:
    case DT_NCHAR:
#if defined(MDB_DEBUG)
        if (strlen_func[res->value.valuetype] (val->u.ptr) != val->value_len)
            sc_assert(0, __FILE__, __LINE__);
#endif
        val_len = val->value_len;
        if (val_len > res->len)
            res->len = val_len;

        if (res->value.is_null ||
                sc_ncollate_func[val->attrdesc.collation] (res->value.u.ptr,
                        val->u.ptr, res->len) > 0)
        {
            if (res->value.value_len > val_len)
            {
                sc_memset(res->value.u.ptr, 0, res->value.value_len);
            }
            memcpy_func[res->value.valuetype] (res->value.u.ptr,
                    val->u.ptr, res->len);
        }
        else
        {
            res->value.value_len = 0;
            return 0;
        }
        res->value.value_len = val->value_len;

        break;
    case DT_DATETIME:
        if (res->value.is_null ||
                sc_memcmp(res->value.u.datetime,
                        val->u.datetime, MAX_DATETIME_FIELD_LEN) > 0)
            sc_memcpy(res->value.u.datetime,
                    val->u.datetime, MAX_DATETIME_FIELD_LEN);
        else
            return 0;
        break;
    case DT_DATE:
        if (res->value.is_null ||
                sc_memcmp(res->value.u.date, val->u.date,
                        MAX_DATE_FIELD_LEN) > 0)
            sc_memcpy(res->value.u.date, val->u.date, MAX_DATE_FIELD_LEN);
        else
            return 0;
        break;
    case DT_TIME:
        if (res->value.is_null ||
                sc_memcmp(res->value.u.time, val->u.time,
                        MAX_TIME_FIELD_LEN) > 0)
            sc_memcpy(res->value.u.time, val->u.time, MAX_TIME_FIELD_LEN);
        else
            return 0;
        break;
    case DT_TIMESTAMP:
        if (res->value.is_null || res->value.u.timestamp > val->u.timestamp)
            sc_memcpy(&res->value.u.timestamp,
                    &val->u.timestamp, MAX_TIMESTAMP_FIELD_LEN);
        else
            return 0;
        break;
    case DT_BYTE:
    case DT_VARBYTE:
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_INVALIDEXPRESSION, 0);
        return RET_ERROR;
    default:
        return -1;
    }

    if (res->value.is_null)
        res->value.is_null = 0;

    return 1;
}

/*****************************************************************************/
//! get_maxvalue 

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param res(in/out)   : result column
 * \param val(in)       : value column
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘 및 기타 설명
 *  - COLLATION 추가
 *****************************************************************************/
int
get_maxvalue(T_RESULTCOLUMN * res, T_VALUEDESC * val)
{
    int val_len;
    DataType datatype = res->value.valuetype;

    if (val->is_null)
        return 0;

    switch (res->value.valuetype)
    {
    case DT_TINYINT:
        if (res->value.is_null || res->value.u.i8 < val->u.i8)
            res->value.u.i8 = val->u.i8;
        else
            return 0;
        break;
    case DT_SMALLINT:
        if (res->value.is_null || res->value.u.i16 < val->u.i16)
            res->value.u.i16 = val->u.i16;
        else
            return 0;
        break;
    case DT_INTEGER:
        if (res->value.is_null || res->value.u.i32 < val->u.i32)
            res->value.u.i32 = val->u.i32;
        else
            return 0;
        break;
    case DT_BIGINT:
        if (res->value.is_null || res->value.u.i64 < val->u.i64)
            res->value.u.i64 = val->u.i64;
        else
            return 0;
        break;
    case DT_FLOAT:
        if (res->value.is_null || res->value.u.f16 < val->u.f16)
            res->value.u.f16 = val->u.f16;
        else
            return 0;   /* add */
        break;
    case DT_DOUBLE:
        if (res->value.is_null || res->value.u.f32 < val->u.f32)
            res->value.u.f32 = val->u.f32;
        else
            return 0;
        break;
    case DT_DECIMAL:
        if (res->value.is_null || res->value.u.dec < val->u.dec)
            res->value.u.dec = val->u.dec;
        else
            return 0;
        break;
    case DT_CHAR:
    case DT_NCHAR:
    case DT_VARCHAR:
    case DT_NVARCHAR:
#if defined(MDB_DEBUG)
        if (strlen_func[res->value.valuetype] (val->u.ptr) != val->value_len)
            sc_assert(0, __FILE__, __LINE__);
#endif
        val_len = val->value_len;

        if (val_len > res->len)
            res->len = val_len;

        if (res->value.is_null ||
                sc_ncollate_func[val->attrdesc.collation] (res->value.u.ptr,
                        val->u.ptr, res->len) < 0)
        {
            if (res->value.value_len > val_len)
            {
                sc_memset(res->value.u.ptr, 0, res->value.value_len);
            }
            memcpy_func[datatype] (res->value.u.ptr, val->u.ptr, res->len);
        }
        else
        {
            res->value.value_len = 0;
            return 0;
        }

        res->value.value_len = val->value_len;

        break;
    case DT_DATETIME:
        if (res->value.is_null ||
                sc_memcmp(res->value.u.datetime,
                        val->u.datetime, MAX_DATETIME_FIELD_LEN) < 0)
            sc_memcpy(res->value.u.datetime,
                    val->u.datetime, MAX_DATETIME_FIELD_LEN);
        else
            return 0;
        break;
    case DT_DATE:
        if (res->value.is_null || sc_memcmp(res->value.u.date,
                        val->u.date, MAX_DATE_FIELD_LEN) < 0)
            sc_memcpy(res->value.u.date, val->u.date, MAX_DATE_FIELD_LEN);
        else
            return 0;
        break;
    case DT_TIME:
        if (res->value.is_null || sc_memcmp(res->value.u.time,
                        val->u.time, MAX_TIME_FIELD_LEN) < 0)
            sc_memcpy(res->value.u.time, val->u.time, MAX_TIME_FIELD_LEN);
        else
            return 0;
        break;
    case DT_TIMESTAMP:
        if (res->value.is_null || res->value.u.timestamp < val->u.timestamp)
            sc_memcpy(&res->value.u.timestamp,
                    &val->u.timestamp, MAX_TIMESTAMP_FIELD_LEN);
        else
            return 0;
        break;
    default:
        return -1;
    }

    if (res->value.is_null)
        res->value.is_null = 0;

    return 1;
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
int
get_sumvalue(T_RESULTCOLUMN * res, T_VALUEDESC * val)
{
    if (val->is_null)
        return 0;

    switch (res->value.valuetype)
    {
    case DT_TINYINT:
        if (res->value.is_null)
        {
            res->value.u.i8 = val->u.i8;
            res->value.is_null = 0;
        }
        else
            res->value.u.i8 += val->u.i8;
        break;
    case DT_SMALLINT:
        if (res->value.is_null)
        {
            res->value.u.i16 = val->u.i16;
            res->value.is_null = 0;
        }
        else
            res->value.u.i16 += val->u.i16;
        break;
    case DT_INTEGER:
        if (res->value.is_null)
        {
            res->value.u.i32 = val->u.i32;
            res->value.is_null = 0;
        }
        else
            res->value.u.i32 += val->u.i32;
        break;
    case DT_BIGINT:
        if (res->value.is_null)
        {
            res->value.u.i64 = val->u.i64;
            res->value.is_null = 0;
        }
        else
            res->value.u.i64 += val->u.i64;
        break;
    case DT_FLOAT:
        if (res->value.is_null)
        {
            res->value.u.f16 = val->u.f16;
            res->value.is_null = 0;
        }
        else
            res->value.u.f16 += val->u.f16;
        break;
    case DT_DOUBLE:
        switch (val->valuetype)
        {
        case DT_TINYINT:
            if (res->value.is_null)
            {
                res->value.u.f32 = (double) val->u.i8;
                res->value.is_null = 0;
            }
            else
                res->value.u.f32 += (double) val->u.i8;
            break;
        case DT_SMALLINT:
            if (res->value.is_null)
            {
                res->value.u.f32 = (double) val->u.i16;
                res->value.is_null = 0;
            }
            else
                res->value.u.f32 += (double) val->u.i16;
            break;
        case DT_INTEGER:
            if (res->value.is_null)
            {
                res->value.u.f32 = (double) val->u.i32;
                res->value.is_null = 0;
            }
            else
                res->value.u.f32 += (double) val->u.i32;
            break;
        case DT_BIGINT:
            if (res->value.is_null)
            {
                res->value.u.f32 = (double) val->u.i64;
                res->value.is_null = 0;
            }
            else
                res->value.u.f32 += (double) val->u.i64;
            break;
        case DT_FLOAT:
            if (res->value.is_null)
            {
                res->value.u.f32 = (double) val->u.f16;
                res->value.is_null = 0;
            }
            else
                res->value.u.f32 += (double) val->u.f16;
            break;
        case DT_DOUBLE:
            if (res->value.is_null)
            {
                res->value.u.f32 = (double) val->u.f32;
                res->value.is_null = 0;
            }
            else
                res->value.u.f32 += (double) val->u.f32;
            break;
        case DT_DECIMAL:
            if (res->value.is_null)
            {
                res->value.u.f32 = (double) val->u.dec;
                res->value.is_null = 0;
            }
            else
                res->value.u.f32 += (double) val->u.dec;
            break;
        default:
            break;
        }
        break;
    case DT_DECIMAL:
        switch (val->valuetype)
        {
        case DT_TINYINT:
            if (res->value.is_null)
            {
                res->value.u.dec = (decimal) val->u.i8;
                res->value.is_null = 0;
            }
            else
                res->value.u.dec += (decimal) val->u.i8;
            break;
        case DT_SMALLINT:
            if (res->value.is_null)
            {
                res->value.u.dec = (decimal) val->u.i16;
                res->value.is_null = 0;
            }
            else
                res->value.u.dec += (decimal) val->u.i16;
            break;
        case DT_INTEGER:
            if (res->value.is_null)
            {
                res->value.u.dec = (decimal) val->u.i32;
                res->value.is_null = 0;
            }
            else
                res->value.u.dec += (decimal) val->u.i32;
            break;
        case DT_BIGINT:
            if (res->value.is_null)
            {
                res->value.u.dec = (decimal) val->u.i64;
                res->value.is_null = 0;
            }
            else
                res->value.u.dec += (decimal) val->u.i64;
            break;
        case DT_FLOAT:
            if (res->value.is_null)
            {
                res->value.u.dec = (decimal) val->u.f16;
                res->value.is_null = 0;
            }
            else
                res->value.u.dec += (decimal) val->u.f16;
            break;
        case DT_DOUBLE:
            if (res->value.is_null)
            {
                res->value.u.dec = (decimal) val->u.f32;
                res->value.is_null = 0;
            }
            else
                res->value.u.dec += (decimal) val->u.f32;
            break;
        case DT_DECIMAL:
            if (res->value.is_null)
            {
                res->value.u.dec = (decimal) val->u.dec;
                res->value.is_null = 0;
            }
            else
                res->value.u.dec += (decimal) val->u.dec;
            break;
        default:
            break;
        }
        break;
    case DT_CHAR:
    case DT_NCHAR:
    case DT_VARCHAR:
    case DT_NVARCHAR:
    case DT_DATETIME:
    case DT_DATE:
    case DT_TIME:
    case DT_TIMESTAMP:
        break;
    default:
        return -1;
    }

    return 1;
}

/*****************************************************************************/
//! convert_rec_attr2value 

/*! \breif  rec_values->rec_oid에 해당하는 레코드를 읽어서, 
 *          필요한 칼럼의 값을 val에 할당한다.
 ************************************
 * \param attr(in)              : not used.
 * \param rec_values(in/out)    : 
 * \param position(in)          : 
 * \param val(in/out)           :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *
 *  - called : set_exprvalue()
 *  - val->value_len에 rec_values에 할당되었던 길이를 할당한다
 *      (NULL 할당 안하기 위함)
 *  - collation type을 설정
 *****************************************************************************/
void
convert_rec_attr2value(T_ATTRDESC * attr, FIELDVALUES_T * rec_values,
        int position, T_VALUEDESC * val)
{
    FIELDVALUE_T *fld_value;

    fld_value = &rec_values->fld_value[position];

    val->valuetype = fld_value->type_;
    if (attr)
        val->attrdesc.collation = attr->collation;
    else
        val->attrdesc.collation = fld_value->collation;

    if (fld_value->is_null)
    {
        val->is_null = 1;
        return;
    }

    val->is_null = 0;

    switch (val->valuetype)
    {
    case DT_VARCHAR:
    case DT_NVARCHAR:
    case DT_CHAR:
    case DT_NCHAR:
        val->u.ptr = fld_value->value_.pointer_;
        val->value_len = fld_value->value_length_;
#if defined(MDB_DEBUG)
        if (val->value_len != strlen_func[val->valuetype] (val->u.ptr))
            sc_assert(0, __FILE__, __LINE__);
#endif
        return;
    case DT_BYTE:
    case DT_VARBYTE:
        val->u.ptr = fld_value->value_.pointer_;
        val->value_len = fld_value->value_length_;
        return;
    case DT_SMALLINT:  // smallint
        val->u.i16 = fld_value->value_.short_;
        return;
    case DT_INTEGER:   // integer
        val->u.i32 = fld_value->value_.int_;
        return;
    case DT_FLOAT:     // float
        val->u.f16 = fld_value->value_.float_;
        return;
    case DT_DOUBLE:    // double
        val->u.f32 = fld_value->value_.double_;
        return;
    case DT_TINYINT:   // tinyint
        val->u.i8 = fld_value->value_.char_;
        return;
    case DT_BIGINT:    // bigint
        val->u.i64 = fld_value->value_.bigint_;
        return;
    case DT_DATETIME:  // datetime
        sc_memcpy(val->u.datetime, fld_value->value_.datetime_,
                MAX_DATETIME_FIELD_LEN);
        return;
    case DT_DATE:      // date
        sc_memcpy(val->u.date, fld_value->value_.date_, MAX_DATE_FIELD_LEN);
        return;
    case DT_TIME:      // time
        sc_memcpy(val->u.time, fld_value->value_.time_, MAX_TIME_FIELD_LEN);
        return;
    case DT_TIMESTAMP: // timestamp
        val->u.timestamp = fld_value->value_.timestamp_;
        val->u.timestamp = val->u.timestamp;

        return;
    case DT_DECIMAL:   // decimal
        char2decimal(&val->u.dec, fld_value->value_.decimal_,
                sc_strlen(fld_value->value_.decimal_));
        return;
    case DT_OID:       // long
        val->u.oid = fld_value->value_.oid_;
        return;
    default:
        break;
    }

}

/*****************************************************************************/
//! convert_attr2value

/*! \breif  DB Record를 val로 copy 시킴
 ************************************
 * \param attr(in)      :
 * \param val(in/out)   :
 * \param rec(in)       : DB's Record Buffer
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *  - 주의사항
 *  value class는 변경하지 말자.
 *  그래야 나중에 이게 원래 constant 인지. 원래는 valiable이었는데
 *  record에서 빼온건지를 구분하기 위해.
 *  단지, 이 함수를 call 한후부터는 attribute 라도 value값이 존재한다는 것을
 *  잊지 말자.
 *  - BYTE/VARBYTE 추가
 *  - collation도 할당 해줘야함
 *
 *****************************************************************************/
void
convert_attr2value(T_ATTRDESC * attr, T_VALUEDESC * val, char *rec)
{
    char *ptr;

    // value class는 변경하지 말자.
    // 그래야 나중에 이게 원래 constant 인지. 원래는 valiable이었는데
    // record에서 빼온건지를 구분하기 위해.
    // 단지, 이 함수를 call 한후부터는 attribute 라도 value값이 존재한다는 것을
    // 잊지 말자.
    val->attrdesc.len = attr->len;
    val->attrdesc.collation = attr->collation;

    ptr = rec + attr->posi.offset;
    switch (attr->type)
    {
    case DT_VARCHAR:   // string
        // valuetype이 정해져있다면, 이미 malloc()을 받은 상태이다.
        if (val->valuetype == DT_NULL_TYPE || val->u.ptr == NULL)
        {
            val->valuetype = DT_VARCHAR;
            if (sql_value_ptrXcalloc(val, (attr->len + 1)) < 0)
                break;
            val->attrdesc.len = attr->len;
        }
        sc_strncpy(val->u.ptr, ptr, attr->len + 1);
        val->value_len = strlen_func[val->valuetype] (val->u.ptr);
        return;
    case DT_NVARCHAR:  // string
        // valuetype이 정해져있다면, 이미 malloc()을 받은 상태이다.
        if (val->valuetype == DT_NULL_TYPE || val->u.Wptr == NULL)
        {
            val->valuetype = DT_NVARCHAR;
            if (sql_value_ptrXcalloc(val, attr->len + 1) < 0)
                break;
            val->attrdesc.len = attr->len;
        }
        sc_wcsncpy(val->u.Wptr, (DB_WCHAR *) ptr, attr->len + 1);
        val->value_len = strlen_func[val->valuetype] (val->u.ptr);
        return;
    case DT_CHAR:      // char
        // valuetype이 정해져있다면, 이미 malloc()을 받은 상태이다.
        if (val->valuetype == DT_NULL_TYPE || val->u.ptr == NULL)
        {
            val->valuetype = DT_CHAR;
            if (sql_value_ptrXcalloc(val, attr->len + 1) < 0)
                break;
            val->attrdesc.len = attr->len;
        }
        /* only for special version */
        sc_memcpy(val->u.ptr, ptr, attr->len);
        val->u.ptr[attr->len] = '\0';
        return;
    case DT_NCHAR:     // bytes
        // valuetype이 정해져있다면, 이미 malloc()을 받은 상태이다.
        if (val->valuetype == DT_NULL_TYPE || val->u.Wptr == NULL)
        {
            val->valuetype = DT_NCHAR;
            if (sql_value_ptrXcalloc(val, attr->len + 1) < 0)
                break;
            val->attrdesc.len = attr->len;
        }
        /* only for special version */
        sc_wmemcpy(val->u.Wptr, (DB_WCHAR *) ptr, attr->len);
        val->u.Wptr[attr->len] = L'\0';
        return;
    case DT_BYTE:
    case DT_VARBYTE:
        // valuetype이 정해져있다면, 이미 malloc()을 받은 상태이다.
        if (val->valuetype == DT_NULL_TYPE || val->u.ptr == NULL)
        {
            val->valuetype = DT_BYTE;
            if (sql_value_ptrXcalloc(val, attr->len) < 0)
                break;
            // 몇바이트가 들어올지 모르므로.. 스키마만큼 할당
            val->attrdesc.len = attr->len;
        }
        sc_memcpy(val->u.ptr, ptr, attr->len);
        return;

    case DT_SMALLINT:  // smallint
        val->valuetype = DT_SMALLINT;
        val->u.i16 = *(smallint *) (ptr);
        return;
    case DT_INTEGER:   // integer
        val->valuetype = DT_INTEGER;
        val->u.i32 = *(int *) (ptr);
        return;
    case DT_FLOAT:     // float
        val->valuetype = DT_FLOAT;
        val->u.f16 = *(float *) (ptr);
        return;
    case DT_DOUBLE:    // double
        val->valuetype = DT_DOUBLE;
        val->u.f32 = *(double *) (ptr);
        return;
    case DT_TINYINT:   // tinyint
        val->valuetype = DT_TINYINT;
        val->u.i8 = *(tinyint *) (ptr);
        return;
    case DT_BIGINT:    // bigint
        val->valuetype = DT_BIGINT;
        val->u.i64 = *(bigint *) (ptr);
        return;
    case DT_DATETIME:  // datetime
        val->valuetype = DT_DATETIME;
        sc_memcpy(val->u.datetime, ptr, MAX_DATETIME_FIELD_LEN);
        return;
    case DT_DATE:      // date
        val->valuetype = DT_DATE;
        sc_memcpy(val->u.date, ptr, MAX_DATE_FIELD_LEN);
        return;
    case DT_TIME:      // time
        val->valuetype = DT_TIME;
        sc_memcpy(val->u.time, ptr, MAX_TIME_FIELD_LEN);
        return;
    case DT_TIMESTAMP: // timestamp
        val->valuetype = DT_TIMESTAMP;
        sc_memcpy(&val->u.timestamp, ptr, MAX_TIMESTAMP_FIELD_LEN);
        return;
    case DT_DECIMAL:   // decimal
        val->valuetype = DT_DECIMAL;
        char2decimal(&val->u.dec, ptr, sc_strlen(ptr));
        return;
    default:
        break;
    }
}

/*****************************************************************************/
//! set_exprvalue 

/*! \breif
 ************************************
 * \param rttable(in/out)   :
 * \param expr(in/out)      : 
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *      rttable에 있는 칼럼이 expr에 사용되었으면 expr에 값을 rttable을 
 *      이용해서설정한다.
 *  - called : exec_update()
 *  - T_EXPRESSIONDESC 구조체 변경
 *
 *****************************************************************************/
void
set_exprvalue(T_RTTABLE * rttable, T_EXPRESSIONDESC * expr, OID rid)
{
    register int i;
    register char *tablename;
    char *name;
    T_POSTFIXELEM *expr_list;

    tablename = rttable->table.aliasname ?
            rttable->table.aliasname : rttable->table.tablename;

    for (i = 0; i < expr->len; i++)
    {
        expr_list = expr->list[i];
        if (expr_list->elemtype != SPT_VALUE)
            continue;

        if (expr_list->u.value.valueclass != SVC_VARIABLE)
            continue;

        if ((expr_list->u.value.valuetype == DT_OID ||
                        expr_list->u.value.attrdesc.type == DT_OID) &&
                expr_list->u.value.attrdesc.Hattr == -1)
        {
            expr_list->u.value.valuetype = DT_OID;
            expr_list->u.value.attrdesc.type = DT_OID;
            if (rttable->rec_values)
            {
                name = expr_list->u.value.attrdesc.table.aliasname ?
                        expr_list->u.value.attrdesc.table.aliasname :
                        expr_list->u.value.attrdesc.table.tablename;
                if (!mdb_strcmp(name, tablename))
                {
                    expr_list->u.value.u.oid = rttable->rec_values->rec_oid;
                    expr_list->u.value.is_null = 0;
                }
            }
            else if (rid != NULL_OID)
            {
                name = expr_list->u.value.attrdesc.table.aliasname ?
                        expr_list->u.value.attrdesc.table.aliasname :
                        expr_list->u.value.attrdesc.table.tablename;
                if (!mdb_strcmp(name, tablename))
                {
                    expr_list->u.value.u.oid = rid;
                    expr_list->u.value.is_null = 0;
                }
            }
            else
                expr_list->u.value.is_null = 1;

            continue;
        }
        // subquery의 경우 table이름이 실제 이름과 다를수 있다. 왜냐면
        // (subquery_) as table_alias_name(i) 일 경우 i의 테이블 이름은
        // subquery_의 from절에 오는 테이블 이름이 지만, rttable에는
        // (subquery_)의 결과로 생기는 임시 테이블 이름이 들어가기 때문이다.
        // 다른 경우에도 테이블 이름을 alias시켰으면 테이블 이름 대신
        // alias이름으로 평가해야 하기 때문에 ...
        // 쫑나는 경우는 이미 check_validation()에서 검증했다 ?

        name = expr_list->u.value.attrdesc.table.aliasname ?
                expr_list->u.value.attrdesc.table.aliasname :
                expr_list->u.value.attrdesc.table.tablename;
        if (mdb_strcmp(name, tablename))
        {
            continue;
        }

        if (rttable->rec_values)
        {
            convert_rec_attr2value(&expr_list->u.value.attrdesc,
                    rttable->rec_values,
                    expr_list->u.value.attrdesc.posi.ordinal,
                    &expr_list->u.value);
            continue;
        }

        if (!DB_VALUE_ISNULL(rttable->rec_buf,
                        expr_list->u.value.attrdesc.posi.ordinal,
                        rttable->nullflag_offset))
        {
            convert_attr2value(&expr_list->u.value.attrdesc,
                    &expr_list->u.value, rttable->rec_buf);
            expr_list->u.value.is_null = 0;
        }
        else
        {
            expr_list->u.value.valuetype = expr_list->u.value.attrdesc.type;
            if (IS_ALLOCATED_TYPE(expr_list->u.value.valuetype))
            {
                if (expr_list->u.value.u.ptr)
                    sc_memset(expr_list->u.value.u.ptr, 0,
                            SizeOfType[expr_list->u.value.valuetype]);
                else
                    sql_value_ptrAlloc(&expr_list->u.value,
                            expr_list->u.value.attrdesc.len + 1);
            }

            if (IS_BYTE(expr_list->u.value.valuetype))
            {
                if (expr_list->u.value.u.ptr)
                {
                    expr_list->u.value.value_len = 0;
                }
                else
                {
                    expr_list->u.value.u.ptr =
                            PMEM_ALLOC(expr_list->u.value.attrdesc.len);
                    expr_list->u.value.value_len = 0;
                }
            }

            expr_list->u.value.is_null = 1;
        }
    }
}

/*****************************************************************************/
//! set_exprvalue_for_rttable 

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param rttable() :
 * \param expr()    : 
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *  - T_EXPRESSIONDESC 구조체 변경
 *
 *****************************************************************************/
void
set_exprvalue_for_rttable(T_RTTABLE * rttable, T_EXPRESSIONDESC * expr)
{
    register int i;
    register char *tablename;
    T_POSTFIXELEM *expr_list;

    tablename = rttable->table.aliasname ?
            rttable->table.aliasname : rttable->table.tablename;

    for (i = 0; i < expr->len; i++)
    {
        expr_list = expr->list[i];
        if (expr_list->elemtype != SPT_VALUE)
            continue;

        if (expr_list->u.value.valueclass != SVC_VARIABLE)
            continue;

        if (expr_list->u.value.valuetype == DT_OID &&
                expr_list->u.value.attrdesc.Hattr == -1)
        {
            if (rttable->rec_values)
            {
                expr_list->u.value.u.oid = rttable->rec_values->rec_oid;
                expr_list->u.value.is_null = 0;
            }
            else
                expr_list->u.value.is_null = 1;

            continue;
        }

        {
            char *name;

            name = expr_list->u.value.attrdesc.table.aliasname ?
                    expr_list->u.value.attrdesc.table.aliasname :
                    expr_list->u.value.attrdesc.table.tablename;
            if (mdb_strcmp(name, tablename))
            {
                continue;
            }
        }
        convert_rec_attr2value(&expr_list->u.value.attrdesc,
                rttable->rec_values,
                expr_list->u.value.attrdesc.posi.ordinal, &expr_list->u.value);
    }
}

/*****************************************************************************/
//! set_correl_attr_values 

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
void
set_correl_attr_values(CORRELATED_ATTR * first_cl)
{
    int i, found = 0;
    CORRELATED_ATTR *cl;        // cl->ptr == T_POSTFIXELEM *
    T_POSTFIXELEM *elem;
    T_SELECT *correl_tbl;

    T_RTTABLE *rttable = NULL;

    for (cl = first_cl; cl; cl = cl->next)
    {
        elem = (T_POSTFIXELEM *) (cl->ptr);
#ifdef MDB_DEBUG
        sc_assert(elem->elemtype == SPT_VALUE, __FILE__, __LINE__);
#endif
        correl_tbl = elem->u.value.attrdesc.table.correlated_tbl;

        i = 0;
        if (correl_tbl->born_stmt)
        {
            char *tablename = NULL;

            if (correl_tbl->born_stmt->sqltype == ST_UPDATE)
                rttable = correl_tbl->born_stmt->u._update.rttable;
            else if (correl_tbl->born_stmt->sqltype == ST_DELETE)
                rttable = correl_tbl->born_stmt->u._delete.rttable;
            else
                sc_assert(0, __FILE__, __LINE__);

            tablename = elem->u.value.attrdesc.table.aliasname
                    ? elem->u.value.attrdesc.table.aliasname
                    : elem->u.value.attrdesc.table.tablename;

#ifdef MDB_DEBUG
            if (tablename == NULL)
                sc_assert(0, __FILE__, __LINE__);
#endif

            if (rttable->table.tablename != NULL &&
                    !mdb_strcmp(tablename, rttable->table.tablename))
                found = 1;

            if (rttable->table.aliasname &&
                    !mdb_strcmp(tablename, rttable->table.aliasname))
                found = 1;
        }
        else
        {

            /* find rttable. 구찮은데 check_attr_correlation()에서
               테이블(nest[])의 인덱스를 설정해 버리면 ... */
            for (i = 0; i < correl_tbl->planquerydesc.querydesc.fromlist.len;
                    i++)
            {

                /* subquery의 경우 table이름이 실제 이름과 다를수 있다. 왜냐면
                   (subquery_) as table_alias_name(i) 일 경우 i의 테이블 이름은
                   subquery_의 from절에 오는 테이블 이름이 지만, rttable에는
                   (subquery_)의 결과로 생기는 임시 테이블 이름이 들어가기 때문이다.
                   다른 경우에도 테이블 이름을 alias시켰으면 테이블 이름 대신
                   alias이름으로 평가해야 하기 때문에 ... */
                if (elem->u.value.attrdesc.table.aliasname == NULL &&
                        mdb_strcmp(elem->u.value.attrdesc.table.tablename,
                                correl_tbl->rttable[i].table.tablename))
                    continue;
                if (correl_tbl->rttable[i].table.aliasname &&
                        elem->u.value.attrdesc.table.aliasname &&
                        mdb_strcmp(elem->u.value.attrdesc.table.aliasname,
                                correl_tbl->rttable[i].table.aliasname))
                    continue;
                if (elem->u.value.attrdesc.table.aliasname &&
                        !correl_tbl->rttable[i].table.aliasname &&
                        mdb_strcmp(elem->u.value.attrdesc.table.aliasname,
                                correl_tbl->rttable[i].table.tablename))
                    continue;
                found = 1;
                break;
            }
            rttable = &correl_tbl->rttable[i];
        }

#ifdef MDB_DEBUG
        // ok .. found
        sc_assert(found, __FILE__, __LINE__);
#endif

        if (rttable->rec_values)
            convert_rec_attr2value(&elem->u.value.attrdesc,
                    rttable->rec_values,
                    elem->u.value.attrdesc.posi.ordinal, &elem->u.value);
        else if (rttable->rec_buf != NULL)
        {
            if (!DB_VALUE_ISNULL(rttable->rec_buf,
                            elem->u.value.attrdesc.posi.ordinal,
                            rttable->nullflag_offset))
            {
                convert_attr2value(&elem->u.value.attrdesc,
                        &elem->u.value, rttable->rec_buf);
                elem->u.value.is_null = 0;
            }
            else
            {
                elem->u.value.valuetype = elem->u.value.attrdesc.type;
                elem->u.value.is_null = 1;
            }
        }
        else
            sc_assert(0, __FILE__, __LINE__);
    }
}

/*****************************************************************************/
//! make_minmax_fldval

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param expr(in)      :
 * \param attr(in)      : 
 * \param fieldval(in/out) : dest
 * \param flag():
 * \param in_expr_idx():
 * \param bound(in) : MDB_GE -> min, MDB_LE -> max
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *****************************************************************************/
extern int sql_compare_elems(void const *_elem1, void const *_elem2);

/* null expr에 대하여 skip, index field의 order만을 사용하도록 수정 */
static int
_distinctsort_expr(T_EXPRESSIONDESC * expr, T_ATTRDESC * attr)
{
    int expr_len = expr->len;
    T_POSTFIXELEM **res_elems;
    int useless_elem_num = 0;
    int use_elem_num = 0;
    int i, j;
    int ret;
    T_VALUEDESC base, res;

    int isAsc;

    res_elems = (T_POSTFIXELEM **)
            PMEM_XCALLOC(sizeof(T_POSTFIXELEM *) * expr->len);

    if (res_elems == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
        return SQL_E_OUTOFMEMORY;
    }

    use_elem_num = expr_len - 1;

    /* type convert를 수행하여 실패하는 경우 null 처리 */
    sc_memset(&base, 0, sizeof(T_VALUEDESC));
    sc_memset(&res, 0, sizeof(T_VALUEDESC));

    base.valueclass = SVC_CONSTANT;
    base.valuetype = attr->type;
    sc_memcpy(&base.attrdesc, attr, sizeof(T_ATTRDESC));

    for (i = 0; i < expr_len; i++)
    {
        if (expr->list[i]->u.value.valuetype != attr->type)
        {
            res.call_free = 0;
            ret = calc_func_convert(&base, &expr->list[i]->u.value, &res,
                    MDB_FALSE);
            sql_value_ptrFree(&expr->list[i]->u.value);
            if (ret < 0)
            {
                er_clear();
                expr->list[i]->u.value.is_null = 1;
            }
            else
            {
                expr->list[i]->u.value = res;
            }
        }
    }

    for (i = 0; i < expr_len; i++)
    {
        j = i + 1;
        if (!expr->list[i]->u.value.is_null)
        {
            for (; j < expr_len; j++)
            {
                if (expr->list[j]->u.value.is_null)
                {
                    continue;
                }

                ret = sql_compare_elems(expr->list[i], expr->list[j]);
                if (ret == 0)
                {
                    break;
                }
            }
        }

        if (j == expr_len)
        {
            res_elems[use_elem_num] = expr->list[i];
            use_elem_num -= 1;
        }
        else
        {
            res_elems[useless_elem_num] = expr->list[i];
            useless_elem_num += 1;
        }
    }

    if (attr->order == 'A')
    {
        isAsc = 1;
    }
    else
    {
        isAsc = 0;
    }

    mdb_sort_array((void **) (res_elems + useless_elem_num),
            expr_len - useless_elem_num, sql_compare_elems, isAsc);

    sc_memcpy(expr->list, res_elems, sizeof(T_POSTFIXELEM *) * expr->len);

    PMEM_FREENUL(res_elems);

    return useless_elem_num;
}

static int
make_minmax_fldval(T_NESTING * nest, int fld_idx, FIELDVALUE_T * fieldval,
        int *flag, int *in_expr_idx, MDB_UINT8 bound)
{
    T_POSTFIXELEM *elem;
    T_VALUEDESC value;
    int i, is_null;
    int ret;

    T_EXPRESSIONDESC in_expr;

    T_EXPRESSIONDESC *expr;
    T_ATTRDESC *attr;

    if (bound == MDB_GE || bound == MDB_GT)
    {
        expr = &nest->min[fld_idx].expr;
    }
    else if (bound == MDB_LE || bound == MDB_LT)
    {
        expr = &nest->max[fld_idx].expr;
    }
    else
    {
        expr = NULL;
#if defined(MDB_DEBUG)
        sc_assert(0, __FILE__, __LINE__);
#endif
    }

    attr = &nest->index_field_info[fld_idx];

    in_expr.list = NULL;
    sc_memset(&value, 0, sizeof(T_VALUEDESC));

    if (expr->len == 0 || *flag == 1)
    {       /* set meaningless key value for partial key */
        value.valueclass = SVC_NONE;
        value.valuetype = DT_NULL_TYPE;
        value.attrdesc = *attr;

        /* possible return values: RET_SUCCESS | RET_ERROR */
        return make_fieldvalue(&value, fieldval, &bound);
    }

    is_null = -1;
    for (i = 0; i < expr->len; i++)
    {
        elem = expr->list[i];
        if (elem->elemtype == SPT_VALUE &&
                elem->u.value.valueclass == SVC_VARIABLE &&
                elem->u.value.valuetype == DT_NULL_TYPE)
        {
            *flag = 1;
            value.valueclass = SVC_NONE;
            value.valuetype = DT_NULL_TYPE;
            value.attrdesc = *attr;

            /* possible return values: RET_SUCCESS | RET_ERROR */
            return make_fieldvalue(&value, fieldval, &bound);
        }
        if (elem->elemtype == SPT_OPERATOR)
        {
            if (elem->u.op.type == SOT_ISNULL)
                is_null = 1;
            else if (elem->u.op.type == SOT_ISNOTNULL)
                is_null = 0;
        }
    }

    if (is_null < 0)
    {
        if (*in_expr_idx > -1)
        {
            in_expr.len = in_expr.max = 1;
            in_expr.list = &(expr->list[*in_expr_idx]);
            expr = &in_expr;
        }

        if (expr->len == 1 && expr->list[0]->elemtype == SPT_VALUE)
        {
            value = expr->list[0]->u.value;
            if (value.is_null)
            {
                return RET_FALSE;
            }
            sc_memcpy(&value.attrdesc, attr, sizeof(T_ATTRDESC));
            ret = make_fieldvalue(&value, fieldval, &bound);
        }
        else
        {
            if (calc_expression(expr, &value, MDB_FALSE) < 0)
                return RET_ERROR;

            if (value.is_null)
                return RET_FALSE;

            sc_memcpy(&value.attrdesc, attr, sizeof(T_ATTRDESC));
            ret = make_fieldvalue(&value, fieldval, &bound);
            sql_value_ptrFree(&value);
        }

        if (ret == RET_TRUE)
            is_null = 0;        /* go downward */
        else    /* possible values of ret: RET_SUCCESS | RET_ERROR | RET_FALSE */
            return ret;
    }

    if ((attr->order == 'A' && bound == MDB_GE)
            || (attr->order == 'D' && bound == MDB_LE))
        return make_minfieldvalue(attr, is_null, fieldval);
    else
        return make_maxfieldvalue(attr, is_null, fieldval);
}

#define NEXT_PTR(_ptr, _is_reverse, _chSize)\
do {\
    if (_is_reverse)\
        _ptr -= _chSize;\
    else\
        _ptr += _chSize;\
} while(0)

#define IS_LIKESPECIAL(_ch) ( (_ch) == '\\' || (_ch) == '%' ||(_ch) == '_' )
#define GET_RES_CH(_is_wch, _resPtr) ((_is_wch) ? *(DB_WCHAR*)(_resPtr) :*(_resPtr))

#define PUT_RES_CH(curPtr, is_wch, resCh) \
do {\
    if (is_wch)\
        *(DB_WCHAR*)curPtr = (DB_WCHAR)resCh;\
    else\
        *curPtr = (DB_WCHAR)resCh;\
}while (0)


static char *
_trim_likestring(char *res, char *src, int src_len, T_ATTRDESC * attr)
{
    int is_wch = 0, is_reverse = 0, resCh = 0, chSize = 1;
    char *curSrcPtr;
    char *curResPtr;
    char *escape_expected;
    char *like_expected;

    int srcbyte_len;

    if (IS_NBS(attr->type))
    {
        is_wch = 1;
        chSize = WCHAR_SIZE;
    }

    if (attr->collation == MDB_COL_CHAR_REVERSE)
        is_reverse = 1;


    if (src_len < attr->len)
        srcbyte_len = DB_BYTE_LENG(attr->type, src_len);
    else
        srcbyte_len = DB_BYTE_LENG(attr->type, attr->len);

    if (is_reverse)
    {
        curSrcPtr = src + srcbyte_len;
        NEXT_PTR(curSrcPtr, is_reverse, chSize);
        curResPtr = res + srcbyte_len;
    }
    else
    {
        curSrcPtr = src;
        curResPtr = res;
        NEXT_PTR(curResPtr, 1, chSize);
    }

    while (srcbyte_len > 0)
    {
        resCh = GET_RES_CH(is_wch, curSrcPtr);

        if (IS_LIKESPECIAL(resCh))
        {
            if (is_reverse)
            {
                escape_expected = curSrcPtr - chSize;
                like_expected = curSrcPtr;

                if (escape_expected < src)      /* out of range */
                    break;
            }
            else
            {
                escape_expected = curSrcPtr;
                like_expected = curSrcPtr + chSize;
            }

            resCh = GET_RES_CH(is_wch, escape_expected);
            if (resCh != '\\')
            {
                break;
            }

            resCh = GET_RES_CH(is_wch, like_expected);
            NEXT_PTR(curResPtr, is_reverse, chSize);
            PUT_RES_CH(curResPtr, is_wch, resCh);

            NEXT_PTR(curSrcPtr, is_reverse, chSize);
            srcbyte_len -= chSize;
        }
        else
        {
            NEXT_PTR(curResPtr, is_reverse, chSize);
            PUT_RES_CH(curResPtr, is_wch, resCh);
        }

        NEXT_PTR(curSrcPtr, is_reverse, chSize);
        srcbyte_len -= chSize;
    }

    if (is_reverse)
        return curResPtr;
    else
        return res;
}

/*****************************************************************************/
//! make_fieldval_for_like

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param expr()        :
 * \param attr()        : 
 * \param min_fval()    :
 * \param max_fval()    :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static int
make_fieldval_for_like(T_EXPRESSIONDESC * expr,
        T_ATTRDESC * attr, FIELDVALUE_T * min_fval, FIELDVALUE_T * max_fval)
{
    T_VALUEDESC res;
    T_VALUEDESC minval, maxval;

    MDB_UINT8 bound;
    int ret;
    char *trimedPtr = NULL;
    int trimedLen;
    char *maxvalPtr = NULL;

    ret = calc_expression(expr, &res, MDB_FALSE);
    if (ret < 0)
        return ret;

    if (res.is_null)
        return RET_FALSE;

    sc_memset(&minval, 0, sizeof(T_VALUEDESC));
    sc_memset(&maxval, 0, sizeof(T_VALUEDESC));

    minval.valueclass = maxval.valueclass = SVC_CONSTANT;

    sc_memcpy(&minval.attrdesc, attr, sizeof(T_ATTRDESC));
    sc_memcpy(&maxval.attrdesc, attr, sizeof(T_ATTRDESC));

    if (IS_NUMERIC(attr->type))
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_INCONSISTENTTYPE, 0);
        return RET_ERROR;
    }

    if (IS_BYTE(res.valuetype))
    {
        sql_value_ptrFree(&res);
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_RESTRICT_KEY_OR_FIELD,
                0);
        return SQL_E_RESTRICT_KEY_OR_FIELD;
    }

    if (IS_BS_OR_NBS(res.valuetype))
    {
        int srcLen = strlen_func[attr->type] (res.u.ptr);

        minval.valuetype = maxval.valuetype = res.valuetype;
        sql_value_ptrXcalloc(&maxval, attr->len + 1);

        /* trim the actual likeString not to have like special character. */
        if (srcLen == 0)
        {   /* null string? */
            sql_value_ptrXcalloc(&minval, attr->len + 1);
        }
        else
        {
            trimedPtr =
                    _trim_likestring(maxval.u.ptr, res.u.ptr, srcLen, attr);
            trimedLen = strlen_func[attr->type] (trimedPtr);

            if (trimedLen == 0)
            {   /* make a full scan. first char is a like char */
                minval.valueclass = maxval.valueclass = SVC_NONE;
                minval.valuetype = maxval.valuetype = DT_NULL_TYPE;
                sql_value_ptrFree(&maxval);
            }
            else
            {
                sql_value_ptrXcalloc(&minval, attr->len + 1);

                /* make min string */
                strcpy_func[res.valuetype] (minval.u.ptr, trimedPtr);

                /* make max string */
                {
                    set_maxvalue(attr->type, attr->len, &maxval);
                    if (attr->collation == MDB_COL_CHAR_REVERSE)
                        maxvalPtr =
                                maxval.u.ptr + (attr->len -
                                trimedLen) * SizeOfType[attr->type];
                    else
                        maxvalPtr = maxval.u.ptr;

                    memcpy_func[attr->type] (maxvalPtr, minval.u.ptr,
                            trimedLen);
                }
            }
        }
    }
    else
    {
        minval.valueclass = SVC_NONE;
        minval.valuetype = DT_NULL_TYPE;
        maxval.valueclass = SVC_NONE;
        maxval.valuetype = DT_NULL_TYPE;
    }

    ret = RET_SUCCESS;

    if (attr->order == 'A')
    {
        bound = MDB_GE;
        ret = make_fieldvalue(&minval, min_fval, &bound);
        if (ret != RET_SUCCESS)
            goto RETURN_PROCESS;

        bound = MDB_LE;
        ret = make_fieldvalue(&maxval, max_fval, &bound);
        if (ret != RET_SUCCESS)
            goto RETURN_PROCESS;
    }
    else
    {
        bound = MDB_GE;
        ret = make_fieldvalue(&maxval, min_fval, &bound);
        if (ret != RET_SUCCESS)
            goto RETURN_PROCESS;

        bound = MDB_LE;
        ret = make_fieldvalue(&minval, max_fval, &bound);
        if (ret != RET_SUCCESS)
            goto RETURN_PROCESS;
    }

  RETURN_PROCESS:
    sql_value_ptrFree(&res);
    sql_value_ptrFree(&minval);
    sql_value_ptrFree(&maxval);

    return RET_SUCCESS;
}

/*****************************************************************************/
//! make_keyvalue 

/*! \breif  min/max keyvalue를 만든다
 ************************************
 * \param nest(in)      : table에 대한 접근 정보
 * \param min(in/out)   : minimum key 
 * \param max(in/out)   : maximum key
 * \param in_expr_idx(in):
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *  - BYTE/VARBYTE의 경우 이곳에 사용할 수 없다
 *****************************************************************************/
static int
make_keyvalue(T_NESTING * nest, T_KEYDESC * min, T_KEYDESC * max,
        int *in_expr_idx)
{
    FIELDVALUE_T *min_fval;
    FIELDVALUE_T *max_fval;
    unsigned char *op_like, bit = 0x1;
    int min_flag = 0, max_flag = 0;
    int i, j, ret;

    min->field_value_ = NULL;
    min->field_count_ = nest->index_field_num;
    min->field_value_ = (struct FieldValue *)
            PMEM_ALLOC(sizeof(struct FieldValue) * min->field_count_);
    if (min->field_value_ == NULL)
    {
        min->field_count_ = 0;
        return DB_E_OUTOFMEMORY;
    }
    min_fval = min->field_value_;

    max->field_value_ = NULL;
    max->field_count_ = nest->index_field_num;
    max->field_value_ = (struct FieldValue *)
            PMEM_ALLOC(sizeof(struct FieldValue) * max->field_count_);
    if (max->field_value_ == NULL)
    {
        min->field_count_ = 0;
        max->field_count_ = 0;
        PMEM_FREENUL(min->field_value_);
        return DB_E_OUTOFMEMORY;
    }
    max_fval = max->field_value_;

    op_like = (unsigned char *) &nest->op_like;
    for (i = 0; i < nest->index_field_num; i++)
    {
        if (*op_like & bit)
        {
            ret = make_fieldval_for_like(&nest->min[i].expr,
                    &nest->index_field_info[i], min_fval + i, max_fval + i);
            if (ret != RET_SUCCESS)
                goto error;
        }
        else
        {
            if (i == 0 && *in_expr_idx == 0)
            {
                ret = _distinctsort_expr(&nest->min[i].expr,
                        &nest->index_field_info[0]);
                if (ret < 0)
                {
                    goto error;
                }
                ret = _distinctsort_expr(&nest->max[i].expr,
                        &nest->index_field_info[0]);
                if (ret < 0)
                {
                    goto error;
                }

                *in_expr_idx = ret;
            }

            ret = make_minmax_fldval(nest, i, min_fval + i,
                    &min_flag, in_expr_idx, MDB_GE);
            if (ret != RET_SUCCESS)
                goto error;
            if (nest->min[i].type == SOT_GT)
            {
                min_fval[i].mode_ = MDB_GT;
            }
            else if (nest->min[i].type == SOT_LT)
            {
                min_fval[i].mode_ = MDB_LT;
            }
            ret = make_minmax_fldval(nest, i, max_fval + i,
                    &max_flag, in_expr_idx, MDB_LE);
            if (ret != RET_SUCCESS)
                goto error;
            if (nest->max[i].type == SOT_GT)
            {
                max_fval[i].mode_ = MDB_GT;
            }
            else if (nest->max[i].type == SOT_LT)
            {
                max_fval[i].mode_ = MDB_LT;
            }
        }

        if (!((bit <<= 1) & 0xff))
        {
            op_like++;
            bit = 0x1;
        }
    }

    return RET_SUCCESS;

  error:
    for (j = 0; j < i; j++)
    {
        if (IS_ALLOCATED_TYPE(min_fval[j].type_))
            PMEM_FREENUL(min_fval[j].value_.pointer_);
        else if (min_fval[j].type_ == DT_DECIMAL)
            PMEM_FREENUL(min_fval[j].value_.decimal_);

        if (IS_ALLOCATED_TYPE(max_fval[j].type_))
            PMEM_FREENUL(max_fval[j].value_.pointer_);
        else if (max_fval[j].type_ == DT_DECIMAL)
            PMEM_FREENUL(max_fval[j].value_.decimal_);

    }

    max->field_count_ = 0;
    min->field_count_ = 0;
    PMEM_FREENUL(max->field_value_);
    PMEM_FREENUL(min->field_value_);

    return ret;
}

/*****************************************************************************/
//! make_single_keyvalue

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param nest(in)      :
 * \param key(in/out)   : 
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *  - BYTE/VARBYTE인 경우 메모리 해제
 *****************************************************************************/
int
make_single_keyvalue(T_NESTING * nest, T_KEYDESC * key)
{
    FIELDVALUE_T fval[MAX_INDEX_FIELD_NUM];
    int flag = 0;
    int i, j, ret;

    for (i = 0; i < nest->index_field_num; i++)
    {
        int dummy_int = -1;

        ret = make_minmax_fldval(nest, i, &fval[i], &flag, &dummy_int, MDB_GE);
        if (ret != RET_SUCCESS)
            goto error;
    }

    dbi_KeyValue(i, fval, key);

    return RET_SUCCESS;

  error:
    for (j = 0; j < i; j++)
    {
        if (IS_ALLOCATED_TYPE(fval[j].type_))
            PMEM_FREENUL(fval[j].value_.pointer_);
        else if (fval[j].type_ == DT_DECIMAL)
            PMEM_FREENUL(fval[j].value_.decimal_);
    }
    return ret;
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
int
delete_single_keyvalue(T_KEYDESC * key)
{
    int n;

    if (key == NULL)
        return RET_SUCCESS;

    if (key->field_value_ == NULL || key->field_count_ == 0)
    {
        key->field_value_ = NULL;
        key->field_count_ = 0;
        return RET_SUCCESS;
    }

    for (n = 0; n < key->field_count_; n++)
    {
        switch (key->field_value_[n].type_)
        {
        case DT_VARCHAR:
        case DT_NVARCHAR:
        case DT_CHAR:
        case DT_NCHAR:
            PMEM_FREENUL(key->field_value_[n].value_.pointer_);
            break;
        case DT_DECIMAL:
            PMEM_FREENUL(key->field_value_[n].value_.decimal_);
            break;
        default:
            break;
        }
    }

    PMEM_FREENUL(key->field_value_);

    key->field_count_ = 0;

    return RET_SUCCESS;
}

/*****************************************************************************/
//! make_keyvalue_for_merge 

/*! \breif  key를 만든다
 ************************************
 * \param rttable(in)   : run-time table info
 * \param nest(in)      : table 접근 정보 
 * \param key(in/out)   : key
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *****************************************************************************/
static int
make_keyvalue_for_merge(T_RTTABLE * rttable, T_NESTING * nest, T_KEYDESC * key)
{
    FIELDVALUE_T *fieldval;
    T_VALUEDESC value, res;
    ibool isFirst, isEqual;
    MDB_UINT8 bound;
    int i, j, ret;

    if (key)
    {
        key->field_count_ = nest->index_field_num;
        key->field_value_ = (struct FieldValue *)
                PMEM_ALLOC(sizeof(struct FieldValue) * key->field_count_);
        if (key->field_value_ == NULL)
        {
            key->field_count_ = 0;
            return DB_E_OUTOFMEMORY;
        }

        fieldval = key->field_value_;
    }

    isFirst = (rttable->status == SCS_CLOSE ? 1 : 0);
    isEqual = 1;

    for (i = 0; i < nest->index_field_num; i++)
    {
        sc_memset(&value, 0, sizeof(T_VALUEDESC));

        if (nest->min[i].expr.len == 0)
        {
            if (key != NULL)
            {
                /* set meaningless key value for partial key */
                value.valueclass = SVC_NONE;
                value.valuetype = DT_NULL_TYPE;
                bound = MDB_LE;
                sc_memcpy(&value.attrdesc, &nest->index_field_info[i],
                        sizeof(T_ATTRDESC));
                ret = make_fieldvalue(&value, &fieldval[i], &bound);
                if (ret != RET_SUCCESS)
                {
                    /* possible values of ret: RET_ERROR */
                    goto error;
                }
            }
            continue;
        }

        ret = calc_expression(&nest->min[i].expr, &value, MDB_FALSE);
        if (ret < 0)
        {
            goto error;
        }
        if (value.is_null)
        {
            ret = RET_FALSE;
            goto error;
        }

        sc_memcpy(&value.attrdesc, &nest->index_field_info[i],
                sizeof(T_ATTRDESC));

        // expression에 대한 type checking이 일어나기 전에 expression이
        // 수행되므로 별도로 key expression에 대한 type checking을 하자
        if (IS_NUMERIC(value.attrdesc.type))
        {
            if (IS_STRING(value.valuetype))
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                        SQL_E_INCONSISTENTTYPE, 0);
                ret = RET_ERROR;
                goto error;
            }
        }
        else
        {
            if (IS_NUMERIC(value.valuetype))
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                        SQL_E_INCONSISTENTTYPE, 0);
                ret = RET_ERROR;
                goto error;
            }
        }

        if (i < rttable->cnt_merge_item)
        {
            if (isFirst ||
                    calc_equal(&rttable->merge_key_values[i],
                            &value, &res) != RET_SUCCESS || res.u.bl == 0)
            {
                sql_value_ptrFree(&rttable->merge_key_values[i]);

                sc_memcpy(&rttable->merge_key_values[i], &value,
                        sizeof(T_VALUEDESC));

                if (IS_ALLOCATED_TYPE(value.valuetype))
                {
                    sql_value_ptrInit(&rttable->merge_key_values[i]);
                    if (sql_value_ptrXcalloc(&rttable->merge_key_values[i],
                                    value.attrdesc.len + 1) < 0)
                    {
                        ret = RET_ERROR;
                        goto error;
                    }
                    memcpy_func[value.valuetype] (rttable->merge_key_values[i].
                            u.ptr, value.u.ptr, value.attrdesc.len);
#ifdef MDB_DEBUG
                    if (value.value_len <= 0)
                        sc_assert(0, __FILE__, __LINE__);
#endif

                    rttable->merge_key_values[i].value_len = value.value_len;
                }
                isEqual = 0;
            }
        }

        if (key != NULL)
        {
            bound = MDB_EQ;
            ret = make_fieldvalue(&value, &fieldval[i], &bound);
            if (ret != RET_SUCCESS)
            {
                /* possible values of ret: RET_ERROR | RET_FALSE */
                /* RET_TRUE cannot be returned. Because, bound is MDB_EQ. */
                goto error;
            }
        }
        sql_value_ptrFree(&value);
    }

    if (isEqual)
        return 0;
    else
        return 1;

  error:
    sql_value_ptrFree(&value);
    if (key != NULL)
    {
        for (j = 0; j < i; j++)
        {
            if (IS_ALLOCATED_TYPE(fieldval[j].type_))
                PMEM_FREENUL(fieldval[j].value_.pointer_);
            else if (fieldval[j].type_ == DT_DECIMAL)
                PMEM_FREENUL(fieldval[j].value_.decimal_);
        }
    }

    if (key)
    {
        key->field_count_ = 0;
        PMEM_FREENUL(key->field_value_);
    }

    return ret;
}

int
filter_mode(T_OPTYPE optype)
{
    switch (optype)
    {
    case SOT_LIKE:
        return LIKE;

    case SOT_ILIKE:
        return MDB_ILIKE;

    case SOT_LT:
        return MDB_LT;

    case SOT_LE:
        return MDB_LE;

    case SOT_GT:
        return MDB_GT;

    case SOT_GE:
        return MDB_GE;

    case SOT_EQ:
        return MDB_EQ;

    case SOT_NE:
        return MDB_NE;

    default:
        return -1;
    }
}

/*****************************************************************************/
//! make_fieldvalue_for_filter

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param fval(in/out)  :
 * \param lval(in)      : 
 * \param rval(in)      :
 * \param op(in)        :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘 및 기타 설명
 *  - called : make_filter()
 *****************************************************************************/
int
make_fieldvalue_for_filter(FIELDVALUE_T * fval, T_POSTFIXELEM * lval,
        T_POSTFIXELEM * rval, T_POSTFIXELEM * op)
{
    T_VALUEDESC vdes;
    MDB_UINT8 bound;
    int ret;

    sc_memset(&vdes, 0, sizeof(T_VALUEDESC));

    if (lval == NULL || op == NULL)
        return RET_ERROR;

    if (rval == NULL)
    {
        int op_tbl[] = { MDB_NU, MDB_NN };
        sc_memcpy(&vdes.attrdesc, &lval->u.value.attrdesc, sizeof(T_ATTRDESC));
        vdes.valueclass = SVC_NONE;
        vdes.valuetype = DT_NULL_TYPE;
        bound = op_tbl[op->u.op.type - SOT_ISNULL];
        ret = make_fieldvalue(&vdes, fval, &bound);
        if (ret != RET_SUCCESS)
        {
            /* possible values of ret: RET_ERROR */
            return ret;
        }
        fval->mode_ = bound;
    }
    else
    {
        sc_memcpy(&vdes.attrdesc, &lval->u.value.attrdesc, sizeof(T_ATTRDESC));
        vdes.valuetype = rval->u.value.valuetype;
        vdes.valueclass = rval->u.value.valueclass;
        vdes.is_null = rval->u.value.is_null;
        vdes.u = rval->u.value.u;
        bound = filter_mode(op->u.op.type);
        ret = make_fieldvalue(&vdes, fval, &bound);
        if (ret != RET_SUCCESS)
        {
            /* possible values of ret: RET_ERROR | RET_FALSE | RET_TRUE */
            /* Caller function must process according to the return value. */
            return ret;
        }
        if ((bound == LIKE || bound == MDB_ILIKE))
        {
            void *newLike = NULL;

            /* nchar ilike : error */
            if ((bound == MDB_ILIKE && IS_NBS(fval->type_)))
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                        SQL_E_INVALIDEXPRESSION, 0);
                return SQL_E_INVALIDEXPRESSION;
            }

            ret = dbi_prepare_like(fval->value_.pointer_,
                    fval->type_, bound,
                    (fval->collation == MDB_COL_CHAR_REVERSE
                            ? fval->length_ : -1), &newLike);
            if (ret < 0)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
                return ret;
            }
            PMEM_FREENUL(fval->value_.pointer_);
            fval->value_.pointer_ = newLike;
        }
        fval->mode_ = bound;
    }
    return RET_SUCCESS;
}

/*****************************************************************************/
//! make_filter 

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param expr_list() :
 * \param nest() : 
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *  - T_EXPRESSIONDESC 구조체 변경
 *
 *****************************************************************************/
int
make_filter(EXPRDESC_LIST * expr_list, T_NESTING * nest)
{
    EXPRDESC_LIST *cur;
    T_EXPRESSIONDESC *expr, valexpr;
    T_POSTFIXELEM value_org;
    T_POSTFIXELEM *col, *op, *value = NULL;
    FIELDVALUE_T *fl_value, *expression_;
    int n_expr;
    int ret;

    valexpr.list = NULL;

    PMEM_FREENUL(nest->filter);

    for (cur = expr_list, n_expr = 0; cur; cur = cur->next)
        n_expr++;

    value = &value_org;

    nest->filter = (T_FILTER *) PMEM_ALLOC(sizeof(T_FILTER));
    if (nest->filter == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
        return SQL_E_OUTOFMEMORY;
    }

    nest->filter->expression_ =
            (FIELDVALUE_T *) PMEM_ALLOC(n_expr * sizeof(FIELDVALUE_T));
    if (nest->filter->expression_ == NULL)
    {
        PMEM_FREENUL(nest->filter);
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
        return SQL_E_OUTOFMEMORY;
    }
    nest->filter->expression_count_ = n_expr;
    fl_value = nest->filter->expression_;

    for (cur = expr_list; cur; cur = cur->next)
    {
        expr = (T_EXPRESSIONDESC *) cur->ptr;

        /* 'expr' is one of {'col something op', 'col op'} */
        col = expr->list[0];
        op = expr->list[expr->len - 1];
        if (op->u.op.argcnt == 1)
        {
            ret = make_fieldvalue_for_filter(fl_value, col, NULL, op);
            if (ret < 0 || ret == RET_FALSE)
                goto error;
        }
        else if (op->u.op.type == SOT_EQ && expr->list[1]->u.value.is_null &&
                expr->list[1]->u.value.param_idx > 0)
        {
            op->u.op.argcnt = 1;
            op->u.op.type = SOT_ISNULL;
            ret = make_fieldvalue_for_filter(fl_value, col, NULL, op);
            if (ret < 0 || ret == RET_FALSE)
                goto error;
        }
        else
        {   /* op->u.op.argcnt == 2 */
            valexpr.max = expr->max - 1;
            valexpr.len = expr->len - 2;
            valexpr.list = &(expr->list[1]);

            ret = calc_expression(&valexpr, &value->u.value, MDB_FALSE);
            if (ret != RET_SUCCESS)
                goto error;
            ret = make_fieldvalue_for_filter(fl_value, col, value, op);
            sql_value_ptrFree(&value->u.value);
            if (ret < 0 || ret == RET_FALSE)
                goto error;
            else if (ret == RET_TRUE)
            {
                sc_memset(fl_value, 0, sizeof(FIELDVALUE_T));
                fl_value--;
                n_expr--;
            }
        }
        fl_value++;
    }
    if (nest->filter->expression_count_ > n_expr)
    {
        if (n_expr == 0)
        {
            PMEM_FREENUL(nest->filter->expression_);
            PMEM_FREENUL(nest->filter);
        }
        else
        {
            expression_ =
                    (FIELDVALUE_T *) PMEM_ALLOC(n_expr * sizeof(FIELDVALUE_T));
            if (expression_ == NULL)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
                ret = SQL_E_OUTOFMEMORY;
                goto error;
            }
            sc_memcpy(expression_, nest->filter->expression_,
                    n_expr * sizeof(FIELDVALUE_T));
            PMEM_FREENUL(nest->filter->expression_);
            nest->filter->expression_ = expression_;
            nest->filter->expression_count_ = n_expr;
        }
    }

    return RET_SUCCESS;

  error:
    Filter_Delete(nest->filter);
    PMEM_FREENUL(nest->filter);
    return ret;
}

/*****************************************************************************/
//! make_filter_for_scandesc 

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param nest :
 * \param scandesc : 
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *  - T_EXPRESSIONDESC 구조체 변경
 *
 *****************************************************************************/
int
make_filter_for_scandesc(T_NESTING * nest, SCANDESC_T * scandesc)
{
    T_FILTER *filter = &scandesc->filter;
    int n_expr, i, ret;
    T_EXPRESSIONDESC *expr, valexpr;
    T_POSTFIXELEM value_org;
    T_POSTFIXELEM *col, *op, *value = NULL;
    FIELDVALUE_T *fl_value, *expression_;

    valexpr.list = NULL;

    if (nest->indexable_orexpr_count == 0)
        return RET_SUCCESS;

    value = &value_org;

    n_expr = nest->indexable_orexpr_count;
    filter->expression_ = PMEM_ALLOC(n_expr * sizeof(FIELDVALUE_T));
    if (filter->expression_ == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
        return SQL_E_OUTOFMEMORY;
    }
    sc_memset(filter->expression_, 0, (n_expr * sizeof(FIELDVALUE_T)));
    filter->expression_count_ = n_expr;

    fl_value = filter->expression_;
    for (i = 0; i < nest->indexable_orexpr_count; i++)
    {
        expr = nest->indexable_orexprs[i]->expr;

        /* 'expr' is one of {'col something op', 'col op'} */
        op = expr->list[expr->len - 1];
        if (nest->indexable_orexprs[i]->type & ORT_REVERSED)
        {
            col = expr->list[expr->len - 2];
            if (op->u.op.argcnt > 1)
            {   /* argcnt == 2 */
                valexpr.max = expr->max - 1;
                valexpr.len = expr->len - 2;
                valexpr.list = &(expr->list[0]);
            }
        }
        else
        {
            col = expr->list[0];
            if (op->u.op.argcnt > 1)
            {   /* argcnt == 2 */
                valexpr.max = expr->max - 1;
                valexpr.len = expr->len - 2;
                valexpr.list = &(expr->list[1]);
            }
        }

        if (op->u.op.argcnt == 1)
        {
            ret = make_fieldvalue_for_filter(fl_value, col, NULL, op);
            if (ret < 0 || ret == RET_FALSE)
                goto error;
        }
        else
        {   /* op->u.op.argcnt == 2 */
            ret = calc_expression(&valexpr, &value->u.value, MDB_FALSE);
            if (ret != RET_SUCCESS)
                goto error;
            if (value->u.value.is_null)
            {
                ret = RET_FALSE;
                goto error;
            }
            ret = make_fieldvalue_for_filter(fl_value, col, value, op);

            sql_value_ptrFree(&value->u.value);

            if (ret < 0 || ret == RET_FALSE)
                goto error;
            else if (ret == RET_TRUE)
            {
                sc_memset(fl_value, 0, sizeof(FIELDVALUE_T));
                fl_value--;
                n_expr--;
            }
        }
        fl_value++;
    }

    if (filter->expression_count_ > n_expr)
    {
        if (n_expr == 0)
        {
            PMEM_FREENUL(filter->expression_);
            filter->expression_count_ = 0;
        }
        else
        {
            expression_ =
                    (FIELDVALUE_T *) PMEM_ALLOC(n_expr * sizeof(FIELDVALUE_T));
            if (expression_ == NULL)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
                ret = SQL_E_OUTOFMEMORY;
                goto error;
            }
            sc_memcpy(expression_, filter->expression_,
                    n_expr * sizeof(FIELDVALUE_T));
            PMEM_FREENUL(filter->expression_);
            filter->expression_ = expression_;
            filter->expression_count_ = n_expr;
        }
    }

    return RET_SUCCESS;

  error:
    KeyValue_Delete((struct KeyValue *) filter);
    return ret;
}

int
make_keyrange_for_scandesc(T_NESTING * nest, SCANDESC_T * scandesc)
{
    int ret;
    int dummy_int;

    if (nest->iscan_list)
    {
        scandesc->idxname = PMEM_STRDUP(nest->iscan_list->indexinfo.indexName);
        if (scandesc->idxname == NULL)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
            return SQL_E_OUTOFMEMORY;
        }
    }

    dummy_int = -1;
    ret = make_keyvalue(nest, &scandesc->minkey, &scandesc->maxkey,
            &dummy_int);

    if (ret < 0 || ret == RET_FALSE)
    {
        PMEM_FREENUL(scandesc->idxname);
        return ret;
    }

    return RET_SUCCESS;
}


/*****************************************************************************/
//! open_cursor 

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param connid(in) :
 * \param rttable(in/out)   : run time table
 * \param nest(in/out)      : key 조건
 * \param expr_list(in/out) : filter 조건
 * \param mode(in)          :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n *      및 기타 설명
 *  - filter를 만든후, key를 만드는 부분에서 에러 발생시. filter 메모리를 해제
 *****************************************************************************/
int
open_cursor(int connid, T_RTTABLE * rttable, T_NESTING * nest,
        EXPRDESC_LIST * expr_list, MDB_UINT8 mode)
{
    T_KEYDESC minkey, maxkey;
    int ret = RET_SUCCESS;

    if (rttable->status != SCS_CLOSE)
        return RET_SUCCESS;

    if (nest == NULL)   /* in case of insertion */
    {
        rttable->Hcursor = dbi_Cursor_Open(connid, rttable->table.tablename,
                NULL, NULL, NULL, NULL, mode, rttable->msync_flag);
    }
    else    /* the other cases => select, delete, update */
    {
        if (expr_list != NULL)
        {
            ret = make_filter(expr_list, nest);
            if (ret < 0)
                return ret;
        }

        if (ret == RET_FALSE)
        {
            rttable->Hcursor =
                    dbi_Cursor_Open(connid, rttable->table.tablename,
                    NULL, NULL, NULL, NULL, mode, rttable->msync_flag);
        }
        else
        {
            if (nest->index_field_num > 0)
            {
                // NOTES
                // 만약, key를 generation하다가 실패할 경우,
                // 그냥 full scan으로 가자.
                if (rttable->cnt_merge_item)
                {       /* sort merge join */
                    rttable->before_cursor_posi = NULL;
                    ret = make_keyvalue_for_merge(rttable, nest, &minkey);
                    if (ret < 0)
                        return ret;
                    if (ret == RET_FALSE)
                    {
                        rttable->Hcursor =
                                dbi_Cursor_Open(connid,
                                rttable->table.tablename, nest->indexname,
                                NULL, NULL, nest->filter, mode,
                                rttable->msync_flag);
                    }
                    else
                    {
                        rttable->Hcursor =
                                dbi_Cursor_Open(connid,
                                rttable->table.tablename, nest->indexname,
                                &minkey, NULL, nest->filter, mode,
                                rttable->msync_flag);

                        if (rttable->before_cursor_posi == NULL)
                            rttable->before_cursor_posi =
                                    PMEM_ALLOC(sizeof(t_cursorposi));
                    }
                }
                else
                {       /* nested-loop join */
                    ret = make_keyvalue(nest, &minkey, &maxkey,
                            &rttable->in_expr_idx);
                    if (ret < 0)
                        goto ERROR_LABEL;
                    if (ret == RET_FALSE)
                    {
                        if (nest->opentype == DESCENDING)
                            rttable->Hcursor =
                                    dbi_Cursor_Open_Desc(connid,
                                    rttable->table.tablename, nest->indexname,
                                    NULL, NULL, nest->filter, mode);
                        else
                            rttable->Hcursor =
                                    dbi_Cursor_Open(connid,
                                    rttable->table.tablename, nest->indexname,
                                    NULL, NULL, nest->filter, mode,
                                    rttable->msync_flag);
                    }
                    else
                    {
                        if (nest->opentype == DESCENDING)
                            rttable->Hcursor =
                                    dbi_Cursor_Open_Desc(connid,
                                    rttable->table.tablename, nest->indexname,
                                    &minkey, &maxkey, nest->filter, mode);
                        else
                            rttable->Hcursor =
                                    dbi_Cursor_Open(connid,
                                    rttable->table.tablename, nest->indexname,
                                    &minkey, &maxkey, nest->filter, mode,
                                    rttable->msync_flag);
                    }
                }
            }
            else
            {
                rttable->Hcursor =
                        dbi_Cursor_Open(connid,
                        rttable->table.tablename, nest->indexname,
                        NULL, NULL, nest->filter, mode, rttable->msync_flag);
            }
        }

        PMEM_FREENUL(nest->filter);
    }

    if (rttable->Hcursor < 0)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, rttable->Hcursor, 0);
        return rttable->Hcursor;
    }

    if (ret == RET_FALSE)
        rttable->status = SCS_REOPEN;
    else
        rttable->status = SCS_OPEN;
    return ret;

  ERROR_LABEL:
    // Filter를 만든후에 key를 만드는 과정에서 뻑나면.. 요 루틴으로 와야한다
    if (nest->filter)
        PMEM_FREENUL(nest->filter->expression_);

    return ret;
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
int
open_cursor_for_insertion(int connid, T_RTTABLE * rttable, MDB_UINT8 mode)
{
    SYSTABLE_T tableinfo;
    T_INDEXINFO index;
    T_ATTRDESC attr;
    int ret;

    ret = dbi_Schema_GetTableInfo(connid, rttable->table.tablename,
            &tableinfo);
    if (ret < 0)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
        return ret;
    }

    if (!IS_FIXED_RECORDS_TABLE(tableinfo.maxRecords)
            || tableinfo.columnName[0] == '#')
    {
        rttable->Hcursor =
                dbi_Cursor_Open(connid,
                rttable->table.tablename, NULL, NULL, NULL, NULL, mode,
                rttable->msync_flag);
    }
    else
    {

        /* cursor open for fixed sized table */

        sc_memset(&index, 0, sizeof(T_INDEXINFO));

        attr.table.tablename = tableinfo.tableName;
        attr.attrname = tableinfo.columnName;
        attr.collation = MDB_COL_NONE;

        ret = set_attrinfobyname(connid, &attr, NULL);
        if (ret != RET_SUCCESS)
            return ret;

        ret = is_firstmemberofindex(connid, &attr, NULL, &index);
        if (ret != RET_SUCCESS)
        {
            PMEM_FREENUL(index.fields);
            return ret;
        }

        rttable->Hcursor = dbi_Cursor_Open(connid,
                rttable->table.tablename, index.info.indexName,
                NULL, NULL, NULL, mode, rttable->msync_flag);

        PMEM_FREENUL(index.fields);
    }

    if (rttable->Hcursor < 0)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, rttable->Hcursor, 0);
        return rttable->Hcursor;
    }

    rttable->status = SCS_OPEN;

    return ret;
}

/*****************************************************************************/
//! reopen_cursor

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param connid(in)        :
 * \param rttable(in/out)   : run-time table
 * \param nest(in)          :
 * \param expr_list(in/out) :
 * \param mode(in)          :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
reopen_cursor(int connid, T_RTTABLE * rttable, T_NESTING * nest,
        EXPRDESC_LIST * expr_list, MDB_UINT8 mode)
{
    T_KEYDESC minkey, maxkey;
    int ret = RET_SUCCESS;

    if (rttable->status == SCS_CLOSE)
        return open_cursor(connid, rttable, nest, expr_list, mode);

    if (rttable->status != SCS_REOPEN)
        return ret;

    if (nest == NULL)
    {
        ret = dbi_Cursor_Reopen(connid, rttable->Hcursor, NULL, NULL, NULL);
    }
    else    /* nest != NULL */
    {
        if (expr_list != NULL)
        {
            ret = make_filter(expr_list, nest);
            if (ret < 0)
                return ret;
        }

        if (ret == RET_FALSE)
        {
            if (dbi_Cursor_Reopen(connid, rttable->Hcursor,
                            NULL, NULL, NULL) != DB_SUCCESS)
                ret = RET_ERROR;
        }
        else
        {
            if (nest->index_field_num > 0)
            {
                if (rttable->cnt_merge_item)
                {       /* sort merge join */
                    ret = make_keyvalue_for_merge(rttable, nest, &minkey);
                    if (ret < 0)
                        return ret;
                    if (ret == RET_FALSE)
                    {
                        if (dbi_Cursor_Reopen(connid, rttable->Hcursor,
                                        NULL, NULL, nest->filter)
                                != DB_SUCCESS)
                            ret = RET_ERROR;
                    }
                    else
                    {
                        ret = dbi_Cursor_Reopen(connid, rttable->Hcursor,
                                &minkey, NULL, nest->filter);
                    }
                }
                else
                {       /* nested loop join */
                    ret = make_keyvalue(nest, &minkey, &maxkey,
                            &rttable->in_expr_idx);
                    if (ret < 0)
                        return ret;
                    if (ret == RET_FALSE)
                    {
                        if (dbi_Cursor_Reopen(connid, rttable->Hcursor,
                                        NULL, NULL, nest->filter)
                                != DB_SUCCESS)
                            ret = RET_ERROR;
                    }
                    else
                    {
                        ret = dbi_Cursor_Reopen(connid, rttable->Hcursor,
                                &minkey, &maxkey, nest->filter);
                    }
                }
            }
            else
            {
                ret = dbi_Cursor_Reopen(connid, rttable->Hcursor,
                        NULL, NULL, nest->filter);
            }
        }

        PMEM_FREENUL(nest->filter);
    }

    if (ret < 0)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
        return RET_ERROR;
    }

    if (ret == RET_FALSE)
        rttable->status = SCS_REOPEN;
    else
        rttable->status = SCS_OPEN;
    return ret;
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
int
reposition_cursor(int connid, T_RTTABLE * rttable, T_NESTING * nest)
{
    int ret;

#ifdef MDB_DEBUG
    if (nest == NULL || nest->index_field_num <= 0)
        return RET_ERROR;
#endif

    ret = make_keyvalue_for_merge(rttable, nest, NULL);
    if (ret < 0)
        return ret;

    if (ret == 0)
    {       /* key equals */
        /* The prev records must be accessed */
        rttable->prev_rec_idx = 0;
    }
    else if (ret == 1 || ret == RET_FALSE)
    {
        /* key doesn't equal */
        /* The next record must be accessed */
        /* clear saved prev records */
        if (rttable->prev_rec_cnt > 0)
        {
            rttable->prev_rec_cnt = 0;
            rttable->has_prev_cursor_posi = 0;
        }
    }

    /* rttable->status : SCS_REPOSITION or SCS_SM_LAST */
    if (rttable->status == SCS_REPOSITION)
    {
        rttable->status = SCS_OPEN;
        return RET_SUCCESS;
    }
    else
    {       /* SCS_SM_LAST */
        return RET_FALSE;
    }
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
void
close_cursor(int connid, T_RTTABLE * rttable)
{
    if (rttable->status)
    {
        dbi_Cursor_Close(connid, rttable->Hcursor);
        rttable->status = SCS_CLOSE;
    }
}

/*****************************************************************************/
//! evaluate_expression 

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param expr :
 * \param is_orexpr : 
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *  - T_EXPRESSIONDESC 구조체 변경
 *  - T_POSTFIXELEM LIST를 할당함
 *
 *****************************************************************************/
int
evaluate_expression(T_EXPRESSIONDESC * expr, MDB_BOOL is_orexpr)
{
    T_EXPRESSIONDESC stack;
    T_POSTFIXELEM result_org;
    T_POSTFIXELEM *elem, **args = NULL, *result = NULL;
    int i, j, ret = RET_SUCCESS;
    int call_free_bk;

    // subquery 
    int result_len;
    char null_value_mask;
    char *result_buffer;

    if (expr->list == NULL)
        return RET_SUCCESS;

    INIT_STACK(stack, expr->len, ret);
    if (ret < 0)
        return RET_ERROR;

    result = &result_org;

    for (i = 0; i < expr->len; i++)
    {
        sc_memset(result, 0, sizeof(T_POSTFIXELEM));

        elem = expr->list[i];

        if (elem->elemtype == SPT_VALUE)
        {
            call_free_bk = elem->u.value.call_free;
            elem->u.value.call_free = 0;
            PUSH_STACK(stack, (*elem), ret);
            elem->u.value.call_free = call_free_bk;
            if (ret < 0)
                break;
        }
        else if (elem->elemtype == SPT_SUBQUERY)
        {
            // 수행하고, uncorrelated subquery의 경우면 TSS_EXECUTED로 설정
            if (i + 1 < expr->len
                    && expr->list[i + 1]->elemtype != SPT_SUBQ_OP)
            {
                T_QUERYRESULT *qresult;

                // 1. correlated variable binding
                // 2. subquery execution:수행 결과는 subq_result_rows & rows에
                // 3. postfix element setting
                //   exec_select()
                ret = exec_singleton_subq(elem->u.subq->handle, elem->u.subq);
                if (ret < 0)
                    break;
                /* check_condition()에서 subquery 검사. 불필요한 작업 제거 */
                if (elem->u.subq->planquerydesc.querydesc.setlist.len > 0)
                {
                    qresult =
                            &elem->u.subq->planquerydesc.querydesc.setlist.
                            list[0]->u.subq->queryresult;
                }
                else
                {
                    qresult = &elem->u.subq->queryresult;
                }
#ifdef MDB_DEBUG
                sc_assert(elem->u.subq->kindofwTable & iTABLE_SUBWHERE,
                        __FILE__, __LINE__);
                sc_assert((char *) &elem->u.value.u.bl ==
                        (char *) &elem->u.value.u.i8, __FILE__, __LINE__);
                sc_assert((char *) &elem->u.value.u.bl ==
                        (char *) &elem->u.value.u.f16, __FILE__, __LINE__);
                sc_assert((char *) &elem->u.value.u.bl ==
                        (char *) &elem->u.value.u.dec, __FILE__, __LINE__);
                sc_assert((char *) &elem->u.value.u.bl ==
                        (char *) &elem->u.value.u.ch, __FILE__, __LINE__);
                sc_assert((char *) &elem->u.value.u.bl ==
                        (char *) &elem->u.value.u.ptr, __FILE__, __LINE__);
#endif
                // node의 타입을 변경하고, 
                result->elemtype = SPT_VALUE;

                result_len = one_binary_column_length(qresult->list, 1);
                result_buffer = (char *) elem->u.subq->subq_result_rows->data;
                if (result_buffer)
                {
                    // 결과 칼럼 수가 1개이므로 첫 번째 byte를 본다.
                    null_value_mask = result_buffer[sizeof(int) + 0];
                    // subquery의 수행 결과로 저장된 scalar 값을 elem에 저장하고,
                    sc_memcpy(&result->u.value, &qresult->list[0].value,
                            sizeof(T_VALUEDESC));

                    result->u.value.call_free = 0;

                    // 레코드 구조는 bin_result_record_len()함수의 주석 참조
                    // 칼럼수가 1개
                    result->u.value.is_null = null_value_mask & 0x1;
                    if (IS_BS(result->u.value.valuetype))
                        result->u.value.u.ptr =
                                result_buffer + sizeof(int) + 1;
                    else if (IS_NBS(result->u.value.valuetype))
                    {
                        /* one_binary_column_length():
                         * length is a byte length.
                         */
                        result_len = result_len / sizeof(DB_WCHAR);
                        if (sql_value_ptrXcalloc(&result->u.value,
                                        result_len + 1) < 0)
                        {
                        }
                        sc_wmemcpy((DB_WCHAR *) result->u.value.u.ptr,
                                (DB_WCHAR *) (result_buffer + sizeof(int) + 1),
                                result_len);
                    }
                    else
                        sc_memcpy(&result->u.value.u,
                                result_buffer + sizeof(int) + 1, result_len);
                }
                else
                    result->u.value.is_null = 1;

                // uncorrelated 인 경우는 다음에 수행 안 하도록 하자.
                if ((elem->u.subq->kindofwTable & iTABLE_CORRELATED) == 0)
                {
                    elem->u.subq->tstatus = TSS_EXECUTED;
                    // TODO: 어디선가 free(elem->u.subq->subq_result_rows) 하고 널로
                    // 설정;
                }
            }
            else
            {
                sc_memcpy(result, elem, sizeof(T_POSTFIXELEM));
                result->u.value.call_free = 0;
            }

            PUSH_STACK(stack, (*result), ret);
            if (ret < 0)
                break;

        }
        else if (elem->elemtype == SPT_SUBQ_OP)
        {
            POP_STACK_ARRAY(stack, args, elem->u.op.argcnt, ret);
            if (ret < 0)
                break;

            ret = calc_subqexpr(args[elem->u.op.type == SOT_EXISTS ||
                            elem->u.op.type ==
                            SOT_NEXISTS ? 0 : 1]->u.subq->handle, &elem->u.op,
                    result, args);

            if (args[0]->elemtype != SPT_SUBQUERY
                    && IS_BS_OR_NBS(args[0]->u.value.valuetype))
            {
                sql_value_ptrFree(&args[0]->u.value);
            }

            if (ret < 0)
                break;

#ifdef MDB_DEBUG
            sc_assert(result->u.value.valuetype == DT_BOOL, __FILE__,
                    __LINE__);
#endif

            PUSH_STACK(stack, (*result), ret);
            if (ret < 0)
                break;

        }
        else if (elem->elemtype == SPT_OPERATOR)
        {
            POP_STACK_ARRAY(stack, args, elem->u.op.argcnt, ret);
            if (ret < 0)
                break;

            if (elem->u.op.type == SOT_IN || elem->u.op.type == SOT_NIN)
            {
                /* type convert를 수행하여 실패하는 경우 null 처리 */
                for (j = 1; j < elem->u.op.argcnt; j++)
                {
                    if (args[0]->u.value.valuetype !=
                            args[j]->u.value.valuetype)
                    {
                        T_VALUEDESC base, res;

                        base = args[0]->u.value;
                        base.valueclass = SVC_CONSTANT;
                        sc_memset(&res, 0, sizeof(T_VALUEDESC));

                        ret = calc_func_convert(&base,
                                &(args[j]->u.value), &res, MDB_FALSE);
                        sql_value_ptrFree(&args[j]->u.value);

                        if (ret < 0)
                        {
                            er_clear();
                            args[j]->u.value.is_null = 1;
                        }
                        else
                        {
                            args[j]->u.value = res;
                        }
                    }
                }
            }
            else if (elem->u.op.argcnt >= 2
                    && elem->u.op.type != SOT_LIKE
                    && elem->u.op.type != SOT_ILIKE)
            {
                for (j = 1; j < elem->u.op.argcnt; j++)
                {
                    if (args[0]->u.value.valuetype !=
                            args[j]->u.value.valuetype)
                    {
                        ret = type_convert(&(args[0]->u.value),
                                &(args[j]->u.value));
                        if (ret < 0)
                        {
                            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
                            break;
                        }
                    }
                }
            }
            // FIX_LATTER : 함수의 인자가 없어서.. 매번 EXECUTION으로 임시로 넣음
            //              (나중에 고칠것)
            ret = calc_simpleexpr(&elem->u.op, result, args, MDB_FALSE);
            for (j = 0; j < elem->u.op.argcnt; j++)
            {
                if (IS_ALLOCATED_TYPE(args[j]->u.value.valuetype))
                    sql_value_ptrFree(&args[j]->u.value);

            }
            if (ret < 0)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
                break;
            }
            if (IS_N_STRING(result->u.value.valuetype))
                result->u.value.call_free = 1;

            if (elem->u.op.type == SOT_OR &&
                    result->u.value.u.bl == 1 && is_orexpr == 1)
            {
                REMOVE_STACK(stack);
                return 1;
            }
            PUSH_STACK(stack, (*result), ret);
            if (ret < 0)
                break;

        }
        else if (elem->elemtype == SPT_FUNCTION)
        {
            POP_STACK_ARRAY(stack, args, elem->u.func.argcnt, ret);
            if (ret < 0)
                break;

            ret = calc_function(&elem->u.func, result, args, MDB_FALSE);
            for (j = 0; j < elem->u.func.argcnt; j++)
                if (IS_ALLOCATED_TYPE(args[j]->u.value.valuetype))
                    sql_value_ptrFree(&args[j]->u.value);

            if (ret < 0)
                break;
            if (IS_N_STRING(result->u.value.valuetype))
                result->u.value.call_free = 1;

            PUSH_STACK(stack, (*result), ret);
            if (ret < 0)
                break;


        }
        else if (elem->elemtype == SPT_AGGREGATION)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, RET_ERROR, 0);
            ret = RET_ERROR;
            break;
        }
    }

    if (ret > 0)
    {
        POP_STACK(stack, (*result), ret);
        if (ret < 0)
            ret = RET_ERROR;
        else if (result->u.value.valuetype != DT_BOOL)
            ret = RET_ERROR;
        else
            ret = result->u.value.u.bl;

    }
    REMOVE_STACK(stack);

    sql_value_ptrFree(&result->u.value);

    return ret;
}


/*****************************************************************************/
//! evaluate_conjunct 

/*! \breif 논리곱 조건식을 평가한다.
 ************************************
 * \param list : 논리곱 형태의 조건식
 ************************************
 * \return true, false or error
 ************************************
 * \note 
 *****************************************************************************/
int
evaluate_conjunct(EXPRDESC_LIST * list)
{
    EXPRDESC_LIST *cur;
    int ret;

    for (cur = list; cur; cur = cur->next)
    {
        ret = evaluate_expression(cur->ptr, 1);
        if (ret <= 0)
            return ret; /* false, stop evaluation */
    }
    return 1;
}

///////////////////////// Util Functions End /////////////////////////


//////////////////////////// Main Function ////////////////////////////

/*****************************************************************************/
//! exec_ddl

/*! \breif  DDL 문장들 수행
 ************************************
 * \param handle(in)    
 * \param stmt(in/out)
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘 및 기타 설명
 *  - ADMIN 문장 추가(EXPORT & IMPORT)
 *****************************************************************************/
int
exec_ddl(int handle, T_STATEMENT * stmt)
{
    int ret = DB_SUCCESS;

    switch (stmt->sqltype)
    {
    case ST_CREATE:
        ret = exec_create(handle, stmt);
        break;
    case ST_DROP:
        ret = exec_drop(handle, stmt);
        break;
    case ST_RENAME:
        ret = exec_rename(handle, stmt);
        break;
    case ST_ALTER:
        ret = exec_alter(handle, stmt);
        break;
    case ST_TRUNCATE:
        ret = exec_truncate(handle, stmt);
        break;
    case ST_ADMIN:
        ret = exec_admin(handle, stmt);
        if (ret < 0)
            return RET_ERROR;
        break;
    default:
        return RET_ERROR;
    }

    if (ret == RET_ERROR)
        return ret;

    return ret;
}

/*****************************************************************************/
//! exec_dml

/*! \breif  DML 문장을 수행한다
 ************************************
 * \param handle(in)        :
 * \param stmt(in/out)      : 
 * \param result_type(in)   :
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *  
 *****************************************************************************/
int
exec_dml(int handle, T_STATEMENT * stmt, int result_type)
{
    int rows;

    switch (stmt->sqltype)
    {
    case ST_DELETE:
        stmt->u._delete.rows = 0;
        if (exec_delete(handle, stmt) == RET_ERROR)
            return RET_ERROR;
        rows = stmt->u._delete.rows;
        break;
    case ST_UPSERT:
    case ST_INSERT:
        stmt->u._insert.rows = 0;

        if (exec_insert(handle, stmt) == RET_ERROR)
            return RET_ERROR;
        rows = stmt->u._insert.rows;
        break;
    case ST_SELECT:
        stmt->u._select.rows = 0;
        if (stmt->u._select.rowsp)
            *stmt->u._select.rowsp = 0;

        rows = exec_select(handle, stmt, result_type);
        if (rows == RET_ERROR)
            return RET_ERROR;
        break;
    case ST_UPDATE:
        stmt->u._update.rows = 0;
        if (exec_update(handle, stmt) == RET_ERROR)
            return RET_ERROR;
        rows = stmt->u._update.rows;
        break;
    default:
        return RET_ERROR;
    }

    return rows;
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
int
exec_others(int handle, T_STATEMENT * stmt, int result_type)
{
    int rows = 0;

    switch (stmt->sqltype)
    {
    case ST_COMMIT:
        if (SQL_trans_commit(handle, stmt) == RET_ERROR)
            return RET_ERROR;
        break;
    case ST_ROLLBACK:
        if (SQL_trans_rollback(handle, stmt) == RET_ERROR)
            return RET_ERROR;
        break;
    case ST_COMMIT_FLUSH:
        if (SQL_trans_commit(handle, stmt) == RET_ERROR)
            return RET_ERROR;

        dbi_FlushBuffer();
        break;
    case ST_ROLLBACK_FLUSH:
        if (SQL_trans_rollback(handle, stmt) == RET_ERROR)
            return RET_ERROR;

        dbi_FlushBuffer();
        break;
    case ST_SET:
        break;
    default:
        return RET_ERROR;
    }

    return rows;
}

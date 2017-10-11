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
#include "sql_func_timeutil.h"

#include "sql_mclient.h"
#include "mdb_syslog.h"

#include "mdb_Server.h"

T_TREE_NODE *
ALLOC_TREE_NODE(T_POSTFIXELEM * v, T_TREE_NODE * l, T_TREE_NODE * r)
{
    T_TREE_NODE *n;

    n = (T_TREE_NODE *) PMEM_ALLOC(sizeof(T_TREE_NODE));
    if (n == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
        return n;
    }
    else
    {
        n->vptr = v;
        n->left = l;
        n->right = r;
    }

    return n;
}

static const char *__op_names[] = {
    "?", "and", "or", "not", "like",
    "ilike",
    "isnull",
    "isnotnull", "+", "-", "*", "/", "exp", "mod", "merge", "<",
    "<=", ">", ">=", "=", "<>", "in", "not in", "not like",
    "not ilike",
    "some", "<some", "<=some", ">some", ">=some", "=some", "<>some",
    "all", "<all", "<=all", ">all", ">=all", "=all", "<>all",
    "exists", "not exists", "hexcomp", "not hexcomp", "union", "union all",
            "intersect", "except"
};

static const char *__func_names[] = {
    "?", "convert", "decode", "ifnull",
    "round", "sign", "trunc",
    "group count",
    "lowercase", "ltrim", "rtrim", "substring", "uppercase",
    "current_date", "current_time", "current_timestamp",
    "date_add", "date_diff", "date_sub", "date_format",
    "time_add", "time_diff", "time_sub", "time_format",
    "now", "sysdate",
    "chartohex", "hexcomp", "hexnotcomp",
    "random", "srandom", "rownum", "rid",
    "sequence currval", "sequence nextval", "replace", "bytesize", "copyfrom",
            "copyto", "objectsize", "tablename", "hexastring", "binarystring"
};

const char *__aggr_names[] = {
    "?", "count",
    "dirty count",
    "min", "max", "sum", "avg"
};

extern int calc_function(T_FUNCDESC *, T_POSTFIXELEM *, T_POSTFIXELEM **,
        MDB_INT32);

extern int (*sc_ncollate_func[]) (const char *s1, const char *s2, int n);
extern int (*sc_collate_func[]) (const char *s1, const char *s2);

extern int calc_func_convert(T_VALUEDESC * base, T_VALUEDESC * src,
        T_VALUEDESC * res, MDB_INT32 is_init);

/*****************************************************************************/
//! check_expression 

/*! \breif  expression의 인자가 제대로 된 type인지를 검사함
 ************************************
 * \param op(in) :
 * \param elem(in) : 
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *  - elem이 (T_POSTFIXELEM*)에서 (T_POSTFIXELEM**)로 변화
 *
 *****************************************************************************/
int
check_expression(T_OPDESC * op, T_POSTFIXELEM ** elem)
{
    int i, is_numeric, is_string;

    for (i = 0; i < op->argcnt; i++)
    {
        if (elem[i]->u.value.is_null)
            return RET_SUCCESS;
    }
    switch (op->type)
    {
    case SOT_AND:
    case SOT_OR:
        if (elem[0]->u.value.valuetype != DT_BOOL ||
                elem[1]->u.value.valuetype != DT_BOOL)
            return RET_ERROR;
        break;
    case SOT_NOT:
        if (elem[0]->u.value.valuetype != DT_BOOL)
            return RET_ERROR;
        break;
    case SOT_LIKE:
    case SOT_NLIKE:
    case SOT_ILIKE:
    case SOT_NILIKE:

        if (!IS_BS_OR_NBS(elem[1]->u.value.valuetype))
            return RET_ERROR;
        if ((IS_NBS(elem[0]->u.value.valuetype)
                        && !IS_NBS(elem[1]->u.value.valuetype))
                || (!IS_NBS(elem[0]->u.value.valuetype)
                        && IS_NBS(elem[1]->u.value.valuetype))
                || (IS_BYTE(elem[0]->u.value.valuetype)
                        || IS_BYTE(elem[1]->u.value.valuetype)))
        {
            return RET_ERROR;
        }
        break;
    case SOT_PLUS:
    case SOT_MINUS:
    case SOT_TIMES:
    case SOT_DIVIDE:
        if (!IS_NUMERIC(elem[0]->u.value.valuetype) ||
                !IS_NUMERIC(elem[1]->u.value.valuetype))
            return RET_ERROR;
        break;
    case SOT_EXPONENT:
        if (!IS_NUMERIC(elem[0]->u.value.valuetype))
            return RET_ERROR;
        if (!IS_INTEGER(elem[1]->u.value.valuetype))
            return RET_ERROR;
        break;
    case SOT_MODULA:
        break;
    case SOT_MERGE:
        if (!IS_BS_OR_NBS(elem[0]->u.value.valuetype))
            return RET_ERROR;
        if (!IS_BS_OR_NBS(elem[1]->u.value.valuetype))
            return RET_ERROR;
        if ((IS_NBS(elem[0]->u.value.valuetype)
                        && !IS_NBS(elem[1]->u.value.valuetype))
                || (IS_NBS(elem[1]->u.value.valuetype)
                        && !IS_NBS(elem[0]->u.value.valuetype)))
            return RET_ERROR;
        break;
    case SOT_LE:
    case SOT_LT:
    case SOT_GE:
    case SOT_GT:
    case SOT_EQ:
    case SOT_NE:
        if (IS_NUMERIC(elem[0]->u.value.valuetype))
        {
            if (IS_STRING(elem[1]->u.value.valuetype)
                    || IS_NBS(elem[1]->u.value.valuetype))
                return RET_ERROR;
        }
        else if (IS_STRING(elem[0]->u.value.valuetype)
                || IS_NBS(elem[0]->u.value.valuetype))
        {
            if (IS_NUMERIC(elem[1]->u.value.valuetype))
                return RET_ERROR;
        }
        else
            return RET_ERROR;
        break;
    case SOT_ISNULL:
    case SOT_ISNOTNULL:
        return RET_SUCCESS;
    case SOT_IN:
    case SOT_NIN:
        is_numeric = 0;
        is_string = 0;
        for (i = 0; i < op->argcnt; i++)
        {
            if (IS_NUMERIC(elem[i]->u.value.valuetype))
                is_numeric = 1;
            else if (IS_STRING(elem[i]->u.value.valuetype)
                    || IS_NBS(elem[i]->u.value.valuetype))
                is_string = 1;
            else
                return RET_ERROR;
        }
        if (is_numeric && is_string)
            return RET_ERROR;
        return RET_SUCCESS;
    default:
        return RET_ERROR;
    }
    return RET_SUCCESS;
}

/*****************************************************************************/
//! calc_and

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param left(in)      :
 * \param right(in)     : 
 * \param res(in/out)   :
 * \param is_init(in)   : MDB_TRUE(결과 type 계산)/MDB_FALSE(실제 계산)
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘 및 기타 설명
 *  - COLLATION TYPE 추가함
 *  - is_init 변수 추가
 *****************************************************************************/
static int
calc_and(T_VALUEDESC * left, T_VALUEDESC * right, T_VALUEDESC * res,
        MDB_INT32 is_init)
{
    res->valuetype = DT_BOOL;
    res->attrdesc.collation = MDB_COL_NUMERIC;

    if (is_init)
        return RET_SUCCESS;

    if (left->u.bl == 1 && right->u.bl == 1)
        res->u.bl = 1;
    else
        res->u.bl = 0;

    return RET_SUCCESS;
}

/*****************************************************************************/
//! calc_or

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param left(in)      :
 * \param right(in)     : 
 * \param res(in/out)   :
 * \param is_init(in)   : MDB_TRUE(결과 type 계산)/MDB_FALSE(실제 계산)
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘 및 기타 설명
 *  - COLLATION TYPE 추가함
 *  - is_init 변수 추가
 *****************************************************************************/
static int
calc_or(T_VALUEDESC * left, T_VALUEDESC * right, T_VALUEDESC * res,
        MDB_INT32 is_init)
{
    res->valuetype = DT_BOOL;

    res->attrdesc.collation = MDB_COL_NUMERIC;

    if (is_init)
        return RET_SUCCESS;

    if (left->u.bl == 1 || right->u.bl == 1)
        res->u.bl = 1;
    else
        res->u.bl = 0;

    return RET_SUCCESS;
}

/*****************************************************************************/
//! calc_not

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param right(in)     : 
 * \param res(in/out)   :
 * \param is_init(in)   : MDB_TRUE(결과 type 계산)/MDB_FALSE(실제 계산)
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘 및 기타 설명
 *  - COLLATION TYPE 추가함
 *  - is_init 변수 추가
 *****************************************************************************/
static int
calc_not(T_VALUEDESC * right, T_VALUEDESC * res, MDB_INT32 is_init)
{
    res->valuetype = DT_BOOL;
    res->attrdesc.collation = MDB_COL_NUMERIC;

    if (is_init)
        return RET_SUCCESS;

    if (right->u.bl == 0)
        res->u.bl = 1;
    else
        res->u.bl = 0;

    return RET_SUCCESS;
}

/*****************************************************************************/
//! calc_like

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param left(in)      :
 * \param right(in)     : 
 * \param res(in/out)   :
 * \param is_init(in)   : MDB_TRUE(결과 type 계산)/MDB_FALSE(실제 계산)
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - byte/varbyte의 경우 에러 리턴
 *  - COLLATION TYPE 추가함
 *  - is_init 변수 추가
 *****************************************************************************/
int
calc_like(T_VALUEDESC * left, T_VALUEDESC * right, void **preProcessedlikeStr,
        T_VALUEDESC * res, int mode, MDB_INT32 is_init)
{
    char *lptr, *rptr = NULL;
    int free_lptr = 0;
    int free_rptr = 0;
    void *newLike = NULL;
    int ret;
    int len4like = 0;
    MDB_COL_TYPE collation = left->attrdesc.collation;

    if (left->valueclass == SVC_VARIABLE || right->valueclass == SVC_VARIABLE)
    {
        res->valueclass = SVC_VARIABLE;
    }

    res->valuetype = DT_BOOL;
    res->attrdesc.collation = MDB_COL_NUMERIC;

    if (is_init)
        return RET_SUCCESS;

    if (left->is_null || right->is_null)
    {
        res->u.bl = 0;
        return 1;
    }

    lptr = left->u.ptr;
    rptr = right->u.ptr;

    len4like = left->attrdesc.len;

    if (IS_NBS(left->valuetype))
    {
        if (mode == MDB_ILIKE)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                    SQL_E_INVALIDEXPRESSION, 0);
            return SQL_E_INVALIDEXPRESSION;
        }
    }
    else if (!IS_BS(left->valuetype))
    {
        lptr = (char *) PMEM_XCALLOC(32);
        if (lptr == NULL)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
            return SQL_E_OUTOFMEMORY;
        }

        print_value_as_string(left, lptr, 32);

        free_lptr = 1;
        collation = DB_Params.default_col_char;
        len4like = 32;
    }

    if (right->valueclass == SVC_VARIABLE)
        PMEM_FREENUL(*preProcessedlikeStr);

    if (preProcessedlikeStr == NULL || *preProcessedlikeStr == NULL)
    {
        ret = dbi_prepare_like(rptr, left->valuetype, mode,
                (collation == MDB_COL_CHAR_REVERSE ? len4like : -1), &newLike);
        if (ret < 0)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
            return ret;
        }
    }
    else
    {
        newLike = *preProcessedlikeStr;
    }

    if (dbi_like_compare(lptr, left->valuetype, FIXED_VARIABLE,
                    newLike, mode, collation) == 0)
        res->u.bl = 1;
    else
        res->u.bl = 0;

    if (preProcessedlikeStr == NULL)
        PMEM_FREENUL(newLike);
    else if (*preProcessedlikeStr == NULL)
        *preProcessedlikeStr = newLike;

    if (free_lptr)
        PMEM_FREE(lptr);

    if (free_rptr)
        PMEM_FREE(rptr);

    return RET_SUCCESS;
}

/*****************************************************************************/
//! calc_isnull

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param right(in)     :
 * \param res(in/out)   :
 * \param is_init(in)   : MDB_TRUE(결과 type 계산)/MDB_FALSE(실제 계산)
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘 및 기타 설명
 *  - COLLATION TYPE 추가함
 *  - is_init 변수 추가
 *****************************************************************************/
static int
calc_isnull(T_VALUEDESC * right, T_VALUEDESC * res, MDB_INT32 is_init)
{
    res->valuetype = DT_BOOL;
    res->attrdesc.collation = MDB_COL_NUMERIC;

    if (is_init)
        return RET_SUCCESS;

    if (right->is_null)
        res->u.bl = 1;
    else
        res->u.bl = 0;

    return RET_SUCCESS;
}

/*****************************************************************************/
//! calc_isnotnull

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param right(in)    :
 * \param res(in/out)   :
 * \param is_init(in)   : MDB_TRUE(결과 type 계산)/MDB_FALSE(실제 계산)
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘 및 기타 설명
 *  - COLLATION TYPE 추가함
 *  - is_init 변수 추가
 *****************************************************************************/
static int
calc_isnotnull(T_VALUEDESC * right, T_VALUEDESC * res, MDB_INT32 is_init)
{
    res->valuetype = DT_BOOL;
    res->attrdesc.collation = MDB_COL_NUMERIC;

    if (is_init)
        return RET_SUCCESS;

    if (right->is_null)
        res->u.bl = 0;
    else
        res->u.bl = 1;

    return RET_SUCCESS;
}


/*****************************************************************************/
//! calc_plus 

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param left(in)      :
 * \param right(in)     : 
 * \param res(in/out)   :
 * \param is_init(in)   : MDB_TRUE(결과 type 계산)/MDB_FALSE(실제 계산)
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *  - DECIMAL TYPE인 경우, 정보를 SET 해줘야 한다
 *  - COLLATION TYPE 추가함
 *  - is_init 변수 추가
 *****************************************************************************/
static int
calc_plus(T_VALUEDESC * left, T_VALUEDESC * right, T_VALUEDESC * res,
        MDB_INT32 is_init)
{
    if (left->valueclass == SVC_VARIABLE || right->valueclass == SVC_VARIABLE)
    {
        res->valueclass = SVC_VARIABLE;
    }

    res->attrdesc.collation = MDB_COL_NUMERIC;

    if (left->valuetype > right->valuetype)
    {
        res->valuetype = left->valuetype;
        res->attrdesc.len = left->attrdesc.len;
        if (res->valuetype == DT_DECIMAL)
            res->attrdesc.dec = left->attrdesc.dec;
    }
    else
    {
        res->valuetype = right->valuetype;
        res->attrdesc.len = right->attrdesc.len;

        if (res->valuetype == DT_DECIMAL)
        {
            res->attrdesc.dec = right->attrdesc.dec;

            if (left->attrdesc.type == DT_DECIMAL)
            {
                if (left->attrdesc.len > right->attrdesc.len)
                    res->attrdesc.len = left->attrdesc.len;
                if (left->attrdesc.dec > right->attrdesc.dec)
                    res->attrdesc.dec = left->attrdesc.dec;
            }
        }
    }

    if (is_init && !(res->valueclass == SVC_CONSTANT))
        return RET_SUCCESS;

    if (left->is_null || right->is_null)
    {
        res->is_null = 1;
        return RET_SUCCESS;
    }

    if (IS_INTEGER(res->valuetype))
    {
        if (left->valuetype == right->valuetype)
        {
            res->u.i64 = left->u.i64 + right->u.i64;
        }
        else
        {
            bigint l_val, r_val;

            GET_INT_VALUE(l_val, bigint, left);
            GET_INT_VALUE(r_val, bigint, right);
            PUT_INT_VALUE(res, l_val + r_val);
        }
    }
    else
    {
        double l_val, r_val;

        GET_FLOAT_VALUE(l_val, double, left);
        GET_FLOAT_VALUE(r_val, double, right);

        PUT_FLOAT_VALUE(res, l_val + r_val);
    }

    return RET_SUCCESS;
}


/*****************************************************************************/
//! calc_minus

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param left(in)      :
 * \param right(in)     : 
 * \param res(in/out)   :
 * \param is_init(in)   : MDB_TRUE(결과 type 계산)/MDB_FALSE(실제 계산)
 ************************************
 * \return  return_value
 ************************************
 *  - DECIMAL TYPE인 경우, 정보를 SET 해줘야 한다
 *  - COLLATION TYPE 추가함
 *  - is_init 변수 추가
 *****************************************************************************/
static int
calc_minus(T_VALUEDESC * left, T_VALUEDESC * right, T_VALUEDESC * res,
        MDB_INT32 is_init)
{
    if (left->valueclass == SVC_VARIABLE || right->valueclass == SVC_VARIABLE)
    {
        res->valueclass = SVC_VARIABLE;
    }

    if (left->valuetype > right->valuetype)
    {
        res->valuetype = left->valuetype;
        res->attrdesc.len = left->attrdesc.len;
        res->attrdesc.collation = left->attrdesc.collation;

        if (res->valuetype == DT_DECIMAL)
        {
            res->attrdesc.dec = left->attrdesc.dec;
        }
    }
    else
    {
        res->valuetype = right->valuetype;
        res->attrdesc.len = right->attrdesc.len;
        res->attrdesc.collation = right->attrdesc.collation;

        if (res->valuetype == DT_DECIMAL)
        {
            res->attrdesc.dec = right->attrdesc.dec;

            if (left->attrdesc.type == DT_DECIMAL)
            {
                if (left->attrdesc.len > right->attrdesc.len)
                    res->attrdesc.len = left->attrdesc.len;
                if (left->attrdesc.dec > right->attrdesc.dec)
                    res->attrdesc.dec = left->attrdesc.dec;
            }
        }
    }

    if (is_init && !(res->valueclass == SVC_CONSTANT))
        return RET_SUCCESS;

    if (left->is_null || right->is_null)
    {
        res->is_null = 1;
        return RET_SUCCESS;
    }

    if (is_init)
        return RET_SUCCESS;

    if (IS_INTEGER(res->valuetype))
    {
        if (left->valuetype == right->valuetype)
        {
            res->u.i64 = left->u.i64 - right->u.i64;
        }
        else
        {
            bigint l_val, r_val;

            GET_INT_VALUE(l_val, bigint, left);
            GET_INT_VALUE(r_val, bigint, right);
            PUT_INT_VALUE(res, l_val - r_val);
        }
    }
    else
    {
        double l_val, r_val;

        GET_FLOAT_VALUE(l_val, double, left);
        GET_FLOAT_VALUE(r_val, double, right);

        PUT_FLOAT_VALUE(res, l_val - r_val);
    }

    return RET_SUCCESS;
}


/*****************************************************************************/
//! calc_multiply

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param left(in)      :
 * \param right(in)     : 
 * \param res(in/out)   :
 * \param is_init(in)   : MDB_TRUE(결과 type 계산)/MDB_FALSE(실제 계산)
 ************************************
 * \return  return_value
 ************************************
 *  - DECIMAL TYPE인 경우, 정보를 SET 해줘야 한다
 *  - COLLATION TYPE 추가함
 *  - is_init 변수 추가
 *****************************************************************************/
static int
calc_multiply(T_VALUEDESC * left, T_VALUEDESC * right,
        T_VALUEDESC * res, MDB_INT32 is_init)
{
    if (left->valueclass == SVC_VARIABLE || right->valueclass == SVC_VARIABLE)
    {
        res->valueclass = SVC_VARIABLE;
    }

    if (left->valuetype > right->valuetype)
    {
        res->valuetype = left->valuetype;
        res->attrdesc.len = left->attrdesc.len;
        res->attrdesc.collation = left->attrdesc.collation;

        if (res->valuetype == DT_DECIMAL)
        {
            res->attrdesc.dec = left->attrdesc.dec;
        }
    }
    else
    {
        res->valuetype = right->valuetype;
        res->attrdesc.len = right->attrdesc.len;
        res->attrdesc.collation = right->attrdesc.collation;

        if (res->valuetype == DT_DECIMAL)
        {
            res->attrdesc.dec = right->attrdesc.dec;

            if (left->valuetype == DT_DECIMAL)
            {
                if (left->attrdesc.len > right->attrdesc.len)
                    res->attrdesc.len = left->attrdesc.len;
                if (left->attrdesc.dec > right->attrdesc.dec)
                    res->attrdesc.dec = left->attrdesc.dec;
            }
        }
    }

    /* left, right의 valueclass 중 하나라도 SVC_VARIABLE 이라면
     * res->valueclass는 SVC_VARIABLE 이기 때문에
     * 아래와 같이 검사한다.
     *
     * left, right 가 모두 SVC_CONSTANT 라면,
     * is_init(prepare 에서 호출)이더라도 결과를 계산한다.
     */
    if (is_init && !(res->valueclass == SVC_CONSTANT))
        return RET_SUCCESS;

    if (left->is_null || right->is_null)
    {
        res->is_null = 1;
        return RET_SUCCESS;
    }

    if (IS_INTEGER(res->valuetype))
    {
        if (left->valuetype == right->valuetype)
        {
            res->u.i64 = left->u.i64 * right->u.i64;
        }
        else
        {
            bigint l_val, r_val;

            GET_INT_VALUE(l_val, bigint, left);
            GET_INT_VALUE(r_val, bigint, right);
            PUT_INT_VALUE(res, l_val * r_val);
        }
    }
    else
    {
        double l_val, r_val;

        GET_FLOAT_VALUE(l_val, double, left);
        GET_FLOAT_VALUE(r_val, double, right);

        PUT_FLOAT_VALUE(res, l_val * r_val);
    }
    return RET_SUCCESS;
}


/*****************************************************************************/
//! calc_divide 

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param left(in)      :
 * \param right(in)     : 
 * \param res(in/out)   :
 * \param is_init(in)   : MDB_TRUE(결과 type 계산)/MDB_FALSE(실제 계산)
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - DECIMAL TYPE인 경우, 정보를 SET 해줘야 한다
 *  - COLLATION TYPE 추가함
 *  - is_init 변수 추가
 *****************************************************************************/
static int
calc_divide(T_VALUEDESC * left, T_VALUEDESC * right, T_VALUEDESC * res,
        MDB_INT32 is_init)
{
    if (left->valueclass == SVC_VARIABLE || right->valueclass == SVC_VARIABLE)
    {
        res->valueclass = SVC_VARIABLE;
    }

    if (left->valuetype > right->valuetype)
    {
        res->valuetype = left->valuetype;
        if (res->valuetype == DT_DECIMAL)
        {
            res->attrdesc.len = left->attrdesc.len;
            res->attrdesc.dec = left->attrdesc.dec;
        }
        res->attrdesc.collation = left->attrdesc.collation;
    }
    else
    {
        res->valuetype = right->valuetype;
        if (res->valuetype == DT_DECIMAL)
        {
            res->attrdesc.len = right->attrdesc.len;
            res->attrdesc.dec = right->attrdesc.dec;
        }
        res->attrdesc.collation = right->attrdesc.collation;
    }

    if (is_init)
        return RET_SUCCESS;

    if (left->is_null || right->is_null || right->u.i64 == 0)
    {
        res->is_null = 1;
        return RET_SUCCESS;
    }
    /* 위의 DIVIDE_ZERO 코드로 아래의 내용(switch)은 필요없다.
     * 확인 후 삭제
     */

    if (IS_INTEGER(res->valuetype))
    {
        if (left->valuetype == right->valuetype)
        {
            res->u.i64 = left->u.i64 / right->u.i64;
        }
        else
        {
            bigint l_val, r_val;

            GET_INT_VALUE(l_val, bigint, left);
            GET_INT_VALUE(r_val, bigint, right);
            PUT_INT_VALUE(res, l_val / r_val);
        }
    }
    else
    {
        double l_val, r_val;

        GET_FLOAT_VALUE(l_val, double, left);
        GET_FLOAT_VALUE(r_val, double, right);

        PUT_FLOAT_VALUE(res, l_val / r_val);
    }

    return RET_SUCCESS;
}

/*****************************************************************************/
//! calc_exponent 

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param left(in)      :
 * \param right(in)     : 
 * \param res(in/out)   :
 * \param is_init(in)   : MDB_TRUE(결과 type 계산)/MDB_FALSE(실제 계산)
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *  - COLLATION TYPE 추가함
 *  - is_init 변수 추가
 *****************************************************************************/
static int
calc_exponent(T_VALUEDESC * left, T_VALUEDESC * right,
        T_VALUEDESC * res, MDB_INT32 is_init)
{
    if (left->valueclass == SVC_VARIABLE || right->valueclass == SVC_VARIABLE)
    {
        res->valueclass = SVC_VARIABLE;
    }

    res->valuetype = DT_DOUBLE;
    res->attrdesc.len = SizeOfType[DT_DOUBLE];

    res->attrdesc.collation = MDB_COL_NUMERIC;

    if (is_init)
        return RET_SUCCESS;

    if (left->is_null || right->is_null)
    {
        res->is_null = 1;
        return RET_SUCCESS;
    }

    if (IS_INTEGER(res->valuetype))
    {
        bigint l_val, r_val;

        GET_INT_VALUE(l_val, bigint, left);
        GET_INT_VALUE(r_val, bigint, right);
        PUT_INT_VALUE(res, sc_pow((double) l_val, (double) r_val));
    }
    else
    {
        double l_val, r_val;

        GET_FLOAT_VALUE(l_val, double, left);
        GET_FLOAT_VALUE(r_val, double, right);

        PUT_FLOAT_VALUE(res, sc_pow(l_val, r_val));
    }

    return RET_SUCCESS;
}

/*****************************************************************************/
//! calc_modula

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param left(in)      :
 * \param right(in)     : 
 * \param res(in/out)   :
 * \param is_init(in)   : MDB_TRUE(결과 type 계산)/MDB_FALSE(실제 계산)
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *  - COLLATION TYPE 추가함
 *  - is_init 변수 추가
 *****************************************************************************/
static int
calc_modula(T_VALUEDESC * left, T_VALUEDESC * right, T_VALUEDESC * res,
        MDB_INT32 is_init)
{
    if (left->valueclass == SVC_VARIABLE || right->valueclass == SVC_VARIABLE)
    {
        res->valueclass = SVC_VARIABLE;
    }

    if (left->valuetype > right->valuetype)
    {
        res->valuetype = left->valuetype;

        res->attrdesc.len = left->attrdesc.len;
        res->attrdesc.collation = left->attrdesc.collation;

        if (res->valuetype == DT_DECIMAL)
        {
            res->attrdesc.dec = left->attrdesc.dec;
        }
    }
    else
    {
        res->valuetype = right->valuetype;
        res->attrdesc.len = right->attrdesc.len;
        res->attrdesc.collation = right->attrdesc.collation;

        if (res->valuetype == DT_DECIMAL)
        {
            res->attrdesc.dec = right->attrdesc.dec;

            if (left->valuetype == DT_DECIMAL)
            {
                if (left->attrdesc.len > right->attrdesc.len)
                    res->attrdesc.len = left->attrdesc.len;
                if (left->attrdesc.dec > right->attrdesc.dec)
                    res->attrdesc.dec = left->attrdesc.dec;
            }
        }
    }

    if (is_init && !(res->valueclass == SVC_CONSTANT))
    {
        return RET_SUCCESS;
    }

    if (left->is_null || right->is_null)
    {
        res->is_null = 1;
        return RET_SUCCESS;
    }

    if ((res->valuetype > DT_BOOL) && (res->valuetype < DT_CHAR))
    {
        bigint l_val, r_val;

        GET_INT_VALUE(l_val, bigint, left);
        GET_INT_VALUE(r_val, bigint, right);

        if (r_val == 0)
        {
            res->is_null = 1;
            return RET_SUCCESS;
        }

        if (IS_INTEGER(res->valuetype))
        {
            PUT_INT_VALUE(res, l_val % r_val);
        }
        else
        {
            PUT_FLOAT_VALUE(res, l_val % r_val);
        }
    }
    else
    {
        return SQL_E_INCONSISTENTTYPE;
    }

    return RET_SUCCESS;
}

/*****************************************************************************/
//! calc_merge 

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param left(in)    :
 * \param right(in)    : 
 * \param res(out)    :
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - LEFT 혹은 RIGHT가 NULL인 경우, NULL이 아닌 값을 출력하도록 수정
 *  - res의 max length를 주지 않음
 *  - 2번 수행 안되도록 수정
 *  - collation 추가
 *      
 *****************************************************************************/
static int
calc_merge(T_VALUEDESC * left, T_VALUEDESC * right, T_VALUEDESC * res,
        MDB_INT32 is_init)
{
    unsigned int max_length = 0;
    int len = 0, pos = 0;

    if (left->valueclass == SVC_VARIABLE || right->valueclass == SVC_VARIABLE)
        res->valueclass = SVC_VARIABLE;
    else
        res->valueclass = SVC_CONSTANT;

    if (left->valuetype > right->valuetype)
    {
        res->valuetype = left->valuetype;
    }
    else
    {
        res->valuetype = right->valuetype;
    }

    res->attrdesc.collation = left->attrdesc.collation;

    res->attrdesc.len = left->attrdesc.len + right->attrdesc.len;

    if (is_init)
    {
        res->is_null = 1;
        return RET_SUCCESS;
    }

    if (left->is_null || right->is_null)
    {
        res->is_null = 1;
        return RET_SUCCESS;
    }
    else
        res->is_null = 0;

    if (left->is_null)
    {
        len += 0;
        max_length += 0;
    }
    else
    {
        len += strlen_func[left->valuetype] (left->u.ptr);
        max_length += left->attrdesc.len;
    }

    if (right->is_null)
    {
        len += 0;
        max_length += 0;
    }
    else
    {
        len += strlen_func[right->valuetype] (right->u.ptr);
        max_length += right->attrdesc.len;
    }

    if (IS_NBS(left->valuetype))
    {
        sql_value_ptrXcalloc(res, len + 1);
        res->attrdesc.len = max_length;
        if (left->is_null)
        {
            sc_wmemcpy(res->u.Wptr, right->u.Wptr, sc_wcslen(right->u.Wptr));
        }
        else if (right->is_null)
        {
            sc_wmemcpy(res->u.Wptr, left->u.Wptr, sc_wcslen(left->u.Wptr));
        }
        else
        {
            sc_wmemcpy(res->u.Wptr, left->u.Wptr, sc_wcslen(left->u.Wptr));
            sc_wmemcpy(res->u.Wptr + sc_wcslen(left->u.Wptr),
                    right->u.Wptr, sc_wcslen(right->u.Wptr));
        }
        res->value_len = sc_wcslen(res->u.Wptr);
    }
    else
    {
        sql_value_ptrXcalloc(res, len + 1);
        res->attrdesc.len = max_length;

        if (left->is_null)
            pos = sc_snprintf(res->u.ptr, len + 1, "%s", right->u.ptr);
        else if (right->is_null)
            pos = sc_snprintf(res->u.ptr, len + 1, "%s", left->u.ptr);
        else
        {
            pos = sc_snprintf(res->u.ptr, len + 1, "%s", left->u.ptr);
            pos += sc_snprintf(res->u.ptr + pos, len + 1 - pos, "%s",
                    right->u.ptr);
        }
        res->value_len = pos;
    }
    return RET_SUCCESS;
}

/*****************************************************************************/
//! calc_less

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param left(in)      :
 * \param right(in)     :
 * \param res(in/out)   :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘 및 기타 설명
 *****************************************************************************/
static int
calc_less(T_VALUEDESC * left, T_VALUEDESC * right, T_VALUEDESC * res)
{
    bigint cmp_result = 0;

    if (left->valueclass == SVC_VARIABLE || right->valueclass == SVC_VARIABLE)
    {
        res->valueclass = SVC_VARIABLE;
    }

    res->valuetype = DT_BOOL;
    res->attrdesc.collation = MDB_COL_NUMERIC;

    if (left->is_null || right->is_null)
    {
        res->u.bl = 0;
        return RET_SUCCESS;
    }

    if (sql_compare_values(left, right, &cmp_result) == RET_ERROR)
        return RET_ERROR;

    if (cmp_result < 0)
        res->u.bl = 1;
    else
        res->u.bl = 0;

    return RET_SUCCESS;
}

/*****************************************************************************/
//! calc_lessnequal

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param left(in)      :
 * \param right(in)     :
 * \param res(in/out)   :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘 및 기타 설명
 *****************************************************************************/
static int
calc_lessnequal(T_VALUEDESC * left, T_VALUEDESC * right, T_VALUEDESC * res)
{
    bigint cmp_result = 0;

    if (left->valueclass == SVC_VARIABLE || right->valueclass == SVC_VARIABLE)
    {
        res->valueclass = SVC_VARIABLE;
    }

    res->valuetype = DT_BOOL;
    res->attrdesc.collation = MDB_COL_NUMERIC;

    if (left->is_null || right->is_null)
    {
        res->u.bl = 0;
        return RET_SUCCESS;
    }

    if (sql_compare_values(left, right, &cmp_result) == RET_ERROR)
        return RET_ERROR;

    if (cmp_result <= 0)
        res->u.bl = 1;
    else
        res->u.bl = 0;

    return RET_SUCCESS;
}

/*****************************************************************************/
//! calc_greater

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param left(in)      :
 * \param right(in)     :
 * \param res(in/out)   :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘 및 기타 설명
 *****************************************************************************/
static int
calc_greater(T_VALUEDESC * left, T_VALUEDESC * right, T_VALUEDESC * res)
{
    bigint cmp_result = 0;

    if (left->valueclass == SVC_VARIABLE || right->valueclass == SVC_VARIABLE)
    {
        res->valueclass = SVC_VARIABLE;
    }

    res->valuetype = DT_BOOL;
    res->attrdesc.collation = MDB_COL_NUMERIC;

    if (left->is_null || right->is_null)
    {
        res->u.bl = 0;
        return RET_SUCCESS;
    }

    if (sql_compare_values(left, right, &cmp_result) == RET_ERROR)
        return RET_ERROR;

    if (cmp_result > 0)
        res->u.bl = 1;
    else
        res->u.bl = 0;

    return RET_SUCCESS;
}


/*****************************************************************************/
//! calc_greaternequal 

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param left(in)      :
 * \param right(in)     :
 * \param res(in/out)   :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘 및 기타 설명
 *****************************************************************************/
static int
calc_greaternequal(T_VALUEDESC * left, T_VALUEDESC * right, T_VALUEDESC * res)
{
    bigint cmp_result = 0;

    if (left->valueclass == SVC_VARIABLE || right->valueclass == SVC_VARIABLE)
    {
        res->valueclass = SVC_VARIABLE;
    }

    res->valuetype = DT_BOOL;
    res->attrdesc.collation = MDB_COL_NUMERIC;

    if (left->is_null || right->is_null)
    {
        res->u.bl = 0;
        return RET_SUCCESS;
    }

    if (sql_compare_values(left, right, &cmp_result) == RET_ERROR)
        return RET_ERROR;

    if (cmp_result >= 0)
        res->u.bl = 1;
    else
        res->u.bl = 0;

    return RET_SUCCESS;
}


/*****************************************************************************/
//! calc_equal

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param left(in)      :
 * \param right(in)     :
 * \param res(in/out)   :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘 및 기타 설명
 *****************************************************************************/
int
calc_equal(T_VALUEDESC * left, T_VALUEDESC * right, T_VALUEDESC * res)
{
    bigint cmp_result = 0;

    if (left->valueclass == SVC_VARIABLE || right->valueclass == SVC_VARIABLE)
    {
        res->valueclass = SVC_VARIABLE;
    }

    res->valuetype = DT_BOOL;
    res->attrdesc.collation = MDB_COL_NUMERIC;

    if ((left->valueclass == SVC_VARIABLE && left->is_null &&
                    right->valueclass == SVC_CONSTANT && right->is_null &&
                    right->param_idx > 0))
    {
        res->u.bl = 1;
        return RET_SUCCESS;
    }
    else if (left->is_null || right->is_null)
    {
        res->u.bl = 0;
        return RET_SUCCESS;
    }

    if (sql_compare_values(left, right, &cmp_result) == RET_ERROR)
        return RET_ERROR;

    if (cmp_result == 0)
        res->u.bl = 1;
    else
        res->u.bl = 0;

    return RET_SUCCESS;
}


/*****************************************************************************/
//! function_name 

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param left(in)      :
 * \param right(in)     :
 * \param res(in/out)   :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘 및 기타 설명
 *****************************************************************************/
static int
calc_notequal(T_VALUEDESC * left, T_VALUEDESC * right, T_VALUEDESC * res)
{
    bigint cmp_result = 0;

    if (left->valueclass == SVC_VARIABLE || right->valueclass == SVC_VARIABLE)
    {
        res->valueclass = SVC_VARIABLE;
    }

    res->valuetype = DT_BOOL;
    res->attrdesc.collation = MDB_COL_NUMERIC;

    if (left->is_null || right->is_null)
    {
        res->u.bl = 0;
        return RET_SUCCESS;
    }

    if (sql_compare_values(left, right, &cmp_result) == RET_ERROR)
        return RET_ERROR;

    if (cmp_result != 0)
        res->u.bl = 1;
    else
        res->u.bl = 0;

    return RET_SUCCESS;
}


/*****************************************************************************/
//! calc_in

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param num(in)       :
 * \param values(in)    : 
 * \param res(in/out)   :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
calc_in(int num, T_VALUEDESC * values, T_VALUEDESC * res)
{
    T_VALUEDESC tmp;
    int i;

    res->valuetype = DT_BOOL;
    res->attrdesc.collation = MDB_COL_NUMERIC;

    if (values[0].is_null)
    {
        res->u.bl = 0;
        return RET_SUCCESS;
    }

    res->u.bl = 0;

    if (IS_NUMERIC(values[0].valuetype))
    {
        if (IS_BS_OR_NBS(values[0].valuetype) || IS_TIMES(values[0].valuetype))
            return RET_SUCCESS;
    }
    else if (IS_BS_OR_NBS(values[0].valuetype) ||
            IS_TIMES(values[0].valuetype))
    {
        if (IS_NUMERIC(values[0].valuetype))
            return RET_SUCCESS;
    }

    for (i = 1; i < num; i++)
    {

        if (calc_equal(&values[0], &values[i], &tmp) < 0)
            return RET_ERROR;
        if (tmp.u.bl)
        {
            res->u.bl = 1;
            break;
        }
    }

    return RET_SUCCESS;
}

extern void set_correl_attr_values(CORRELATED_ATTR *);
extern int bin_result_record_len(T_QUERYRESULT *, int);
extern int SQL_fetch(int *, T_SELECT *, T_RESULT_INFO *);
extern int copy_one_binary_column(char *, T_RESULTCOLUMN *, int);
extern int dbi_Relation_Drop(int, char *);
extern int get_result_column_offset(T_QUERYRESULT *, int, int);

// TODO : get_recordlen()등의 함수와 모아서 ...
extern int one_binary_column_length(T_RESULTCOLUMN *, int);
extern int close_all_open_tables(int, T_SELECT *);
extern int make_work_table(int *, T_SELECT *);

/*****************************************************************************/
//! calc_subexpr 

/*! \breif  subquery가 포함된 연산(op)을 args를 인자로 해서 수행하고, 결과를 res에 저장한다.
 ************************************
 * \param handle(in) :
 * \param op(in) : 
 * \param res(in/out) :
 * \param args(in) :
 ************************************
 * \return  RET_ERROR/RET_SUCCESS
 ************************************
 * \note 
 *  - args이 (T_POSTFIXELEM*)에서 (T_POSTFIXELEM**)로 변화
 *      이 경우에는 새로 메모리를 할당해주어야 한다...
 *      (parsing시 할당된 메모리를 건들면 안됨)
 *
 *****************************************************************************/
int
calc_subqexpr(int handle, T_OPDESC * op, T_POSTFIXELEM * res,
        T_POSTFIXELEM ** args)
{
    T_SELECT *select;
    T_SELECT *fetch_select;
    T_QUERYRESULT *qres = NULL;
    T_QUERYDESC *qdesc;
    iSQL_ROWS *row = NULL;
    T_POSTFIXELEM arg2_org;
    T_POSTFIXELEM *arg2, *t_args0;
    char *target, *null_ptr;
    int *rec_len, bi_rec_len = 0, col_len;
    int opi;
    int ret = RET_SUCCESS, ret_fetch = RET_SUCCESS, row_num = 0;
    int subq_inited = 0;
    int column_offset = 0;
    int column_length = 0;

    arg2 = &arg2_org;
    sc_memset(arg2, 0x00, sizeof(T_POSTFIXELEM));

    res->u.value.u.bl = 0;      // false
    if (op->type == SOT_ALL_LT || op->type == SOT_ALL_LE ||
            op->type == SOT_ALL_GT || op->type == SOT_ALL_GE ||
            op->type == SOT_ALL_EQ || op->type == SOT_ALL_NE ||
            op->type == SOT_NEXISTS || op->type == SOT_NIN)
    {
        res->u.value.u.bl = 1;  // true
    }

    opi = (op->type == SOT_EXISTS || op->type == SOT_NEXISTS) ? 0 : 1;
    if (args[opi]->elemtype != SPT_SUBQUERY || args[opi]->u.subq == NULL)
    {
        return SQL_E_INCONSISTENTTYPE;
    }

    select = args[opi]->u.subq;
    qdesc = &select->planquerydesc.querydesc;

#ifdef MDB_DEBUG
    sc_assert(select->kindofwTable & iTABLE_SUBWHERE, __FILE__, __LINE__);
#endif

    res->elemtype = SPT_VALUE;
    res->u.value.valueclass = SVC_CONSTANT;
    res->u.value.valuetype = DT_BOOL;

    res->u.value.call_free = 0;
    res->u.value.is_null = 0;
    res->u.value.attrdesc.len = 0;

    arg2_org = *(args[0]);
    arg2->u.value.call_free = 0;

    // 1. fetch a tuple and test it.
    while (1)
    {
        // already executed uncorrelated subquery
        if ((select->kindofwTable & iTABLE_CORRELATED) == 0 &&
                select->tstatus == TSS_EXECUTED)
        {
            row = (row == NULL) ? select->subq_result_rows : row->next;
            // row는 재 사용되고 링크도 유지된다. subq.rows를 확인해서 유효한지
            // 확인한다.
            if (row_num++ == select->rows)
            {
                row = NULL;
            }

            // 마지막 레코드 or error
            if (row == NULL)
            {
                select->rows = row_num;
                goto RETURN_LABEL;
            }

            if (!column_offset)
            {
                if (qdesc->setlist.len > 0)
                {
                    qres = &qdesc->setlist.list[0]->u.subq->queryresult;
                }
                else
                    qres = &select->queryresult;
                column_offset = get_result_column_offset(qres, 1, 1);
                column_length = one_binary_column_length(qres->list, 1);
            }

            target = (char *) (row->data) + column_offset;

            sql_value_ptrFree(&arg2->u.value);

            arg2->u.value.call_free = 0;
            // type casting
            arg2->u.value.valueclass = qres->list[0].value.valueclass;
            arg2->u.value.valuetype = qres->list[0].value.valuetype;

            null_ptr = (char *) row->data + sizeof(int);

            if ((*null_ptr & 0x01) == 1)
            {
                arg2->u.value.is_null = 1;
            }
            else
            {
                arg2->u.value.is_null = 0;
            }

            if (IS_BS(qres->list[0].value.valuetype))
            {
                arg2->u.value.u.ptr = target;
            }
            else if (IS_BYTE(qres->list[0].value.valuetype))
            {
                arg2->u.value.u.ptr = target;
            }
            else if (IS_NBS(qres->list[0].value.valuetype))
            {
                if (sql_value_ptrXcalloc(&arg2->u.value,
                                qres->list[0].len + 1) < 0)
                {
                    ret = RET_ERROR;
                    goto RETURN_LABEL;
                }
                sc_wmemcpy((DB_WCHAR *) arg2->u.value.u.ptr,
                        (DB_WCHAR *) target, qres->list[0].len);
            }
            else
            {
                sc_memcpy(&arg2->u.value.u.bl, target, column_length);
            }
        }
        else
        {
            // fetching a subquery result
            if (subq_inited == 0)
            {
                //  binding correlated variable
                set_correl_attr_values(qdesc->correl_attr);

                // subquery execution
                // TODO: memory leak은 없나? 확인
                // 계속해서 tmp tbl을 만드는 문제.
                if (make_work_table(&handle, select) == RET_ERROR)
                {
                    close_all_open_tables(handle, select);
                    return RET_ERROR;
                }

                if (select->tmp_sub)
                {
                    fetch_select = select->tmp_sub;
                }
                else
                {
                    fetch_select = select;
                }

                qres = &fetch_select->queryresult;

                /* query result 중 첫번째 column만 사용 두번째 부터는 불필요 */
                col_len = qres->len;
                qres->len = 1;

                bi_rec_len = bin_result_record_len(qres, 1);
                // 결과 레코드 길이

                qres->len = col_len;

                subq_inited = 1;

                select->rows = 0;
                if (select->rowsp)
                {
                    *select->rowsp = 0;
                }
            }

            // Q) rttable[]의 close_cursor()는 언제 부르나 ?

            ret = ret_fetch;

            if (ret != RET_END_USED_DIRECT_API)
            {
                ret_fetch = SQL_fetch(&handle, fetch_select, NULL);
            }

            if (ret == RET_END_USED_DIRECT_API || ret_fetch == RET_END)
            {
                if ((select->kindofwTable & iTABLE_CORRELATED) == 0)
                {
                    select->tstatus = TSS_EXECUTED;
                }
                row = NULL;
            }
            else if (ret_fetch != RET_ERROR)
            {
                if ((row = sql_xcalloc(sizeof(iSQL_ROWS), 0)) == NULL)
                {
                    ret = SQL_E_OUTOFMEMORY;
                    goto RETURN_LABEL;
                    // 적당한 error handling
                }
                row->next = select->subq_result_rows;
                if ((row->data = sql_xcalloc(bi_rec_len + 1, 0)) == NULL)
                {
                    PMEM_FREENUL(row);
                    ret = SQL_E_OUTOFMEMORY;
                    goto RETURN_LABEL;
                    // 적당한 error handling
                }
                rec_len = (int *) row->data;
                null_ptr = (char *) row->data + sizeof(int);
                /* query result 중 첫번째 column만 처리 */
                target = (char *) row->data + sizeof(int) + 1;
                *rec_len = sizeof(int) + 1;

                if (qres->list[0].value.is_null)
                {
                    col_len = 0;
                    *null_ptr |= 1;
                }
                else
                {
                    col_len =
                            copy_one_binary_column(target, &qres->list[0], 1);
                }

                (*rec_len) += col_len;

                select->subq_result_rows = row;
                ++row_num;
            }

            if (row == NULL)
            {
                select->rows = row_num;
                goto RETURN_LABEL;
            }

            sql_value_ptrFree(&arg2->u.value);
            arg2->u.value = qres->list[0].value;
            arg2->u.value.call_free = 0;
        }

        if (op->argcnt >= 2 &&
                args[0]->u.value.valuetype != arg2->u.value.valuetype &&
                op->type != SOT_LIKE && op->type != SOT_ILIKE)
        {
            ret = type_convert(&(args[0]->u.value), &(arg2->u.value));
            if (ret < 0)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
                ret = RET_ERROR;
                goto RETURN_LABEL;
            }
        }

        t_args0 = args[0];
        // 2. calc each expression.
        switch (op->type)
        {
        case SOT_SOME_LE:
            ret = calc_lessnequal(&t_args0->u.value, &arg2->u.value,
                    &res->u.value);
            if (ret < 0)
                goto RETURN_LABEL;
            if (res->u.value.u.bl)
                goto RETURN_LABEL;
            break;
        case SOT_SOME_LT:
            ret = calc_less(&t_args0->u.value, &arg2->u.value, &res->u.value);
            if (ret < 0)
                goto RETURN_LABEL;
            if (res->u.value.u.bl)
                goto RETURN_LABEL;
            break;
        case SOT_SOME_GE:
            ret = calc_greaternequal(&t_args0->u.value, &arg2->u.value,
                    &res->u.value);
            if (ret < 0)
                goto RETURN_LABEL;
            if (res->u.value.u.bl)
            {
                ret = RET_SUCCESS;
                goto RETURN_LABEL;
            }
            break;
        case SOT_SOME_GT:
            ret = calc_greater(&t_args0->u.value, &arg2->u.value,
                    &res->u.value);
            if (ret < 0)
                goto RETURN_LABEL;
            if (res->u.value.u.bl)
                goto RETURN_LABEL;
            break;
        case SOT_IN:
        case SOT_SOME_EQ:
            ret = calc_equal(&t_args0->u.value, &arg2->u.value, &res->u.value);
            if (ret < 0)
                goto RETURN_LABEL;
            if (res->u.value.u.bl)
                goto RETURN_LABEL;
            break;
        case SOT_SOME_NE:
            ret = calc_notequal(&t_args0->u.value, &arg2->u.value,
                    &res->u.value);
            if (ret < 0)
                goto RETURN_LABEL;
            if (res->u.value.u.bl)
                goto RETURN_LABEL;
            break;
        case SOT_ALL_LE:
            ret = calc_lessnequal(&t_args0->u.value, &arg2->u.value,
                    &res->u.value);
            if (ret < 0)
                goto RETURN_LABEL;
            if (!res->u.value.u.bl)
                goto RETURN_LABEL;
            break;
        case SOT_ALL_LT:
            ret = calc_less(&t_args0->u.value, &arg2->u.value, &res->u.value);
            if (ret < 0)
                goto RETURN_LABEL;
            if (!res->u.value.u.bl)
                goto RETURN_LABEL;
            break;
        case SOT_ALL_GE:
            ret = calc_greaternequal(&t_args0->u.value, &arg2->u.value,
                    &res->u.value);
            if (ret < 0)
                goto RETURN_LABEL;
            if (!res->u.value.u.bl)
                goto RETURN_LABEL;
            break;
        case SOT_ALL_GT:
            ret = calc_greater(&t_args0->u.value, &arg2->u.value,
                    &res->u.value);
            if (ret < 0)
                goto RETURN_LABEL;
            if (!res->u.value.u.bl)
                goto RETURN_LABEL;
            break;
        case SOT_ALL_EQ:
            ret = calc_equal(&t_args0->u.value, &arg2->u.value, &res->u.value);
            if (ret < 0)
                goto RETURN_LABEL;
            if (!res->u.value.u.bl)
                goto RETURN_LABEL;
            break;
        case SOT_NIN:
        case SOT_ALL_NE:
            ret = calc_notequal(&t_args0->u.value, &arg2->u.value,
                    &res->u.value);
            if (ret < 0)
                goto RETURN_LABEL;
            if (!res->u.value.u.bl)
                goto RETURN_LABEL;
            break;
        case SOT_EXISTS:
            res->u.value.u.bl = 1;
            goto RETURN_LABEL;
        case SOT_NEXISTS:
            res->u.value.u.bl = 0;
            goto RETURN_LABEL;

        case SOT_ISNULL:
        case SOT_ISNOTNULL:
            MDB_SYSLOG(("Not supported op(ISNULL | ISNOTNULL)\n"));

        default:
            ret = RET_ERROR;
            goto RETURN_LABEL;
        }
    }

  RETURN_LABEL:
    if (subq_inited)
    {
        if (select->tmp_sub)
        {
            close_all_open_tables(handle, select->tmp_sub);
            if (qdesc->setlist.len > 0)
            {
                SQL_cleanup_subselect(handle, select->tmp_sub, NULL);
                PMEM_FREENUL(select->tmp_sub);
            }
        }
        else
        {
            close_all_open_tables(handle, select);
        }

        if (select->wTable)
        {
            dbi_Relation_Drop(handle, select->wTable);
        }

        if ((select->kindofwTable & iTABLE_CORRELATED) == 0 &&
                select->tstatus != TSS_EXECUTED)
        {
            if (op->type == SOT_IN || op->type == SOT_NIN)
            {
                SQL_destroy_ResultSet(select->subq_result_rows);
                select->subq_result_rows = NULL;
            }
        }
    }

    sql_value_ptrFree(&arg2->u.value);

    return ret;
}

/*****************************************************************************/
//! calc_simpleexpr 

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param op() :
 * \param res(in/out) : 
 * \param args(in) :
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *  - args이 (T_POSTFIXELEM*)에서 (T_POSTFIXELEM**)로 변화
 *  - 초기화 되는 경우만 호출되도록 수정함
 *  - is_init이 true인 경우 parsing이므로..
 *      타입 검사를 하면 안될듯하다
 *
 *****************************************************************************/
int
calc_simpleexpr(T_OPDESC * op, T_POSTFIXELEM * res,
        T_POSTFIXELEM ** args, MDB_INT32 is_init)
{
    T_VALUEDESC *vals, *res_val;
    int i, ret;
    T_POSTFIXELEM *t_args0;

    if (!is_init && check_expression(op, args) < 0)
        return SQL_E_INCONSISTENTTYPE;

    if (op->type != SOT_ISNULL && op->type != SOT_ISNOTNULL)
    {
        for (i = 0; i < op->argcnt; i++)
        {
            if (IS_BYTE(args[i]->u.value.valuetype) ||
                    IS_BYTE(args[i]->u.value.attrdesc.type))
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                        SQL_E_INVALIDEXPRESSION, 0);
                return SQL_E_INVALIDEXPRESSION;
            }
        }
    }

    ret = RET_SUCCESS;

    res->elemtype = SPT_VALUE;
    res->u.value.valueclass = SVC_CONSTANT;
    res->u.value.valuetype = DT_NULL_TYPE;

    res_val = &res->u.value;
    res_val->call_free = res_val->is_null = res_val->attrdesc.len = 0;

    t_args0 = args[0];

    switch (op->type)
    {
    case SOT_AND:
        ret = calc_and(&t_args0->u.value, &args[1]->u.value, res_val, is_init);
        break;
    case SOT_OR:
        ret = calc_or(&t_args0->u.value, &args[1]->u.value, res_val, is_init);
        break;
    case SOT_NOT:
        ret = calc_not(&t_args0->u.value, res_val, is_init);
        break;
    case SOT_LIKE:
    case SOT_ILIKE:
        if (op->type == SOT_LIKE)
            i = LIKE;
        else
            i = MDB_ILIKE;
        ret = calc_like(&t_args0->u.value, &args[1]->u.value,
                &op->likeStr, res_val, i, is_init);
        break;
    case SOT_NLIKE:
    case SOT_NILIKE:
        if (op->type == SOT_NLIKE)
            i = LIKE;
        else
            i = MDB_ILIKE;
        ret = calc_like(&t_args0->u.value, &args[1]->u.value,
                &op->likeStr, res_val, i, is_init);
        res_val->u.bl = !res_val->u.bl;
        break;
    case SOT_ISNULL:
        ret = calc_isnull(&t_args0->u.value, res_val, is_init);
        break;
    case SOT_ISNOTNULL:
        ret = calc_isnotnull(&t_args0->u.value, res_val, is_init);
        break;
    case SOT_PLUS:
        ret = calc_plus(&t_args0->u.value,
                &args[1]->u.value, res_val, is_init);
        break;
    case SOT_MINUS:
        ret = calc_minus(&t_args0->u.value,
                &args[1]->u.value, res_val, is_init);
        break;
    case SOT_TIMES:
        ret = calc_multiply(&t_args0->u.value,
                &args[1]->u.value, res_val, is_init);
        break;
    case SOT_DIVIDE:
        ret = calc_divide(&t_args0->u.value,
                &args[1]->u.value, res_val, is_init);
        break;
    case SOT_EXPONENT:
        ret = calc_exponent(&t_args0->u.value,
                &args[1]->u.value, res_val, is_init);
        break;
    case SOT_MODULA:
        ret = calc_modula(&t_args0->u.value,
                &args[1]->u.value, res_val, is_init);
        break;

    case SOT_MERGE:
        ret = calc_merge(&t_args0->u.value,
                &args[1]->u.value, res_val, is_init);
        break;
    case SOT_LE:
        ret = calc_lessnequal(&t_args0->u.value, &args[1]->u.value, res_val);
        break;
    case SOT_LT:
        ret = calc_less(&t_args0->u.value, &args[1]->u.value, res_val);
        break;
    case SOT_GE:
        ret = calc_greaternequal(&t_args0->u.value,
                &args[1]->u.value, res_val);
        break;
    case SOT_GT:
        ret = calc_greater(&t_args0->u.value, &args[1]->u.value, res_val);
        break;
    case SOT_EQ:
        ret = calc_equal(&t_args0->u.value, &args[1]->u.value, res_val);
        break;
    case SOT_NE:
        ret = calc_notequal(&t_args0->u.value, &args[1]->u.value, res_val);
        break;
    case SOT_IN:
    case SOT_NIN:
        vals = (T_VALUEDESC *) PMEM_XCALLOC(sizeof(T_VALUEDESC) * op->argcnt);
        if (!vals)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
            return RET_ERROR;
        }
        for (i = 0; i < op->argcnt; i++)
        {
            if (op->type == SOT_NIN && args[i]->u.value.is_null)
            {
                /* result type 및 collation 설정 */
                res_val->valuetype = DT_BOOL;
                res_val->attrdesc.collation = MDB_COL_NUMERIC;
                res_val->u.bl = 0;
                PMEM_FREENUL(vals);
                return RET_SUCCESS;
            }
            vals[i] = args[i]->u.value;
        }

        ret = calc_in(op->argcnt, vals, res_val);
        PMEM_FREENUL(vals);
        if (ret < 0)
            return ret;
        if (op->type == SOT_NIN)
            res_val->u.bl = !res_val->u.bl;
        break;


    default:
        ret = RET_ERROR;
    }

    return ret;
}

extern int exec_singleton_subq(int, T_SELECT *);

/*****************************************************************************/
//! calc_expression

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param expr(in)      : 계산할 expression
 * \param val(in/out)   : 계산된 값
 * \param is_init(in)   : MDB_TRUE(결과 type 계산)/MDB_FALSE(실제 계산)
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *  - T_EXPRESSIONDESC 자료구조 변화
 *  - texpr을 내부에서 할당 받도록 수정
 *      (안에 들어가는 list(T_POSTFIXELEM)을 잘 유지(?)
 *  - BYTE/VARBYTE 지원한다
 *  - TEMP TABLE을 생성할때 COLLATION 정보가 필요하다
 *
 *****************************************************************************/
int
calc_expression(T_EXPRESSIONDESC * expr, T_VALUEDESC * val, MDB_INT32 is_init)
{
    T_EXPRESSIONDESC stack;
    T_EXPRESSIONDESC texpr;
    T_POSTFIXELEM result_org;
    T_POSTFIXELEM *node, **args = NULL, *result;
    int call_free_bk;
    int i, j, ret = RET_SUCCESS;

    // subquery
    int result_len, result_offset;
    char null_value_mask, null_bit;
    char *result_buffer;

    result = &result_org;

    sc_memset(result, 0, sizeof(T_POSTFIXELEM));

    sc_memset(val, 0, sizeof(T_VALUEDESC));

    if (expr->len == 1)
    {
        if (expr->list[0]->elemtype == SPT_AGGREGATION)
        {
            node = expr->list[0];
            if ((node->u.aggr.type == SAT_COUNT ||
                            node->u.aggr.type == SAT_DCOUNT)
                    && !node->u.aggr.argcnt)
            {
                node->u.aggr.valtype = DT_INTEGER;
                node->u.aggr.length = 4;
                node->u.aggr.decimals = 0;

                val->valueclass = SVC_CONSTANT;
                val->valuetype = DT_INTEGER;
                val->call_free = val->is_null = 0;
                val->attrdesc.len = 4;
                val->attrdesc.dec = 0;
                val->attrdesc.collation = MDB_COL_NUMERIC;
            }
            else
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, RET_ERROR, 0);
                return RET_ERROR;
            }
        }
        else if (expr->list[0]->elemtype == SPT_SUBQUERY)
        {
            if (is_init)
            {
                if (expr->list[0]->u.subq->queryresult.len == 1)
                {
                    sc_memcpy(val,
                            &(expr->list[0]->u.subq->queryresult.list[0].
                                    value), sizeof(T_VALUEDESC));
                    return RET_SUCCESS;
                }
            }

            node = expr->list[0];

            if (node->m_offset == 0)
            {
                ret = exec_singleton_subq(node->u.subq->handle, node->u.subq);
            }

            if (ret < 0)
            {
                return RET_ERROR;
            }

            result_buffer = (char *) node->u.subq->subq_result_rows->data;

            // 결과 칼럼 수가 1개이므로 첫 번째 byte를 본다.
            sc_memcpy(val,
                    &node->u.subq->queryresult.list[node->m_offset].value,
                    sizeof(T_VALUEDESC));
            val->call_free = 0; // 이거 설정해야 하나 ?

            if (result_buffer)
            {
                T_QUERYRESULT *qres = &node->u.subq->queryresult;

                null_value_mask =
                        result_buffer[sizeof(int) + (node->m_offset >> 3)];
                null_bit = 1;
                null_bit <<= (node->m_offset & 0x07);
                val->is_null = null_value_mask & null_bit;

                if (!val->is_null)
                {
                    result_len =
                            one_binary_column_length(&qres->list[node->
                                    m_offset], 1);
                    result_offset =
                            get_result_column_offset(qres, node->m_offset + 1,
                            1);

                    if (IS_BS(val->valuetype))
                        val->u.ptr = result_buffer + result_offset;
                    else if (IS_NBS(val->valuetype))
                    {
                        if (_checkalign((unsigned long) (result_buffer +
                                                result_offset),
                                        WCHAR_SIZE) == 0)
                            val->u.ptr = result_buffer + result_offset;
                        else
                        {
                            if (sql_value_ptrAlloc(val, result_len) < 0)
                            {
                                return RET_ERROR;
                            }
                            sc_memcpy(val->u.ptr,
                                    result_buffer + result_offset, result_len);
                        }
                    }
                    else if (IS_BYTE(val->valuetype))
                    {
                        val->u.ptr = result_buffer + result_offset + 4;
                    }
                    else
                    {
                        char *value_p = (char *) &val->u;

                        sc_memcpy(value_p, result_buffer + result_offset,
                                result_len);
                    }
                }
            }
            else
            {
                val->is_null = 1;
                val->call_free = 0;
            }

            if ((node->u.subq->kindofwTable & iTABLE_CORRELATED) == 0)
                node->u.subq->tstatus = TSS_EXECUTED;

        }
        else if (expr->list[0]->elemtype == SPT_FUNCTION)
        {
            node = expr->list[0];
            args = expr->list;
            sc_memset(result, 0, sizeof(T_POSTFIXELEM));
            if (calc_function(&node->u.func, result, args, is_init) < 0)
            {
                return RET_ERROR;
            }

            for (j = 0; j < node->u.func.argcnt; j++)
            {
                if (IS_ALLOCATED_TYPE(args[j]->u.value.valuetype))
                    sql_value_ptrFree(&(args[j]->u.value));
            }

            sc_memcpy(val, &(result->u.value), sizeof(T_VALUEDESC));
        }
        else if (expr->list[0]->elemtype != SPT_SUBQ_OP)
        {
            sc_memcpy(val, &(expr->list[0]->u.value), sizeof(T_VALUEDESC));

            if (!val->is_null && !is_init)
            {
                if (IS_BS_OR_NBS(val->valuetype))
                {
                    void *p = val->u.ptr;

#ifdef MDB_DEBUG
                    sc_assert(val->u.ptr != NULL, __FILE__, __LINE__);
#endif

                    if (val->attrdesc.len <= 0)
                    {
                        val->attrdesc.len =
                                strlen_func[val->valuetype] (val->u.ptr);
                    }

                    sql_value_ptrInit(val);

                    if (sql_value_ptrXcalloc(val, val->attrdesc.len + 1) < 0)
                    {
                        return RET_ERROR;
                    }

                    if ((unsigned long) p < (unsigned long) val->u.ptr &&
                            (unsigned long) p +
                            DB_BYTE_LENG(val->valuetype, val->attrdesc.len) >
                            (unsigned long) val->u.ptr)
                    {
                        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, RET_ERROR, 0);
                    }

                    // memcpy로 가도 될것 같다..    
                    // 끝에 NULL 안집어 넣어도 될듯한데...  : value_len 제대로 존재한다
                    if (IS_N_STRING(val->valuetype))
                        strncpy_func[val->valuetype] (val->u.ptr, p,
                                val->attrdesc.len);
                    else
                        memcpy_func[val->valuetype] (val->u.ptr, p,
                                val->attrdesc.len);
                }
                else if (IS_BYTE(val->valuetype) && val->value_len > 0)
                {
                    void *p = val->u.ptr;

#ifdef MDB_DEBUG
                    sc_assert(val->u.ptr != NULL, __FILE__, __LINE__);
#endif
                    if (val->value_len <= 0 && val->attrdesc.len <= 0)
                    {
                        // 일단은 이런 경우가 존재하면 안되므로..
                        // 지금은 assert를 걸자..
#if defined(MDB_DEBUG)
                        sc_assert(0, __FILE__, __LINE__);
#else
                        return RET_ERROR;
#endif
                    }

                    sql_value_ptrInit(val);

                    if (sql_value_ptrXcalloc(val, val->value_len) < 0)
                    {
                        return RET_ERROR;
                    }

                    if ((unsigned long) p < (unsigned long) val->u.ptr &&
                            (unsigned long) p + val->value_len >
                            (unsigned long) val->u.ptr)
                    {
                        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, RET_ERROR, 0);
                    }

                    sc_memcpy(val->u.ptr, p, val->value_len);
                }
            }
        }
        else
        {
            return RET_ERROR;
        }

        return RET_SUCCESS;
    }
    else if (expr->len == 0)
    {       // COUNT(*) 인 경우
        sc_memset(val, 0, sizeof(T_VALUEDESC));
        return RET_SUCCESS;
    }

    INIT_STACK(stack, expr->len, ret);
    if (ret < 0)
    {
        return RET_ERROR;
    }

    for (i = 0; i < expr->len; i++)
    {
        node = expr->list[i];
        if (node->elemtype == SPT_VALUE)
        {
            call_free_bk = node->u.value.call_free;
            node->u.value.call_free = 0;
            PUSH_STACK(stack, (*node), ret);
            node->u.value.call_free = call_free_bk;
            if (ret < 0)
                break;
        }
        else if (node->elemtype == SPT_SUBQUERY)
        {
            // 수행하고, uncorrelated subquery의 경우면 TSS_EXECUTED로 설정
            if (is_init && i + 1 < expr->len &&
                    expr->list[i + 1]->elemtype != SPT_SUBQ_OP)
            {
                result->elemtype = SPT_VALUE;
                sc_memcpy(&result->u.value,
                        &node->u.subq->queryresult.list[0].value,
                        sizeof(T_VALUEDESC));
            }
            else if (i + 1 < expr->len
                    && expr->list[i + 1]->elemtype != SPT_SUBQ_OP)
            {
                // 1. correlated variable binding
                // 2. subquery execution:수행 결과는 subq_result_rows & rows에
                // 3. postfix element setting
                //   exec_select()
                ret = exec_singleton_subq(node->u.subq->handle, node->u.subq);
                if (ret < 0)
                    break;

#ifdef MDB_DEBUG
                sc_assert(node->u.subq->queryresult.len == 1, __FILE__,
                        __LINE__);
                sc_assert(node->u.subq->kindofwTable & iTABLE_SUBWHERE,
                        __FILE__, __LINE__);
                sc_assert((char *) &node->u.value.u.bl ==
                        (char *) &node->u.value.u.i8, __FILE__, __LINE__);
                sc_assert((char *) &node->u.value.u.bl ==
                        (char *) &node->u.value.u.f16, __FILE__, __LINE__);
                sc_assert((char *) &node->u.value.u.bl ==
                        (char *) &node->u.value.u.dec, __FILE__, __LINE__);
                sc_assert((char *) &node->u.value.u.bl ==
                        (char *) &node->u.value.u.ch, __FILE__, __LINE__);
                sc_assert((char *) &node->u.value.u.bl ==
                        (char *) &node->u.value.u.ptr, __FILE__, __LINE__);
#endif

                result_len =
                        one_binary_column_length(node->u.subq->queryresult.
                        list, 1);
                result_buffer = (char *) node->u.subq->subq_result_rows->data;
                // node의 타입을 변경하고, 
                // subquery의 수행 결과로 저장된 scalar 값을 node에 저장하고,
                result->elemtype = SPT_VALUE;
                sc_memcpy(&result->u.value,
                        &node->u.subq->queryresult.list[0].value,
                        sizeof(T_VALUEDESC));
                // 레코드 구조는 bin_result_record_len()함수의 주석 참조
                // 칼럼수가 1개
                if (result_buffer)
                {
                    // 결과 칼럼 수가 1개이므로 첫 번째 byte를 본다.
                    null_value_mask = result_buffer[sizeof(int) + 0];
                    result->u.value.is_null = null_value_mask & 0x1;
                    result->u.value.call_free = 0;      // 이거 설정해야 하나 ?
                    if (IS_ALLOCATED_TYPE(result->u.value.valuetype))
                        result->u.value.u.ptr =
                                result_buffer + sizeof(int) + 1;
                    else
                        sc_memcpy(&result->u.value.u.bl,
                                result_buffer + sizeof(int) + 1, result_len);

                    // uncorrelated 인 경우는 다음에 수행 안 하도록 하자.
                    if ((node->u.subq->kindofwTable & iTABLE_CORRELATED) == 0)
                    {
                        node->u.subq->tstatus = TSS_EXECUTED;
                        // TODO: 어디선가 mdb_free(node->u.subq->subq_result_rows) 하고 널로
                        // 설정;
                    }
                }
                else
                {
                    result->u.value.is_null = 1;
                    result->u.value.call_free = 0;
                }
            }
            else
            {
                sc_memcpy(result, node, sizeof(T_POSTFIXELEM));
                result->u.value.call_free = 0;
            }

            PUSH_STACK(stack, (*result), ret);
            if (ret < 0)
                break;
        }
        else if (node->elemtype == SPT_SUBQ_OP)
        {
            POP_STACK_ARRAY(stack, args, node->u.op.argcnt, ret);
            if (ret < 0)
                break;

            ret = calc_subqexpr(args[1]->u.subq->handle,
                    &node->u.op, result, args);

            if (IS_ALLOCATED_TYPE(args[0]->u.value.valuetype))
                sql_value_ptrFree(&(args[0]->u.value));

#ifdef MDB_DEBUG
            sc_assert(result->u.value.valuetype == DT_BOOL, __FILE__,
                    __LINE__);
#endif

            PUSH_STACK(stack, (*result), ret);
            if (ret < 0)
                break;
        }
        else if (node->elemtype == SPT_OPERATOR)
        {
            POP_STACK_ARRAY(stack, args, node->u.op.argcnt, ret);
            if (ret < 0)
                break;

            if (is_init)
            {
                if (check_all_constant(node->u.op.argcnt, args))
                {
                    ret = calc_simpleexpr(&node->u.op, result, args, 0);

                    if (ret > -1)
                    {
                        expression_convert(node->u.op.argcnt, result,
                                expr, &i);
                    }
                }
                else
                {
                    ret = calc_simpleexpr(&node->u.op, result, args, is_init);
                    result->u.value.valueclass = SVC_VARIABLE;
                }
            }
            else
                ret = calc_simpleexpr(&node->u.op, result, args, is_init);

            for (j = 0; j < node->u.op.argcnt; j++)
            {
                if (IS_ALLOCATED_TYPE(args[j]->u.value.valuetype))
                    sql_value_ptrFree(&(args[j]->u.value));
            }

            if (ret < 0)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
                break;
            }

            PUSH_STACK(stack, (*result), ret);
            if (ret < 0)
                break;
        }
        else if (node->elemtype == SPT_FUNCTION)
        {
            POP_STACK_ARRAY(stack, args, node->u.func.argcnt, ret);
            if (ret < 0)
                break;

            sc_memset(result, 0, sizeof(T_POSTFIXELEM));

            if (is_init)
            {
                if (check_all_constant(node->u.op.argcnt, args)
                        && node->u.func.type != SFT_CHARTOHEX)
                {
                    ret = calc_function(&node->u.func, result, args, 0);
                    if (result->u.value.attrdesc.len <
                            args[0]->u.value.attrdesc.len)
                        result->u.value.attrdesc.len =
                                args[0]->u.value.attrdesc.len;

                    if (ret > -1)
                    {
                        if (IS_NBS(result->u.value.valuetype))
                        {
                            if (result->u.value.attrdesc.len * WCHAR_SIZE < 8)
                                result->u.value.attrdesc.len = 4;
                        }
                        else if (result->u.value.attrdesc.len < 8)
                            result->u.value.attrdesc.len = 8;
                        expression_convert(node->u.op.argcnt, result,
                                expr, &i);
                    }
                }
                else
                {
                    ret = calc_function(&node->u.func, result, args, is_init);
                    result->u.value.valueclass = SVC_VARIABLE;
                }
            }
            else
                ret = calc_function(&node->u.func, result, args, is_init);

            for (j = 0; j < node->u.func.argcnt; j++)
            {
                if (IS_ALLOCATED_TYPE(args[j]->u.value.valuetype))
                    sql_value_ptrFree(&(args[j]->u.value));
            }
            if (ret < 0)
                break;

            PUSH_STACK(stack, (*result), ret);
            if (ret < 0)
                break;
        }
        else if (node->elemtype == SPT_AGGREGATION)
        {
            if (node->u.aggr.type == SAT_COUNT && !node->u.aggr.argcnt)
            {
                result->elemtype = SPT_VALUE;
                result->u.value.valueclass = SVC_CONSTANT;
                result->u.value.valuetype = DT_INTEGER;
                result->u.value.call_free = 0;
                result->u.value.is_null = 0;
                result->u.value.attrdesc.type = DT_INTEGER;
                result->u.value.attrdesc.len = 4;
                result->u.value.attrdesc.dec = 0;
                result->u.value.attrdesc.collation = MDB_COL_NUMERIC;
            }
            else
            {
                POP_STACK_ARRAY(stack, texpr.list, node->u.aggr.argcnt, ret);
                if (ret < 0)
                    break;
                texpr.len = node->u.aggr.argcnt;
                texpr.max = node->u.aggr.argcnt;

                result->elemtype = SPT_VALUE;
                result->u.value.valueclass = SVC_CONSTANT;
                result->u.value.call_free = 0;
                result->u.value.is_null = 0;
                result->u.value.attrdesc.len = 0;

                ret = calc_expression(&texpr, &result->u.value, is_init);

                for (j = 0; j < node->u.aggr.argcnt; j++)
                {
                    if (texpr.list[j]->elemtype == SPT_VALUE &&
                            IS_ALLOCATED_TYPE(texpr.list[j]->u.value.
                                    valuetype))
                        sql_value_ptrFree(&(texpr.list[j]->u.value));
                }

                if (ret < 0)
                    break;
            }

            if (node->u.aggr.type == SAT_COUNT)
            {
                node->u.aggr.valtype = result->u.value.valuetype;
                node->u.aggr.length = result->u.value.attrdesc.len;
                node->u.aggr.decimals = result->u.value.attrdesc.dec;
                /* COUNT(DISTINCT colname) colname string type */
                node->u.aggr.collation = result->u.value.attrdesc.collation;
                if (!result->u.value.is_null &&
                        (IS_ALLOCATED_TYPE(result->u.value.valuetype)))
                    sql_value_ptrFree(&result->u.value);

                result->u.value.valuetype = DT_INTEGER;
            }
            else
            {
                node->u.aggr.valtype = result->u.value.valuetype;
                node->u.aggr.length = result->u.value.attrdesc.len;
                node->u.aggr.decimals = result->u.value.attrdesc.dec;
                node->u.aggr.collation = result->u.value.attrdesc.collation;

                if (node->u.aggr.type == SAT_AVG
                        || node->u.aggr.type == SAT_SUM)
                    result->u.value.valuetype = DT_DOUBLE;

                //SAT_MIN, SAT_MAX, SAT_SUM, SAT_AVG
                if (IS_BYTE(node->u.aggr.valtype))
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                            SQL_E_INVALID_AGGREGATION_TYPE, 0);
                    REMOVE_STACK(stack);
                    return RET_ERROR;
                }
            }

            PUSH_STACK(stack, (*result), ret);
            if (ret < 0)
                break;
        }
        else
        {
            call_free_bk = node->u.value.call_free;
            node->u.value.call_free = 0;
            PUSH_STACK(stack, (*node), ret);
            node->u.value.call_free = call_free_bk;
            if (ret < 0)
                break;
        }
    }

    if (ret > 0)
    {
        POP_STACK(stack, (*result), ret);
        if (ret < 0)
            ret = RET_ERROR;
        else
            sc_memcpy(val, &(result->u.value), sizeof(T_VALUEDESC));
    }
    REMOVE_STACK(stack);

    return ret;
}


/*****************************************************************************/
//! transform_tree 

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param expr() :
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *  - T_EXPRESSIONDESC 구조체 변경
 *
 *****************************************************************************/
T_TREE_NODE *
transform_tree(T_EXPRESSIONDESC * expr)
{
    T_POSTFIXELEM *elem;
    T_TREE_NODE *node = NULL;
    T_TREE_NODE **args = NULL;
    t_stack_template *stack;
    int i, j;

    init_stack_template(&stack);
    for (i = 0; i < expr->len; i++)
    {
        elem = expr->list[i];
        switch (elem->elemtype)
        {
        case SPT_VALUE:
        case SPT_SUBQUERY:
            node = ALLOC_TREE_NODE(elem, NULL, NULL);
            if (node == NULL)
                goto end;
            push_stack_template(node, &stack);
            break;
        case SPT_OPERATOR:
        case SPT_SUBQ_OP:
            /* expr이 1개로 이루어져 argumnet가 존재한지 않아
               stack 처리시 error 발생.
               error를 제거하기 위해 임의의 node 생성 */
            if (expr->len == 1 && (elem->u.op.type == SOT_ISNULL ||
                            elem->u.op.type == SOT_ISNOTNULL))
            {
                node = ALLOC_TREE_NODE(elem, NULL, NULL);
                if (node == NULL)
                {
                    goto end;
                }

                push_stack_template(node, &stack);
                break;
            }

            if (elem->u.op.argcnt != 0)
            {
                args = (T_TREE_NODE **) PMEM_ALLOC(elem->u.op.argcnt *
                        sizeof(T_TREE_NODE *));
                if (args == NULL)
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                            SQL_E_OUTOFMEMORY, 0);
                    return NULL;
                }

                for (j = 0; j < elem->u.op.argcnt; j++)
                {
                    args[elem->u.op.argcnt - j - 1] =
                            (T_TREE_NODE *) pop_stack_template(&stack);
                }
                if (elem->u.op.argcnt > 2)
                {
                    for (j = 2; j < elem->u.op.argcnt; j++)
                        PMEM_FREENUL(args[j]);
                    args[1]->vptr = NULL;       /* omit arguments */
                }
                if (elem->u.op.argcnt == 1)
                    node = ALLOC_TREE_NODE(elem, args[0], NULL);
                else
                    node = ALLOC_TREE_NODE(elem, args[0], args[1]);
                if (node == NULL)
                    goto end;
                push_stack_template(node, &stack);

                PMEM_FREENUL(args);
            }
            break;
        case SPT_FUNCTION:
            /* function에 대한 node를 생성하지 않아 stack 처리시 error가
               발생하여 임의로 node 생성 */
            if (elem->u.func.argcnt == 0)
            {
                node = ALLOC_TREE_NODE(elem, NULL, NULL);
                if (node == NULL)
                    goto end;
                push_stack_template(node, &stack);
            }
        case SPT_AGGREGATION:
            break;
        default:       /* error */
            break;
        }
    }

    node = pop_stack_template(&stack);

  end:
    {
        T_TREE_NODE *temp = NULL;

        while (1)
        {
            /* Remove error check */
            if (stack == NULL)
                break;

            temp = pop_stack_template(&stack);

            if (temp == NULL)
                break;

            destroy_tree_node(temp);
        }
    }

    return node;
}

/*****************************************************************************/
//! append_element_string 

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param sbuf() :
 * \param elem() : 
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
void
append_element_string(void *sbuf, T_POSTFIXELEM * elem)
{
    _T_STRING *p = (_T_STRING *) sbuf;
    char tbuf[STRING_BLOCK_SIZE] = "\0";

    append_nullstring(p, " ");
    if (elem == NULL)
    {
        append_nullstring(p, "...");
        return;
    }
    sc_memset(tbuf, 0, STRING_BLOCK_SIZE);
    switch (elem->elemtype)
    {
    case SPT_VALUE:
        if (elem->u.value.valueclass == SVC_CONSTANT)
        {
            if (IS_BS_OR_NBS(elem->u.value.valuetype))
                print_value_as_string(&elem->u.value, tbuf,
                        sizeof(tbuf) / SizeOfType[elem->u.value.valuetype] -
                        1);
            else
                print_value_as_string(&elem->u.value, tbuf, sizeof(tbuf));
        }
        else
        {   /* valueclass == SVC_VARIABLE */
            sc_sprintf(tbuf, "%s.%s",
                    ((elem->u.value.attrdesc.table.aliasname) ?
                            elem->u.value.attrdesc.table.aliasname :
                            elem->u.value.attrdesc.table.tablename),
                    elem->u.value.attrdesc.attrname);
        }
        append_nullstring(p, tbuf);
        break;
    case SPT_SUBQUERY:
        append_nullstring(p, "?");
        break;
    case SPT_OPERATOR:
    case SPT_SUBQ_OP:
        append_nullstring(p, __op_names[elem->u.op.type]);
        break;
    case SPT_FUNCTION:
        append_nullstring(p, __func_names[elem->u.func.type]);
        break;
    case SPT_AGGREGATION:
        append_nullstring(p, __aggr_names[elem->u.aggr.type]);
        break;
    default:
        append_nullstring(p, "?");
        break;
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
append_left_parenthesis(void *sbuf, T_POSTFIXELEM * elem)
{
    if (elem && elem->elemtype == SPT_OPERATOR && elem->u.op.type == SOT_OR)
        append_nullstring((_T_STRING *) sbuf, "(");
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
append_right_parenthesis(void *sbuf, T_POSTFIXELEM * elem)
{
    if (elem && elem->elemtype == SPT_OPERATOR && elem->u.op.type == SOT_OR)
        append_nullstring((_T_STRING *) sbuf, ")");
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
append_expression_string(_T_STRING * sbuf, T_EXPRESSIONDESC * expr)
{
    T_TREE_NODE *tree;
    TRAVERSE_TREE_ARG arg;

    arg.pre_fun = append_left_parenthesis;
    arg.pre_arg = (void *) sbuf;
    arg.in_fun = append_element_string;
    arg.in_arg = (void *) sbuf;
    arg.post_fun = append_right_parenthesis;
    arg.post_arg = (void *) sbuf;

    tree = transform_tree(expr);
    traverse_tree_node(tree, &arg);
    destroy_tree_node(tree);
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
check_all_constant(int argcnt, T_POSTFIXELEM ** args)
{
    int i, is_constant = 1;

    if (argcnt > 0)
    {
        for (i = 0; i < argcnt; i++)
        {
            if (args[i]->u.value.valueclass != SVC_CONSTANT)
            {
                is_constant = 0;
                break;
            }
            else if (args[i]->u.value.valuetype == DT_NULL_TYPE)
            {
                is_constant = 0;
                break;
            }
            else if (args[i]->u.value.valueclass == SVC_CONSTANT &&
                    args[i]->u.aggr.type == SAT_DCOUNT)
            {
                is_constant = 0;
                break;
            }
        }
    }
    else
        is_constant = 0;

    return is_constant;
}

int
expression_convert(int argcnt, T_POSTFIXELEM * result,
        T_EXPRESSIONDESC * expr, int *position)
{
    int i, ret = DB_SUCCESS;

    for (i = *position - argcnt; i < *position; i++)
        if (IS_ALLOCATED_TYPE(expr->list[i]->u.value.valuetype))
            sql_value_ptrFree(&(expr->list[i]->u.value));
    sc_memcpy(expr->list[*position - argcnt], result, sizeof(T_POSTFIXELEM));
    result->u.value.call_free = 0;

    for (i = *position + 1; i < expr->len; i++)
        expr->list[i - argcnt] = expr->list[i];

    expr->len -= argcnt;
    *position -= argcnt;

    return ret;
}

int
preconvert_expression(int argcnt, T_EXPRESSIONDESC * expr, int *position)
{
    int ret = DB_SUCCESS;
    T_POSTFIXELEM temp_org;
    T_POSTFIXELEM *elem, *temp, **exprs;
    int elem_argcnt = 0;

    if (argcnt == 0)
        return ret;

    elem = expr->list[*position];
    if (elem->elemtype == SPT_OPERATOR)
        elem_argcnt = elem->u.op.argcnt;
    else if (elem->elemtype == SPT_FUNCTION)
        elem_argcnt = elem->u.func.argcnt;

    if (elem_argcnt == 0)
        return ret;
    temp = &temp_org;
    exprs = &(expr->list[*position - elem_argcnt]);

    if (check_all_constant(elem_argcnt, exprs))
    {
        if (elem->elemtype == SPT_OPERATOR)
        {
            ret = calc_simpleexpr(&elem->u.op, temp, exprs, 0);
            if (ret > -1)
            {
                temp->u.value.value_len = temp->u.value.attrdesc.len;
                expression_convert(elem->u.op.argcnt, temp, expr, position);
            }
        }

        if (elem->elemtype == SPT_FUNCTION &&
                elem->u.func.type != SFT_CHARTOHEX)
        {
            ret = calc_function(&elem->u.func, temp, exprs, 0);
            if (ret > -1)
            {
                temp->u.value.value_len = temp->u.value.attrdesc.len;
                if (IS_NBS(temp->u.value.valuetype))
                {
                    if (temp->u.value.attrdesc.len * WCHAR_SIZE < 8)
                        temp->u.value.attrdesc.len = 4;
                }
                else if (temp->u.value.attrdesc.len < 8)
                    temp->u.value.attrdesc.len = 8;

                expression_convert(elem->u.func.argcnt, temp, expr, position);
            }
        }
    }

    return ret;
}

int
type_convert(T_VALUEDESC * val1, T_VALUEDESC * val2)
{
    int ret = RET_SUCCESS;
    T_ATTRDESC *attrdesc;
    T_VALUEDESC *src;

    if ((IS_NUMERIC(val1->valuetype)
                    && IS_NUMERIC(val2->valuetype)) ||
            (IS_NUMERIC(val1->attrdesc.type)
                    && IS_NUMERIC(val2->attrdesc.type)))
        return ret;

    if (val1->valueclass == SVC_CONSTANT && val2->valueclass == SVC_VARIABLE)
    {
        src = val1;
        attrdesc = &(val2->attrdesc);
    }
    else if (val2->valueclass == SVC_CONSTANT &&
            val1->valueclass == SVC_VARIABLE)
    {
        src = val2;
        attrdesc = &(val1->attrdesc);
    }
    else
        return ret;

    if (src->valuetype != DT_NULL_TYPE && attrdesc->type != DT_NULL_TYPE &&
            src->valuetype != attrdesc->type && attrdesc->type != DT_BYTE)
    {
        T_VALUEDESC temp, base;

        sc_memcpy(&temp, src, sizeof(T_VALUEDESC));
        sc_memset(&base, 0, sizeof(T_VALUEDESC));

        base.attrdesc.len = src->attrdesc.len = attrdesc->len + 1;
        base.valuetype = src->valuetype = attrdesc->type;
        base.attrdesc.type = src->attrdesc.type = attrdesc->type;
        base.attrdesc.collation = src->attrdesc.collation =
                attrdesc->collation;
        base.call_free = src->call_free;
        src->call_free = 0;
        base.u.ptr = src->u.ptr;

        ret = calc_func_convert(&base, &temp, src, MDB_FALSE);
        if (ret < 0)
        {
            sql_value_ptrFree(&base);
            return ret;
        }

        if (IS_BS_OR_NBS(src->valuetype) && !src->is_null)
        {
            src->attrdesc.len = strlen_func[src->valuetype] (src->u.ptr);
            src->value_len = src->attrdesc.len;
        }

        sql_value_ptrFree(&base);
    }
    return ret;
}

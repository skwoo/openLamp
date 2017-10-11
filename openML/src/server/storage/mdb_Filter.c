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

#include "mdb_Server.h"
#include "mdb_Filter.h"
#include "mdb_KeyValue.h"
#include "mdb_dbi.h"
#include "mdb_PMEM.h"

int KeyValue_Delete(struct KeyValue *pKeyValue);

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
struct Filter
dbi_Filter(int n, FIELDVALUE_T * expression)
{
    struct Filter filter;
    int k;

    filter.expression_count_ = n;

    filter.expression_ = (FIELDVALUE_T *) PMEM_ALLOC(sizeof(FIELDVALUE_T) * n);

    for (k = 0; k < n; k++)
    {
        filter.expression_[k] = expression[k];
    }

    return filter;
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
extern int (*compare_func[]) ();
int
Filter_Compare(struct Filter *filter, char *record, int recSize)
{
    register int n, count;
    register FIELDVALUE_T *fieldval;
    register int result;

    count = filter->expression_count_;

    for (n = 0; n < count; n++)
    {
        fieldval = &(filter->expression_[n]);

        if (fieldval->mode_ == MDB_NU)
            result = DB_VALUE_ISNULL(record, fieldval->position_,
                    recSize) ? 1 : 0;
        else if (fieldval->mode_ == MDB_NN)
            result = DB_VALUE_ISNULL(record, fieldval->position_,
                    recSize) ? 0 : 1;
        else
        {
            if (DB_VALUE_ISNULL(record, fieldval->position_, recSize)
                    || fieldval->is_null)
            {
                result = 0;
            }
            else
            {
                result = compare_func[fieldval->type_] (fieldval, record,
                        recSize);
                switch (fieldval->mode_)
                {
                case MDB_GT:
                    result = (result < 0);
                    break;      /* fieldval < record */
                case MDB_GE:
                    result = (result <= 0);
                    break;      /* <= */
                case MDB_LT:
                    result = (result > 0);
                    break;      /* > */
                case MDB_LE:
                    result = (result >= 0);
                    break;      /* >= */
                case MDB_EQ:
                    result = (result == 0);
                    break;      /* == */
                case MDB_NE:
                    result = (result != 0);
                    break;      /* != */
                case LIKE:
                    result = (result == 0);
                    break;
                case MDB_ILIKE:
                    result = (result == 0);
                    break;

                default:
                    result = 0;
                    break;
                }
            }
        }

        if (result == 0)
            return DB_FAIL;
    }

    return DB_SUCCESS;
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
Filter_Delete(struct Filter *pFilter)
{
    return KeyValue_Delete((struct KeyValue *) pFilter);
}

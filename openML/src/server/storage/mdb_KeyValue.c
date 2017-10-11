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
#include "mdb_Mem_Mgr.h"
#include "mdb_KeyValue.h"
#include "mdb_KeyDesc.h"
#include "mdb_syslog.h"
#include "mdb_dbi.h"
#include "mdb_PMEM.h"

#include "sql_datast.h"

static int compare_NULL_TYPE(FIELDVALUE_T * field, char *, DB_INT32 recSize);
static int compare_BOOL(FIELDVALUE_T * field, char *, DB_INT32 recSize);
static int compare_TINYINT(FIELDVALUE_T * field, char *, DB_INT32 recSize);
static int compare_SMALLINT(FIELDVALUE_T * field, char *, DB_INT32 recSize);
static int compare_INTEGER(FIELDVALUE_T * field, char *, DB_INT32 recSize);
static int compare_FLOAT(FIELDVALUE_T * field, char *, DB_INT32 recSize);
static int compare_DOUBLE(FIELDVALUE_T * field, char *, DB_INT32 recSize);
static int compare_BIGINT(FIELDVALUE_T * field, char *, DB_INT32 recSize);
static int compare_LIKESTRING(FIELDVALUE_T * field, char *, DB_INT32 recSize);
static int compare_CHAR(FIELDVALUE_T * field, char *, DB_INT32 recSize);
static int compare_NCHAR(FIELDVALUE_T * field, char *, DB_INT32 recSize);
static int compare_VARCHAR(FIELDVALUE_T * field, char *, DB_INT32 recSize);
static int compare_NVARCHAR(FIELDVALUE_T * field, char *, DB_INT32 recSize);

static int compare_DATETIME(FIELDVALUE_T * field, char *, DB_INT32 recSize);
static int compare_DATE(FIELDVALUE_T * field, char *, DB_INT32 recSize);
static int compare_TIME(FIELDVALUE_T * field, char *, DB_INT32 recSize);
static int compare_TIMESTAMP(FIELDVALUE_T * field, char *, DB_INT32 recSize);
static int compare_DECIMAL(FIELDVALUE_T * field, char *, DB_INT32 recSize);
static int compare_HEX(FIELDVALUE_T * field, char *, DB_INT32 recSize);
static int compare_BYTE(struct FieldValue *field, char *, DB_INT32 recSize);
static int compare_VARBYTE(struct FieldValue *field, char *, DB_INT32 recSize);

int (*compare_func[]) () =
{
    compare_NULL_TYPE,  /* DT_NULL_TYPE */
            compare_BOOL,       /* DT_BOOL */
            compare_TINYINT,    /* DT_TINYINT */
            compare_SMALLINT,   /* DT_SMALLINT */
            compare_INTEGER,    /* DT_INTEGER */
            /* unused */
            NULL, compare_BIGINT,       /* DT_BIGINT */
            compare_FLOAT,      /* DT_FLOAT */
            compare_DOUBLE,     /* DT_DOUBLE */
            compare_DECIMAL,    /* DT_DECIMAL */
            compare_CHAR,       /* DT_CHAR */
            compare_NCHAR,      /* DT_NCHAR */
            compare_VARCHAR,    /* DT_VARCHAR */
            compare_NVARCHAR,   /* DT_NVARCHAR */
            compare_BYTE,       /* DT_BYTE */
            compare_VARBYTE,    /* DT_VARBYTE */
            /* unused */
            NULL, NULL, compare_TIMESTAMP,      /* DT_TIMESTAMP */
            compare_DATETIME,   /* DT_DATETIME */
            compare_DATE,       /* DT_DATE */
            compare_TIME,       /* DT_TIME */
compare_HEX};

extern int (*sc_collate_func[]) (const char *s1, const char *s2);
extern int (*sc_ncollate_func[]) (const char *s1, const char *s2, int n);

///////////////////////////////////////////////////////////
//
// Function Name :
//
///////////////////////////////////////////////////////////

/*****************************************************************************/
//! dbi_KeyValue
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param param_name        :
 * \param fieldValue(in)    : src
 * \param keyvalue(in/out)  : dest
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
dbi_KeyValue(int n, FIELDVALUE_T * fieldValue, struct KeyValue *keyvalue)
{
    /* init keyValue */
    keyvalue->field_value_ = NULL;

    keyvalue->field_count_ = n;

    /* Caller must free field_value_ */
    keyvalue->field_value_ =
            (struct FieldValue *) PMEM_ALLOC(sizeof(struct FieldValue) * n);
    if (keyvalue->field_value_ == NULL)
    {
        keyvalue->field_count_ = 0;
        return DB_E_OUTOFMEMORY;
    }

    sc_memcpy(keyvalue->field_value_, fieldValue,
            sizeof(struct FieldValue) * n);

    return DB_SUCCESS;
}

/*****************************************************************************/
//! KeyValue_Compare
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param keyvalue()  :
 * \param record()        : 
 * \param recSize()       :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘 및 기타 설명
 *****************************************************************************/
int
KeyValue_Compare(struct KeyValue *keyvalue, char *record, int recSize)
{
    register int n;
    register struct FieldValue *fieldval;
    register int result = 0;

    n = 0;
    while (1)
    {
        if (n == keyvalue->field_count_)
            break;

        fieldval = &(keyvalue->field_value_[n]);

        if (fieldval->mode_ == MDB_NN || fieldval->mode_ == MDB_GT ||
                fieldval->mode_ == MDB_LT || fieldval->mode_ == ' ')
        {
            result = DB_VALUE_ISNULL(record, fieldval->position_, recSize);

            /* record 값이 NULL이면 key 값는 record값 보다 작다 */
            if (result)
                result = -1;
            else
            {
                result = compare_func[fieldval->type_] (fieldval,
                        record, recSize);
                if (result == 0)
                {
                    if (fieldval->mode_ == MDB_GT)
                    {
                        result = 1;
                    }
                    else if (fieldval->mode_ == MDB_LT)
                    {
                        result = -1;
                    }
                }
            }
        }
        else if (fieldval->mode_ == MDB_NU)
        {
            result = DB_VALUE_ISNULL(record, fieldval->position_, recSize);
            /* NULL을 가장 큰 값으로 취급하므로,
               record에서 해당 필드값이 NULL이 아닌 경우
               key 값이 큰 것으로 return */
            if (result)
                result = 0;
            else
                result = 1;     /* null 이므로 같음 */
        }
        else
        {
            /* 필드값에 상관없이 ok하는 mode. */
            /* AL이 나온 경우 현재 필드 포함 이후 것은 무시하고
               바로 return 처리 */
            return result;
        }

        if (result == 0)
        {
        }
        else
        {
            if (fieldval->order_ == 'D')
                return -result;

            return result;
        }

        n++;
    }       /* while */

    return 0;
}

/*****************************************************************************/
//!  KeyValue_Create
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param record(in)                : record
 * \param key_desc1(in)              : 
 * \param recSize(in):
 * \param keyValue(in/out):
 * \param f_memory_record(in)
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *  - called : TTree_FindPositionWithValueAndID()
 *  - VARIABLE TYPE 함수 변경
 *  - collation 멤버 변수 추가
 *  - 기존에는 dest's byte length를 리턴했으나..
 *      함수가 애매 모호해져서, 안쪽에서 변환한다
 *
 *****************************************************************************/
int
KeyValue_Create(const char *record, struct KeyDesc *key_desc1,
        int recSize, struct KeyValue *keyValue, int f_memory_record)
{
    struct KeyDesc *key_desc = key_desc1;
    int ret_val;
    DB_UINT16 n = 0;
    int offset;

    keyValue->field_count_ = key_desc->field_count_;

    keyValue->field_value_ = (struct FieldValue *)
            PMEM_ALLOC(sizeof(struct FieldValue) * keyValue->field_count_);
    if (keyValue->field_value_ == NULL)
    {
        ret_val = DB_E_OUTOFMEMORY;
        goto error;
    }

    for (n = 0; n < keyValue->field_count_; n++)
    {
        if (DB_VALUE_ISNULL(record, key_desc->field_desc_[n].position_,
                        recSize))
            keyValue->field_value_[n].mode_ = MDB_NU;
        else
            keyValue->field_value_[n].mode_ = MDB_NN;

        keyValue->field_value_[n].type_ = key_desc->field_desc_[n].type_;
        keyValue->field_value_[n].position_ =
                key_desc->field_desc_[n].position_;
        keyValue->field_value_[n].offset_ = key_desc->field_desc_[n].offset_;
        keyValue->field_value_[n].h_offset_ =
                key_desc->field_desc_[n].h_offset_;
        keyValue->field_value_[n].length_ = key_desc->field_desc_[n].length_;
        // 아래는 실제 길이가 할당되는것이 맞을것 같다
        keyValue->field_value_[n].value_length_ =
                key_desc->field_desc_[n].length_;
        keyValue->field_value_[n].order_ = key_desc->field_desc_[n].order_;
        keyValue->field_value_[n].fixed_part =
                key_desc->field_desc_[n].fixed_part;

        // 안정화가 되면 아래 루틴은 제거할것
#if defined(MDB_DEBUG)
        if (key_desc->field_desc_[n].collation == MDB_COL_NONE)
            sc_assert(0, __FILE__, __LINE__);
#endif
        keyValue->field_value_[n].collation =
                key_desc->field_desc_[n].collation;

        if (f_memory_record)
            offset = key_desc->field_desc_[n].offset_;
        else
            offset = key_desc->field_desc_[n].h_offset_;

        switch (key_desc->field_desc_[n].type_)
        {
        case DT_TINYINT:
            keyValue->field_value_[n].value_.char_ =
                    *(char *) (record + offset);
            break;
        case DT_SHORT:
            keyValue->field_value_[n].value_.short_ =
                    *(short *) (record + offset);
            break;
        case DT_INTEGER:
            keyValue->field_value_[n].value_.int_ = *(int *) (record + offset);
            break;
        case DT_BIGINT:
            sc_memcpy(&(keyValue->field_value_[n].value_.bigint_),
                    (DB_INT64 *) (record + offset),
                    key_desc->field_desc_[n].length_);
            break;
        case DT_DATETIME:
        case DT_DATE:
        case DT_TIME:
            sc_memcpy(&(keyValue->field_value_[n].value_.datetime_),
                    (char *) (record + offset),
                    key_desc->field_desc_[n].length_);
            break;
        case DT_TIMESTAMP:
            sc_memcpy(&(keyValue->field_value_[n].value_.timestamp_),
                    (DB_INT64 *) (record + offset),
                    key_desc->field_desc_[n].length_);
            break;
        case DT_FLOAT:
            keyValue->field_value_[n].value_.float_ =
                    *(float *) (record + offset);
            break;
        case DT_DOUBLE:
            sc_memcpy(&(keyValue->field_value_[n].value_.double_),
                    (double *) (record + offset),
                    key_desc->field_desc_[n].length_);
            break;
        case DT_VARCHAR:
        case DT_CHAR:
            keyValue->field_value_[n].value_.pointer_ =
                    (char *) PMEM_ALLOC(key_desc->field_desc_[n].length_ + 1);
            if (keyValue->field_value_[n].value_.pointer_ == NULL)
            {
                ret_val = DB_E_OUTOFMEMORY;
                goto error;
            }

            if (f_memory_record)
            {
                sc_memcpy(keyValue->field_value_[n].value_.pointer_,
                        (char *) (record + offset),
                        key_desc->field_desc_[n].length_);
                keyValue->field_value_[n].value_.pointer_[key_desc->
                        field_desc_[n].length_] = '\0';
                keyValue->field_value_[n].value_length_ =
                        sc_strlen(keyValue->field_value_[n].value_.pointer_);
                break;
            }

            sc_memset(keyValue->field_value_[n].value_.pointer_, 0,
                    key_desc->field_desc_[n].length_ + 1);

            if (IS_VARIABLE_DATA(key_desc->field_desc_[n].type_,
                            key_desc->field_desc_[n].fixed_part))
            {
                keyValue->field_value_[n].value_length_ =
                        dbi_strcpy_variable(keyValue->field_value_[n].value_.
                        pointer_, (char *) (record + offset),
                        key_desc->field_desc_[n].fixed_part,
                        key_desc->field_desc_[n].type_,
                        key_desc->field_desc_[n].length_);
            }
            else
            {
                sc_memcpy(keyValue->field_value_[n].value_.pointer_,
                        (char *) (record + offset),
                        key_desc->field_desc_[n].length_);

                keyValue->field_value_[n].value_length_ =
                        sc_strlen(keyValue->field_value_[n].value_.pointer_);
            }
            break;
        case DT_NVARCHAR:
        case DT_NCHAR:
            keyValue->field_value_[n].value_.Wpointer_ = (DB_WCHAR *)
                    PMEM_ALLOC((key_desc->field_desc_[n].length_ + 1) *
                    sizeof(DB_WCHAR));
            if (keyValue->field_value_[n].value_.Wpointer_ == NULL)
            {
                ret_val = DB_E_OUTOFMEMORY;
                goto error;
            }

            if (f_memory_record)
            {
                sc_memcpy(keyValue->field_value_[n].value_.Wpointer_,
                        (char *) (record + offset),
                        key_desc->field_desc_[n].length_ * sizeof(DB_WCHAR));
                keyValue->field_value_[n].value_.Wpointer_[key_desc->
                        field_desc_[n].length_] = '\0';
                keyValue->field_value_[n].value_length_ =
                        sc_wcslen(keyValue->field_value_[n].value_.Wpointer_);
                break;
            }

            sc_memset(keyValue->field_value_[n].value_.Wpointer_, 0,
                    (key_desc->field_desc_[n].length_ + 1) * sizeof(DB_WCHAR));

            if (IS_VARIABLE_DATA(key_desc->field_desc_[n].type_,
                            key_desc->field_desc_[n].fixed_part))
            {
                keyValue->field_value_[n].value_length_ =
                        dbi_strcpy_variable(keyValue->field_value_[n].value_.
                        pointer_, (char *) (record + offset),
                        key_desc->field_desc_[n].fixed_part,
                        key_desc->field_desc_[n].type_,
                        (key_desc->field_desc_[n].length_) * sizeof(DB_WCHAR));
            }
            else
            {
                sc_memcpy(keyValue->field_value_[n].value_.Wpointer_,
                        (char *) (record + offset),
                        key_desc->field_desc_[n].length_ * sizeof(DB_WCHAR));

                keyValue->field_value_[n].value_length_ =
                        sc_wcslen(keyValue->field_value_[n].value_.Wpointer_);
            }
            break;

        case DT_DECIMAL:
            keyValue->field_value_[n].value_.decimal_ =
                    (char *) PMEM_ALLOC(key_desc->field_desc_[n].length_ + 3);
            if (keyValue->field_value_[n].value_.decimal_ == NULL)
            {
                ret_val = DB_E_OUTOFMEMORY;
                goto error;
            }

            sc_memcpy(keyValue->field_value_[n].value_.decimal_,
                    (char *) (record + offset),
                    key_desc->field_desc_[n].length_);
            break;

        default:
            MDB_SYSLOG(("Error: Unknown key_value type %d\n",
                            key_desc->field_desc_[n].type_));
            ret_val = DB_FAIL;
            goto error;
        }
    }

    return DB_SUCCESS;

  error:
    /* delete incomplete keyvalue */
    if (keyValue->field_value_ != NULL)
    {
        int i;

        for (i = 0; i < n; i++)
        {
            switch (keyValue->field_value_[i].type_)
            {
            case DT_CHAR:
            case DT_VARCHAR:
            case DT_NCHAR:
            case DT_NVARCHAR:
            case DT_DECIMAL:
                PMEM_FREENUL(keyValue->field_value_[i].value_.decimal_);
                break;
            default:
                break;
            }
        }
        PMEM_FREENUL(keyValue->field_value_);
    }

    keyValue->field_count_ = 0;
    return ret_val;
}


/*****************************************************************************/
//! KeyValue_Check
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param min(in)   :
 * \param max(in)   : 
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘 및 기타 설명
 *  - min/max KEY's COLLATION이 다른 경우에 error 처리를 하자
 *****************************************************************************/
int
KeyValue_Check(struct KeyValue *min, struct KeyValue *max)
{
    DB_UINT16 n;
    int comp_res;

    if (min == NULL && max == NULL)
        return DB_FAIL;

    if (min == NULL && max != NULL)
        return DB_FAIL;

    if (min != NULL && max == NULL)
        return DB_FAIL;

    if (min->field_count_ == 0 && max->field_count_ == 0)
        return DB_FAIL;

    if (min->field_count_ != max->field_count_)
        return DB_FAIL;

#if defined(MDB_DEBUG)
    for (n = 0; n < min->field_count_; n++)
    {
        if (min->field_value_[n].collation != max->field_value_[n].collation)
            sc_assert(0, __FILE__, __LINE__);
    }
#endif

    for (n = 0; n < min->field_count_; n++)
    {
        if (min->field_value_[n].mode_ != MDB_NN ||
                max->field_value_[n].mode_ != MDB_NN)
            return DB_FAIL;

        switch (min->field_value_[n].type_)
        {
        case DT_VARCHAR:
        case DT_NVARCHAR:
        case DT_CHAR:
        case DT_NCHAR:
            comp_res =
                    sc_ncollate_func[min->field_value_[n].collation] (min->
                    field_value_[n].value_.pointer_,
                    max->field_value_[n].value_.pointer_,
                    min->field_value_[n].length_);
            break;
        case DT_DECIMAL:
            comp_res = sc_memcmp(min->field_value_[n].value_.pointer_,
                    max->field_value_[n].value_.pointer_,
                    min->field_value_[n].length_ + 2);
            break;
        default:
            comp_res = sc_memcmp(&min->field_value_[n].value_.int_,
                    &max->field_value_[n].value_.int_,
                    min->field_value_[n].length_);
            break;
        }

        if (comp_res != 0)
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
 * \note dbi_KeyValue() 용
 *****************************************************************************/
int
KeyValue_Delete(struct KeyValue *pKeyValue)
{
    DB_UINT32 n;

    if (pKeyValue == NULL)
        return DB_SUCCESS;

    if (pKeyValue->field_value_ == NULL || pKeyValue->field_count_ == 0)
    {
        pKeyValue->field_value_ = NULL;
        pKeyValue->field_count_ = 0;
        return DB_SUCCESS;
    }

    for (n = 0; n < pKeyValue->field_count_; n++)
    {
        switch (pKeyValue->field_value_[n].type_)
        {
        case DT_VARCHAR:
        case DT_NVARCHAR:
        case DT_CHAR:
        case DT_NCHAR:
            PMEM_FREENUL(pKeyValue->field_value_[n].value_.pointer_);
            break;
        case DT_DECIMAL:
            PMEM_FREENUL(pKeyValue->field_value_[n].value_.decimal_);
            break;
        default:
            break;
        }
    }

    PMEM_FREENUL(pKeyValue->field_value_);

    pKeyValue->field_count_ = 0;

    return DB_SUCCESS;
}

/* KeyValue_Create() 용 */
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
 * \note KeyValue_Create()
 *****************************************************************************/
int
KeyValue_Delete2(struct KeyValue *pKeyValue)
{
    DB_UINT32 n;

    if (pKeyValue == NULL)
        return DB_SUCCESS;

    if (pKeyValue->field_value_ == NULL || pKeyValue->field_count_ == 0)
    {
        pKeyValue->field_value_ = NULL;
        pKeyValue->field_count_ = 0;
        return DB_SUCCESS;
    }

    for (n = 0; n < pKeyValue->field_count_; n++)
    {
        switch (pKeyValue->field_value_[n].type_)
        {
        case DT_VARCHAR:
        case DT_NVARCHAR:
        case DT_CHAR:
        case DT_NCHAR:
            PMEM_FREENUL(pKeyValue->field_value_[n].value_.pointer_);
            break;
        case DT_DECIMAL:
            PMEM_FREENUL(pKeyValue->field_value_[n].value_.decimal_);
            break;
        default:
            break;
        }
    }

    PMEM_FREENUL(pKeyValue->field_value_);
    pKeyValue->field_count_ = 0;

    return DB_SUCCESS;
}


/*****************************************************************************/
//! KeyField_Compare 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param field(in) :
 * \param record(in) : 
 * \param recSize(in) :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
KeyField_Compare(struct FieldValue *field, char *record, DB_INT32 recSize)
{
    return compare_func[field->type_] (field, record, recSize);
}

/***************************************************************
  *         Type Compare functions
  ***************************************************************/

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
 * \note 
 *****************************************************************************/
static int
compare_NULL_TYPE(struct FieldValue *field, char *record, DB_INT32 recSize)
{
    return DB_FAIL;
}

/*****************************************************************************/
//! compare_BOOL
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param field(in)     : field's value
 * \param record(in)    : record's value
 * \param recSize(in)   : size of record's value
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *****************************************************************************/
static int
compare_BOOL(struct FieldValue *field, char *record, DB_INT32 recSize)
{
    return TRUE;
}

/*****************************************************************************/
//! compare_TINYINT
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param field(in)     : field's value
 * \param record(in)    : record's value
 * \param recSize(in)   : size of record's value
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *****************************************************************************/
static int
compare_TINYINT(struct FieldValue *field, char *record, DB_INT32 recSize)
{
    char r_value = record[field->h_offset_];

    return (field->value_.char_ < r_value) ? -1 :
            (r_value == field->value_.char_) ? 0 : 1;
}

/*****************************************************************************/
//! compare_SMALLINT
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param field(in)     : field's value
 * \param record(in)    : record's value
 * \param recSize(in)   : size of record's value
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *****************************************************************************/
static int
compare_SMALLINT(struct FieldValue *field, char *record, DB_INT32 recSize)
{
    char *r_value = record + field->h_offset_;
    short s = *(short *) r_value;

    return (field->value_.short_ < s) ? -1 :
            (s == field->value_.short_) ? 0 : 1;
}

/*****************************************************************************/
//! compare_INTEGER
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param field(in)     : field's value
 * \param record(in)    : record's value
 * \param recSize(in)   : size of record's value
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *****************************************************************************/
static int
compare_INTEGER(struct FieldValue *field, char *record, DB_INT32 recSize)
{
    int i = *(int *) (record + field->h_offset_);

    if (field->value_.int_ == i)
        return 0;
    if (field->value_.int_ > i)
        return 1;
    else if (field->value_.int_ < i)
        return -1;

    return 0;
}


/*****************************************************************************/
//! compare_FLOAT
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param field(in)     : field's value
 * \param record(in)    : heap record's value
 * \param recSize(in)   : size of record's value
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *****************************************************************************/
static int
compare_FLOAT(struct FieldValue *field, char *record, DB_INT32 recSize)
{
    char *r_value = record + field->h_offset_;
    float f = *(float *) r_value;

    return ((field->value_.float_ < f) ? -1 :
            (field->value_.float_ > f) ? 1 : 0);
}

/*****************************************************************************/
//! compare_DOUBLE
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param field(in)     : field's value
 * \param record(in)    : heap record's value
 * \param recSize(in)   : size of record's value
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *****************************************************************************/
static int
compare_DOUBLE(struct FieldValue *field, char *record, DB_INT32 recSize)
{
    char *r_value = record + field->h_offset_;
    double d;

    /* 8 byte alignment */
    sc_memcpy(&d, r_value, sizeof(double));

    return (field->value_.double_ < d) ? -1 :
            (field->value_.double_ == d) ? 0 : 1;
}

/*****************************************************************************/
//! compare_BIGINT
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param field(in)     : field's value
 * \param record(in)    : heap record's value
 * \param recSize(in)   : size of record's value
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *****************************************************************************/
static int
compare_BIGINT(struct FieldValue *field, char *record, DB_INT32 recSize)
{
    char *r_value = record + field->h_offset_;
    DB_INT64 ll;

    /* 8 byte alignment */
    sc_memcpy(&ll, r_value, sizeof(DB_INT64));

    return (field->value_.bigint_ < ll) ? -1 :
            (ll == field->value_.bigint_) ? 0 : 1;
}

/*****************************************************************************/
//! compare_LIKESTRING
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param field(in)     : field's value
 * \param record(in)    : heap record's value
 * \param recSize(in)   : size of record's value
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *  - variable의 경우 루틴이 변경됨
 *  - alloc 받는 루틴을 수정
 *****************************************************************************/
static int
compare_LIKESTRING(struct FieldValue *field, char *record, DB_INT32 recSize)
{
    int ret;
    char *r_value = record + field->h_offset_;

    if (!IS_VARIABLE_DATA(field->type_, field->fixed_part))
    {
        if (strlen_func[field->type_] (r_value) >= (int) field->length_)
        {
            if (IS_NBS(field->type_))
                r_value =
                        PMEM_XCALLOC(sizeof(DB_WCHAR) * (field->length_ + 1));
            else
                r_value = PMEM_XCALLOC((field->length_ + 1));

            if (r_value == NULL)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
                return SQL_E_OUTOFMEMORY;
            }
            memcpy_func[field->type_] (r_value, (record + field->h_offset_),
                    field->length_);
        }
    }

    ret = dbi_like_compare(r_value, field->type_, field->fixed_part,
            (void *) field->value_.pointer_, field->mode_, field->collation);

    if (r_value && (r_value != record + field->h_offset_))
        PMEM_FREE(r_value);

    return ret;
}

/*****************************************************************************/
//! compare_CHAR 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param field(in)     : field's value
 * \param record(in)    : heap record's value
 * \param recSize(in)   : size of record's value
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - compare_BYTES -> compare_CHAR
 *  - collation 함수 사용
 *****************************************************************************/
static int
compare_CHAR(struct FieldValue *field, char *record, DB_INT32 recSize)
{
    int ret;
    char *r_value;

    if (field->mode_ == LIKE || field->mode_ == MDB_ILIKE)
    {
        return compare_LIKESTRING(field, record, recSize);
    }

    r_value = record + field->h_offset_;

    if (IS_REVERSE(field->collation))
    {
        ret = mdb_strnrcmp2(field->value_.pointer_, field->length_,
                r_value, field->length_, field->length_);
    }
    else
    {
        ret = sc_ncollate_func[field->collation] (field->value_.pointer_,
                r_value, field->length_);
    }

    if (ret == 0 && field->length_ < field->value_length_)
    {
        return 1;
    }

    return ret;
}

/*****************************************************************************/
//! compare_NCHAR 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param field(in)     : field's value
 * \param record(in)    : heap record's value
 * \param recSize(in)   : size of record's value
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - compare_NBYTES -> compare_NCHAR
 *  - collation 함수 사용
 *****************************************************************************/
static int
compare_NCHAR(struct FieldValue *field, char *record, DB_INT32 recSize)
{
    int ret;

    DB_WCHAR *r_value;

    if (field->mode_ == LIKE || field->mode_ == MDB_ILIKE)
        return compare_LIKESTRING(field, record, recSize);

    r_value = (DB_WCHAR *) (record + field->h_offset_);

    ret = sc_ncollate_func[field->collation] (
            (char *) field->value_.pointer_, (char *) r_value, field->length_);

    if (ret == 0 && field->value_length_ > field->length_)
        return 1;

    return ret;
}

/*****************************************************************************/
//! compare_VARCHAR 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param field(in)     : field's value
 * \param record(in)    : heap record's value
 * \param recSize(in)   : size of record's value
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - record format reference : _Schema_CheckFieldDesc()
 *  - compare_STRING -> compare_VARCHAR
 *  - varchar format이 변경
 *  - varchar의 경우는 SizeOfType[field->type_])가 항상 1이다
 *  - left 와 right를 비교함수의 리턴값(left - right를 리턴함)
 *      left가 right보다 큰 경우 1을 리턴함
 *      left가 right보다 작은 경우 -1을 리턴함
 *  
 *****************************************************************************/
static int
compare_VARCHAR(struct FieldValue *field, char *record, DB_INT32 recSize)
{
    char *r_value;
    int ret;
    char *pbuf = NULL;

    if (field->mode_ == LIKE || field->mode_ == MDB_ILIKE)
        return compare_LIKESTRING(field, record, recSize);

    r_value = record + field->h_offset_;

#if defined(MDB_DEBUG)
    if (strlen_func[field->type_] (field->value_.pointer_) !=
            field->value_length_)
    {
        sc_assert(0, __FILE__, __LINE__);
    }
#endif

    if (IS_VARIABLE_DATA(field->type_, field->fixed_part))
    {
        DB_FIX_PART_LEN blen;

        blen = *((DB_FIX_PART_LEN *) (r_value + field->fixed_part));

        if (blen < MDB_UCHAR_MAX)
        {   // 1. not extended
            ret = sc_ncollate_func[field->collation] (field->value_.pointer_,
                    r_value, field->fixed_part);
            if (ret == 0 && field->value_length_ > field->fixed_part)
            {
                ret = 1;
            }
        }
        else
        {
            // 1. heap part
            if (field->type_ == DT_VARCHAR &&
                    field->collation == MDB_COL_CHAR_DICORDER)
            {
                ret = mdb_strncasecmp(field->value_.pointer_,
                        r_value + OID_SIZE, field->fixed_part - OID_SIZE);
            }
            else
                ret = sc_ncollate_func[field->collation] (field->value_.
                        pointer_, r_value + OID_SIZE,
                        field->fixed_part - OID_SIZE);
            // 2. extended part
            if (ret == 0)
            {
                OID exted;
                int issys1 = TRUE;
                DB_EXT_PART_LEN exted_blen;

                if (field->type_ == DT_VARCHAR &&
                        field->collation == MDB_COL_CHAR_DICORDER)
                {
                    char *tr_value = PMEM_ALLOC(field->length_ + 1);

                    if (tr_value == NULL)
                    {
                        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                                SQL_E_OUTOFMEMORY, 0);
                        return SQL_E_OUTOFMEMORY;
                    }
                    dbi_strcpy_variable(tr_value, r_value,
                            field->fixed_part, field->type_, field->length_);
                    ret = sc_ncollate_func[field->collation] (field->value_.
                            pointer_, tr_value, field->length_);
                    PMEM_FREENUL(tr_value);

                    if (ret == 0 && field->value_length_ > field->length_)
                        ret = 1;
                }
                else
                {
                    // OID Setting
                    sc_memcpy(&exted, r_value, OID_SIZE);

                    SetBufSegment(issys1, exted);

                    // OID Setting
                    pbuf = (char *) OID_Point(exted);
                    if (pbuf == NULL)
                    {
                        UnsetBufSegment(issys1);
                        return DB_E_OIDPOINTER;
                    }

                    sc_memcpy(&exted_blen, pbuf, MDB_EXT_LEN_SIZE);
                    ret = sc_ncollate_func[field->collation] (field->value_.
                            pointer_ + (field->fixed_part - OID_SIZE),
                            pbuf + MDB_EXT_LEN_SIZE, exted_blen);

                    UnsetBufSegment(issys1);
                    if (ret == 0 && field->value_length_ >
                            field->fixed_part - OID_SIZE + exted_blen)
                        ret = 1;

                }
            }
        }
    }
    else
    {
        ret = sc_ncollate_func[field->collation] (field->value_.pointer_,
                r_value, field->length_);
    }

    if (ret == 0 && field->value_length_ > field->length_)
        return 1;

    return ret;
}

/*****************************************************************************/
//! compare_NVARCHAR 
/*! \breif  
 *         
 ************************************
 * \param field(in)     : field's value
 * \param record(in)    : heap record's value
 * \param recSize(in)   : size of record's value
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *  - compare_NSTRING -> compare_NVARCHAR
 *  - collation 함수 사용
 *  - SizeOfType[field->type_] 대신 sizeof(DB_WCHAR)를 사용
 *      (속도를 위해서)
 *  - left 와 right를 비교함수의 리턴값(left - right를 리턴함)
 *      left가 right보다 큰 경우 1을 리턴함
 *      left가 right보다 작은 경우 -1을 리턴함
 *****************************************************************************/
static int
compare_NVARCHAR(struct FieldValue *field, char *record, DB_INT32 recSize)
{
    char *r_value;
    int ret;
    char *pbuf = NULL;

    if (field->mode_ == LIKE || field->mode_ == MDB_ILIKE)
        return compare_LIKESTRING(field, record, recSize);

    r_value = record + field->h_offset_;

#if defined(MDB_DEBUG)
    if (strlen_func[field->type_] (field->value_.pointer_) !=
            field->value_length_)
    {
        sc_assert(0, __FILE__, __LINE__);
    }
#endif

    {
        if (IS_VARIABLE_DATA(field->type_, field->fixed_part))
        {
            DB_FIX_PART_LEN blen;

            blen = *((DB_FIX_PART_LEN *) (r_value + field->fixed_part));

            if (blen < MDB_UCHAR_MAX)
            {   // 1. not extended
                ret = sc_ncollate_func[field->collation] (field->value_.
                        pointer_, r_value,
                        field->fixed_part / sizeof(DB_WCHAR));
                /* fixedpart means byte size */
                if (ret == 0
                        && field->value_length_ >
                        field->fixed_part / WCHAR_SIZE)
                    ret = 1;
            }
            else
            {
                // 1. heap part
                ret = sc_ncollate_func[field->collation] (field->value_.
                        pointer_, r_value + OID_SIZE,
                        (field->fixed_part - OID_SIZE) / sizeof(DB_WCHAR));
                // 2. extended part
                if (ret == 0)
                {
                    OID exted;
                    int issys1 = 0;
                    DB_EXT_PART_LEN exted_blen;

                    sc_memcpy(&exted, r_value, OID_SIZE);

                    SetBufSegment(issys1, exted);

                    pbuf = (char *) OID_Point(exted);
                    if (pbuf == NULL)
                    {
                        UnsetBufSegment(issys1);
                        return DB_E_OIDPOINTER;
                    }
                    sc_memcpy(&exted_blen, pbuf, MDB_EXT_LEN_SIZE);

                    ret = sc_ncollate_func[field->collation] (field->value_.
                            pointer_ + (field->fixed_part - OID_SIZE),
                            pbuf + MDB_EXT_LEN_SIZE,
                            (exted_blen / sizeof(DB_WCHAR)));

                    UnsetBufSegment(issys1);

                    if (ret == 0 && field->value_length_ >
                            (field->fixed_part - OID_SIZE + exted_blen)
                            / sizeof(DB_WCHAR))
                        ret = 1;
                }
            }
        }
        else
        {
            ret = sc_ncollate_func[field->collation] (field->value_.pointer_,
                    r_value, field->length_);
        }
    }

    if (ret == 0 && field->value_length_ > field->length_)
        return 1;

    return ret;
}

/*****************************************************************************/
//! compare_DATETIME 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param field(in)     : field's value
 * \param record(in)    : heap record's value
 * \param recSize(in)   : size of record's value
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *****************************************************************************/
static int
compare_DATETIME(struct FieldValue *field, char *record, DB_INT32 recSize)
{
    char *r_value = record + field->h_offset_;

    return sc_memcmp(field->value_.datetime_, r_value, field->length_);
}

/*****************************************************************************/
//! compare_DATE 
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
 * \note 
 *****************************************************************************/
static int
compare_DATE(struct FieldValue *field, char *record, DB_INT32 recSize)
{
    char *r_value = record + field->h_offset_;

    return sc_memcmp(field->value_.date_, r_value, field->length_);
}

/*****************************************************************************/
//! compare_TIME 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param field(in)     : field's value
 * \param record(in)    : heap record's value
 * \param recSize(in)   : size of record's value
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *****************************************************************************/
static int
compare_TIME(struct FieldValue *field, char *record, DB_INT32 recSize)
{
    char *r_value = record + field->h_offset_;

    return sc_memcmp(field->value_.time_, r_value, field->length_);
}

/*****************************************************************************/
//! compare_TIMESTAMP 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param field(in)     : field's value
 * \param record(in)    : heap record's value
 * \param recSize(in)   : size of record's value
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *****************************************************************************/
static int
compare_TIMESTAMP(struct FieldValue *field, char *record, DB_INT32 recSize)
{
    char *r_value = record + field->h_offset_;
    DB_INT64 ll;

    /* 8 byte alignment */
    sc_memcpy(&ll, r_value, sizeof(DB_INT64));

    return (field->value_.bigint_ < ll) ? -1 :
            (ll == field->value_.bigint_) ? 0 : 1;
}

/*****************************************************************************/
//! compare_HEX 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param field(in)     : field's value
 * \param record(in)    : heap record's value
 * \param recSize(in)   : size of record's value
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *****************************************************************************/
static int
compare_HEX(struct FieldValue *field, char *record, DB_INT32 recSize)
{
    return DB_FAIL;
}

/*****************************************************************************/
//! compare_DECIMAL 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param field(in)     : field's value
 * \param record(in)    : heap record's value
 * \param recSize(in)   : size of record's value
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *****************************************************************************/
static int
compare_DECIMAL(struct FieldValue *field, char *record, DB_INT32 recSize)
{
    char *r_value = record + field->h_offset_;
    int result;
    char *r1 = field->value_.pointer_;
    char *r2 = r_value;

    if (r1[0] != r2[0])
    {
        if (r1[0] == '-')
            return -1;
        else
            return 1;
    }

    result = mdb_strcmp(r1 + 1, r2 + 1);

    if (r1[0] == '-')
        result = -result;

    return result;
}

/*****************************************************************************/
//! compare_BYTE
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param field(in)     : field's value
 * \param record(in)    : heap record's value
 * \param recSize(in)   : size of record's value
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - 함수 추가
 *  - BYTE TYPE에는 COLLATION이 존재하지 않는다
 *****************************************************************************/
static int
compare_BYTE(struct FieldValue *field, char *record, DB_INT32 recSize)
{
    int ret;
    char *r_value;

    sc_assert(0, __FILE__, __LINE__);

    if (field->mode_ == LIKE || field->mode_ == MDB_ILIKE)
        return compare_LIKESTRING(field, record, recSize);

    r_value = record + field->h_offset_;

    ret = sc_memcmp(field->value_.pointer_, r_value, field->length_);
    if (ret == 0 && field->length_ < field->value_length_)
        return 1;

    return ret;
}

/*****************************************************************************/
//! compare_VARBYTE
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param field(in)     : field's value
 * \param record(in)    : heap record's value
 * \param recSize(in)   : size of record's value
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - record format reference : _Schema_CheckFieldDesc()
 *  - heap record format 변경
 *  - BYTE TYPE에는 COLLATION이 존재하지 않는다
 *  - SizeOfType[field->type_] 의 경우 항상 값이 1이다
 *      (속도를 위해서)
 *  - left 와 right를 비교함수의 리턴값(left - right를 리턴함)
 *      left가 right보다 큰 경우 1을 리턴함
 *      left가 right보다 작은 경우 -1을 리턴함
 *****************************************************************************/
static int
compare_VARBYTE(struct FieldValue *field, char *record, DB_INT32 recSize)
{
    char *r_value;
    int ret;
    char *pbuf = NULL;

    if (field->mode_ == LIKE || field->mode_ == MDB_ILIKE)
        return compare_LIKESTRING(field, record, recSize);

    r_value = record + field->h_offset_;

#if defined(MDB_DEBUG)
    if (strlen_func[field->type_] (field->value_.pointer_) !=
            field->value_length_)
        sc_assert(0, __FILE__, __LINE__);
#endif

    if (IS_VARIABLE_DATA(field->type_, field->fixed_part))
    {
        DB_FIX_PART_LEN blen;

        blen = *((DB_FIX_PART_LEN *) (r_value + field->fixed_part));

        if (blen < MDB_UCHAR_MAX)
        {   // 1. not extended
            ret = strncmp_func[field->type_] (field->value_.pointer_ + 4,
                    r_value, field->fixed_part);
            if (ret == 0 && field->value_length_ > field->fixed_part)
                ret = 1;
        }
        else
        {
            // 1. heap part
            ret = strncmp_func[field->type_] (field->value_.pointer_ + 4,
                    r_value + OID_SIZE, field->fixed_part - OID_SIZE);
            // 2. extended part
            if (ret == 0)
            {
                OID exted;
                int issys1 = 0;
                DB_EXT_PART_LEN exted_blen;

                // OID Setting
                sc_memcpy(&exted, r_value, OID_SIZE);

                SetBufSegment(issys1, exted);

                pbuf = (char *) OID_Point(exted);
                if (pbuf == NULL)
                {
                    UnsetBufSegment(issys1);
                    return DB_E_OIDPOINTER;
                }
                sc_memcpy(&exted_blen, pbuf, MDB_EXT_LEN_SIZE);
                ret = strncmp_func[field->type_] (field->value_.pointer_ + 4
                        + (field->fixed_part - OID_SIZE),
                        pbuf + MDB_EXT_LEN_SIZE, exted_blen);

                UnsetBufSegment(issys1);

                if (ret == 0 && field->value_length_ >
                        field->fixed_part - OID_SIZE + exted_blen)
                    ret = 1;
            }
        }
    }
    else
    {
        ret = strncmp_func[field->type_] (field->value_.pointer_,
                r_value, field->length_);
    }

    if (ret == 0 && field->value_length_ > field->length_)
        return 1;

    return ret;
}

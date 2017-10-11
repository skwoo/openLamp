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
#include "mdb_FieldVal.h"
#include "mdb_syslog.h"
#include "mdb_PMEM.h"

#include "sql_util.h"

/* Only for numeric type */
static void
cast_numeric_data(DataType in_type, void *value,
        DataType out_type, union Value *out)
{
    bigint bi = 0;
    double dl = 0;

    if (IS_INTEGER(out_type))
    {
        switch (in_type)
        {
        case DT_BOOL:
        case DT_TINYINT:
            bi = (bigint) * ((char *) value);
            break;
        case DT_SMALLINT:
            bi = (bigint) * ((short *) value);
            break;
        case DT_INTEGER:
            bi = (bigint) * ((int *) value);
            break;
        case DT_BIGINT:
            bi = (bigint) * ((DB_INT64 *) value);
            break;
        case DT_FLOAT:
            bi = (bigint) * ((float *) value);
            break;
        case DT_DOUBLE:
            bi = (bigint) * ((double *) value);
            break;
        case DT_OID:
            bi = (bigint) * ((OID *) value);
            break;
        default:
            bi = 0;
        }
    }
    else    /* real */
    {
        switch (in_type)
        {
        case DT_BOOL:
        case DT_TINYINT:
            dl = (double) *((char *) value);
            break;
        case DT_SMALLINT:
            dl = (double) *((short *) value);
            break;
        case DT_INTEGER:
            dl = (double) *((int *) value);
            break;
        case DT_BIGINT:
            dl = (double) *((DB_INT64 *) value);
            break;
        case DT_FLOAT:
            dl = (double) *((float *) value);
            break;
        case DT_DOUBLE:
            dl = (double) *((double *) value);
            break;
        case DT_OID:
#if defined(_64BIT) && defined(WIN32)
            dl = (double) (DB_INT64) * ((OID *) value);
#else
            dl = (double) *((OID *) value);
#endif
            break;
        default:
            dl = 0;
        }
    }

    switch (out_type)
    {
    case DT_BOOL:
    case DT_TINYINT:
        out->char_ = (char) bi;
        break;
    case DT_SMALLINT:
        out->short_ = (short) bi;
        break;
    case DT_INTEGER:
        out->int_ = (int) bi;
        break;
    case DT_BIGINT:
        out->bigint_ = bi;
        break;
    case DT_FLOAT:
        out->float_ = (float) dl;
        break;
    case DT_DOUBLE:
        out->double_ = dl;
        break;
    case DT_OID:
        out->oid_ = (OID) bi;
        break;
    default:
        /* never get here */
        sc_assert(0, __FILE__, __LINE__);
    }
}

/*****************************************************************************/
//! dbi_FieldValue 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param mode(in)          : mode
 * \param type(in)          : DATA TYPE
 * \param position(in)      : field's number in record
 * \param offset(in)        : field's offset in record
 * \param length(in)        : field's length
 * \param scale(in)         : field's scale(use ONLY DT_DECIMAL type)
 * \param collation(in)     : field's collation
 * \param value(in)         : field's value
 * \param value_type(in)    : field's value type
 * \param value_length(in)  : the length of the field's value 
 * \param fieldval(in/out)  : field  
 ************************************
 * \return  return_value
 ************************************
 * \note 
 *  - 함수 추가
 *  - 아래의 함수들 대신 사용됨
 *      TinyintFieldValue()     -> DT_TINYINT
 *      ShortFieldValue()       -> DT_SMALLINT
 *      IntFieldValue()         -> DT_INTEGER
 *      BigintFieldValue()      -> DT_BIGINT
 *      LongFieldValue()        -> DT_LONG
 *      FloatFieldValue()       -> DT_FLOAT
 *      DoubleFieldValue()      -> DT_DOUBLE
 *      DecimalFieldValue()     -> DT_DECIMAL
 *      StringFieldValue()      -> DT_CHAR, DT_VARCHAR
 *      NStringFieldValue()     -> DT_NCHAR, DT_NVARCHAR
 *      ByteFieldValue()        -> DT_BYTE
 *      VarbyteFieldValue()     -> DT_VARBYTE
 *      TextFieldValue()        -> DT_TEXT
 *      BlobFieldValue()        -> DT_BLOB
 *      TimestampFieldValue()   -> DT_TIMESTAMP
 *      DatetimeFieldValue()    -> DT_DATETIME
 *      DateFieldValue()        -> DT_DATE
 *      TimeFieldValue()        -> DT_TIME
 *
 *****************************************************************************/
int
dbi_FieldValue(int mode, int type, int position, int offset, int length,
        int scale, int collation, void *value, DataType value_type,
        int value_length, /* input */ FIELDVALUE_T * fieldval /* output */ )
{
    sc_memset(fieldval, 0, sizeof(FIELDVALUE_T));
    fieldval->mode_ = (DB_INT8) mode;
    fieldval->type_ = (DataType) type;
    fieldval->position_ = (DB_UINT16) position;
    fieldval->offset_ = (DB_UINT32) offset;
    fieldval->length_ = (DB_UINT32) length;

    switch (type)
    {
    case DT_TINYINT:
        fieldval->value_length_ = (DB_UINT32) length;
        fieldval->collation = MDB_COL_NUMERIC;
        if (value != NULL)
            cast_numeric_data(value_type, value, type, &fieldval->value_);
        break;
    case DT_SMALLINT:
        fieldval->value_length_ = (DB_UINT32) length;
        fieldval->collation = MDB_COL_NUMERIC;
        if (value != NULL)
            cast_numeric_data(value_type, value, type, &fieldval->value_);
        break;
    case DT_INTEGER:
        fieldval->value_length_ = (DB_UINT32) length;
        fieldval->collation = MDB_COL_NUMERIC;
        if (value != NULL)
            cast_numeric_data(value_type, value, type, &fieldval->value_);
        break;
    case DT_BIGINT:
    case DT_OID:
        fieldval->value_length_ = (DB_UINT32) length;
        fieldval->collation = MDB_COL_NUMERIC;
        if (value != NULL)
            cast_numeric_data(value_type, value, type, &fieldval->value_);
        break;
    case DT_FLOAT:
        fieldval->value_length_ = (DB_UINT32) length;
        fieldval->collation = MDB_COL_NUMERIC;
        if (value != NULL)
            cast_numeric_data(value_type, value, type, &fieldval->value_);
        break;
    case DT_DOUBLE:
        fieldval->value_length_ = (DB_UINT32) length;
        fieldval->collation = MDB_COL_NUMERIC;
        if (value != NULL)
            cast_numeric_data(value_type, value, type, &fieldval->value_);
        break;
    case DT_DECIMAL:
        fieldval->value_length_ = (DB_UINT32) (length + 3);
        fieldval->scale_ = (DB_INT8) scale;
        fieldval->collation = MDB_COL_NUMERIC;
        if (length == 0 || value == NULL)
        {
            fieldval->value_.decimal_ = value;
        }
        else
        {
            if ((int) sc_strlen((char *) value) >= length + 3)
                length++;
            fieldval->value_.decimal_ = (char *) PMEM_ALLOC(length + 3);
            sc_memcpy(fieldval->value_.pointer_, value, length + 3);
        }
        break;

    case DT_CHAR:
    case DT_VARCHAR:
        fieldval->collation = (MDB_COL_TYPE) collation;

        if (fieldval->collation == MDB_COL_NONE)
            fieldval->collation = DB_Params.default_col_char;

        if (length == 0 || value == NULL)
        {
            fieldval->value_.pointer_ = NULL;
            fieldval->value_length_ = 0;
        }
        else
        {
            fieldval->value_length_ = sc_strlen(value);
            if (fieldval->value_length_ > length)
                length = fieldval->value_length_;

            fieldval->value_.pointer_ = (char *) PMEM_ALLOC(length + 1);
            sc_strncpy(fieldval->value_.pointer_, value,
                    fieldval->value_length_);

            // char의 경우 남는 공간을 모두 0으로 채워주어야함
            if (type == DT_CHAR)
                sc_memset(fieldval->value_.pointer_ + fieldval->value_length_,
                        0, length + 1 - fieldval->value_length_);
        }
        break;
    case DT_NCHAR:
    case DT_NVARCHAR:
        fieldval->collation = (MDB_COL_TYPE) collation;

        if (fieldval->collation == MDB_COL_NONE)
            fieldval->collation = DB_Params.default_col_nchar;

        if (length == 0 || value == NULL)
        {
            fieldval->value_.Wpointer_ = NULL;
            fieldval->value_length_ = 0;
        }
        else
        {
            fieldval->value_length_ = sc_wcslen(value);
            if (fieldval->value_length_ > length)
                length = fieldval->value_length_;

            fieldval->value_.Wpointer_ = PMEM_WALLOC(length + 1);
            if (fieldval->type_ == DT_NVARCHAR)
                sc_wcsncpy(fieldval->value_.Wpointer_, value,
                        fieldval->value_length_);
            else
            {
                sc_wmemcpy(fieldval->value_.Wpointer_, value,
                        fieldval->value_length_);
                sc_wmemset(fieldval->value_.Wpointer_ +
                        fieldval->value_length_, L'\0',
                        length + 1 - fieldval->value_length_);
            }
        }
        break;
    case DT_BYTE:
    case DT_VARBYTE:
        fieldval->collation = (MDB_COL_TYPE) collation;

        if (fieldval->collation == MDB_COL_NONE)
            fieldval->collation = MDB_COL_DEFAULT_BYTE;

        if (length == 0 || value == NULL)
        {
            fieldval->value_.pointer_ = NULL;
            fieldval->value_length_ = 0;
        }
        else
        {
#if defined(MDB_DEBUG)
            // current.. not allow use this
            sc_assert(0, __FILE__, __LINE__);
#else
            fieldval->value_length_ = value_length;
            if (value_length > length)
                length = value_length;

            fieldval->value_.pointer_ = (char *) PMEM_ALLOC(length);
            sc_memcpy(fieldval->value_.pointer_, value, value_length);
#endif
        }
        break;
    case DT_TIMESTAMP:
        fieldval->value_length_ = (DB_UINT32) length;
        if (value != NULL)
            fieldval->value_.timestamp_ = *((DB_INT64 *) value);
        fieldval->collation = MDB_COL_NUMERIC;
        break;
    case DT_DATETIME:
        fieldval->value_length_ = (DB_UINT32) length;
        fieldval->collation = MDB_COL_NUMERIC;
        if (value == NULL)
            fieldval->value_.datetime_[0] = '\0';
        else
            sc_memcpy(fieldval->value_.datetime_, value, 8);
        break;
    case DT_DATE:
        fieldval->value_length_ = (DB_UINT32) length;
        fieldval->collation = MDB_COL_NUMERIC;
        if (value == NULL)
            fieldval->value_.date_[0] = '\0';
        else
            sc_memcpy(fieldval->value_.date_, value, 4);
        break;
    case DT_TIME:
        fieldval->value_length_ = (DB_UINT32) length;
        fieldval->collation = MDB_COL_NUMERIC;
        if (value == NULL)
            fieldval->value_.time_[0] = '\0';
        else
            sc_memcpy(fieldval->value_.time_, value, 4);
        break;
    case DT_HEX:       // char하고 유사한 형태
        return dbi_FieldValue(mode, DT_CHAR, position, offset, length, scale,
                collation, value, value_type, value_length, fieldval);
    default:
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_INVALIDEXPRESSION, 0);
        return DB_FAIL;
    }

    return DB_SUCCESS;
}

struct UpdateValue
dbi_UpdateValue_Create(DB_UINT32 n)
{
    struct UpdateValue updValues;

    if (n < 1)
    {
        updValues.update_field_count = 0;
        return updValues;
    }

    updValues.update_field_value = PMEM_ALLOC(n * sizeof(UPDATEFIELDVALUE_T));
    if (updValues.update_field_value == NULL)
        updValues.update_field_count = 0;
    else
        updValues.update_field_count = n;

    return updValues;
}

int
dbi_UpdateValue_Destroy(struct UpdateValue *updValues)
{
    DB_UINT32 n;

    if (updValues == NULL)
        return DB_SUCCESS;

    if (updValues->update_field_value == NULL ||
            updValues->update_field_count == 0)
    {
        updValues->update_field_count = 0;
        updValues->update_field_value = NULL;
        return DB_SUCCESS;
    }

    for (n = 0; n < updValues->update_field_count; n++)
    {
        if (updValues->update_field_value[n].data != NULL &&
                updValues->update_field_value[n].data !=
                &updValues->update_field_value[n].space[0])
        {
            PMEM_FREENUL(updValues->update_field_value[n].data);
        }
    }

    PMEM_FREENUL(updValues->update_field_value);
    updValues->update_field_count = 0;

    return DB_SUCCESS;
}

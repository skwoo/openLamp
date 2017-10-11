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
#include "mdb_inner_define.h"
#include "mdb_FieldDesc.h"
#include "mdb_FieldVal.h"
#include "mdb_compat.h"
#include "mdb_er.h"
#include "mdb_dbi.h"
#include "mdb_Server.h"

#include "sql_datast.h"

extern int (*sc_ncollate_func[]) (const char *s1, const char *s2, int n);

/*****************************************************************************/
//! KeyFieldDesc_Create
/*! \breif  KEY_FILED 정보 할당
 ************************************
 * \param fieldDesc(in/out) : KEY_FIELDDESC 구조체
 * \param pSysField(in/out) : SYSTEM FILED
 * \param order(in)         : order
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - called : _Schema_MakeKeyDesc()
 *  - BYTE TYPE 지원..(이곳에서는 불필요해 보이긴함..)
 *  - 함수의 interface가 변경됨
 *  - Key를 만들때 index에서 넘어오는 인자가 order와 collation
 *      2가지 정보를 넘겨줘야 하므로...
 *      요렇게 수정함
 *
 *****************************************************************************/
int
KeyFieldDesc_Create(KEY_FIELDDESC_T * fielddesc, SYSFIELD_T * pSysField,
        char order, MDB_COL_TYPE * collation)
{
    fielddesc->type_ = pSysField->type;
    fielddesc->position_ = pSysField->position;
    fielddesc->offset_ = pSysField->offset;
    fielddesc->flag_ = pSysField->flag;
    fielddesc->h_offset_ = pSysField->h_offset;
    fielddesc->fixed_part = pSysField->fixed_part;
    fielddesc->order_ = order;
    if (*collation == MDB_COL_NONE)
    {
        fielddesc->collation = pSysField->collation;
        *collation = pSysField->collation;
    }
    else
        fielddesc->collation = *collation;

    switch (fielddesc->type_)
    {
    case DT_VARCHAR:
    case DT_NVARCHAR:
    case DT_CHAR:
    case DT_NCHAR:
        fielddesc->length_ = pSysField->length;
        break;
    case DT_TINYINT:
        fielddesc->length_ = 1;
        break;
    case DT_SMALLINT:
        fielddesc->length_ = 2;
        break;
    case DT_INTEGER:
    case DT_FLOAT:
    case DT_DATE:
    case DT_TIME:
        fielddesc->length_ = 4;
        break;
    case DT_DOUBLE:
    case DT_BIGINT:
    case DT_OID:
    case DT_DATETIME:
    case DT_TIMESTAMP:
        fielddesc->length_ = 8;
        break;

    case DT_DECIMAL:
        fielddesc->length_ = pSysField->length + 3;
        break;

    case DT_BYTE:
    case DT_VARBYTE:
        return DB_E_INDEX_HAS_BYTE_OR_VARBYTE;
    default:
        break;
    }

    return DB_SUCCESS;
}

///////////////////////////////////////////////////////////////////////
//
// Function Name :
//
///////////////////////////////////////////////////////////////////////

/*****************************************************************************/
//! FieldDesc_Compare
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param fielddesc(in) :
 * \param r1(in)        :
 * \param r2(in)        :
 ************************************
 * \return  return_value
 ************************************
 * \note
 *  - reference the heap record format : _Schema_CheckFieldDesc()
 *  - VARIABLE TYPE이 변경되었음
 *  - collation 함수를 사용하도록 변경함
 *
 *****************************************************************************/
int
FieldDesc_Compare(KEY_FIELDDESC_T * fielddesc, const char *r1, const char *r2)
{
    int result;

    /* must use h_offset_ in storage */
    char *value1;
    char *value2;

    value1 = (char *) (r1 + fielddesc->h_offset_);
    value2 = (char *) (r2 + fielddesc->h_offset_);

    switch (fielddesc->type_)
    {
    case DT_TINYINT:
        result = (*value1 < *value2) ? -1 : (*value1 == *value2) ? 0 : 1;
        break;

    case DT_SHORT:
        {
            short s1, s2;

            s1 = *(short *) value1;
            s2 = *(short *) value2;
            result = (s1 < s2) ? -1 : (s1 == s2) ? 0 : 1;
            break;
        }

    case DT_INTEGER:
        {
            int s1, s2;

            s1 = *(int *) value1;
            s2 = *(int *) value2;

            result = (s1 < s2) ? -1 : (s1 == s2) ? 0 : 1;
            break;
        }

    case DT_BIGINT:
    case DT_TIMESTAMP:
    case DT_OID:
        {
            DB_INT64 b1, b2;

            sc_memcpy(&b1, value1, sizeof(DB_INT64));
            sc_memcpy(&b2, value2, sizeof(DB_INT64));

            result = (b1 < b2) ? -1 : (b1 == b2) ? 0 : 1;
            /* result can be overflowed on int return */
            break;
        }

    case DT_DATETIME:
        {
            result = sc_memcmp(value1, value2, 7);
            break;
        }

    case DT_DATE:
    case DT_TIME:
        {
            result = sc_memcmp(value1, value2, 4);
            break;
        }

    case DT_FLOAT:
        {
            float s1, s2;

            sc_memcpy(&s1, value1, sizeof(float));
            sc_memcpy(&s2, value2, sizeof(float));
            result = (s1 < s2) ? -1 : (s1 == s2) ? 0 : 1;
            break;
        }

    case DT_DOUBLE:
        {
            double s1, s2;

            sc_memcpy(&s1, value1, sizeof(double));
            sc_memcpy(&s2, value2, sizeof(double));
            result = (s1 < s2) ? -1 : (s1 == s2) ? 0 : 1;
            break;
        }

    case DT_VARCHAR:
    case DT_NVARCHAR:
        {
            result = dbi_strncmp_variable(value1, value2, fielddesc);
            break;
        }

    case DT_CHAR:
    case DT_NCHAR:
        {
            if (IS_REVERSE(fielddesc->collation))
            {
                result = mdb_strnrcmp2(value1, fielddesc->length_,
                        value2, fielddesc->length_, fielddesc->length_);
            }
            else
            {
                result = sc_ncollate_func[fielddesc->collation] (value1,
                        value2, fielddesc->length_);
            }
            break;
        }

    case DT_DECIMAL:
        {
            if (value1[0] != value2[0])
            {
                if (value1[0] == '-')
                    result = -1;
                else
                    result = 1;

                break;
            }

            result = mdb_strcmp(value1 + 1, value2 + 1);

            if (value1[0] == '-')
                result = -result;

            break;
        }

    case DT_BYTE:
    case DT_VARBYTE:
        result = 0;
        sc_assert(0, __FILE__, __LINE__);
        break;

    default:
        result = 0;
        break;
    }

    if (fielddesc->order_ == 'D')
        result = -result;

    return result;
}

int
dbi_FieldDesc(char *fieldname, DataType type, DB_UINT32 length,
        DB_INT32 scale, DB_UINT8 flag, void *defaultValue,
        DB_INT32 fixed_part, FIELDDESC_T * fieldDesc, MDB_COL_TYPE collation)
{
    char buf[64];

    fieldDesc->position_ = 0;
    fieldDesc->offset_ = 0;
    fieldDesc->h_offset_ = 0;
    sc_memset(fieldDesc->fieldName, 0, sizeof(fieldDesc->fieldName));
    sc_memset(fieldDesc->defaultValue, 0, sizeof(fieldDesc->defaultValue));
    sc_strcpy(fieldDesc->fieldName, fieldname);
    fieldDesc->type_ = type;
    fieldDesc->length_ = length;
    fieldDesc->scale_ = scale;
    fieldDesc->flag = flag;
    /* 0x1: iskey, 0x2: null_allowed, 0x4: for sysdate default */

    fieldDesc->collation = collation;

    length = DB_BYTE_LENG(type, length);

    if (fixed_part > 0)
    {
        fixed_part = DB_BYTE_LENG(type, fixed_part);
    }

    if (IS_VARIABLE_DATA(type, fixed_part))
    {
        if (isTempField_name(fieldname))
        {
            if (IS_N_STRING(type) && length > TEMPVAR_FIXEDPART + 2 * OID_SIZE)
            {
                fixed_part = TEMPVAR_FIXEDPART;
            }
        }

        if (fixed_part == 0)
        {
            fixed_part = server ? server->shparams.default_fixed_part
                    : FIXED_SIZE_FOR_VARIABLE;

            if (fixed_part + 2 * OID_SIZE > length)
            {
                fixed_part = FIXED_VARIABLE;
            }
        }

        if (fixed_part > 0)
        {
            if (fixed_part > length - 2 * OID_SIZE ||
                    fixed_part < MINIMUM_FIXEDPART)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                        SQL_E_INVALIDFIXEDPART, 0);
                return SQL_E_INVALIDFIXEDPART;
            }

            fieldDesc->fixed_part = (type == DT_NVARCHAR
                    ? _alignedlen(fixed_part, WCHAR_SIZE) : fixed_part);
        }
        else
        {
            fieldDesc->fixed_part = FIXED_VARIABLE;
        }
    }
    else
    {
        fieldDesc->fixed_part = FIXED_VARIABLE;
    }

    if (defaultValue == NULL)
    {
        fieldDesc->defaultValue[0] = '\0';
        DB_DEFAULT_SETNULL(fieldDesc->defaultValue);
    }
    else
    {
        if (fieldDesc->flag & DEF_SYSDATE_BIT)
        {
            // 현 상태에서는 DEFAULT_VALUE_LEN를 넘어가지 않음
            // 'SYSDATE' or 'CURRENT_TIMESTAMP'
            sc_strcpy(fieldDesc->defaultValue, defaultValue);
        }
        else
        {
            switch (fieldDesc->type_)
            {
            case DT_TINYINT:
                sc_sprintf(fieldDesc->defaultValue, "%d",
                        *(char *) defaultValue);
                break;

            case DT_SHORT:
                sc_sprintf(fieldDesc->defaultValue, "%d",
                        *(short *) defaultValue);
                break;

            case DT_INTEGER:
                sc_sprintf(fieldDesc->defaultValue, "%d",
                        *(int *) defaultValue);
                break;

            case DT_BIGINT:
            case DT_OID:
                sc_sprintf(fieldDesc->defaultValue, I64_FORMAT,
                        *(DB_INT64 *) defaultValue);
                break;

            case DT_DATETIME:
                {
                    unsigned char *dt = (unsigned char *) defaultValue;

                    sc_sprintf(fieldDesc->defaultValue,
                            "%02x%02x-%02x-%02x %02x:%02x:%02x",
                            dt[0], dt[1], dt[2], dt[3], dt[4], dt[5], dt[6]);
                    break;
                }

            case DT_DATE:
                {
                    unsigned char *dt = (unsigned char *) defaultValue;

                    sc_sprintf(fieldDesc->defaultValue, "%02x%02x-%02x-%02x",
                            dt[0], dt[1], dt[2], dt[3]);
                    break;
                }

            case DT_TIME:
                {
                    unsigned char *dt = (unsigned char *) defaultValue;

                    sc_sprintf(fieldDesc->defaultValue, "%02x:%02x:%02x",
                            dt[0], dt[1], dt[2]);
                    break;
                }

            case DT_TIMESTAMP:
                {
                    DB_INT64 ts;
                    int ms;
                    struct db_tm _tm;

                    ts = *(DB_INT64 *) defaultValue;
                    ms = (int) (ts & 0x000000000000ffff);
                    /* ms: lower 2 bytes */
                    ts = ts >> 16;      /* sec: higher 6 bytes */

                    gmtime_as(&ts, &_tm);
                    localtime_as(&ts, &_tm);

                    sc_sprintf(fieldDesc->defaultValue,
                            "%04d-%02d-%02d %02d:%02d:%02d.%03d",
                            _tm.tm_year + 1900, _tm.tm_mon + 1, _tm.tm_mday,
                            _tm.tm_hour, _tm.tm_min, _tm.tm_sec, ms);
                    break;
                }

            case DT_FLOAT:
                sc_sprintf(buf, "%f", *(float *) defaultValue);
                if (sc_strlen(buf) >= DEFAULT_VALUE_LEN)
                {
                    sc_sprintf(fieldDesc->defaultValue, "%f",
                            *(float *) defaultValue);
                }
                else
                {
                    sc_memcpy(fieldDesc->defaultValue, buf,
                            sc_strlen(buf) + 1);
                }
                break;

            case DT_DOUBLE:
                sc_sprintf(buf, "%f", *(double *) defaultValue);
                if (sc_strlen(buf) >= DEFAULT_VALUE_LEN)
                {
                    sc_sprintf(fieldDesc->defaultValue, "%f",
                            *(double *) defaultValue);
                }
                else
                {
                    sc_memcpy(fieldDesc->defaultValue, buf,
                            sc_strlen(buf) + 1);
                }

                break;

            case DT_DECIMAL:
                length += 3;
                /* fallthrough */

            case DT_VARCHAR:   /* case DT_NVARCHAR: */
            case DT_CHAR:      /* case DT_NCHAR: */
                length = strlen_func[fieldDesc->type_] (defaultValue);
                if (length >= DEFAULT_VALUE_LEN)
                {
                    length = DEFAULT_VALUE_LEN - 1;
                }
                sc_memcpy(fieldDesc->defaultValue, defaultValue, length);
                fieldDesc->defaultValue[length] = '\0';
                break;

            case DT_NVARCHAR:
            case DT_NCHAR:
                length = strlen_func[fieldDesc->type_] (defaultValue)
                        * WCHAR_SIZE;
                if (length >= DEFAULT_VALUE_LEN)
                {
                    length = DEFAULT_VALUE_LEN - WCHAR_SIZE;
                }
                sc_memcpy(fieldDesc->defaultValue, defaultValue, length);
                sc_memset(fieldDesc->defaultValue + length, '\0', WCHAR_SIZE);
                break;

            case DT_BYTE:
            case DT_VARBYTE:
                length += 4;
                if (length >= DEFAULT_VALUE_LEN)
                {
                    length = DEFAULT_VALUE_LEN - 1;
                }

                sc_memcpy(fieldDesc->defaultValue, defaultValue, length);
                break;

            default:
                sc_memset(fieldDesc->defaultValue, 0, DEFAULT_VALUE_LEN);
                break;
            }   /* switch */
        }
    }

    return DB_SUCCESS;
}

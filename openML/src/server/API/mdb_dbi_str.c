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

char *
sc_wstrcpy(char *s1, char *s2)
{
    return (char *) sc_wcscpy((DB_WCHAR *) s1, (DB_WCHAR *) s2);
}

char *
sc_wstrncpy(char *s1, char *s2, int n)
{
    return (char *) sc_wcsncpy((DB_WCHAR *) s1, (DB_WCHAR *) s2, n);
}


int
sc_strlen2(char *str)
{
    return sc_strlen(str);
}

int
sc_strlen_byte(char *str)
{
    int len;

#ifdef MDB_DEBUG
    if (!str)   // 디버깅용임
        sc_assert(0, __FILE__, __LINE__);
#endif

    sc_memcpy(&len, str, 4);
    return len;
}


int
sc_strcmp2(const char *s1, const char *s2)
{
    return mdb_strcmp(s1, s2);
}

int
sc_strncmp2(const char *s1, const char *s2, int n)
{
    return mdb_strncmp(s1, s2, n);
}


char *
sc_strncpy2(char *s1, const char *s2, int n)
{
    return sc_strncpy(s1, s2, n);
}

char *
sc_strncpy_byte(char *s1, const char *s2, int n)
{
    return ((char *) sc_memcpy(s1, s2, n));
}

void *
sc_memcpy2(void *mem1, void *mem2, size_t len)
{
    return sc_memcpy(mem1, mem2, len);
}

char *
sc_strstr2(const char *haystack, const char *needle)
{
    return sc_strstr(haystack, needle);
}

char *
sc_wcsstr2(const char *haystack, const char *needle)
{
    return (char *) sc_wcsstr((DB_WCHAR *) haystack, (DB_WCHAR *) needle);
}

/*****************************************************************************/
//! dbi_strncpy_variable
/*! \breif VARCHAR/NVARCHAR의 값을 가져옴
  *        FIXED와 EXTENDED 값을 하나로 합쳐서 가져옴
 ************************************
 * \param dest(in/out)  : memory format에 맞게끔 넣자
 * \param h_value(in)   : heap record format
 * \param fixed_part(in): byte length
 ************************************
 * \return dest's byte length
 ************************************
 * \note
 *  - this function is called so many times, so ... readability ...
 *  - called : Col_Read_Values(), compare_LIKESTRING()
 *  - reference heap record format : _Schema_CheckFieldDesc()
 *  - dest memory record format이지만,
 *    BYTE/VARBYTE TYPE에서 4 BYTES 를 더 할당할 필요는 없을 것 같다
 *  - 함수 추가
 *  - 기존에는 dest's byte length를 리턴했으나..
 *      함수가 애매 모호해져서, 안쪽에서 변환한다
 *
 *****************************************************************************/
DB_INT32
dbi_strcpy_variable(char *dest, char *h_value, DB_INT32 fixed_part,
        DataType type, int buflen)
{
    char *ptr = dest;
    DB_WCHAR *wPtr = (DB_WCHAR *) dest;
    int value_len = 0;
    DB_FIX_PART_LEN fix_blength;
    char *pbuf = NULL;

    sc_memcpy(&fix_blength, h_value + fixed_part, MDB_FIX_LEN_SIZE);

    if (fix_blength != MDB_UCHAR_MAX)
    {       // read in heap record
        sc_memcpy(dest, h_value, fix_blength);

        if (type == DT_NVARCHAR)
        {
            value_len = ((int) fix_blength / sizeof(DB_WCHAR));
            wPtr[value_len] = 0x00;
        }
        else
        {
            value_len = (int) fix_blength;
            ptr[value_len] = 0x00;
        }
        goto end;
    }
    else
    {
        OID exted;
        DB_EXT_PART_LEN ext_blength;
        int issys1 = 0;

        // 1. get OID
        sc_memcpy((char *) &exted, h_value, OID_SIZE);

        // 2. copy in fixed part

        /* fix_blength가 MDB_UCHAR_MAX로 설정되어 있는데도
         * exted가 NULL_OID 또는 잘못된 경우 방어코드 추가...
         * 이 경우 fix_blength 부분인 h_value_[fixed_part]를 0으로 처리를 하고
         * 필드값도 null-string으로 처리를 한다.
         */
        if (exted == NULL_OID || isInvalidRid(exted))
        {
            /* memset 0 */
            dest[0] = dest[1] = '\0';
            sc_memset(h_value, 0, fixed_part + MDB_FIX_LEN_SIZE);
            return 0;
        }

        sc_memcpy(dest, h_value + OID_SIZE, fixed_part - OID_SIZE);
        h_value += fixed_part - OID_SIZE;
        dest += fixed_part - OID_SIZE;

        // 3. get extened byte length
        pbuf = (char *) OID_Point(exted);
        if (pbuf == NULL)
        {
            value_len = DB_E_OIDPOINTER;
            goto end;
        }
        sc_memcpy(&ext_blength, pbuf, MDB_EXT_LEN_SIZE);

        SetBufSegment(issys1, exted);

#ifdef MDB_DEBUG
        sc_assert(ext_blength < S_PAGE_SIZE, __FILE__, __LINE__);
        sc_assert(ext_blength <= (buflen - (fixed_part - OID_SIZE) + 2),
                __FILE__, __LINE__);
#endif

        // 4. copy in extened part
        if (ext_blength <= 0 || ext_blength >= S_PAGE_SIZE ||
                ext_blength > (buflen - (fixed_part - OID_SIZE) + 2))
        {
            /* memset 0 */
            h_value -= (fixed_part - OID_SIZE);
            dest -= (fixed_part - OID_SIZE);
            dest[0] = dest[1] = '\0';
            UnsetBufSegment(issys1);
            return 0;
        }
        else
        {
            pbuf = (char *) OID_Point(exted);
            if (pbuf == NULL)
            {
                UnsetBufSegment(issys1);
                value_len = DB_E_OIDPOINTER;
                goto end;
            }
            sc_memcpy(dest, pbuf + MDB_EXT_LEN_SIZE, ext_blength);
        }

#ifdef CHECK_VARCHAR_EXTLEN
        if (type == DT_VARCHAR)
        {
            sc_assert(dest[0] != '\0', __FILE__, __LINE__);
            sc_assert(sc_strlen(dest) + 1 == ext_blength, __FILE__, __LINE__);
        }
#endif

        UnsetBufSegment(issys1);

        if (type == DT_NVARCHAR)
        {
            /* the last extended var record has a null character */
            value_len =
                    ((ext_blength + fixed_part -
                            OID_SIZE) / sizeof(DB_WCHAR)) - 1;
            wPtr[value_len] = 0x00;
        }
        else if (type == DT_VARCHAR)
        {
            /* the last extended var record has a null character */
            value_len = (ext_blength + fixed_part - OID_SIZE) - 1;
            ptr[value_len] = 0x00;
        }
        else if (type == DT_VARBYTE)
        {
            value_len = (ext_blength + fixed_part - OID_SIZE);
            ptr[value_len] = 0x00;
        }
        else
            sc_assert(0, __FILE__, __LINE__);

        goto end;
    }

#if defined(MDB_DEBUG)
    sc_assert(0, __FILE__, __LINE__);
#endif

    value_len = DB_FAIL;

  end:

    return value_len;
}

/*****************************************************************************/
//! dbi_strncmp_variable
/*! \breif
 ************************************
 * \param dest(in/out)  : memory format에 맞게끔 넣자
 * \param h_value(in)   : heap record format
 * \param fielddesc(in)  : field 정보
 ************************************
 * \return dest's byte length
 ************************************
 * \note
 *  - called : FieldDesc_Compare()
 *  - reference heap record format : _Schema_CheckFieldDesc()
 *  - heap record 내부의 특정 field를 비교한다
 *  - left 와 right를 비교함수의 리턴값(left - right를 리턴함)
 *      left가 right보다 큰 경우 1을 리턴함
 *      left가 right보다 작은 경우 -1을 리턴함
 *****************************************************************************/
DB_INT32
dbi_strncmp_variable(char *str1, char *str2, KEY_FIELDDESC_T * fielddesc)
{
    int ret = 0;
    int compare_len;
    char *extend_part;
    DB_EXT_PART_LEN ext_blength;

    DB_INT32 len = fielddesc->length_;
    DB_INT32 fixed_part = fielddesc->fixed_part;
    DataType type = fielddesc->type_;

    MDB_COL_TYPE collation = fielddesc->collation;

    if (fixed_part == FIXED_VARIABLE)
    {       // not variable type
        return sc_ncollate_func[collation] (str1, str2, len);
    }
    else
    {
        DB_FIX_PART_LEN str1_blen, str2_blen;
        int within_len;

        sc_memcpy(&str1_blen, str1 + fixed_part, MDB_FIX_LEN_SIZE);
        sc_memcpy(&str2_blen, str2 + fixed_part, MDB_FIX_LEN_SIZE);

        if (str1_blen < MDB_UCHAR_MAX && str2_blen < MDB_UCHAR_MAX)
        {   // str1 in fixed-part and str2 in fixed-part
            within_len = fixed_part / SizeOfType[type];

            return sc_ncollate_func[collation] (str1, str2, within_len);
        }
        else if (str1_blen < MDB_UCHAR_MAX && str2_blen == MDB_UCHAR_MAX)
        {   // str1 in fixed-part and str2 in extended-part
            {
                OID exted2;

                sc_memcpy(&exted2, str2, OID_SIZE);

                if (exted2 == NULL_OID || isInvalidRid(exted2))
                {
                    /* memset 0 */
                    sc_memset(str2, 0, fixed_part + MDB_FIX_LEN_SIZE);
                }
            }

            // a. compare in fixed part
            within_len = (fixed_part - OID_SIZE) / SizeOfType[type];
            if (fielddesc->type_ == DT_VARCHAR &&
                    fielddesc->collation == MDB_COL_CHAR_DICORDER)
                ret = sc_ncollate_func[MDB_COL_CHAR_NOCASE] (str1,
                        str2 + OID_SIZE, within_len);
            else
                ret = sc_ncollate_func[collation] (str1, str2 + OID_SIZE,
                        within_len);
            if (ret == 0)
            {
                int issys2 = TRUE;
                OID exted2;

                sc_memcpy((char *) &exted2, str2, OID_SIZE);

                SetBufSegment(issys2, exted2);

                extend_part = OID_Point(exted2);
                if (extend_part == NULL)
                {
                    UnsetBufSegment(issys2);
                    return DB_E_OIDPOINTER;
                }
                sc_memcpy(&ext_blength, extend_part, MDB_EXT_LEN_SIZE);

                extend_part = extend_part + MDB_EXT_LEN_SIZE;

                ext_blength = ext_blength / SizeOfType[type];
                ext_blength -= 1;       /* null character */

                str1_blen /= SizeOfType[type];
                str1_blen -= within_len;

                if (str1_blen < ext_blength)
                    compare_len = str1_blen;
                else
                    compare_len = ext_blength;

                ret = sc_ncollate_func[collation] (str1 + (fixed_part -
                                OID_SIZE), extend_part, compare_len);
                if (ret == 0)
                {
                    if (str1_blen > ext_blength)
                        ret = 1;
                    else if (str1_blen < ext_blength)
                        ret = -1;

                }
                UnsetBufSegment(issys2);

            }
        }
        else if (str1_blen == MDB_UCHAR_MAX && str2_blen < MDB_UCHAR_MAX)
        {   // str1 in extended-part and str2 in fixed-part
            OID exted1;

            sc_memcpy(&exted1, str1, OID_SIZE);

            if (exted1 == NULL_OID || isInvalidRid(exted1))
            {
                /* memset 0 */
                sc_memset(str1, 0, fixed_part + MDB_FIX_LEN_SIZE);
            }

            // a. compare in fixed part
            within_len = (fixed_part - OID_SIZE) / SizeOfType[type];
            if (fielddesc->type_ == DT_VARCHAR &&
                    fielddesc->collation == MDB_COL_CHAR_DICORDER)
                ret = sc_ncollate_func[MDB_COL_CHAR_NOCASE]
                        (str1 + OID_SIZE, str2, within_len);
            else
                ret = sc_ncollate_func[collation] (str1 + OID_SIZE, str2,
                        within_len);
            if (ret == 0)
            {
                int issys1 = 0;
                OID exted1;

                sc_memcpy((char *) &exted1, str1, OID_SIZE);

                SetBufSegment(issys1, exted1);

                extend_part = OID_Point(exted1);
                if (extend_part == NULL)
                {
                    UnsetBufSegment(issys1);
                    return DB_E_OIDPOINTER;
                }
                sc_memcpy(&ext_blength, extend_part, MDB_EXT_LEN_SIZE);

                extend_part = extend_part + MDB_EXT_LEN_SIZE;

                ext_blength = ext_blength / SizeOfType[type];
                ext_blength -= 1;       /* null character */

                str2_blen /= SizeOfType[type];
                str2_blen -= within_len;

                if (ext_blength < str2_blen)
                    compare_len = ext_blength;
                else
                    compare_len = str2_blen;

                ret = sc_ncollate_func[collation] (extend_part,
                        str2 + (fixed_part - OID_SIZE), compare_len);
                if (ret == 0)
                {
                    if (ext_blength > str2_blen)
                        ret = 1;
                    else if (ext_blength < str2_blen)
                        ret = -1;
                }
                UnsetBufSegment(issys1);

            }
        }
        else if (str1_blen == MDB_UCHAR_MAX && str2_blen == MDB_UCHAR_MAX)
        {   // str1 in extended-part and str2 in extended-part
            OID exted1, exted2;

            sc_memcpy(&exted1, str1, OID_SIZE);
            sc_memcpy(&exted2, str2, OID_SIZE);

            if (exted1 == NULL_OID || isInvalidRid(exted1))
            {
                /* memset 0 */
                sc_memset(str1, 0, fixed_part + MDB_FIX_LEN_SIZE);
                exted1 = NULL_OID;
            }

            if (exted2 == NULL_OID || isInvalidRid(exted2))
            {
                /* memset 0 */
                sc_memset(str2, 0, fixed_part + MDB_FIX_LEN_SIZE);
                exted2 = NULL_OID;
            }

            if (exted1 == NULL_OID && exted2 == NULL_OID)       /* == */
                return 0;
            else if (exted1 == NULL_OID)        /* str1 < str2 */
                return -1;
            else if (exted2 == NULL_OID)        /* str1 > str2 */
                return 1;

            within_len = (fixed_part - OID_SIZE) / SizeOfType[type];
            if (type == DT_VARCHAR
                    && fielddesc->collation == MDB_COL_CHAR_DICORDER)
            {
                ret = mdb_strncasecmp(str1 + OID_SIZE, str2 + OID_SIZE,
                        within_len);
            }
            else
                ret = sc_ncollate_func[collation] (str1 + OID_SIZE,
                        str2 + OID_SIZE, within_len);
            if (ret == 0)
            {
                if (type == DT_VARCHAR
                        && fielddesc->collation == MDB_COL_CHAR_DICORDER)
                {
                    char *tstr1, *tstr2 = NULL;

                    tstr1 = PMEM_ALLOC(len + 1);
                    if (tstr1 != NULL)
                        tstr2 = PMEM_ALLOC(len + 1);

                    if (tstr1 == NULL || tstr2 == NULL)
                    {
                        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                                DB_E_OUTOFMEMORY, 0);
                        return DB_E_OUTOFMEMORY;
                    }
                    dbi_strcpy_variable(tstr1, str1, fielddesc->fixed_part,
                            type, len);
                    dbi_strcpy_variable(tstr2, str2, fielddesc->fixed_part,
                            type, len);
                    ret = sc_ncollate_func[collation] (tstr1, tstr2, len);

                    PMEM_FREENUL(tstr1);
                    PMEM_FREENUL(tstr2);
                }
                else
                {
                    int issys1;
                    int issys2;
                    OID exted1;
                    OID exted2;
                    char *pbuf1 = NULL, *pbuf2 = NULL;

                    sc_memcpy((char *) &exted1, str1, OID_SIZE);
                    sc_memcpy((char *) &exted2, str2, OID_SIZE);

                    SetBufSegment(issys1, exted1);
                    SetBufSegment(issys2, exted2);

                    pbuf1 = (char *) OID_Point(exted1);
                    pbuf2 = (char *) OID_Point(exted2);
                    if (pbuf1 == NULL || pbuf2 == NULL)
                    {
                        UnsetBufSegment(issys2);
                        UnsetBufSegment(issys1);
                        return DB_E_OIDPOINTER;
                    }
                    ret = sc_ncollate_func[collation] (pbuf1 +
                            MDB_EXT_LEN_SIZE, pbuf2 + MDB_EXT_LEN_SIZE,
                            len - within_len);

                    // I think the code below must be located here.
                    /* Keep the unset ordering, var_rid2 to var_rid1 */
                    UnsetBufSegment(issys2);
                    UnsetBufSegment(issys1);
                }

            }
        }
    }

    return ret;
}

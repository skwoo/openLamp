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

#include "mdb_ucs2vsutf.h"

/* Some fundamental constants */
#define UNI_REPLACEMENT_CHAR (UTF32)0x0000FFFD
#define UNI_MAX_BMP             (UTF32)0x0000FFFF
#define UNI_MAX_UTF16         (UTF32)0x0010FFFF
#define UNI_MAX_UTF32         (UTF32)0x7FFFFFFF
#define UNI_MAX_LEGAL_UTF32  (UTF32)0x0010FFFF

/* isolated surrogates area */
#define UNI_SURROGATE_START  ((UTF32)0xD800)
#define UNI_SURROGATE_END    ((UTF32)0xDFFF)
#define MC_IsSurrogateArea(c) ( ((c) >= UNI_SURROGATE_START) && ((c) <= UNI_SURROGATE_END) )

/* --------------------------------------------------------------------- */
/*
 * Utility routine to tell whether a sequence of bytes is legal UTF-8.
 * If presented with a length > 4, this returns false.  The Unicode
 * definition of UTF-8 goes up to 4-byte sequences.
 */
 /* ---------------------------------------------------------------------
    Legal UTF-8 sequences are:
    Code Points            1st Byte    2nd Byte    3rd Byte    4th Byte 
    U+  0000 ~ U+ 007F     00 ~ 7F       
    U+  0080 ~ U+ 07FF     C2 ~ DF    80 ~ BF     
    U+  0800 ~ U+ 0FFF     E0            A0 ~ BF        80 ~ BF   
    U+  1000 ~ U+ CFFF     E1 ~ EC    80 ~ BF        80 ~ BF   
    U+  D000 ~ U+ D7FF     ED            80 ~ 9F        80 ~ BF   
    U+  D800 ~ U+ DFFF    ill-formed (surrogate 부분이므로 인코딩되어서는 안됨) 
    U+  E000 ~ U+ FFFF     EE ~ EF    80 ~ BF        80 ~ BF   
    U+ 10000 ~ U+3FFFF     F0            90 ~ BF        80 ~ BF        80 ~ BF 
    U+ 40000 ~ U+FFFFF     F1 ~ F3    80 ~ BF        80 ~ BF        80 ~ BF 
    U+100000 ~ U+10FFFF     F4            80 ~ 8F        80 ~ BF        80 ~ BF 

    UCS-4                   UTF-8     
    0x00000000 - 0x0000007F 0xxxxxxx 
    0x00000080 - 0x000007FF 110xxxxx 10xxxxxx 
    0x00000800 - 0x0000FFFF 1110xxxx 10xxxxxx 10xxxxxx 
    0x00010000 - 0x001FFFFF 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx 
    0x00200000 - 0x03FFFFFF 111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 
    0x04000000 - 0x7FFFFFFF 1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 
    --------------------------------------------------------------------- */


/* ------------------------------------------------------------------------------------------------------------------
 *
 * UTF32  vs  UTF-8.
 *   ( Code value )             ( Binary )      ----------------->      ( Binary )
 *     0x0000 -     0x007F   00000000 0xxxxxxx                      0xxxxxxx
 *     0x0080 -     0x07FF   00000xxx yyyyyyyy                      110xxxyy 10yyyyyy  
 *     0x0800 -     0xFFFF   xxxxxxxx yyyyyyyy                      1110xxxx 10xxxxyy 10yyyyyy           // BMP
 * 0x00010000 - 0x0010FFFF   00000000 000x0000 yyyyyyyy zzzzzzzz    11110x00 1000yyyy 10yyyyzz 10zzzzzz  // surrogate
 * 0x00110000 - 0x001FFFFF   00000000 000xxxxx yyyyyyyy zzzzzzzz    11110xxx 10xxyyyy 10yyyyzz 10zzzzzz 
 * 0x00200000 - 0x03FFFFFF   000000ww xxxxxxxx yyyyyyyy zzzzzzzz    111110ww 10xxxxxx 10xxyyyy 10yyyyzz 10zzzzzz  
 * 0x04000000 - 0x7FFFFFFF   0wwwwwww xxxxxxxx yyyyyyyy zzzzzzzz    1111110w 10wwwwww 10xxxxxx 10xxyyyy 10yyyyzz 10zzzzzz  
 * 
 * --------------------------------------------------------------------------------------------------------------------- */

/*
 * Once the bits are split out into bytes of UTF-8, this is a mask OR-ed
 * into the first byte, depending on how many bytes follow.  There are
 * as many entries in this table as there are UTF-8 sequence types.
 * (I.e., one byte sequence, two byte... etc.). Remember that sequencs
 * for *legal* UTF-8 will be 4 or fewer bytes total.
 */

/* -----------------------------------------------------------------
 *  (특정값 v &  0x000000BF) | 0x00000080 = 10xxxxxx(binary)가 된다.
 *
 *  UTF8 단어의 길이에 따라서 첫번째 바이트에 올수있는 특정 패턴이 정해지며
 *  이는 gl_firstByteMark[]에 정의 되어 있다.
  --------------------------------------------------------------------*/

#define MC_GETUTF8_BYTESEQUENCE_10XXXXXX(v) ( ((v)& 0xBF)| 0x80 )
/* --------------------------------------------------------------------- */


#define conversionOK     (0)    /* conversion successful */
#define sourceoverbmp    (-1)   /* over bmp */
#define sourceIllegal    (-2)   /* source sequence is illegal/malformed */
#define targetExhausted  (-3)   /* insuff. room in target for conversion */


/* ----------------------------------------------------------------------------------------------------
  Magic value를 사용한 복원 방법

  아래 알고리즘에서 offsetsFromUTF8[]의 내용은 다음과 같은 원리에 의한다.
  예를 들어 2바이트로 표현된 UTF8을 복원한다고 가정할때 

  본래의 값 u32를 6bit씩 나누어 같고 특정 비트를 설정했었다.
  따라서 그 값을 복원하는 방법으로 6비트씩 쉬프트를 한것이다.
  이건 이해가 쉽다.
  이때 offsetsFromUTF8[]은 특정비트를 설정했었던 부분을 제거 하기 위한것이다.
  바이너리로 표현해서 시도해보면 이해가 쉽다.

 static const UTF32 offsetsFromUTF8[7] = { 
    0x00000000UL, 0x00000000UL, 0x00003080UL, 0x000E2080UL, 0x03C82080UL, 0xFA082080UL, 0x82082080UL };

            nUniCodeCnt = MC_GetUTF8ByteSize(*p);

            u32 = 0;
            switch (nUniCodeCnt) 
            {
            case 6: 
                u32 += *p++; 
                u32 <<= 6;
            case 5: 
                u32 += *p++; 
                u32 <<= 6;
            case 4: 
                u32 += *p++; 
                u32 <<= 6;
            case 3: 
                u32 += *p++; 
                u32 <<= 6;
            case 2: 
                u32 += *p++; 
                u32 <<= 6;
            case 1: 
                u32 += *p++;
                break;
            default:
                return sourceIllegal;
            }
            
            u32 -= offsetsFromUTF8[nUniCodeCnt];

-------------------------------------------------------------------------------------------------------- */


DB_BOOL
Convert_isLegalUTF8(const UTF8 * source)
{
    const UTF8 *p = source;

    while (*p)
    {
        switch (MC_GetUTF8ByteSize(*p))
        {
        case 1:
            p++;
            break;
        case 2:
            if ((*(p + 1) >> 6) != 0x02)
                return FALSE;
            p += 2;
            break;
        case 3:
            if ((*(p + 1) >> 6) != 0x02 || (*(p + 2) >> 6) != 0x02)
            {
                return FALSE;
            }

            p += 3;
            break;
        default:
            return FALSE;       // UCS2즉 BMP판이 아니다.                
        }
    }

    return TRUE;
}


int
Convert_UCS2toUTF8(UCS2 * source, UTF8 * dest, int *pdestsize)
{
    int i, targetBufSize;
    UCS2 *p = source;
    UTF32 u32;
    int bytesToWrite = 0;
    const UTF8 szFirstByteMark[7] =
            { 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };

    if (!dest)
    {
        targetBufSize = 1;      // null 문자를 포함한 길이를 구해준다.
        while (*p)
        {
            u32 = (UTF32) * p;
            if (u32 < (UTF32) 0x80)
            {
                targetBufSize++;
            }
            else if (u32 < (UTF32) 0x800)
            {
                targetBufSize += 2;
            }
            else if (u32 < (UTF32) 0x10000)
            {
                targetBufSize += 3;
            }
            else        // over UNI_MAX_BMP
            {
                return sourceoverbmp;
            }

            p++;
        }

        *pdestsize = targetBufSize;
        return conversionOK;
    }

    // conversion string
    i = 0;
    targetBufSize = *pdestsize;

    while (*p)
    {
        u32 = (UTF32) * p++;
        if (u32 < (UTF32) 0x80)
        {
            bytesToWrite = 1;
        }
        else if (u32 < (UTF32) 0x800)
        {
            bytesToWrite = 2;
        }
        else if (u32 < (UTF32) 0x10000)
        {
            bytesToWrite = 3;
        }
        else    // over UNI_MAX_BMP
        {
            return sourceoverbmp;
        }

        i += bytesToWrite;
        if (i >= targetBufSize)
        {
            return targetExhausted;
        }

        switch (bytesToWrite)
        {   /* note: everything falls through. */
        case 3:
            dest[--i] = (UTF8) MC_GETUTF8_BYTESEQUENCE_10XXXXXX(u32);
            u32 >>= 6;
        case 2:
            dest[--i] = (UTF8) MC_GETUTF8_BYTESEQUENCE_10XXXXXX(u32);
            u32 >>= 6;
        case 1:
            dest[--i] = (UTF8) (u32 | szFirstByteMark[bytesToWrite]);
        }

        i += bytesToWrite;
    }

    dest[i] = (UTF8) 0x00;
    *pdestsize = i + 1;
    return conversionOK;
}


int
Convert_UTF8toUCS2(UTF8 * source, UCS2 * dest, int *pdestsize)
{
    int i, nBytes, targetBufSize;
    const UTF8 *p = source;
    UTF32 u32;

    if (!dest)
    {
        targetBufSize = 1;
        while (*p)
        {
            nBytes = MC_GetUTF8ByteSize(*p);

            u32 = (UTF32) * p;
            switch (nBytes)
            {
            case 1:
                break;
            case 2:    // 110xxxxx 10xxxxxx
                u32 &= 0x1F;
                break;
            case 3:    // 1110xxxx 10xxxxxx 10xxxxxx
                u32 &= 0x0F;
                break;
            default:   // over UNI_MAX_BMP
                return sourceoverbmp;
            }

            p++;
            while (--nBytes > 0)
            {
                u32 <<= 6;
                u32 |= (*p & 0x3f);
                p++;
            }

            if (u32 <= UNI_MAX_BMP)
            {
                targetBufSize++;
            }
            else
            {
                return sourceoverbmp;
            }
        }

        *pdestsize = targetBufSize;
        return conversionOK;
    }


    // conversion string
    i = 0;
    targetBufSize = *pdestsize;

    while (*p)
    {
        nBytes = MC_GetUTF8ByteSize(*p);

        u32 = *p;
        switch (nBytes)
        {
        case 1:
            break;
        case 2:        // 110xxxxx 10xxxxxx
            u32 &= 0x1F;
            break;
        case 3:        // 1110xxxx 10xxxxxx 10xxxxxx
            u32 &= 0x0F;
            break;
        default:
            return sourceIllegal;
        }

        p++;
        while (--nBytes > 0)
        {
            u32 <<= 6;
            u32 |= (*p & 0x3f);
            p++;
        }

        if (i >= targetBufSize)
        {
            return targetExhausted;
        }

        if (u32 <= UNI_MAX_BMP)
        {
            dest[i++] = (UCS2) u32;     /* normal case */
        }
        else
        {
            return sourceoverbmp;
        }
    }

    dest[i] = (UCS2) 0x00;
    *pdestsize = i + 1;
    return conversionOK;
}


__DECL_PREFIX int
MDB_UCS2toUTF8(UCS2 * source, UTF8 ** dest, int *pdestsize)
{
    int nRet, nLen;

    if (!source)
    {
        return IRS_CONVERT_SOURCE1;
    }
    else if (dest == 0x00)
    {
        nRet = Convert_UCS2toUTF8(source, 0x00, &nLen);
    }
    else if (*dest == 0x00)
    {
        nRet = Convert_UCS2toUTF8(source, 0x00, &nLen);
        if (nRet == conversionOK)
        {
            *dest = (UTF8 *) sc_malloc(nLen);
            if (*dest == 0x00)
            {
                return IRS_CONVERT_ALLOC1;
            }

            nRet = Convert_UCS2toUTF8(source, *dest, &nLen);
        }

        if (nRet != conversionOK)
        {
            sc_free(*dest);
            *dest = 0x00;
        }
    }
    else if (pdestsize && *pdestsize > 0)
    {
        nLen = *pdestsize;
        nRet = Convert_UCS2toUTF8(source, *dest, &nLen);
    }
    else
    {
        return IRS_CONVERT_INVALIDPARAM1;
    }

    if (nRet == conversionOK)
    {
        if (pdestsize)
        {
            *pdestsize = nLen;
        }

        return DB_SUCCESS;
    }

    switch (nRet)
    {
    case sourceoverbmp:
        return IRS_CONVERT_OVERBMP1;
    case sourceIllegal:
        return IRS_CONVERT_ILLEGAL1;
    case targetExhausted:
        return IRS_CONVERT_DESTSIZE1;
    default:
        return IRS_CONVERT_UNKNOWN1;
    }
}

__DECL_PREFIX int
MDB_UTF8toUCS2(UTF8 * source, UCS2 ** dest, int *pdestsize)
{
    int nRet, nLen;

    if (!source)
    {
        return IRS_CONVERT_SOURCE2;
    }
    else if (dest == 0x00)
    {
        nRet = Convert_UTF8toUCS2(source, 0x00, &nLen);
    }
    else if (*dest == 0x00)
    {
        nRet = Convert_UTF8toUCS2(source, 0x00, &nLen);
        if (nRet == conversionOK)
        {
            *dest = (UCS2 *) sc_malloc(nLen * sizeof(UCS2));
            if (*dest == 0x00)
            {
                return IRS_CONVERT_ALLOC2;
            }

            nRet = Convert_UTF8toUCS2(source, *dest, &nLen);
        }

        if (nRet != conversionOK)
        {
            sc_free(*dest);
            *dest = 0x00;
        }
    }
    else if (pdestsize && *pdestsize > 0)
    {
        nLen = *pdestsize;
        nRet = Convert_UTF8toUCS2(source, *dest, &nLen);
    }
    else
    {
        return IRS_CONVERT_INVALIDPARAM2;
    }

    if (nRet == conversionOK)
    {
        if (pdestsize)
        {
            *pdestsize = nLen;
        }

        return DB_SUCCESS;
    }

    switch (nRet)
    {
    case sourceoverbmp:
        return IRS_CONVERT_OVERBMP2;
    case sourceIllegal:
        return IRS_CONVERT_ILLEGAL2;
    case targetExhausted:
        return IRS_CONVERT_DESTSIZE2;
    default:
        return IRS_CONVERT_UNKNOWN2;
    }
}

DB_BOOL
mdb_IsLegalUTF8(const UTF8 * source)
{
    return Convert_isLegalUTF8(source);
}

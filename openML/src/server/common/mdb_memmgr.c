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
#include "mdb_typedef.h"
#include "mdb_PMEM.h"

#define HEAP_UNIT_SIZE_MB   32
#define HEAP_UNIT_LOW_BITS  25
#define HEAP_UNIT_LOW_OFFSET 0x01ffffff

#ifdef linux
#define DMEM_UNIT_SIZE_MB    32
#define DMEM_UNIT_LOW_BITS    25
#define DMEM_UNIT_LOW_OFFSET 0x01ffffff
#else
      /* !linux */
#define DMEM_UNIT_SIZE_MB    128
#define DMEM_UNIT_LOW_BITS    27
#define DMEM_UNIT_LOW_OFFSET 0x07ffffff
#endif

#define HEAP_UNIT_SIZE        (HEAP_UNIT_SIZE_MB * 1024 * 1024)
#define DMEM_UNIT_SIZE        (DMEM_UNIT_SIZE_MB * 1024 * 1024)

/* segment size가 1MB가 안될 때 고려 */
#if PAGE_PER_SEGMENT < 64
#define NUM_HEAP_UNIT (SEGMENT_NO * (SEGMENT_SIZE/1024) / (HEAP_UNIT_SIZE_MB*1024))
#define NUM_DMEM_UNIT (SEGMENT_NO * (SEGMENT_SIZE/1024) / (DMEM_UNIT_SIZE_MB*1024))
#else
#define NUM_HEAP_UNIT (SEGMENT_NO * (SEGMENT_SIZE/1024/1024) / HEAP_UNIT_SIZE_MB)
#define NUM_DMEM_UNIT (SEGMENT_NO * (SEGMENT_SIZE/1024/1024) / DMEM_UNIT_SIZE_MB)
#endif

int num_heap_unit = NUM_HEAP_UNIT;
int num_dmem_unit = NUM_DMEM_UNIT;

/* heap:상위 xbit는 id, 하위 bit는 offset(16MB) */
#define HEAP_VADDR(id, offset)    (((id) << HEAP_UNIT_LOW_BITS) | (offset))
#define HEAP_MEMID(v)            ((v) >> HEAP_UNIT_LOW_BITS)
#define HEAP_OFFSET(v)            ((v) & HEAP_UNIT_LOW_OFFSET)

/* dmem:상위 xbit는 id, 하위 bit는 offset */
#define DMEM_VADDR(id, offset)    ((((OID)id+1) << DMEM_UNIT_LOW_BITS) | (offset))
#define DMEM_MEMID(v)            (((v) >> DMEM_UNIT_LOW_BITS) - 1)
#define DMEM_OFFSET(v)            ((v) & DMEM_UNIT_LOW_OFFSET)

static int DBDataMem_Alloc_count = 0;
void DBDataMem_RealFree(long v);

static int DBMem_Init_Client(int shmkey, char **ptr);

static char *dmem_free_head = NULL;


/*****************************************************************************/
//! DBMem_Init 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param shmkey :
 * \param ptr : 
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
DBMem_Init(int shmkey, char **ptr)
{
    return DBMem_Init_Client(shmkey, ptr);
}

/*****************************************************************************/
//! DBMem_Init_Client 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param shmkey :
 * \param ptr : 
 ************************************
 * \return  DB_SUCCESS or DB_FAIL
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static int
DBMem_Init_Client(int shmkey, char **ptr)
{
    return DB_SUCCESS;
}

/*****************************************************************************/
//! DBMem_Alloc 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param size :
 ************************************
 * \return  DB_SUCCESS or DB_FAIL
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
/* physical addr return */
__DECL_PREFIX void *
DBMem_Alloc(int size)
{
    return PMEM_ALLOC(size);
}

/*****************************************************************************/
//! DBMem_Free 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param ptr :
 ************************************
 * \return  void
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
/* physical address input */
__DECL_PREFIX void
DBMem_Free(void *ptr)
{
    if (ptr == NULL)
        return;
    PMEM_FREE(ptr);
    return;
}

/*****************************************************************************/
//! DBMem_AllFree 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param void 
 ************************************
 * \return  void
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
void
DBMem_AllFree(void)
{
    return;
}

/*****************************************************************************/
//! DBMem_V2P 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param pv :
 ************************************
 * \return  pv
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
__DECL_PREFIX void *
DBMem_V2P(void *pv)
{
    return pv;
}

/*****************************************************************************/
//! DBMem_P2V 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param p :
 ************************************
 * \return  void * : 
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
__DECL_PREFIX void *
DBMem_P2V(void *p)
{
    return p;
}

/*****************************************************************************/
//! DBDataMem_Alloc 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param size :
 * \param memid : 
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 * 1MB 단위의 alloc을 가정. 
 * physical address return 
 *****************************************************************************/
#if defined(_AS_WINCE_) ||defined(_AS_WIN32_MOBILE_)
HANDLE hDBDataMem = NULL;
#endif
void *
DBDataMem_Alloc(int size, unsigned long *memid)
{
    char *ptr;

    if (dmem_free_head)
    {
        ptr = dmem_free_head;
        dmem_free_head = (char *) (*(unsigned long *) ptr);
    }
    else
    {
#if defined(_AS_WINCE_) ||defined(_AS_WIN32_MOBILE_)
        if (hDBDataMem == NULL)
            hDBDataMem = HeapCreate(0, 5 * 1024 * 1024, 0);

        ptr = (char *) HeapAlloc(hDBDataMem, 0, size);
        if (ptr == NULL)
        {
#ifdef MDB_DEBUG
            MEMORYSTATUS membuffer;

            GlobalMemoryStatus(&membuffer);
            MDB_SYSLOG(("GlobalMemoryStatus: availPhys: %d, availVirtual:%d\n",
                            membuffer.dwAvailPhys, membuffer.dwAvailVirtual));
#endif

#ifdef MDB_DEBUG
            GlobalMemoryStatus(&membuffer);
            MDB_SYSLOG(("GlobalMemoryStatus: availPhys: %d, availVirtual:%d\n",
                            membuffer.dwAvailPhys, membuffer.dwAvailVirtual));
#endif
            ptr = (char *) HeapAlloc(hDBDataMem, 0, size);
        }
#else
        ptr = (char *) mdb_malloc(size);
#endif
        if (ptr)
            DBDataMem_Alloc_count++;
    }

    *memid = (unsigned long) ptr;

    return ptr;
}

/*****************************************************************************/
//! DBDataMem_V2P 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param v :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
void *
DBDataMem_V2P(unsigned long v)
{
    return (char *) v;
}

/*****************************************************************************/
//! DBDataMem_Free 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param v(in) :
 ************************************
 * \return  void
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
void
DBDataMem_Free(long v)
{
    char *ptr;

    if (dmem_free_head == NULL)
    {
        ptr = (char *) v;
        *(unsigned long *) ptr = (unsigned long) dmem_free_head;
        dmem_free_head = ptr;
    }
    else
        DBDataMem_RealFree(v);

    return;
}

void
DBDataMem_RealFree(long v)
{
    char *ptr;

    ptr = (char *) v;

#if defined(_AS_WINCE_) || defined(_AS_WIN32_MOBILE_)
    if (HeapFree(hDBDataMem, 0, ptr) == 0)
    {
        MDB_SYSLOG(("heap free error %d\n", GetLastError()));
    }
#else
    mdb_free(ptr);
#endif

    DBDataMem_Alloc_count--;

    return;
}

/*****************************************************************************/
//! DBDataMem_All_Free 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param void 
 ************************************
 * \return  void
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
void
DBDataMem_All_Free(void)
{
    if (dmem_free_head)
    {
        char *ptr;

#ifdef MDB_DEBUG
        __SYSLOG("DBDataMem_All_Free count %d ", DBDataMem_Alloc_count);
#endif
        while (dmem_free_head)
        {
            ptr = dmem_free_head;

            dmem_free_head = (char *) (*(unsigned long *) ptr);
#if defined(_AS_WINCE_) ||defined(_AS_WIN32_MOBILE_)
            HeapFree(hDBDataMem, 0, ptr);
#else
            mdb_free(ptr);
#endif
            DBDataMem_Alloc_count--;
        }
#ifdef MDB_DEBUG
        __SYSLOG("-> %d\n", DBDataMem_Alloc_count);
#endif
    }

#if defined(_AS_WINCE_) ||defined(_AS_WIN32_MOBILE_)
    if (hDBDataMem != NULL)
    {
        HeapDestroy(hDBDataMem);
        hDBDataMem = NULL;
    }
#endif

    return;
}

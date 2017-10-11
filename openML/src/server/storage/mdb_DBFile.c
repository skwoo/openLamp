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
#include "mdb_LogMgr.h"
#include "mdb_syslog.h"

extern int LogMgr_error;

static int
dbfile_GetNumOfFiles(int ftype)
{
    int num_files;

    if (mem_anchor_ == NULL)    /* When read MemBase data */
        return 1;

    switch (ftype)
    {
    case DBFILE_DATA0_TYPE:
    case DBFILE_DATA1_TYPE:
        if (mem_anchor_->allocated_segment_no_ == 0)
            num_files = 0;
        else
            num_files = (mem_anchor_->allocated_segment_no_ - 1)
                    / SEGMENTS_PER_DBFILE + 1;
        break;

    case DBFILE_TEMP_TYPE:
        if (mem_anchor_->tallocated_segment_no_ == 0)
            num_files = 0;
        else
            num_files = (mem_anchor_->tallocated_segment_no_ - 1)
                    / SEGMENTS_PER_DBFILE + 1;
        break;

    case DBFILE_INDEX_TYPE:
        if (mem_anchor_->iallocated_segment_no_ == 0)
            num_files = 0;
        else
            num_files =
                    (mem_anchor_->iallocated_segment_no_ - 1)
                    / SEGMENTS_PER_DBFILE + 1;
        break;

    default:
        MDB_SYSLOG(("FAIL: GetNumOfFiles ftype=%d\n", ftype));
        return DB_FAIL;
    }

    if (num_files >= MAX_FILE_ID)
    {
        MDB_SYSLOG(("FAIL: GetNumOfFiles ftype=%d, num_files=%d \n",
                        ftype, num_files));
        return DB_E_NUMSEGMENT;
    }

    return num_files;
}

/* optype = 1 (read), 0 (other) */
static int
dbfile_GetFileAccessInfo(int ftype, DB_UINT32 sn, int *fidx, int *sidx,
        int optype)
{
    int file_sn, num_segs;

    if (mem_anchor_ == NULL)
    {       /* When read MemBase data */
        if (sn == 0 && ftype <= DBFILE_DATA1_TYPE)
        {
            *fidx = 0;
            *sidx = 0;
            return DB_SUCCESS;
        }
        MDB_SYSLOG(
                ("FAIL: GetFileAccessInfo mem_anchor_=NULL,ftype=%d,sn=%d %d\n",
                        ftype, sn, __LINE__));
        return DB_E_INVALIDSEGNUM;
    }

    switch (ftype)
    {
    case DBFILE_DATA0_TYPE:
    case DBFILE_DATA1_TYPE:
        if (0 <= (int) sn && sn < TEMPSEG_BASE_NO)
            file_sn = sn;
        else
            file_sn = -1;
        num_segs = mem_anchor_->allocated_segment_no_;
        /* 할당된 sn 갯수(num_segs)보다 요청하는 sn(file_sn)이 큰 경우
         *              * 할당된 갯수는 file_sn + 1 값으로 설정 */
        if (optype && file_sn >= num_segs)
            mem_anchor_->allocated_segment_no_ = file_sn + 1;
        break;

    case DBFILE_TEMP_TYPE:
        if (isTemp_SEGMENT(sn))
            file_sn = sn - TEMPSEG_BASE_NO;
        else
            file_sn = -1;
        num_segs = mem_anchor_->tallocated_segment_no_;
        if (optype && file_sn >= num_segs)
            mem_anchor_->tallocated_segment_no_ = file_sn + 1;
        break;

    case DBFILE_INDEX_TYPE:
        file_sn = (SEGMENT_NO - 1) - sn;
        num_segs = mem_anchor_->iallocated_segment_no_;
        break;

    default:
        MDB_SYSLOG(("FAIL: GetFileAccessInfo ftype=%d %d\n", ftype, __LINE__));
        return DB_FAIL;
    }

    if (file_sn < 0 || file_sn > num_segs)
    {
        MDB_SYSLOG(
                ("FAIL: GetFileAccessInfo ftype=%d,sn=%d,f_sn=%d,n_segs=%d %d\n",
                        ftype, sn, file_sn, num_segs, __LINE__));
        if (file_sn < 0)
        {
#ifdef MDB_DEBUG
            sc_assert(0, __FILE__, __LINE__);
#endif
            return DB_E_INVALIDSEGNUM;
        }
    }

    *fidx = file_sn / SEGMENTS_PER_DBFILE;      /* file index */
    *sidx = file_sn % SEGMENTS_PER_DBFILE;      /* segment index in the file */

    return DB_SUCCESS;
}

/*****************************************************************************/
//! DBFile_Allocate 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param db_name :
 * \param db_path : 
 ************************************
 * \return  DB_SUCCESS or DB_FAIL
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
DBFile_Allocate(char *db_name, char *db_path)
{
    struct DB_File *db_file = &Mem_mgr->database_file_;
    int i, j;

    sc_strncpy(db_file->db_name_, db_name, (MDB_FILE_NAME - 1));

    sc_sprintf(db_file->file_name_[DBFILE_DATA0_TYPE],
            "%s" DIR_DELIM "%s_data.0", db_path, db_name);
    sc_sprintf(db_file->file_name_[DBFILE_DATA1_TYPE],
            "%s" DIR_DELIM "%s_data.1", db_path, db_name);
    sc_sprintf(db_file->file_name_[DBFILE_TEMP_TYPE],
            "%s" DIR_DELIM "%s_temp.t", db_path, db_name);

    /*  log path 아래에 index file을 만들자.
       db file이 flash ROM에 저장될 수 있으므로...
       깨어지더라도  index file은 다시 만들 수 있으므로, flash에 둘 필요 없음 */
    if (Log_Mgr && Log_Mgr->log_path != NULL)
    {
        sc_sprintf(db_file->file_name_[DBFILE_INDEX_TYPE],
                "%s" DIR_DELIM "%s_indx.i", Log_Mgr->log_path, db_name);
    }

    for (i = 0; i < DBFILE_TYPE_COUNT; i++)
    {
        for (j = 0; j < MAX_FILE_ID; j++)
            db_file->file_id_[i][j] = -1;
    }

    return DB_SUCCESS;
}

int
DBFile_Free(void)
{
    return DB_SUCCESS;
}


////////////////////////////////////////////////////////////////////
//
// Function Name :
//
///////////////////////////////////////////////////////////////////

int
DBFile_Fillup(int fd, int fillup_size)
{
    int ret;
    char x = '\0';

    if (sc_lseek(fd, fillup_size - 1, SEEK_SET) != (fillup_size - 1))
    {
        ret = DB_E_DBFILESEEK;
        goto end;
    }

    if (sc_write(fd, &x, 1) != 1)
    {
        ret = DB_E_DBFILEWRITE;
        goto end;
    }

    ret = fillup_size;

  end:
    return ret;
}

/*****************************************************************************/
//! DBFile_Create 
/*! \breif  DB File을 생성할 경우에 사용함 
 ************************************
 * \param num_data_segs
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
DBFile_Create(int num_data_segs)
{
    struct DB_File *db_file = &Mem_mgr->database_file_;
    int ftype, fidx;
    int num_files;
    int remainder;
    int file_size;
    int open_flag = 0;
    char db_file_name[MDB_FILE_NAME];
    int ret;

    for (ftype = 0; ftype < DBFILE_TYPE_COUNT; ftype++)
    {
#ifdef _ONE_BDB_
        if (ftype == DBFILE_DATA1_TYPE)
            continue;
#endif
        if (ftype == DBFILE_TEMP_TYPE)
            continue;

        if (db_file->file_name_[ftype][0] == '\0')
            continue;

        if (ftype <= DBFILE_DATA1_TYPE)
        {
            /* DBFILE_DATA0_TYPE, DBFILE_DATA1_TYPE */
            num_files = (num_data_segs - 1) / SEGMENTS_PER_DBFILE + 1;
            remainder = num_data_segs % SEGMENTS_PER_DBFILE;
        }
        else
        {
            /* DBFILE_TEMP_TYPE, DBFILE_INDEX_TYPE */
            num_files = 1;
            remainder = 1;
        }

        for (fidx = 0; fidx < num_files; fidx++)
        {
            sc_sprintf(db_file_name, "%s%d", db_file->file_name_[ftype], fidx);

            db_file->file_id_[ftype][fidx] =
                    sc_open(db_file_name,
                    O_BINARY | O_CREAT | O_TRUNC | O_RDWR, OPEN_MODE);
            if (db_file->file_id_[ftype][fidx] < 0)
            {
                MDB_SYSLOG(("DBFile_Create: create (%s) FAIL errno=%d\n",
                                db_file_name, sc_file_errno()));
                ret = DB_E_DBFILECREATE;
                goto error;
            }

            open_flag = 1;

            if (fidx < (num_files - 1))
                file_size = SEGMENT_SIZE * SEGMENTS_PER_DBFILE;
            else
                file_size = SEGMENT_SIZE * remainder;

            /* DBFile_Fillup */
            ret = DBFile_Fillup(db_file->file_id_[ftype][fidx], file_size);
            if (ret != file_size)
            {
                MDB_SYSLOG(("DBFile_Create: fillup (%s) FAIL errno=%d\n",
                                db_file_name, sc_file_errno()));
                ret = DB_E_DBFILEFILLUP;
                goto error;
            }
            ret = sc_fsync(db_file->file_id_[ftype][fidx]);
            if (ret < 0)
            {
                MDB_SYSLOG(("DBFile_Create: fsync (%s) FAIL errno=%d\n",
                                db_file_name, sc_file_errno()));
                ret = DB_E_DBFILESYNC;
                goto error;
            }

            sc_close(db_file->file_id_[ftype][fidx]);
            db_file->file_id_[ftype][fidx] = -1;

            open_flag = 0;
        }
    }       /* for ftype */

    return DB_SUCCESS;

  error:
    if (open_flag)
    {
        sc_close(db_file->file_id_[ftype][fidx]);
        db_file->file_id_[ftype][fidx] = -1;
    }
    return ret;
}

/*****************************************************************************/
//! DBFile_Create 
/*! \breif  DB File을 생성할 경우에 사용함 
 ************************************
 * \param 
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
DBFile_InitTemp(void)
{
    struct DB_File *db_file = &Mem_mgr->database_file_;
    char db_file_name[MDB_FILE_NAME];
    int ftype = DBFILE_TEMP_TYPE;
    int fidx;
    int ret;

    /* remove all DB temp files, except 0 */
    for (fidx = 1; fidx < MAX_FILE_ID; fidx++)
    {
        sc_sprintf(db_file_name, "%s%d", db_file->file_name_[ftype], fidx);

        if (sc_unlink(db_file_name) < 0)
            break;
    }

    /* create the first DB temp file */
    sc_sprintf(db_file_name, "%s%d", db_file->file_name_[ftype], 0);

    fidx = 0;

    db_file->file_id_[ftype][fidx] =
            sc_open(db_file_name,
            O_BINARY | O_CREAT | O_TRUNC | O_RDWR, OPEN_MODE);
    if (db_file->file_id_[ftype][fidx] < 0)
    {
        MDB_SYSLOG(("DBFile_InitTemp: create (%s) FAIL errno=%d\n",
                        db_file_name, sc_file_errno()));
        ret = DB_E_DBFILECREATE;
        goto error;
    }

    /* DBFile_Fillup */
    ret = DBFile_Fillup(db_file->file_id_[ftype][fidx], SEGMENT_SIZE);
    if (ret != SEGMENT_SIZE)
    {
        MDB_SYSLOG(("DBFile_InitTemp: fillup (%s) FAIL errno=%d\n",
                        db_file_name, sc_file_errno()));
        ret = DB_E_DBFILEFILLUP;
        goto error;
    }


    sc_close(db_file->file_id_[ftype][fidx]);
    db_file->file_id_[ftype][fidx] = -1;
    return DB_SUCCESS;

  error:
    sc_close(db_file->file_id_[ftype][fidx]);
    db_file->file_id_[ftype][fidx] = -1;
    return ret;
}

/*****************************************************************************/
//! FileOpen 
/*! \breif  DB File을 Open 시 사용하는 함수 
 ************************************
 * \param db_file :
 * \param db_idx : 
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
/*
 * sn = -1 : 전체 file open, 파일 없는 경우 생성
 * sn = -2 : MDB_MemoryFree()에서 호출
 *           전체 file open, 파일 없는 경우 생성하지 않음
 */
int
DBFile_Open(int ftype, int sn)
{
    struct DB_File *db_file = &Mem_mgr->database_file_;
    char db_file_name[MDB_FILE_NAME];
    int base_fidx;
    int num_files;
    int fidx, sidx;
    int open_flag = 0;
    int ret;

#ifdef _ONE_BDB_
    if (ftype == DBFILE_DATA1_TYPE)
        ftype = DBFILE_DATA0_TYPE;
#endif

    if (db_file->file_name_[ftype][0] == '\0')
        return -1;

    if (sn < 0) /* open all files of the given ftype */
    {
        base_fidx = 0;
        num_files = dbfile_GetNumOfFiles(ftype);
        if (num_files < 0)
        {
            return num_files;
        }
    }
    else    /* sn != -1 : open a file containing the segment */
    {
        ret = dbfile_GetFileAccessInfo(ftype, sn, &fidx, &sidx, 0);
        if (ret < 0)
        {
            return ret;
        }
        base_fidx = fidx;
        num_files = 1;
    }

    for (fidx = base_fidx; fidx < (base_fidx + num_files); fidx++)
    {
        if (db_file->file_id_[ftype][fidx] != -1)
            continue;   /* already opened */

        if (db_file->file_name_[ftype][0] == '\0')
            continue;

        sc_sprintf(db_file_name, "%s%d", db_file->file_name_[ftype], fidx);

        db_file->file_id_[ftype][fidx]
                = sc_open(db_file_name, O_RDWR | O_BINARY, OPEN_MODE);
        if (sn != -2 && db_file->file_id_[ftype][fidx] < 0)
        {
            MDB_SYSLOG(("DBFile_Open: open (%s) FAIL errno=%d\n",
                            db_file_name, sc_file_errno()));

            /* file create 시도 */
            db_file->file_id_[ftype][fidx] =
                    sc_open(db_file_name,
                    O_BINARY | O_CREAT | O_TRUNC | O_RDWR, OPEN_MODE);
            if (db_file->file_id_[ftype][fidx] < 0)
            {
                MDB_SYSLOG(("DBFile_Open: creat (%s) FAIL errno=%d\n",
                                db_file_name, sc_file_errno()));
                ret = DB_E_DBFILECREATE;
                goto error;
            }

            open_flag = 1;

            /* DBFile_Fillup */
            ret = DBFile_Fillup(db_file->file_id_[ftype][fidx], SEGMENT_SIZE);
            if (ret != SEGMENT_SIZE)
            {
                MDB_SYSLOG(("DBFile_Open: fillup (%s) FAIL errno=%d\n",
                                db_file_name, sc_file_errno()));
                ret = DB_E_DBFILEFILLUP;
                goto error;
            }
            ret = sc_fsync(db_file->file_id_[ftype][fidx]);
            if (ret < 0)
            {
                MDB_SYSLOG(("DBFile_Open: write (%s) FAIL errno=%d\n",
                                db_file_name, sc_file_errno()));
                ret = DB_E_DBFILESYNC;
                goto error;
            }

            sc_close(db_file->file_id_[ftype][fidx]);
            db_file->file_id_[ftype][fidx] = -1;

            open_flag = 0;

            MDB_SYSLOG(("> FILE Create SUCCESS %d, %d\n", fidx, num_files));

            db_file->file_id_[ftype][fidx]
                    = sc_open(db_file_name, O_RDWR | O_BINARY, OPEN_MODE);

            if (db_file->file_id_[ftype][fidx] < 0)
            {
                MDB_SYSLOG(("DBFile_Open: reopen (%s) FAIL errno=%d\n",
                                db_file_name, sc_file_errno()));
                ret = DB_E_DBFILEOPEN;
                goto error;
            }
        }
    }       /* for */

    return DB_SUCCESS;

  error:
    if (open_flag)
    {
        if (db_file->file_id_[ftype][fidx] >= 0)
            sc_close(db_file->file_id_[ftype][fidx]);
        db_file->file_id_[ftype][fidx] = -1;
    }
    return ret;
}

/*****************************************************************************/
//! FileClose 
/*! \breif  DB File을  close 할때 사용되는 함수
 ************************************
 * \param db_file :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
DBFile_Close(int ftype, DB_INT32 sn)
{
    struct DB_File *db_file = &Mem_mgr->database_file_;
    int base_fidx;
    int num_files;
    int fidx, sidx;
    int ret;

#ifdef _ONE_BDB_
    if (ftype == DBFILE_DATA1_TYPE)
        ftype = DBFILE_DATA0_TYPE;
#endif

    if (sn == -1)       /* close all files of the given ftype */
    {
        base_fidx = 0;
        num_files = dbfile_GetNumOfFiles(ftype);
        if (num_files < 0)
        {
            ret = num_files;
            goto error;
        }
    }
    else    /* sn != -1 : close a file containing the segment */
    {
        ret = dbfile_GetFileAccessInfo(ftype, sn, &fidx, &sidx, 0);
        if (ret < 0)
        {
            goto error;
        }
        base_fidx = fidx;
        num_files = 1;
    }


    if (MDB_IS_SHUTDOWN_MODE())
    {
        for (fidx = base_fidx; fidx < (base_fidx + num_files); fidx++)
        {
            if (db_file->file_id_[ftype][fidx] < 0)
                continue;

            ret = sc_close(db_file->file_id_[ftype][fidx]);
            if (ret != 0)
            {
                MDB_SYSLOG(("DBFile_Close: close FAIL (ftype=%d,sn=%d) "
                                "errno=%d\n", ftype, sn, sc_file_errno()));
                ret = DB_E_DBFILECLOSE;
                goto error;
            }

            db_file->file_id_[ftype][fidx] = -1;
        }
    }

    return DB_SUCCESS;

  error:

    return ret;
}

/*****************************************************************************/
//! FileSync 
/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param db_file(in)    :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
DBFile_Sync(int ftype, int sn)
{
    struct DB_File *db_file = &Mem_mgr->database_file_;
    int base_fidx;
    int num_files;
    int fidx, sidx;
    int ret;

#ifdef _ONE_BDB_
    if (ftype == DBFILE_DATA1_TYPE)
        ftype = DBFILE_DATA0_TYPE;
#endif

    if (ftype == DBFILE_INDEX_TYPE && server->status == SERVER_STATUS_INDEXING)
    {
        return DB_SUCCESS;
    }

    if (sn == -1)       /* sync all files of the given ftype */
    {
        base_fidx = 0;
        num_files = dbfile_GetNumOfFiles(ftype);
        if (num_files < 0)
        {
            return num_files;
        }
    }
    else    /* sn != -1 : sync a file containing the segment */
    {
        ret = dbfile_GetFileAccessInfo(ftype, sn, &fidx, &sidx, 0);
        if (ret < 0)
        {
            return ret;
        }
        base_fidx = fidx;
        num_files = 1;
    }

    for (fidx = base_fidx; fidx < (base_fidx + num_files); fidx++)
    {
        if (db_file->file_id_[ftype][fidx] < 0)
            continue;

        ret = sc_fsync(db_file->file_id_[ftype][fidx]);
        if (ret < 0)
        {
            MDB_SYSLOG(("DBFile_Sync: fsync FAIL (ftype=%d,sn=%d) errno=%d\n",
                            ftype, sn, sc_file_errno()));
            return DB_E_DBFILESYNC;
        }
    }

    return DB_SUCCESS;
}


/*****************************************************************************/
//! DBFile_Read
/*! \breif  파일에서 특정 세그먼트로 데이타를 읽어옴  
 ************************************
 * \param sn(in)        :
 * \param ftype(in)    :
 ************************************
 * \return  DB_SUCCESS or DB_FAIL
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
DBFile_Read(int ftype, DB_INT32 sn, DB_INT32 pn, void *pgptr, int count)
{
    int open_fid;
    int fidx, sidx;
    int offset;
    int read_size;
    int ret;

#ifdef MDB_DEBUG
    sc_assert(sn < SEGMENT_NO, __FILE__, __LINE__);
    sc_assert(pn < PAGE_PER_SEGMENT, __FILE__, __LINE__);
    sc_assert(pn + count <= PAGE_PER_SEGMENT, __FILE__, __LINE__);
#endif

#ifdef _ONE_BDB_
    if (ftype == DBFILE_DATA1_TYPE)
        ftype = DBFILE_DATA0_TYPE;
#endif

    ret = dbfile_GetFileAccessInfo(ftype, sn, &fidx, &sidx, 1);
    if (ret < 0)
    {
        /* 로그와 관련하여 임시방법으로 처리 
         *  LogMgr_Restart() 에서 해당 변수(LogMgr_error) 사용  
         */
        LogMgr_error = 1;
        MDB_SYSLOG(("DBFile_Read: Get Acess File Info FAIL %d \n", ret));
        return ret;
    }

    offset = (SEGMENT_SIZE * sidx) + (S_PAGE_SIZE * pn);

    read_size = (S_PAGE_SIZE * count);

    open_fid = Mem_mgr->database_file_.file_id_[ftype][fidx];

    if (open_fid < 0)
    {
        LogMgr_error = 1;
        MDB_SYSLOG(("DBFile_Read: Not Opened (sn=%d,pn=%d,cnt=%d)\n",
                        sn, pn, count));
        return DB_E_DBFILEOPEN;
    }

    ret = sc_lseek(open_fid, offset, SEEK_SET);
    if (ret != offset)
    {
        LogMgr_error = 1;
        MDB_SYSLOG(("DBFile_Read: lseek FAIL (sn=%d,pn=%d,cnt=%d) errno=%d\n",
                        sn, pn, count, sc_file_errno()));
        return DB_E_DBFILESEEK;
    }


    ret = sc_read(open_fid, pgptr, read_size);
    if (ret != read_size)
    {
        LogMgr_error = 1;
        MDB_SYSLOG(("DBFile_Read: read FAIL (sn=%d,pn=%d,cnt=%d) errno=%d\n",
                        sn, pn, count, sc_file_errno()));
        return DB_E_DBFILEREAD;
    }

    {
        int i;

        for (i = 0; i < count; i++)
        {
            if (OID_Cal(sn, pn + i,
                            0) != ((struct Page *) pgptr + i)->header_.self_)
            {
                MDB_SYSLOG(("DBFile Read page OID fail type "
                                "%d, OID(%d,%d) %d, %d\n",
                                ftype, sn, pn + i, OID_Cal(sn, pn + i, 0),
                                ((struct Page *) pgptr + i)->header_.self_));
                /* If ftype is index file, return error and make the index
                 * file rebuild. */
                if (ftype != DBFILE_INDEX_TYPE)
                {
                    // Out of MDB_DEBUG. Should we assert?
                    sc_assert(0, __FILE__, __LINE__);
                }
                LogMgr_error = 1;
                return DB_E_DBFILEOID;
            }
        }
    }

    return DB_SUCCESS;
}


/*****************************************************************************/
//! DBFile_Write
/*! \breif  DB File의 특정 위치(offset)에  pgptr 을 쓴다
 ************************************
 * \param db_file(in)     :
 * \param ftype(in)     : 
 * \param sn(in)         :
 * \param pn(in)         :
 * \param pgptr(in)    :
 ************************************
 * \return  DB_SUCCESS or DB_FAIL
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
DBFile_Write(int ftype, DB_INT32 sn, DB_INT32 pn, void *pgptr, int count)
{
    int open_fid;
    int fidx, sidx;
    int offset;
    int write_size;
    int ret;
    int i;

#ifdef MDB_DEBUG
    sc_assert(sn < SEGMENT_NO, __FILE__, __LINE__);
    sc_assert(pn < PAGE_PER_SEGMENT, __FILE__, __LINE__);
    sc_assert(pn + count <= PAGE_PER_SEGMENT, __FILE__, __LINE__);
#endif

    for (i = 0; i < count; i++)
    {
        if (IS_CRASHED_PAGE(sn, pn + i, ((struct Page *) pgptr + i)))
        {
            sc_assert(0, __FILE__, __LINE__);
            return DB_E_INVALIDPAGE;
        }
        if (OID_Cal(sn, pn + i,
                        0) != ((struct Page *) pgptr + i)->header_.self_)
        {
            // Out of MDB_DEBUG. Should we assert?
            sc_assert(0, __FILE__, __LINE__);
            return DB_E_DBFILEOID;
        }
    }

#ifdef _ONE_BDB_
    if (ftype == DBFILE_DATA1_TYPE)
        ftype = DBFILE_DATA0_TYPE;
#endif

    ret = dbfile_GetFileAccessInfo(ftype, sn, &fidx, &sidx, 0);
    if (ret < 0)
    {
        MDB_SYSLOG(("DBFile_Write: Get Acess File Info FAIL %d \n", ret));
        return ret;
    }

    offset = (SEGMENT_SIZE * sidx) + (S_PAGE_SIZE * pn);

    write_size = (S_PAGE_SIZE * count);

    open_fid = Mem_mgr->database_file_.file_id_[ftype][fidx];

    if (open_fid < 0)
    {
        MDB_SYSLOG(("DBFile_Write: Not Opened (sn=%d,pn=%d,cnt=%d)\n",
                        sn, pn, count));
        return DB_E_DBFILEOPEN;
    }

    ret = sc_lseek(open_fid, offset, SEEK_SET);
    if (ret != offset)
    {
        MDB_SYSLOG(("DBFile_Write: lseek FAIL (sn=%d,pn=%d,cnt=%d) errno=%d\n",
                        sn, pn, count, sc_file_errno()));
        return DB_E_DBFILESEEK;
    }

    ret = sc_write(open_fid, pgptr, write_size);
    if (ret != write_size)
    {
        MDB_SYSLOG(("DBFile_Write: write FAIL (sn=%d,pn=%d,cnt=%d) errno=%d\n",
                        sn, pn, count, sc_file_errno()));
        return DB_E_DBFILEWRITE;
    }

    return DB_SUCCESS;
}

/*****************************************************************************/
//! DBFile_Read_OC
/*! \breif  파일에서 특정 세그먼트로 데이타를 읽어옴  
 ************************************
 * \param sn(in)        :
 * \param ftype(in)    :
 ************************************
 * \return  DB_SUCCESS or DB_FAIL
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
DBFile_Read_OC(int ftype, DB_INT32 sn, DB_INT32 pn, void *pgptr, int count)
{
    struct DB_File *db_file = &Mem_mgr->database_file_;
    int fidx, sidx;
    int offset;
    int read_size;
    int open_flag = 0;
    int ret;

#ifdef MDB_DEBUG
    sc_assert(sn < SEGMENT_NO, __FILE__, __LINE__);
    sc_assert(pn < PAGE_PER_SEGMENT, __FILE__, __LINE__);
    sc_assert(pn + count <= PAGE_PER_SEGMENT, __FILE__, __LINE__);
#endif

#ifdef _ONE_BDB_
    if (ftype == DBFILE_DATA1_TYPE)
        ftype = DBFILE_DATA0_TYPE;
#endif

    ret = dbfile_GetFileAccessInfo(ftype, sn, &fidx, &sidx, 1);
    if (ret < 0)
    {
        LogMgr_error = 1;
        MDB_SYSLOG(("DBFile_Read: Get Acess File Info FAIL %d \n", ret));
        return ret;
    }

    offset = (SEGMENT_SIZE * sidx) + (S_PAGE_SIZE * pn);

    read_size = (S_PAGE_SIZE * count);

    if (db_file->file_id_[ftype][fidx] < 0)
    {
        char db_file_name[MDB_FILE_NAME];

        sc_sprintf(db_file_name, "%s%d", db_file->file_name_[ftype], fidx);

        db_file->file_id_[ftype][fidx]
                = sc_open(db_file_name, O_RDWR | O_BINARY, OPEN_MODE);

        if (db_file->file_id_[ftype][fidx] < 0)
        {
            MDB_SYSLOG(("DBFile_Read_OC: open (%s) FAIL errno=%d\n",
                            db_file_name, sc_file_errno()));
            return DB_E_DBFILEOPEN;
        }

        open_flag = 1;
    }

    ret = sc_lseek(db_file->file_id_[ftype][fidx], offset, SEEK_SET);
    if (ret != offset)
    {
        LogMgr_error = 1;
        MDB_SYSLOG(("DBFile_Read_OC: lseek FAIL (sn=%d,pn=%d,cnt=%d) "
                        "errno=%d\n", sn, pn, count, sc_file_errno()));
        ret = DB_E_DBFILESEEK;
        goto error;
    }

    ret = sc_read(db_file->file_id_[ftype][fidx], pgptr, read_size);
    if (ret != read_size)
    {
        LogMgr_error = 1;
        MDB_SYSLOG(("DBFile_Read_OC: read FAIL (sn=%d,pn=%d,cnt=%d) "
                        "ret=%d, errno=%d\n",
                        sn, pn, count, ret, sc_file_errno()));
        ret = DB_E_DBFILEREAD;
        goto error;
    }

    if (open_flag)
    {
        if (MDB_IS_SHUTDOWN_MODE())
        {
            ret = sc_close(db_file->file_id_[ftype][fidx]);
            if (ret != 0)
            {
                MDB_SYSLOG(("DBFile_Read_OC: close FAIL(sn=%d,pn=%d,cnt=%d) "
                                "errno=%d\n", sn, pn, count, sc_file_errno()));
                ret = DB_E_DBFILECLOSE;
                goto error;
            }

            db_file->file_id_[ftype][fidx] = -1;
        }
    }

    {
        int i;

        for (i = 0; i < count; i++)
        {
            if (OID_Cal(sn, pn + i,
                            0) != ((struct Page *) pgptr + i)->header_.self_)
            {
                /* index가 아닌 경우 assert. index file인 경우 error return하여
                 * index file을 재생성 하도록 처리 */
                if (ftype != DBFILE_INDEX_TYPE)
                {
                    /* writing __SYSLOG, instaed of assertion */
                    MDB_SYSLOG(
                            ("page is not matched, sn = %d, pn = %d, i = %d\n",
                                    sn, pn, i));
                }
                /* 여기서 assert가 발생한 경우
                 * 페이지를 읽었는데 다른 위치의 페이지가 읽혀온 경우 발생된 assert
                 * 예전에 중국랩에서 이러한 문제가 있었는데
                 * file system full 상황에서 page를 write를 했는데 아래쪽
                 * 파일시스템에서 제대로 파일에 write를 해주지 못한 경우로,
                 * 파일시스템에서 여분의 space를 남겨두고 않을 때 발생되었던 것 같음.
                 */
                LogMgr_error = 1;

                return DB_E_DBFILEOID;
            }
        }
    }

    return DB_SUCCESS;

  error:
    if (open_flag)
    {
        sc_close(db_file->file_id_[ftype][fidx]);
        db_file->file_id_[ftype][fidx] = -1;
    }

    return ret;
}

/*****************************************************************************/
//! DBFile_Write_OC
/*! \breif  DB File의 특정 위치(offset)에  pgptr 을 쓴다
 ************************************
 * \param db_file(in)     :
 * \param ftype(in)     : 
 * \param sn(in)         :
 * \param pn(in)         :
 * \param pgptr(in)    :
 ************************************
 * \return  DB_SUCCESS or DB_FAIL
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
DBFile_Write_OC(int ftype, DB_INT32 sn, DB_INT32 pn, void *pgptr, int count,
        int f_sync)
{
    struct DB_File *db_file = &Mem_mgr->database_file_;
    int fidx, sidx;
    int offset;
    int write_size;
    int open_flag = 0;
    int ret;
    int i;

#ifdef MDB_DEBUG
    sc_assert(sn < SEGMENT_NO, __FILE__, __LINE__);
    sc_assert(pn < PAGE_PER_SEGMENT, __FILE__, __LINE__);
    sc_assert(pn + count <= PAGE_PER_SEGMENT, __FILE__, __LINE__);
#endif

    for (i = 0; i < count; i++)
    {
        if (IS_CRASHED_PAGE(sn, pn + i, ((struct Page *) pgptr + i)))
        {
            sc_assert(0, __FILE__, __LINE__);
            return DB_E_INVALIDPAGE;
        }
        if (OID_Cal(sn, pn + i,
                        0) != ((struct Page *) pgptr + i)->header_.self_)
        {
            // Out of MDB_DEBUG. Should we assert?
            sc_assert(0, __FILE__, __LINE__);
            return DB_E_DBFILEOID;
        }
    }

#ifdef _ONE_BDB_
    if (ftype == DBFILE_DATA1_TYPE)
        ftype = DBFILE_DATA0_TYPE;
#endif

#ifdef MDB_DEBUG
    if (ftype == DBFILE_INDEX_TYPE && count > PAGE_PER_SEGMENT)
        sc_assert(0, __FILE__, __LINE__);
#endif

    ret = dbfile_GetFileAccessInfo(ftype, sn, &fidx, &sidx, 0);
    if (ret < 0)
    {
        MDB_SYSLOG(("DBFile_Write_OC: Get Acess File Info FAIL %d \n", ret));
        return ret;
    }

    offset = (SEGMENT_SIZE * sidx) + (S_PAGE_SIZE * pn);

    write_size = (S_PAGE_SIZE * count);

    if (db_file->file_id_[ftype][fidx] < 0)
    {
        char db_file_name[MDB_FILE_NAME];

        sc_sprintf(db_file_name, "%s%d", db_file->file_name_[ftype], fidx);

        db_file->file_id_[ftype][fidx]
                = sc_open(db_file_name, O_RDWR | O_BINARY, OPEN_MODE);

        if (db_file->file_id_[ftype][fidx] < 0)
        {
            /* create the file */
            db_file->file_id_[ftype][fidx] =
                    sc_open(db_file_name,
                    O_BINARY | O_CREAT | O_TRUNC | O_RDWR, OPEN_MODE);
            if (db_file->file_id_[ftype][fidx] < 0)
            {
                MDB_SYSLOG(("DBFile_Write_OC: create (%s) FAIL errno=%d\n",
                                db_file_name, sc_file_errno()));
                ret = DB_E_DBFILECREATE;
                goto error;
            }

            open_flag = 1;

            /* DBFile_Fillup */
            ret = DBFile_Fillup(db_file->file_id_[ftype][fidx], SEGMENT_SIZE);
            if (ret != SEGMENT_SIZE)
            {
                MDB_SYSLOG(("DBFile_Write: fillup (%s) FAIL errno=%d\n",
                                db_file_name, sc_file_errno()));
                ret = DB_E_DBFILEFILLUP;
                goto error;
            }
            ret = sc_fsync(db_file->file_id_[ftype][fidx]);
            if (ret < 0)
            {
                MDB_SYSLOG(("DBFile_Write_OC: fsync (%s) FAIL errno=%d\n",
                                db_file_name, sc_file_errno()));
                ret = DB_E_DBFILESYNC;
                goto error;
            }

            sc_close(db_file->file_id_[ftype][fidx]);
            db_file->file_id_[ftype][fidx] = -1;

            open_flag = 0;

            /* reopen the file */

            db_file->file_id_[ftype][fidx]
                    = sc_open(db_file_name, O_RDWR | O_BINARY, OPEN_MODE);

            if (db_file->file_id_[ftype][fidx] < 0)
            {
                MDB_SYSLOG(("DBFile_Write_OC: reopen (%s) FAIL errno=%d\n",
                                db_file_name, sc_file_errno()));
                ret = DB_E_DBFILEOPEN;
                goto error;
            }
        }

        open_flag = 1;
    }

    ret = sc_lseek(db_file->file_id_[ftype][fidx], offset, SEEK_SET);
    if (ret != offset)
    {
        MDB_SYSLOG(("DBFile_Write_OC: lseek FAIL (sn=%d,pn=%d,cnt=%d) "
                        "errno=%d\n", sn, pn, count, sc_file_errno()));
        ret = DB_E_DBFILESEEK;
        goto error;
    }

    ret = sc_write(db_file->file_id_[ftype][fidx], pgptr, write_size);
    if (ret != write_size)
    {
        MDB_SYSLOG(("DBFile_Write_OC: write FAIL (sn=%d,pn=%d,cnt=%d) "
                        "errno=%d\n", sn, pn, count, sc_file_errno()));
        ret = DB_E_DBFILEWRITE;
        goto error;
    }

    if (f_sync)
    {
        ret = sc_fsync(db_file->file_id_[ftype][fidx]);
        if (ret != 0)
        {
            MDB_SYSLOG(("DBFile_Write_OC: fsync FAIL(sn=%d,pn=%d,cnt=%d) "
                            "errno=%d\n", sn, pn, count, sc_file_errno()));
            ret = DB_E_DBFILESYNC;
            goto error;
        }
    }

    if (open_flag)
    {
        if (MDB_IS_SHUTDOWN_MODE())
        {
            ret = sc_close(db_file->file_id_[ftype][fidx]);
            if (ret != 0)
            {
                MDB_SYSLOG(("DBFile_Write_OC: close FAIL(sn=%d,pn=%d,cnt=%d) "
                                "errno=%d\n", sn, pn, count, sc_file_errno()));
                ret = DB_E_DBFILECLOSE;
                goto error;
            }

            db_file->file_id_[ftype][fidx] = -1;
        }
    }

    return DB_SUCCESS;

  error:
    if (open_flag)
    {
        sc_close(db_file->file_id_[ftype][fidx]);
        db_file->file_id_[ftype][fidx] = -1;
    }
    return ret;
}

int
DBFile_GetVolumeSize(int ftype)
{
    struct DB_File *db_file = &Mem_mgr->database_file_;
    int ret;
    int fid, fidx;
    int volume_size = 0;
    int file_size;

    if (db_file->file_name_[ftype][0] == '\0')
        return 0;

    ret = DBFile_Open(ftype, -1);
    if (ret < 0)
    {
        __SYSLOG("DBFile_GetVolumeSize(%d): %d, %d\n", ftype, ret, __LINE__);
        return ret;
    }

    for (fidx = 0; fidx < MAX_FILE_ID; fidx++)
    {
        fid = Mem_mgr->database_file_.file_id_[ftype][fidx];
        if (fid < 0)
            break;

        file_size = sc_lseek(fid, 0, SEEK_END);

        if (file_size < 0)
            break;

        volume_size += file_size;
    }

    DBFile_Close(ftype, -1);

    return volume_size;
}

int
DBFile_AddVolume(int ftype, int num_data_segs)
{
    struct DB_File *db_file = &Mem_mgr->database_file_;
    int fidx;
    int num_files;
    char db_file_name[MDB_FILE_NAME];
    int remain_size;
    int file_size;
    int write_size;
    int fid;

    if (mem_anchor_ == NULL)
        return DB_E_DBMEMINIT;

    if (num_data_segs <= 0)
        return DB_E_INVALIDPARAM;

    remain_size = (num_data_segs * SEGMENT_SIZE);

    num_files = dbfile_GetNumOfFiles(ftype);
    if (num_files < 0)
    {
        return num_files;
    }

    for (fidx = (num_files - 1); fidx < MAX_FILE_ID; fidx++)
    {
        sc_sprintf(db_file_name, "%s%d", db_file->file_name_[ftype], fidx);

        fid = sc_open(db_file_name, O_RDWR | O_BINARY, OPEN_MODE);
        if (fid < 0)
        {
            fid = sc_open(db_file_name,
                    O_BINARY | O_CREAT | O_TRUNC | O_RDWR, OPEN_MODE);
            if (fid < 0)
            {
                return DB_E_DBFILECREATE;
            }
        }

        file_size = sc_lseek(fid, 0, SEEK_END);
        if (file_size < 0)
        {
            MDB_SYSLOG(("FAIL: seek %s 0 SEEK_END %d, %d\n",
                            db_file_name, file_size, __LINE__));
            sc_close(fid);
            return DB_E_DBFILESEEK;
        }

        if (file_size >= SEGMENT_SIZE * SEGMENTS_PER_DBFILE)
        {
            sc_close(fid);
            continue;
        }

        write_size = file_size + remain_size;

        if (write_size > SEGMENT_SIZE * SEGMENTS_PER_DBFILE)
            write_size = SEGMENT_SIZE * SEGMENTS_PER_DBFILE;

        remain_size -= (write_size - file_size);

        /* DBFile_Fillup */
        if (DBFile_Fillup(fid, write_size) != write_size)
        {
            MDB_SYSLOG(("FAIL: fillup %s %d, %d\n",
                            db_file_name, write_size, __LINE__));
            sc_close(fid);
            return DB_E_DBFILEFILLUP;
        }

        sc_close(fid);

        if (remain_size == 0)
            break;
    }

    return DB_SUCCESS;
}

int
DBFile_DiskFreeSpace(int ftype)
{
#if defined(DO_NOT_CHECK_DISKFREESPACE)
    return SEGMENT_SIZE * 10;
#else
    struct DB_File *db_file = &Mem_mgr->database_file_;
    char db_file_name[MDB_FILE_NAME];

    if (mem_anchor_ == NULL)
        return DB_E_DBMEMINIT;

    if (DB_Params.check_free_space == 0)
    {
        return SEGMENT_SIZE * 10;
    }

    sc_sprintf(db_file_name, "%s%d", db_file->file_name_[ftype], 0);

    return sc_get_disk_free_space(db_file_name);
#endif
}

int
DBFile_GetLastSegment(int ftype, int file_size)
{
    struct DB_File *db_file = &Mem_mgr->database_file_;
    char db_file_name[MDB_FILE_NAME];
    int fid;
    int last_seg;
    OID page_oid;
    int ret = 0;
    int offset;

    if (db_file->file_name_[ftype][0] == '\0')
    {
        return 0;
    }

    if (file_size < SEGMENT_SIZE)
    {
        file_size = DBFile_GetVolumeSize(DBFILE_DATA0_TYPE);
    }

    if (file_size < SEGMENT_SIZE)
    {
        return 0;
    }

    last_seg = file_size / SEGMENT_SIZE;

    sc_sprintf(db_file_name, "%s0", db_file->file_name_[ftype]);

    fid = sc_open(db_file_name, O_RDWR | O_BINARY, OPEN_MODE);
    if (fid < 0)
    {
        return 0;
    }

    while (last_seg)
    {
        /* segment의 마지막 page offset 계산 */
        offset = last_seg * SEGMENT_SIZE + (PAGE_PER_SEGMENT -
                1) * S_PAGE_SIZE;

        if (offset > file_size)
        {
            last_seg--;
            continue;
        }

        ret = sc_lseek(fid, offset, SEEK_SET);
        if (ret < 0)
            break;

        ret = sc_read(fid, &page_oid, sizeof(OID));
        if (ret < 0)
            break;

        /* found */
        if (OID_Cal(last_seg, PAGE_PER_SEGMENT - 1, 0) == page_oid)
            break;

        last_seg--;
    }

    sc_close(fid);

    if (ret < 0)
        return 0;

    return last_seg;
}

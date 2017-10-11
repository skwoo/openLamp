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

#ifndef __DBFILE_H
#define __DBFILE_H

#include "mdb_config.h"
#include "mdb_ppthread.h"
#include "mdb_inner_define.h"

#define MAX_FILE_ID        ((SEGMENT_NO-1) / 1024 + 1)

#if defined(SEGMENT_NO_65536)
#define SEGMENTS_PER_DBFILE (65536)
#else
#define SEGMENTS_PER_DBFILE (4096)      //1024
#endif

#define DBFILE_DATA0_TYPE   0
#define DBFILE_DATA1_TYPE   1

#define DBFILE_TEMP_TYPE    2
#define DBFILE_INDEX_TYPE   3
#define DBFILE_TYPE_COUNT   4

struct DB_File
{
    char db_name_[MDB_FILE_NAME];
    char file_name_[DBFILE_TYPE_COUNT][MDB_FILE_NAME];
    int file_id_[DBFILE_TYPE_COUNT][MAX_FILE_ID];
};

extern int DBFile_Allocate(char *db_name, char *db_path);
extern int DBFile_Free(void);
extern int DBFile_Create(int num_data_segs);
extern int DBFile_InitTemp(void);
extern int DBFile_Open(int ftype, int sn);
extern int DBFile_Close(int ftype, DB_INT32 sn);
extern int DBFile_Sync(int ftype, int sn);
extern int DBFile_Read(int ftype, DB_INT32 sn, DB_INT32 pn, void *pgptr,
        int count);
extern int DBFile_Write(int ftype, DB_INT32 sn, DB_INT32 pn, void *pgptr,
        int count);

extern int DBFile_Read_OC(int ftype, DB_INT32 sn, DB_INT32 pn, void *pgptr,
        int count);
extern int DBFile_Write_OC(int ftype, DB_INT32 sn, DB_INT32 pn, void *pgptr,
        int count, int sync);
extern int DBFile_GetVolumeSize(int ftype);
extern int DBFile_AddVolume(int ftype, int num_data_segs);

extern int DBFile_DiskFreeSpace(int ftype);

int DBFile_GetLastSegment(int ftype, int file_size);

#endif

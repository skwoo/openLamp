/* 
   This file is part of openML, mobile and embedded DBMS.

   Copyright (C) 2012 Inervit Co., Ltd.
   support@inervit.com

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mdb_config.h"
#include "../../version_mmdb.h"
#include "mdb_Server.h"

static void
print_log_anchor(struct LogAnchor *anchor)
{
    printf("# Log Anchor: size=%d\n", sizeof(struct LogAnchor));
    printf("###############################################\n");
    printf("check1 = %d, check2 = %d, check3 = %d\n",
            anchor->check1, anchor->check2, anchor->check3);
    printf("Current_Log_File_No = %d\n", anchor->Current_Log_File_No_);
    printf("Last_Deleted_Log_File_No = %d\n",
            anchor->Last_Deleted_Log_File_No_);
    printf("Anchor_File_Name = %s\n", anchor->Anchor_File_Name);
    printf("DB_File_Idx= %d\n", anchor->DB_File_Idx_);
    printf("Begin_Chkpt_lsn = (%d|%d)\n",
            anchor->Begin_Chkpt_lsn_.File_No_,
            anchor->Begin_Chkpt_lsn_.Offset_);
    printf("End_Chkpt_lsn = (%d|%d)\n",
            anchor->End_Chkpt_lsn_.File_No_, anchor->End_Chkpt_lsn_.Offset_);
    printf("###############################################\n");
}

static int
dump_loganchor(char *loganame)
{
    int ret = 0;
    int fd = -1;
    int size;
    struct LogAnchor anchor;

    fd = sc_open(loganame, O_RDONLY | O_BINARY, OPEN_MODE);
    if (fd == -1)
    {
        printf("open() fail..\n");
        ret = -1;
        goto done;
    }

    if (sc_lseek(fd, 0, SEEK_SET) != 0)
    {
        printf("lseek() fail..\n");
        ret = -1;
        goto done;
    }

    size = sc_read(fd, &anchor, sizeof(struct LogAnchor));
    if (size != sizeof(struct LogAnchor))
    {
        printf("loganchor read() fail..\n");
        ret = -1;
        goto done;
    }

    print_log_anchor(&anchor);

  done:
    if (fd != -1)
        sc_close(fd);
    return ret;
}


static char *
get_log_op_type(int op_type)
{
    switch (op_type)
    {
    case INSERT_HEAPREC:
        return "INSERT_HEAPREC";
    case REMOVE_HEAPREC:
        return "REMOVE_HEAPREC";
    case UPDATE_HEAPREC:
        return "UPDATE_HEAPREC";
    case UPDATE_HEAPFLD:
        return "UPDATE_HEAPFLD";
    case INSERT_VARDATA:
        return "INSERT_VARDATA";
    case REMOVE_VARDATA:
        return "REMOVE_VARDATA";
    case TEMPRELATION_CREATE:
        return "TEMPRELATION_CREATE";
    case TEMPRELATION_DROP:
        return "TEMPRELATION_DROP";
    case TEMPINDEX_CREATE:
        return "TEMPINDEX_CREATE";
    case TEMPINDEX_DROP:
        return "TEMPINDEX_DROP";
    case RELATION_CREATE:
        return "RELATION_CREATE";
    case RELATION_DROP:
        return "RELATION_DROP";
    case RELATION_UPDATE:
        return "RELATION_UPDATE";
    case AUTOINCREMENT:
        return "AUTOINCREMENT";
    case INDEX_CREATE:
        return "INDEX_CREATE";
    case INDEX_DROP:
        return "INDEX_DROP";
    case RELATION:
        return "RELATION";
    case INDEX:
        return "INDEX";
    case RELATION_CATALOG:
        return "RELATION_CATALOG";
    case INDEX_CATALOG:
        return "INDEX_CATALOG";
    case SYS_ANCHOR:
        return "SYS_ANCHOR";
    case SYS_MEMBASE:
        return "SYS_MEMBASE";
    case SYS_PAGELINK:
        return "SYS_PAGELINK";
    case SYS_SLOTLINK:
        return "SYS_SLOTLINK";
    case SYS_SLOTUSE_BIT:
        return "SYS_SLOTUSE_BIT";
    case SYS_PAGEINIT:
        return "SYS_PAGEINIT";
    case RELATION_CATALOG_PAGEALLOC:
        return "RELATION_CATALOG_PAGEALLOC";
    case INDEX_CATALOG_PAGEALLOC:
        return "INDEX_CATALOG_PAGEALLOC";
    case RELATION_PAGEALLOC:
        return "RELATION_PAGEALLOC";
    case RELATION_CATALOG_PAGEDEALLOC:
        return "RELATION_CATALOG_PAGEDEALLOC";
    case INDEX_CATALOG_PAGEDEALLOC:
        return "INDEX_CATALOG_PAGEDEALLOC";
    case RELATION_PAGEDEALLOC:
        return "RELATION_PAGEDEALLOC";
    case RELATION_PAGETRUNCATE:
        return "RELATION_PAGETRUNCATE";
    default:
        return "UNKNOWN_OP_TYPE";
    }
}


static void
print_log_record(struct LogRec *logrec)
{
    int len;

    switch (logrec->header_.type_)
    {
    case TRANS_BEGIN_LOG:
        /* print log header information */
        printf("[%d|%d(%x)] %d BEGI_TRANS %10s %d\n",
                logrec->header_.record_lsn_.File_No_,
                logrec->header_.record_lsn_.Offset_,
                logrec->header_.record_lsn_.Offset_,
                logrec->header_.trans_id_, "", logrec->header_.lh_length_);
        break;

    case TRANS_COMMIT_LOG:
        /* print log header information */
        printf("[%d|%d(%x)] %d COMM_TRANS %10s %d\n",
                logrec->header_.record_lsn_.File_No_,
                logrec->header_.record_lsn_.Offset_,
                logrec->header_.record_lsn_.Offset_,
                logrec->header_.trans_id_, "", logrec->header_.lh_length_);
        break;

    case TRANS_ABORT_LOG:
        /* print log header information */
        printf("[%d|%d(%x)] %d ABRT_TRANS %10s %d\n",
                logrec->header_.record_lsn_.File_No_,
                logrec->header_.record_lsn_.Offset_,
                logrec->header_.record_lsn_.Offset_,
                logrec->header_.trans_id_, "", logrec->header_.lh_length_);
        break;

    case BEGIN_CHKPT_LOG:
        /* print log header information */
        if (logrec->header_.op_type_ == 1)
            printf("\n    NEW START\n\n");
        printf("[%d|%d(%x)] %d BEGI_CHKPT %10s %d\n",
                logrec->header_.record_lsn_.File_No_,
                logrec->header_.record_lsn_.Offset_,
                logrec->header_.record_lsn_.Offset_,
                logrec->header_.trans_id_, "", logrec->header_.lh_length_);
        break;

    case INDEX_CHKPT_LOG:
        /* print log header information */
        printf("[%d|%d(%x)] %d INDX_CHKPT %10s %d\n",
                logrec->header_.record_lsn_.File_No_,
                logrec->header_.record_lsn_.Offset_,
                logrec->header_.record_lsn_.Offset_,
                logrec->header_.trans_id_, "", logrec->header_.lh_length_);
        break;

    case END_CHKPT_LOG:
        /* print log header information */
        printf("[%d|%d(%x)] %d ENDD_CHKPT %10s %d\n",
                logrec->header_.record_lsn_.File_No_,
                logrec->header_.record_lsn_.Offset_,
                logrec->header_.record_lsn_.Offset_,
                logrec->header_.trans_id_, "", logrec->header_.lh_length_);
        {
            int trans_count = *(int *) (logrec->data1);
            int i;
            struct Trans4Log trans;

            printf("\t No. of trans = %d\n", trans_count);
            for (i = 0; i < trans_count; i++)
            {
                sc_memcpy(&trans,
                        logrec->data1 + sizeof(int) +
                        i * sizeof(struct Trans4Log),
                        sizeof(struct Trans4Log));
                printf("\t\t%d\n", trans.trans_id_);
            }
        }
        break;

    case PHYSICAL_LOG:
        /* print log header information */
        printf("[%d|%d] %d PHYSIC_LOG %10s %lu (%ld|%ld|%ld) %d\n",
                logrec->header_.record_lsn_.File_No_,
                logrec->header_.record_lsn_.Offset_,
                logrec->header_.trans_id_,
                get_log_op_type(logrec->header_.op_type_),
                logrec->header_.oid_,
                getSegmentNo(logrec->header_.oid_),
                getPageNo(logrec->header_.oid_),
                OID_GetOffSet(logrec->header_.oid_),
                logrec->header_.lh_length_);

        if (logrec->header_.op_type_ == RELATION_CREATE)
        {
            struct Container *cont;

            cont = (struct Container *) ((char *) logrec->data1 +
                    sizeof(struct Container));
            printf("\t table_name=%s, table_id=%d,cont_oid=%ld(%ld|%ld|%ld)\n",
                    cont->base_cont_.name_, cont->base_cont_.id_,
                    cont->collect_.cont_id_,
                    getSegmentNo(cont->collect_.cont_id_),
                    getPageNo(cont->collect_.cont_id_),
                    OID_GetOffSet(cont->collect_.cont_id_));
        }
        else if (logrec->header_.op_type_ == INDEX_CREATE)
        {
            struct TTree *ttree;

            ttree = (struct TTree *) ((char *) logrec->data1 +
                    sizeof(struct TTree));
            printf("\t index_name=%s, index_id=%d\n",
                    ttree->base_cont_.name_, ttree->base_cont_.id_);
        }
        if (logrec->header_.op_type_ == TEMPRELATION_CREATE)
        {
            struct Container *cont;

            cont = (struct Container *) ((char *) logrec->data1 +
                    sizeof(struct Container));
            printf("\t temp table, cont_oid=%ld(%ld|%ld|%ld)\n",
                    cont->collect_.cont_id_,
                    getSegmentNo(cont->collect_.cont_id_),
                    getPageNo(cont->collect_.cont_id_),
                    OID_GetOffSet(cont->collect_.cont_id_));
        }
        else if (logrec->header_.op_type_ == TEMPINDEX_CREATE)
        {
            printf("\t temp index\n");
        }
        break;

    case PAGEALLOC_LOG:
        {
            OID cont_oid = (OID) logrec->header_.relation_pointer_;
            OID head_pid = logrec->header_.oid_;
            OID tail_pid = logrec->header_.oid_;
            OID next_pid = logrec->header_.oid_;

            sc_memcpy(&tail_pid, logrec->data1, sizeof(OID));
            sc_memcpy(&next_pid, logrec->data1 + sizeof(OID), sizeof(OID));

            /* print log header information */
            printf("[%d|%d] %d PG_ALLOC__ %s %lu (%ld|%ld|%ld) %d\n",
                    logrec->header_.record_lsn_.File_No_,
                    logrec->header_.record_lsn_.Offset_,
                    logrec->header_.trans_id_,
                    get_log_op_type(logrec->header_.op_type_),
                    logrec->header_.oid_,
                    getSegmentNo(logrec->header_.oid_),
                    getPageNo(logrec->header_.oid_),
                    OID_GetOffSet(logrec->header_.oid_),
                    logrec->header_.lh_length_);
            printf("\tmem_anchor_->free_page_count_ = %d\n",
                    logrec->header_.recovery_leng_);
            printf("\t cont_oid=%ld(%ld|%ld|%ld)\n",
                    cont_oid,
                    getSegmentNo(cont_oid), getPageNo(cont_oid),
                    OID_GetOffSet(cont_oid));
            printf("\t new pid=%ld(%ld|%ld), prev tail=%ld(%ld|%ld), "
                    "free next=%ld(%ld|%ld)\n",
                    head_pid, getSegmentNo(head_pid), getPageNo(head_pid),
                    tail_pid, getSegmentNo(tail_pid),
                    getPageNo(tail_pid),
                    next_pid, getSegmentNo(next_pid), getPageNo(next_pid));
        }
        break;

    case PAGEDEALLOC_LOG:
        /* print log header information */
        printf("[%d|%d] %d PG_DEALLOC %10s %lu (%ld|%ld|%ld) %d\n",
                logrec->header_.record_lsn_.File_No_,
                logrec->header_.record_lsn_.Offset_,
                logrec->header_.trans_id_,
                get_log_op_type(logrec->header_.op_type_),
                logrec->header_.oid_,
                getSegmentNo(logrec->header_.oid_),
                getPageNo(logrec->header_.oid_),
                OID_GetOffSet(logrec->header_.oid_),
                logrec->header_.lh_length_);
        break;

    case SEGMENTALLOC_LOG:
        /* print log header information */
        printf("[%d|%d] %d SG_ALLOC__ %10s %d\n",
                logrec->header_.record_lsn_.File_No_,
                logrec->header_.record_lsn_.Offset_,
                logrec->header_.trans_id_, "", logrec->header_.lh_length_);

        /* print log body information */
        {
            OID prev_pid, sn;

            prev_pid = *(OID *) (logrec->data1);
            sn = *(OID *) (logrec->data1 + sizeof(OID));
            printf("\t prev_pid=(%ld|%ld), segment_num=%ld, "
                    "next_pid=(%ld|%ld)\n",
                    getSegmentNo(prev_pid), getPageNo(prev_pid), sn,
                    getSegmentNo(logrec->header_.oid_),
                    getPageNo(logrec->header_.oid_));
        }
        break;

    case BEFORE_LOG:
        /* print log header information */
        printf("[%d|%d] %d BEFORE_LOG %10s %lu (%ld|%ld|%ld) %d\n",
                logrec->header_.record_lsn_.File_No_,
                logrec->header_.record_lsn_.Offset_,
                logrec->header_.trans_id_,
                get_log_op_type(logrec->header_.op_type_),
                logrec->header_.oid_,
                getSegmentNo(logrec->header_.oid_),
                getPageNo(logrec->header_.oid_),
                OID_GetOffSet(logrec->header_.oid_),
                logrec->header_.lh_length_);
        break;

    case AFTER_LOG:
        /* print log header information */
        printf("[%d|%d] %d AFTER_LOG_ %10s %lu (%ld|%ld|%ld) %d\n",
                logrec->header_.record_lsn_.File_No_,
                logrec->header_.record_lsn_.Offset_,
                logrec->header_.trans_id_,
                get_log_op_type(logrec->header_.op_type_),
                logrec->header_.oid_,
                getSegmentNo(logrec->header_.oid_),
                getPageNo(logrec->header_.oid_),
                OID_GetOffSet(logrec->header_.oid_),
                logrec->header_.lh_length_);
        break;

    case INSERT_LOG:
        /* print log header information */
        printf("[%d|%d(%x)] %d INSERT_LOG %10s %lu (%ld|%ld|%ld) %d\n",
                logrec->header_.record_lsn_.File_No_,
                logrec->header_.record_lsn_.Offset_,
                logrec->header_.record_lsn_.Offset_,
                logrec->header_.trans_id_,
                get_log_op_type(logrec->header_.op_type_),
                logrec->header_.oid_,
                getSegmentNo(logrec->header_.oid_),
                getPageNo(logrec->header_.oid_),
                OID_GetOffSet(logrec->header_.oid_),
                logrec->header_.lh_length_);

        /* print table id */
        printf("\t tab_id=%d\n", logrec->header_.tableid_);

        {
            struct SlotUsedLogData s_log_data;

            sc_memcpy(&s_log_data,
                    logrec->data1 + logrec->header_.recovery_leng_
                    - SLOTUSEDLOGDATA_SIZE, SLOTUSEDLOGDATA_SIZE);
            printf("\t collect new item count = %d\n",
                    s_log_data.collect_new_item_count);
            printf("\t page new record count = %d\n",
                    s_log_data.page_new_record_count);
            printf("\t page new free slot id = %ld\n",
                    s_log_data.page_new_free_slot_id);
            printf("\t page cur free page next = %ld\n",
                    s_log_data.page_cur_free_page_next);
        }

        /* print some body information */
        if (logrec->header_.tableid_ == 0)
        {
            struct Container *cont;

            cont = (struct Container *) logrec->data1;
            printf("\t record=(%d, %s, %d, %d, ...)\n",
                    cont->base_cont_.id_, cont->base_cont_.name_,
                    cont->collect_.numfields_, cont->collect_.record_size_);
        }
        else if (logrec->header_.tableid_ == 1)
        {
            SYSTABLE_T *table;

            table = (SYSTABLE_T *) logrec->data1;
            printf("\t record=(%d, %s, %d, ...)\n",
                    table->tableId, table->tableName, table->numFields);
        }
        else if (logrec->header_.tableid_ == 2)
        {
            SYSFIELD_T *field;

            field = (SYSFIELD_T *) logrec->data1;
            printf("\t record=(%d, %d, %s, ...)\n",
                    field->fieldId, field->tableId, field->fieldName);
        }
        else if (logrec->header_.tableid_ == 3)
        {
            SYSINDEX_T *index;

            index = (SYSINDEX_T *) logrec->data1;
            printf("\t record=(%d, %d, %d, %d, %s, ...)\n",
                    index->tableId, index->indexId, index->type,
                    index->numFields, index->indexName);
        }
        else if (logrec->header_.tableid_ == 4)
        {
            SYSINDEXFIELD_T *indexfield;

            indexfield = (SYSINDEXFIELD_T *) logrec->data1;
            printf("\t record=(%d, %d, %d, ...)\n",
                    indexfield->tableId, indexfield->indexId,
                    indexfield->fieldId);

        }
        else
        {
        }
        break;

    case DELETE_LOG:
        /* print log header information */
        printf("[%d|%d(%x)] %d DELETE_LOG %10s %lu (%ld|%ld|%ld) %d\n",
                logrec->header_.record_lsn_.File_No_,
                logrec->header_.record_lsn_.Offset_,
                logrec->header_.record_lsn_.Offset_,
                logrec->header_.trans_id_,
                get_log_op_type(logrec->header_.op_type_),
                logrec->header_.oid_,
                getSegmentNo(logrec->header_.oid_),
                getPageNo(logrec->header_.oid_),
                OID_GetOffSet(logrec->header_.oid_),
                logrec->header_.lh_length_);

        /* print table id */
        printf("\t tab_id=%d\n", logrec->header_.tableid_);

        {
            struct SlotFreeLogData s_log_data;

            sc_memcpy(&s_log_data,
                    logrec->data1 + logrec->header_.recovery_leng_
                    - SLOTUSEDLOGDATA_SIZE, SLOTUSEDLOGDATA_SIZE);
            printf("\t collect new item count = %d\n",
                    s_log_data.collect_new_item_count);
            printf("\t page new record count = %d\n",
                    s_log_data.page_new_record_count);
            printf("\t new page curr free slot id = %lu (0x%lx)\n",
                    s_log_data.page_cur_free_slot_id,
                    s_log_data.page_cur_free_slot_id);
            printf("\t new free page next = %lu (0x%lx)\n",
                    s_log_data.page_new_free_page_next,
                    s_log_data.page_new_free_page_next);
        }

        if (logrec->header_.tableid_ == 0)
        {
            struct TTree *ttree;
            struct SlotFreeLogData *s_log_data;

            ttree = (struct TTree *) logrec->data1;
            printf("\t record=(%d, %s, ...)\n",
                    ttree->base_cont_.id_, ttree->base_cont_.name_);

            s_log_data = (struct SlotFreeLogData *) (logrec->data1 +
                    logrec->header_.recovery_leng_ - SLOTFREELOGDATA_SIZE);
            printf("\t new item count = %d\n",
                    s_log_data->collect_new_item_count);
            printf("\t new page record count = %d\n",
                    s_log_data->page_new_record_count);
            printf("\t new page curr free slot id = %lu (0x%lx)\n",
                    s_log_data->page_cur_free_slot_id,
                    s_log_data->page_cur_free_slot_id);
            printf("\t new free page next = %lu (0x%lx)\n",
                    s_log_data->page_new_free_page_next,
                    s_log_data->page_new_free_page_next);
        }
        else if (logrec->header_.tableid_ == 1 &&
                logrec->header_.op_type_ == REMOVE_HEAPREC)
        {
            SYSTABLE_T *table;

            table = (SYSTABLE_T *) logrec->data1;
            printf("\t record=(%d, %s, %d, ...)\n",
                    table->tableId, table->tableName, table->numFields);
        }
        else if (logrec->header_.tableid_ == 2 &&
                logrec->header_.op_type_ == REMOVE_HEAPREC)
        {
            SYSFIELD_T *field;

            field = (SYSFIELD_T *) logrec->data1;
            printf("\t record=(%d, %d, %s, ...)\n",
                    field->fieldId, field->tableId, field->fieldName);
        }
        else if (logrec->header_.tableid_ == 3 &&
                logrec->header_.op_type_ == REMOVE_HEAPREC)
        {
            SYSINDEX_T *index;

            index = (SYSINDEX_T *) logrec->data1;
            printf("\t record=(%d, %d, %d, %d, %s, ...)\n",
                    index->tableId, index->indexId, index->type,
                    index->numFields, index->indexName);
        }
        else if (logrec->header_.tableid_ == 4 &&
                logrec->header_.op_type_ == REMOVE_HEAPREC)
        {
            SYSINDEXFIELD_T *indexfield;

            indexfield = (SYSINDEXFIELD_T *) logrec->data1;
            printf("\t record=(%d, %d, %d, ...)\n",
                    indexfield->tableId, indexfield->indexId,
                    indexfield->fieldId);
        }
        break;

    case UPDATE_LOG:
        /* print log header information */
        printf("[%d|%d(%x)] %d UPDATE_LOG %10s %lu (%ld|%ld|%ld) %d\n",
                logrec->header_.record_lsn_.File_No_,
                logrec->header_.record_lsn_.Offset_,
                logrec->header_.record_lsn_.Offset_,
                logrec->header_.trans_id_,
                get_log_op_type(logrec->header_.op_type_),
                logrec->header_.oid_,
                getSegmentNo(logrec->header_.oid_),
                getPageNo(logrec->header_.oid_),
                OID_GetOffSet(logrec->header_.oid_),
                logrec->header_.lh_length_);

        /* print table id */
        printf("\t tab_id=%d\n", logrec->header_.tableid_);

        /* print some body information */
        len = logrec->header_.recovery_leng_ / 2;
        if (logrec->header_.tableid_ == 1)
        {
            SYSTABLE_T *table;

            table = (SYSTABLE_T *) & logrec->data1[0];
            printf("\t oldrec=(%d, %s, %d, ...)\n",
                    table->tableId, table->tableName, table->numFields);
            table = (SYSTABLE_T *) & logrec->data1[len];
            printf("\t newrec=(%d, %s, %d, ...)\n",
                    table->tableId, table->tableName, table->numFields);
        }
        else if (logrec->header_.tableid_ == 2)
        {
            SYSFIELD_T *field;

            field = (SYSFIELD_T *) & logrec->data1[0];
            printf("\t oldrec=(%d, %d, %s, ...)\n",
                    field->fieldId, field->tableId, field->fieldName);
            field = (SYSFIELD_T *) & logrec->data1[len];
            printf("\t newrec=(%d, %d, %s, ...)\n",
                    field->fieldId, field->tableId, field->fieldName);
        }
        else if (logrec->header_.tableid_ == 3)
        {
            SYSINDEX_T *index;

            index = (SYSINDEX_T *) & logrec->data1[0];
            printf("\t oldrec=(%d, %d, %d, %d, %s, ...)\n",
                    index->tableId, index->indexId, index->type,
                    index->numFields, index->indexName);
            index = (SYSINDEX_T *) & logrec->data1[len];
            printf("\t newrec=(%d, %d, %d, %d, %s, ...)\n",
                    index->tableId, index->indexId, index->type,
                    index->numFields, index->indexName);
        }
        else if (logrec->header_.tableid_ == 4)
        {
            SYSINDEXFIELD_T *indexfield;

            indexfield = (SYSINDEXFIELD_T *) & logrec->data1[0];
            printf("\t oldrec=(%d, %d, %d, ...)\n",
                    indexfield->tableId, indexfield->indexId,
                    indexfield->fieldId);
            indexfield = (SYSINDEXFIELD_T *) & logrec->data1[len];
            printf("\t newrec=(%d, %d, %d, ...)\n",
                    indexfield->tableId, indexfield->indexId,
                    indexfield->fieldId);
        }
        else if (logrec->header_.tableid_ == 10)        /* syssequences */
        {
            SYSSEQUENCE_REC_T *sequence;

            sequence = (SYSSEQUENCE_REC_T *) & logrec->data1[0];
            printf("\t oldrec=(%d, %s, %d, ...)\n",
                    sequence->sequenceId, sequence->sequenceName,
                    sequence->lastNum);
            sequence = (SYSSEQUENCE_REC_T *) & logrec->data1[len];
            printf("\t newrec=(%d, %s, %d, ...)\n",
                    sequence->sequenceId, sequence->sequenceName,
                    sequence->lastNum);
        }
        else
        {
        }
        break;

    case UPDATEFIELD_LOG:
        /* print log header information */
        printf("[%d|%d(%x)] %d UPDFLD_LOG %10s %lu (%ld|%ld|%ld) %d\n",
                logrec->header_.record_lsn_.File_No_,
                logrec->header_.record_lsn_.Offset_,
                logrec->header_.record_lsn_.Offset_,
                logrec->header_.trans_id_,
                get_log_op_type(logrec->header_.op_type_),
                logrec->header_.oid_,
                getSegmentNo(logrec->header_.oid_),
                getPageNo(logrec->header_.oid_),
                OID_GetOffSet(logrec->header_.oid_),
                logrec->header_.lh_length_);

        /* print table id */
        printf("\t tab_id=%d\n", logrec->header_.tableid_);
        /* update field 내용 출력.. 4 byte.. */
        {
            DB_UINT32 field_count, i, pp;
            int log_offset;
            struct UpdateFieldInfo fldinfo;

            sc_memcpy((char *) &field_count, logrec->data1, 4);
            log_offset = 4;

            for (i = 0; i < field_count; i++)
            {
                sc_memcpy((char *) &fldinfo, logrec->data1 + log_offset, 4);
                log_offset += 4;

                if (fldinfo.length == 4)
                {
                    printf("\t   %04d: ", fldinfo.offset);

                    /* before */
                    sc_memcpy(&pp, logrec->data1 + log_offset, fldinfo.length);
                    printf("%d(0x%x) -> ", pp, pp);

                    /* after */
                    sc_memcpy(&pp,
                            logrec->data1 + log_offset + fldinfo.length,
                            fldinfo.length);

                    printf("%d(0x%x)\n", pp, pp);
                }

                log_offset += (fldinfo.length * 2);
            }
        }
        break;

    case COMPENSATION_LOG:
        /* print log header information */
        printf("[%d|%d(%x)] %d COMPENSATI %10s %lu (%ld|%ld|%ld) %d\n",
                logrec->header_.record_lsn_.File_No_,
                logrec->header_.record_lsn_.Offset_,
                logrec->header_.record_lsn_.Offset_,
                logrec->header_.trans_id_, "",
                logrec->header_.oid_,
                getSegmentNo(logrec->header_.oid_),
                getPageNo(logrec->header_.oid_),
                OID_GetOffSet(logrec->header_.oid_),
                logrec->header_.lh_length_);
        break;

    default:
        /* print log header information */
        printf("[%d|%d] %d UNDEF(%d) %d %lu (%ld|%ld|%ld) %d %d\n",
                logrec->header_.record_lsn_.File_No_,
                logrec->header_.record_lsn_.Offset_,
                logrec->header_.trans_id_,
                logrec->header_.type_,
                logrec->header_.op_type_,
                logrec->header_.oid_,
                getSegmentNo(logrec->header_.oid_),
                getPageNo(logrec->header_.oid_),
                OID_GetOffSet(logrec->header_.oid_),
                logrec->header_.recovery_leng_, logrec->header_.lh_length_);
        break;
    }

    printf(" \t trans_prev_lsn_ (%d,%d)\n",
            logrec->header_.trans_prev_lsn_.File_No_,
            logrec->header_.trans_prev_lsn_.Offset_);
}

static int
skip_logfile(int fd)
{
    int check;
    int size;

    printf("<<< skip >>>\n");
    while (1)
    {
        size = sc_read(fd, &check, 4);
        if (size != 4)
            return -1;
        if (check == 0x0d0e0d0e)
            return 0;
    }
}

static int
dump_logfile(char *logfname)
{
    int ret = 0;
    int fd = -1;
    int size;
    struct LogRec logrec;
    int file_num = 0;
    int file_offset;

    logrec.data1 = sc_malloc(32 * 1024);
    if (logrec.data1 == NULL)
    {
        printf("open() fail.. fd = %d\n", fd);
        return -1;
    }

    fd = sc_open(logfname, O_RDONLY | O_BINARY, OPEN_MODE);
    if (fd == -1)
    {
        printf("open() fail..\n");
        ret = -1;
        goto done;
    }

    if (sc_lseek(fd, 0, SEEK_SET) != 0)
    {
        printf("lseek() fail..\n");
        ret = -1;
        goto done;
    }

    printf("########################################################\n");
    printf("  LSN  trans_id  log_type  log_op_type  (OID)  length  \n");
    printf("########################################################\n");
    while (1)
    {
        file_offset = sc_lseek(fd, 0, SEEK_CUR);

        size = sc_read(fd, &logrec.header_, LOG_HDR_SIZE);

        if (sc_memcmp(&logrec.header_, Str_EndOfLog,
                        sizeof(Str_EndOfLog)) == 0)
        {
            printf("############### End of Log File ###############\n");
        }

        if (size != LOG_HDR_SIZE)
        {
            break;
        }

        if (file_offset != logrec.header_.record_lsn_.Offset_)
        {
            printf("==> offset mismatch %d(%x) %d(%x)\n",
                    file_offset, file_offset,
                    logrec.header_.record_lsn_.Offset_,
                    logrec.header_.record_lsn_.Offset_);
            if (logrec.header_.record_lsn_.Offset_ < 0)
            {   /* skip file */
                if (skip_logfile(fd) < 0)
                    break;

                continue;
            }
        }

        if (logrec.header_.lh_length_ > 32 * 1024)
        {
            printf("data length too long %d\n", logrec.header_.lh_length_);
            if (skip_logfile(fd) < 0)
                break;

            continue;
        }

        size = sc_read(fd, logrec.data1, logrec.header_.lh_length_);
        if (size != logrec.header_.lh_length_)
        {
            printf("==> read logrec body fail.. len:%d\n",
                    logrec.header_.lh_length_);
            if (skip_logfile(fd) < 0)
                break;

            continue;
        }
        print_log_record(&logrec);

        if (IS_CRASHED_LOGRECORD(&logrec.header_))
        {
            printf("  ==> crashed\n");
            if (skip_logfile(fd) < 0)
                break;

            continue;
        }

        {
            char *check_ptr = logrec.data1 + logrec.header_.lh_length_ - 4;

            if (check_ptr[0] != 0xE || check_ptr[1] != 0xD ||
                    check_ptr[2] != 0xE || check_ptr[3] != 0xD)
            {
                printf("  ==> body crashed\n");
                continue;
            }
        }

        if (file_num == 0)
            file_num = logrec.header_.record_lsn_.File_No_;
    }
    printf("########################################################\n");

  done:
    if (fd != -1)
        sc_close(fd);
    if (logrec.data1)
        sc_free(logrec.data1);
    return ret;
}

int
main(int argc, char *argv[])
{
    char dbname[256];
    char loganame[256];
    char logfname[256];
    int logfnum;

    printf("\nlogdump for %s %s\n", _DBNAME_, _DBVERSION_);
    printf("%s\n\n", _COPYRIGHT_);

    if (argc != 3)
    {
        printf("Usage: logdump database_name logfile_number\n");
        printf("Exam.) logdump testdb 2 > testdb_logfile2.txt\n\n");
        exit(-1);
    }

    sc_strcpy(dbname, argv[1]);
    logfnum = sc_atoi(argv[2]);

    if (logfnum < 0)
    {
        printf("Logfile Number(%d) is smaller than 0.\n", logfnum);
        exit(-1);
    }

#if defined(WIN32)
    do
    {
        // MOBILE_LITE_CONFIG 환경변수 설정을 위해...
        char dbfilename[512];

        __check_dbname(dbname, dbfilename);
    } while (0);
#endif

    if (logfnum == 0)   /* dump loganchor */
    {
        if (dbname[0] == DIR_DELIM_CHAR)
        {
            char *ptr = strrchr(dbname, DIR_DELIM_CHAR);

            *ptr = '\0';
            sprintf(loganame, "%s%c%s_loganchor", dbname, DIR_DELIM_CHAR,
                    ptr + 1);
        }
        else
            sprintf(loganame, "%s%c%s_loganchor",
                    sc_getenv("MOBILE_LITE_CONFIG"), DIR_DELIM_CHAR, dbname);
        printf("\nloganchorname = %s\n", loganame);

        if (dump_loganchor(loganame) < 0)
        {
            printf("dump_loganchor Fail.\n");
            exit(-1);
        }
    }
    else    /* logfnum > 0 : dump logfile */
    {
        if (dbname[0] == DIR_DELIM_CHAR /* '/' */ )
        {
            char *ptr = strrchr(dbname, DIR_DELIM_CHAR);

            *ptr = '\0';
            sprintf(logfname, "%s%c%s_logfile%d", dbname, DIR_DELIM_CHAR,
                    ptr + 1, logfnum);
        }
        else
            sprintf(logfname, "%s%c%s_logfile%d",
                    sc_getenv("MOBILE_LITE_CONFIG"), DIR_DELIM_CHAR, dbname,
                    logfnum);
        printf("\nlogfilename = %s\n", logfname);

        if (dump_logfile(logfname) < 0)
        {
            printf("dump_logfile Fail.\n");
            exit(-1);
        }
    }

    exit(0);
}

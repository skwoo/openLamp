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

int
dbi_Checkpoint(int connid)
{
    int *pTransid;
    extern int INTSQL_closetransaction(int handle, int mode);
    int ret = DB_SUCCESS;

    DBI_CHECK_SERVER_TERM();

    pTransid = (int *) CS_getspecific(my_trans_id);
    /* try commit */
    if (pTransid && *pTransid != -1)
    {
        INTSQL_closetransaction(connid, 0);
        dbi_Trans_Commit(connid);
    }

    /* if not committed, return error */
    if (pTransid && *pTransid != -1)
    {
        ret = DB_E_NOTCOMMITTED;
        goto end;
    }

    _Checkpointing(0, 1);

  end:

    return ret;
}

int
dbi_AddVolume(int connid, int numsegment, int idxseg_num, int tmpseg_num)
{
    int ret = DB_SUCCESS;

    int volume_size;

    DBI_CHECK_SERVER_TERM();

#ifdef CHECK_DBI_PARAM
    if (numsegment < 0 || idxseg_num < 0 || tmpseg_num < 0)
        return DB_E_INVALIDPARAM;
#endif

    if (numsegment)
    {
        volume_size = DBFile_GetVolumeSize(DBFILE_DATA0_TYPE);

        if (volume_size <= 0)
        {
            ret = volume_size;
            goto end;
        }

        volume_size /= (SEGMENT_SIZE);

        if (DB_Params.max_segments &&
                numsegment + volume_size >= DB_Params.max_segments)
        {
            ret = DB_E_LIMITDBSIZE;
            goto end;
        }

        if (volume_size + numsegment >= SEGMENT_NO)
        {
            ret = DB_E_LIMITDBSIZE;
            goto end;
        }

        ret = DBFile_AddVolume(DBFILE_DATA0_TYPE, numsegment);
    }

    if (idxseg_num)
    {
        volume_size = DBFile_GetVolumeSize(DBFILE_INDEX_TYPE);

        if (volume_size <= 0)
        {
            ret = volume_size;
            goto end;
        }

        volume_size /= (SEGMENT_SIZE);

        ret = DBFile_AddVolume(DBFILE_INDEX_TYPE, idxseg_num);
    }

    if (tmpseg_num)
    {
        volume_size = DBFile_GetVolumeSize(DBFILE_TEMP_TYPE);

        if (volume_size <= 0)
        {
            ret = volume_size;
            goto end;
        }

        volume_size /= (SEGMENT_SIZE);

        ret = DBFile_AddVolume(DBFILE_TEMP_TYPE, tmpseg_num);
    }

  end:
    return ret;
}

int
dbi_AddSegment(int connid, int numsegment, int numIdxSegment)
{
    int i;
    int seg_num;
    OID pageid;

    OID pageid_idx = NULL_OID;
    int sn;
    struct Page *page;
    int *pTransid;
    int curr_transid = 0;


    DBI_CHECK_SERVER_TERM();

#ifdef CHECK_DBI_PARAM
    if (numsegment <= 0)
        return DB_E_INVALIDPARAM;
#endif

    seg_num = numsegment + mem_anchor_->allocated_segment_no_;

    if (DB_Params.max_segments && seg_num >= DB_Params.max_segments)
        return DB_E_LIMITDBSIZE;

    if (seg_num >= SEGMENT_NO)
        return DB_E_LIMITDBSIZE;

    pTransid = (int *) CS_getspecific(my_trans_id);
    if (pTransid == NULL)
        return DB_E_INVALIDPARAM;

    curr_transid = *pTransid;
    if (curr_transid == -1)
    {
        i = dbi_Trans_Begin(connid);
        if (i < 0)
            goto end;
    }

    i = 0;

    pageid = mem_anchor_->first_free_page_id_;

    if (pageid == NULL_OID)
    {
        if (MemMgr_Check_FreeSegment() < 0)
            goto end;
        i++;
        pageid = mem_anchor_->first_free_page_id_;
    }

    while (1)
    {
        page = (struct Page *) PageID_Pointer(pageid);
        if (page == NULL)
        {
            i = DB_E_OIDPOINTER;
            goto end;
        }

        if (page->header_.link_.next_ == NULL_OID)
            break;

        pageid = page->header_.link_.next_;
    }

    if (numIdxSegment > 0)
    {
        pageid_idx = mem_anchor_->ifirst_free_page_id_;
        while (1)
        {
            page = (struct Page *) Index_PageID_Pointer(pageid_idx);
            if (page == NULL)
            {
                i = DB_E_OIDPOINTER;
                goto end;
            }
            if (page->header_.link_.next_ == NULL_OID)
                break;
            pageid_idx = page->header_.link_.next_;
        }
    }

    dbi_DataMem_Protect_RW();

    for (; i < numsegment; i++)
    {
        sn = mem_anchor_->allocated_segment_no_;

        if (MemMgr_AllocateSegment(pageid, 0, SEGMENT_DATA) < 0)
            break;

        segment_[sn] = DBDataMem_V2P(Mem_mgr->segment_memid[sn]);
        pageid = segment_[sn]->page_[PAGE_PER_SEGMENT - 1].header_.self_;
    }
    if (numIdxSegment > 0)
    {
        for (i = 0; i < numIdxSegment; i++)
        {
            if (Index_MemMgr_AllocateSegment(pageid_idx) < 0)
                break;

            sn = mem_anchor_->iallocated_segment_no_;
            sn = SEGMENT_NO - sn;

            pageid_idx =
                    isegment_[sn]->page_[PAGE_PER_SEGMENT - 1].header_.self_;
        }
    }

    dbi_DataMem_Protect_RO();

  end:
    if (curr_transid == -1)
    {
        if (i < 0)
            dbi_Trans_Abort(connid);
        else
            dbi_Trans_Commit(connid);
    }

    _Checkpointing(0, 0);

    return i;
}

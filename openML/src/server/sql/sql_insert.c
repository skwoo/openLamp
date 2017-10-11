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
#include "mdb_compat.h"

#include "sql_main.h"

#include "sql_decimal.h"

#include "mdb_er.h"

int make_work_table(int *handle, T_SELECT * select);
int SQL_fetch(int *handle, T_SELECT * select, T_RESULT_INFO * result);
int close_all_open_tables(int, T_SELECT *);

int open_cursor(int connection, T_RTTABLE * rttable,
        T_NESTING * nest, EXPRDESC_LIST * cond, MDB_UINT8 mode);
int open_cursor_for_insertion(int connection, T_RTTABLE * rttable,
        MDB_UINT8 mode);

int close_cursor(int connection, T_RTTABLE * rttable);

char *datetime2char(char *str, char *datetime);
char *char2datetime(char *datetime, char *str, int str_len);
char *timestamp2char(char *str, t_astime * timestamp);
t_astime *char2timestamp(t_astime * timestamp, char *str, int str_len);
char *date2char(char *str, char *date);
char *char2date(char *date, char *str, int str_len);
char *time2char(char *str, char *time);
char *char2time(char *time, char *str, int str_len);
int calc_func_convert(T_VALUEDESC * base, T_VALUEDESC * src,
        T_VALUEDESC * res, MDB_INT32 is_init);

/*****************************************************************************/
//! set_insertrecord

/*! \breif  간단하게 어떤 함수인지스트용 \n
 *          두줄을 쓸때는 이런식으로
 ************************************
 * \param handle(in)        :
 * \param insert(in/out)    : 
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static int
set_insertrecord(int handle, T_INSERT * insert, T_SQLTYPE sqltype)
{
    T_COLDESC *col_list;
    T_EXPRESSIONDESC *val_list;
    T_RTTABLE *rttable;
    T_VALUEDESC res;
    char *target;
    int i, ret = RET_SUCCESS;
    int heap_check = 0;

    if (!IS_FIXED_RECORDS_TABLE(insert->tableinfo.maxRecords))
        if (insert->tableinfo.has_variabletype && sqltype != ST_UPSERT)
            heap_check = 1;

    col_list = insert->columns.list;
    val_list = insert->u.values.list;
    rttable = insert->rttable;

    if (col_list == 0)
    {
        for (i = 0; i < insert->tableinfo.numFields; i++)
        {
            if (heap_check)
                target = rttable->rec_buf + insert->fieldinfo[i].h_offset;
            else
                target = rttable->rec_buf + insert->fieldinfo[i].offset;
            res.call_free = 0;
            if (val_list[i].len == 1 &&
                    val_list[i].list[0]->u.value.valuetype != DT_NULL_TYPE)
            {
                sc_memcpy(&res, &(val_list[i].list[0]->u.value),
                        sizeof(T_VALUEDESC));
                res.call_free = 0;
            }
            else if (val_list[i].len)
            {
                ret = calc_expression(&val_list[i], &res, MDB_FALSE);
                if (ret < 0)
                    goto RETURN_LABEL;
            }
            if (val_list[i].len && !res.is_null)
            {   /* not null value */
                ret = copy_value_into_record(rttable->Hcursor, target, &res,
                        &insert->fieldinfo[i], heap_check);
                if (ret < 0)
                    goto RETURN_LABEL;

                if (res.call_free)
                    sql_value_ptrFree(&res);

            }
            else
            {   /* null value */
                if (insert->fieldinfo[i].flag & NULL_BIT
                        || insert->fieldinfo[i].flag & AUTO_BIT)
                {
                    DB_VALUE_SETNULL(rttable->rec_buf,
                            insert->fieldinfo[i].position,
                            rttable->nullflag_offset);
                }
                else
                {
                    ret = SQL_E_NOTALLOWEDNULLVALUE;
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
                    goto RETURN_LABEL;
                }
            }
        }

    }
    else
    {
        for (i = 0; i < insert->columns.len; i++)
        {
            if (heap_check)
                target = rttable->rec_buf + col_list[i].fieldinfo->h_offset;
            else
                target = rttable->rec_buf + col_list[i].fieldinfo->offset;
            res.call_free = 0;
            res.is_null = 0;
            if (val_list[i].len)
            {
                ret = calc_expression(&val_list[i], &res, MDB_FALSE);
                if (ret < 0)
                    goto RETURN_LABEL;
            }
            if (val_list[i].len && !res.is_null)
            {   /* not null value */
                ret = copy_value_into_record(rttable->Hcursor, target, &res,
                        col_list[i].fieldinfo, heap_check);
                if (ret < 0)
                    goto RETURN_LABEL;

                if (res.call_free)
                    sql_value_ptrFree(&res);

            }
            else
            {   /* null value */
                if (col_list[i].fieldinfo->flag & NULL_BIT
                        || col_list[i].fieldinfo->flag & AUTO_BIT)
                {
                    DB_VALUE_SETNULL(rttable->rec_buf,
                            col_list[i].fieldinfo->position,
                            rttable->nullflag_offset);
                }
                else
                {
                    ret = SQL_E_NOTALLOWEDNULLVALUE;
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
                    goto RETURN_LABEL;
                }
            }
        }
    }

  RETURN_LABEL:

    if (res.call_free)
        sql_value_ptrFree(&res);

    return ret;
}

/*****************************************************************************/
//! exec_insert_values

/*! \breif  insert 수행
 ************************************
 * \param handle(in)    :
 * \param stmt(in/out)  : 
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
static int
exec_insert_values(int handle, T_STATEMENT * stmt)
{
    T_RTTABLE *rttable;
    int ret = RET_SUCCESS;
    OID inserted_rid;

    rttable = stmt->u._insert.rttable;

    if (!rttable->table.tablename)
    {       // 전에 prepare & execute를 수행했다면..
        rttable->table.tablename = stmt->u._insert.table.tablename;
        rttable->table.aliasname = stmt->u._insert.table.aliasname;
        if ((!IS_FIXED_RECORDS_TABLE(stmt->u._insert.tableinfo.maxRecords))
                && stmt->sqltype != ST_UPSERT)
        {
            rttable->nullflag_offset = h_get_nullflag_offset(handle,
                    rttable->table.tablename, &stmt->u._insert.tableinfo);

            rttable->rec_len =
                    h_get_recordlen(handle, rttable->table.tablename,
                    &stmt->u._insert.tableinfo);
        }
        else
        {
            rttable->nullflag_offset = get_nullflag_offset(handle,
                    rttable->table.tablename, &stmt->u._insert.tableinfo);

            rttable->rec_len = get_recordlen(handle, rttable->table.tablename,
                    &stmt->u._insert.tableinfo);
        }

        if (rttable->rec_len < 0)
            return RET_ERROR;

        rttable->rec_buf = PMEM_ALLOC(sizeof(char) * rttable->rec_len);
    }
    else
        sc_memset(rttable->rec_buf, 0, rttable->rec_len);

    if (stmt->sqltype == ST_UPSERT && stmt->u._insert.is_upsert_msync)
    {
        rttable->msync_flag = SYNCED_SLOT;
    }

    if (rttable->status != SCS_OPEN)
    {
        ret = open_cursor_for_insertion(handle, &rttable[0], LK_EXCLUSIVE);
    }
    if (ret < 0)
        return RET_ERROR;

    ret = set_insertrecord(handle, &stmt->u._insert, stmt->sqltype);
    if (ret < 0)
        return RET_ERROR;

    if (rttable->status != SCS_OPEN)
        ret = open_cursor_for_insertion(handle, &rttable[0], LK_EXCLUSIVE);
    if (ret < 0)
        return RET_ERROR;

    if (stmt->sqltype == ST_UPSERT)
    {
        inserted_rid = 0;

        ret = dbi_Cursor_Upsert(handle, rttable->Hcursor,
                rttable->rec_buf, rttable->rec_len,
                stmt->u._insert.columns.ins_cols,
                &inserted_rid, UPSERT_CMP_DEFAULT);
    }
    else if (!IS_FIXED_RECORDS_TABLE(stmt->u._insert.tableinfo.maxRecords))
    {
        ret = dbi_Cursor_Insert(handle, rttable->Hcursor,
                rttable->rec_buf, rttable->rec_len, 1, &inserted_rid);
    }
    else
        ret = dbi_Cursor_Insert(handle, rttable->Hcursor,
                rttable->rec_buf, rttable->rec_len, 0, &inserted_rid);

    if (ret < 0)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
        ret = RET_ERROR;
    }
    else if (stmt->sqltype == ST_UPSERT)
    {
        stmt->u._insert.rows = ret;
        stmt->affected_rid = inserted_rid;
    }
    else
    {
        stmt->u._insert.rows++;
        stmt->affected_rid = inserted_rid;
    }

    if (ret < 0)
        close_cursor(handle, rttable);

    return ret;
}

/*****************************************************************************/
//! exec_insert_query

/*! \breif  ALTER 구문시, 새로운 TABLE에 INSERT 시킴
 ************************************
 * \param handle(in) :
 * \param stmt(in/out): 
 * \param param_name :
 * \param param_name :
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *  - default 값을 할당하고 해제를 안키기는 leak이 있었음
 *  - tmpval가 초기화가 안되서 문제를 발생시킬 수도 있다
 *  - BS or NBS의 경우 길이를 설정해줌
 *
 *****************************************************************************/
int
exec_insert_query(int handle, T_STATEMENT * stmt)
{
    T_SELECT *select;
    T_QUERYRESULT *qresult;
    T_RTTABLE *rttable;
    SYSFIELD_T *fieldinfo = NULL;
    T_LIST_COLDESC *col_list;
    T_VALUEDESC *val, tmpval;
    char *target;
    int fieldnum;
    int i, j, ret = RET_SUCCESS, ret_fetch;
    OID inserted_rid;

    rttable = stmt->u._insert.rttable;
    if (!rttable->table.tablename)
    {       // 전에 prepare & execute를 수행했다면..
        rttable->table.tablename = stmt->u._insert.table.tablename;
        rttable->table.aliasname = stmt->u._insert.table.aliasname;
        rttable->nullflag_offset = get_nullflag_offset(handle,
                rttable->table.tablename, &stmt->u._insert.tableinfo);
        rttable->rec_len = get_recordlen(handle, rttable->table.tablename,
                &stmt->u._insert.tableinfo);
        if (rttable->rec_len < 0)
        {
            return RET_ERROR;
        }
        rttable->rec_buf = PMEM_ALLOC(sizeof(char) * rttable->rec_len);
    }

    col_list = &stmt->u._insert.columns;

    ret = open_cursor_for_insertion(handle, &rttable[0], LK_EXCLUSIVE);
    if (ret < 0)
    {
        ret = RET_ERROR;
        goto RETURN_LABEL;
    }

    select = &stmt->u._insert.u.query;
    select->main_parsing_chunk = stmt->parsing_memory;

    ret = make_work_table(&handle, select);
    if (ret == RET_ERROR)
    {
        goto RETURN_LABEL;
    }

    if (select->tmp_sub)
    {
        select = select->tmp_sub;
    }

    qresult = &select->queryresult;

    fieldnum = stmt->u._insert.tableinfo.numFields;
    fieldinfo = stmt->u._insert.fieldinfo;

    while (1)
    {
        ret_fetch = SQL_fetch(&handle, select, NULL);
        if (ret_fetch == RET_ERROR)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_ERROR, 0);
            ret = RET_ERROR;
            goto RETURN_LABEL;
        }
        else if (ret_fetch == RET_END)
        {
            goto RETURN_LABEL;
        }

        sc_memset(rttable->rec_buf, 0, rttable->rec_len);

        if (col_list->len == 0)
        {
            for (i = 0; i < fieldnum; i++)
            {
                target = rttable->rec_buf + fieldinfo[i].offset;
                val = &qresult->list[i].value;

                if (val->is_null)
                {
                    if ((fieldinfo[i].flag & NULL_BIT) == 0
                            && !(fieldinfo[i].flag & AUTO_BIT))
                    {
                        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                                SQL_E_NOTALLOWEDNULLVALUE, 0);
                        ret = RET_ERROR;
                        goto RETURN_LABEL;
                    }
                    DB_VALUE_SETNULL(rttable->rec_buf, fieldinfo[i].position,
                            rttable->nullflag_offset);
                }
                else
                {
                    ret = copy_value_into_record(rttable->Hcursor, target,
                            val, &fieldinfo[i], 0);
                    if (ret != RET_SUCCESS)
                    {
                        ret = RET_ERROR;
                        goto RETURN_LABEL;
                    }
                    DB_VALUE_SETNOTNULL(rttable->rec_buf,
                            fieldinfo[i].position, rttable->nullflag_offset);
                }
            }
        }
        else
        {
            for (i = 0; i < fieldnum; i++)
            {
                target = rttable->rec_buf + fieldinfo[i].offset;
                for (j = 0; j < col_list->len; j++)
                {
                    if (!mdb_strcmp(fieldinfo[i].fieldName,
                                    col_list->list[j].name))
                    {
                        break;
                    }
                }
                if (j < col_list->len)
                {       // found it
                    val = &qresult->list[j].value;
                    if (val->is_null)
                    {
                        if ((col_list->list[j].fieldinfo->flag & NULL_BIT) == 0
                                && !(fieldinfo[i].flag & AUTO_BIT))
                        {
                            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                                    SQL_E_NOTALLOWEDNULLVALUE, 0);
                            ret = RET_ERROR;
                            goto RETURN_LABEL;
                        }
                        DB_VALUE_SETNULL(rttable->rec_buf,
                                col_list->list[j].fieldinfo->position,
                                rttable->nullflag_offset);
                    }
                    else
                    {
                        ret = copy_value_into_record(rttable->Hcursor, target,
                                val, &fieldinfo[i], 0);
                        if (ret != RET_SUCCESS)
                        {
                            ret = RET_ERROR;
                            goto RETURN_LABEL;
                        }
                        DB_VALUE_SETNOTNULL(rttable->rec_buf,
                                col_list->list[j].fieldinfo->position,
                                rttable->nullflag_offset);
                    }
                }
                else
                {       // not found it
                    if (!DB_DEFAULT_ISNULL(fieldinfo[i].defaultValue))
                    {   // default value: NOT NULL
                        sc_memset(&tmpval, 0x00, sizeof(T_VALUEDESC));
                        ret = get_defaultvalue(&tmpval.attrdesc,
                                &fieldinfo[i]);
                        if (ret < 0)
                        {
                            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
                            ret = RET_ERROR;
                            goto RETURN_LABEL;
                        }
                        tmpval.u = tmpval.attrdesc.defvalue.u;

                        tmpval.valuetype = fieldinfo[i].type;

                        if (IS_BS_OR_NBS(tmpval.valuetype))
                        {
                            tmpval.value_len =
                                    strlen_func[tmpval.valuetype] (tmpval.u.
                                    ptr);
                        }
#ifdef MDB_DEBUG
                        else if (IS_BYTE(tmpval.valuetype))
                        {
                            sc_assert(0, __FILE__, __LINE__);
                        }
#endif

                        ret = copy_value_into_record(rttable->Hcursor, target,
                                &tmpval, &fieldinfo[i], 0);
                        if (ret != RET_SUCCESS)
                        {
                            ret = RET_ERROR;
                            goto RETURN_LABEL;
                        }

                        if (IS_BS(tmpval.valuetype))
                        {
                            PMEM_FREE(tmpval.u.ptr);
                        }
                        else if (IS_NBS(tmpval.valuetype))
                        {
                            PMEM_FREE(tmpval.u.Wptr);
                        }

                        DB_VALUE_SETNOTNULL(rttable->rec_buf,
                                fieldinfo[i].position,
                                rttable->nullflag_offset);
                    }
                    else
                    {   // default value: NULL
                        if ((fieldinfo[i].flag & NULL_BIT) == 0
                                && !(fieldinfo[i].flag & AUTO_BIT))
                        {
                            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                                    SQL_E_NOTALLOWEDNULLVALUE, 0);
                            ret = RET_ERROR;
                            goto RETURN_LABEL;
                        }
                        DB_VALUE_SETNULL(rttable->rec_buf,
                                fieldinfo[i].position,
                                rttable->nullflag_offset);
                    }
                }
            }
        }

        ret = dbi_Cursor_Insert(handle, rttable->Hcursor, rttable->rec_buf,
                rttable->rec_len, 0, &inserted_rid);
        if (ret < 0)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
            ret = RET_ERROR;
            goto RETURN_LABEL;
        }
        else
        {
            stmt->u._insert.rows++;
        }

        if (ret_fetch == RET_END_USED_DIRECT_API)
        {
            break;
        }
    }

  RETURN_LABEL:
    close_cursor(handle, rttable);
    close_all_open_tables(handle, &stmt->u._insert.u.query);

    return ret;
}

/*****************************************************************************/
//! exec_insert 

/*! \breif  INSERT 문장을 수행
 ************************************
 * \param handle(in)    :
 * \param stmt(in/out)  : 
 ************************************
 * \return  return_value
 ************************************
 * \note 내부 알고리즘\n
 *      및 기타 설명
 *****************************************************************************/
int
exec_insert(int handle, T_STATEMENT * stmt)
{
    if (stmt->u._insert.type == SIT_VALUES)
        return exec_insert_values(handle, stmt);
    else    /* NONE or SIT_QUERY */
        return exec_insert_query(handle, stmt);
}

// rttable은 상위 query의 from 절에 있는 rttable(즉 temp table에 대한)을
// 사용하면 된다
// exec_insert_query()를 베꼈는데 ...
/*****************************************************************************/
//! exec_insert_subquery 

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
 * \note 내부 알고리즘\n
 *  - SQL_cleanup_memory를 호출할때, 기존에 할당된 buffer의 공간인지
 *      모르고 free하는 경우가 있어서, 
 *      함수의 인자로 main_stmt의 chunk 공간을 받음 
 *  - VIEW와 TABLE을 조인해서 ORDERBY 시킨 또다른 VIEW를 생성할때
 *      버그가 존재햇음
 *      
 *****************************************************************************/
int
exec_insert_subquery(int handle, T_SELECT * select, T_RTTABLE * rttable,
        T_PARSING_MEMORY * main_parsing_chunk)
{
    T_SELECT *fetch_select;
    T_QUERYRESULT *qresult;
    SYSFIELD_T *fieldinfo = NULL;
    T_VALUEDESC *val;

    char *target;
    int fieldnum;
    int i, ret = RET_SUCCESS, ret_fetch;
    OID inserted_rid;

    if (rttable->rec_buf == NULL)
    {
        extern int init_rec_buf(T_RTTABLE * rttable, SYSTABLE_T * tableinfo);

        ret = init_rec_buf(rttable, NULL);
        if (ret < 0)
        {
            return RET_ERROR;
        }
    }

    rttable->status = SCS_CLOSE;

    ret = open_cursor(handle, &rttable[0], NULL, NULL, LK_EXCLUSIVE);
    if (ret < 0)
    {
        return RET_ERROR;
    }

    select->main_parsing_chunk = main_parsing_chunk;

    ret = make_work_table(&handle, select);
    if (ret == RET_ERROR)
    {
        return -1;
    }

    if (select->tmp_sub)
    {
        fetch_select = select->tmp_sub;
    }
    else
    {
        fetch_select = select;
    }

    qresult = &fetch_select->queryresult;

    fieldnum = dbi_Schema_GetFieldsInfo(handle, rttable->table.tablename,
            &fieldinfo);
    if (fieldnum < 0)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_ERROR, 0);
        ret = RET_ERROR;
        goto RETURN_LABEL;
    }

    while (1)
    {
        ret_fetch = SQL_fetch(&handle, fetch_select, NULL);

        if (ret_fetch == RET_ERROR)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_ERROR, 0);
            ret = RET_ERROR;
            goto RETURN_LABEL;
        }
        else if (ret_fetch == RET_END)
        {
            goto RETURN_LABEL;
        }

        sc_memset(rttable->rec_buf, 0, rttable->rec_len);
        for (i = 0; i < fieldnum; i++)
        {
            target = rttable->rec_buf + fieldinfo[i].offset;
            val = &qresult->list[i].value;

            if (!val->is_null)
            {
                ret = copy_value_into_record(-1, target, val, &fieldinfo[i],
                        0);
                if (ret < 0)
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_ERROR, 0);
                    ret = RET_ERROR;
                    goto RETURN_LABEL;

                }
            }
            else
            {
                DB_VALUE_SETNULL(rttable->rec_buf, i,
                        rttable->nullflag_offset);
            }
        }

        ret = dbi_Cursor_Insert(handle, rttable->Hcursor, rttable->rec_buf,
                rttable->rec_len, 0, &inserted_rid);
        if (ret < 0)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
            ret = RET_ERROR;
            goto RETURN_LABEL;
        }

        if (ret_fetch == RET_END_USED_DIRECT_API)
        {
            break;
        }
    }

  RETURN_LABEL:
    PMEM_FREENUL(fieldinfo);
    close_cursor(handle, rttable);
    select->tstatus = TSS_EXECUTED;

    return ret;
}

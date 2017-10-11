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
#include "sql_main.h"
#include "sql_decimal.h"
#include "sql_parser.h"
#include "mdb_er.h"
#include "ErrorCode.h"

extern int SQL_cleanup(int *, T_STATEMENT *, int, int);
extern int exec_insert_query(int handle, T_STATEMENT * stmt);
extern int sql_planning(int handle, T_STATEMENT * stmt);

/*****************************************************************************/
//! function_name 

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
 *      및 기타 설명
 *****************************************************************************/
static int
exec_droptable(int handle, char *table)
{
    int ret;

    ret = dbi_Relation_Drop(handle, table);
    if (ret < 0)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
        return SQL_E_ERROR;
    }
    else
        return SQL_E_NOERROR;
}

/*****************************************************************************/
//! exec_drop_view 

/*! \breif DROP VIEW 문을 수행한다.
 ************************************
 * \param handle     :
 * \param table      : 
 ************************************
 * \return SQL_E_NOERROR or SQL_E_ERROR
 ************************************
 * \note
 *****************************************************************************/
static int
exec_drop_view(int handle, char *table)
{
    int ret;

    ret = dbi_ViewDefinition_Drop(handle, table);
    if (ret < 0)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
        return SQL_E_ERROR;
    }
    ret = dbi_Relation_Drop(handle, table);
    if (ret < 0)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
        return SQL_E_ERROR;
    }
    return SQL_E_NOERROR;
}

static int
exec_drop_sequence(int handle, char *sequenceName)
{
    int ret = -1;

    ret = dbi_Sequence_Drop(handle, sequenceName);
    if (ret < 0)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
        return SQL_E_ERROR;
    }

    return SQL_E_NOERROR;

}

static int
exec_drop_rid(int handle, char *rid_to_drop)
{
    int ret = -1;
    OID rid;

    sc_memcpy(&rid, rid_to_drop, OID_SIZE);

    ret = dbi_Rid_Drop(handle, rid);
    if (ret < 0)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
        return SQL_E_ERROR;
    }

    return SQL_E_NOERROR;

}

/*****************************************************************************/
//! function_name 

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
 *      및 기타 설명
 *****************************************************************************/
static int
exec_dropindex(int handle, char *index)
{
    int ret;

    ret = dbi_Index_Drop(handle, index);
    if (ret < 0)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
        return SQL_E_ERROR;
    }
    else
        return SQL_E_NOERROR;
}

/*****************************************************************************/
//! function_name 

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
 *      및 기타 설명
 *****************************************************************************/
int
drop_temp_index(int handle, char *index)
{
    return (exec_dropindex(handle, index));
}

/*****************************************************************************/
//! function_name 

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
 *      및 기타 설명
 *****************************************************************************/
int
exec_drop(int handle, T_STATEMENT * stmt)
{
    if (stmt->u._drop.type == SDT_TABLE)
    {
        if (exec_droptable(handle, stmt->u._drop.target) < 0)
        {
            return RET_ERROR;
        }
    }
    else if (stmt->u._drop.type == SDT_VIEW)
    {
        if (exec_drop_view(handle, stmt->u._drop.target) < 0)
        {
            return RET_ERROR;
        }
    }
    else if (stmt->u._drop.type == SDT_SEQUENCE)
    {
        if (exec_drop_sequence(handle, stmt->u._drop.target) < 0)
        {
            return RET_ERROR;
        }
    }
    else if (stmt->u._drop.type == SDT_RID)
    {
        if (exec_drop_rid(handle, stmt->u._drop.target) < 0)
        {
            return RET_ERROR;
        }
    }
    else
    {
        if (exec_dropindex(handle, stmt->u._drop.target) < 0)
        {
            return RET_ERROR;
        }
    }

    return RET_SUCCESS;
}

/*****************************************************************************/
//! exec_rename_table 

/*! \breif 테이블명을 변경한다.
 *
 ************************************
 * \param handle     :
 * \param old_name     : 
 * \param new_name     :
 * \param param_name :
 ************************************
 * \return RET_SUCCESS or RET_ERROR
 ************************************
 * \note 테이블명을 변경하고 그 테이블에 Primary Key가 있는 경우,\n
 *        그 칼럼에 생성된 색인명을 변경한다.
 *****************************************************************************/
int
exec_rename_table(int handle, char *old_name, char *new_name)
{
    SYSINDEX_T *indexes = NULL;
    char pIndex[INDEX_NAME_LENG];
    int n_indexes;
    int i, ret;

    /* rename index */
    n_indexes = dbi_Schema_GetTableIndexesInfo(handle, old_name,
            SCHEMA_REGU_INDEX, &indexes);
    if (n_indexes < 0)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, n_indexes, 0);
        return RET_ERROR;
    }
    for (i = 0; i < n_indexes; i++)
    {
        if (!mdb_strncmp(indexes[i].indexName, PRIMARY_KEY_PREFIX, 3))
        {
            sc_snprintf(pIndex, INDEX_NAME_LENG, "%s%s",
                    PRIMARY_KEY_PREFIX, new_name);
            ret = dbi_Index_Rename(handle, indexes[i].indexName, pIndex);
            if (ret != DB_SUCCESS)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
                PMEM_FREENUL(indexes);
                return RET_ERROR;
            }
        }
    }
    /* rename table */
    ret = dbi_Relation_Rename(handle, old_name, new_name);
    if (ret != DB_SUCCESS)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
        PMEM_FREENUL(indexes);
        return RET_ERROR;
    }
    PMEM_FREENUL(indexes);
    return RET_SUCCESS;
}

/*****************************************************************************/
//! exec_rename 

/*! \breif  테이블명/색인명을 변경한다.
 *
 ************************************
 * \param handle     :
 * \param stmt          : 
 ************************************
 * \return RET_SUCCESS or RET_ERROR
 ************************************
 * \note
 *****************************************************************************/
int
exec_rename(int handle, T_STATEMENT * stmt)
{
    char *old_name, *new_name;
    int ret;

    old_name = stmt->u._rename.oldname;
    new_name = stmt->u._rename.newname;

    if (stmt->u._rename.type == SRT_TABLE)
    {
        return exec_rename_table(handle, old_name, new_name);
    }
    else if (stmt->u._rename.type == SRT_INDEX)
    {
        ret = dbi_Index_Rename(handle, old_name, new_name);
        if (ret == DB_SUCCESS)
            return RET_SUCCESS;
        else
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
            return RET_ERROR;
        }
    }
    else
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_INVALIDCOMMAND, 0);
        return RET_ERROR;
    }
}

/*****************************************************************************/
//! make_add_fielddesc 

/*! \breif 추가된 필드를 포함한 새로운 테이블의 필드 정보를 설정함
 *
 ************************************
 * \param handle(in)    :
 * \param alter(in/out) : 
 ************************************
 * \return RET_SUCCESS or RET_ERROR
 ************************************
 * \note 
 *  - COLLATION 지원에 따른 dbi_FieldDesc 인터페이스 변경
 *****************************************************************************/
int
make_add_fielddesc(int handle, T_ALTER * alter)
{
    SYSFIELD_T *old_fields;
    FIELDDESC_T *fielddesc;
    T_ATTRDESC *column_attr;
    char *column_defval = NULL;
    char str_dec[MAX_DEFAULT_VALUE_LEN] = "\0";
    int old_fieldnum;
    int fieldnum;
    int i, n;

    old_fields = alter->old_fields;
    old_fieldnum = alter->old_fieldnum;

    fieldnum = old_fieldnum + alter->column.attr_list.len;
    fielddesc = PMEM_ALLOC(sizeof(FIELDDESC_T) * fieldnum);
    if (fielddesc == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
        return RET_ERROR;
    }
    /* make old fielddesc */
    for (i = 0, n = 0; i < old_fieldnum; i++, n++)
    {
        dbi_FieldDesc(old_fields[i].fieldName, old_fields[i].type,
                old_fields[i].length, old_fields[i].scale, old_fields[i].flag,
                NULL, old_fields[i].type == DT_NVARCHAR
                ? old_fields[i].fixed_part / WCHAR_SIZE
                : old_fields[i].fixed_part,
                &(fielddesc[n]), old_fields[i].collation);
        sc_memcpy(fielddesc[n].defaultValue, old_fields[n].defaultValue, 40);

        if (alter->scope == SAT_CONSTRAINT &&
                alter->constraint.type == SAT_PRIMARY_KEY)
        {
            if (old_fields[i].fieldId == -1)
            {
                fielddesc[n].flag |= PRI_KEY_BIT;
                fielddesc[n].flag &= ~NULL_BIT;
            }
        }
    }
    /* make fielddesc to add */
    for (i = 0; i < alter->column.attr_list.len; i++, n++)
    {
        column_attr = &alter->column.attr_list.list[i];
        if (column_attr->defvalue.defined && column_attr->defvalue.not_null)
            PUT_DEFAULT_VALUE(column_defval, column_attr, str_dec);
        else
            column_defval = NULL;
        dbi_FieldDesc(column_attr->attrname, column_attr->type,
                column_attr->len, column_attr->dec, column_attr->flag,
                column_defval, column_attr->fixedlen, &(fielddesc[n]),
                column_attr->collation);
    }
    alter->fieldnum = fieldnum;
    alter->fielddesc = fielddesc;

    return RET_SUCCESS;
}

/*****************************************************************************/
//! make_drop_fielddesc 

/*! \breif 필드가 제거된 새로운 테이블의 필드 정보를 만듬(ALTER TABLE ... DROP)
 ************************************
 * \param handle(in)    :
 * \param alter(in/out) : 
 ************************************
 * \return RET_SUCCESS or RET_ERROR
 ************************************
 * \note 
 *  - COLLATION 지원에 따른 dbi_FieldDesc 인터페이스 변경
 *****************************************************************************/
int
make_drop_fielddesc(int handle, T_ALTER * alter)
{
    SYSFIELD_T *old_fields;
    FIELDDESC_T *fielddesc;
    int old_fieldnum;
    int fieldnum;
    int PK_fieldid = -1;
    int i, n;

    old_fields = alter->old_fields;
    old_fieldnum = alter->old_fieldnum;

    fieldnum = old_fieldnum - alter->column.name_list.len;
    fielddesc = PMEM_ALLOC(sizeof(FIELDDESC_T) * fieldnum);
    if (fielddesc == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
        return RET_ERROR;
    }
    for (i = 0, n = 0; i < old_fieldnum; i++)
    {
        if (old_fields[i].fieldId == -1)
        {
            if ((old_fields[i].flag & PRI_KEY_BIT) != 0)
                PK_fieldid = i;
            continue;
        }
        dbi_FieldDesc(old_fields[i].fieldName, old_fields[i].type,
                old_fields[i].length, old_fields[i].scale, old_fields[i].flag,
                NULL, old_fields[i].type == DT_NVARCHAR
                ? old_fields[i].fixed_part / WCHAR_SIZE
                : old_fields[i].fixed_part, &(fielddesc[n]),
                old_fields[i].collation);
        sc_memcpy(fielddesc[n++].defaultValue, old_fields[i].defaultValue, 40);
    }
    if (PK_fieldid >= 0 ||
            (alter->scope == SAT_CONSTRAINT &&
                    alter->constraint.type == SAT_PRIMARY_KEY))
    {
        SYSTABLE_T tableinfo;
        int ret;

        ret = dbi_Schema_GetTableInfo(handle, alter->tablename, &tableinfo);
        if (ret < 0)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
            return RET_ERROR;
        }

        if (tableinfo.flag & _CONT_MSYNC)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                    DB_E_NOTFOUND_PRIMARYKEY, 0);
            return RET_ERROR;
        }

        for (i = 0; i < n; i++)
        {
            if ((fielddesc[i].flag & PRI_KEY_BIT) != 0)
            {
                fielddesc[i].flag &= ~PRI_KEY_BIT;
                fielddesc[i].flag |= NULL_BIT;
            }
        }
    }
    alter->fieldnum = fieldnum;
    alter->fielddesc = fielddesc;

    return RET_SUCCESS;
}

/*****************************************************************************/
//! make_alter_fielddesc 

/*! \breif ALTER TABLE ... ALTER 에 대해 새로운 테이블 정의문을 생성한다.
 *
 ************************************
 * \param handle(in)    :
 * \param alter(in/out) : 
 ************************************
 * \return RET_SUCCESS or RET_ERROR
 ************************************
 * \note 
 *  - COLLATION 지원에 따른 dbi_FieldDesc 인터페이스 변경
 *****************************************************************************/
int
make_alter_fielddesc(int handle, T_ALTER * alter)
{
    SYSFIELD_T *old_fields;
    FIELDDESC_T *fielddesc;
    T_ATTRDESC *column_attr = NULL;
    char *column_defval = NULL;
    char str_dec[MAX_DEFAULT_VALUE_LEN] = "\0";
    int old_fieldnum;
    int fieldnum;
    int PK_fieldid = -1;
    int i, j;
    int exist_old_pk = 0;

    old_fields = alter->old_fields;
    old_fieldnum = alter->old_fieldnum;

    fieldnum = old_fieldnum;
    fielddesc = PMEM_ALLOC(sizeof(FIELDDESC_T) * fieldnum);
    if (fielddesc == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
        return RET_ERROR;
    }
    for (i = 0; i < old_fieldnum; i++)
    {
        if (alter->scope == SAT_COLUMN && old_fields[i].fieldId == -1)
        {
            /* below routine for "alter column  ..." statement */
            if (alter->column.name_list.list)
            {   /* rename column */
                column_attr = &alter->column.attr_list.list[0];
                dbi_FieldDesc(column_attr->attrname, old_fields[i].type,
                        old_fields[i].length, old_fields[i].scale,
                        old_fields[i].flag, NULL,
                        old_fields[i].type == DT_NVARCHAR
                        ? old_fields[i].fixed_part / WCHAR_SIZE
                        : old_fields[i].fixed_part,
                        &(fielddesc[i]), old_fields[i].collation);
            }
            else
            {   /* alter column attribute */
                for (j = 0; j < alter->column.attr_list.len; j++)
                {
                    if (!mdb_strcmp(old_fields[i].fieldName,
                                    alter->column.attr_list.list[j].attrname))
                    {
                        column_attr = &alter->column.attr_list.list[j];
                        break;
                    }
                }
                if (column_attr == NULL)
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                            SQL_E_INVALIDCOLUMN, 0);
                    return RET_ERROR;
                }
                /* compensate column attribute */
                if (column_attr->len <= 0)
                {       /* that means data type is not defined */
                    column_attr->type = old_fields[i].type;
                    column_attr->len = old_fields[i].length;
                    column_attr->dec = old_fields[i].scale;
                    column_attr->fixedlen = old_fields[i].fixed_part;
                    if (column_attr->type == DT_NVARCHAR)
                        column_attr->fixedlen /= WCHAR_SIZE;

                    column_attr->collation = old_fields[i].collation;
                }
                if (column_attr->defvalue.defined == 0 ||
                        column_attr->defvalue.not_null == 0)
                    column_defval = NULL;
                else
                    PUT_DEFAULT_VALUE(column_defval, column_attr, str_dec);

                if ((column_attr->flag & PRI_KEY_BIT) != 0)
                {
                    PK_fieldid = i;
                }

                if (PK_fieldid == -1 && (old_fields[i].flag & PRI_KEY_BIT))
                {
                    exist_old_pk = 1;
                }

                dbi_FieldDesc(column_attr->attrname, column_attr->type,
                        column_attr->len, column_attr->dec, column_attr->flag,
                        column_defval, column_attr->fixedlen,
                        &(fielddesc[i]), column_attr->collation);
            }
        }
        else
        {
            dbi_FieldDesc(old_fields[i].fieldName, old_fields[i].type,
                    old_fields[i].length, old_fields[i].scale,
                    old_fields[i].flag, NULL,
                    old_fields[i].type == DT_NVARCHAR
                    ? old_fields[i].fixed_part / WCHAR_SIZE
                    : old_fields[i].fixed_part,
                    &(fielddesc[i]), old_fields[i].collation);
            sc_memcpy(fielddesc[i].defaultValue,
                    old_fields[i].defaultValue, 40);
        }
    }

    if (PK_fieldid == -1 && exist_old_pk)
    {
        SYSTABLE_T tableinfo;
        int ret;

        ret = dbi_Schema_GetTableInfo(handle, alter->tablename, &tableinfo);
        if (ret < 0)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
            return RET_ERROR;
        }

        if (tableinfo.flag & _CONT_MSYNC)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                    DB_E_NOTFOUND_PRIMARYKEY, 0);
            return RET_ERROR;
        }

        for (i = 0; i < fieldnum; i++)
        {
            fielddesc[i].flag &= ~PRI_KEY_BIT;
            fielddesc[i].flag |= NULL_BIT;
        }
    }

    if (PK_fieldid >= 0)
    {
        for (i = 0; i < fieldnum; i++)
        {
            if (i == PK_fieldid)
            {
                fielddesc[i].flag |= PRI_KEY_BIT;
                fielddesc[i].flag &= ~NULL_BIT;
            }
            else if ((fielddesc[i].flag & PRI_KEY_BIT) != 0)
            {
                fielddesc[i].flag &= ~PRI_KEY_BIT;
                fielddesc[i].flag |= NULL_BIT;
            }
        }
    }
    else if (alter->scope == SAT_CONSTRAINT &&
            alter->constraint.type == SAT_PRIMARY_KEY)
    {
        for (i = 0; i < fieldnum; i++)
        {
            if (old_fields[i].fieldId == -1)
            {
                fielddesc[i].flag |= PRI_KEY_BIT;
                fielddesc[i].flag &= ~NULL_BIT;
            }
            else if ((fielddesc[i].flag & PRI_KEY_BIT) != 0)
            {
                fielddesc[i].flag &= ~PRI_KEY_BIT;
                fielddesc[i].flag |= NULL_BIT;
            }
        }
    }
    alter->fieldnum = fieldnum;
    alter->fielddesc = fielddesc;

    return RET_SUCCESS;
}

/*****************************************************************************/
//! make_tmp_structure 

/*! \breif 테이블을 새로운 테이블의 정의에 맞게 재구성한다.
 ************************************
 * \param handle(int)    :
 * \param stmt(in/out)      : 
 * \param tablename()      :
 * \param alter()     :
 ************************************
 * \return RET_SUCCESS or RET_ERROR
 ************************************
 * \note 
 *  - T_EXPRESSIONDESC 구조체 변경
 *
 *****************************************************************************/
int
make_tmp_structure(int handle, T_STATEMENT * stmt, char *tablename,
        T_ALTER * alter)
{
    SYSFIELD_T *old_fields;
    FIELDDESC_T *fielddesc;
    T_INSERT *insert = NULL;
    T_SELECT *select = NULL;
    T_QUERYDESC *qdesc;
    T_LIST_SELECTION *selection;
    T_EXPRESSIONDESC *expr = NULL;
    T_LIST_JOINTABLEDESC *fromlist = NULL;
    T_TABLEDESC *table;
    int old_fieldnum;
    int n_insert;
    int i, n;

    old_fields = alter->old_fields;
    old_fieldnum = alter->old_fieldnum;
    fielddesc = alter->fielddesc;
    n_insert =
            (alter->type == SAT_DROP) ? alter->fieldnum : alter->old_fieldnum;

    sc_memset(stmt, 0, sizeof(T_STATEMENT));
    stmt->sqltype = ST_INSERT;

    /* make insertion structure */
    insert = &stmt->u._insert;
    insert->type = SIT_QUERY;
    insert->table.tablename = PMEM_STRDUP(tablename);
    if (!insert->table.tablename)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
        goto ERROR_LABEL;
    }
    insert->table.aliasname = NULL;

    insert->columns.list =
            (T_COLDESC *) PMEM_ALLOC(sizeof(T_COLDESC) * n_insert);

    if (insert->columns.list == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
        goto ERROR_LABEL;
    }
    insert->columns.max = insert->columns.len = n_insert;
    for (i = 0; i < n_insert; i++)
    {
        insert->columns.list[i].name = PMEM_STRDUP(fielddesc[i].fieldName);
        if (!insert->columns.list[i].name)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
            goto ERROR_LABEL;
        }
        insert->columns.list[i].fieldinfo = NULL;
    }

    /* make selection structure */
    select = &insert->u.query;
    select->tstatus = TSS_PARSED;

    qdesc = &select->planquerydesc.querydesc;
    qdesc->is_distinct = 0;
    qdesc->querytype = SQT_NORMAL;

    selection = &qdesc->selection;

    selection->list =
            (T_SELECTION *) PMEM_XCALLOC(sizeof(T_SELECTION) * n_insert);
    if (selection->list == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
        goto ERROR_LABEL;
    }
    selection->max = selection->len = n_insert;
    for (i = 0, n = 0; i < old_fieldnum; i++)
    {
        if (alter->type == SAT_DROP && old_fields[i].fieldId == -1)
            continue;
        sc_memcpy(selection->list[n].res_name, old_fields[i].fieldName,
                FIELD_NAME_LENG - 1);
        selection->list[n].res_name[FIELD_NAME_LENG - 1] = '\0';
        selection->list[n].is_distinct = 0;
        expr = &selection->list[n++].expr;
        expr->list =
                sql_mem_get_free_postfixelem_list(stmt->parsing_memory, 1);
        if (expr->list == NULL)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
            goto ERROR_LABEL;
        }

        expr->list[0] = sql_mem_get_free_postfixelem(stmt->parsing_memory);
        if (expr->list[0] == NULL)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
            goto ERROR_LABEL;
        }

        expr->max = expr->len = 1;
        sc_memset(expr->list[0], 0, sizeof(T_POSTFIXELEM));
        expr->list[0]->elemtype = SPT_VALUE;
        expr->list[0]->u.value.valueclass = SVC_VARIABLE;
        expr->list[0]->u.value.attrdesc.attrname =
                PMEM_STRDUP(old_fields[i].fieldName);
        if (!expr->list[0]->u.value.attrdesc.attrname)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
            goto ERROR_LABEL;
        }
    }
    fromlist = &qdesc->fromlist;
    fromlist->list =
            (T_JOINTABLEDESC *) PMEM_ALLOC(sizeof(T_JOINTABLEDESC) * 1);
    if (fromlist->list == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
        goto ERROR_LABEL;
    }
    fromlist->max = fromlist->len = 1;
    table = &fromlist->list[0].table;
    table->tablename = PMEM_STRDUP(alter->tablename);
    if (!table->tablename)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
        goto ERROR_LABEL;
    }
    table->aliasname = NULL;
    table->correlated_tbl = NULL;

    fromlist->list[0].fieldinfo = NULL;

    qdesc->limit.offset = 0;
    qdesc->limit.rows = -1;

    if (sql_parsing(handle, stmt) == RET_ERROR)
        goto ERROR_LABEL;

    if (sql_validating(handle, stmt) == RET_ERROR)
        goto ERROR_LABEL;

    return RET_SUCCESS;

  ERROR_LABEL:

    SQL_cleanup(&handle, stmt, MODE_MANUAL, 1);
    sc_memset(stmt, 0, sizeof(T_STATEMENT));

    return RET_ERROR;
}

/*****************************************************************************/
//! rebuild_indexes 

/*! \breif 새로 정의한 테이블에 기존 테이블과 동일하게 색인을 생성한다.
 *
 ************************************
 * \param handle(in)    :
 * \param tablename(in) : 
 * \param alter(in/out) :
 ************************************
 * \return RET_SUCCESS or RET_ERROR
 ************************************
 * \note
 *  - 알고리즘
 *      1. 기존 테이블에 생성된 색인을 모두 찾는다.\n
 *        2. 각 색인의 키칼럼을 보고 생성할 필요없는 색인을 찾는다.\n
 *        3. 새로운 테이블에 키 제약이 설정된 경우, 해당하는 유일 색인을 생성한다.\n
 *        4. 새로운 테이블에 동일한 색인명과 키칼럼으로 구성된 색인을 생성한다.
 *  - COLLATION 지원에 따른 dbi_FieldDesc 인터페이스 변경
 *  - INDEX 재생성시 ordering 넣어준다
 *      
 *****************************************************************************/
int
rebuild_indexes(int handle, char *tablename, T_ALTER * alter)
{
    SYSINDEX_T *indexes = NULL;
    SYSINDEXFIELD_T *indexfields = NULL;
    FIELDDESC_T *indexfdesc = NULL;
    SYSFIELD_T *old_fields;
    FIELDDESC_T *fielddesc;
    char pIndex[INDEX_NAME_LENG];
    int n_indexes;
    int fieldnum;
    int skip_indexing;
    int i, j, n, ret = RET_SUCCESS;

    old_fields = alter->old_fields;
    fielddesc = alter->fielddesc;
    fieldnum = alter->fieldnum;

    n_indexes = dbi_Schema_GetTableIndexesInfo(handle,
            alter->tablename, SCHEMA_ALL_INDEX, &indexes);
    if (n_indexes < 0)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, n_indexes, 0);
        goto ERROR_LABEL;
    }
    /* rebuild indexes from the old table */
    for (i = 0; i < n_indexes; i++)
    {

        skip_indexing = 0;

        /* if temp index, drop it */
        if (isTempIndex_name(indexes[i].indexName))
        {
            ret = dbi_Index_Drop(handle, indexes[i].indexName);
            if (ret < 0)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
                goto ERROR_LABEL;
            }
            continue;
        }

        /* if a index for primary key, 
           delay to rebuild for new primary key definition */
        if (!mdb_strncmp(indexes[i].indexName, PRIMARY_KEY_PREFIX, 3))
        {
            if (alter->constraint.type == SAT_PRIMARY_KEY)
                continue;
        }
        ret = dbi_Schema_GetIndexInfo(handle, indexes[i].indexName,
                NULL, &indexfields, alter->tablename);
        if (ret < 0)
            goto ERROR_LABEL;
        indexfdesc = PMEM_ALLOC(sizeof(FIELDDESC_T) * indexes[i].numFields);
        if (indexfdesc == NULL)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
            goto ERROR_LABEL;
        }
        for (j = 0; j < indexes[i].numFields; j++)
        {
            sc_memset(&indexfdesc[j], 0, sizeof(FIELDDESC_T));
            if (alter->type == SAT_DROP)
            {
                if (old_fields[indexfields[j].fieldId].fieldId == -1)
                {
                    skip_indexing = 1;
                    PMEM_FREENUL(indexfdesc);
                    break;
                }
                else
                {
                    sc_memcpy(indexfdesc[j].fieldName,
                            old_fields[indexfields[j].fieldId].fieldName,
                            FIELD_NAME_LENG);
                    indexfdesc[j].collation = indexfields[j].collation;
                    if (indexfields[j].order == 'D')
                        indexfdesc[j].flag |= FD_DESC;
                }
            }
            else
            {   /* SAT_ADD or SAT_ALTER */
                if (fielddesc[indexfields[j].fieldId].flag & NOLOGGING_BIT)
                {
                    PMEM_FREENUL(indexfdesc);
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                            SQL_E_NOLOGGING_COLUMN_INDEX, 0);
                    goto ERROR_LABEL;
                }
                if (old_fields[indexfields[j].fieldId].flag & PRI_KEY_BIT
                        && !(fielddesc[indexfields[j].fieldId].
                                flag & PRI_KEY_BIT)
                        && !mdb_strncmp(indexes[i].indexName,
                                PRIMARY_KEY_PREFIX, 3))
                {
                    skip_indexing = 1;
                    PMEM_FREENUL(indexfdesc);
                    break;
                }

                sc_memcpy(indexfdesc[j].fieldName,
                        fielddesc[indexfields[j].fieldId].fieldName,
                        FIELD_NAME_LENG);
                indexfdesc[j].collation = indexfields[j].collation;

                if (indexfields[j].order == 'D')
                    indexfdesc[j].flag |= FD_DESC;
            }
        }
        /* drop old index and create new index */
        if (skip_indexing == 0)
        {
            ret = dbi_Trans_NextTempName(handle, 0, pIndex, NULL);
            if (ret < 0)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
                PMEM_FREENUL(indexfdesc);
                goto ERROR_LABEL;
            }
            pIndex[0] = '@';
            ret = dbi_Index_Rename(handle, indexes[i].indexName, pIndex);
            if (ret < 0)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
                PMEM_FREENUL(indexfdesc);
                goto ERROR_LABEL;
            }
            if (!mdb_strncmp(indexes[i].indexName, PRIMARY_KEY_PREFIX, 3))
            {
                sc_sprintf(pIndex, "$pk.%s", tablename);
                ret = dbi_Index_Create(handle, tablename,
                        pIndex, indexes[i].numFields, indexfdesc,
                        UNIQUE_INDEX_BIT);
                if (ret < 0)
                {
                    PMEM_FREENUL(indexfdesc);
                    goto ERROR_LABEL;
                }
            }
            else
            {
                ret = dbi_Index_Create(handle, tablename,
                        indexes[i].indexName, indexes[i].numFields, indexfdesc,
                        (MDB_UINT8) (indexes[i].type & UNIQUE_INDEX_BIT));
                if (ret < 0)
                {
                    PMEM_FREENUL(indexfdesc);
                    goto ERROR_LABEL;
                }
            }
        }
        PMEM_FREENUL(indexfdesc);
        PMEM_FREENUL(indexfields);

    }

    /* create new index for primary key */
    if (alter->constraint.list.list)
    {
        sc_sprintf(pIndex, "$pk.%s", tablename);
        indexfdesc =
                PMEM_ALLOC(sizeof(FIELDDESC_T) * alter->constraint.list.len);
        if (indexfdesc == NULL)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
            goto ERROR_LABEL;
        }
        for (n = 0; n < alter->constraint.list.len; n++)
        {
            for (i = 0; i < fieldnum; i++)
            {
                if (!mdb_strcmp(alter->constraint.list.list[n],
                                fielddesc[i].fieldName)
                        && (fielddesc[i].flag & PRI_KEY_BIT))
                {
                    dbi_FieldDesc(fielddesc[i].fieldName, 0, 0, 0, 0, 0, -1,
                            &(indexfdesc[n]), fielddesc[i].collation);
                    break;
                }
            }
            if (i == fieldnum)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                        SQL_E_INVALIDKEYCOLUMN, 0);
                goto ERROR_LABEL;
            }
        }
        ret = dbi_Index_Create(handle, tablename, pIndex, n, indexfdesc,
                UNIQUE_INDEX_BIT);
        if (ret < 0)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
            goto ERROR_LABEL;
        }
        PMEM_FREENUL(indexfdesc);
    }
    PMEM_FREENUL(indexes);
    PMEM_FREENUL(indexfields);

    return RET_SUCCESS;

  ERROR_LABEL:

    PMEM_FREENUL(indexes);
    PMEM_FREENUL(indexfields);

    return RET_ERROR;
}

/*****************************************************************************/
//! exec_alter 

/*! \breif ALTER TABLE 문을 수행한다.
 ************************************
 * \param handle(in)   :
 * \param stmt(in/out)      : 
 ************************************
 * \return RET_SUCCESS or RET_ERROR
 ************************************
 * \note 다음을 수행한다.\n
 *        1. 변경된 테이블 정의로 새로운 테이블을 생성한다.\n
 *        2. 이전 테이블에 생성된 색인을 새로운 테이블에도 생성한다.\n
 *        3. 이젠 테이블의 레코드를 새로운 테이블에 삽입한다.\n
 *        4. 이전 테이블을 삭제한다.\n
 *        5. 새로운 테이블의 이름을 이전 테이블 이름으로 변경한다.
 *  - T_STMT의 PARSING MOMORY를 TMEMP STATEMENT로 전달한다.
 *  - index rebuild interface 추가
 *****************************************************************************/
int
exec_alter(int handle, T_STATEMENT * stmt)
{
    T_ALTER *alter;
    T_STATEMENT *tmpstmt = NULL;
    char dest[REL_NAME_LENG];
    char tmp[REL_NAME_LENG];
    int ret;

    alter = &stmt->u._alter;

    if (alter->type == SAT_SEQUENCE)
    {

        DB_SEQUENCE dbSequence;

        ret = dbi_Sequence_Read(handle, alter->u.sequence.name, &dbSequence);
        if (ret < 0)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
            return RET_ERROR;
        }

        sc_strcpy(dbSequence.sequenceName, alter->u.sequence.name);
        if (alter->u.sequence.bStart)
            dbSequence.startNum = alter->u.sequence.start;
        if (alter->u.sequence.bIncrement)
            dbSequence.increaseNum = alter->u.sequence.increment;
        if (alter->u.sequence.bMinValue)
            dbSequence.minNum = alter->u.sequence.minValue;
        if (alter->u.sequence.bMaxValue)
            dbSequence.maxNum = alter->u.sequence.maxValue;
        if (alter->u.sequence.cycled)
            dbSequence.cycled = alter->u.sequence.cycled;

        ret = dbi_Sequence_Alter(handle, &dbSequence);
        if (ret < 0)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
            return RET_ERROR;
        }

        return RET_SUCCESS;

    }
    else if (alter->type == SAT_REBUILD)
    {
        /* check this user can do this alter */
        ret = dbi_Index_Rebuild(handle, alter->u.rebuild_idx.num_object,
                alter->u.rebuild_idx.objectname);
        if (ret < 0)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
            return RET_ERROR;
        }

        return RET_SUCCESS;
    }
    else if (alter->type == SAT_MSYNC)
    {
        if (alter->msync_alter_type < 2)
        {
            if (alter->msync_alter_type == 0)
            {
                ret = dbi_Relation_mSync_Flush(handle, alter->tablename, 0);
                if (ret < 0)
                {
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
                    return RET_ERROR;
                }
            }
            ret = dbi_Relation_mSync_Alter(handle, alter->tablename,
                    alter->msync_alter_type);
        }
        else
        {
            ret = dbi_Relation_mSync_Flush(handle, alter->tablename, 1);
        }
        if (ret < 0)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
            return RET_ERROR;
        }

        return RET_SUCCESS;
    }

    if (alter->type == SAT_LOGGING)
    {
        int logging_on_off;

        if (alter->logging.on == 2)
            logging_on_off = 0;
        else
            logging_on_off = 1;

        ret = dbi_Relation_Logging(handle, alter->tablename, logging_on_off,
                alter->logging.runtime, _USER_USER);
        if (ret < 0)
        {
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
            return RET_ERROR;
        }

        return RET_SUCCESS;
    }

    tmpstmt = (T_STATEMENT *) PMEM_ALLOC(sizeof(T_STATEMENT));
    if (tmpstmt == NULL)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_OUTOFMEMORY, 0);
        return RET_ERROR;
    }

    tmpstmt->parsing_memory = stmt->parsing_memory;

    /* create new table */
    ret = dbi_Trans_NextTempName(handle, 1, dest, NULL);
    if (ret < 0)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_ERROR, 0);
        goto ERROR_LABEL;
    }
    dest[0] = '@';      /* change name as user table */

    switch (alter->type)
    {
    case SAT_ADD:
        if (make_add_fielddesc(handle, alter) == RET_ERROR)
            goto ERROR_LABEL;
        break;
    case SAT_DROP:
        if (make_drop_fielddesc(handle, alter) == RET_ERROR)
            goto ERROR_LABEL;
        break;
    case SAT_ALTER:
        if (make_alter_fielddesc(handle, alter) == RET_ERROR)
            goto ERROR_LABEL;
        break;
    default:
        PMEM_FREE(tmpstmt);
        return RET_ERROR;
    }
    ret = dbi_Relation_Create(handle, dest, alter->fieldnum, alter->fielddesc,
            _CONT_TABLE, _USER_USER, 0, NULL);
    if (ret < 0)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
        goto ERROR_LABEL;
    }

    /* rebuild index on the new table */
    ret = rebuild_indexes(handle, dest, alter);
    if (ret < 0)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
        goto ERROR_LABEL_DROP;
    }

    dbi_Log_Buffer_Flush(handle);

    /* if the old table is not empty, rebuild data */
    if (alter->having_data)
    {
        ret = make_tmp_structure(handle, tmpstmt, dest, alter);
        if (ret >= 0)
            ret = exec_insert_query(handle, tmpstmt);
        SQL_close_all_cursors(handle, tmpstmt);
        if (ret < 0)
        {
            switch (er_errid())
            {
            case SQL_E_NOTALLOWEDNULLVALUE:
                er_clear();
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_SETNOTNULL, 0);
                break;
            case DB_E_DUPUNIQUEKEY:
                er_clear();
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE,
                        SQL_E_SETPRIMARYKEY, 0);
                break;
            default:
                break;
            }
            goto ERROR_LABEL_DROP;
        }
    }

    ret = dbi_Trans_NextTempName(handle, 1, tmp, NULL);
    if (ret < 0)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, SQL_E_ERROR, 0);
        goto ERROR_LABEL_DROP;
    }
    tmp[0] = '@';       /* change name as user table */


    /* rename old table */
    ret = exec_rename_table(handle, alter->tablename, tmp);
    if (ret == RET_ERROR)
    {
        goto ERROR_LABEL_DROP;
    }

    if (exec_rename_table(handle, dest, alter->tablename) == RET_ERROR)
        goto ERROR_LABEL_RENAME;

    /* drop old table */
    ret = dbi_Relation_Drop(handle, tmp);
    if (ret < 0)
    {
        er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
        goto ERROR_LABEL_RENAME;
    }

    /* FIX_ALTER_RECOVERY */
    dbi_Log_Buffer_Flush(handle);

    SQL_cleanup(&handle, tmpstmt, MODE_MANUAL, 1);

    PMEM_FREE(tmpstmt);

    return RET_SUCCESS;

  ERROR_LABEL_RENAME:
    (void) dbi_Relation_Rename(handle, tmp, alter->tablename);

  ERROR_LABEL_DROP:
    (void) dbi_Relation_Drop(handle, dest);

  ERROR_LABEL:

    SQL_cleanup(&handle, stmt, MODE_MANUAL, 1);
    SQL_cleanup(&handle, tmpstmt, MODE_MANUAL, 1);

    PMEM_FREE(tmpstmt);

    return RET_ERROR;
}

/*****************************************************************************/
//! exec_truncate 

/*! \breif TRUNCATE TABLE 문을 실행한다.
 ************************************
 * \param handle     :
 * \param stmt       : 
 ************************************
 * \return RET_SUCCESS or RET_ERROR
 ************************************
 * \note
 *  - truncate 구문 확장
 *****************************************************************************/
int
exec_truncate(int handle, T_STATEMENT * stmt)
{
    int ret;

    ret = dbi_Relation_Truncate(handle, stmt->u._truncate.objectname);
    if (ret < 0)
    {
        return RET_ERROR;
    }

    return RET_SUCCESS;
}

/*****************************************************************************/
//! exec_admin

/*! \breif administrator SQL 문장 수행
 ************************************
 * \param handle(in)    :
 * \param stmt(in)      :
 ************************************
 * \return RET_SUCCESS or RET_ERROR
 ************************************
 * \note
 *  - ADMIN 문장 추가(EXPORT & IMPORT)
 *****************************************************************************/
int
exec_admin(int handle, T_STATEMENT * stmt)
{
    T_ADMIN *admin = &(stmt->u._admin);
    int ret;
    int i;

    switch (admin->type)
    {
    case SADM_EXPORT_ALL:
        {
            int table_cnt;
            SYSTABLE_T *sysTable;

            // get the user's table info
            table_cnt = dbi_Schema_GetUserTablesInfo(handle, &sysTable);
            if (table_cnt < 0)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, table_cnt, 0);
                return table_cnt;
            }

            for (i = 0; i < table_cnt; ++i)
            {
                if (admin->u.export.data_or_schema == 0)
                {
                    ret = dbi_Table_Export(handle,
                            admin->u.export.export_file,
                            sysTable[i].tableName,
                            (i ? 1 : admin->u.export.f_append));
                }
                else
                {
                    ret = dbi_Table_Export_Schema(handle,
                            admin->u.export.export_file,
                            sysTable[i].tableName,
                            (i ? 1 : admin->u.export.f_append), 1);
                }

                if (ret < 0)
                {
                    PMEM_FREE(sysTable);
                    er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
                    return ret;
                }
            }
            PMEM_FREE(sysTable);
        }
        break;
    case SADM_EXPORT_ONE:
        {
            if (admin->u.export.data_or_schema == 0)
            {
                ret = dbi_Table_Export(handle,
                        admin->u.export.export_file,
                        admin->u.export.table_name, admin->u.export.f_append);
            }
            else
            {
                ret = dbi_Table_Export_Schema(handle,
                        admin->u.export.export_file,
                        admin->u.export.table_name,
                        admin->u.export.f_append, 0);
            }

            if (ret < 0)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
                return ret;
            }
        }
        break;
    case SADM_IMPORT:
        {
            ret = dbi_Table_Import(handle, admin->u.import.import_file);
            if (ret < 0)
            {
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, ret, 0);
                return ret;
            }
        }
        break;
    default:
        return RET_ERROR;
    }


    return RET_SUCCESS;
}

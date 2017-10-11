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

#ifndef _SQL_UTIL_H
#define _SQL_UTIL_H

#include "mdb_config.h"
#include "dbport.h"
#include "mdb_compat.h"
#include "sql_datast.h"

#include "sql_packet_format.h"

#include "mdb_PMEM.h"

#define    STRING_BLOCK_SIZE        512
#define RESULT_PAGE_SIZE    DEFAULT_PACKET_SIZE

#ifdef MDB_DEBUG
#define INIT_STACK(stack_, size_, ret_) \
    do { \
        int     i_ = 0;                                             \
        T_POSTFIXELEM *elem_ = NULL;                                \
        (ret_) = RET_SUCCESS;                                       \
        if ((size_) <= 0)                                           \
            sc_assert(0, __FILE__, __LINE__);                       \
        (stack_).list = sql_mem_get_free_postfixelem_list(NULL, (size_));   \
        if ((stack_).list == NULL) {                                \
            (ret_) = SQL_E_OUTOFMEMORY;                             \
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, (ret_), 0);    \
        }                                                           \
        if ((ret_) != SQL_E_OUTOFMEMORY) {                          \
            (elem_) = PMEM_ALLOC(sizeof(T_POSTFIXELEM) * (size_));  \
            if ((elem_) == NULL) {                                  \
                (ret_) = SQL_E_OUTOFMEMORY;                         \
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, (ret_),0); \
            } else {                                                \
                for(i_ = 0; i_ < size_; ++i_) {                     \
                    (stack_).list[(i_)] = &(elem_)[(i_)];           \
                }                                                   \
            }                                                       \
        }                                                           \
        (stack_).max = size_; (stack_).len = 0; \
    } while(0)
#else
#define INIT_STACK(stack_, size_, ret_) \
    do { \
        int     i_;                                                 \
        T_POSTFIXELEM *elem_;                                       \
        (stack_).list = sql_mem_get_free_postfixelem_list(NULL, (size_));   \
        if ((stack_).list == NULL) {                                \
            (ret_) = SQL_E_OUTOFMEMORY;                             \
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, (ret_), 0);    \
        }                                                           \
        else {                                                      \
            (elem_) = PMEM_ALLOC(sizeof(T_POSTFIXELEM) * (size_));  \
            if ((elem_) == NULL) {                                  \
                (ret_) = SQL_E_OUTOFMEMORY;                         \
                er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, (ret_),0); \
            } else {                                                \
                (ret_) = RET_SUCCESS;                               \
                for(i_ = 0; i_ < size_; ++i_) {                     \
                    (stack_).list[(i_)] = &(elem_)[(i_)];           \
                }                                                   \
            }                                                       \
        }                                                           \
        (stack_).max = size_; (stack_).len = 0;                     \
    } while(0)
#endif

#define PUSH_STACK(stack_, elem_, ret_) \
    do { \
        sc_memcpy((stack_).list[(stack_).len++], &(elem_), sizeof(T_POSTFIXELEM)); \
        (ret_) = RET_SUCCESS; \
    } while(0)

#define POP_STACK(stack_, elem_, ret_)                              \
    do {                                                            \
        if ((stack_).len <= 0) {                                    \
            (ret_) = SQL_E_STACKUNDERFLOW;                          \
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, (ret_), 0);    \
        } else {                                                    \
            sc_memcpy(&(elem_), (stack_).list[--(stack_).len],        \
                    sizeof(T_POSTFIXELEM));                         \
            (ret_) = RET_SUCCESS;                                   \
        }                                                           \
    } while(0)

#define POP_STACK_ARRAY(stack_, array_, n_, ret_) \
    do { \
        (stack_).len -= (n_); \
        if ((stack_).len <= -1) { \
            (stack_).len += (n_); \
            (ret_) = SQL_E_STACKUNDERFLOW; \
            er_set(ER_ERROR_SEVERITY, ARG_FILE_LINE, (ret_), 0); \
        } else { \
            (array_) = &((stack_).list[(stack_).len]); \
            (ret_) = RET_SUCCESS; \
        } \
    } while(0)

#define IS_EMPTYSTACK(stack_) ((stack_).len <= -1)

#define REMOVE_STACK(stack_) \
    do { \
        int i_; \
        for (i_ = 0; i_ < (stack_).len; i_++) { \
            if ((stack_).list[i_]->elemtype == SPT_VALUE && \
                (stack_).list[i_]->u.value.is_null == 0) \
                sql_value_ptrFree(&((stack_).list[i_]->u.value));    \
        } \
        sql_mem_set_free_postfixelem(NULL, (stack_).list[0]); \
        sql_mem_set_free_postfixelem_list(NULL, (stack_).list); \
        (stack_).max = 0; (stack_).len = 0; \
    } while(0)

struct __linked_list
{
    void *ptr;
    struct __linked_list *next;
};

typedef struct stack_template
{
    void *vptr;
    struct stack_template *next;
} t_stack_template;

typedef struct t_string
{
    unsigned char *bytes;
    int len;
    int max;
} _T_STRING;

typedef struct tree_node
{
    void *vptr;
    struct tree_node *left;
    struct tree_node *right;
} T_TREE_NODE;

typedef void (*TRAVERSE_TREE_FUNCTION) (void *arg, T_POSTFIXELEM * node);

typedef struct traverse_tree_arg
{
    TRAVERSE_TREE_FUNCTION pre_fun;
    void *pre_arg;
    TRAVERSE_TREE_FUNCTION in_fun;
    void *in_arg;
    TRAVERSE_TREE_FUNCTION post_fun;
    void *post_arg;
} TRAVERSE_TREE_ARG;

#define LINK_EXPRDESC_LIST(l_, e_, ret) \
        do { \
            EXPRDESC_LIST    *n_; \
            n_ = (EXPRDESC_LIST *)PMEM_ALLOC(sizeof(EXPRDESC_LIST)); \
            if (n_ == NULL) ret = SQL_E_OUTOFMEMORY; \
            else { \
                n_->ptr = e_; \
                n_->next = l_; l_ = n_; \
                ret = RET_SUCCESS; \
            } \
        } while(0)

#define LINK_ATTRED_EXPRDESC_LIST(l_, e_, ret) \
        do { \
            ATTRED_EXPRDESC_LIST   *n_; \
            n_ = (ATTRED_EXPRDESC_LIST *)PMEM_ALLOC(sizeof(ATTRED_EXPRDESC_LIST)); \
            if (n_ == NULL) ret = SQL_E_OUTOFMEMORY; \
            else { \
                n_->ptr = e_; \
                n_->next = l_; l_ = n_; \
                ret = RET_SUCCESS; \
            } \
        } while(0)

extern void init_stack_template(t_stack_template ** stack);
extern void remove_stack_template(t_stack_template ** stack);
extern int push_stack_template(void *vptr, t_stack_template ** stack);
extern void *pop_stack_template(t_stack_template ** stack);
extern void *head_stack_template(t_stack_template ** stack);
extern int is_emptystack_template(t_stack_template ** stack);

extern t_stack_template *push_back_stack_template(void *vptr,
        t_stack_template ** stack);
extern void *iterate_stack_template(t_stack_template ** stack);
extern int make_value_from_string(T_VALUEDESC * value, char *row,
        DataType type);

extern int is_filterop(T_OPTYPE optype);

extern int append_nullstring(_T_STRING * sbuf, const char *s);
extern void traverse_tree_node(T_TREE_NODE * node, TRAVERSE_TREE_ARG * arg);
extern void destroy_tree_node(T_TREE_NODE * node);

extern bigint pow_lld(bigint a, bigint b);
extern double pow_lf(double a, bigint b);
extern float pow_f(float a, bigint b);
extern int is_logicalop(T_OPTYPE optype);
extern int is_indexop(T_OPTYPE optype);
extern int is_compop(T_OPTYPE optype);
extern T_OPTYPE get_commute_op(T_OPTYPE optype);
extern T_OPTYPE get_optype(char *op);
extern char *get_opstr(T_OPTYPE);
extern int is_firstmemberofindex(int connid, T_ATTRDESC * attr,
        char *hint_indexname, T_INDEXINFO * index);
extern int is_firstgroupofindex(int connid, T_ORDERBYDESC * orderby,
        OPENTYPE opentype, char *tablename, char *indexname,
        T_INDEXINFO * index);
extern int get_recordlen(int connid, char *tablename, SYSTABLE_T * pTableInfo);
extern int get_nullflag_offset(int connid, char *tablename,
        SYSTABLE_T * pTableInfo);
extern int h_get_recordlen(int connid, char *tablename,
        SYSTABLE_T * pTableInfo);
extern int h_get_nullflag_offset(int connid, char *tablename,
        SYSTABLE_T * pTableInfo);
extern int set_attrinfobyname(int connid, T_ATTRDESC * attr,
        SYSFIELD_T * field);
extern int set_attrinfobyid(int connid, T_ATTRDESC * attr, SYSFIELD_T * field);
extern __DECL_PREFIX int check_systable(char *table);
extern int check_validtable(int connid, char *table);
extern int check_valid_basetable(int connid, char *table);
extern void set_minvalue(DataType type, int len, T_VALUEDESC * value);
extern void set_maxvalue(DataType type, int len, T_VALUEDESC * value);

extern int get_defaultvalue(T_ATTRDESC * attr, SYSFIELD_T * field);
extern int print_value_as_string(T_VALUEDESC * value, char *buf, int bufsize);

extern int copy_value_into_record(int cursorID, char *target,
        T_VALUEDESC * res, SYSFIELD_T * field, int heap_check);

extern T_RESULT_LIST *RESULT_LIST_Create(int max_rec_size);
extern int RESULT_LIST_Add(T_RESULT_LIST * list, char *row, int size);
extern T_RESULT_PAGE *RESULT_LIST_Get_Page(T_RESULT_LIST * list);
extern char *RESULT_LIST_Fetch_as_Binary(T_RESULT_LIST * list, int *rec_len);
extern char *RESULT_LIST_Fetch_as_String(T_RESULT_LIST * list, int n_field,
        DataType * datatypes, int *rec_len);
extern __DECL_PREFIX void RESULT_LIST_Destroy(T_RESULT_LIST * list);

MDB_INLINE extern void *sql_malloc(unsigned int size, DataType type);
MDB_INLINE extern void *sql_xcalloc(unsigned int size, DataType type);
MDB_INLINE extern void sql_mfree(void *ptr);

MDB_INLINE extern int sql_value_ptrAlloc(T_VALUEDESC * value, int size);
MDB_INLINE extern int sql_value_ptrXcalloc(T_VALUEDESC * value, int size);
MDB_INLINE extern void sql_value_ptrFree(T_VALUEDESC * value);
extern int sql_value_ptrStrdup(T_VALUEDESC * value, char *src,
        int default_size);
MDB_INLINE extern void sql_value_ptrInit(T_VALUEDESC * value);

#ifdef max
#undef max
#endif
#define max(a,b)        (((unsigned int)(a)>(unsigned int)(b))?(a):(b))

#ifdef min
#undef min
#endif
#define min(a,b)        (((unsigned int)(a)<(unsigned int)(b))?(a):(b))


#define DEBUG_ENTER(s)  do { printf("%s\n", s); } while (0)

#define MODE_MANUAL     0
#define MODE_AUTO       1

#define DEF_INTEGER \
    ((1 << DT_TINYINT) | (1 << DT_SMALLINT) | (1 << DT_INTEGER) |   \
     (1 << DT_BIGINT) | (1 << DT_OID))
#define DEF_REAL        ((1 << DT_FLOAT) | (1 << DT_DOUBLE) | (1 << DT_DECIMAL))
#define IS_INTEGER(v_)  ((1 << v_) & DEF_INTEGER)
#define IS_REAL(v_)     ((1 << v_) & DEF_REAL)

#define IS_NUMERIC(v_)  ((1 << v_) & (DEF_INTEGER | DEF_REAL))

#define IS_STRING(v_)   ((1 << v_) & (DEF_BS | DEF_TIMES))

#define IS_SINGLE_OP(nest_)                                 \
    ((nest_)->cnt_single_item > 0 &&                        \
     (nest_)->cnt_single_item == (nest_)->index_field_num)

#define IS_BYTES_OR_STRING(v_) (IS_BS(v_))

extern char *__PUT_DEFAULT_VALUE(T_ATTRDESC * attr_, char *str_dec_);

#define PUT_DEFAULT_VALUE(defval_, attr_, str_dec_) \
                    defval_ = __PUT_DEFAULT_VALUE(attr_, str_dec_)

#define CHECK_OVERFLOW(value_, type_, length_, ret_) \
    do { \
        double val4dec_; \
        ret_ = RET_FALSE; \
        switch ((type_)) { \
            case DT_TINYINT: \
                if ((value_) > MDB_TINYINT_MAX || (value_) < MDB_TINYINT_MIN) \
                    ret = RET_TRUE; \
                break; \
            case DT_SMALLINT: \
                if ((value_) > MDB_SHRT_MAX || (value_) < MDB_SHRT_MIN) \
                    ret = RET_TRUE; \
                break; \
            case DT_INTEGER: \
                if ((value_) > MDB_INT_MAX || (value_) < MDB_INT_MIN) \
                    ret = RET_TRUE; \
                break; \
            case DT_BIGINT: \
                if ((value_) > MDB_LLONG_MAX || (value_) < MDB_LLONG_MIN) \
                    ret = RET_TRUE; \
                break; \
            case DT_FLOAT: \
                if ((value_) > MDB_FLT_MAX || (value_) < -MDB_FLT_MAX) \
                    ret = RET_TRUE; \
                break; \
            case DT_DOUBLE: \
                if ((value_) > MDB_DBL_MAX || (value_) < -MDB_DBL_MAX) \
                    ret = RET_TRUE; \
                break; \
            case DT_DECIMAL:                                            \
                val4dec_ = sc_pow(10, (length_));                       \
                if ((value_) >= val4dec_ || (value_) <= -val4dec_)      \
                    ret = RET_TRUE;                                     \
                break;                                                  \
            default: break;                                             \
        }                                                               \
    } while(0)

#define CHECK_UNDERFLOW(value_, type_, length_,  ret_) \
    do { \
        ret_ = RET_FALSE; \
        switch ((type_)) { \
            case DT_FLOAT: \
                if (((value_) > 0 && (value_) < MDB_FLT_MIN) || \
                    ((value_) < 0 && (value_) > -MDB_FLT_MIN)) \
                    ret = RET_TRUE; \
                break; \
            case DT_DOUBLE: \
                if (((value_) > 0 && (value_) < MDB_DBL_MIN) || \
                    ((value_) < 0 && (value_) > -MDB_DBL_MIN)) \
                    ret = RET_TRUE; \
                break; \
            default: break; \
        } \
    } while(0)

bigint __GET_INT_VALUE(T_VALUEDESC * v_, DB_BOOL is_float);

#define GET_INT_VALUE(intv_, type_, v_) intv_=(type_)__GET_INT_VALUE(v_, 0)
#define GET_INT_VALUE2(intv_, type_, v_, i_) intv_=(type_)__GET_INT_VALUE(v_, i_)

double __GET_FLOAT_VALUE(T_VALUEDESC * v_, DB_BOOL is_float);

#define GET_FLOAT_VALUE(flv_, type_, v_) flv_=(type_)__GET_FLOAT_VALUE(v_, 0)
#define GET_FLOAT_VALUE2(flv_, type_, v_, i_) flv_=(type_)__GET_FLOAT_VALUE(v_, i_)

void __PUT_INT_VALUE(T_VALUEDESC * v_, bigint int_);

#define PUT_INT_VALUE(v_, int_) __PUT_INT_VALUE(v_, int_);

void __PUT_FLOAT_VALUE(T_VALUEDESC * v_, double flv_);

#define PUT_FLOAT_VALUE(v_, flv_) __PUT_FLOAT_VALUE(v_, flv_);


#define CONNECTED(s) ((s).status!=HANDLE_STATUS_NOT_READY)

extern T_PARSING_MEMORY *sql_mem_create_chunk(void);
extern T_PARSING_MEMORY *sql_mem_destroy_chunk(T_PARSING_MEMORY * chunk);
extern T_PARSING_MEMORY *sql_mem_cleanup_chunk(T_PARSING_MEMORY * chunk);
extern void *sql_mem_alloc(T_PARSING_MEMORY * chunk, int size);
extern void *sql_mem_strdup(T_PARSING_MEMORY * chunk, char *src);
extern void *sql_mem_calloc(T_PARSING_MEMORY * chunk, int size);
extern void *sql_mem_walloc(T_PARSING_MEMORY * chunk, int size);
extern void sql_mem_free(T_PARSING_MEMORY * chunk, char *ptr);

#define sql_mem_freenul(m_, p_)                                 \
        do {                                                    \
            sql_mem_free((m_), (char*)(p_));                    \
            (p_) = NULL;                                        \
        } while(0)

extern T_POSTFIXELEM *sql_mem_get_free_postfixelem(T_PARSING_MEMORY * chunk);
extern void sql_mem_set_free_postfixelem(T_PARSING_MEMORY * chunk,
        T_POSTFIXELEM * elem);
extern T_POSTFIXELEM **sql_mem_get_free_postfixelem_list(T_PARSING_MEMORY *
        chunk, int max_expr);
extern void sql_mem_set_free_postfixelem_list(T_PARSING_MEMORY * chunk,
        T_POSTFIXELEM ** list);
extern T_EXPRESSIONDESC *sql_mem_get_free_expressiondesc(T_PARSING_MEMORY *
        chunk, int use_expr, int max_expr);
extern void sql_mem_set_free_expressiondesc(T_PARSING_MEMORY * chunk,
        T_EXPRESSIONDESC * expr);

extern int sc_bstring2binary(char *src, char *dst);
extern int sc_hstring2binary(char *src, char *dst);

extern int check_numeric_bound(T_VALUEDESC * value, DataType type, int length);
extern int sql_expr_has_ridfunc(T_EXPRESSIONDESC * expr);
extern int sql_collation_compatible_check(DataType datatype,
        MDB_COL_TYPE coltype);
extern int sql_compare_values(T_VALUEDESC * left, T_VALUEDESC * right,
        bigint * result);

extern char *mdb_lwrcpy(int n, char *dest, const char *src);

#endif // _SQL_UTIL_H

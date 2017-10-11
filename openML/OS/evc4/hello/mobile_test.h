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

FILE* SIMUL_LOG_Init(char* result_file);
void SIMUL_LOG(char* result_file, char *format, ...);

// Start point
void Start_Test(char* cfg_path, char* out_path, char* db_path);

// File Size return
int FileSize(char* path);
// Test.cfg read
int ReadTestCFG(char* cfg_path, char *param, int nSize);
// aslite.cfg set
int SetasliteCFG(char* bufSize);
// config set
int SetCFGnStart(char* cfg_path, char* out_path, char* db_path, int nsize);
// test start
int CASE_ONE_TestStart(int iterration, char* out_path, char* db_path, int nbufsize);

// DBMS create
int CreateDBMS(char* result_file, char* db_path);
// DBMS drop
int DropDBMS(char* result_file, char* db_path);

// Table create
int CreateTable(char* result_file);
// Table drop
int DropTable(char* result_file); 

// Index create
int CreateIndex(char* result_file);
// Index drop
int DropIndex(char* result_file); 

// 데이터 insert
int InsertRecord(char* result_file);

// 레코드의 값 모두 변경
int UpdateRecord(char* result_file);

// 레코드 삭제
int DeleteRecord(char* result_file);

// select test
// 싱글 테이블에서 PK를 가지고 select
int SelectSingleTableWithPK(char* result_file);
// 싱글 테이블에서 Index를 가지고 select
int SelectSingleTableWithIndex(char* result_file);
// 싱글 테이블에서 Index 없이 select
int SelectSingleTableWithoutIndex(char* result_file);
// 싱글 테이블에서 PK를 가지고 Like select
int SelectSingleTableLikeWithPK(char* result_file);
// 2개의 테이블을 조인하여 select 
int SelectJoin2WithIndex(char* result_file);
// 2개의 테이블을 조인하여 Like select
int SelectJoin2LiktWithIndex(char* result_file);
// 3개의 테이블을 조인하여 select 
int SelectJoin3WithIndex(char* result_file);
// 3개의 테이블을 조인하여 Like select
int SelectJoin3LiktWithIndex(char* result_file);


// CASE 3 테스트 start point
int CASE_THREE_TestStart(char* out_path);

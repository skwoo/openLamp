# Microsoft Developer Studio Project File - Name="openML" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=openML - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "openML.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "openML.mak" CFG="openML - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "openML - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "openML - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "openML - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "OPENML_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "OPENML_EXPORTS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x412 /d "NDEBUG"
# ADD RSC /l 0x412 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386

!ELSEIF  "$(CFG)" == "openML - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "OPENML_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I "../../../src/include" /I "../../../src/server/include" /I "../../../src/server/sql" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "WIN32_MOBILE" /D "MOBILEDB_EXPORTS" /D "MDB_DEBUG" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x412 /d "_DEBUG"
# ADD RSC /l 0x412 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=mkdir ..\bin	copy .\Debug\openML.dll ..\bin	copy .\Debug\openML.lib ..\bin
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "openML - Win32 Release"
# Name "openML - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "client"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\src\client\sql_iSQL.c
# End Source File
# End Group
# Begin Group "comm_stub"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\src\comm_stub\mdb_commstub.c
# End Source File
# End Group
# Begin Group "server"

# PROP Default_Filter ""
# Begin Group "API"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\src\server\API\mdb_dbi.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\API\mdb_Insert.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\API\mdb_Read.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\API\mdb_Remove.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\API\mdb_Schema.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\API\mdb_Update.c
# End Source File
# End Group
# Begin Group "common"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\src\server\common\mdb_charset.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\common\mdb_compat.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\common\mdb_configure.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\common\mdb_er.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\common\mdb_ErrorCode.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\common\mdb_memmgr.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\common\mdb_PMEM.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\common\mdb_ppthread.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\common\mdb_Speed.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\common\mdb_SysLog.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\common\mdb_TransMgr.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\common\sc_default.c
# End Source File
# End Group
# Begin Group "lock"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\src\server\lock\mdb_LockMgr.c
# End Source File
# End Group
# Begin Group "recovery"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\src\server\recovery\mdb_AfterLog.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\recovery\mdb_BeforeLog.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\recovery\mdb_ChkPnt.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\recovery\mdb_CLR.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\recovery\mdb_LogAnchor.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\recovery\mdb_LogBuffer.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\recovery\mdb_LogFile.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\recovery\mdb_LogMgr.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\recovery\mdb_LogRec.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\recovery\mdb_MemAnchorLog.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\recovery\mdb_PhysicalLog.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\recovery\mdb_TransAbortLog.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\recovery\mdb_TransBeginLog.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\recovery\mdb_TransCommitLog.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\recovery\mdb_UpdateTrans.c
# End Source File
# End Group
# Begin Group "sql"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\src\server\sql\sql_calc_expression.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\sql\sql_calc_function.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\sql\sql_calc_timefunc.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\sql\sql_ce_lex.yy.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\sql\sql_ce_y.tab.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\sql\sql_create.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\sql\sql_decimal.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\sql\sql_delete.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\sql\sql_drop.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\sql\sql_execute.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\sql\sql_func_timeutil.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\sql\sql_insert.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\sql\sql_interface.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\sql\sql_main.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\sql\sql_mclient.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\sql\sql_norm.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\sql\sql_parser.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\sql\sql_plan.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\sql\sql_result.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\sql\sql_select.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\sql\sql_update.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\sql\sql_util.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\sql\sql_worktable.c
# End Source File
# End Group
# Begin Group "storage"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\src\server\storage\mdb_Collect.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\storage\mdb_Cont.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\storage\mdb_ContCat.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\storage\mdb_ContIter.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\storage\mdb_CsrMgr.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\storage\mdb_DBFile.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\storage\mdb_DbMgr.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\storage\mdb_FieldDesc.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\storage\mdb_FieldVal.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\storage\mdb_Filter.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\storage\mdb_hiddenfield.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\storage\mdb_Index_Collect.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\storage\mdb_Index_MemMgr.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\storage\mdb_Index_Mgr.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\storage\mdb_Index_OID.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\storage\mdb_Index_PageList.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\storage\mdb_IndexCat.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\storage\mdb_Iterator.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\storage\mdb_KeyDesc.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\storage\mdb_KeyValue.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\storage\mdb_Mem_Mgr.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\storage\mdb_OID.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\storage\mdb_PageID.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\storage\mdb_TTree.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\storage\mdb_TTree_Create.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\storage\mdb_TTree_Insert.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\storage\mdb_TTree_Remove.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\storage\mdb_TTreeIter.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\server\storage\mdb_TTreeNode.c
# End Source File
# End Group
# End Group
# Begin Group "utils"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\src\utils\mdb_createdb.c
# End Source File
# End Group
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project

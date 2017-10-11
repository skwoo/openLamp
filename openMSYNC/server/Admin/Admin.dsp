# Microsoft Developer Studio Project File - Name="Admin" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=Admin - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "Admin.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "Admin.mak" CFG="Admin - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Admin - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "Admin - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Admin - Win32 Release"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "../../../_common/_include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "WINDOWS" /Yu"stdafx.h" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x412 /d "NDEBUG" /d "_AFXDLL"
# ADD RSC /l 0x412 /d "NDEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /machine:I386
# ADD LINK32 Ws2_32.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386 /out:"../../bin/Admin.exe"

!ELSEIF  "$(CFG)" == "Admin - Win32 Debug"

# PROP BASE Use_MFC 6
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 6
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "../../../_common/_include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_AFXDLL" /D "_MBCS" /D "WINDOWS" /FR /Yu"stdafx.h" /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x412 /d "_DEBUG" /d "_AFXDLL"
# ADD RSC /l 0x412 /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 Ws2_32.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /out:"../../bin/Admin.exe" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "Admin - Win32 Release"
# Name "Admin - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\AddDSNDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\Admin.cpp
# End Source File
# Begin Source File

SOURCE=.\Admin.rc
# End Source File
# Begin Source File

SOURCE=.\AdminDoc.cpp
# End Source File
# Begin Source File

SOURCE=.\AdminView.cpp
# End Source File
# Begin Source File

SOURCE=.\ApplicationDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\DBConnectDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\LeftTreeView.cpp
# End Source File
# Begin Source File

SOURCE=.\MainFrm.cpp
# End Source File
# Begin Source File

SOURCE=.\ModifyDSNDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\ModifyScriptDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\ModifyTableDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\MSyncIPBar.cpp
# End Source File
# Begin Source File

SOURCE=.\PanelScript.cpp
# End Source File
# Begin Source File

SOURCE=.\PanelTable.cpp
# End Source File
# Begin Source File

SOURCE=.\RightListView.cpp
# End Source File
# Begin Source File

SOURCE=.\RightView.cpp
# End Source File
# Begin Source File

SOURCE=.\SQLDirect.cpp
# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp
# ADD CPP /Yc"stdafx.h"
# End Source File
# Begin Source File

SOURCE=.\TabItem.cpp
# End Source File
# Begin Source File

SOURCE=.\UserDlg.cpp
# End Source File
# Begin Source File

SOURCE=.\WizardTableScript.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\AddDSNDlg.h
# End Source File
# Begin Source File

SOURCE=.\Admin.h
# End Source File
# Begin Source File

SOURCE=.\AdminDoc.h
# End Source File
# Begin Source File

SOURCE=.\AdminView.h
# End Source File
# Begin Source File

SOURCE=.\ApplicationDlg.h
# End Source File
# Begin Source File

SOURCE=.\DBConnectDlg.h
# End Source File
# Begin Source File

SOURCE=.\LeftTreeView.h
# End Source File
# Begin Source File

SOURCE=.\MainFrm.h
# End Source File
# Begin Source File

SOURCE=.\ModifyDSNDlg.h
# End Source File
# Begin Source File

SOURCE=.\ModifyScriptDlg.h
# End Source File
# Begin Source File

SOURCE=.\ModifyTableDlg.h
# End Source File
# Begin Source File

SOURCE=.\MSyncIPBar.h
# End Source File
# Begin Source File

SOURCE=.\PanelScript.h
# End Source File
# Begin Source File

SOURCE=.\PanelTable.h
# End Source File
# Begin Source File

SOURCE=.\Resource.h
# End Source File
# Begin Source File

SOURCE=.\RightListView.h
# End Source File
# Begin Source File

SOURCE=.\RightView.h
# End Source File
# Begin Source File

SOURCE=.\SQLDirect.h
# End Source File
# Begin Source File

SOURCE=.\StdAfx.h
# End Source File
# Begin Source File

SOURCE=.\TabItem.h
# End Source File
# Begin Source File

SOURCE=.\UserDlg.h
# End Source File
# Begin Source File

SOURCE=.\WizardTableScript.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\Admin.ico
# End Source File
# Begin Source File

SOURCE=.\res\Admin.rc2
# End Source File
# Begin Source File

SOURCE=.\res\AdminDoc.ico
# End Source File
# Begin Source File

SOURCE=.\res\applicat.bmp
# End Source File
# Begin Source File

SOURCE=.\res\closed.bmp
# End Source File
# Begin Source File

SOURCE=.\res\DB.bmp
# End Source File
# Begin Source File

SOURCE=.\res\DispTable.bmp
# End Source File
# Begin Source File

SOURCE=.\res\folder.bmp
# End Source File
# Begin Source File

SOURCE=.\res\open_sel.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Table.bmp
# End Source File
# Begin Source File

SOURCE=.\res\Toolbar.bmp
# End Source File
# Begin Source File

SOURCE=.\res\treeapp.bmp
# End Source File
# Begin Source File

SOURCE=.\res\treedb.bmp
# End Source File
# Begin Source File

SOURCE=.\res\treefile.bmp
# End Source File
# Begin Source File

SOURCE=.\res\treeupgr.bmp
# End Source File
# End Group
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# End Target
# End Project

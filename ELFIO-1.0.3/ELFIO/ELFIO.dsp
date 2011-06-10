# Microsoft Developer Studio Project File - Name="ELFIO" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=ELFIO - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ELFIO.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ELFIO.mak" CFG="ELFIO - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ELFIO - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "ELFIO - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ELFIO - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"ELFIO.lib"

!ELSEIF  "$(CFG)" == "ELFIO - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "..\..\ELFIO" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"ELFIO.lib"

!ENDIF 

# Begin Target

# Name "ELFIO - Win32 Release"
# Name "ELFIO - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\ELFIDynamic.cpp
# End Source File
# Begin Source File

SOURCE=.\ELFIImpl.cpp
# End Source File
# Begin Source File

SOURCE=.\ELFINote.cpp
# End Source File
# Begin Source File

SOURCE=.\ELFIO.cpp
# End Source File
# Begin Source File

SOURCE=.\ELFIOUtils.cpp
# End Source File
# Begin Source File

SOURCE=.\ELFIRelocation.cpp
# End Source File
# Begin Source File

SOURCE=.\ELFISection.cpp
# End Source File
# Begin Source File

SOURCE=.\ELFISegment.cpp
# End Source File
# Begin Source File

SOURCE=.\ELFIStrings.cpp
# End Source File
# Begin Source File

SOURCE=.\ELFISymbols.cpp
# End Source File
# Begin Source File

SOURCE=.\ELFODynamic.cpp
# End Source File
# Begin Source File

SOURCE=.\ELFOImpl.cpp
# End Source File
# Begin Source File

SOURCE=.\ELFONote.cpp
# End Source File
# Begin Source File

SOURCE=.\ELFORelocation.cpp
# End Source File
# Begin Source File

SOURCE=.\ELFOSection.cpp
# End Source File
# Begin Source File

SOURCE=.\ELFOSegment.cpp
# End Source File
# Begin Source File

SOURCE=.\ELFOString.cpp
# End Source File
# Begin Source File

SOURCE=.\ELFOSymbols.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\ELFI.h
# End Source File
# Begin Source File

SOURCE=.\ELFIImpl.h
# End Source File
# Begin Source File

SOURCE=.\ELFIO.h
# End Source File
# Begin Source File

SOURCE=.\ELFIOUtils.h
# End Source File
# Begin Source File

SOURCE=.\ELFO.h
# End Source File
# Begin Source File

SOURCE=.\ELFOImpl.h
# End Source File
# Begin Source File

SOURCE=.\ELFTypes.h
# End Source File
# End Group
# End Target
# End Project

/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    registry.c

Abstract:

    Registry related functions in the setupdll

    Detect Routines:
    ----------------

    1. GetEnvVar <SYSTEM | USER> <VARNAME> returns
           List <ValueName, ValueTitle, ValueRegType, ValueData>
           The value data is decomposed into a list of the elements:
           e.g. val1;val2;val3 becomes {val1, val2, val3}

    2. GetMyComputerName

    Install Routines Workers:
    -------------------------

    1. MakeLastKnownGoodWorker: To copy a given control set to another
                                  control set and indicating

    2. SetEnvVarWorker:  To set a USER or SYSTEM environment variable

    3. ExpandSzWorker: To expand all the expand sz components in an
                       environment string.


    General Subroutines:
    --------------------

    1. GetValueEntry: To fetch a value entry given a key and a value name
                      This is useful when we don't know the size of the
                      value entry.

    2. GetMaxSizeValueInKey: This is to find the maximum size needed to
                      query a value in the key.


Author:

    Sunil Pai (sunilp) April 1992

--*/
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <comstf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wcstr.h>
#include <time.h>
#include "misc.h"
#include "tagfile.h"
#include "setupdll.h"
#include <winreg.h>

extern CHAR ReturnTextBuffer[];

//
// Detect Routines
//

CB
GetEnvVar(
    IN  RGSZ    Args,
    IN  USHORT  cArgs,
    OUT SZ      ReturnBuffer,
    IN  CB      cbReturnBuffer
    )
{

    SZ      sz, sz1, szEnv;
    HKEY    hKey, hRootKey;
    DWORD   cbData;
    CB      Length;
    LONG    Status;
    DWORD   ValueType;
    PVOID   ValueData;
    CHAR    szValueType[25];
    RGSZ    rgsz;


    #define UNDEF_VAR_VALUE "{}"
    Unused( cbReturnBuffer );

    //
    // If the environment variable cannot be determine we
    // return the empty list as the value of this call.
    // Initialise this anyway to avoid doing this at a
    // a number of places.
    //

    lstrcpy( ReturnBuffer, UNDEF_VAR_VALUE );
    Length = lstrlen( UNDEF_VAR_VALUE ) + 1;


    //
    // Do parameter validation
    //

    if( cArgs < 2 ) {
        return( Length );
    }

    //
    // Check to see if the user wants a system environment variable or
    // a user environment variable.  Accordingly find out the right
    // place in the registry to look.
    //

    if ( !lstrcmpi( Args[0], "SYSTEM" ) ) {
        hRootKey = HKEY_LOCAL_MACHINE;
        szEnv    = "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment";
    }
    else if ( !lstrcmpi( Args[0], "USER" ) ) {
        hRootKey = HKEY_CURRENT_USER;
        szEnv    = "Environment";
    }
    else {
        return( Length );
    }

    //
    // Open the environment variable key.
    //

    Status = RegOpenKeyEx(
                 hRootKey,
                 szEnv,
                 0,
                 KEY_READ,
                 &hKey
                 );

    if( Status != ERROR_SUCCESS ) {
        return( Length );
    }

    //
    // Get the environment variable value
    //

    if( !GetMaxSizeValueInKey( hKey, &cbData )      ||
        ( ValueData = MyMalloc( cbData ) ) == NULL
      ) {
        RegCloseKey( hKey );
        return( Length );
    }

    Status = RegQueryValueEx(
                 hKey,
                 Args[1],
                 NULL,
                 &ValueType,
                 ValueData,
                 &cbData
                 );

    RegCloseKey( hKey );

    if (Status != ERROR_SUCCESS) {
        MyFree( ValueData );
        return( Length );
    }

    if ( ( sz = SzListValueFromPath( (SZ)ValueData ) ) == NULL ) {
        MyFree( ValueData );
        return( Length    );
    }
    MyFree( ValueData );

    ultoa( ValueType,  szValueType, 10 );

    //
    // Allocate an Rgsz structure: (Note same style as internal registry usage)
    //
    // Field 0: EnvVarValueName
    // Field 1: EnvVarTitleIndex "0"
    // Field 2: EnvVarValueType
    // Field 3: EnvVarValue
    //

    if ( rgsz = RgszAlloc( 5 ) ) {

        rgsz[0] = Args[1];
        rgsz[1] = "0";
        rgsz[2] = szValueType;
        rgsz[3] = sz;
        rgsz[4] = NULL;

        sz1 = SzListValueFromRgsz( rgsz );
        if( sz1 ) {
            lstrcpy( ReturnBuffer, sz1 );
            Length = lstrlen( sz1 ) + 1;
            MyFree( sz1 );

        }
        MyFree( rgsz );
    }
    MyFree( sz );

    return( Length );
}



CB
GetMyComputerName(
    IN  RGSZ    Args,
    IN  USHORT  cArgs,
    OUT SZ      ReturnBuffer,
    IN  CB      cbReturnBuffer
    )
/*++

Routine Description:

    DetectRoutine for GetComputerName. This finds out the computername of
    this machine in the current control set.

Arguments:

    Args   - C argument list to this detect routine (None exist)

    cArgs  - Number of arguments.

    ReturnBuffer - Buffer in which detected value is returned.

    cbReturnBuffer - Buffer Size.


Return value:

    Returns length of detected value.


--*/
{
    CHAR   ComputerName[MAX_PATH];
    DWORD  nSize = MAX_PATH;
    BOOL   bStatus = FALSE;
    CB     Length;

    #define DEFAULT_COMPUTERNAME ""

    Unused(Args);
    Unused(cArgs);
    Unused(cbReturnBuffer);


    if( GetComputerName( ComputerName, &nSize ) ) {
        lstrcpy( ReturnBuffer, ComputerName );
        Length = lstrlen( ComputerName ) + 1;
    }
    else {
        lstrcpy( ReturnBuffer, DEFAULT_COMPUTERNAME );
        Length = lstrlen( DEFAULT_COMPUTERNAME ) + 1;
    }

    return( Length );

}


//
// Install Routines
//

BOOL
SetEnvVarWorker(
    LPSTR UserOrSystem,
    LPSTR Name,
    LPSTR Title,
    LPSTR RegType,
    LPSTR Data
    )
{
    HKEY hKey, hRootKey;
    SZ   szEnv;
    LONG Status;

    Unused(Title);

    //
    // Check to see if the user wants to modify a system environment variable
    // or a user environment variable.  Accordingly find out the right
    // place in the registry to look.
    //

    if ( !lstrcmpi( UserOrSystem, "SYSTEM" ) ) {
        hRootKey = HKEY_LOCAL_MACHINE;
        szEnv    = "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment";
    }
    else if ( !lstrcmpi( UserOrSystem, "USER" ) ) {
        hRootKey = HKEY_CURRENT_USER;
        szEnv    = "Environment";
    }
    else {
        return( FALSE );
    }

    //
    // Open the environment variable key.
    //

    Status = RegOpenKeyEx(
                 hRootKey,
                 szEnv,
                 0,
                 KEY_WRITE,
                 &hKey
                 );

    if( Status != ERROR_SUCCESS ) {
        SetErrorText( IDS_ERROR_REGOPEN );
        return( FALSE );
    }

    //
    // Write the value given
    //

    Status = RegSetValueEx(
                 hKey,
                 Name,
                 0,
                 (DWORD)atol( RegType ),
                 (LPBYTE)Data,
                 lstrlen( Data ) + 1
                 );

    RegCloseKey( hKey );

    if( Status != ERROR_SUCCESS ) {
        SetErrorText( IDS_ERROR_REGSETVALUE );
        return( FALSE );
    }

    //
    // Send a WM_WININICHANGE message so that progman picks up the new
    // variable
    //

    SendMessageTimeout(
        (HWND)-1,
        WM_WININICHANGE,
        0L,
        (LONG)"Environment",
        SMTO_ABORTIFHUNG,
        1000,
        NULL
        );

    return( TRUE );
}




BOOL
ExpandSzWorker(
    LPSTR EnvironmentString,
    LPSTR ReturnBuffer,
    DWORD cbReturnBuffer
    )
{
    DWORD cbBuffer;

    cbBuffer = ExpandEnvironmentStrings(
                   (LPCSTR)EnvironmentString,
                   ReturnBuffer,
                   cbReturnBuffer
                   );

    if ( cbBuffer > cbReturnBuffer ) {
        SetErrorText( IDS_BUFFER_OVERFLOW );
        return( FALSE );
    }

    return( TRUE );
}




BOOL
MakeLastKnownGoodWorker(
    LPSTR ControlSet
    )
/*++

Routine Description:

    To generate an alternate control set from the control set given.  This
    new control set is indicated in the select parameters Reserved and
    LastKnownGood.

Arguments:

    ControlSet - A control set which has been used successfully to boot NT


Return value:

    TRUE if succeeds in creating, FALSE otherwise.  If TRUE, "ControlSet"
    is copied to a new controlset subkey and the select value entries -
    Reserved and LastKnownGood are initialized to the number representing
    the new control set.

--*/
{
    DWORD  i;
    CHAR   NewControlSet[] = "ControlSet001";
    CHAR   SetNum[4];
    CHAR   TempPath[MAX_PATH];
    DWORD  nTempPathLength = MAX_PATH;
    CHAR   TempFile[MAX_PATH];
    DWORD  nTempFileLength = MAX_PATH;
    HKEY   hKey, hKeySelect, hKeyCurrent, hKeyNew;
    UINT   Options = 0;
    DWORD  Disposition;
    TOKEN_PRIVILEGES  PrevState;
    ULONG             ReturnLength = sizeof( TOKEN_PRIVILEGES );
    LONG   Status;

    // Open all the registry nodes we need to modify upfront so that
    // we can detect access errors early

    //
    // Open the system key for read, write access
    //

    Status = RegOpenKeyEx(
                 HKEY_LOCAL_MACHINE,
                 "System",
                 0,
                 KEY_ALL_ACCESS,
                 &hKey
                 );

    if( Status != ERROR_SUCCESS )  {
        SetErrorText(IDS_ERROR_REGOPEN);
        return( FALSE );
    }

    //
    // Open the select subkey for read, write access
    //

    Status = RegOpenKeyEx(
                 hKey,
                 "Select",
                 0,
                 KEY_ALL_ACCESS,
                 &hKeySelect
                 );

    if( Status != ERROR_SUCCESS )  {
        RegCloseKey( hKey );
        SetErrorText(IDS_ERROR_REGOPEN);
        return( FALSE );
    }


    //
    // Open the controlset for read, write access
    //

    Status = RegOpenKeyEx(
                 hKey,
                 ControlSet,
                 0,
                 KEY_ALL_ACCESS,
                 &hKeyCurrent
                 );

    if( Status != ERROR_SUCCESS )  {
        RegCloseKey( hKey );
        RegCloseKey( hKeySelect );
        SetErrorText(IDS_ERROR_REGOPEN);
        return( FALSE );
    }

    //
    // Find a control set name which doesn't exist already
    //

    for( i = 1; i < 1000; i++ ) {

        wsprintf( SetNum, "%.3d", i );
        lstrcpy( NewControlSet, "ControlSet" );
        lstrcat( NewControlSet, SetNum );


        //
        // Try creating the subkey and then see if it existed already
        // or was created new
        //

        Status = RegCreateKeyEx(
                     hKey,
                     NewControlSet,
                     0,
                     "Generic",
                     0,   //BUGBUG** Once tested, this is 0
                     KEY_ALL_ACCESS,
                     NULL,
                     &hKeyNew,
                     &Disposition
                     );

        if( Status == ERROR_SUCCESS ) {
            if( Disposition & REG_CREATED_NEW_KEY ) {
                break;
            }
            else {
                RegCloseKey( hKeyNew );
            }
        }

    }

    if( i >= 1000 ) {
        RegCloseKey( hKey );
        RegCloseKey( hKeySelect );
        RegCloseKey( hKeyCurrent );
        SetErrorText(IDS_ERROR_UNIQUENAMEKEY); // BUGBUG
        return( FALSE );
    }

    //
    // We need no more access to the system key
    //

    RegCloseKey( hKey );

    //
    // Use reg save to save the current control set to a hive on disk
    //

    if( !GetWindowsDirectory( TempPath, nTempPathLength)
        || !GenerateUniqueFileName( TempPath, "reg", TempFile )
      ) {
        RegCloseKey( hKeySelect );
        RegCloseKey( hKeyCurrent );
        RegCloseKey( hKeyNew );
        SetErrorText(IDS_ERROR_TEMPFILE);
        return( FALSE );
    }

    if ( !AdjustPrivilege(
              SE_BACKUP_PRIVILEGE,
              ENABLE_PRIVILEGE,
              &PrevState,
              &ReturnLength
              )
       ) {

        RegCloseKey( hKeySelect );
        RegCloseKey( hKeyCurrent );
        RegCloseKey( hKeyNew );
        SetErrorText( IDS_ERROR_PRIVILEGE );
        return( FALSE );
    }

    Status = RegSaveKey( hKeyCurrent, TempFile, NULL );
    RestorePrivilege( &PrevState );

    if( Status != ERROR_SUCCESS ) {
        RegCloseKey( hKeySelect );
        RegCloseKey( hKeyCurrent );
        RegCloseKey( hKeyNew );
        DeleteFile(TempFile);
        SetErrorText(IDS_ERROR_REGSAVE);
        lstrcat(ReturnTextBuffer, TempFile);
        return( FALSE );
    }

    //
    // Restore the tree from the hive to the newly created control set
    //

    if ( !AdjustPrivilege(
              SE_RESTORE_PRIVILEGE,
              ENABLE_PRIVILEGE,
              &PrevState,
              &ReturnLength
              )
       ) {

        RegCloseKey( hKeySelect );
        RegCloseKey( hKeyCurrent );
        RegCloseKey( hKeyNew );
        DeleteFile(TempFile);
        SetErrorText( IDS_ERROR_PRIVILEGE );
        return( FALSE );
    }


    Status = RegRestoreKey( hKeyNew, TempFile, 0 );
    RestorePrivilege( &PrevState );

    if( Status != ERROR_SUCCESS ) {
        RegCloseKey( hKeySelect );
        RegCloseKey( hKeyCurrent );
        RegCloseKey( hKeyNew );
        DeleteFile( TempFile );
        SetErrorText(IDS_ERROR_REGRESTORE);
        return( FALSE );
    }

    //
    // Delete the temp file, if there is an error ignore. Close the
    // control set keys
    //

    DeleteFile( TempFile );
    RegCloseKey( hKeyCurrent );
    RegCloseKey( hKeyNew );

    //
    // Set the reserved and lastknown good to this new control set
    //

    Status = RegSetValueEx(
                 hKeySelect,
                 "Reserved",
                 0,
                 REG_DWORD,
                 (LPBYTE)&i,
                 sizeof(DWORD)
                 );


    if( Status != ERROR_SUCCESS ) {
        RegCloseKey( hKeySelect );
        SetErrorText(IDS_ERROR_REGSETVALUE);
        return( FALSE );
    }

    Status = RegSetValueEx(
                 hKeySelect,
                 "LastKnownGood",
                 0,
                 REG_DWORD,
                 (LPBYTE)&i,
                 sizeof( DWORD )
                 );


    if( Status != ERROR_SUCCESS ) {
        RegCloseKey( hKeySelect );
        SetErrorText(IDS_ERROR_REGSETVALUE);
        return( FALSE );
    }

    //
    // Successfully create alternate control sets, exit now
    //

    RegCloseKey( hKeySelect );

    return( TRUE );
}





BOOL
SetMyComputerNameWorker(
    LPSTR ComputerName
    )
/*++

Routine Description:

    To set the computername value in the services tree.

Arguments:

    ComputerName - Value to set as the computername


Return value:

    TRUE if successful, FALSE otherwise.

--*/
{
    if( !SetComputerName( ComputerName ) ) {
        SetErrorText(IDS_ERROR_SETCOMPUTERNAME);
        return( FALSE );
    }

    return( TRUE );
}




//=====================================================================
// The following are registry access subroutines..
//=====================================================================

BOOL
GetMaxSizeValueInKey(
    HKEY    hKey,
    LPDWORD cbData
    )
{
    LONG        Status;
    CHAR        szKClass[ MAX_PATH ];
    DWORD       cbKClass = MAX_PATH;
    DWORD       KSubKeys;
    DWORD       cbKMaxSubKeyLen;
    DWORD       cbKMaxClassLen;
    DWORD       KValues;
    DWORD       cbKMaxValueNameLen;
    DWORD       SizeSecurityDescriptor;
    FILETIME    KLastWriteTime;

    Status = RegQueryInfoKey (
                 hKey,
                 szKClass,
                 &cbKClass,
                 NULL,
                 &KSubKeys,
                 &cbKMaxSubKeyLen,
                 &cbKMaxClassLen,
                 &KValues,
                 &cbKMaxValueNameLen,
                 cbData,
                 &SizeSecurityDescriptor,
                 &KLastWriteTime
                 );

    if (Status != ERROR_SUCCESS) {
        return( FALSE );
    }

    return( TRUE );

}


PVOID
GetValueEntry(
    HKEY  hKey,
    LPSTR szValueName
    )
{
    LONG        Status;
    CHAR        szKClass[ MAX_PATH ];
    DWORD       cbKClass = MAX_PATH;
    DWORD       KSubKeys;
    DWORD       cbKMaxSubKeyLen;
    DWORD       cbKMaxClassLen;
    DWORD       KValues;
    DWORD       cbKMaxValueNameLen;
    DWORD       cbData;
    DWORD       SizeSecurityDescriptor;
    FILETIME    KLastWriteTime;
    DWORD       ValueType;


    PVOID       ValueData;

    Status = RegQueryInfoKey (
                 hKey,
                 szKClass,
                 &cbKClass,
                 NULL,
                 &KSubKeys,
                 &cbKMaxSubKeyLen,
                 &cbKMaxClassLen,
                 &KValues,
                 &cbKMaxValueNameLen,
                 &cbData,
                 &SizeSecurityDescriptor,
                 &KLastWriteTime
                 );

    if (Status != ERROR_SUCCESS) {
        return( NULL );
    }

    if ( ( ValueData = MyMalloc( cbData ) ) == NULL ) {
        return( NULL );
    }

    Status = RegQueryValueEx(
                 hKey,
                 szValueName,
                 NULL,
                 &ValueType,
                 ValueData,
                 &cbData
                 );


     if (Status != ERROR_SUCCESS) {
        MyFree( ValueData );
        return( NULL );
     }
     else {
        return( ValueData );
     }

}



BOOL
GenerateUniqueFileName(
    IN     LPSTR TempPath,
    IN     LPSTR Prefix,
    IN OUT LPSTR TempFile
    )
/*++

Routine Description:

    To generate a unique filename (with no extension) in the TempPath
    Directory with the Prefix given and digits from 1 to 99999

Arguments:

    TempPath - string containing a valid directory path ending
               in a backslash.

    Prefix   - a prefix string no longer than 3 characters.

    TempFile - buffer to return the full path to the temp file

Return value:

    TRUE if succeds in finding, FALSE otherwise.  If TRUE, TempFile buffer
    has the full path to the temp file.


--*/

{
    CHAR  Number[6];
    DWORD i;

    //
    // Check parameters
    //

    if ( TempPath    == (LPSTR)NULL
         || Prefix   == (LPSTR)NULL
         || TempFile == (LPSTR)NULL
         || lstrlen( Prefix ) > 3

       ) {

        return( FALSE );
    }

    //
    // go through all numbers upto 5 digits long looking
    // for file which doesn't exist
    //

    for( i = 1; i <99999; i++ ) {
        lstrcpy( TempFile, TempPath);
        lstrcat( TempFile, "\\" );
        lstrcat( TempFile, Prefix  );
        sprintf( Number, "%d", i );
        lstrcat( TempFile, Number );
        if (!FFileExist( TempFile ) ) {
            return( TRUE );
        }
    }

    //
    // not found, return false
    //

    return( FALSE );

}

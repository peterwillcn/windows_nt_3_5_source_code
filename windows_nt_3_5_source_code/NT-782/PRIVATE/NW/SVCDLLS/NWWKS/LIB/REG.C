/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    reg.c

Abstract:

    This module provides helpers to call the registry used by both
    the client and server sides of the workstation.

Author:

    Rita Wong (ritaw)     22-Apr-1993

--*/

#include <wcstr.h>

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windef.h>
#include <winerror.h>
#include <winbase.h>
#include <winreg.h>

#include <nwreg.h>
#include <nwapi.h>
#include <lmcons.h>
#include <lmerr.h>

static
DWORD
NwRegQueryValueExW(
    IN      HKEY    hKey,
    IN      LPWSTR  lpValueName,
    OUT     LPDWORD lpReserved,
    OUT     LPDWORD lpType,
    OUT     LPBYTE  lpData,
    IN OUT  LPDWORD lpcbData
    );

static
DWORD
EnumAndDeleteShares(
    VOID
    ) ;


DWORD
NwReadRegValue(
    IN HKEY Key,
    IN LPWSTR ValueName,
    OUT LPWSTR *Value
    )
/*++

Routine Description:

    This function allocates the output buffer and reads the requested
    value from the registry into it.

Arguments:

    Key - Supplies opened handle to the key to read from.

    ValueName - Supplies name of the value to retrieve data.

    Value - Returns a pointer to the output buffer which points to
        the memory allocated and contains the data read in from the
        registry.  This pointer must be freed with LocalFree when done.

Return Value:

    ERROR_NOT_ENOUGH_MEMORY - Failed to create buffer to read value into.

    Error from registry call.

--*/
{
    LONG    RegError;
    DWORD   NumRequired = 0;
    DWORD   ValueType;
    

    //
    // Set returned buffer pointer to NULL.
    //
    *Value = NULL;

    RegError = NwRegQueryValueExW(
                   Key,
                   ValueName,
                   NULL,
                   &ValueType,
                   (LPBYTE) NULL,
                   &NumRequired
                   );

    if (RegError != ERROR_SUCCESS && NumRequired > 0) {

        if ((*Value = (LPWSTR) LocalAlloc(
                                      LMEM_ZEROINIT,
                                      (UINT) NumRequired
                                      )) == NULL) {

            KdPrint(("NWWORKSTATION: NwReadRegValue: LocalAlloc of size %lu failed %lu\n",
                     NumRequired, GetLastError()));

            return ERROR_NOT_ENOUGH_MEMORY;
        }

        RegError = NwRegQueryValueExW(
                       Key,
                       ValueName,
                       NULL,
                       &ValueType,
                       (LPBYTE) *Value,
                       &NumRequired
                       );
    }
    else if (RegError == ERROR_SUCCESS) {
        KdPrint(("NWWORKSTATION: NwReadRegValue got SUCCESS with NULL buffer!!"));
        ASSERT(FALSE);
        return ERROR_FILE_NOT_FOUND;
    }

    if (RegError != ERROR_SUCCESS) {

        if (*Value != NULL) {
            (void) LocalFree((HLOCAL) *Value);
            *Value = NULL;
        }

        return (DWORD) RegError;
    }

    return NO_ERROR;
}


static
DWORD
NwRegQueryValueExW(
    IN HKEY hKey,
    IN LPWSTR lpValueName,
    OUT LPDWORD lpReserved,
    OUT LPDWORD lpType,
    OUT LPBYTE  lpData,
    IN OUT LPDWORD lpcbData
    )
/*++

Routine Description:

    This routine supports the same functionality as Win32 RegQueryValueEx
    API, except that it works.  It returns the correct lpcbData value when
    a NULL output buffer is specified.

    This code is stolen from the service controller.

Arguments:

    same as RegQueryValueEx

Return Value:

    NO_ERROR or reason for failure.

--*/
{    
    NTSTATUS ntstatus;
    UNICODE_STRING ValueName;
    PKEY_VALUE_FULL_INFORMATION KeyValueInfo;
    DWORD BufSize;


    UNREFERENCED_PARAMETER(lpReserved);

    //
    // Make sure we have a buffer size if the buffer is present.
    //
    if ((ARGUMENT_PRESENT(lpData)) && (! ARGUMENT_PRESENT(lpcbData))) {
        return ERROR_INVALID_PARAMETER;
    }

    RtlInitUnicodeString(&ValueName, lpValueName);

    //
    // Allocate memory for the ValueKeyInfo
    //
    BufSize = *lpcbData + sizeof(KEY_VALUE_FULL_INFORMATION) +
              ValueName.Length
              - sizeof(WCHAR);  // subtract memory for 1 char because it's included
                                // in the sizeof(KEY_VALUE_FULL_INFORMATION).

    KeyValueInfo = (PKEY_VALUE_FULL_INFORMATION) LocalAlloc(
                                                     LMEM_ZEROINIT,
                                                     (UINT) BufSize
                                                     );

    if (KeyValueInfo == NULL) {
        KdPrint(("NWWORKSTATION: NwRegQueryValueExW: LocalAlloc failed %lu\n",
                 GetLastError()));
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    ntstatus = NtQueryValueKey(
                   hKey,
                   &ValueName,
                   KeyValueFullInformation,
                   (PVOID) KeyValueInfo,
                   (ULONG) BufSize,
                   (PULONG) &BufSize
                   );

    if ((NT_SUCCESS(ntstatus) || (ntstatus == STATUS_BUFFER_OVERFLOW))
          && ARGUMENT_PRESENT(lpcbData)) {

        *lpcbData = KeyValueInfo->DataLength;
    }

    if (NT_SUCCESS(ntstatus)) {

        if (ARGUMENT_PRESENT(lpType)) {
            *lpType = KeyValueInfo->Type;
        }


        if (ARGUMENT_PRESENT(lpData)) {
            memcpy(
                lpData,
                (LPBYTE)KeyValueInfo + KeyValueInfo->DataOffset,
                KeyValueInfo->DataLength
                );
        }
    }

    (void) LocalFree((HLOCAL) KeyValueInfo);

    return RtlNtStatusToDosError(ntstatus);

}

VOID
NwLuidToWStr(
    IN PLUID LogonId,
    OUT LPWSTR LogonIdStr
    )
/*++

Routine Description:

    This routine converts a LUID into a string in hex value format so
    that it can be used as a registry key.

Arguments:

    LogonId - Supplies the LUID.

    LogonIdStr - Receives the string.  This routine assumes that this
        buffer is large enough to fit 17 characters.

Return Value:

    None.

--*/
{
    swprintf(LogonIdStr, L"%08lx%08lx", LogonId->HighPart, LogonId->LowPart);
}

VOID
NwWStrToLuid(
    IN LPWSTR LogonIdStr,
    OUT PLUID LogonId
    )
/*++

Routine Description:

    This routine converts a string in hex value format into a LUID.

Arguments:

    LogonIdStr - Supplies the string.

    LogonId - Receives the LUID.

Return Value:

    None.

--*/
{
    swscanf(LogonIdStr, L"%08lx%08lx", &LogonId->HighPart, &LogonId->LowPart);
}

VOID
NwDeleteCurrentUser(
    VOID
    )
/*++

Routine Description:

    This routine deletes the current user value under the parameters key.

Arguments:

    None.

Return Value:

    None.

--*/
{
    LONG RegError;
    HKEY WkstaKey;

    //
    // Open HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services
    // \NWCWorkstation\Parameters
    //
    RegError = RegOpenKeyExW(
                   HKEY_LOCAL_MACHINE,
                   NW_WORKSTATION_REGKEY,
                   REG_OPTION_NON_VOLATILE,
                   KEY_READ | KEY_WRITE | DELETE,
                   &WkstaKey
                   );

    if (RegError != NO_ERROR) {
        KdPrint(("NWPROVAU: NwpInitializeRegistry open NWCWorkstation\\Parameters key unexpected error %lu!\n",
                 RegError));
        return;
    }

    //
    // Delete CurrentUser value first so that the workstation won't be
    // reading this stale value. Ignore error since it may not exist.
    //
    (void) RegDeleteValueW(
               WkstaKey,
               NW_CURRENTUSER_VALUENAME
               );

    (void) RegCloseKey(WkstaKey);
}

DWORD
NwDeleteServiceLogon(
    IN PLUID Id OPTIONAL
    )
/*++

Routine Description:

    This routine deletes a specific service logon ID key in the registry
    if a logon ID is specified, otherwise it deletes all service logon
    ID keys.

Arguments:

    Id - Supplies the logon ID to delete.  NULL means delete all.

Return Status:

    None.

--*/
{
    LONG RegError;
    LONG DelError;
    HKEY ServiceLogonKey;

    WCHAR LogonIdKey[NW_MAX_LOGON_ID_LEN];


    RegError = RegOpenKeyExW(
                   HKEY_LOCAL_MACHINE,
                   NW_SERVICE_LOGON_REGKEY,
                   REG_OPTION_NON_VOLATILE,
                   KEY_READ | KEY_WRITE | DELETE,
                   &ServiceLogonKey
                   );

    if (RegError != ERROR_SUCCESS) {
        return RegError;
    }

    if (ARGUMENT_PRESENT(Id)) {

        //
        // Delete the key specified.
        //
        NwLuidToWStr(Id, LogonIdKey);

        DelError = RegDeleteKeyW(ServiceLogonKey, LogonIdKey);

    }
    else {

        //
        // Delete all service logon ID keys.
        //

        do {

            RegError = RegEnumKeyW(
                           ServiceLogonKey,
                           0,
                           LogonIdKey,
                           sizeof(LogonIdKey) / sizeof(WCHAR)
                           );

            if (RegError == ERROR_SUCCESS) {

                //
                // Got a logon id key, delete it.
                //

                DelError = RegDeleteKeyW(ServiceLogonKey, LogonIdKey);
            }
            else if (RegError != ERROR_NO_MORE_ITEMS) {
                KdPrint(("     NwDeleteServiceLogon: failed to enum logon IDs %lu\n", RegError));
            }

        } while (RegError == ERROR_SUCCESS);
    }

    (void) RegCloseKey(ServiceLogonKey);

    return ((DWORD) DelError);
}

    

DWORD
NwpRegisterGatewayShare(
    IN LPWSTR ShareName,
    IN LPWSTR DriveName
    )
/*++

Routine Description:

    This routine remembers that a gateway share has been created so
    that it can be cleanup up when NWCS is uninstalled.

Arguments:

    ShareName - name of share
    DriveName - name of drive that is shared

Return Status:

    Win32 error of any failure.

--*/
{
    DWORD status ;


    //
    // make sure we have valid parameters
    //
    if (ShareName && DriveName)
    {
        HKEY hKey ;
        DWORD dwDisposition ;

        //
        //
        // Open HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services
        // \NWCWorkstation\Shares (create it if not there)
        //
        status  = RegCreateKeyExW(
                      HKEY_LOCAL_MACHINE,
                      NW_WORKSTATION_GATEWAY_SHARES,
                      0, 
                      L"",
                      REG_OPTION_NON_VOLATILE,
                      KEY_WRITE,                 // desired access
                      NULL,                      // default security
                      &hKey,
                      &dwDisposition             // ignored
                      );

        if ( status ) 
            return status ;

        //
        // wtite out value with valuename=sharename, valuedata=drive
        //
        status = RegSetValueExW(
                     hKey,
                     ShareName,
                     0,
                     REG_SZ,
                     (LPBYTE) DriveName,
                     (wcslen(DriveName)+1) * sizeof(WCHAR)) ;
    
        (void) RegCloseKey( hKey );
    }
    else
        status = ERROR_INVALID_PARAMETER ;
    
    return status ;

}

DWORD
NwpCleanupGatewayShares(
    VOID
    )
/*++

Routine Description:

    This routine cleans up all persistent share info and also tidies
    up the registry for NWCS. Later is not needed in uninstall, but is
    there so we have a single routine that cvompletely disables the
    gateway.

Arguments:

    None.

Return Status:

    Win32 error for failed APIs.

--*/
{
    DWORD status, status1 ;
    HKEY WkstaKey = NULL;

    status = EnumAndDeleteShares() ;

    //
    // if update registry by cleaning out both Drive and Shares keys.
    // ignore return values here. the keys may not be present.
    //
    (void) RegDeleteKeyW(
                      HKEY_LOCAL_MACHINE,
                      NW_WORKSTATION_GATEWAY_DRIVES
                      ) ;

    (void) RegDeleteKeyW(
                      HKEY_LOCAL_MACHINE,
                      NW_WORKSTATION_GATEWAY_SHARES
                      ) ;
    
    //
    // Open HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services
    // \NWCWorkstation\Parameters
    //
    status1 = RegOpenKeyExW(
                   HKEY_LOCAL_MACHINE,
                   NW_WORKSTATION_REGKEY,
                   REG_OPTION_NON_VOLATILE,   // options
                   KEY_WRITE,                 // desired access
                   &WkstaKey
                   );

    if (status1 == ERROR_SUCCESS) 
    {
        //
        // delete the gateway account and gateway enabled flag.
        // ignore failures here (the values may not be present)
        //
        (void) RegDeleteValueW(
                       WkstaKey,
                       NW_GATEWAYACCOUNT_VALUENAME
                       ) ;
        (void) RegDeleteValueW(
                       WkstaKey,
                       NW_GATEWAY_ENABLE
                       ) ;

        (void) RegCloseKey( WkstaKey );
    }

    return (status ? status : status1) ;
}

DWORD
NwpClearGatewayShare(
    IN LPWSTR ShareName
    )
/*++

Routine Description:

    This routine deletes a specific share from the remembered gateway
    shares in the registry.

Arguments:

    ShareName - share value to delete

Return Status:

    Win32 status code.

--*/
{
    DWORD status = NO_ERROR ;

    //
    // check that paramter is non null
    //
    if (ShareName)
    {
        HKEY hKey ;

        //
        //
        // Open HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services
        // \NWCWorkstation\Drives
        //
        status  = RegOpenKeyExW(
                      HKEY_LOCAL_MACHINE,
                      NW_WORKSTATION_GATEWAY_SHARES,
                      REG_OPTION_NON_VOLATILE,   // options
                      KEY_WRITE,                 // desired access
                      &hKey
                      );

        if ( status ) 
            return status ;

        status = RegDeleteValueW(
                     hKey,
                     ShareName
                     ) ;
    
        (void) RegCloseKey( hKey );
    }
    else
        status = ERROR_INVALID_PARAMETER ;

    return status ;
}

typedef NET_API_STATUS (*PF_NETSHAREDEL) (
    LPWSTR server,
    LPWSTR name,
    DWORD  reserved) ;

#define NETSHAREDELSTICKY_API   "NetShareDelSticky"
#define NETSHAREDEL_API         "NetShareDel"
#define NETAPI_DLL             L"NETAPI32"

DWORD
EnumAndDeleteShares(
    VOID
    )
/*++

Routine Description:

    This routine removes all persister share info in the server for 
    all gateway shares.

Arguments:

    None.

Return Status:

    Win32 error code.

--*/
{
    DWORD err, i, type ;
    HKEY hKey = NULL ;
    FILETIME FileTime ;
    HANDLE hNetapi = NULL ;
    PF_NETSHAREDEL pfNetShareDel, pfNetShareDelSticky ;
    WCHAR Class[256], Share[NNLEN+1], Device[MAX_PATH+1] ;
    DWORD dwClass, dwSubKeys, dwMaxSubKey, dwMaxClass,
          dwValues, dwMaxValueName, dwMaxValueData, dwSDLength,
          dwShareLength, dwDeviceLength ;

    //
    // load the library so that not everyone needs link to netapi32
    //
    if (!(hNetapi = LoadLibraryW(NETAPI_DLL)))
        return (GetLastError()) ; 
 
    //
    // get addresses of the 2 functions we are interested in
    //
    if (!(pfNetShareDel = (PF_NETSHAREDEL) GetProcAddress(hNetapi,
                                               NETSHAREDEL_API)))
    {
        err = GetLastError() ; 
        goto ExitPoint ; 
    }

    if (!(pfNetShareDelSticky = (PF_NETSHAREDEL) GetProcAddress(hNetapi,
                                                     NETSHAREDELSTICKY_API)))
    {
        err = GetLastError() ; 
        goto ExitPoint ; 
    }

    //
    // Open HKEY_LOCAL_MACHINE\System\CurrentControlSet\Services
    // \NWCGateway\Shares
    //
    err = RegOpenKeyExW(
              HKEY_LOCAL_MACHINE,
              NW_WORKSTATION_GATEWAY_SHARES,
              REG_OPTION_NON_VOLATILE,   // options
              KEY_READ,                  // desired access
              &hKey
              );

    if ( err ) 
        goto ExitPoint ;

    //
    // read the info about that key
    //
    dwClass = sizeof(Class)/sizeof(Class[0]) ;
    err = RegQueryInfoKeyW(hKey,  
                           Class,
                           &dwClass, 
                           NULL, 
                           &dwSubKeys, 
                           &dwMaxSubKey, 
                           &dwMaxClass,
                           &dwValues, 
                           &dwMaxValueName, 
                           &dwMaxValueData, 
                           &dwSDLength,
                           &FileTime) ;
    if ( err ) 
    {
        goto ExitPoint ;
    }

    //
    // for each value found, we have a share to delete
    //
    for (i = 0; i < dwValues; i++)
    {
        dwShareLength = sizeof(Share)/sizeof(Share[0]) ;
        dwDeviceLength = sizeof(Device) ;
        type = REG_SZ ;
        err = RegEnumValueW(hKey,
                            i,
                            Share,
                            &dwShareLength,
                            NULL, 
                            &type,
                            (LPBYTE)Device,
                            &dwDeviceLength) ;

        //
        // cleanup the share. try delete the share proper. if not
        // there, remove the sticky info instead.
        //
        if (!err) 
        {
            err = (*pfNetShareDel)(NULL, Share, 0) ;

            if (err == NERR_NetNameNotFound) 
            {
                (void) (*pfNetShareDelSticky)(NULL, Share, 0) ;
            }
        }

        //
        // ignore errors within the loop. we can to carry on to 
        // cleanup as much as possible.
        //
        err = NO_ERROR ;
    }



ExitPoint:

    if (hKey)
        (void) RegCloseKey( hKey );

    if (hNetapi)
        (void) FreeLibrary(hNetapi) ;

    return err  ;
}

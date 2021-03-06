/*++

Copyright (c) 1990 Microsoft Corporation

Module Name:

    volume.hxx

Abstract:

    Provides volume methods.

Author:

    Mark Shavlik (marks) 13-Feb-90
    Norbert P. Kusters (norbertk) 22-Feb-91

--*/

#if !defined (VOL_LOG_IO_DP_DRIVE_DEFN)

#define VOL_LOG_IO_DP_DRIVE_DEFN

#if !defined( _SETUP_LOADER_ )

#include "drive.hxx"
#include "numset.hxx"

//
//	Forward references
//

DECLARE_CLASS( HMEM );
DECLARE_CLASS( MESSAGE );
DECLARE_CLASS( SUPERAREA );
DECLARE_CLASS( VOL_LIODPDRV	);
DECLARE_CLASS( WSTRING );
DECLARE_CLASS( WSTRING );

// This number describes the minimum number of bytes in a boot sector.
#define BYTES_PER_BOOT_SECTOR   512


typedef ULONG VOLID;

#define MAXVOLNAME 11

enum FIX_LEVEL {
    CheckOnly,
    TotalFix
};

class VOL_LIODPDRV : public LOG_IO_DP_DRIVE {

    public:

	    VIRTUAL
	    ~VOL_LIODPDRV(
            );

        NONVIRTUAL
        BOOLEAN
        Format(
            IN      PCWSTRING   Label           DEFAULT NULL,
            IN OUT  PMESSAGE    Message         DEFAULT NULL,
            IN      ULONG       ClusterSize     DEFAULT 0,
            IN      ULONG       VirtualSectors  DEFAULT 0
            );

        NONVIRTUAL
        BOOLEAN
        ChkDsk(
            IN      FIX_LEVEL   FixLevel,
            IN OUT  PMESSAGE    Message         DEFAULT NULL,
            IN      BOOLEAN     Verbose         DEFAULT FALSE,
            IN      BOOLEAN     OnlyIfDirty     DEFAULT FALSE,
            IN      BOOLEAN     RecoverFree     DEFAULT FALSE,
            IN      BOOLEAN     RecoverAlloc    DEFAULT FALSE,
            OUT     PULONG      ExitStatus      DEFAULT NULL
            );

        NONVIRTUAL
        BOOLEAN
        Recover(
            IN      PCWSTRING   FileName,
            IN OUT  PMESSAGE    Message DEFAULT NULL
            );

        NONVIRTUAL
	    PSUPERAREA
        GetSa(
            );

        VIRTUAL
        PVOL_LIODPDRV
        QueryDupVolume(
            IN      PCWSTRING   NtDriveName,
            IN OUT  PMESSAGE    Message         DEFAULT NULL,
            IN      BOOLEAN     ExclusiveWrite  DEFAULT FALSE,
            IN      BOOLEAN     FormatMedia     DEFAULT FALSE,
            IN      MEDIA_TYPE  MediaType       DEFAULT Unknown
            ) CONST PURE;

        NONVIRTUAL
        BOOLEAN
        ForceAutochk(
            IN  BOOLEAN     Recover,
            IN  PCWSTRING   Name
        );


    protected:

		DECLARE_CONSTRUCTOR( VOL_LIODPDRV );

        NONVIRTUAL
        BOOLEAN
        Initialize(
            IN      PCWSTRING    NtDriveName,
            IN      PSUPERAREA          SuperArea,
            IN OUT  PMESSAGE            Message         DEFAULT NULL,
            IN      BOOLEAN             ExclusiveWrite  DEFAULT FALSE,
            IN      BOOLEAN             FormatMedia     DEFAULT FALSE,
            IN      MEDIA_TYPE          MediaType       DEFAULT Unknown
            );

        NONVIRTUAL
		BOOLEAN
		Initialize(
            IN      PCWSTRING   NtDriveName,
            IN      PCWSTRING   HostFileName,
            IN      PSUPERAREA  SuperArea,
            IN OUT  PMESSAGE    Message         DEFAULT NULL,
            IN      BOOLEAN     ExclusiveWrite  DEFAULT FALSE
			);


	private:

		NONVIRTUAL
		VOID
		Construct (
			);

        NONVIRTUAL
        VOID
        Destroy(
            );

        NONVIRTUAL
        SECTORCOUNT
        ReadABunch(
            IN OUT  PHMEM               HeapMem,
            IN      LBN                 StartLbn,
            IN      SECTORCOUNT         NumSectors,
            IN OUT  PMESSAGE            Message         DEFAULT NULL,
            IN      PCWSTRING    SrcDosDriveName DEFAULT NULL
            );

        PSUPERAREA  _sa;
        NUMBER_SET  _bad_sectors;

};


INLINE
PSUPERAREA
VOL_LIODPDRV::GetSa(
    )
/*++

Routine Description:

    This routine returns a pointer to the current super area.

Arguments:

    None.

Return Value:

    A pointer to the current super area.

--*/
{
	return _sa;
}

#else // _SETUP_LOADER_ is defined

#include "drive.hxx"
#include "intstack.hxx"

//
//	Forward references
//

DECLARE_CLASS( HMEM );
DECLARE_CLASS( MESSAGE );
DECLARE_CLASS( SUPERAREA );
DECLARE_CLASS( VOL_LIODPDRV	);
DECLARE_CLASS( WSTRING );
DECLARE_CLASS( WSTRING );


typedef ULONG VOLID;

#define MAXVOLNAME 11

enum FIX_LEVEL {
    CheckOnly,
    TotalFix
};

// This number describes the minimum number of bytes in a boot sector.
#define BYTES_PER_BOOT_SECTOR   512

class VOL_LIODPDRV : public LOG_IO_DP_DRIVE {

    public:

	    VIRTUAL
	    ~VOL_LIODPDRV(
            );

        NONVIRTUAL
        BOOLEAN
        ChkDsk(
            IN      FIX_LEVEL   FixLevel,
            IN OUT  PMESSAGE    Message     DEFAULT NULL,
			IN		BOOLEAN 	Verbose     DEFAULT FALSE,
			IN		BOOLEAN 	OnlyIfDirty DEFAULT FALSE
            );

        NONVIRTUAL
	    PSUPERAREA
        GetSa(
            );

        VIRTUAL
        BOOLEAN
        IsHpfs(
            );

        VIRTUAL
        BOOLEAN
        IsNtfs(
            );

        VIRTUAL
        ARC_STATUS
        MarkDirty(
            ) PURE;

        VIRTUAL
        ARC_STATUS
        Flush(
            IN  BOOLEAN JustHandle
            ) PURE;

    protected:

		DECLARE_CONSTRUCTOR( VOL_LIODPDRV );

        NONVIRTUAL
        BOOLEAN
        Initialize(
            IN      ULONG       DeviceId,
            IN OUT  PSUPERAREA  SuperArea
            );

    private:

		NONVIRTUAL
		VOID
		Construct (
			);

        NONVIRTUAL
        VOID
        Destroy(
            );

        NONVIRTUAL
        SECTORCOUNT
        ReadABunch(
            IN OUT  PHMEM               HeapMem,
            IN      LBN                 StartLbn,
            IN      SECTORCOUNT         NumSectors,
            IN OUT  PMESSAGE            Message         DEFAULT NULL,
            IN      PCWSTRING    SrcDosDriveName DEFAULT NULL
            );

        PSUPERAREA  _sa;
        INTSTACK    _bad_sectors;

};


INLINE
PSUPERAREA
VOL_LIODPDRV::GetSa(
    )
/*++

Routine Description:

    This routine returns a pointer to the current super area.

Arguments:

    None.

Return Value:

    A pointer to the current super area.

--*/
{
	return _sa;
}

INLINE
BOOLEAN
VOL_LIODPDRV::IsHpfs(
    )
/*++

Routine Description:

    This method determines whether the volume is HPFS.

Arguments:

    None.

Return Value:

    TRUE if this volume is an HPFS volume.

--*/
{
    return FALSE;
}

INLINE
BOOLEAN
VOL_LIODPDRV::IsNtfs(
    )
/*++

Routine Description:

    This method determines whether the volume is NTFS.

Arguments:

    None.

Return Value:

    TRUE if this volume is an NTFS volume.

--*/
{
    return FALSE;
}

#endif

#endif

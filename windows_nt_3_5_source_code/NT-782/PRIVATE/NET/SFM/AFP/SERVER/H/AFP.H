/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	afp.h

Abstract:

	This file defines some server globals as well as include all relevant
	header files.

Author:

	Jameel Hyder (microsoft!jameelh)


Revision History:
	25 Apr 1992             Initial Version

Notes:  Tab stop: 4
--*/


#ifndef _AFP_
#define _AFP_

#include <ntos.h>
#include <ntseapi.h>
#include <ntlsa.h>
#include <ntmsv1_0.h>
#include <ntiolog.h>
#define	_NTIFS_
#include <fsrtl.h>
#include <string.h>
#include <wcstr.h>
#include <ntiologc.h>
#include <tdi.h>
#include <tdikrnl.h>

#if		DBG && DEVL
#define	DEBUG
#endif

#ifdef  DEBUG
#define LOCAL
#else
#define LOCAL
// #define LOCAL   static
#endif

#ifdef	_GLOBALS_
#define	GLOBAL
#define	EQU				=
#else
#define	GLOBAL			extern
#define	EQU				; / ## /
#endif

#include <atalktdi.h>
#include <afpconst.h>
#include <fwddecl.h>
#include <intrlckd.h>
#include <macansi.h>
#include <macfile.h>
#include <admin.h>
#include <swmr.h>
#include <fileio.h>
#include <server.h>
#include <forks.h>
#include <sda.h>
#include <afpinfo.h>
#include <idindex.h>
#include <desktop.h>
#ifdef	USINGPATHCACHE
#include <idcache.h>
#endif
#include <atalkio.h>
#include <volume.h>
#include <afpmem.h>
#include <errorlog.h>
#include <srvmsg.h>
#include <time.h>
#include <lists.h>
#include <filenums.h>

#endif  // _AFP_

/***
*inittime.c - contains _init_time
*
*	Copyright (c) 1991-1993, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	Contains the locale-category initialization function: _init_time().
*	
*	Each initialization function sets up locale-specific information
*	for their category, for use by functions which are affected by
*	their locale category.
*
*	*** For internal use by setlocale() only ***
*
*Revision History:
*	12-08-91  ETC	Created.
*	12-20-91  ETC	Updated to use new NLSAPI GetLocaleInfo.
*	12-18-92  CFW	Ported to Cuda tree, changed _CALLTYPE4 to _CRTAPI3.
*	12-29-92  CFW	Updated to use new _getlocaleinfo wrapper function.
*	01-25-93  KRS	Adapted to use ctry or lang dependent data, as approp.
*	02-08-93  CFW	Casts to remove warnings.
*	02-16-93  CFW	Added support for date and time strings.
*	03-09-93  CFW	Use char* time_sep in storeTimeFmt.
*	05-20-93  GJF	Include windows.h, not individual win*.h files
*	06-11-93  CFW	Now inithelp takes void *.
*
*******************************************************************************/

#ifdef _INTL

#include <stdlib.h>
#include <windows.h>
#include <locale.h>
#include <setlocal.h>
#include <malloc.h>

static int _CRTAPI3 _get_lc_time(struct _lc_time_data *lc_time);
static void _CRTAPI3 _free_lc_time(struct _lc_time_data *lc_time);

/* Pointer to current time strings */
extern struct _lc_time_data *_lc_time_curr;

/* C locale time strings */
extern struct _lc_time_data _lc_time_c;

/* Pointer to non-C locale time strings */
struct _lc_time_data *_lc_time_intl = NULL;

int storeTimeFmt(
	int ctryid,
	struct _lc_time_data *lc_time
);

/***
*int _init_time() - initialization for LC_TIME locale category.
*
*Purpose:
*	In non-C locales, read the localized time/date strings into
*	_lc_time_intl, and set _lc_time_curr to point to it.  The old
*	_lc_time_intl is not freed until the new one is fully established.
*	
*	In the C locale, _lc_time_curr is made to point to _lc_time_c.
*	Any allocated _lc_time_intl structures are freed.
*
*Entry:
*	None.
*
*Exit:
*	0 success
*	1 fail
*
*Exceptions:
*
*******************************************************************************/

int _CRTAPI3 _init_time (
	void
	)
{
	/* Temporary date/time strings */
	struct _lc_time_data *lc_time;

	if (_lc_handle[LC_TIME] != _CLOCALEHANDLE)
   {
		/* Allocate structure filled with NULL pointers */
		if ((lc_time = (struct _lc_time_data *) 
			calloc (1, sizeof(struct _lc_time_data))) == NULL)
			return 1;

		if (_get_lc_time (lc_time))
      {
			_free_lc_time (lc_time);
			free ((void *)lc_time);
			return 1;
		}

		_lc_time_curr = lc_time;	/* point to new one */
		_free_lc_time (_lc_time_intl);	/* free the old one */
		free ((void *)_lc_time_intl);
		_lc_time_intl = lc_time;
		return 0;

	} else {
		_lc_time_curr = &_lc_time_c;	/* point to new one */
		_free_lc_time (_lc_time_intl);	/* free the old one */
		free ((void *)_lc_time_intl);
		_lc_time_intl = NULL;
		return 0;
	}
}

/*
 *  Get the localized time strings.
 *  Of course, this can be beautified with some loops!
 */
static int _CRTAPI3 _get_lc_time (
	struct _lc_time_data *lc_time
	)
{
   int ret = 0;

/* Some things are language-dependent and some are country-dependent.
   This works around an NT limitation and lets us distinguish the two. */

   LCID langid = MAKELCID(_lc_id[LC_TIME].wLanguage, SORT_DEFAULT);
   LCID ctryid = MAKELCID(_lc_id[LC_TIME].wCountry, SORT_DEFAULT);

   if (lc_time == NULL)
	   return -1;

/* All the text-strings are Language-dependent: */

   ret |= _getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SABBREVDAYNAME1, (void *)&lc_time->wday_abbr[1]);
   ret |= _getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SABBREVDAYNAME2, (void *)&lc_time->wday_abbr[2]);
   ret |= _getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SABBREVDAYNAME3, (void *)&lc_time->wday_abbr[3]);
   ret |= _getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SABBREVDAYNAME4, (void *)&lc_time->wday_abbr[4]);
   ret |= _getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SABBREVDAYNAME5, (void *)&lc_time->wday_abbr[5]);
   ret |= _getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SABBREVDAYNAME6, (void *)&lc_time->wday_abbr[6]);
   ret |= _getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SABBREVDAYNAME7, (void *)&lc_time->wday_abbr[0]);

   ret |= _getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SDAYNAME1, (void *)&lc_time->wday[1]);
   ret |= _getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SDAYNAME2, (void *)&lc_time->wday[2]);
   ret |= _getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SDAYNAME3, (void *)&lc_time->wday[3]);
   ret |= _getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SDAYNAME4, (void *)&lc_time->wday[4]);
   ret |= _getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SDAYNAME5, (void *)&lc_time->wday[5]);
   ret |= _getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SDAYNAME6, (void *)&lc_time->wday[6]);
   ret |= _getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SDAYNAME7, (void *)&lc_time->wday[0]);

   ret |= _getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SABBREVMONTHNAME1, (void *)&lc_time->month_abbr[0]);
   ret |= _getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SABBREVMONTHNAME2, (void *)&lc_time->month_abbr[1]);
   ret |= _getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SABBREVMONTHNAME3, (void *)&lc_time->month_abbr[2]);
   ret |= _getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SABBREVMONTHNAME4, (void *)&lc_time->month_abbr[3]);
   ret |= _getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SABBREVMONTHNAME5, (void *)&lc_time->month_abbr[4]);
   ret |= _getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SABBREVMONTHNAME6, (void *)&lc_time->month_abbr[5]);
   ret |= _getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SABBREVMONTHNAME7, (void *)&lc_time->month_abbr[6]);
   ret |= _getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SABBREVMONTHNAME8, (void *)&lc_time->month_abbr[7]);
   ret |= _getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SABBREVMONTHNAME9, (void *)&lc_time->month_abbr[8]);
   ret |= _getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SABBREVMONTHNAME10, (void *)&lc_time->month_abbr[9]);
   ret |= _getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SABBREVMONTHNAME11, (void *)&lc_time->month_abbr[10]);
   ret |= _getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SABBREVMONTHNAME12, (void *)&lc_time->month_abbr[11]);

   ret |= _getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SMONTHNAME1, (void *)&lc_time->month[0]);
   ret |= _getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SMONTHNAME2, (void *)&lc_time->month[1]);
   ret |= _getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SMONTHNAME3, (void *)&lc_time->month[2]);
   ret |= _getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SMONTHNAME4, (void *)&lc_time->month[3]);
   ret |= _getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SMONTHNAME5, (void *)&lc_time->month[4]);
   ret |= _getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SMONTHNAME6, (void *)&lc_time->month[5]);
   ret |= _getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SMONTHNAME7, (void *)&lc_time->month[6]);
   ret |= _getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SMONTHNAME8, (void *)&lc_time->month[7]);
   ret |= _getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SMONTHNAME9, (void *)&lc_time->month[8]);
   ret |= _getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SMONTHNAME10, (void *)&lc_time->month[9]);
   ret |= _getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SMONTHNAME11, (void *)&lc_time->month[10]);
   ret |= _getlocaleinfo(LC_STR_TYPE, langid, LOCALE_SMONTHNAME12, (void *)&lc_time->month[11]);

   ret |= _getlocaleinfo(LC_STR_TYPE, langid, LOCALE_S1159, (void *)&lc_time->ampm[0]);
   ret |= _getlocaleinfo(LC_STR_TYPE, langid, LOCALE_S2359, (void *)&lc_time->ampm[1]);


/* The following relate to time format and are Country-dependent: */

	ret |= _getlocaleinfo(LC_STR_TYPE, ctryid, LOCALE_SSHORTDATE, (void *)&lc_time->ww_sdatefmt);
	ret |= _getlocaleinfo(LC_STR_TYPE, ctryid, LOCALE_SLONGDATE, (void *)&lc_time->ww_ldatefmt);

/* NLS API dropped time format strings, we have to make up our own */
	ret |= storeTimeFmt(ctryid, lc_time);

   return ret;
}

/*
 *  Free the localized time strings.
 *  Of course, this can be beautified with some loops!
 */
static void _CRTAPI3 _free_lc_time (
	struct _lc_time_data *lc_time
	)
{
   if (lc_time == NULL)
	   return;

   free ((void *)lc_time->wday_abbr[1]);
   free ((void *)lc_time->wday_abbr[2]);
   free ((void *)lc_time->wday_abbr[3]);
   free ((void *)lc_time->wday_abbr[4]);
   free ((void *)lc_time->wday_abbr[5]);
   free ((void *)lc_time->wday_abbr[6]);
   free ((void *)lc_time->wday_abbr[0]);

   free ((void *)lc_time->wday[1]);
   free ((void *)lc_time->wday[2]);
   free ((void *)lc_time->wday[3]);
   free ((void *)lc_time->wday[4]);
   free ((void *)lc_time->wday[5]);
   free ((void *)lc_time->wday[6]);
   free ((void *)lc_time->wday[0]);

   free ((void *)lc_time->month_abbr[0]);
   free ((void *)lc_time->month_abbr[1]);
   free ((void *)lc_time->month_abbr[2]);
   free ((void *)lc_time->month_abbr[3]);
   free ((void *)lc_time->month_abbr[4]);
   free ((void *)lc_time->month_abbr[5]);
   free ((void *)lc_time->month_abbr[6]);
   free ((void *)lc_time->month_abbr[7]);
   free ((void *)lc_time->month_abbr[8]);
   free ((void *)lc_time->month_abbr[9]);
   free ((void *)lc_time->month_abbr[10]);
   free ((void *)lc_time->month_abbr[11]);

   free ((void *)lc_time->month[0]);
   free ((void *)lc_time->month[1]);
   free ((void *)lc_time->month[2]);
   free ((void *)lc_time->month[3]);
   free ((void *)lc_time->month[4]);
   free ((void *)lc_time->month[5]);
   free ((void *)lc_time->month[6]);
   free ((void *)lc_time->month[7]);
   free ((void *)lc_time->month[8]);
   free ((void *)lc_time->month[9]);
   free ((void *)lc_time->month[10]);
   free ((void *)lc_time->month[11]);

   free ((void *)lc_time->ampm[0]);
   free ((void *)lc_time->ampm[1]);

   free ((void *)lc_time->ww_sdatefmt);
   free ((void *)lc_time->ww_ldatefmt);
   free ((void *)lc_time->ww_timefmt);
/* Don't need to make these pointers NULL */
}

	
/* NLS API dropped time format strings, we have to make up our own */
int storeTimeFmt(
	int ctryid,
	struct _lc_time_data *lc_time
)
{
	int ret = 0;
	char *pfmt, *psep;
	char *time_sep;
	unsigned clock24 = 0;
	unsigned leadzero = 0;

   ret |= _getlocaleinfo(LC_INT_TYPE, ctryid, LOCALE_ITIME, (void *)&clock24);
   ret |= _getlocaleinfo(LC_INT_TYPE, ctryid, LOCALE_ITLZERO, (void *)&leadzero);
   ret |= _getlocaleinfo(LC_STR_TYPE, ctryid, LOCALE_STIME, (void *)&time_sep);

	if (ret)
		return ret;
	
	/* 13 = 6 chars for hhmmss + up to three for each separator + NULL */

	pfmt = lc_time->ww_timefmt = (char *)malloc(13);

	if (clock24)
	{
		*pfmt++ = 'H';
		if (leadzero)
			*pfmt++ = 'H';
	}
	else {
		*pfmt++ = 'h';
		if (leadzero)
			*pfmt++ = 'h';
	}
	for(psep = time_sep; *psep; *pfmt++ = *psep++) ;
	*pfmt++ = 'm';
	if (leadzero)
		*pfmt++ = 'm';
	for(psep = time_sep; *psep; *pfmt++ = *psep++) ;
	*pfmt++ = 's';
	*pfmt++ = 's';
	*pfmt = '\0';

	free(time_sep);

	return ret;
}

#endif /* _INTL */

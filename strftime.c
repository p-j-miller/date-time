/*  strftime.c
	==========
	Note that the function defined here is called ya_strftime() to avoid issues with tryng to rename an existing strftime() function (especially in C++).

   See comments at the start of strptime.c for the format options supported (basically C99 in the C locale with a couple of extensions).
    
   This file was created by starting from strftime.c obtained from https://github.com/arnoldrobbins/strftime on 3/4/2022 
   A significant number of changes mave been made, initially to make it compile for Windows with C99 using TDM-GCC 10.3.0, but then to expand the functionality and remove bugs.
   This file now compiles and runs correctly with devC++/TDMgcc 10.3.0 on Windows, Builder C++ (10.2) on Windows and gcc on Linux.
   
   This file uses the structure struct strp_tz_struct strp_tz to pass information between strptime() and strftime() and to support timezones in a way thats independent of the operating system.
   In general if strptime() is not used then strftime() will get things like the timezone from the OS [ Windows/Linux ], but if strptime() is called the last strptime() call may provide these.
   This allows for example strptime() to be used to decode "time stamps" in a "Log file" recorded at a different time (and even time zone), 
   with strftime() used to display values from these timestamps as if it was in the locale where the log was created.
   If you want to guarantee that no information is passed into strftime() from strptime(), then call init_strp_tz() before calling strptime(). 
   
   An extensive test program is also provided (main.c).
   For gcc under Linux compile test program with :

     gcc -Wall -O3 -o date-time main.c strftime.c strptime.c
     ./date-time
  
   For dev-C++ (tested with tdmgcc 10.3.0) there is a *.dev file, and the project files (*.cbproj) for Builder C++ are also present. 
   You should see no errors or warnings when compiling these files.
   
   Thanks to Arnold Robbins and the other contributors to the original file who are all listed below in the original file header. 
*/
/*----------------------------------------------------------------------------
 * Copyright (c) 2022 Peter Miller
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHOR OR COPYRIGHT HOLDER BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *--------------------------------------------------------------------------*/
 
/* Header from file as obtained from github: https://github.com/arnoldrobbins/strftime on 3/4/2022  */
/* 
 * strftime.c
 *
 * Public-domain implementation of ISO C library routine.
 *
 * If you can't do prototypes, get GCC.
 *
 * The C99 standard now specifies just about all of the formats
 * that were additional in the earlier versions of this file.
 *
 * For extensions from SunOS, add SUNOS_EXT.
 * For extensions from HP/UX, add HPUX_EXT.
 * For VMS dates, add VMS_EXT.
 * For complete POSIX semantics, add POSIX_SEMANTICS.
 *
 * The code for %X follows the C99 specification for
 * the "C" locale.
 *
 * The code for %c, and %x follows the C11 specification for
 * the "C" locale.
 *
 * With HAVE_NL_LANGINFO defined, locale-based values are used.
 *
 * This version doesn't worry about multi-byte characters.
 *
 * Arnold Robbins
 * January, February, March, 1991
 * Updated March, April 1992
 * Updated April, 1993
 * Updated February, 1994
 * Updated May, 1994
 * Updated January, 1995
 * Updated September, 1995
 * Updated January, 1996
 * Updated July, 1997
 * Updated October, 1999
 * Updated September, 2000
 * Updated December, 2001
 * Updated January, 2011
 * Updated April, 2012
 * Updated March, 2015
 * Updated June, 2015
 *
 * Fixes from ado@elsie.nci.nih.gov,
 * February 1991, May 1992
 * Fixes from Tor Lillqvist tml@tik.vtt.fi,
 * May 1993
 * Further fixes from ado@elsie.nci.nih.gov,
 * February 1994
 * %z code from chip@chinacat.unicom.com,
 * Applied September 1995
 * %V code fixed (again) and %G, %g added,
 * January 1996
 * %v code fixed, better configuration,
 * July 1997
 * Moved to C99 specification.
 * September 2000
 * Fixes from Tanaka Akira <akr@m17n.org>
 * December 2001
 */
/* end of original header */

#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <stdint.h>
// #include <inttypes.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
// #include <math.h>
#include "time_local.h"

#define YEAR0LEAP /* if defined make year 0 a leap year (if not defined then its not). This should normally be defined as ISO 8601 says year 0 is a leap year */

/* defaults: season to taste , note tests below use the fact if these are defined or not, the value does not matter  */
// #define SUNOS_EXT		/* stuff in SunOS strftime routine */
// #define VMS_EXT			/* include %v for VMS date format */
// #define HPUX_EXT		/* non-conflicting stuff in HP-UX date */
// #define POSIX_SEMANTICS		/* call tzset() if TZ changes */
// #define POSIX_2008		/* flag and fw for C, F, G, Y formats */
// #define HAVE_NL_LANGINFO		/* locale-based values [ does not work for Windows at present ] */
#define HAVE_TZNAME 1 /* has to be defined as 1 ! */

#ifdef HAVE_NL_LANGINFO
#include <locale.h>
#endif

#ifdef __BORLANDC__
 /* builder C++ */
#define timezone _timezone /* _timezone is the difference, in seconds, between the current local time and Greenwich mean time */
#define daylight _daylight /* _daylight is set to 1 if the daylight savings time conversion should be applied */
#endif

#ifdef __linux
 /* linux */
 #define _tzname tzname
#endif

extern const char * strp_weekdays[] ;/* full names of weekdays and months - defined in strptime.c */
extern const char * strp_monthnames[];

extern void tzset(void);
static int weeknumber(const struct tm *timeptr, int firstweekday);
static int iso8601wknum(const struct tm *timeptr);


#define range(low, item, hi)	max(low, min(item, hi))


#undef min	/* just in case */

/* min --- return minimum of two numbers */

static inline int
min(int a, int b)
{
	return (a < b ? a : b);
}

#undef max	/* also, just in case */

/* max --- return maximum of two numbers */

static inline int
max(int a, int b)
{
	return (a > b ? a : b);
}

#ifdef __linux
 /* need strnicmp() , note we just need equal & != here the ordering does not matter
    if char is signed, you use chars with more than 7 bits and you are using this function for ordering check if it does what you want ! 
*/	
int strnicmp( const char * s1, const char * s2, size_t n )
{
 while ( n && *s1 && ( tolower(*s1) == tolower(*s2) ) )
    {
     ++s1;
     ++s2;
     --n;
    }
 if ( n == 0 ) return 0;
 return tolower(*s1) - tolower(*s2);

}
#endif 

// 3 functions below based on those in K&R 2nd ed pp 111, changed so month is 0..11 to match the rest of the code
static char daytab[2][13]={ /* table of days in a month for non leap years and leap years*/
	 {31,28,31,30,31,30,31,31,30,31,30,31},
	 {31,29,31,30,31,30,31,31,30,31,30,31}
};


bool is_leap(int64_t year) /* returns true if year [with no offset] is a leap year */
{if(year==0) 
#ifdef YEAR0LEAP 
	return true; //  year 0 IS a leap year 
#else
	return false; // year 0 is a NOT a leap year 
#endif	
 else if(year<0) year= -year; // just in case % operator does silly things with negative arguments
 return (year%4==0 && year%100!=0) || year%400==0; // no need to worry about sign of year here as just check ==0 or !=0
}


void month_day(int64_t year, int yearday, int*pmonth, int *pday)
/* set month, day from year (actual year with no offset) and day of year (actually days since jan 1st, 0-365) */
/* returns pmonth as 0->11, 0-Jan, and pday=1->31 */
{int i;
 bool leap =is_leap(year);
 ++yearday; // supplied as 0=jan 1st, below treats that as the 1st day of the year
 for(i=0;yearday>daytab[leap][i] && i<12;i++) // test for i<12 avoids overrunning the array if yearday is too big
	yearday-=daytab[leap][i];
 *pmonth=i;// 0->11
 *pday=yearday;
}

int day_of_year(int64_t year, int month, int day) // year without offset, month 0->11, day 1->31, returns days since 1st jan (0>365)
{bool leap =is_leap(year);
 if(month>11) month=11;// ensure we don't overrun the array
 for(int i=0;i<month;++i)
 	 day+=daytab[leap][i];
 return day-1; // days since 1st jan start at 0
}

static inline time_t year_to_s(int64_t year)
{ // converts year to seconds since 1st Jan 1970.
  // While year is a int64_t its assumed to come from an int (32 bits) with a 1900 offset, so we don't need to worry about overflow in the conversion
 int64_t lyears;
 /* we want to factor in leap years to the year before noting year 0 is not a leap year, so 0,1,2,3,4,5,6,.. we want to count 1st leap year when year=5 (as year 4 is a leap year) 
 	for negative years the first one we want to count is the year -5 */
 if(year>=0) lyears=year-1; /* 5-1=4 so lyears/4=1.  0 => -1 but -1/4 is still 0 so thats OK */
 else lyears=year+1;/* -5 +1 = -4 so lyears/4= -1 */
  // calculation now just needs to use yday, year and lyears
#ifdef YEAR0LEAP  
	/* if year>0 then 1 more leap year to add, also adds 1 to offset for 1970 so overall this cancels out for 1970 */
 return ( (time_t)(lyears/4) - (time_t)(lyears/100) + (time_t)(lyears/400 )   + (time_t)(year>0?1:0)+  
		  (time_t)year*365 - (time_t)719528  /* offset to make 1st Jan 1970=0 */
	    )*(time_t)86400; /* 86400=24*60*60;  hours->minutes->seconds */ 
#else
 return ( (time_t)(lyears/4) - (time_t)(lyears/100) + (time_t)(lyears/400 )   +  
		  (time_t)year*365 - (time_t)719527  /* offset to make 1st Jan 1970=0 */
	    )*(time_t)86400; /* 86400=24*60*60;  hours->minutes->seconds */
#endif	    
}

static inline time_t ya_mktime_s(int64_t year, int month, int mday, int yday, int hour, int min, int sec ) /* version of mktime() that returns secs  */
{// Converts Gregorian date to seconds since 1970-01-01 00:00:00.
 // Gregorian date has been used in the UK since 1752 ,but in most other places since 15th Oct 1582
 // leap seconds allowed in sec, and midnight=24:00:00 is also allowed (but this is not allowed by strptime() ).
 // ignores timezone 
 // ignores leap seconds in previous years (so is currently 37 sec in error) - as the timing of future leap seconds is unknown these would be impossible to allow for!
 // used by %s format
 // year is actual year (no offset) - its assume year is basically an int, we use int64_t to allow year+1900 to be passed as an argument without overflowing as tm_year=0 for year 1900.
 // month 0..11, mday 1..31.
 // yday is the number of days since 1st Jan 0-365
 // if mday<=0 (which is invalid) then use yday, otherwise use month, mday. As all values in tm are initialised to 0 mday==0 means its not been set
 if(mday>0 && yday <=0)
 	{// get yday from month & mday [ yday might already be valid, but we have no way to know ]
 	 yday=day_of_year(year,month,mday);
 	}
 return year_to_s(year)+((( (time_t)yday 
						    )*24 + hour /* now have hours - midnight tomorrow handled here */
						  )*60 + min /* now have minutes */
						)*60 + sec; /* finally seconds */
}


time_t ya_mktime_tm(const struct tm *tp) /* version of mktime() that returns secs and takes (but does not change) timeptr */
{// Converts Gregorian date to seconds since 1970-01-01 00:00:00.
 // just calls ya_mktime() with the corect arguments
 return ya_mktime_s((int64_t)tp->tm_year+1900,tp->tm_mon,tp->tm_mday,tp->tm_yday,tp->tm_hour,tp->tm_min,tp->tm_sec);
}



int day_of_week_yd(int64_t year,int yday) /* year with no offset eg 1900 and yday is days since 1st Jan 0->365 */ 
{// time_t ya_mktime_s(int64_t year, int month,int mday,const int yday,const int hour,const int min, const int sec ) /* version of mktime() that returns secs  */
 int64_t sy=ya_mktime_s(year, 0,0,yday,0,0,0 ); /* version of mktime() that returns secs since 1st Jan 1970 */
 int64_t days=sy/(3600*24); 
 days=(days+4)%7; // +4 as 1st jan 1970 (secs=0) was a Thursday . 
 if(days<0)days+=7;
 return days;
}

int day_of_week(int64_t y, int m, int d)	/* 0 = Sunday  */
{ // year with no offset, m 0=jan..11=dec, d = day of month 1..31
 int yday=day_of_year(y,m,d);
 return day_of_week_yd(y,yday);
}

int cmp_tm(struct tm *tm1,struct tm *tm2) /* returns sign(tm1-tm2) */
{time_t s1,s2;
 s1=ya_mktime_tm(tm1);
 s2=ya_mktime_tm(tm2);
 if(s1==s2) return 0;
 else if(s1<s2) return -1;
 return 1;
}

#if 0 /* use a binary search to find the year This takes 11.4 secs for test program vs 10.3 secs for approximation below, so #if 0 is recommended */
void sec_to_tm(time_t t,struct tm *tp) // reverse of ya_mktime_tm, converts secs since epoch to the numbers of tp
{
 int64_t year;
 int month=0, mday=1, hour=0, min=0, sec=0,yday=0;
 time_t t_y;
 /* binary search for year .. */
 time_t high=INT_MAX,low= -INT_MAX,mid;
 bool found=false;
 high*=2; low*=2;// widen range to allow for offset from int in struct tm
 while(low<=high)
 	{mid=(low+high)/2;// no need to worry about overflow as int 32 bits and maths in 64 bits
 	 t_y=year_to_s(mid);// jan 1st 0:0:0 of year mid 
 	 if(t_y >t)
 	 	high=mid-1;// we are using integers
 	 else if(t_y<t)
	  	low=mid+1;
	 else 
	 	{found=true;
		 break; // t_y=t
		}
	}

 if(!found)
 	{ // low <=high is false to cause termination, so high is the lower
	 year=high;
	 t_y=year_to_s(high);
	}
else year=mid;// found exactly 

 t-=t_y;// get remainder = secs into year
 yday=t/(24*3600);// 24 hrs in a day so this gives us days in the year
 t-=yday*24*3600; // whats left is seconds in the day

 month_day(year,yday,&month,&mday);
 hour=t/3600;
 t-=hour*3600;
 min=t/60;
 sec=t-min*60;
 if(year-1900 < -INT_MAX ) year=-INT_MAX+1900; // clip at min (subtract 1900 below)
 else if(year-1900 > INT_MAX ) year=(int64_t)INT_MAX+1900; // clip at max (subtract 1900 below)
 tp->tm_year=year-1900;
 tp->tm_mon=month;
 tp->tm_mday=mday;
 tp->tm_hour=hour;
 tp->tm_min=min;
 tp->tm_sec=sec;
 tp->tm_yday=yday;
 tp->tm_wday=day_of_week_yd(year,yday);
 // does not set  tm_isdst
}
#else /* use an approximation to find the year */
void sec_to_tm(time_t t,struct tm *tp) // reverse of ya_mktime_tm, converts secs since epoch to the numbers of tp
{/* note that sec_to_tm() sets all fields in tm (except tz), whereas ya_mktime_tm() does not need all fields set to work, so calling ya_mktime_tm() then sec_to_tm() will ensure all fields are set */
 int64_t year;
 int month, mday, hour, min, sec,yday;
 time_t t_y;
#ifdef YEAR0LEAP  
 year=(t+INT64_C(62167219200))/INT64_C(31556952); // +719528*86400=62,167,219,200 removes "1970" offset added in year_to_s(), 31,556,952=365.2425*24*60*60 which approximates leap years (100-4+1=97 year years in 400 years so 97/400=.2425)
#else 
 year=(t+INT64_C(62167132800))/INT64_C(31556952); // +719527*86400=62,167,132,800 removes "1970" offset added in year_to_s(), 31,556,952=365.2425*24*60*60 which approximates leap years (100-4+1=97 year years in 400 years so 97/400=.2425)
#endif 
 t_y=year_to_s(year); 
 for(int i=0; t_y<t && i<10000;++i)// i stops infinite loops, should only go around this a few times as our initial guess should be quite accurate
 	{t_y=year_to_s(++year); // have to start 1 higher
 	}
 for(int i=0; t_y>t && i<10000;++i) // i stops infinite loops , our initial guess should be out by a small amount as our approximation above is quite accurate
	{
	 t_y=year_to_s(--year); // keep reducing year till its smaller
	}	
 t-=t_y;// get remainder = secs into year
 yday=t/(24*3600);// 24 hrs in a day so this gives us days in the year
 t-=yday*24*3600; // whats left is seconds in the day
 month_day(year,yday,&month,&mday);
 hour=t/3600;
 t-=hour*3600;
 min=t/60;
 sec=t-min*60;
 if(year-1900 < -INT_MAX ) year=-INT_MAX+1900; // clip at min (subtract 1900 below)
 else if(year-1900 > INT_MAX ) year=(int64_t)INT_MAX+1900; // clip at max (subtract 1900 below)
 tp->tm_year=year-1900;
 tp->tm_mon=month;
 tp->tm_mday=mday;
 tp->tm_hour=hour;
 tp->tm_min=min;
 tp->tm_sec=sec;
 tp->tm_yday=yday;
 tp->tm_wday=day_of_week_yd(year,yday);
 // does not set  tm_isdst
}
#endif

time_t ya_mktime(struct tm *tp) /* fully functional version of mktime() that returns secs and takes (and changes if necessary) timeptr */
{time_t s;
 s=ya_mktime_tm(tp); // convert tp to secs
 sec_to_tm(s,tp); // convert secs back to tp - setting all fields of tp to standardised values
 return s;
}

time_t UTC_mktime(struct tm *tp,struct strp_tz_struct *tz ) /* version of mktime() that also uses struct strp_tz_struct to adjust secs returned for timezones. Returns UTC secs since epoch (time_t) */
{time_t s;
 s=ya_mktime_tm(tp); // convert tp to secs
 sec_to_tm(s,tp); // convert secs back to tp - setting all fields of tp to standardised values
 if(tp->tm_isdst>0) s-=3600; // if in summer time (daylight savings time) local time has an extra 1 hour added, so subtract that here.
 if(tz->tz_off_mins!=strp_tz_default) s-=60*tz->tz_off_mins; // correct for time zone ofset							 
 return s;
}

void UTC_sec_to_tm(time_t t,struct tm *tp,struct strp_tz_struct *tz ) // reverse of UTC_mktime(), converts UTC time as secs since epoch to the numbers of tp, takinginto account struct strp_tz to adjust secs  for timezones
{time_t s=t;
 if(tp->tm_isdst>0) s+=3600; // if in summer time (daylight savings time) local time has an extra 1 hour added, so add that here.
 if(tz->tz_off_mins!=strp_tz_default) s+=60*tz->tz_off_mins; // correct for time zone ofset		
 sec_to_tm(s,tp); 	
}

#ifdef POSIX_2008
/* iso_8601_2000_year --- format a year per ISO 8601:2000 as in 1003.1 */

static void
iso_8601_2000_year(char *buf, int year, size_t fw)
{
	int extra;
	char sign = '\0';

	if (year >= -9999 && year <= 9999) {
		sprintf(buf, "%0*d", (int) fw, year);
		return;
	}

	/* now things get weird */
	if (year > 9999) {
		sign = '+';
	} else {
		sign = '-';
		year = -year;
	}

	extra = year / 10000;
	year %= 10000;
	sprintf(buf, "%c_%04d_%d", sign, extra, year);
}
#endif /* POSIX_2008 */

#ifdef HAVE_NL_LANGINFO
/* days_a() --- return the short name for the day of the week */
static const char *
days_a(int index)
{
	static const nl_item data[] = {
		ABDAY_1,
		ABDAY_2,
		ABDAY_3,
		ABDAY_4,
		ABDAY_5,
		ABDAY_6,
		ABDAY_7,
	};
	return nl_langinfo(data[index]);
}

/* days_l() --- return the long name for the day of the week */
static const char *
days_l(int index)
{
	static const nl_item data[] = {
		DAY_1,
		DAY_2,
		DAY_3,
		DAY_4,
		DAY_5,
		DAY_6,
		DAY_7,
	};
	return nl_langinfo(data[index]);
}

/* months_a() --- return the short name for the month */
static const char *
months_a(int index)
{
	static const nl_item data[] = {
		ABMON_1,
		ABMON_2,
		ABMON_3,
		ABMON_4,
		ABMON_5,
		ABMON_6,
		ABMON_7,
		ABMON_8,
		ABMON_9,
		ABMON_10,
		ABMON_11,
		ABMON_12,
	};
	return nl_langinfo(data[index]);
}

/* months_l() --- return the short name for the month */
static const char *
months_l(int index)
{
	static const nl_item data[] = {
		MON_1,
		MON_2,
		MON_3,
		MON_4,
		MON_5,
		MON_6,
		MON_7,
		MON_8,
		MON_9,
		MON_10,
		MON_11,
		MON_12,
	};
	return nl_langinfo(data[index]);
}

/* ampm() --- return am/pm string */
static const char *
ampm(int index)
{
	static const nl_item data[] = {
		AM_STR,
		PM_STR,
	};
	return nl_langinfo(data[index]);
}
#endif /* ifdef HAVE_NL_LANGINFO */


/* strftime() --- produce formatted time */
size_t ya_strftime(char *s, size_t maxsize, const char *format, const struct tm *timeptr)
{
	char *endp = s + maxsize;
	char *start = s;
	auto char tbuf[100];
	long off;
	int i, w;
	long y;
	static short first = 1;
#ifdef POSIX_SEMANTICS
	static char *savetz = NULL;
	static int savetzlen = 0;
	char *tz;
#endif /* POSIX_SEMANTICS */

#ifdef POSIX_2008
	int pad;
	size_t fw;
	char flag;
#endif /* POSIX_2008 */

	if(strp_tz.initialised==0)
    	init_strp_tz(&strp_tz);
#ifdef __GNUC__ 
 #pragma GCC diagnostic ignored "-Wnonnull-compare" /* Peter Miller - "fix" incorrect gcc warning 321	26	strftime.c	[Warning] 'nonnull' argument 'format' compared to NULL [-Wnonnull-compare] */
#endif
	if (s == NULL || format == NULL || timeptr == NULL || maxsize == 0)
		return 0;
#ifdef __GNUC__ 		
 #pragma GCC diagnostic warning "-Wnonnull-compare" /* turn warning back on again */
#endif 
	/* quick check if we even need to bother */
	if (strchr(format, '%') == NULL && strlen(format) + 1 >= maxsize)
		return 0;

#ifndef POSIX_SEMANTICS
	if (first) {
		tzset();
		first = 0;
	}
#else	/* POSIX_SEMANTICS */
	tz = getenv("TZ");
	if (first) {
		if (tz != NULL) {
			int tzlen = strlen(tz);

			savetz = (char *) malloc(tzlen + 1);
			if (savetz != NULL) {
				savetzlen = tzlen + 1;
				strcpy(savetz, tz);
			}
		}
		tzset();
		first = 0;
	}
	/* if we have a saved TZ, and it is different, recapture and reset */
	if (tz && savetz && (tz[0] != savetz[0] || strcmp(tz, savetz) != 0)) {
		i = strlen(tz) + 1;
		if (i > savetzlen) {
			savetz = (char *) realloc(savetz, i);
			if (savetz) {
				savetzlen = i;
				strcpy(savetz, tz);
			}
		} else
			strcpy(savetz, tz);
		tzset();
	}
#endif	/* POSIX_SEMANTICS */

	for (; *format && s < endp - 1; format++) {
		tbuf[0] = '\0';
		if (*format != '%') {
			*s++ = *format;
			continue;
		}
#ifdef POSIX_2008
		pad = '\0';
		fw = 0;
		flag = '\0';
		switch (*++format) {
		case '+':
			flag = '+';
			/* fall through */
		case '0':
			pad = '0';
			format++;
			break;

		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			break;

		default:
			format--;
			goto again;
		}
		for (; isdigit(*format); format++) {
			fw = fw * 10 + (*format - '0');
		}
		format--;
#endif /* POSIX_2008 */

	again:
		switch (*++format) {
		case '\0':
			*s++ = '%';
			goto out;

		case '%':
			*s++ = '%';
			continue;

		case 'a':	/* abbreviated weekday name */
			if (timeptr->tm_wday < 0 || timeptr->tm_wday > 6)
				strcpy(tbuf, "?");
			else
#ifndef HAVE_NL_LANGINFO
                {
                 strncpy(tbuf,strp_weekdays[timeptr->tm_wday],3);
                 tbuf[3]=0;// make null terminated after 1st 3 chars
                }
#else			
				strcpy(tbuf, days_a(timeptr->tm_wday));
#endif				
			break;

		case 'A':	/* full weekday name */
			if (timeptr->tm_wday < 0 || timeptr->tm_wday > 6)
				strcpy(tbuf, "?");
			else
#ifndef HAVE_NL_LANGINFO
                strcpy(tbuf, strp_weekdays[timeptr->tm_wday]);
#else			
				strcpy(tbuf, days_l(timeptr->tm_wday));
#endif				
			break;

		case 'b':	/* abbreviated month name */
		short_month:
			if (timeptr->tm_mon < 0 || timeptr->tm_mon > 11)
				strcpy(tbuf, "?");
			else
#ifndef HAVE_NL_LANGINFO
                {
                 strncpy(tbuf,strp_monthnames[timeptr->tm_mon],3);
                 tbuf[3]=0;// make null terminated after 1st 3 chars
                }
#else			
				strcpy(tbuf, months_a(timeptr->tm_mon));
#endif				
			break;

		case 'B':	/* full month name */
			if (timeptr->tm_mon < 0 || timeptr->tm_mon > 11)
				strcpy(tbuf, "?");
			else
#ifndef HAVE_NL_LANGINFO
                strcpy(tbuf, strp_monthnames[timeptr->tm_mon]);
#else				
				strcpy(tbuf, months_l(timeptr->tm_mon));
#endif				
			break;

		case 'c':	/* appropriate date and time representation */
			/*
			 * This used to be:
			 *
			 * strftime(tbuf, sizeof tbuf, "%a %b %e %H:%M:%S %Y", timeptr);
			 *
			 * Per the ISO 1999 C standard, it was this:
			 * strftime(tbuf, sizeof tbuf, "%A %B %d %T %Y", timeptr);
			 *
			 * Per the ISO 2011 C standard, it is now this:
			 */
#ifdef HAVE_NL_LANGINFO
			ya_strftime(tbuf, sizeof tbuf, nl_langinfo(D_T_FMT), timeptr);
#else
			{ const char *f="%a %b %e %T %Y";// done this way to avoid gcc -Wformat= errors in line below
			 ya_strftime(tbuf, sizeof tbuf, f, timeptr);
			}
#endif
			break;

		case 'C':
#ifdef POSIX_2008
			if (pad != '\0' && fw > 0) {
				size_t min_fw = (flag ? 3 : 2);

				fw = max(fw, min_fw);
				snprintf(tbuf,sizeof(tbuf), flag
						? "%+0*ld"
						: "%0*ld", (int) fw,
						(timeptr->tm_year + 1900L) / 100);
			} else
#endif /* POSIX_2008 */
#ifdef HPUX_EXT
		century:
#endif			
				snprintf(tbuf,sizeof(tbuf), "%02ld", (timeptr->tm_year + 1900L) / 100);
			break;

		case 'd':	/* day of the month, 01 - 31 */
			i = range(1, timeptr->tm_mday, 31);
			snprintf(tbuf,sizeof(tbuf), "%02d", i);
			break;

		case 'D':	/* date as %m/%d/%y */
			ya_strftime(tbuf, sizeof tbuf, "%m/%d/%y", timeptr);
			break;

		case 'e':	/* day of month, blank padded */
			snprintf(tbuf,sizeof(tbuf), "%2d", range(1, timeptr->tm_mday, 31));
			break;

		case 'E':
			/* POSIX (now C99) locale extensions, ignored for now */
			goto again;
		case 'f': /* local extension - fractional part of seconds */
			{
			 if(strp_tz.f_secs_p10>=0)
				{// -ve values for f_secs_p10 are not allowed (default is big negative)
#if 1
				 snprintf(tbuf,sizeof(tbuf),"%0.*f",strp_tz.f_secs_p10,strp_tz.f_secs);
				 if(tbuf[0]=='0' && tbuf[1]=='.')
					{ // delete leading 0. from number as we only want digits after decimal point
					 char *s1,*s2;
					 for(s1=tbuf,s2=tbuf+2;*s1!=0 && *s2!=0;)*s1++=*s2++; // copy backwards so we overwrite 0. at start which we don't want
					 *s1=0;// null terminate resultant string (above copy should have stopped with *s2==0)
					}
#else
 #ifdef __GNUC__
  #pragma GCC diagnostic ignored "-Wformat=" /* Peter Miller - "fix" [Warning] unknown conversion type character '#' in format [-Wformat=] */
  #pragma GCC diagnostic ignored "-Wformat-extra-args" /* Peter Miller - "fix" [Warning] too many arguments for format [-Wformat-extra-args] */
 #endif
				 snprintf(tbuf,sizeof(tbuf),"%0.*#f",strp_tz.f_secs_p10,strp_tz.f_secs);// the # in this line causes gcc compiler warnings which are turned off before and on again after this line.
 #ifdef __GNUC__
  #pragma GCC diagnostic warning "-Wformat" /* turn warning back on again. If this says  "-Wformat=" as you might expect to match "ignored" above, GCCgives an error and fails compilation! */
  #pragma GCC diagnostic warning "-Wformat-extra-args" /* turn warning back on again */
 #endif
				 char *s1,*s2;
				 for(s1=tbuf,s2=tbuf+2;*s1!=0 && *s2!=0;)*s1++=*s2++; // copy backwards so we overwrite 0. at start which we don't want
				 *s1=0;// null terminate resultant string (above copy should have stopped with *s2==0)
#endif
			 	}
			 else
			 	{tbuf[0]='?'; // produce "?" for invalid inputs (otherwise test program fails)
			 	 tbuf[1]=0;
				}
			}		
			break;
		case 'F':	/* ISO 8601 date representation */
		{
#ifdef POSIX_2008
			/*
			 * Field width for %F is for the whole thing.
			 * It must be at least 10.
			 */
			char m_d[10];
			ya_strftime(m_d, sizeof m_d, "-%m-%d", timeptr);
			size_t min_fw = 10;

			if (pad != '\0' && fw > 0) {
				fw = max(fw, min_fw);
			} else {
				fw = min_fw;
			}

			fw -= 6;	/* -XX-XX at end are invariant */

			iso_8601_2000_year(tbuf, timeptr->tm_year + 1900, fw);
			strcat(tbuf, m_d);
#else
			ya_strftime(tbuf, sizeof tbuf, "%Y-%m-%d", timeptr);
#endif /* POSIX_2008 */
		}
			break;

		case 'g':
		case 'G':
			/*
			 * Year of ISO week.
			 * If strp_tz.year_G!= strp_tz_default we already have it, otherwsie calculate it as below
			 *  If it's December but the ISO week number is one,
			 *  that week is in next year.
			 *  If it's January but the ISO week number is 52 or
			 *  53, that week is in last year.
			 *  Otherwise, it's this year.
			 */
			if(strp_tz.year_G== strp_tz_default)
				{
				 w = iso8601wknum(timeptr);
				 if (timeptr->tm_mon == 11 && w == 1)
					y = 1900L + timeptr->tm_year + 1;
				 else if (timeptr->tm_mon == 0 && w >= 52)
					y = 1900L + timeptr->tm_year - 1;
				 else
					y = 1900L + timeptr->tm_year;
				}
			else
				y=strp_tz.year_G;	// simple case - we already have the correct year
			if (*format == 'G') {
#ifdef POSIX_2008
				if (pad != '\0' && fw > 0) {
					size_t min_fw = 4;

					fw = max(fw, min_fw);
					snprintf(tbuf,sizeof(tbuf), flag
							? "%+0*ld"
							: "%0*ld", (int) fw,
							y);
				} else
#endif /* POSIX_2008 */
					snprintf(tbuf,sizeof(tbuf), "%ld", y);
			}
			else
				snprintf(tbuf,sizeof(tbuf), "%02ld", y % 100);
			break;

		case 'h':	/* abbreviated month name */
			goto short_month;

		case 'H':	/* hour, 24-hour clock, 00 - 23 */
			i = range(0, timeptr->tm_hour, 23);
			snprintf(tbuf,sizeof(tbuf), "%02d", i);
			break;

		case 'I':	/* hour, 12-hour clock, 01 - 12 */
			i = range(0, timeptr->tm_hour, 23);
			if (i == 0)
				i = 12;
			else if (i > 12)
				i -= 12;
			snprintf(tbuf,sizeof(tbuf), "%02d", i);
			break;

		case 'j':	/* day of the year, 001 - 366 */
			snprintf(tbuf,sizeof(tbuf), "%03d", timeptr->tm_yday + 1);
			break;

		case 'm':	/* month, 01 - 12 */
			i = range(0, timeptr->tm_mon, 11);
			snprintf(tbuf,sizeof(tbuf), "%02d", i + 1);
			break;

		case 'M':	/* minute, 00 - 59 */
			i = range(0, timeptr->tm_min, 59);
			snprintf(tbuf,sizeof(tbuf), "%02d", i);
			break;

		case 'n':	/* same as \n */
			tbuf[0] = '\n';
			tbuf[1] = '\0';
			break;

		case 'O':
			/* POSIX (now C99) locale extensions, ignored for now */
			goto again;

		case 'p':	/* am or pm based on 12-hour clock */
			i = range(0, timeptr->tm_hour, 23);
#ifndef HAVE_NL_LANGINFO
			if (i < 12)
				strcpy(tbuf, "AM");
			else
				strcpy(tbuf, "PM");
#else			
			if (i < 12)
				strcpy(tbuf, ampm(0));
			else
				strcpy(tbuf, ampm(1));
#endif				
			break;

		case 'r':	/* time as %I:%M:%S %p */
			ya_strftime(tbuf, sizeof tbuf, "%I:%M:%S %p", timeptr);
			break;

		case 'R':	/* time as %H:%M */
			ya_strftime(tbuf, sizeof tbuf, "%H:%M", timeptr);
			break;
			
		case 's':	/* time as seconds since the Epoch */
			 snprintf(tbuf,sizeof(tbuf), "%lld", (long long)ya_mktime_tm(timeptr)); // (long long) added as time_t might only be long
			 break;
			 
		case 'S':	/* second, 00 - 60 */
			i = range(0, timeptr->tm_sec, 60);
			snprintf(tbuf,sizeof(tbuf), "%02d", i);
			break;

		case 't':	/* same as \t */
			tbuf[0] = '\t';
			tbuf[1] = '\0';
			break;

		case 'T':	/* time as %H:%M:%S */
		the_time:
			ya_strftime(tbuf, sizeof tbuf, "%H:%M:%S", timeptr);
			break;

		case 'u':
		/* ISO 8601: Weekday as a decimal number [1 (Monday) - 7] {very similar to %w which outputs 0->6 with sunday as 0 }*/
			i = range(0, timeptr->tm_wday, 6);
			snprintf(tbuf,sizeof(tbuf), "%d", i== 0 ? 7 :i);
			break;

		case 'U':	/* week of year, Sunday is first day of week */
				//  if a value has been set in strp_tz.week_nos_U then use that, otherwise calculate it from other entries		
			if(strp_tz.week_nos_U== strp_tz_default)
				snprintf(tbuf,sizeof(tbuf), "%02d", weeknumber(timeptr, 0));
			else
				snprintf(tbuf,sizeof(tbuf), "%02d", strp_tz.week_nos_U);				
			break;

		case 'V':	/* week of year according ISO 8601 */
			//  if a value has been set in strp_tz.week_nos_V then use that, otherwise calculate it from other entries
			if(strp_tz.week_nos_V== strp_tz_default)
				snprintf(tbuf,sizeof(tbuf), "%02d", iso8601wknum(timeptr));
			else
				snprintf(tbuf,sizeof(tbuf), "%02d", strp_tz.week_nos_V);
			break;

		case 'w':	/* weekday, Sunday == 0, 0 - 6 */
			i = range(0, timeptr->tm_wday, 6);
			snprintf(tbuf,sizeof(tbuf), "%d", i);
			break;

		case 'W':	/* week of year, Monday is first day of week */
				//  if a value has been set in strp_tz.week_nos_U then use that, otherwise calculate it from other entries		
			if(strp_tz.week_nos_W== strp_tz_default)		
				snprintf(tbuf,sizeof(tbuf), "%02d", weeknumber(timeptr, 1));
			else
				snprintf(tbuf,sizeof(tbuf), "%02d", strp_tz.week_nos_W);				
			break;

		case 'x':	/* appropriate date representation */
			/*
			 * Up to the 2011 standard, this code used:
			 * strftime(tbuf, sizeof tbuf, "%A %B %d %Y", timeptr);
			 *
			 * Now, per the 2011 C standard (C99), this is: "%m/%d/%y"
			 */
#ifdef HAVE_NL_LANGINFO
			ya_strftime(tbuf, sizeof tbuf, nl_langinfo(D_FMT), timeptr);
#else
			ya_strftime(tbuf, sizeof tbuf, "%m/%d/%y", timeptr);
#endif
			break;

		case 'X':	/* appropriate time representation */
#ifdef HAVE_NL_LANGINFO
			ya_strftime(tbuf, sizeof tbuf, nl_langinfo(T_FMT), timeptr);
#else
			goto the_time; /* same as %T */
#endif
			break;

		case 'y':	/* year without a century, 00 - 99 */
#ifdef HPUX_EXT		
		year:
#endif			
			if(timeptr->tm_year>=0)
				i = timeptr->tm_year % 100;
			else
				i = 100+(timeptr->tm_year % 100); // Peter Miller - fix for negative years, which otherwise gave a negative i
			snprintf(tbuf,sizeof(tbuf), "%02d", i);
			break;

		case 'Y':	/* year with century */
#ifdef POSIX_2008
			if (pad != '\0' && fw > 0) {
				size_t min_fw = 4;

				fw = max(fw, min_fw);
				snprintf(tbuf,sizeof(tbuf), flag
						? "%+0*ld"
						: "%0*ld", (int) fw,
						1900L + timeptr->tm_year);
			} else
#endif /* POSIX_2008 */
			snprintf(tbuf,sizeof(tbuf), "%ld", 1900L + timeptr->tm_year);
			break;

 		case 'z':	/* time zone offset east of GMT e.g. -0600 */
 			if(strp_tz.tz_off_mins== strp_tz_default)
 				{// strp_tz.tz_off_mins has not been set - get value from operating system
 				 if (timeptr->tm_isdst < 0)
 					break;
				 /*
			 	  * Systems with tzname[] probably have timezone as
			 	  * secs west of GMT.  Convert to mins east of GMT.
			 	 */
				 off = -timezone / 60;
				 if (off < 0) 
				 	{
					 tbuf[0] = '-';
					 off = -off;
				 	} 
				  else 
				  	{
					 tbuf[0] = '+';
					}
				 snprintf(tbuf+1,sizeof(tbuf)-1, "%02ld%02ld", off/60, off%60);
				}
			else
				{//strp_tz.tz_off_mins has been set - use it
				 off = strp_tz.tz_off_mins;			
				}
			// common code
			if (off < 0) 
			 	{
				 tbuf[0] = '-';
				 off = -off;
			 	} 
			 else 
			  	{
				 tbuf[0] = '+';
				}
			snprintf(tbuf+1,sizeof(tbuf)-1, "%02ld%02ld", off/60, off%60);					
			break;

		case 'Z':	/* time zone name or abbrevation */
#if 1
			if(strp_tz.tz_name[0]!=0) // has been set by strptime()
				{for(int i=0;i<4;++i)
					tbuf[i]=strp_tz.tz_name[i];
				 tbuf[4]=0;// make 0 terminated string 
				}
			else
				{// has NOT been set by strptime(), use OS supplied value 	
				 int j;
				 i = (daylight && timeptr->tm_isdst > 0); /* 0 or 1 */
				 for(j=0;j<4 && isalpha(_tzname[i][j]);++j); // at most 4 chars - all must be letters 
				 strncpy(tbuf, _tzname[i],j); // at most 4 chars copied 
				 tbuf[j]=0;// ensure 0 terminated string 
				}
#else	/* original code, use OS value always */
			i = (daylight && timeptr->tm_isdst > 0); /* 0 or 1 */
			strcpy(tbuf, _tzname[i]);
#endif			
			break;

#ifdef SUNOS_EXT
		case 'k':	/* hour, 24-hour clock, blank pad */
			snprintf(tbuf,sizeof(tbuf), "%2d", range(0, timeptr->tm_hour, 23));
			break;

		case 'l':	/* hour, 12-hour clock, 1 - 12, blank pad */
			i = range(0, timeptr->tm_hour, 23);
			if (i == 0)
				i = 12;
			else if (i > 12)
				i -= 12;
			snprintf(tbuf,sizeof(tbuf), "%2d", i);
			break;
#endif

#ifdef HPUX_EXT
		case 'N':	/* Emperor/Era name */
#ifdef HAVE_NL_LANGINFO
			ya_strftime(tbuf, sizeof tbuf, nl_langinfo(ERA), timeptr);
#else
			/* this is essentially the same as the century */
			goto century;	/* %C */
#endif

		case 'o':	/* Emperor/Era year */
			goto year;	/* %y */
#endif /* HPUX_EXT */


#ifdef VMS_EXT
		case 'v':	/* date as dd-bbb-YYYY */
			snprintf(tbuf,sizeof(tbuf), "%2d-%3.3s-%4ld",
				range(1, timeptr->tm_mday, 31),
				months_a(range(0, timeptr->tm_mon, 11)),
				timeptr->tm_year + 1900L);
			for (i = 3; i < 6; i++)
				if (islower(tbuf[i]))
					tbuf[i] = toupper(tbuf[i]);
			break;
#endif

		default:
			tbuf[0] = '%';
			tbuf[1] = *format;
			tbuf[2] = '\0';
			break;
		}
		i = strlen(tbuf);
		if (i) {
			if (s + i < endp - 1) {
				strcpy(s, tbuf);
				s += i;
			} else
				return 0;
		}
	}
out:
	if (s < endp && *format == '\0') {
		*s = '\0';
		return (s - start);
	} else
		return 0;
}

/* iso8601wknum --- compute week number according to ISO 8601 */

static int
iso8601wknum(const struct tm *timeptr)
{
	/*
	 * From 1003.2:
	 *	If the week (Monday to Sunday) containing January 1
	 *	has four or more days in the new year, then it is week 1;
	 *	otherwise it is the highest numbered week of the previous
	 *	year (52 or 53), and the next week is week 1.
	 *
	 * ADR: This means if Jan 1 was Monday through Thursday,
	 *	it was week 1, otherwise week 52 or 53.
	 *
	 * XPG4 erroneously included POSIX.2 rationale text in the
	 * main body of the standard. Thus it requires week 53.
	 */

	int weeknum, jan1day;

	/* get week number, Monday as first day of the week */
	weeknum = weeknumber(timeptr, 1);

	/*
	 * With thanks and tip of the hatlo to tml@tik.vtt.fi
	 *
	 * What day of the week does January 1 fall on?
	 * We know that
	 *	(timeptr->tm_yday - jan1.tm_yday) MOD 7 ==
	 *		(timeptr->tm_wday - jan1.tm_wday) MOD 7
	 * and that
	 * 	jan1.tm_yday == 0
	 * and that
	 * 	timeptr->tm_wday MOD 7 == timeptr->tm_wday
	 * from which it follows that. . .
 	 */
	jan1day = timeptr->tm_wday - (timeptr->tm_yday % 7);
	if (jan1day < 0)
		jan1day += 7;

	/*
	 * If Jan 1 was a Monday through Thursday, it was in
	 * week 1.  Otherwise it was last year's highest week, which is
	 * this year's week 0.
	 *
	 * What does that mean?
	 * If Jan 1 was Monday, the week number is exactly right, it can
	 *	never be 0.
	 * If it was Tuesday through Thursday, the weeknumber is one
	 *	less than it should be, so we add one.
	 * Otherwise, Friday, Saturday or Sunday, the week number is
	 * OK, but if it is 0, it needs to be 52 or 53.
	 */
	switch (jan1day) {
	case 1:		/* Monday */
		break;
	case 2:		/* Tuesday */
	case 3:		/* Wednesday */
	case 4:		/* Thursday */
		weeknum++;
		break;
	case 5:		/* Friday */
	case 6:		/* Saturday */
	case 0:		/* Sunday */
		if (weeknum == 0) {
#ifdef USE_BROKEN_XPG4
			/* XPG4 (as of March 1994) says 53 unconditionally */
			weeknum = 53;
#else
			/* get week number of last week of last year */
			struct tm dec31ly;	/* 12/31 last year */
			dec31ly = *timeptr;
			dec31ly.tm_year--;
			dec31ly.tm_mon = 11;
			dec31ly.tm_mday = 31;
			dec31ly.tm_wday = (jan1day == 0) ? 6 : jan1day - 1;
			dec31ly.tm_yday = 364 + is_leap((int64_t)dec31ly.tm_year + 1900L);
			weeknum = iso8601wknum(& dec31ly);
#endif
		}
		break;
	}

	if (timeptr->tm_mon == 11) {
		/*
		 * The last week of the year
		 * can be in week 1 of next year.
		 * Sigh.
		 *
		 * This can only happen if
		 *	M   T  W
		 *	29  30 31
		 *	30  31
		 *	31
		 */
		int wday, mday;

		wday = timeptr->tm_wday;
		mday = timeptr->tm_mday;
		if (   (wday == 1 && (mday >= 29 && mday <= 31))
		    || (wday == 2 && (mday == 30 || mday == 31))
		    || (wday == 3 &&  mday == 31))
			weeknum = 1;
	}

	return weeknum;
}

/* weeknumber --- figure how many weeks into the year */

/* With thanks and tip of the hatlo to ado@elsie.nci.nih.gov */

static int
weeknumber(const struct tm *timeptr, int firstweekday)
{
	int wday = timeptr->tm_wday;
	int ret;

	if (firstweekday == 1) {
		if (wday == 0)	/* sunday */
			wday = 6;
		else
			wday--;
	}
	ret = ((timeptr->tm_yday + 7 - wday) / 7);
	if (ret < 0)
		ret = 0;
	return ret;
}

#if 0
/* ADR --- I'm loathe to mess with ado's code ... */

Date:         Wed, 24 Apr 91 20:54:08 MDT
From: Michal Jaegermann <audfax!emory!vm.ucs.UAlberta.CA!NTOMCZAK>
To: arnold@audiofax.com

Hi Arnold,
in a process of fixing of strftime() in libraries on Atari ST I grabbed
some pieces of code from your own strftime.  When doing that it came
to mind that your weeknumber() function compiles a little bit nicer
in the following form:
/*
 * firstweekday is 0 if starting in Sunday, non-zero if in Monday
 */
{
    return (timeptr->tm_yday - timeptr->tm_wday +
	    (firstweekday ? (timeptr->tm_wday ? 8 : 1) : 7)) / 7;
}
How nicer it depends on a compiler, of course, but always a tiny bit.

   Cheers,
   Michal
   ntomczak@vm.ucs.ualberta.ca
#endif



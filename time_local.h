/* time_local.h
   defines strftime() for situations (like on windows) where its not already defined
   must be included after <time.h> as that defines struct tm
   Note actually defines ya_strptime(), ya_strftime() and  ya_mktime() to avoid issues with trying to redefine strptime(), strftime() and mktime()
*/
#ifndef __TIME_LOCAL_H
 #define __TIME_LOCAL_H
 #ifdef __cplusplus
  extern "C" {
 #endif
	#ifndef __WIN64
	 /* if building for 32 bits then we need to force time_t to 64 bits as its likely to be 32 bits by default */
	 #define time_t int64_t /* the code assumes this is signed 64 bits */
						  /* this is possible as we use ya_mktime() to avoid conflicts with the mktime() in the standard compiler library */
	#endif
    #ifdef __linux
	 /* need strnicmp() as its not in standard libraries */
     int strnicmp( const char * s1, const char * s2, size_t n );
    #endif 
	
	char * ya_strptime(const char *s, const char *format, struct tm *tm);// in strptime.c 
	size_t ya_strftime(char *s, size_t maxsize, const char *format, const struct tm *timeptr); // in strftime.c
	
	struct strp_tz_struct
		{
		 int initialised; /* set to 1 when structure is 1st initialised on a call to strptime() or strftime() */
		 // The next 2 variables define a timezone in a portable way
		 char tz_name[4]; /* alpha name of time zone [ all zero's if unknown] */
		 int tz_off_mins; /* time zone offset from UTC in minutes [ strp_tz_default for unknown ]*/
		 // variables below are passed between strftime() and strptime() they are not designed for general use
		 int week_nos_U; /* week number 0->53 with Sunday as the 1st day of the week [ strp_tz_default means unknown] */
		 int week_nos_V; /* iso 8601 week number [ strp_tz_default means unknown ] */
		 int week_nos_W; /* week number 0->53 with Monday as the 1st day of the week  [ strp_tz_default means unknown ] */
		 int year_G; /* week based year as a decimal number (no offset) (iso 8601) [ strp_tz_default for unknown] */
		 double f_secs;/* fractional seconds [ portion after decimal point ] from %f , default 0 */
		 int f_secs_p10;/* number of digits in f_secs (after decimal point), Used to allow %f to be round loop exact. default strp_tz_default */
		};
	 #define strp_tz_default (-INT_MAX) /* default value for all apart from tz_name & initialised*/	

	extern struct strp_tz_struct strp_tz;// strp_tz is a global thats sets by strptime() and used by strftime()
	void init_strp_tz(struct strp_tz_struct *d); /* initialise a strp_tz_struct (mainly to strp_tz_default)  */	
	time_t ya_mktime(struct tm *tp); /* fully functional version of mktime() that returns secs and takes (and changes if necessary) timeptr */
    void sec_to_tm(time_t t,struct tm *tp); // reverse of ya_mktime, converts secs since epoch to the numbers of tp
	time_t UTC_mktime(struct tm *tp,struct strp_tz_struct *tz ); /* version of mktime() that also uses tz to adjust secs returned for timezones. Returns UTC secs since epoch (time_t) */
	void UTC_sec_to_tm(time_t t,struct tm *tp,struct strp_tz_struct *tz ); // reverse of UTC_mktime(), converts UTC time as secs since epoch to the numbers of tp, taking into account tz to adjust secs  for timezones
	/* year as int64_t below to avoid overflow issues when converting int years with an offset to one with no offset */
	int day_of_week(int64_t year,int month, int mday); /* returns day of week(0-6), 0=sunday given year (with no offset eg 1970), month (0-11, 0=jan) and day of month (1-31) */
	void month_day(int64_t year, int yearday, int *pmonth, int *pday);// year with no offset and days in year (0->), sets pmonth (0->11) and pday(1-31)
	bool is_leap(int64_t year); /* returns true if year [with no offset] is a leap year */
	bool check_tm(struct tm *tm);/* returns true only if all elements of tm are valid */
	extern const char * strp_weekdays[]; // strings - names of weekdays (Monday,...)
	extern const char * strp_monthnames[] ;// strings - names of Months (January,...)
 #ifdef __cplusplus
    }
 #endif
#endif
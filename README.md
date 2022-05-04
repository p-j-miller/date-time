# date-time
Code for the C functions strftime() and strptime() that deal with the conversion of dates and times on input &amp; output for Windows and Linux

It includes a reasonably comprehensive test program (main.c)
This file compiles and runs correctly with devC++/TDMgcc 10.3.0 on Windows, Builder C++ (10.2) on Windows and gcc 9.4.0 and 10.3.0 on Ubuntu Linux.

For gcc under linux compile test program with :
~~~
  gcc -Wall -O3 -o date-time main.c strftime.c strptime.c 
  ./date-time
~~~  
For dev-C++ (tested with tdmgcc 10.3.0) there is a *.dev file, and the project files (*.cbproj) for Builder C++ are also present. 

You should see no errors or warnings when compiling these files.

In all cases when running the executable you should see lots of output with the last line reading:

7300275 tests conducted, no errors found
# Functionality
Functions defined (#include "time_local.h"):
~~~
	char * strptime(const char *s, const char *format, struct tm *tm);// in strptime.c 
	size_t strftime(char *s, size_t maxsize, const char *format, const struct tm *timeptr); // in strftime.c
  
	time_t ya_mktime(struct tm *tp); /* fully functional version of mktime() that returns secs and takes (and changes if necessary) tp */
        void sec_to_tm(time_t t,struct tm *tp); // reverse of ya_mktime, converts secs since epoch to the numbers of tp
	time_t UTC_mktime(struct tm *tp,struct strp_tz_struct *tz ); /* version of mktime() that also uses tz to adjust secs returned for timezones. Returns UTC secs since epoch (time_t) */
	void UTC_sec_to_tm(time_t t,struct tm *tp,struct strp_tz_struct *tz ); // reverse of UTC_mktime(), converts UTC time as secs since epoch to the numbers of tp, taking into account tz to adjust secs  for timezones
	/* year as int64_t below to avoid overflow issues when converting int years with an offset to one with no offset */
	int day_of_week(int64_t year,int month, int mday); /* returns day of week(0-6), 0=sunday given year (with no offset eg 1970), month (0-11, 0=jan) and day of month (1-31) */
	void month_day(int64_t year, int yearday, int *pmonth, int *pday);// year with no offset and days in year (0->), sets pmonth (0->11) and pday(1-31)
	bool is_leap(int64_t year); /* returns true if year [with no offset] is a leap year */
	extern const char * strp_weekdays[]; // strings - names of weekdays (Monday,...)
	extern const char * strp_monthnames[] ;// strings - names of Months (January,...)  
~~~
Note that time_local.h defines time_t as int64_t (many 32 bit compilers define time_t as 32 bits by default) - this is done so that the same results and limits exist when compiled for 32 bits as when compiled for 64 bits.

These functions give the full C99 strftime()/strptime() functionality (in the C locale) and a large subset of the Linux/BSD/POSIX strptime functionality.
They do not support multibyte character sequences in the format string.

It assumes the C locale (so effectively has no locale support, the E and O modifiers are just ignored as per the C99 standard).

The return value of strptime() is a pointer to the first character not processed by this function call.

When the input string contains more characters than required by the format string the return value points right after the last consumed input character.

When the whole input string is consumed the return value points to the null byte at the end of the string.

If strptime() fails to match all of the format string and therefore an error occurred the function returns NULL. 

To avoid unwanted/unexpected side effects strptime() and strftime() use struct strp_tz_struct strp_tz to store items that are not in struct tm.

This approach also allows us to be "round trip exact" in more situations.

Every time strptime() is called strp_tz is re-initialised to its default values by automatically calling init_strp_tz(). 

init_strp_tz() is also called by strftime() if strp_tz has not already been initialised (ie if strftime() is called before calling strptime() ).

Values set in strp_tz override the operating system supplied defaults.

Note that these routines assume the Gregorian calendar which was adopted by different countries on different dates, it has been used in the UK since 1752 ,but in most other places since 15th Oct 1582 

Also note that years are signed integers and the year 0 is used (strictly the year after 1BC was 1AD ) - but as the Gregorian calendar was not used then thats not seen as a bug.

The limits in years are +MAX_INT+1900 (2,147,485,547) to -MAX_INT+1900 (-2,147,481,747) which are basically the limits of a 32 bit integer year with an offset of 1900 (ie tm_year can be between MAX_INT and -MAX_INT)

These limits are enforced if you use strptime(%s) to read a date/time or if you call ya_mktime().

If YEAR0LEAP is defined (see near the start of strftime.c) then year 0 is considered a leap year, otherwise it is not.

~~~
Conversion specifiers follow a % sign, those defined are:
a The day of the week (Monday, Tuesday,...) ; either the abbreviated (3 character) or full name may be specified. Both upper and lower case is allowed.
A Equivalent to %a. [ Note for strftime, %A gives the full day of the week while %a gives the abbreviated version ]
b The month (January, ...) ; either the abbreviated (3 characters) or full name may be specified. Both upper and lower case is allowed.
B Equivalent to %b.[ Note for strftime, %B gives the full day of the month while %b gives the abbreviated version ]
c  date and time  [ c99 says this should be %a %b %e %T %Y ]
C All but the last two digits of the year {2}; leading zeros shall be permitted but shall not be required. 
  Normally used before %y, but can also be used before %g. This means a %C also sets the ISO 8601 century 
d The day of the month [01,31]; leading zeros shall be permitted but shall not be required.
D The date as %m/%d/%y.
e The day of month (1-31), leading space if only 1 digit
f fraction of a second (the values after the decimal point). The decimal point is implied (so needs to be in the format string if its actually present)  
	Eg "%H:%M:%S.%f" will read 12:59:59.12345 
	The fractional seconds are stored as a double f_secs and an integer number of input digits in f_secs_p10 element of struct strp_tz.
	This approach is done to allow exact "round the loop" input by strptime() and then output by strftime.
	 15 significant figures for %f is the limit for a double (beyond this not round loop exact, but the double f_secs should still be valid).
	Normally only the f_secs bit is needed to extract the fractional seconds. 
  This is a local extension it is not defined by C99.	 
F Equivalent to %Y-%m-%d (the iso 8601 date format)
%G* The ISO 8601 week-based year with century as a decimal number.
  The 4-digit year corresponds to the ISO week number (see also %V etc). 
  This has the same format and value as %Y, except that if the ISO week number belongs to the previous or next year, that year is used instead.
  * %G (or %C%g or just %g) should only be used with %V and the day of the week (%a,%A,%u,%w) if these are all present then all the values in struct tm (except tm_isdst) are filled in.
    If not all of these values are input, those entered are remembered (in strp_tz) and wil be displayed as entered by strftime() if requested rather than calculated from other values as is normally done.
    If a value has not be set my the immediatly previous strptime() then strftime() will calculate the value from the date.
    See also https://en.wikipedia.org/wiki/ISO_week_date and https://webspace.science.uu.nl/~gent0113/calendar/isocalendar.htm
    
%g* Replaced by the same year as in %G, but as a decimal number without century (00-99).
h Equivalent to %b.
H The hour (24-hour clock) [00,23]; leading zeros shall be permitted but shall not be required.
I The hour (12-hour clock) [01,12]; leading zeros shall be permitted but shall not be required.
j The day number of the year [001,366]; leading zeros shall be permitted but shall not be required.
m The month number [01,12]; leading zeros shall be permitted but shall not be required.
M The minute [00,59]; leading zeros shall be permitted but shall not be required.
n Any white space. strftime() will output a newline (\n) character.
p "am" or "pm"
r 12-hour clock time using the AM/PM notation; equivalent to %I:%M:%S %p
R The time as %H:%M.
s seconds since the epoch as a multidigit signed integer. This is not defined by C99 or POSIX, but is a reasonably common extension.
  strptime() calls sec_to_tm() to set the other contents of tm from the number of seconds supplied (everything except tz is set)
  strftime() calls ya_mktime_tm() to get the number of seconds from the epoch from the other elements of tm.
  Limits in years are +MAX_INT+1900 (2,147,485,547) to -MAX_INT+1900 (-2,147,481,747)
  Note leap seconds are ignored when calculating a date/time from a given number of seconds (mainly as the dates when future leap seconds will be added cannot be predicted).
S The seconds [00,60]; leading zeros shall be permitted but shall not be required.
t Any white space. strftime() will output a tab character.
T The time as %H:%M:%S.
u Weekday as a number 1->7 where Monday=1 and Sunday=7
  strftime() never calculates the weekday from the other elements of tm (unless you use the ISO 8601 week-based conversion specifiers - see * above). 
  If you need this calculation to be done call ya_mktime(&tm) before calling strftime().
U The week number of the year (Sunday as the first day of the week) as a decimal number [00,53]; leading zeros shall be permitted but shall not be required.
  If %Y or %C%y or %y together with %U and the day of the week (%a,%A,%u,%w) are all read by the format then all the values in struct tm (except tm_isdst) are filled in.
  Otherwise The value input by %U is remembered and will be output by strftime(). If a value has not be set my the immediatly previous strptime() then strftime() will calculate the value from the date.
%V* Replaced by the ISO 8601 week number of the year (Monday as the first day of the week) as a decimal number (01-53). 
  If the week containing January 1 has four or more days in the new year, then it is week 1; otherwise it is the last week of the previous year, and the next week is week 1.
  See %G above for more information.
w The weekday as a decimal number [0,6], with 0 representing Sunday.
  strftime() never calculates the weekday from the other elements of tm (unless you use the ISO 8601 week-based conversion specifiers - see * above).
  If you need this calculation to be done call ya_mktime(&tm) before calling strftime().
W The week number of the year (Monday as the first day of the week) as a decimal number [00,53]; leading zeros shall be permitted but shall not be required.
  If %Y or %C%y or %y together with %U and the day of the week (%a,%A,%u,%w) are all read by the format then all the values in struct tm (except tm_isdst) are filled in.
  Otherwise The value input by %U is remembered and will be output by strftime(). If a value has not be set my the immediatly previous strptime() then strftime() will calculate the value from the date.
x The date, the same as %D.
X The time, the same as %T
y The last two digits of the year. 
  When format contains neither a C conversion specifier nor a Y conversion specifier,
   values in the range [69,99] shall refer to years 1969 to 1999 inclusive and values in the range [00,68] shall refer to years 2000 to 2068 inclusive;
   leading zeros shall be permitted but shall not be required.

Y The full year If POSIX_2008 defined then {4} [0,9999]; leading zeros shall be permitted but shall not be required. 
  If POSIX_2008 is not defined then allow an optional sign followed by a number of digits (which can be more than 4 - limits are +391220960 , -39171945 which matches %s ).
z Time zone offset from UTC; a leading plus sign stands for east of UTC, a minus sign or west of UTC,
  hours and minutes follow with two digits each and no delimiter between them (as in ISO8601 & common form for RFC 822 date headers). eg “-0500” or "+0000". The sign is always required.
  The value input by %z is remembered and will be output by strftime(). If a value has not be set my the immediatly previous strptime() then strftime() will use the value supplied by the OS.
Z  time zone name. eg “EDT”, "UTC", "GMT","AKST","ET" etc.  2, 3 or 4 letters is required. See https://www.nist.gov/pml/time-and-frequency-division/local-time-faqs#zones for the names used in the USA
   The value input by %Z is remembered and will be output by strftime(). If a value has not be set my the immediatly previous strptime() then strftime() will use the value supplied by the OS.
% Replaced by %.
~~~
The file main.c gives lots of examples.

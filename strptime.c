/* 	strptime.c
	========== 
	Note - to avoid issues (especially with C++) the function is called ya_strptime().

   This file was created by starting from https://stackoverflow.com/questions/667250/strptime-in-windows on 23/3/2022 
   A significant number of changes mave been made, initially to make it compile for Windows with C99 (it was originally in C++) using TDM-GCC 10.3.0,
   but then to significantly expand the functionality and remove bugs.
   This file now compiles and runs correctly with devC++/TDMgcc 10.3.0 on Windows, Builder C++ (10.2) on Windows and gcc on Linux.
   
   See below for a full description of the functionality offered, but it provides full C99 strftime functionality (in the C locale).
   strftime.c offers identical functionality for date/time output, and is configured to be "round the loop" exact when used with this strptime() for input.
   
   An extensive test program is also provided (main.c).
	
   Thanks to K. Shepherd who supplied the original stackoverflow answer and  jjv360, Arno Duvenhage, ryyker &  Gebi Miguel who commented on it.
  
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
 
/*
This gives the full C99 strftime functionality (in the C locale) and a large subset of the Linux/BSD/POSIX strptime functionality.
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

The range of these routines is 
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
  If POSIX_2008 is not defined then allow an optional sign followed by a number of digits 
  (which can be more than 4 - limits are +MAX_INT+1900 (2,147,485,547) to -MAX_INT+1900 (-2,147,481,747) which matches %s ).
z Time zone offset from UTC; a leading plus sign stands for east of UTC, a minus sign or west of UTC,
  hours and minutes follow with two digits each and no delimiter between them (as in ISO8601 & common form for RFC 822 date headers). eg “-0500” or "+0000". The sign is always required.
  The value input by %z is remembered and will be output by strftime(). If a value has not be set my the immediatly previous strptime() then strftime() will use the value supplied by the OS.
Z  time zone name. eg “EDT”, "UTC", "GMT","AKST","ET" etc.  2, 3 or 4 letters is required. See https://www.nist.gov/pml/time-and-frequency-division/local-time-faqs#zones for the names used in the USA
   The value input by %Z is remembered and will be output by strftime(). If a value has not be set my the immediatly previous strptime() then strftime() will use the value supplied by the OS.
% Replaced by %.

*/
/* Missing vs C99 standard S7.23.3.5 :
It assumes the C locale (so effectively has no locale support, the E and O modifiers are just ignored as per the C99 standard).
It does not support a multibyte character sequence for the format string. 
*/
#include <stdio.h>
#include <stdlib.h> /* for strtoul() etc */
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
// #include <math.h>
#include <limits.h>


// #define POSIX_2008 /* if defined then %Y is limited to 4 digits as per POSIX-2008, otherwise an optional sign and more digits are allowed */

#include "time_local.h"


static double const dblpowersOf10[] = /* powers of 10 as doubles */

                {
                    1e0,   1e1,   1e2,   1e3,   1e4,   1e5,   1e6,   1e7,   1e8,    1e9,
                    1e10,  1e11,  1e12,  1e13,  1e14,  1e15,  1e16,  1e17,  1e18,  1e19,
                    1e20,  1e21,  1e22,  1e23,  1e24,  1e25,  1e26,  1e27,  1e28,  1e29,
                    1e30,  1e31,  1e32,  1e33,  1e34,  1e35,  1e36,  1e37,  1e38,  1e39,
                    1e40,  1e41,  1e42,  1e43,  1e44,  1e45,  1e46,  1e47,  1e48,  1e49,
                    1e50,  1e51,  1e52,  1e53,  1e54,  1e55,  1e56,  1e57,  1e58,  1e59,
                    1e60,  1e61,  1e62,  1e63,  1e64,  1e65,  1e66,  1e67,  1e68,  1e69,
                    1e70,  1e71,  1e72,  1e73,  1e74,  1e75,  1e76,  1e77,  1e78,  1e79,
                    1e80,  1e81,  1e82,  1e83,  1e84,  1e85,  1e86,  1e87,  1e88,  1e89,
                    1e90,  1e91,  1e92,  1e93,  1e94,  1e95,  1e96,  1e97,  1e98,  1e99,
                    1e100, 1e101, 1e102, 1e103, 1e104, 1e105, 1e106, 1e107, 1e108, 1e109,
                    1e110, 1e111, 1e112, 1e113, 1e114, 1e115, 1e116, 1e117, 1e118, 1e119,
                    1e120, 1e121, 1e122, 1e123, 1e124, 1e125, 1e126, 1e127, 1e128, 1e129,
                    1e130, 1e131, 1e132, 1e133, 1e134, 1e135, 1e136, 1e137, 1e138, 1e139,
                    1e140, 1e141, 1e142, 1e143, 1e144, 1e145, 1e146, 1e147, 1e148, 1e149,
                    1e150, 1e151, 1e152, 1e153, 1e154, 1e155, 1e156, 1e157, 1e158, 1e159,
                    1e160, 1e161, 1e162, 1e163, 1e164, 1e165, 1e166, 1e167, 1e168, 1e169,
                    1e170, 1e171, 1e172, 1e173, 1e174, 1e175, 1e176, 1e177, 1e178, 1e179,
                    1e180, 1e181, 1e182, 1e183, 1e184, 1e185, 1e186, 1e187, 1e188, 1e189,
                    1e190, 1e191, 1e192, 1e193, 1e194, 1e195, 1e196, 1e197, 1e198, 1e199,
                    1e200, 1e201, 1e202, 1e203, 1e204, 1e205, 1e206, 1e207, 1e208, 1e209,
                    1e210, 1e211, 1e212, 1e213, 1e214, 1e215, 1e216, 1e217, 1e218, 1e219,
                    1e220, 1e221, 1e222, 1e223, 1e224, 1e225, 1e226, 1e227, 1e228, 1e229,
                    1e230, 1e231, 1e232, 1e233, 1e234, 1e235, 1e236, 1e237, 1e238, 1e239,
                    1e240, 1e241, 1e242, 1e243, 1e244, 1e245, 1e246, 1e247, 1e248, 1e249,
                    1e250, 1e251, 1e252, 1e253, 1e254, 1e255, 1e256, 1e257, 1e258, 1e259,
                    1e260, 1e261, 1e262, 1e263, 1e264, 1e265, 1e266, 1e267, 1e268, 1e269,
                    1e270, 1e271, 1e272, 1e273, 1e274, 1e275, 1e276, 1e277, 1e278, 1e279,
                    1e280, 1e281, 1e282, 1e283, 1e284, 1e285, 1e286, 1e287, 1e288, 1e289,
                    1e290, 1e291, 1e292, 1e293, 1e294, 1e295, 1e296, 1e297, 1e298, 1e299,
                    1e300, 1e301, 1e302, 1e303, 1e304, 1e305, 1e306, 1e307, 1e308
                };
                
static double ipow10(unsigned int i) /* 10.0^i where i>=0 */ 
	{
	 if(i>308) return 1e308; // largest possible - in the code here this will never happen
	 return dblpowersOf10[i];
	}
	
const char * strp_weekdays[] = 
    { "sunday", "monday", "tuesday", "wednesday", "thursday", "friday", "saturday"};
    
const char * strp_monthnames[] = 
    { "january", "february", "march", "april", "may", "june", "july", "august", "september", "october", "november", "december"};

struct strp_tz_struct strp_tz;/* extra variables not in struct tm - initialised on a call to strptime() or strftime() */

void init_strp_tz(struct strp_tz_struct *d) /* initialise d to special values so we can detect when a field has a value written to it */	
{
 d->tz_name[0]=0;
 d->tz_name[1]=0;
 d->tz_name[2]=0;
 d->tz_name[3]=0;
 d->tz_off_mins= strp_tz_default;
 d->week_nos_U= strp_tz_default;
 d->week_nos_V= strp_tz_default;
 d->week_nos_W= strp_tz_default; 
 d->year_G= strp_tz_default;
 d->f_secs=0;
 d->f_secs_p10=strp_tz_default;
 d->initialised=1; // now initialised
}

bool check_tm(struct tm *tm)
{ // check each field of tm. returns true if all OK, otherwise returns false
 bool OK=true;
 if(tm->tm_sec<0 || tm->tm_sec>60) OK=false;   // 60 for leap seconds
 if(tm->tm_min<0 || tm->tm_min>59) OK=false;
 if(tm->tm_hour<0 || tm->tm_hour>23) OK=false;
 if(tm->tm_mday<1 || tm->tm_mday>31) OK=false; // lower limit is strictly 1, but default value is 0
 if(tm->tm_mon<0 || tm->tm_mon>11) OK=false;
 // tm_year can have any integer value
 if(tm->tm_wday<0 || tm->tm_wday>6) OK=false;
 if(tm->tm_yday<0 || tm->tm_yday>365) OK=false;
 // tm_isdst can be any value
 return OK;
}
    
bool strp_atoi(const char **s, int *result, unsigned int low, unsigned int high, unsigned int offset)
    {
    /* this traps too long input (in terms of total digits including leading zeros) */   
    const char * end=*s;
	unsigned int num=0;
	if(!isdigit(*end)) return false; // not a number, if we get past here we do have a valid decimal number 
	num=(*end++ -'0'); // convert 1st digit to number (doing this outside of loop below avoids the need for a multiply by 10 of zero )
	// now convert remaining digits (if there are any), check 1st as entering the loop needs a divide 
	if(isdigit(*end))
		{for(unsigned int i=high/10;i>0; i/=10) // high defines number of digits allowed - where leading zero's count as a digit so eg high=6 allows 1 digit as (integer)  6/10=0
			{// know *end is a digit if we get here so no need to check again
			 num=(*end++ -'0')+num*10;
			 if(!isdigit(*end))  break; /* end of number (but there has been at least 1 valid decimal digit) */
			}
		}
    if (num >= low && num <= high)
        {
         *result = (int)(num + offset);
         *s = end;
         return true; // valid result
        }	
    return false; // invalid number found , don't change s        
    }

char * ya_strptime(const char *s, const char *format, struct tm *tm)
    {
    bool valid = true;
    bool per_C_found=false; // set when %C found (1st 2 digits year )
    bool per_G_found=false; // set when %G found (iso 8601 year)
    bool weekday_found=false; // set when %a,%A, %u or %w found
    bool per_U_found=false; // set when %U found
    bool per_V_found=false; // set when %V found
    bool per_W_found=false; // set when %W found
    init_strp_tz(&strp_tz); // always initialse strp_tz as we want to know what items are set by this call to strptime()
	if (s == NULL || format == NULL || tm == NULL )
		return NULL;    	
    while (valid && *format && *s)
        {
        switch (*format)
            {
        case '%': /* all special format designators start with a % */
            {
            ++format;
            if(*format=='E') ++format;// ignore E modifier (as in C locale)
            else if(*format=='O') ++ format; // also ignore O modifier (as in C locale)
            
            switch (*format)
                {
            case 'a':
            case 'A': /* The weekday name, in abbreviated form or the full name */
                valid = false;
                for (size_t i = 0; i < 7; ++ i)
                    {
                    size_t len = strlen(strp_weekdays[i]);
                    if (!strnicmp(strp_weekdays[i], s, len)) /* match to full name, strnicmp() does a case insensitive match. strncasecmp() is the equivalent POSIX function  */
                        {
                        tm->tm_wday = i;
                        s += len;
                        valid = true;
                        break;
                        }
                    else if (!strnicmp(strp_weekdays[i], s, 3)) /* first 3 characters of weekday is an allowable abbreviation */
                        {
                        tm->tm_wday = i;
                        s += 3;
                        valid = true;
                        break;
                        }
                    }
                if(valid) weekday_found=true;
                break;
            case 'b':
            case 'B':
            case 'h': /* The month name, in abbreviated form or the full name.  */
                valid = false;
                for (size_t i = 0; i < 12; ++ i)
                    {
                    size_t len = strlen(strp_monthnames[i]);
                    if (!strnicmp(strp_monthnames[i], s, len)) /* match to full name, strnicmp does a case insensitive match */
                        {
                        tm->tm_mon = i;
                        s += len;
                        valid = true;
                        break;
                        }
                    else if (!strnicmp(strp_monthnames[i], s, 3)) /* first 3 characters of weekday is an allowable abbreviation */
                        {
                        tm->tm_mon = i;
                        s += 3;
                        valid = true;
                        break;
                        }
                    }
                break;
            case 'c': /* date and time C99 in C locale defines this to be %a %b %e %T %Y */
            	{char *r=ya_strptime(s,"%a %b %e %T %Y", tm);
            	 valid=r!=NULL;
            	 if(valid) s=r;
            	}
            	break;
            case 'C': /*  %C found (1st 2 digits of year ) , normally used before %y but can also be used before %g */
            	{int C=0;
            	 valid = strp_atoi(&s, &(C), 0, 99, 0);
            	 if(valid)
            	 	{// put in new top 2 digits, leaving lower digits [ which by default will be zero ]
            	 	 if(tm->tm_year>=0) tm->tm_year%=100; // leave just 2 lower digits
            	 	 else tm->tm_year=100+(tm->tm_year % 100); // leave just 2 lower digits
            	 	 
            	 	 tm->tm_year+=C*100-1900; // add in upper digits [ 1900 is required ofset for tm_year ]
            	 	 per_C_found=true;
            	 	 // now repeat similar logic for strp_tz.year_G
            	 	 if(strp_tz.year_G==strp_tz_default)
            	 	 	{strp_tz.year_G=C*100;// just put century in
            	 	 	}
            	 	 else
					  	{ // already set, just replace upper 2 digits
					  	 if(strp_tz.year_G>=0) strp_tz.year_G%=100; // leave just 2 lower digits
            	 	     else strp_tz.year_G=100+(strp_tz.year_G % 100); // leave just 2 lower digits	
            	 	     strp_tz.year_G+=C*100;
						}
            	 	}
				}
				break;
            case 'd':  /* The day of month (01-31)  */
                valid = strp_atoi(&s, &(tm->tm_mday), 1, 31, 0);
                break;    
            case 'e': /* The day of month (1-31), leading space if only 1 digit  */
            	if(isspace(*s))
            		{++s;
            		 valid = strp_atoi(&s, &(tm->tm_mday), 1, 9, 0);
            		}
				else		
                	valid = strp_atoi(&s, &(tm->tm_mday), 1, 31, 0); // 2 digits (or 1 digit and space "gobbled" up on whitespace between fields)
                break;   				           
            case 'x': /* same as %D for now */
            case 'D': /* Equivalent to %m/%d/%y. (This is the American style date) */
            	{char *r=ya_strptime(s,"%m/%d/%y", tm);
            	 valid=r!=NULL;
            	 if(valid) s=r;
            	}               
                break;
            case 'f': /* fractional seconds (after decimal point) -> store to strp_tz.f_secs and number of digits after dp is stored in strp_tz.f_secs_p10. 
						 Will accept as many digits as are present, but double is limited to ~ 15 significant digits */
            	{
            	 if(isdigit(*s))
            	 	{uint64_t fsec=0;// the mantissa of a double is 53 bits, so 64 bits is plenty to use here [ means overflow detection can be quite simple]
            	 	 unsigned int power10=0; // count of digits after dp
					 valid=true;
		 			 while(isdigit(*s) && (fsec & UINT64_C(0xf000000000000000)) == 0   )
						{fsec=fsec*10+(uint64_t)(*s++ -'0');// note leading zeros just change power10, they do not change fsec
			 			 power10++; // keep track of decimal point position
						}
		 			 if(isdigit(*s) && *s>='5') fsec++; // round if next digit present
		 			 while(isdigit(*s)) ++s; // eat up any more digits that are present (ignore them)
		 			 strp_tz.f_secs_p10=power10;// number of digits entered, needed to allow "round loop exact" output (this is limited by the resolution of a double, but that should be OK here)
		 			 strp_tz.f_secs=(double)fsec/ipow10(power10); 	 			 
					}
				 else valid=false;
            	}
            	break;
            case 'F': /* %F Equivalent to %Y-%m-%d (the iso 8601 date format) */
            	{char *r=ya_strptime(s,"%Y-%m-%d", tm);
            	 valid=r!=NULL;
            	 if(valid) s=r;
            	}               
                break;  
			case 'g': /* The ISO 8601 week-based year (see * in description at the top of this file) within century as a 2 digit decimal number. Here we treat almost identically to %y   
					     When a century is not otherwise specified, values in the range 69-99 refer to years in the twentieth century (1969-1999);
						 values in the range 00-68 refer to years in the twenty-first century (2000-2068). 
						 if [%C]%g and %G are both present the last will be used. If %C %G %g appear in that order the result may not be what is expected! 
            		 */
            	  {int y;
                   valid = strp_atoi(&s,&(y), 0, 99, 0);
                   //int v=y;// save for printf below
                   if(valid)
                   	{
                     if (per_G_found || per_C_found)
                     	{int C=strp_tz.year_G;// extract century from current year (no ofset )
                     	 C/=100;
                     	 C*=100; // above two operations zero out the last 2 digits of any existing date
                		 y+=C; // add in new century part of year to the 2 digits in y
                		}
                   	 else if (y < 69) // note tm_year=0 => 1900
                    	y += 2000;// 2000-2068
                     else y+=1900;// 1969-1999	
                     //printf("\n%%y integer found is %d, tm_year was %d now %d",v,strp_tz.year_G,y);
                     strp_tz.year_G=y;	// store result back into strp_tz structure                     
                	}
            	  }	
                 break;		                
            case 'G': /* The ISO 8601 week-based year (see * in description at the top of this file) with century as a 4 digit decimal number.  */
            	{int t;
                 valid = strp_atoi(&s,&t, 0, 9999, 0);
                 if(valid) strp_tz.year_G=t;
                 per_G_found=valid;// set flag to say we have a century already
             	}	
                break;  				               
            case 'H': /* The hour (0-23) */
                valid = strp_atoi(&s,&(tm->tm_hour), 0, 23, 0);
                break;
            case 'I': /* The hour on a 12-hour clock (1-12) */
                valid = strp_atoi(&s,&(tm->tm_hour), 1, 12, 0);
                break;
            case 'j': /* The day number in the year (1-366) */
                valid = strp_atoi(&s,&(tm->tm_yday), 1, 366, -1);
                break;
            case 'm': /* The month number (1-12) */
                valid = strp_atoi(&s,&(tm->tm_mon), 1, 12, -1);
                break;
            case 'M': /* The minute (0-59) */
                valid = strp_atoi(&s,&(tm->tm_min), 0, 59, 0);
                break;
            case 'n': // arbitrary whitespace
            case 't':
                while (isspace((int)*s)) 
                    ++s;
                break;
            case 'p': // am / pm
                if (!strnicmp(s, "am", 2))
                    { // the hour will be 1 -> 12 maps to 12 am, 1 am .. 11 am, 12 noon 12 pm .. 11 pm
                    if (tm->tm_hour == 12) // 12 am == 00 hours
                        tm->tm_hour = 0;
                    s += 2;
                    }
                else if (!strnicmp(s, "pm", 2))
                    {
                    if (tm->tm_hour < 12) // 12 pm == 12 hours
                        tm->tm_hour += 12; // 1 pm -> 13 hours, 11 pm -> 23 hours
                    s += 2;
                    }
                else
                    valid = false;
                break;
            case 'r': // 12 hour clock %I:%M:%S %p
            	{char *r=ya_strptime(s,"%I:%M:%S %p", tm);
            	 valid=r!=NULL;
            	 if(valid) s=r;
            	}      
                break;
            case 'R': // %H:%M
            	{char *r=ya_strptime(s,"%H:%M", tm);
            	 valid=r!=NULL;
            	 if(valid) s=r;
            	}            
                break;
            case 's': /* seconds since the epoch [1900] as an signed integer (possibly with leading zeros).  */
            	{bool neg=false;
            	 if(*s=='-')
            	 	{neg=true;
            	 	 ++s;
            	 	}
            	 else if(*s=='+') ++s;
				 valid=isdigit(*s); /* number must start with a digit (but can be any length) */
            	 if(valid)
            	 	{
            	 	 time_t t=(*s++)-'0';/* process 1st digit */
            	 	 while(isdigit(*s))
            	 		{/* we have another digit of the number */
            	 		 if(t*10+(*s-'0')<t) valid=false; // overflow - we don't know the type of time_t so this test should always work
            	 	 	 t=t*10+(*s++-'0');
						}
					 /* new limits on t based on moving to int64_t for year in some functions in strftime() */
					 /* -67,678,052,500,542,456 is calculated value at -MAX_INT+1900 years & 67,768,036,162,659,144 = MAX_INT+1900 years converted to secs using algorithm in ya_mktime_s() */
					 if(t>INT64_C(67768036162659144)) valid=false;
					 else if(neg && t>INT64_C(67678052500542456)) valid=false;				 
            	 	 if(neg) t= -t;
            	 	 // printf("\n%%s: t=%lld\n",t);	
					 //void sec_to_tm(time_t t,struct tm *tp) // reverse of ya_mktime_tm, converts secs since epoch to the numbers of tp
					 if(valid) sec_to_tm(t,tm);				   
					}
            	}
            	break;
            case 'S': /* The second (0-60; 60 may occur for leap seconds). */
                valid = strp_atoi(&s,&(tm->tm_sec), 0, 60, 0);
                break;
             
            case 'U' : /* %U The week number with Sunday the first day of the week (0-53). The first Sunday of January is the first day of week 1.    */
            	{int wk_nos;// this value is not in the tm structure, so set it in strp_tz.week_nos_U
                 valid = strp_atoi(&s,&(wk_nos), 0, 53, 0);
                 if(valid) 
				 	{strp_tz.week_nos_U=wk_nos;
				 	 per_U_found=true; // set when %U found
				 	}
            	}
                break;    
            case 'u': // weekday number 1->7 where Monday=1 needs to be converted for tm to 0->6 sunday->saturday. Sunday =0.
                valid = strp_atoi(&s,&(tm->tm_wday), 1, 7, 0);
                if(tm->tm_wday==7) tm->tm_wday=0;// fix sunday from 7 to 0
                if(valid) weekday_found=true;
                break;                
            case 'V' : /* %V The week number in week-based year as defined by the ISO 8601 standard   */
            	{int wk_nos;// this value is not in the tm structure, so we have to put it into strp_tz.week_nos_V
                 valid = strp_atoi(&s,&(wk_nos), 1, 53, 0);
                 if(valid) 
				 	{strp_tz.week_nos_V=wk_nos;
				 	 per_V_found=true; // set when %V found
				 	}
            	}
                break; 	
            case 'W' : /* %W The week number with Monday the first day of the week (0-53). The first Monday of January is the first day of week 1.  */    
            	{int wk_nos;// this value is not in the tm structure, so set it in strp_tz.week_nos_U
                 valid = strp_atoi(&s,&(wk_nos), 0, 53, 0);
                 if(valid)
				 	{strp_tz.week_nos_W=wk_nos;
				 	 per_W_found=true; // set when %W found
				 	}
            	}
                break; 							        
            case 'X' : /* same as T */                
            case 'T': // %H:%M:%S
            	{char *r=ya_strptime(s,"%H:%M:%S", tm);
            	 valid=r!=NULL;
            	 if(valid) s=r;
            	}      
                break;
            case 'w': // weekday number 0->6 sunday->saturday. Sunday =0.
                valid = strp_atoi(&s,&(tm->tm_wday), 0, 6, 0);
                if(valid) weekday_found=true;
                break;
          
            case 'Y': /* The year, including century (for example, 1991) - POSIX limits the year to 4 digits */
                {
#ifdef POSIX_2008                
                 valid = strp_atoi(&s,&(tm->tm_year), 0, 9999, -1900);// max 4 digits
#else			/* read in an integer with an optional sign */
				 bool neg=false;
            	 if(*s=='-')
            	 	{neg=true;
            	 	 ++s;
            	 	}
            	 else if(*s=='+') ++s;
				 valid=isdigit(*s); /* number must start with a digit (but can be any length) */
            	 if(valid)
            	 	{
            	 	 int y=(*s++)-'0';/* process 1st digit */
            	 	 while(isdigit(*s))
            	 		{/* we have another digit of the number */
            	 		 if(y*10+(*s-'0')<y) valid=false; // overflow - we don't know the type of time_t so this test should always work
            	 	 	 y=y*10+(*s++-'0');
            	 		}
            	 	 if(y>391220960) valid=false;	// Apply same limits as we have for %s (+391220960 , -39171945)
            	 	 else if(neg && y>39171945) valid=false; 
            	 	 if(neg) y= -y;	
            	 	 // printf("\n%%s: y=%d\n",y);	
					 if(valid) tm->tm_year=y-1900;				   
					}
#endif                
                 per_C_found=valid;// set flag to say we have a century already
             	}	
                break;
	             
            case 'y': /* The year within century (0-99). 
					     When a century is not otherwise specified, values in the range 69-99 refer to years in the twentieth century (1969-1999);
						 values in the range 00-68 refer to years in the twenty-first century (2000-2068). 
						 if [%C]%y and %Y are both present the last will be used. If %C %Y %y appear in that order the result may not be what is expected! 
            		 */
            	  {int y;
                   valid = strp_atoi(&s,&(y), 0, 99, 0);
                   //int v=y;// save for printf below
                   if(valid)
                   	{
                     if (per_C_found)
                     	{int C=tm->tm_year+1900;// extract century from current year (ofset is 1900)
                     	 C/=100;
                     	 C*=100; // above two operations zero out the last 2 digits of any existing date
                		 y+=C-1900; // add in new century part of year to the 2 digits in y
                		}
                   	 else if (y < 69) // note tm_year=0 => 1900
                    	y += 100;
                     //printf("\n%%y integer found is %d, tm_year was %d now %d",v,tm->tm_year,y);
                     tm->tm_year=y;	// store result back into tm structure                     
                	}
            	  }	
                 break;
			case 'z': // %z Time zone offset from UTC; a leading plus sign stands for east of UTC, a minus sign or west of UTC, followed by 4 digits eg “-0500” .
#if 1			/* set strp_tz.tz_off_mins */
				 {bool negative=false;
				 if(*s=='-') 
				 	{negative=true;
				 	 ++s;
				 	}
				 else if(*s=='+') ++s; // skip sign
				 valid=isdigit(*s); /* number must start with a digit (but can be any length) */
            	 if(valid)
            	 	{int nos_digits=1;
            	 	 int t=(*s++)-'0';/* process 1st digit */
            	 	 while(isdigit(*s))
            	 		{/* we have another digit of the number */
            	 	 	 t=t*10+(*s++-'0');
            	 	 	 ++nos_digits;
            	 		}
            	 	 valid=nos_digits==4; // we need exactly 4 digits
					 if(valid)
					 	{ t = 60*(t/100)+(t%100) ; // last 2 digits are minutes, first 2 digits are hours (which we multiply by 60 to get to minutes)	
					 	 if(negative)  strp_tz.tz_off_mins= -t;
					 	 else  strp_tz.tz_off_mins=t;
					 	}
            	 	}
            	 }
				// use  strp_tz.tz_off_mins= -1; 
#else  			/* Just check - do not do anything with the value. */
				if(*s=='+' || *s=='-') ++s; // leading sign (required)
				 else valid=false;
				if(valid && isdigit(*s)) ++s;// 1st digit of number
				 else valid=false;			
				if(valid && isdigit(*s)) ++s;// 2nd digit of number
				 else valid=false;		
				if(valid && isdigit(*s)) ++s;// 3rd digit of number
				 else valid=false;		
				if(valid && isdigit(*s)) ++s;// 4th digit of number
				 else valid=false;						 				 				 	
#endif				 
				break;
			case 'Z':// %Z  time zone name. 2, 3 or 4 letters eg "ET", “EDT”, "UTC", "GMT", "AKST" etc. Put the value found into the strp_tz structure 
					/* case is ignored in TZ comparisons (only impacts UTC test below) */
				if(isalpha(*s)) strp_tz.tz_name[0]=*s++;// 1st character of name (required)
				 else valid=false;			
				if(valid && isalpha(*s)) 
					{strp_tz.tz_name[1]=*s++;// 2nd character of name (required)	
					 if(isalpha(*s)) 
						{strp_tz.tz_name[2]=*s++;// optional 3rd character of name
					     if(isalpha(*s)) strp_tz.tz_name[3]=*s++;// optional 4th character of name 
					 	}
					}
				 else valid=false;				 
				if(!valid) 
					{ strp_tz.tz_name[0]=0; // if not valid go back to default TZ
 					  strp_tz.tz_name[1]=0;
 					  strp_tz.tz_name[2]=0;	
 					  strp_tz.tz_name[3]=0;	
 					}
 				else
 					{// we could set mydate.tz_off_mins here - for now just do for UTC as in general this is quite hard and as some time zone names are ambigeous in general may be impossible!
 					 if(strnicmp(strp_tz.tz_name,"UTC",4)==0)
 					 	strp_tz.tz_off_mins=0;// UTC has zero offset to UTC
 					}
				break;			
            case '%': // %% in the format string means we need a % character in the input 
                if (*s != '%')
                    valid = false;
                ++s;
                break;
            default:
                valid = false;
                }
            }
            break;
        case ' ':
        case '\t':
        case '\r':
        case '\n':
        case '\f':
        case '\v':
            // zero or more whitespaces:
            while (isspace((int)*s))
                ++ s;
            break;
        default:
            // match character
            if (*s != *format)
                valid = false;
            else
                ++s;
            break;
            }
        ++format;
        }
    if(!valid || *format!=0) return NULL; //  return NULL on error (not using all the format is clearly an error)   
	/* if %U, %V or %W has been given and we also have day of week (%u or %w) then we can work out date (or rather days in to year (0->366)) from the other things entered */
	int day_of_week_yd(int64_t year,int yday); /* year with no offset eg 1900 and yday is day of year 0->365 */ 
	if(weekday_found) // set when %u or %w found
		{
		 int wday=tm->tm_wday;// sunday=0
		 if(per_U_found)
			{// week of year, sunday being first day of week (0-53)
			 int day1jan=day_of_week_yd(tm->tm_year+1900,0);// day of week of 1st Jan
			 if(strp_tz.week_nos_U==0)
			 	{// 1st week is special case
			 	 tm->tm_yday=wday-day1jan;
			 	}
			 else	
			 	tm->tm_yday=7*(strp_tz.week_nos_U)+wday-day1jan; // days in year is simple
			}
		 else if(per_V_found && strp_tz.year_G!=strp_tz_default)
		 	{/* week of year using a week based year - algorithm from https://en.wikipedia.org/wiki/ISO_week_date#Calculating_an_ordinal_or_month_date_from_a_week_date
		 	    Multiply the week number by 7.
    			Then add the weekday number. (1->7 with 1 as Monday)
    			From this sum subtract the correction for the year:
        			Get the weekday of 4 January.
        			Add 3.
    			The result is the ordinal date, which can be converted into a calendar date.
        		If the ordinal date thus obtained is zero or negative, the date belongs to the previous calendar year;
        		if it is greater than the number of days in the year, it belongs to the following year.
        		Note input year here comes from %G NOT %Y
        	 */
        	 if(wday==0) wday=7;// make sunday 7 rather than 0, 1=monday
        	 int i=strp_tz.week_nos_V*7+wday;
        	 tm->tm_year=strp_tz.year_G-1900;// assume actual year is the same as iso year
        	 int dayjan4=day_of_week_yd(tm->tm_year+1900,3);// day of week of 4th Jan
        	 if(dayjan4==0) dayjan4=7; // as above make 1=monday to 7=sunday
        	 i-=dayjan4+3; // note i may be negative (and so in previous year) or > days/year in which case its in the next year !
        	 if(i<=0)
        	 	{// in previous year 
        	 	 tm->tm_year--; // previous year
        	 	 int days_per_year=is_leap(tm->tm_year+1900)?366:365;
        	 	 tm->tm_yday=days_per_year+i-1; // i is negative
        	 	}
        	 else
			 	{// i>0
				 int days_per_year=is_leap(tm->tm_year+1900)?366:365;
				 if(i>days_per_year)
				 	{tm->tm_year++; // next year
					 tm->tm_yday=i-days_per_year-1;	
					}
				 else	
        	 		tm->tm_yday=i-1;
        		}
			}
		 else if(per_W_found)
		 	{// week of year , monday being 1st day
		 	 // printf("** %%W found year=%d weeknos=%d day of week=%d",tm->tm_year+1900,strp_tz.week_nos_W,wday);
			 int day1jan=day_of_week_yd(tm->tm_year+1900,0);// day of week of 1st Jan
			 if(day1jan==0) day1jan=6; // sunday=>6
			  else day1jan--; // monday=0 etc
			 if(wday==0) wday=6; // sunday=>6
			  else wday--; // monday=0 etc	 
			 if(strp_tz.week_nos_W==0)
			 	{// 1st week is special case
			 	 tm->tm_yday=wday-day1jan;
			 	}
			 else	
			 	tm->tm_yday=7*(strp_tz.week_nos_W)+wday-day1jan; // days in year is simple
			 // printf(" after calcs wday=%d day1jan=%d\n",wday,day1jan);	
		 	}
		 if(per_U_found||(per_V_found && strp_tz.year_G!=strp_tz_default)||per_W_found)
		 	{	
		 	 // now set other fields (eg month, day of month) from tm_yday
		 	 /*
			   printf("  before ya_mktime: Year=%d (%d) Month=%u (%u=%s) Day of month=%u Hours=%u Mins=%u Secs=%u Day=%s Day of year=%u, isdst=%d\n",
 				tm->tm_year,tm->tm_year+1900,tm->tm_mon,tm->tm_mon+1,strp_monthnames[tm->tm_mon],tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec,
    			(tm->tm_wday>=0 && tm->tm_wday<=6)?strp_weekdays[tm->tm_wday]:"???", tm->tm_yday, tm->tm_isdst);  
    		 */	
#if 1   			
			 month_day(tm->tm_year+1900,tm->tm_yday, &(tm->tm_mon), &(tm->tm_mday));	// this does not set tm_yday, but thats not an issue as its set above (and its used here to set month & day of month)	 
#else
			 tm->tm_mday=0; // set month to an invalid value so it has to be calculated
			 ya_mktime(tm); // this should do the same as above, setting tm to standardised values based on year & yday
#endif
			/*
			 printf("  after ya_mktime: Year=%d (%d) Month=%u (%u=%s) Day of month=%u Hours=%u Mins=%u Secs=%u Day=%s Day of year=%u, isdst=%d\n",
 				tm->tm_year,tm->tm_year+1900,tm->tm_mon,tm->tm_mon+1,strp_monthnames[tm->tm_mon],tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec,
    			(tm->tm_wday>=0 && tm->tm_wday<=6)?strp_weekdays[tm->tm_wday]:"???", tm->tm_yday, tm->tm_isdst);  		 	 
    		*/
 			 strp_tz.week_nos_U= strp_tz_default; // set back to defaults as we went them to be calculated if used for output, not just echoed ....
			 strp_tz.week_nos_V= strp_tz_default;
			 strp_tz.week_nos_W= strp_tz_default; 
			 strp_tz.year_G= strp_tz_default;	
		 	}

		} 

    return (char *)s;// or character after last match if sucessfull.
    }


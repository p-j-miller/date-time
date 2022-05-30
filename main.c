/* test program for strptime() & strftime()

   Written by Peter Miller 24/3/3022

This file compiles and runs correctly with devC++/TDMgcc 10.3.0 on Windows, Builder C++ (10.2) on Windows and gcc 9.4.0 and 10.3.0 on Ubuntu Linux.

For gcc under linux compile test program with :

  gcc -Wall -O3 -o date-time main.c strftime.c strptime.c 
  ./date-time
  
For dev-C++ (tested with tdmgcc 10.3.0) there is a *.dev file, and the project files (*.cbproj) for Builder C++ are also present. 
You should see no errors or warnings when compiling these files.

In all cases when running the executable you should see lots of output with the last line reading:
7300275 tests conducted, no errors found

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <ctype.h>
#include <locale.h>
#include <stdint.h>
#include <limits.h>
#include "time_local.h"
#ifdef _WIN32 /* defined when compiling for windows, either 32 or 64 bits */
 #include <windows.h> /* to allow colour changes on text output - windows only ! */
#endif

// #define POSIX_2008 /* if defined the %Y is limited to 4 digits */ 

/* the two #defines below sets this code to use the new functions defined in strptime.c & strftime.c rather than the ones (that might be) in the standard libraries */
/* note because of the extensions present in our new functions (and tested by the code below), its extremely unlikely implementations in the standard libraries
   will pass all the test cases */
// char * ya_strptime(const char *s, const char *format, struct tm *tm);
#define strptime(s,f,m) ya_strptime(s,f,m) 
// size_t ya_strftime(char *s, size_t maxsize, const char *format, const struct tm *timeptr); 
#define strftime(s,m,f,t) ya_strftime(s,m,f,t) 

unsigned int nos_tests=0;
unsigned int errs=0;

enum test_type {good,not_mktime,ignore_0,ignore_plus,bad};// possible expected outcomes for test
char *type_name[]={"good","not_mktime","ignore_0","ignore_plus","bad"}; // string representation of above enum test_type


int strcmp_ign0(const char *s,const char *t) /* compare s with t ignoring case, whitespace and  0's [ascii digit 'zero'], so returns 0 iff string s == string t ignoring any whitespace 0's in s or t */
{while(1)
	{while(isspace(*s)) ++s;// skip any leading whitespace in s
	 while(isspace(*t)) ++t;// skip any leading whitespace in t
	 while(*s=='0') ++s;// skip any leading 0's in s
	 while(*t=='0') ++t;// skip any leading 0's in t
	 if(toupper(*s)!=toupper(*t)) return *s-*t; // different, return in same way as strcmp
	 if(*s==0) return 0; // match, and end of string
	 ++s;++t; // match and not end of string, move to next characters
	}
}

int strcmp_ign_plus(const char *s,const char *t) /* compare s with t ignoring case, whitespace and  leading + sign, so returns 0 iff string s == string t ignoring any whitespace or leading + sign in s or t */
{
 while(isspace(*s)) ++s;// skip any leading whitespace in s
 while(isspace(*t)) ++t;// skip any leading whitespace in t
 if(*s=='+') ++s;// skip any leading + sign in s
 if(*t=='+') ++t;// skip any leading + sign in t
 while(1)
	{while(isspace(*s)) ++s;// skip any leading whitespace in s
	 while(isspace(*t)) ++t;// skip any leading whitespace in t
	 if(toupper(*s)!=toupper(*t)) return *s-*t; // different, return in same way as strcmp
	 if(*s==0) return 0; // match, and end of string
	 ++s;++t; // match and not end of string, move to next characters
	}
}

struct tm tm; /* used for testing - but global so we can use it afterwards */

void red_text(void) /* black text on a bright red background */
{
#ifdef _WIN32 /* defined when compiling for windows, either 32 or 64 bits */ 
  HANDLE console_color; // see https://www.geeksforgeeks.org/colorizing-text-and-console-background-in-c/?ref=rp 
  console_color = GetStdHandle(STD_OUTPUT_HANDLE);// colours below can be more generally defined via the constants defined at https://docs.microsoft.com/en-us/windows/console/console-screen-buffers#character-attributes 
  SetConsoleTextAttribute(console_color, 0xC0);// C0=> black text on a bright red background . 0C = bright Red text on black background
#endif  
#ifdef __linux
  printf("\033[1;30m\033[1;41m"); // black text on a red background see https://www.tutorialspoint.com/how-to-output-colored-text-to-a-linux-terminal for all colour options
#endif 	
}

void normal_text(void) 
{
#ifdef _WIN32 /* defined when compiling for windows, either 32 or 64 bits */  
  HANDLE console_color; // see https://www.geeksforgeeks.org/colorizing-text-and-console-background-in-c/?ref=rp 
  console_color = GetStdHandle(STD_OUTPUT_HANDLE);// colours below can be more generally defined via the constants defined at https://docs.microsoft.com/en-us/windows/console/console-screen-buffers#character-attributes 
  SetConsoleTextAttribute(console_color, 0x07);// back to white text on black background (normal console colours) 
#endif
#ifdef __linux
  printf("\033[0m");// normal text
#endif	
}

void display_tm(void)
{ // check each field of tm and display them via printf - in red if out of valid range. no newline at the end
 bool OK=true;
 int mday=tm.tm_mday;
 if(mday==0) tm.tm_mday=1; // lower limit is strickly 1, but 0 is default so we deal with this case in printf below without flagging it as an error
 OK=check_tm(&tm); // check tm is all valid
 tm.tm_mday=mday; // restore mday
 if(!OK) 
 	{red_text();
 	 ++errs;
 	}
 printf("Year=%d (%.0f) Month=%d (%d=%s) Day of month=%d%c Hours=%d Mins=%d Secs=%d Day=%d(%s) Day of year=%d, isdst=%d",
 	tm.tm_year,(double)tm.tm_year+1900,tm.tm_mon,tm.tm_mon+1,(tm.tm_mon>=0 && tm.tm_mon<12)?strp_monthnames[tm.tm_mon]:"???",tm.tm_mday,tm.tm_mday==0?'!':' ', tm.tm_hour, tm.tm_min, tm.tm_sec,
    tm.tm_wday,(tm.tm_wday>=0 && tm.tm_wday<=6)?strp_weekdays[tm.tm_wday]:"???", tm.tm_yday, tm.tm_isdst);  
 if(!OK) normal_text();
}
 

bool test(const char *string, const char *format,enum test_type tt) /* returns true if round loop  strptime -> strftime gives expected result */
																	/* if tt=good then expect everything to work (ie round loop exact)
																	   if tt==bad then result is expected to be incorrect 
																	   if tt==not_mktime then expect result from mktime() to be wrong, rest OK 
																	   if tt==ignore_0 then expect strptime to return OK, and string compares to work if zero's ignored (strftime() will put leading zero's in even if they were missing in the input)
																	   if tt==ignore_plus then expect strptime to return OK, and string compares to work if leading + sign is ignored 
																	*/
{
 char buf[255];
 time_t ti=0;
 bool r,r1;
 char *end;
 bool expect_valid_s,expect_valid_r,expect_valid_r1;
 extern const char * strp_weekdays[];
 extern const char * strp_monthnames[] ;
 expect_valid_s=(tt==good || tt==not_mktime||tt==ignore_0||tt==ignore_plus ); // we expect strptime() to parse data correctly
 expect_valid_r=(tt==good || tt==not_mktime || tt==ignore_0|| tt==ignore_plus);// we expect round loop to be correct 
 expect_valid_r1=(tt==good || tt==ignore_0 || tt==ignore_plus);// if not_mktime we expect this to fail
 nos_tests++;
 memset(&tm, 0, sizeof(struct tm));// zero all members of tm
 init_strp_tz(&strp_tz); 
 end=strptime(string, format, &tm);
 printf("Input:      %s format %s => ",string,format); 
 display_tm();
 printf(" : ");
 if(expect_valid_s)
 	{// don't expect an error	
 	 if(end==NULL) puts("Conversion failed! [returned NULL]");
 	 else if(*end!=0) printf("Conversion failed! [not at end of string = string remaining=\"%s\"]\n",end);
 	 else puts("OK"); // if *end==0
 	}
 else
	{// we do expect an error to be found
  	 if(end==NULL) puts("Conversion failed! [returned NULL] - OK");
 	 else if(*end!=0) printf("Conversion failed! [not at end of string = string remaining=\"%s\"] - OK\n",end);
 	 else puts("Wrong! (error was not detected)"); // if *end==0
 	}	
 strftime(buf, sizeof(buf), format, &tm);
 printf("  strftime=>%s ",buf);
 if(tt==ignore_0)
 	{buf[sizeof(buf)-1]=0;// make sure buf is zero terminated
 	 r=strcmp_ign0(string,buf)==0; 	
 	}
 else if(tt==ignore_plus)
 	{buf[sizeof(buf)-1]=0;// make sure buf is zero terminated
 	 r=strcmp_ign_plus(string,buf)==0; 	
 	}	
 else	
 	r=strnicmp(string,buf,sizeof(buf))==0; 	// strnicmp() used as want to ignore case eg pm->PM
 puts(r^!expect_valid_r?"OK":"Wrong!");// puts() adds newline
 tm.tm_isdst=-1;// make sure daylight savings time is not on
 ti=ya_mktime(&tm);// convert tm to a time_t, also "normalises" tm and sets other members (like day of week).
 tm.tm_isdst=-1; // make sure daylight savings time is not on
 strftime(buf, sizeof(buf), format, &tm);
 // printf("  mktime()=>%s (ti=%.0f) ",buf,(double)ti);// convert ti to a double for display as we don't know what type it actually is
 printf("  mktime()=>%s (ti=%.0f) => ",buf,(double)ti); 
 display_tm();
 printf(" : ");
 if(tt==ignore_0)
 	{buf[sizeof(buf)-1]=0;// make sure buf is zero terminated
 	 r1=strcmp_ign0(string,buf)==0; 	
 	}
 else if(tt==ignore_plus)
 	{buf[sizeof(buf)-1]=0;// make sure buf is zero terminated
 	 r1=strcmp_ign_plus(string,buf)==0; 	
 	} 	
 else	
 	r1=strnicmp(string,buf,sizeof(buf))==0; 	 	
 puts(r1^!expect_valid_r1?"OK":"Wrong!"); // puts() adds newline
 return (r^!expect_valid_r) && (r1^!expect_valid_r1) && ((end!=NULL && *end==0)^!expect_valid_s);// only return true if result was what we expected 
}

void perr(const unsigned int l,const char *string, const char *format,enum test_type tt)
{// error found - highlight it in red to make it easy to spot 
 red_text();
 printf(" ***  Error found on line %u : %s %s %s\n",l,string,format,type_name[tt] );   
 normal_text();
}

#define err_chk(s,f,g) {if(!test(s,f,g)){errs++;perr(__LINE__,s,f,g);}else printf(" Line %u OK : %s %s %s\n",__LINE__,s,f,type_name[g] ); } /* do test and highlight line if error */

bool UTC_test(const char *in_buf, const char *format,int isdst)
{
 /* now check UTC_mktime() and UTC_sec_to_tm() [so uses mydate]*/
 init_strp_tz(&strp_tz); 
 char buf[255];
 bool r;
 r=test(in_buf,format,good);// do test as done previoulsy, as a side effect set tm & strp_tz
 if(!r) 
 	{printf("UTC_test: test() failed: ");
	 return false; // test() failed
	}	
 tm.tm_isdst=isdst; // set daylight savings time value
 time_t ti=UTC_mktime(&tm,&strp_tz);
 UTC_sec_to_tm(ti,&tm,&strp_tz);
 printf("  UTC_mktime()=>%s (ti=%.0f) => ",in_buf,(double)ti);  
 display_tm();
 printf("\n");	 
 strftime(buf, sizeof(buf), format, &tm);
 return strnicmp(in_buf,buf,sizeof(buf))==0; // true means OK
}

#define err_UTC_chk(s,f,dst) {if(!UTC_test(s,f,dst)){errs++;perr(__LINE__,s,f,good);}else printf(" Line %u OK : %s %s isdst=%d\n",__LINE__,s,f,dst ); } /* do test and highlight line if error */

int main(void) 
{ errs=0;
 setlocale(LC_TIME,"C"); // this now makes no difference, but as the C standard defines exactly what this means its left in.
 // expect OK	
 err_chk("2001-11-12 18:31:01","%Y-%m-%d %H:%M:%S",good);// example of date/time matching format
 err_chk("1970-01-01 00:00:00","%Y-%m-%d %H:%M:%S",good);// should give 0 zeros past epoch
 err_chk("2001-1-1 8:1:1","%Y-%m-%d %H:%M:%S",ignore_0);// single digits where possible [  flagged as ignore_0 as strftime will print single digit numbers as 2 digits with leading zeros ]
 err_chk("2001-1-1 8:1:1","%Y-%m-%d %H:%M:%S",ignore_0);// single digits where possible [ flagged as ignore_0 as strftime will print single digit numbers as 2 digits with leading zeros ]
 err_chk("2001-01-02 08:01:01","%Y-%m-%d %H:%M:%S",good);// leading zeros where allowed (max 2 digits)
 err_chk("0-01-01 00:00:00","%Y-%m-%d %H:%M:%S",good);// min valid values
 err_chk("9999-12-31 23:59:60","%Y-%m-%d %H:%M:%S",not_mktime);// max valid values (invalid values checked below) - note 9999 is too high for mktime() test as 60 secs "rounds up" to 10000-01-01 00:00:00
 err_chk("9999-12-31 23:59:59","%Y-%m-%d %H:%M:%S",good);// max valid values (invalid values checked below) - 59 secs is OK
 #ifndef POSIX_2008 
 err_chk("+9999-12-31 23:59:59","%Y-%m-%d %H:%M:%S",ignore_plus);// value with +ve sign
 err_chk("391220960-11-12 18:31:01","%Y-%m-%d %H:%M:%S",good);// year max +ve  (+391220960)
 err_chk("-39171945-11-12 18:31:01","%Y-%m-%d %H:%M:%S",good);// year max-ve (-39171945 )
 #endif 
 err_chk("2001-11-12 18:31:01","%Y-%m-%e %H:%M:%S",good);// use of %e instead of %d -> with tdm-gcc 10.3.0 strftime() does not seem to support %e even though its in c99 !
 err_chk("2001-11- 2 18:31:01","%Y-%m-%e %H:%M:%S",good);// use of %e instead of %d (%e has leading space for single digit numbers)
 err_chk("2001-11-12 06:31:01 pm","%Y-%m-%d %I:%M:%S %p",good);// 12 hour time format
 err_chk("2001-11-12 06:31:01 am","%Y-%m-%d %I:%M:%S %p",good);// 12 hour time format
 err_chk("2001-11-12 6:31:1%pm","%Y-%m-%d %I:%M:%S%%%p",ignore_0);// 12 hour time format 1 digit hour test %%
 err_chk("2001-11-12 06:31:01am","%Y-%m-%d %I:%M:%S%p",good);// 12 hour time format, no whitespace before am/pm
 err_chk("2001-Jan-12 18:31:01","%Y-%b-%d %H:%M:%S",good);// %b abreviated date (3 chars) in words  
 err_chk("2001-Feb-12 18:31:01","%Y-%b-%d %H:%M:%S",good);
 err_chk("2001-Mar-12 18:31:01","%Y-%b-%d %H:%M:%S",good);
 err_chk("2001-Apr-12 18:31:01","%Y-%b-%d %H:%M:%S",good);
 err_chk("2001-May-12 18:31:01","%Y-%b-%d %H:%M:%S",good);
 err_chk("2001-Jun-12 18:31:01","%Y-%b-%d %H:%M:%S",good);
 err_chk("2001-Jul-12 18:31:01","%Y-%b-%d %H:%M:%S",good);
 err_chk("2001-Aug-12 18:31:01","%Y-%b-%d %H:%M:%S",good);
 err_chk("2001-Sep-12 18:31:01","%Y-%b-%d %H:%M:%S",good);
 err_chk("2001-Oct-12 18:31:01","%Y-%b-%d %H:%M:%S",good);
 err_chk("2001-Nov-12 18:31:01","%Y-%b-%d %H:%M:%S",good);
 err_chk("2001-Dec-12 18:31:01","%Y-%b-%d %H:%M:%S",good);
 err_chk("2001-January-12 18:31:01","%Y-%B-%d %H:%M:%S",good);// Full date in words "january", "february", "march", "april", "may", "june", "july", "august", "september", "october", "november", "december"
 err_chk("2001-February-12 18:31:01","%Y-%B-%d %H:%M:%S",good); 
 err_chk("2001-March-12 18:31:01","%Y-%B-%d %H:%M:%S",good); 
 err_chk("2001-April-12 18:31:01","%Y-%B-%d %H:%M:%S",good); 
 err_chk("2001-May-12 18:31:01","%Y-%B-%d %H:%M:%S",good); 
 err_chk("2001-June-12 18:31:01","%Y-%B-%d %H:%M:%S",good); 
 err_chk("2001-July-12 18:31:01","%Y-%B-%d %H:%M:%S",good); 
 err_chk("2001-August-12 18:31:01","%Y-%B-%d %H:%M:%S",good); 
 err_chk("2001-September-12 18:31:01","%Y-%B-%d %H:%M:%S",good); 
 err_chk("2001-October-12 18:31:01","%Y-%B-%d %H:%M:%S",good); 
 err_chk("2001-November-12 18:31:01","%Y-%B-%d %H:%M:%S",good); 
 err_chk("2001-December-12 18:31:01","%Y-%B-%d %H:%M:%S",good);
 err_chk("2001-Dec-12 18:31:01","%Y-%h-%d %H:%M:%S",good);// %h is C99, but is missing from TDM-GCC 10.3.0
 // day of week "sunday", "monday", "tuesday", "wednesday", "thursday", "friday", "saturday"
 err_chk("Mon","%a",good);// abreviated weekday name - by fluke this is correct for mktime(), the rest will give errors with mktime() 
 err_chk("Tue","%a",not_mktime);
 err_chk("Wed","%a",not_mktime);
 err_chk("Thu","%a",not_mktime);
 err_chk("Fri","%a",not_mktime);
 err_chk("Sat","%a",not_mktime);
 err_chk("Sun","%a",not_mktime); 
 err_chk("Monday","%A",good); // full weekday name by fluke this is correct for mktime(), the rest will give errors with mktime() 
 err_chk("Tuesday","%A",not_mktime);
 err_chk("Wednesday","%A",not_mktime);
 err_chk("Thursday","%A",not_mktime);
 err_chk("Friday","%A",not_mktime); 
 err_chk("Saturday","%A",not_mktime);
 err_chk("Sunday","%A",not_mktime);  
 err_chk("1","%j",ignore_0);// day of year (1->366)
 err_chk("01","%j",ignore_0);// day of year (1->366)
 err_chk("001","%j",good);// day of year (1->366)
 err_chk("365","%j",good);
 err_chk("366","%j",not_mktime);// 1900n was not a leap year so this fails mktime() 
 err_chk("2001-11-12 06:31:01 am","%Y-%m-%d%n%I:%M:%S%t%p",ignore_0);// ignore0 also ignores whitespace. %n and %t should be in C99 both appear to be missing from TDM-GCC 10.3.0
 err_chk("2001-11-12 06:31:01 am","%Y-%m-%d %I:%M:%S%t%p",ignore_0);// ignore0 also ignores whitespace. %n and %t should be in C99 both appear to be missing from TDM-GCC 10.3.0
 err_chk("2001-11-12 06:31:01 am","%Y-%m-%d%n%I:%M:%S %p",ignore_0);// ignore0 also ignores whitespace. %n and %t should be in C99 both appear to be missing from TDM-GCC 10.3.0
 err_chk("0","%U",ignore_0);// week number of year (0-53) - this does not set a value in tm structure, so nothing to check
 err_chk("00","%U",good);
 err_chk("53","%U",good);
 err_chk("0","%W",ignore_0);// week number of year (0-53)
 err_chk("00","%W",good);
 err_chk("53","%W",good);
 err_chk("1","%V",ignore_0);// %V week number of year (1-53) 
 err_chk("01","%V",good);
 err_chk("53","%V",good); 
 err_chk("0","%w",not_mktime);// weekday as a number (0->6)
 err_chk("1","%w",good);// weekday as a number (1->7) - by fluke this is correct
 err_chk("6","%w",not_mktime); 
 err_chk("1","%u",good);// weekday as a number (1->7) - by fluke this is correct
 err_chk("7","%u",not_mktime);  
 err_chk("00-11-12 18:31:01","%y-%m-%d %H:%M:%S",good);// 2 digit year [69,99] shall refer to years 1969 to 1999 & [00,68] shall refer to years 2000 to 2068 inclusive
 err_chk("68-11-12 18:31:01","%y-%m-%d %H:%M:%S",good);// 2 digit year
 err_chk("69-11-12 18:31:01","%y-%m-%d %H:%M:%S",good);// 2 digit year
 err_chk("99-11-12 18:31:01","%y-%m-%d %H:%M:%S",good);// 2 digit year
 err_chk("1-11-12 18:31:01","%y-%m-%d %H:%M:%S",ignore_0);// 2 digit year
 
 err_chk("00-11-12 18:31:01","%g-%m-%d %H:%M:%S",good);// %g: 2 digit year [69,99] shall refer to years 1969 to 1999 & [00,68] shall refer to years 2000 to 2068 inclusive
 err_chk("68-11-12 18:31:01","%g-%m-%d %H:%M:%S",good);// 2 digit year
 err_chk("69-11-12 18:31:01","%g-%m-%d %H:%M:%S",good);// 2 digit year
 err_chk("99-11-12 18:31:01","%g-%m-%d %H:%M:%S",good);// 2 digit year
 err_chk("1-11-12 18:31:01","%g-%m-%d %H:%M:%S",ignore_0);// 2 digit year
 err_chk("2001-11-12 18:31:01","%G-%m-%d %H:%M:%S",good);// %G (rather than %Y) example of date/time matching format
 
 err_chk("2001-11-12 18:31:01","%C%y-%m-%d %H:%M:%S",good);// 4 digit year via %C%y. %C should be in C99 
 err_chk("20c01-11-12 18:31:01","%Cc%y-%m-%d %H:%M:%S",good);// 4 digit year via %C%y %C should be in C99
 err_chk("0001-11-12 18:31:01","%C%y-%m-%d %H:%M:%S",good);// 4 digit year via %C%y. %C should be in C99 
 err_chk("00c01-11-12 18:31:01","%Cc%y-%m-%d %H:%M:%S",good);// 4 digit year via %C%y %C should be in C99 
 err_chk("9901-11-12 18:31:01","%C%y-%m-%d %H:%M:%S",good);// 4 digit year via %C%y. %C should be in C99 
 err_chk("99c01-11-12 18:31:01","%Cc%y-%m-%d %H:%M:%S",good);// 4 digit year via %C%y %C should be in C99 
 err_chk("2c01-11-12 18:31:01","%Cc%y-%m-%d %H:%M:%S",ignore_0);// 4 digit year via %C%y. %C should be in C99 
 err_chk("0c01-11-12 18:31:01","%Cc%y-%m-%d %H:%M:%S",ignore_0);// 4 digit year via %C%y %C should be in C99    
 
 err_chk("01/11/22","%D",good);// %D = %m/%d/%y . %D should be in C99 but appears to be missing from TDM-GCC 10.3.0
 err_chk("01/01/00","%D",good);// %D = %m/%d/%y . %D should be in C99 but appears to be missing from TDM-GCC 10.3.0
 err_chk("12/31/99","%D",good);// %D = %m/%d/%y . %D should be in C99 but appears to be missing from TDM-GCC 10.3.0
 err_chk("01/11/22","%x",good);// %x = %m/%d/%y . 
 err_chk("01/01/00","%x",good);// %x = %m/%d/%y . 
 err_chk("12/31/99","%x",good);// %x = %m/%d/%y . 
 
 err_chk("18:31:01","%X",good);// %X = %H:%M:%S
 err_chk("00:00:00","%X",good);// %X = %H:%M:%S
 err_chk("23:59:59","%X",good);// %X = %H:%M:%S
 err_chk("18:31:01","%T",good);// %T = %H:%M:%S . %C should be in C99 but appears to be missing from TDM-GCC 10.3.0
 err_chk("00:00:00","%T",good);// %X = %H:%M:%S
 err_chk("23:59:59","%T",good);// %X = %H:%M:%S
 
 err_chk("mon jan  1 12:31:01 2001","%c",good);// %c= %a %b %e %T %Y  
 
 err_chk("18:31","%R",good);// %R = %H:%M .%C should be in C99 but appears to be missing from TDM-GCC 10.3.0
 err_chk("00:00","%R",good);
 err_chk("23:59","%R",good);

 err_chk("06:31:01 am","%r",good);// %r = 12 hour clock %I:%M:%S %p . Should be in C99 but appears to be missing from TDM-GCC 10.3.0
 err_chk("06:31:01 pm","%r",good);// 
 err_chk("12:59:59 am","%r",good);// 
 
 err_chk("2001-05-31","%F",good);// %F Equivalent to %Y-%m-%d (the iso 8601 date format).Should be in C99 but appears to be missing from TDM-GCC 10.3.0
 
 err_chk("01/11/22","%Ex",good);// %Ex = %m/%d/%y .Should be in C99 but appears to be missing from TDM-GCC 10.3.0 
 err_chk("01/01/00","%Ex",good);
 err_chk("12/31/99","%Ex",good); 
 
 err_chk("18:31:01","%EX",good);// %EX = %H:%M:%S .Should be in C99 but appears to be missing from TDM-GCC 10.3.0
 err_chk("00:00:00","%EX",good);
 err_chk("23:59:59","%EX",good);
 
 err_chk("68-11-12 18:31:01","%Oy-%Om-%Od %OH:%OM:%OS",good);// %Oy %Om etc should be processed as if the O was not present.Should be in C99 but appears to be missing from TDM-GCC 10.3.0 
 
 err_chk("+0500","%z",good); // time ofset from UTC sign(required) and 4 digits
 err_chk("+0000","%z",good); // time ofset from UTC
 err_chk("-0500","%z",good); // time ofset from UTC
 err_chk("ET","%Z",good);// time zone name (2,3 or 4 letters)
 err_chk("GMT","%Z",good);// time zone name
 err_chk("BST","%Z",good);// time zone name
 err_chk("UTC","%Z",good);// time zone name
 err_chk("AKST","%Z",good);// AKST is Alaska
  
 err_chk("123456789","%s",good);/* secs past the epoch */
 err_chk("0123456789","%s",ignore_0);
 err_chk("-123456789","%s",good);
 err_chk("+123456789","%s",ignore_plus); // test must ignore + sign, as this is valid on input, but not produced on o/p
 err_chk("0 Thursday","%s %A",good);// January 1st 1970 (t=0) was a Thursday (= 4)
 // printf(" January 1st 1970 (t=0) was a Thursday (= 4)\n");
 err_chk("12345678900","%s",good);// 12,345,678,900 is OK and is in the year 2361 
 err_chk("123456789000","%s",good);// 123456789000 fails when using gmtime() with TDM-GCC 10.3.0, but sec_to_tm() [as now used] works. 12,345,678,900 is OK and is the year 2361 so this is not an immediate issue but as 2^31=2,147,483,648 its not clear why there is a limit here?
 err_chk("1234567890000","%s",good);
 err_chk("12345678900000","%s",good);
 err_chk("123456789000000","%s",good);
 err_chk("1234567890000000","%s",good); // this is only 4 62D5 3C88 D880 so a long way from the highest possible with a 64 bit signed number 
 err_chk("12345678900000000","%s",good); // largest allowed
 err_chk("-123456789000","%s",good); // check -ve
 err_chk("-1234567890000000","%s",good); // largest -ve allowed
 // 67723104210192000 is new max & -66822984373392000 new min 
  err_chk("67723104210192000","%s",good); // largest allowed
  err_chk("67768036162659144","%s",good); // largest allowed
 err_chk("-66822984373392000","%s",good); // largest -ve allowed 
 err_chk("-67678052500542456","%s",good);
 
 // %W Monday is the 1st day of the week
 err_chk("2005 34 mon 22 Aug","%Y %W %a %e %b",good); // 22nd Aug (this bit will be recalculated by the code from week number of day of week)
 err_chk("2005 34 tue 23 Aug","%Y %W %a %e %b",good); // 23rd Aug
 err_chk("2005 34 wed 24 Aug","%Y %W %a %e %b",good); // 24th Aug
 err_chk("2005 34 thu 25 Aug","%Y %W %a %e %b",good); // 25th Aug
 err_chk("2005 34 fri 26 Aug","%Y %W %a %e %b",good); // 26th Aug
 err_chk("2005 34 sat 27 Aug","%Y %W %a %e %b",good); // 27th Aug
 err_chk("2005 34 sun 28 Aug","%Y %W %a %e %b",good); // 28th Aug
 err_chk("2005 35 mon 29 Aug","%Y %W %a %e %b",good); // 29th Aug
 err_chk("2005 35 tue 30 Aug","%Y %W %a %e %b",good); // 30th Aug
 err_chk("2005 35 wed 31 Aug","%Y %W %a %e %b",good); // 31th Aug
 err_chk("2005 35 thu  1 Sep","%Y %W %a %e %b",good); // 1st sept (note extra space required before "1")
 
 err_chk("2004 52 mon 27 Dec","%Y %W %a %e %b",good); // end 2004 -> start 2005
 err_chk("2004 52 tue 28 Dec","%Y %W %a %e %b",good); // 
 err_chk("2004 52 wed 29 Dec","%Y %W %a %e %b",good); // 
 err_chk("2004 52 thu 30 Dec","%Y %W %a %e %b",good); // 
 err_chk("2004 52 fri 31 Dec","%Y %W %a %e %b",good); // 
 err_chk("2005 00 sat  1 Jan","%Y %W %a %e %b",good); // note extra space required before "1"
 err_chk("2005 00 sun  2 Jan","%Y %W %a %e %b",good); // 
 err_chk("2005 01 mon  3 Jan","%Y %W %a %e %b",good); // 
 err_chk("2005 01 tue  4 Jan","%Y %W %a %e %b",good); // 
 
 // Week 1	January 2, 2022	January 8, 2022 for %U (Sunday is 1st day of the week)
 err_chk("2021 52 fri 31 Dec","%Y %U %a %e %b",good); // 
 err_chk("2022 00 sat  1 Jan","%Y %U %a %e %b",good); // note extra space required before "1"
 err_chk("2022 01 sun  2 Jan","%Y %U %a %e %b",good); // 
 err_chk("2022 01 mon  3 Jan","%Y %U %a %e %b",good); // 
 err_chk("2022 01 tue  4 Jan","%Y %U %a %e %b",good); //  
 
 // %V (iso week) - needs to be used with %G as the year
 err_chk("2021 52 fri 31 Dec","%G %V %a %e %b",good); // 
 err_chk("2021 52 sat  1 Jan","%G %V %a %e %b",good); // note extra space required before "1" => year is 2022 !
 err_chk("2021 52 sun  2 Jan","%G %V %a %e %b",good); // 
 err_chk("2022 01 mon  3 Jan","%G %V %a %e %b",good); // 
 err_chk("2022 01 tue  4 Jan","%G %V %a %e %b",good); //   
 
  // %f for fractional secs
  err_chk("2001-11-12 18:31:01.0","%Y-%m-%e %H:%M:%S.%f",good);
  err_chk("2001-11-12 18:31:01.1","%Y-%m-%e %H:%M:%S.%f",good);
  err_chk("2001-11-12 18:31:01.9","%Y-%m-%e %H:%M:%S.%f",good);
  err_chk("2001-11-12 18:31:01.01","%Y-%m-%e %H:%M:%S.%f",good);
  err_chk("2001-11-12 18:31:01.010","%Y-%m-%e %H:%M:%S.%f",good);
  err_chk("2001-11-12 18:31:01.01234567890","%Y-%m-%e %H:%M:%S.%f",good);
  err_chk("2001-11-12 18:31:01.0123456789012345","%Y-%m-%e %H:%M:%S.%f",good);// 15 sg for %f is the limit for a double (beyond this not round loop exact)
 
 // expect failure
 printf("\n*** Tests below are all of type \"bad\"\n\n"); 
 
 err_chk("200111-12 18:31:01","%Y-%m-%d %H:%M:%S",bad); // 1st - is missing
 err_chk("2001?11-12 18:31:01","%Y-%m-%d %H:%M:%S",bad);// 1st - is wrong character
 err_chk("2001-11?12 18:31:01","%Y-%m-%d %H:%M:%S",bad);// 2nd - is wrong character
 err_chk("2001-11-12 18?31:01","%Y-%m-%d %H:%M:%S",bad);// 1st : is wrong character
 err_chk("2001-11-12 18:31?01","%Y-%m-%d %H:%M:%S",bad);// 2nd : is wrong character
 err_chk("2001-11-12 18:31:01?","%Y-%m-%d %H:%M:%S",bad);//extra character at end
 // err_chk("1899-11-12 18:31:01","%Y-%m-%d %H:%M:%S",bad);// year low - not needed as min is now 0
 #ifdef POSIX_2008 
 err_chk("10000-11-12 18:31:01","%Y-%m-%d %H:%M:%S",bad);// year high (max is 9999)
 err_chk("02001-11-12 18:31:01","%Y-%m-%d %H:%M:%S",bad);// year digits/leading zeros (max is 9999) 
 err_chk("+9999-12-31 23:59:59","%Y-%m-%d %H:%M:%S",bad);// + sign invalid in POSIX 2008
 err_chk("-9999-12-31 23:59:59","%Y-%m-%d %H:%M:%S",bad);// - sign invalid in POSIX 2008
 #else
 err_chk("391220961-11-12 18:31:01","%Y-%m-%d %H:%M:%S",bad);// year >max +ve  (+391220960)
 err_chk("-39171946-11-12 18:31:01","%Y-%m-%d %H:%M:%S",bad);// year< max-ve (-39171945 )
 #endif
 err_chk("2001-0-12 18:31:01","%Y-%m-%d %H:%M:%S",bad);// month lo
 err_chk("2001-13-12 18:31:01","%Y-%m-%d %H:%M:%S",bad);// month hi
 err_chk("2001-001-12 18:31:01","%Y-%m-%d %H:%M:%S",bad);// month digits/leading zeros
 err_chk("2001-11-0 18:31:01","%Y-%m-%d %H:%M:%S",bad);// day lo
 err_chk("2001-11-32 18:31:01","%Y-%m-%d %H:%M:%S",bad);// day hi
 err_chk("2001-11-012 18:31:01","%Y-%m-%d %H:%M:%S",bad);// day digits/leading zeros
 err_chk("2001-11-12 24:31:01","%Y-%m-%d %H:%M:%S",bad);// hour hi (0 is valid)
 err_chk("2001-11-12 000:31:01","%Y-%m-%d %H:%M:%S",bad);// hour digits/leading zeros
 err_chk("2001-11-12 18:60:01","%Y-%m-%d %H:%M:%S",bad);// minute hi (0 is valid)
 err_chk("2001-11-12 18:000:01","%Y-%m-%d %H:%M:%S",bad);// minute digits/leading zeros (0 is valid) 
 err_chk("2001-11-12 18:31:61","%Y-%m-%d %H:%M:%S",bad);// second hi (0 and 60 are valid)
 err_chk("2001-11-12 18:31:000","%Y-%m-%d %H:%M:%S",bad);// second digits/leading zeros (0 is valid) 
 err_chk("2001-11-12 18:31:060","%Y-%m-%d %H:%M:%S",bad);// too many digits in seconds [ so "0" left after processing ]
 err_chk("2001-11-12 06:31:01 zm","%Y-%m-%d %I:%M:%S %p",bad);// 12 hour time format zm instead of a/pm
 err_chk("2001-11-12 06:31:01 p","%Y-%m-%d %I:%M:%S %p",bad);// 12 hour time format mising "m" from pm
 err_chk("2001-11-12 06:31:01 ","%Y-%m-%d %I:%M:%S %p",bad);// 12 hour time format missing am/pm
 err_chk("2001-11-12 06:31:01","%Y-%m-%d %I:%M:%S %p",bad);// 12 hour time format missing whitespace and am/pm
  err_chk("2001-Nev-12 18:31:01","%Y-%B-%d %H:%M:%S",bad); // bad month
 err_chk("2001-Decembr-12 18:31:01","%Y-%B-%d %H:%M:%S",bad);
 err_chk("Mun","%a",bad);// abreviated weekday name 
 err_chk("Munday","%A",bad);
 err_chk("Mondy","%A",bad);
 err_chk("0","%j",bad);// day of year (1->366)
 err_chk("0001","%j",bad);// day of year (1->366)
 err_chk("367","%j",bad);
 err_chk("2001-11-12+06:31:01 am","%Y-%m-%d%n%I:%M:%S%t%p",bad);// + instead of whitespace
 err_chk("2001-11-12 06:31:01+am","%Y-%m-%d%n%I:%M:%S%t%p",bad);
 err_chk("000","%U",bad);// week number of year (0-53)
 err_chk("54","%U",bad);
 err_chk("000","%W",bad);// week number of year (0-53)
 err_chk("54","%W",bad);
 err_chk("0","%V",bad);// %V week number of year (1-53) 
 err_chk("00","%V",bad);// %V week number of year (1-53)  
 err_chk("001","%V",bad);// %V week number of year (1-53)  
 err_chk("54","%V",bad);// %V week number of year (1-53)  
 err_chk("00","%w",bad);// weekday as a number (0->6)
 err_chk("7","%w",bad);  
 err_chk("0","%u",bad);// weekday as a number (1->7)
 err_chk("01","%u",bad);
 err_chk("8","%u",bad);   
 err_chk("001-11-12 18:31:01","%y-%m-%d %H:%M:%S",bad);// %y 2 digit year
 err_chk("001-11-12 18:31:01","%g-%m-%d %H:%M:%S",bad);// %g 2 digit year
 err_chk("00/11/22 18:31:01","%c",bad);// Bad month : %c= %x %X  ie %m/%d/%y %H:%M:%S 
 err_chk("01/32/22 18:31:01","%c",bad);// Bad day of month : %c= %x %X  ie %m/%d/%y %H:%M:%S 
 err_chk("00/11/22 25:31:01","%c",bad);// Bad hours : %c= %x %X  ie %m/%d/%y %H:%M:%S 
 err_chk("00/11/22 18:61:01","%c",bad);// Bad mins : %c= %x %X  ie %m/%d/%y %H:%M:%S 
 err_chk("00/11/22 18:31:62","%c",bad);// Bad secs : %c= %x %X  ie %m/%d/%y %H:%M:%S 
 err_chk("","%s",bad);// %s uses its on number converter as it may need to take a big unsigned integer, so do more checks here.
 err_chk("+","%s",bad);
 err_chk("-","%s",bad);
 err_chk("z","%s",bad);
 err_chk("+z","%s",bad);
 err_chk("-z","%s",bad);
 err_chk("+0z","%s",bad);
 err_chk("-0z","%s",bad);
 err_chk("0z","%s",bad);
 err_chk("10z","%s",bad); 
 err_chk("123456789000000000","%s",bad); // much too big
 err_chk("1234567890000000000","%s",bad); // too big
 err_chk("12345678900000000000","%s",bad); // too big
 err_chk("-123456789000000000","%s",bad); // much too big  
  err_chk("67768036162659145","%s",bad); // just too big +ve
 err_chk("-67678052500542457","%s",bad); // just too big -ve
 err_chk("A","%Z",bad);// needs to be 2,3 or 4 alpha chars
 err_chk("AKSTZ","%Z",bad);// needs to be 2,3 or 4 alpha chars
 err_chk("1KST","%Z",bad);// needs to be 2,3 or 4 alpha chars
 err_chk("A1ST","%Z",bad);// needs to be 2,3 or 4 alpha chars
 err_chk("AK1T","%Z",bad);// needs to be 2,3 or 4 alpha chars
 err_chk("AKS1","%Z",bad);// needs to be 2,3 or 4 alpha chars
 
 err_chk("2001-11-12 18:31:01.","%Y-%m-%e %H:%M:%S.%f",bad);// no digits in fractional seconds
 err_chk("2001-11-12 18:31:01.1x","%Y-%m-%e %H:%M:%S.%f",bad);// x is not a digit
 // err_chk("2001-11-12 18:31:01.123456789012345678901234567890","%Y-%m-%e %H:%M:%S.%f",bad);// too many significant figures to be round loop exact [ cannot do this test as strptime() returns valid - as it is valid]
 
 /* now check UTC_mktime() and UTC_sec_to_tm() [so uses mydate]*/
 err_UTC_chk("2001-11-12 18:31:01 -0500","%Y-%m-%d %H:%M:%S %z",-1); // -1 is isdst "unknown"
 err_UTC_chk("2001-11-12 18:31:01 -0500","%Y-%m-%d %H:%M:%S %z",0); // 0 is isdst "known, not DST"
 err_UTC_chk("2001-11-12 18:31:01 -0500","%Y-%m-%d %H:%M:%S %z",1); // 1 is isdst "known, in DST"
 
 err_UTC_chk("2001-11-12 18:31:01 +0000","%Y-%m-%d %H:%M:%S %z",-1); // -1 is isdst "unknown"
 err_UTC_chk("2001-11-12 18:31:01 +0000","%Y-%m-%d %H:%M:%S %z",0); // 0 is isdst "known, not DST"
 err_UTC_chk("2001-11-12 18:31:01 +0000","%Y-%m-%d %H:%M:%S %z",1); // 1 is isdst "known, in DST"
 
 err_UTC_chk("2001-11-12 18:31:01 +0500","%Y-%m-%d %H:%M:%S %z",-1); // -1 is isdst "unknown"
 err_UTC_chk("2001-11-12 18:31:01 +0500","%Y-%m-%d %H:%M:%S %z",0); // 0 is isdst "known, not DST"
 err_UTC_chk("2001-11-12 18:31:01 +0500","%Y-%m-%d %H:%M:%S %z",1); // 1 is isdst "known, in DST"  

 // check round loop correct for a wide range of times (stepping 1 day at a time) - also check weekdays change in the correct pattern
 int prev_day_of_week,day_of_week;
 time_t s=0,s1;// "1970-01-01 00:00:00" = 0 secs past epoch
 sec_to_tm(s,& tm );
 prev_day_of_week=tm.tm_wday;
 printf("checking days of week increment correctly:\n");
 time_t ya_mktime_tm(const struct tm *tp); /* version of mktime() that returns secs and takes (but does not change) timeptr */
 s1=ya_mktime_tm(&tm); // convert back to secs (so s1 should = s)
 if(s1!=s)
   	{++errs;
   	 red_text();
   	 printf ("Error: not round=loop correct started with secs=%.0f ended up with secs=%.0f - ",
			(double)s,(double)s1);
   	 display_tm();
   	 printf("\n");
   	 normal_text();
   	}  
 // now check weekday by stepping day in year by 1
 for(int i=0;i<365*10000;++i) // 10,000 years forward
 	{ nos_tests++;
	  s+=3600*24; // add 1 day
 	  sec_to_tm(s,& tm );
 	  day_of_week=tm.tm_wday;
 	  if((prev_day_of_week+1)%7 !=day_of_week)
 	  	{++errs;
 	  	 red_text();
 	  	 printf ("Error: days of week did not increment correctly at secs=%.0f prev day=%d new day=%d\n",(double)s,prev_day_of_week,day_of_week);
 	  	 normal_text();
 	  	}
 	  s1=ya_mktime_tm(&tm); // convert back to secs (so s1 should = s)
 	  if(s1!=s)
 	  	{++errs;
 	  	 red_text();
 	  	 printf ("Error: not round=loop correct started with secs=%.0f ended up with secs=%.0f - ",
			(double)s,(double)s1);
 	  	 display_tm();
 	  	 printf("\n");
 	  	 normal_text();
 	  	} 	 	  	
 	 prev_day_of_week=day_of_week;
 	}
 s=0;// "1970-01-01 00:00:00" = 0 secs past epoch
 int pdoy; // previous day of year
 sec_to_tm(s,& tm );
 prev_day_of_week=tm.tm_wday;
 pdoy=tm.tm_yday;
 printf("checking days of week decrement correctly:\n");
 for(int i=0;i<365*10000;++i) // 10,000 years backwards
 	{ 
	  nos_tests++;
	  s-=3600*24; // subtract 1 day
 	  sec_to_tm(s,& tm );
 	  day_of_week=tm.tm_wday;
 	  if((prev_day_of_week-1<0?6:prev_day_of_week-1)!=day_of_week)
 	  	{++errs;
 	  	 red_text();
 	  	 printf ("Error: days of week did not decrement correctly at secs=%.0f prev day=%d, previous day of year=%d new day=%d new day of year=%d - ",
			(double)s,prev_day_of_week,pdoy-1<0?(is_leap((int64_t)tm.tm_year+1900)?365:364):pdoy-1,day_of_week,tm.tm_yday);
 	  	 display_tm();
 	  	 printf("\n");
 	  	 normal_text();
 	  	}
 	  s1=ya_mktime_tm(&tm); // convert back to secs (so s1 should = s)
 	  if(s1!=s)
 	  	{++errs;
 	  	 red_text();
 	  	 printf ("Error: not round=loop correct started with secs=%.0f ended up with secs=%.0f - ",
			(double)s,(double)s1);
 	  	 display_tm();
 	  	 printf("\n");
 	  	 normal_text();
 	  	} 	 
 	 prev_day_of_week=day_of_week;
 	 pdoy=tm.tm_yday;
 	} 	
 if(errs)
 	printf("\n%u tests conducted, %u error(s) found\n",nos_tests,errs);
 else
	printf("\n%u tests conducted, no errors found\n",nos_tests);
#ifdef __BORLANDC__
  printf("Press <return> to finish\n");
  getchar();
#endif
 exit(1);
}

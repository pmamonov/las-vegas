#include "time.h"

static unsigned char DpM[]={31,28,31,30,31,30,31,31,30,31,30,31};

unsigned int calc_leap(year){
  return (((year%4==0 && year%100!=0) || year%400==0) ? 1 : 0);
}

void stamp2date(time_t epoch, struct tm *date){
//  time_t epoch = *stamp; // [sec]
  unsigned int leap;
  date->sec = epoch % 60;
  epoch /= 60; // [min]
  date->min = epoch % 60;
  epoch /= 60; // [hours]
  date->hour = epoch % 24;
  epoch /= 24; // [days]
  date->year = 0;
  leap = 1; // year 2k
  while(epoch >= 365+leap) {
    epoch -= 365+leap;
    date->year++;
    leap = calc_leap(date->year);
  }
  date->year += 2000;
  date->mon = 0;
  while ( epoch >= ((date->mon == 1) ? DpM[date->mon]+leap : DpM[date->mon]) ){
    epoch -= ((date->mon == 1) ? DpM[date->mon]+leap : DpM[date->mon]);
    date->mon++;
  }
  date->mon++;
  date->day = epoch+1;
}

void date2stamp(struct tm *date, volatile time_t *epoch){
  time_t stamp;
  unsigned int i;
  unsigned int leap;
  if (date->year < 2000) date->year = 2000;
  if (date->mon < 1) date->mon = 1;
  if (date->mon > 12) date->mon = 12;
  if (date->hour < 0) date->hour = 0;
  if (date->hour > 23) date->hour = 23;
  if (date->min < 0) date->min = 0;
  if (date->min > 59) date->min = 59;
  if (date->sec < 0) date->sec = 0;
  if (date->sec > 59) date->sec = 59;
  stamp = 0;
  for (i=2000; i < date->year; i++)
    stamp += 86400L*(365 + calc_leap(i));
  leap = calc_leap(i);
  for (i=0; i < (date->mon-1); i++)
    stamp += 86400L*((i==1) ? DpM[i] + calc_leap(date->year) : DpM[i]); 
  if (date->day < 1) date->day = 1;
  if (date->day > ((i==1) ? DpM[i] + calc_leap(date->year) : DpM[i]))
    date->day = ((i==1) ? DpM[i] + calc_leap(date->year) : DpM[i]);
  stamp +=  date->sec + date->min*60L + date->hour*3600L + (date->day-1)*86400L;
  *epoch=stamp;
}


#include <stdint.h>

typedef unsigned long int time_t;

struct tm {
  uint8_t sec;
  uint8_t min;
  uint8_t hour;
  uint8_t day;
  uint8_t mon;
  int  year;
};

void date2stamp(struct tm *date, volatile time_t *epoch);
void stamp2date(time_t epoch, struct tm *date);

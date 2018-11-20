typedef unsigned long int time_t;

struct tm {
  char sec;
  char min;
  char hour;
  char day;
  char mon;
  int  year;
};

void date2stamp(struct tm *date, volatile time_t *epoch);
void stamp2date(time_t epoch, struct tm *date);

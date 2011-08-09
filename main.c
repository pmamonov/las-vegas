#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lcd.h"
#include "menu.h"
#include "time.h"

extern uint8_t __stack;

time_t time;

volatile uint16_t status;
#define RUN 0
#define UPD_SCRN 1

//general purpose timers timers
#define NTIMERS 8
volatile uint16_t timer[NTIMERS];

ISR(TIMER1_OVF_vect) {
	static int i;
	TCNT1=61536; // overflow in 1 ms
	for (i=0; i<NTIMERS; i++)	if (timer[i]) timer[i]--;
//timer 0 is for time
	if (!timer[0]) {timer[0]=1000; time++; status|=1<<UPD_SCRN;}
}

void print_menu(char *s, stMenuItem *p){
	stMenuItem *_p;
	s[0]='>';
	s[1]=0;
	_p=p;
	do{
		snprintf(&s[strlen(s)], 33-strlen(s), "%s ", _p->text);
		_p=_p->typ & (1<<RIGHT) ? NULL : _p->right;
	} while (_p && _p!=p);
}

void startstop(stMenuItem *p){
	(status&(1<<RUN)) ? (status&=~(1<<RUN)) : (status|=1<<RUN);
	if (status&(1<<RUN)) snprintf(p->text, 6, "%s", "STOP");
	else snprintf(p->text, 6, "%s", "START");
}

void incsec(stMenuItem *p){	time++;}
void decsec(stMenuItem *p){	time--;}
void incmin(stMenuItem *p){	time+=60;}
void decmin(stMenuItem *p){	time-=60;}
void inchour(stMenuItem *p){	time+=3600;}
void dechour(stMenuItem *p){	time-=3600;}
void incday(stMenuItem *p){	time+=86400;}
void decday(stMenuItem *p){	time-=86400;}
void incmon(stMenuItem *p){struct tm d; stamp2date(time, &d); d.mon=d.mon<12?d.mon+1:d.mon; date2stamp(&d, &time);}
void decmon(stMenuItem *p){struct tm d; stamp2date(time, &d); d.mon=d.mon>1?d.mon-1:d.mon; date2stamp(&d, &time);}
void incyear(stMenuItem *p){struct tm d; stamp2date(time, &d); d.year+=1; date2stamp(&d, &time);}
void decyear(stMenuItem *p){struct tm d; stamp2date(time, &d); d.year-=1; date2stamp(&d, &time);}

/*
void nextch(void){
	snprintf(sBanner, 33, "%x: %c", x, (char)x++);

}
void prevch(void){
	snprintf(sBanner, 33, "%x: %c", x, (char)x--);
}*/

void main(void) {
	char sDisp[33]; // display buffer
	char sBanner[33];
	char sStatus[6];

	char sSec[5];
	char sMin[5];
	char sHour[5];
	char sDay[5];
	char sMonth[5];
	char sYear[7];

	struct tm date;

	time=0;
//allocate timers
	timer[0]=0;
	timer[1]=0;//kbd processing delay

	//menu
	stMenuItem mRoot, mSS, mSetProg, mSetTime,\
		mSetYear, mSetMonth, mSetDay, mSetHour, mSetMin, mSetSec,\ 
		mSetYear1,mSetMonth1,mSetDay1,mSetHour1,mSetMin1, mSetSec1;

	mRoot=(stMenuItem){NULL,&mSS,NULL,NULL,0, sBanner};

	mSS=(stMenuItem){&mRoot,startstop,&mSetTime,&mSetProg,1<<DOWN,sStatus}; 
	mSetProg=(stMenuItem){&mRoot,NULL,&mSS,&mSetTime,0,"SETUP"}; 
	mSetTime=(stMenuItem){&mRoot,&mSetDay,&mSetProg,&mSS,0,"TIME"};
	
	mSetDay=(stMenuItem){&mSetTime,&mSetDay1,NULL,&mSetMonth,0,&sDay[2]};
	mSetMonth=(stMenuItem){&mSetTime,&mSetMonth1,&mSetDay,&mSetYear,0,&sMonth[2]};
	mSetYear=(stMenuItem){&mSetTime,&mSetYear1,&mSetMonth,&mSetHour,0,&sYear[2]};
	mSetHour=(stMenuItem){&mSetTime,&mSetHour1,&mSetDay,&mSetMin,0,&sHour[2]};
	mSetMin=(stMenuItem){&mSetTime,&mSetMin1,&mSetHour,&mSetSec,0,&sMin[1]};
	mSetSec=(stMenuItem){&mSetTime,&mSetSec1,&mSetMin,NULL,0,&sSec[1]};

	mSetSec1=(stMenuItem){&mSetSec,NULL,decsec, incsec, (1<<LEFT)|(1<<RIGHT), sSec};
	mSetMin1=(stMenuItem){&mSetMin,NULL,decmin, incmin, (1<<LEFT)|(1<<RIGHT), sMin};
	mSetHour1=(stMenuItem){&mSetHour,NULL,dechour, inchour, (1<<LEFT)|(1<<RIGHT), sHour};
	mSetDay1=(stMenuItem){&mSetDay,NULL,decday, incday, (1<<LEFT)|(1<<RIGHT), sDay};
	mSetMonth1=(stMenuItem){&mSetMonth,NULL,decmon, incmon, (1<<LEFT)|(1<<RIGHT), sMonth};
	mSetYear1=(stMenuItem){&mSetYear,NULL,decyear, incyear, (1<<LEFT)|(1<<RIGHT), sYear};

	stMenuItem *p = &mRoot;

	status=1; startstop(&mSS);

	//keyboard setup
	unsigned char kbd, kbd0, but;
	PORTB=0xf0; // enable pullups
	kbd=0; kbd0=1; but=UP;

	//timer
	TCCR1B|=1<<CS10;//clk
	TIMSK|=1<<TOIE1;
	TCNT1 = 0xffff;
  
	lcd_Init();

	sei();

  while (1){
		kbd=PINB&0xf0;
		if ((kbd^kbd0) && !timer[1]) {
  		timer[1]=250;
			if (~kbd&(kbd^kbd0)){
			if (~kbd&0x10) but=LEFT;
			if (~kbd&0x20) but=UP;
			if (~kbd&0x80) but=RIGHT;
			if (~kbd&0x40) but=DOWN;
			p=processButton(p, but);
			status|=1<<UPD_SCRN;
			}
		}
		kbd0=kbd;

		if (status & (1<<UPD_SCRN)){
			status &= ~(1<<UPD_SCRN);
			//update time
			stamp2date(time, &date);
			snprintf(sSec, 5, "s:%02d", date.sec);
			snprintf(sMin, 5, "m:%02d", date.min);
			snprintf(sHour, 5, "h:%02d", date.hour);
			snprintf(sDay, 5, "D:%02d", date.day);
			snprintf(sMonth, 5, "M:%02d", date.mon);
			snprintf(sYear, 7, "Y:%4d", date.year);
			snprintf(sBanner, 33, "%2s.%2s.%2s %2s:%2s",\
				&sDay[2], &sMonth[2], &sYear[4], &sHour[2], &sMin[2]);
			print_menu(sDisp, p);
			lcd_Clear();
	  	lcd_Print(sDisp);
		}

	}
}

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lcd.h"
#include "menu.h"
#include "time.h"

#define NPWM 4

extern uint8_t __stack;

typedef struct _stStep{
	void *next;
	uint32_t duration;
	uint8_t pwm_val[NPWM];
} stStep;

stStep newStep={NULL, 3600};
stStep step0={NULL,0};
char sStep[16];

// menu declaration
stMenuItem mRoot, mSS, mSetProg, mSetTime,\
		mSetYear, mSetMonth, mSetDay, mSetHour, mSetMin, mSetSec,\
		mSetYear1,mSetMonth1,mSetDay1,mSetHour1,mSetMin1, mSetSec1,\
		mNewStep, mNewStepT, mNewAdd, mSetVal1, mShowStep;


time_t time;

volatile uint16_t status;
#define RUN 0
#define UPD_SCRN 1

//general purpose timers
#define NTIMERS 8
volatile uint16_t timer[NTIMERS];

ISR(TIMER1_OVF_vect){
	static int i;
	TCNT1=53536; // overflow in 1 ms
	for (i=0; i<NTIMERS; i++)	if (timer[i]) timer[i]--;
//timer 0 is for time
	if (!timer[0]) {timer[0]=1000; time++; status|=1<<UPD_SCRN;}
}

volatile uint8_t pwm_cnt;
unsigned char pwm_val[NPWM];
uint8_t *pwm_port[NPWM]={&PORTD, &PORTD, &PORTD, &PORTD};
uint8_t *pwm_ddr[NPWM]={&DDRD,&DDRD,&DDRD,&DDRD};
uint8_t pwm_bit[NPWM]={0, 1, 3, 5};

ISR(TIMER0_OVF_vect){
	TCNT0=156;
	static unsigned char i;
	pwm_cnt++;
	for (i=0; i<NPWM; i++){
		if (!pwm_cnt && pwm_val[i]) *pwm_port[i]|=1<<pwm_bit[i];
		if (pwm_cnt == pwm_val[i]) *pwm_port[i]&=~(1<<pwm_bit[i]);
	}
}

stMenuItem *goShowStep(stMenuItem *p){
  uint8_t l=0, n=*(uint8_t*)(p->text-1), i=0;
  stStep* s=&step0;
  while (i<n && s->next) {s=s->next; i++;}
  snprintf(sStep,sizeof(sStep),"%d:%d",i, s->duration); 
  for (i=0; i<NPWM; i++){
    l=strlen(sStep);
    snprintf(&sStep[l],sizeof(sStep)-l," %d",s->pwm_val[i]);
  }
  return &mShowStep;
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

stMenuItem *startstop(stMenuItem *p){
	(status&(1<<RUN)) ? (status&=~(1<<RUN)) : (status|=1<<RUN);
	if (status&(1<<RUN)) snprintf(p->text, 6, "%s", "STOP");
	else snprintf(p->text, 6, "%s", "START");
	return p;
}

stMenuItem *incsec(stMenuItem *p){	time++; return p;}
stMenuItem *decsec(stMenuItem *p){	time--;return p;}
stMenuItem *incmin(stMenuItem *p){	time+=60;return p;}
stMenuItem *decmin(stMenuItem *p){	time-=60;return p;}
stMenuItem *inchour(stMenuItem *p){	time+=3600;return p;}
stMenuItem *dechour(stMenuItem *p){	time-=3600;return p;}
stMenuItem *incday(stMenuItem *p){	time+=86400;return p;}
stMenuItem *decday(stMenuItem *p){	time-=86400;return p;}
stMenuItem *incmon(stMenuItem *p){struct tm d; stamp2date(time, &d); d.mon=d.mon<12?d.mon+1:d.mon; date2stamp(&d, &time);return p;}
stMenuItem *decmon(stMenuItem *p){struct tm d; stamp2date(time, &d); d.mon=d.mon>1?d.mon-1:d.mon; date2stamp(&d, &time);return p;}
stMenuItem *incyear(stMenuItem *p){struct tm d; stamp2date(time, &d); d.year+=1; date2stamp(&d, &time);return p;}
stMenuItem *decyear(stMenuItem *p){struct tm d; stamp2date(time, &d); d.year-=1; date2stamp(&d, &time);return p;}

stMenuItem *goSetVal1(stMenuItem *p){
	mSetVal1.text = p->text-3;
	return &mSetVal1;
}

stMenuItem *addStep(stMenuItem *p){
	stStep *st=&step0;
  stMenuItem *m;
  char *s;
  uint8_t ist=0;

	while (st->next) {st=st->next; ist++;}
  ist++;
	st->next=malloc(sizeof(stStep));
	memcpy(st->next, &newStep, sizeof(stStep));

  m=&mNewStep;
  while (m->right) m=m->right;
  m->right=malloc(sizeof(stMenuItem));
  s=malloc(4);
  snprintf(&s[1], 3, "%02d", ist);
  *(uint8_t*)s=ist;
  *(stMenuItem*)m->right=(stMenuItem){&mSetProg,goShowStep,m,NULL,1<<DOWN,&s[1]};

	return p->up;
}

stMenuItem *incval(stMenuItem *p){
	uint8_t i = *(uint8_t*)(p->text-1);
	snprintf(p->text, 7,"%2d:%03d", i, ++newStep.pwm_val[i]);
	return p;
}

stMenuItem *decval(stMenuItem *p){
	uint8_t i = *(uint8_t*)(p->text-1);
	snprintf(p->text, 7,"%2d:%03d", i, --newStep.pwm_val[i]);
	return p;
}


/*
void nextch(void){
	snprintf(sBanner, 33, "%x: %c", x, (char)x++);

}
void prevch(void){
	snprintf(sBanner, 33, "%x: %c", x, (char)x--);
}*/

void main(void) {
	int i;
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

	//menu setup

	mRoot=(stMenuItem){NULL,&mSS,NULL,NULL,0, sBanner};

	mSS=(stMenuItem){&mRoot,startstop,&mSetTime,&mSetProg,1<<DOWN,sStatus}; 
	mSetProg=(stMenuItem){&mRoot,&mNewStep,&mSS,&mSetTime,0,"PROG"}; 
	mSetTime=(stMenuItem){&mRoot,&mSetDay,&mSetProg,&mSS,0,"TIME"};
	
	mSetDay=(stMenuItem){&mSetTime,&mSetDay1,NULL,&mSetMonth,0,&sDay[2]};
	mSetMonth=(stMenuItem){&mSetTime,&mSetMonth1,&mSetDay,&mSetYear,0,&sMonth[2]};
	mSetYear=(stMenuItem){&mSetTime,&mSetYear1,&mSetMonth,&mSetHour,0,&sYear[2]};
	mSetHour=(stMenuItem){&mSetTime,&mSetHour1,&mSetYear,&mSetMin,0,&sHour[2]};
	mSetMin=(stMenuItem){&mSetTime,&mSetMin1,&mSetHour,&mSetSec,0,&sMin[1]};
	mSetSec=(stMenuItem){&mSetTime,&mSetSec1,&mSetMin,NULL,0,&sSec[1]};

	mSetSec1=(stMenuItem){&mSetSec,NULL,decsec, incsec, (1<<LEFT)|(1<<RIGHT), sSec};
	mSetMin1=(stMenuItem){&mSetMin,NULL,decmin, incmin, (1<<LEFT)|(1<<RIGHT), sMin};
	mSetHour1=(stMenuItem){&mSetHour,NULL,dechour, inchour, (1<<LEFT)|(1<<RIGHT), sHour};
	mSetDay1=(stMenuItem){&mSetDay,NULL,decday, incday, (1<<LEFT)|(1<<RIGHT), sDay};
	mSetMonth1=(stMenuItem){&mSetMonth,NULL,decmon, incmon, (1<<LEFT)|(1<<RIGHT), sMonth};
	mSetYear1=(stMenuItem){&mSetYear,NULL,decyear, incyear, (1<<LEFT)|(1<<RIGHT), sYear};

	mNewStep=(stMenuItem){&mSetProg,&mNewStepT,NULL,NULL,0, "NEW"};
	mNewStepT=(stMenuItem){&mNewStep,NULL,NULL,NULL,0, "T"};

	stMenuItem *mSetVal, *mSetValLeft;

	//add menus for pwm fill factor setup
	mSetValLeft=&mNewStepT;
	for (i=0; i<NPWM; i++){
//		while (mSetValLeft->right) mSetValLeft=mSetValLeft->right;
		mSetVal=malloc(sizeof(stMenuItem));
		char* sVal=malloc(8);
		*(uint8_t *)sVal=i;
		snprintf(&sVal[1], 7,"%2d:%03d", i, newStep.pwm_val[i]);
		*mSetVal=(stMenuItem){&mNewStep,goSetVal1,mSetValLeft,NULL,1<<DOWN,&sVal[4]};
		mSetValLeft->right=mSetVal;
		mSetValLeft=mSetVal;
	}
	mSetVal->right=&mNewAdd;
	mNewAdd=(stMenuItem){&mNewStep,addStep,mSetVal,NULL,1<<DOWN, "ADD"};
	mSetVal1=(stMenuItem){&mNewStepT, NULL, decval, incval, (1<<LEFT)|(1<<RIGHT), NULL};
  mShowStep=(stMenuItem){&mNewStep, NULL, NULL, NULL, 0, sStep};

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

	//  software pwm
	TCCR0|=1<<CS00;
	TIMSK|=1<<TOIE0;
	for (i=0; i<NPWM; i++){
		pwm_ddr[i] = 1<<pwm_bit[i];
		pwm_val[i]=0x7f;
	}

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

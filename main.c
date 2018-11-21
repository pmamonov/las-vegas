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
	time_t duration;
	uint8_t pwm_val[NPWM];
} stStep;

int curstep;

stStep newStep={NULL, 3600UL};
stStep step0={NULL,0};
char sStep[33];
char sStatus[17];
char sMemFree[8];
uint16_t *pmem;

// menu declaration
volatile stMenuItem mRoot, mSS, mSetProg, mSetTime,\
		mSetYear, mSetMonth, mSetDay, mSetHour, mSetMin, mSetSec,\
		mSetYear1,mSetMonth1,mSetDay1,mSetHour1,mSetMin1, mSetSec1,\
		mNewStep, mNewStepT, mNewAdd, mSetVal1, mShowStep, 
		mNewStepTday, mNewStepThour, mNewStepTmin, mNewStepTsec,\
		mNewStepTday1, mNewStepThour1, mNewStepTmin1, mNewStepTsec1, mMemFree, mMemFree1;

time_t time, start;

volatile uint16_t status;
#define RUN 0
#define UPD_SCRN 1
#define UPD_TIM 2

//general purpose timers
#define NTIMERS 4
volatile uint16_t timer[NTIMERS];

ISR(TIMER1_OVF_vect){
	static int i;
	TCNT1=63536; // overflow in 1 ms
	for (i=0; i<NTIMERS; i++)	if (timer[i]) timer[i]--;
//timer 0 is for time
	if (!timer[0]) {timer[0]=1000; time++; status|=1<<UPD_TIM;}
}

volatile uint8_t pwm_cnt;
unsigned char pwm_val[NPWM];
uint8_t *pwm_port[NPWM]={&PORTB, &PORTB, &PORTB, &PORTB};
uint8_t *pwm_ddr[NPWM]={&DDRB,&DDRB,&DDRB,&DDRB};
uint8_t pwm_bit[NPWM]={5, 4, 3, 2};

ISR(TIMER0_OVF_vect){
	TCNT0 = 256 - 78;
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
  snprintf(sStep,sizeof(sStep),"DEL? %hd:%ld-%02ld:%02ld:%02ld",\
    i, s->duration/86400UL, s->duration%86400UL/3600UL, s->duration%3600/60, s->duration%60); 
  for (i=0; i<NPWM; i++){
    l=strlen(sStep);
    snprintf(&sStep[l],sizeof(sStep)-l," %d",s->pwm_val[i]);
  }
	mShowStep.up=p;
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
	int i;
	(status&(1<<RUN)) ? (status&=~(1<<RUN)) : (status|=1<<RUN);
	if (status&(1<<RUN)){ 
		start=time;
		curstep=0;
		for (i=0; i<NPWM; i++) pwm_val[i] = 0;
		TIMSK0|=1<<TOIE0;
		snprintf(p->text, 6, "%s", "STOP");
	}
	else{
		snprintf(p->text, 6, "%s", "START");
    snprintf(sStatus, sizeof(sStatus), "STOPPED");
		TIMSK0 &= ~(1<<TOIE0);
		for (i=0; i<NPWM; i++) *pwm_port[i] &= ~(1<<pwm_bit[i]);
	}
	return p;
}

char sStepDay[5], sStepHour[5], sStepMin[5], sStepSec[5];

void updStTs(void){
//	struct tm d;
//	stamp2date(newStep.duration, &d);
	snprintf(sStepDay, sizeof(sStepDay), "d:%02d",newStep.duration/86400UL);
	snprintf(sStepHour, sizeof(sStepDay), "h:%02d",newStep.duration%86400UL/3600);
	snprintf(sStepMin, sizeof(sStepDay), "m:%02d",newStep.duration%3600/60);
	snprintf(sStepSec, sizeof(sStepDay), "s:%02d",newStep.duration%60);
}

stMenuItem *incStSec(stMenuItem *p){	newStep.duration++;  updStTs(); return p;}
stMenuItem *decStSec(stMenuItem *p){	newStep.duration--; updStTs(); return p;}
stMenuItem *incStMin(stMenuItem *p){	newStep.duration+=60; updStTs(); return p;}
stMenuItem *decStMin(stMenuItem *p){	newStep.duration-=60; updStTs(); return p;}
stMenuItem *incStHour(stMenuItem *p){	newStep.duration+=3600; updStTs(); return p;}
stMenuItem *decStHour(stMenuItem *p){	newStep.duration-=3600; updStTs(); return p;}
stMenuItem *incStDay(stMenuItem *p){	newStep.duration+=86400UL; updStTs(); return p;}
stMenuItem *decStDay(stMenuItem *p){	newStep.duration-=86400UL; updStTs(); return p;}

stMenuItem *incsec(stMenuItem *p){	time++; return p;}
stMenuItem *decsec(stMenuItem *p){	time--;return p;}
stMenuItem *incmin(stMenuItem *p){	time+=60;return p;}
stMenuItem *decmin(stMenuItem *p){	time-=60;return p;}
stMenuItem *inchour(stMenuItem *p){	time+=3600;return p;}
stMenuItem *dechour(stMenuItem *p){	time-=3600;return p;}
stMenuItem *incday(stMenuItem *p){	time+=86400UL;return p;}
stMenuItem *decday(stMenuItem *p){	time-=86400UL;return p;}
stMenuItem *incmon(stMenuItem *p){struct tm d; stamp2date(time, &d); d.mon=d.mon<12?d.mon+1:d.mon; date2stamp(&d, &time);return p;}
stMenuItem *decmon(stMenuItem *p){struct tm d; stamp2date(time, &d); d.mon=d.mon>1?d.mon-1:d.mon; date2stamp(&d, &time);return p;}
stMenuItem *incyear(stMenuItem *p){struct tm d; stamp2date(time, &d); d.year+=1; date2stamp(&d, &time);return p;}
stMenuItem *decyear(stMenuItem *p){struct tm d; stamp2date(time, &d); d.year-=1; date2stamp(&d, &time);return p;}

stMenuItem *goSetVal1(stMenuItem *p){
	mSetVal1.text = p->text-3;
	return &mSetVal1;
}

stMenuItem *goMemFree1(stMenuItem *p){
	int sz=0;
	int i=RAMEND/2;
	for (; i; i--){
		if (pmem[i]==0xbeef) sz++;
	}
	snprintf(sMemFree, sizeof(sMemFree), "%d", sz);
	return &mMemFree1;
}

stMenuItem *addStep(stMenuItem *p){
	stStep *st=&step0;
  stMenuItem *m;
  char *s;
  uint8_t ist=0;
//add elem into sequence of steps
	while (st->next) {st=st->next; ist++;}
  ist++;
	st->next=malloc(sizeof(stStep));
	if (!st->next) lcd_Print("OUT OF MEM");
	memcpy(st->next, &newStep, sizeof(stStep));
// add corresponding menu item
  m=&mNewStep;
  while (m->right) m=m->right;
  m->right=malloc(sizeof(stMenuItem));
	if (!m->right) lcd_Print("OUT OF MEM");
  s=malloc(4);
	if (!s) lcd_Print("OUT OF MEM");
  snprintf(&s[1], 3, "%02d", ist);
  *(uint8_t*)s=ist;
  *(stMenuItem*)m->right=(stMenuItem){&mSetProg,goShowStep,m,NULL,1<<DOWN,&s[1]};

	return p->up;
}

stMenuItem *delStep(stMenuItem *p){
	uint8_t n = *(uint8_t*)(((stMenuItem*)p->up)->text-1), i=0;
	//del step from sequence
	stStep *prev, *this;
	this=&step0;
  while (i<n-1 && this->next) {
		this=this->next; 
		i++;
	}
	prev=this;
	this=this->next;
	prev->next=this->next;
	free(this);
//delete menu item
	stMenuItem *m=&mNewStep;
	i=0;
	m=((stMenuItem*)p->up)->right;
	while (m) { // update labels and IDs in right part of menu sequence
		snprintf(m->text, 3, "%02d", --*(uint8_t*)(m->text-1));
		m=m->right;
	}
	m=p->up;
	((stMenuItem*)m->left)->right=m->right; // adjust pointers in menu sequence
	if (m->right) ((stMenuItem*)m->right)->left=m->left;
	free(m->text-1);//del string
	free(m);
	return &mNewStep;
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
  char sSS[6];

	char sSec[5];
	char sMin[5];
	char sHour[5];
	char sDay[5];
	char sMonth[5];
	char sYear[7];

	struct tm date;
	time_t t;
	stStep *s;
	unsigned char kbd, kbd0, but;

/*	pmem=0; i=200;
	while (!pmem) pmem=malloc(2*i--);
	for (;i;i--) pmem[i]=0xbeef;
	free(pmem);*/

	time=0;
//allocate timers
	timer[0]=0;
	timer[1]=0;//kbd processing delay

	//menu setup

	mRoot=(stMenuItem){NULL,&mSS,&mMemFree,NULL,0, sBanner};

	mSS=(stMenuItem){&mRoot,startstop,&mSetTime,&mSetProg,1<<DOWN,sSS}; 
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
	mNewStepT=(stMenuItem){&mNewStep,&mNewStepTday,NULL,NULL,0, "T"};

	mNewStepTday	=(stMenuItem){&mNewStepT,&mNewStepTday1,NULL,&mNewStepThour,0, &sStepDay[2]};
	mNewStepThour	=(stMenuItem){&mNewStepT,&mNewStepThour1,&mNewStepTday,&mNewStepTmin,0, &sStepHour[2]};
	mNewStepTmin	=(stMenuItem){&mNewStepT,&mNewStepTmin1,&mNewStepThour,&mNewStepTsec,0, &sStepMin[1]};
	mNewStepTsec	=(stMenuItem){&mNewStepT,&mNewStepTsec1,&mNewStepTmin,NULL,0, &sStepSec[1]};

	mNewStepTday1	=(stMenuItem){&mNewStepTday,NULL,decStDay,incStDay,(1<<LEFT)|(1<<RIGHT), sStepDay};
	mNewStepThour1=(stMenuItem){&mNewStepThour,NULL,decStHour,incStHour,(1<<LEFT)|(1<<RIGHT), sStepHour};
	mNewStepTmin1	=(stMenuItem){&mNewStepTmin,NULL,decStMin,incStMin,(1<<LEFT)|(1<<RIGHT), sStepMin};
	mNewStepTsec1	=(stMenuItem){&mNewStepTsec,NULL,decStSec,incStSec,(1<<LEFT)|(1<<RIGHT), sStepSec};

	updStTs();

	stMenuItem *mSetVal, *mSetValLeft;

	//add menus for pwm fill factor setup
	mSetValLeft=&mNewStepT;
	for (i=0; i<NPWM; i++){
//		while (mSetValLeft->right) mSetValLeft=mSetValLeft->right;
		mSetVal=malloc(sizeof(stMenuItem));
		if (!mSetVal) lcd_Print("OUT OF MEM");
		char* sVal=malloc(8);
		if (!sVal) lcd_Print("OUT OF MEM");
		*(uint8_t *)sVal=i;
		snprintf(&sVal[1], 7,"%2d:%03d", i, newStep.pwm_val[i]);
		*mSetVal=(stMenuItem){&mNewStep,goSetVal1,mSetValLeft,NULL,1<<DOWN,&sVal[4]};
		mSetValLeft->right=mSetVal;
		mSetValLeft=mSetVal;
	}
	mSetVal->right=&mNewAdd;
	mNewAdd=(stMenuItem){&mNewStep,addStep,mSetVal,NULL,1<<DOWN, "ADD"};
	mSetVal1=(stMenuItem){&mNewStepT, NULL, decval, incval, (1<<LEFT)|(1<<RIGHT), NULL};
  mShowStep=(stMenuItem){&mNewStep, delStep, NULL, NULL, 1<<DOWN, sStep};
	mMemFree=(stMenuItem){NULL,goMemFree1,&mRoot,NULL,1<<DOWN,"MemFree"};
	mMemFree1=(stMenuItem){&mMemFree,NULL,NULL,NULL,0,sMemFree};

	stMenuItem *p = &mRoot;

	status=1; startstop(&mSS);

	//keyboard setup
	PORTC=0xf; // enable pullups
	kbd=0; kbd0=0; but=UP;

	//timer
	TCCR1B|=2<<CS10;//clk/8
	TIMSK1|=1<<TOIE1;
	TCNT1 = 0xffff;

	//  software pwm
	TCCR0B|=2<<CS00;
//	TIMSK|=1<<TOIE0;
	for (i=0; i<NPWM; i++){
		*pwm_ddr[i] |= 1<<pwm_bit[i];
		pwm_val[i]=0x0;
	}

	lcd_Init();

	sei();

	status|=1<<UPD_SCRN;

  while (1){
		kbd=PINC&0xf;
		if ((kbd^kbd0) && !timer[1]){
  		timer[1]=100;
			if (~kbd&(kbd^kbd0)){
			if (~kbd&8) but=LEFT;
			if (~kbd&4) but=UP;
			if (~kbd&1) but=RIGHT;
			if (~kbd&2) but=DOWN;
			p=processButton(p, but);
			status|=1<<UPD_SCRN;
			}
		}
		kbd0=kbd;
		
		if (status & (1<<UPD_TIM)){
			status &= ~(1<<UPD_TIM);
			if (p==&mRoot || p==&mSetTime || p->up==&mSetTime || ((stMenuItem*)p->up)->up==&mSetTime) status |= 1<<UPD_SCRN;
      if (status&(1<<RUN)){
			s=&step0;
			t=start;
			i=0;
			while (i<curstep && s->next) {
				s=s->next;
				t+=s->duration;
				i++;
			}
			if (time>=t){
				if (s->next) {s=s->next; curstep++;}
				else {
          start=t;
          s=step0.next ? step0.next : &step0; 
          curstep=step0.next?1:0;
        }
				for (i=0; i<NPWM; i++) pwm_val[i]=s->pwm_val[i];
        snprintf(sStatus, sizeof(sStatus), "RUNNING(%02d)", curstep);
			}
      }
		}

		if (status & (1<<UPD_SCRN)){
			status &= ~(1<<UPD_SCRN);
			//update banner
			stamp2date(time, &date);
			snprintf(sSec, 5, "s:%02d", date.sec);
			snprintf(sMin, 5, "m:%02d", date.min);
			snprintf(sHour, 5, "h:%02d", date.hour);
			snprintf(sDay, 5, "D:%02d", date.day);
			snprintf(sMonth, 5, "M:%02d", date.mon);
			snprintf(sYear, 7, "Y:%4d", date.year);
			snprintf(sBanner, 33, "%2s.%2s.%2s %2s:%2s\n%s",\
				&sDay[2], &sMonth[2], &sYear[4], &sHour[2], &sMin[2], sStatus);
			print_menu(sDisp, p);
			lcd_Clear();
	  	lcd_Print(sDisp);
		}

	}
}

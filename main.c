#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdlib.h>
#include "lcd.h"
#include "menu.h"
#include <alloca.h>

volatile unsigned int tim;
volatile unsigned char status;

char sDisp[33]; // display buffer
char sStatus[6];
char sBanner[33];
int x;

extern uint8_t __stack;

ISR(TIMER1_OVF_vect) {
	TCNT1=65035; // 0xffff-1000_us/(8_clk/f_MHz);
//	tim ? tim-- : tim;
	if (tim) tim--;
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

void startstop(void){
	status = status ? 0 : 1;
	if (status) snprintf(sStatus, 6, "%s", "STOP");
	else snprintf(sStatus, 6, "%s", "START");
}

void nextch(void){
	snprintf(sBanner, 33, "%x: %c", x, (char)x++);

}
void prevch(void){
	snprintf(sBanner, 33, "%x: %c", x, (char)x--);
}

void main(void) {

	x=0; nextch();
//  x=2048;
//  void *ptr=alloca(1);
//  while (!(ptr=malloc(x))) x--;
//  free(ptr);
  
//  itoa(StackCount, sDisp, 10);


	//menu
	stMenuItem m00={NULL,NULL,NULL,NULL,0, sBanner}; m00.right=nextch; m00.typ|=1<<RIGHT;m00.left=prevch; m00.typ|=1<<LEFT;
	stMenuItem m10={NULL,NULL,NULL,NULL,0,sStatus}; m10.up=&m00;m00.down=&m10;m10.down=startstop;m10.typ|=1<<DOWN;
	stMenuItem m11={NULL,NULL,NULL,NULL,0,"SETUP"}; m11.left=&m10; m10.right=&m11; m11.up=&m00;
	stMenuItem m12={NULL,NULL,NULL,NULL,0,"TIME"}; m12.left=&m11;m11.right=&m12; m12.right=&m10;m10.left=&m12; m12.up=&m10;

	stMenuItem *p = &m00;

	status=1;startstop();

	//keyboard setup
	unsigned char kbd, kbd0, but;
	PORTB=0xf0; // enable pullups
	kbd=0; kbd0=1; but=UP;

	//timer
	TCCR1B|=2<<CS10;//clk/8
	TIMSK|=1<<TOIE1;
  
/*  TCCR0|=4<<CS00;
  TIMSK|=1<<TOIE0;*/
	tim=0;

	lcd_Init();

	sei();

  while (1){
		kbd=PINB&0xf0;
		if (~kbd&(kbd^kbd0) && !tim) {
  		tim=250;
			if (~kbd&0x10) but=LEFT;
			if (~kbd&0x20) but=UP;
			if (~kbd&0x80) but=RIGHT;
			if (~kbd&0x40) but=DOWN;
			p=processButton(p, but);
//			snprintf(sBanner, 33, "%u", (unsigned int)(&__stack));
			print_menu(sDisp, p);
			lcd_Clear();
	  	lcd_Print(sDisp);
		}
		kbd0=kbd;
	}
}

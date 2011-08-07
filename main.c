#include <stdio.h>
#include "lcd.h"
#include "menu.h"


void main(void) {
	stMenuItem m0={NULL,NULL,NULL,NULL,0,"MENU1"};
	stMenuItem m1={NULL,NULL,NULL,NULL,0,"MENU2"}; m1.left=&m0; m0.right=&m1; 
	stMenuItem m2={NULL,NULL,NULL,NULL,0,"MENU_last"}; m2.left=&m1; m1.right=&m2; m2.right=&m0; m0.left=&m2;

	stMenuItem *p = &m1;

	char s[33];
	s[0]=0;

	lcd_Init();

	stMenuItem *_p;
	s[0]='>';
	s[1]=0;
	_p=p;
	do{
		snprintf(&s[strlen(s)], 33-strlen(s), "%s ", _p->text);
		_p=_p->right;
	} while (_p!=p);

  lcd_Print(s);
  while (1);
}


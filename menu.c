#include "menu.h"

stMenuItem* processButton(stMenuItem* itm, unsigned char but){
	void *p;
	switch (but){
		case UP: p=itm->up; break;
		case DOWN: p=itm->down; break;
		case LEFT: p=itm->left; break;
		case RIGHT: p=itm->right; break;
	}
	if (p){
		if (itm->typ&(1<<but)) return ((stMenuItem* (*)(stMenuItem*))p)(itm);
		else return p;
	}
	return itm;
}

#define UP 0
#define DOWN 1
#define LEFT 2
#define RIGHT 3

typedef struct _stMenuItem{
	void *up;
	void *down;
	void *left;
	void *right;
	unsigned char typ;
	char *text;
} stMenuItem;

//void addMenuItem(stMenuItem* parent, unsigned char but, unsigned char typ, char *text);

stMenuItem* processButton(stMenuItem* menu, unsigned char but);

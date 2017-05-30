#include <inc/lib.h>

char c[125*1024*1024];

void
umain(int argc, char **argv)
{
	memset(c,0,sizeof c);
	int r=open("/new-file", O_RDWR|O_CREAT);
	for(int i = 0; i <= 5000; i++) {
		write(r, "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!", 100);
	}
}

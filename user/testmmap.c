#include <inc/lib.h>

char buf[PGSIZE] __attribute__((aligned(PGSIZE)));

void
umain(int argc, char **argv)
{
	int fd = open("/newmotd", O_RDWR);
	int r = mmap(0, buf, PGSIZE, fd, 0, PTE_W);
	if(r < 0) {
		cprintf("mmap error\n");
		return;
	}
	cprintf("In file newmotd: %s\n", buf);
	buf[0] = 'T';
	close(fd);
	fd = open("/newmotd", O_RDONLY);
	char ch;
	read(fd, &ch, 1);
	cprintf("%c\n", ch);
}

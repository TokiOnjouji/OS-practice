// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/pmap.h>
#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>

#define CMDBUF_SIZE	80	// enough for one VGA text line
int console_color;

struct Command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char** argv, struct Trapframe* tf);
};

static struct Command commands[] = {
	{ "help", "Display this list of commands", mon_help },
	{ "kerninfo", "Display information about the kernel", mon_kerninfo },
	{ "backtrace", "", mon_backtrace},
	{ "color", "", set_color},
	{ "showmappings", "", mon_showmappings},
	{ "setpermissions", "", mon_changepermissions},
	{ "clearpermissions", "", mon_changepermissions},
	{ "dumpmemory", "", mon_dumpmemory}
};

/***** Implementations of basic kernel monitor commands *****/

int getNum(char c) {
	if('0'<=c&&c<='9') return c-'0';
	if('a'<=c&&c<='f') return c-'a';
	if('A'<=c&&c<='F') return c-'A';
	return -1;
}

int
set_color(int argc, char **argv, struct Trapframe *tf) {
	console_color=0;
	if(argv[1][2]) return -1;
	for(int i=0;argv[1][i];i++) {
		int num=getNum(argv[1][i]);
		if(num<0) return -1;
		console_color=console_color*16+num;
	}
	return 0;
}

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(commands); i++)
		cprintf("%s - %s\n", commands[i].name, commands[i].desc);
	return 0;
}

int
mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
	extern char _start[], entry[], etext[], edata[], end[];

	cprintf("Special kernel symbols:\n");
	cprintf("  _start                  %08x (phys)\n", _start);
	cprintf("  entry  %08x (virt)  %08x (phys)\n", entry, entry - KERNBASE);
	cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
	cprintf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
	cprintf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
	cprintf("Kernel executable memory footprint: %dKB\n",
		ROUNDUP(end - entry, 1024) / 1024);
	return 0;
}

int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
	cprintf("Stack backtrace:\n");
	for(int ebp=read_ebp(); ebp; ebp=*((int*)ebp)) {
		cprintf("  ebp %08x  eip %08x  args ", ebp, *((int*)ebp+1));
		for(int i=2; i<=6; i++)
			cprintf(" %08x",*((int*)ebp+i));
		cprintf("\n");
		struct Eipdebuginfo eip;
		debuginfo_eip(*((int*)ebp+1), &eip);
		cprintf("         %s:%d: %.*s+%d\n",eip.eip_file,eip.eip_line,eip.eip_fn_namelen,eip.eip_fn_name,*((int*)ebp+1)-eip.eip_fn_addr);
	}
	return 0;
}

bool AddrToNum(char *str, unsigned int *res) {
	unsigned int x = 0;
	if(str[0] != '0' || str[1] != 'x') return 0;
	str = str + 2;
	for(int i = 0; str[i]; i++) {
		if(i >= 8) return 0;
		x = x*16;
		if('0'<=str[i]&&str[i]<='9')
			x += str[i]-'0';
		else if('A'<=str[i]&&str[i]<='F')
			x += str[i]-'A'+10;
		else if('a'<=str[i]&&str[i]<='f')
			x += str[i]-'a'+10;
		else
			return 0;
	}
	*res = x;
	return 1;
}

int
mon_showmappings(int argc, char **argv, struct Trapframe *tf)
{
	if(argc != 3) {
		cprintf("Usage: showmappings <start_addr> <end_addr>\n");
		cprintf("e.g.,  showmappings 0x3000 0x5000\n");
		return 0;
	}
	unsigned int down, up;
	if(!AddrToNum(argv[1], &down) || !AddrToNum(argv[2], &up)) {
		cprintf("Invalid virtual address, it should be a 32-bit hexadecimal number\n");
		return 0;
	}
	for(void* i = (void*)down; i <= (void*)up; i+=PGSIZE) {
		pte_t* pte = pgdir_walk(kern_pgdir, i, 0);
		if(!pte) cprintf("no mapping at 0x%x\n", ROUNDDOWN(i, PGSIZE));
		else {
			if((*pte)&PTE_P) {
				cprintf("VA 0x%x----PA 0x%x:", i, PTE_ADDR(*pte));
				if((*pte)&PTE_W) cprintf(" PTE_W");
				if((*pte)&PTE_U) cprintf(" PTE_U");
				cprintf("\n");
			}
			else {
				cprintf("no mapping at 0x%x\n", ROUNDDOWN(i, PGSIZE));
			}		
		}
	}
	return 0;
}

int
mon_changepermissions(int argc, char **argv, struct Trapframe *tf)
{
	if(argc != 3) {
		cprintf("Usage: <setpermissions/clearpermissions/changepermissions> <v_addr> <perm>");
		return 0;
	}
	if(strcmp(argv[0], "setpermissions") && strcmp(argv[0], "clearpermissions")) {
		cprintf("Usage: <setpermissions/clearpermissions/changepermissions> <v_addr> <perm>");
		return 0;	
	}
	unsigned int va;
	if(!AddrToNum(argv[1], &va)) {
		cprintf("Invalid virtual address, it should be a 32-bit hexadecimal number\n");
		return 0;
	}
	int perm = 0;
	if(!strcmp(argv[2], "PTE_W")) {
		perm = PTE_W;
	}
	else if(!strcmp(argv[2], "PTE_U")) {
		perm = PTE_U;
	}
	else if(!strcmp(argv[2], "PTE_P")){
		perm = PTE_P;
	}
	else {
		cprintf("Invalid permmision, it should be PTE_P, PTE_U or PTE_W\n");
		return 0;	
	}
	pte_t* pte = pgdir_walk(kern_pgdir, (void*)va, 0);
	if(!pte) {
		cprintf("no mapping at 0x%x\n", va);
		return 0;
	}
	else {
		if((*pte)&PTE_P) {
			if(!strcmp(argv[0], "setpermissions")) (*pte) |= perm;
			else (*pte) &= ~perm;
			cprintf("VA 0x%x----PA 0x%x:", va, PTE_ADDR(*pte));
			if((*pte)&PTE_P) cprintf(" PTE_P");
 			if((*pte)&PTE_W) cprintf(" PTE_W");
			if((*pte)&PTE_U) cprintf(" PTE_U");
			cprintf("\n");
		}
		else {
			cprintf("no mapping at 0x%x\n", va);
			return 0;
		}
	}
	return 0;
}

int
mon_dumpmemory(int argc, char **argv, struct Trapframe *tf) {
	if(argc != 4) {
		cprintf("Usage: dumpmemory <VA/PA> <start_addr> <end_addr>\n");
		return 0;	
	}
	if(strcmp(argv[1], "VA") && strcmp(argv[1], "PA")) {
		cprintf("Usage: dumpmemory <VA/PA> <start_addr> <end_addr>\n");
		return 0;	
	}
	unsigned int down, up;
	if(!AddrToNum(argv[2], &down) || !AddrToNum(argv[3], &up)) {
		cprintf("Invalid virtual address, it should be a 32-bit hexadecimal number\n");
		return 0;
	}
	for(unsigned int* i = (unsigned int*)down; i <= (unsigned int*)up; i++) {
		void* va = (void*)i;
		if(strcmp(argv[1], "VA")) {
			va = va + KERNBASE;
		}
		pte_t* pte = pgdir_walk(kern_pgdir, va, 0);
		if((!pte) || (!((*pte)&PTE_P)))
			cprintf("no mapping at 0x%x\n", va);
		else {
			if(!strcmp(argv[1], "VA"))
				cprintf("vaddr 0x%x: 0x%x\n", va, *((int*)va));
			else
				cprintf("paddr 0x%x: 0x%x\n", i, *((int*)va));
		}
	}
	return 0;
	
}

/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int
runcmd(char *buf, struct Trapframe *tf)
{
	int argc;
	char *argv[MAXARGS];
	int i;

	// Parse the command buffer into whitespace-separated arguments
	argc = 0;
	argv[argc] = 0;
	while (1) {
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS-1) {
			cprintf("Too many arguments (max %d)\n", MAXARGS);
			return 0;
		}
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the command
	if (argc == 0)
		return 0;
	for (i = 0; i < ARRAY_SIZE(commands); i++) {
		if (strcmp(argv[0], commands[i].name) == 0)
			return commands[i].func(argc, argv, tf);
	}
	cprintf("Unknown command '%s'\n", argv[0]);
	return 0;
}

void
monitor(struct Trapframe *tf)
{
	char *buf;

	cprintf("Welcome to the JOS kernel monitor!\n");
	cprintf("Type 'help' for a list of commands.\n");


	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}

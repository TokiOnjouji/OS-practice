#include <inc/lib.h>

#define MATN 4

/*
	calculate Matrix C = Matrix A x Matrix B
*/
struct Type {
	int data;
	int gap[PGSIZE-sizeof(int)];
};

struct Type in __attribute__((aligned(PGSIZE)));
struct Type out __attribute__((aligned(PGSIZE)));
struct Type out1 __attribute__((aligned(PGSIZE)));

void EAST() {
	for(int i = 0; i < MATN; i++) {
		int r = ipc_recv(0, 0, 0);
		/*
			do nothing
		*/
	}
}

void WEST(int to, int buf[]) {
	for(int i = 0; i < MATN; i++) {
		out.data = 1;
		ipc_send(to, buf[i], &out, PTE_U|PTE_P);
	}
}

void CENTER(int east, int south, int Aij) {
	int q1[MATN], q2[MATN];
	int h1 = 0, t1 = 0, h2 = 0, t2 = 0;
	int envid;
	for(int i = 0; i < MATN; i++) {
		while(h1 >= t1) {
			int r = ipc_recv(&envid, &in, 0);
			if(in.data == 1) {
				q1[t1++] = r;
			}
			if(in.data == 0) {
				q2[t2++] = r;			
			}
		}
		int x = q1[h1++];
		out.data = 1;
		ipc_send(east, x, &out, PTE_U|PTE_P);
		while(h2 >= t2) {
			int r = ipc_recv(&envid, &in, 0);
			if(in.data == 1) {
				q1[t1++] = r;
			}
			if(in.data == 0) {
				q2[t2++] = r;			
			}
		}
		int sum = q2[h2++];
		sum = sum + Aij*x;
		out1.data = 0;
		ipc_send(south, sum, &out1, PTE_U|PTE_P);
	}
}

void NORTH(int to) {
	for(int i = 0; i < MATN; i++) {
		out.data = 0;
		ipc_send(to, 0, &out, PTE_U|PTE_P);	
	}
}

void SOUTH(int col) {
	for(int row = 1; row <= MATN; row++) {
		int r = ipc_recv(0, 0, 0);
		cprintf("In Matrix C = A x B, C[%d][%d] = %d\n", row, col, r);
	}
}

void
umain(int argc, char **argv)
{
	int A[MATN][MATN]={
		{1,4,2,5},
		{6,1,2,7},
		{4,2,1,7},
		{2,6,1,4}
	};
	int B[MATN][MATN]={
		{1,2,3,4},
		{2,3,4,5},
		{3,4,5,6},
		{4,5,6,7}
	};
	/*
		expected output:
		Matrix C[][]={
			{35,47,59,71},
			{42,58,74,90},
			{39,53,67,81},
			{33,46,59,72}			
		}
	*/
	for(int i = 0; i < MATN; i++)
		for(int j = i+1; j < MATN; j++) {
			A[i][j] ^= A[j][i];
			A[j][i] ^= A[i][j];
			A[i][j] ^= A[j][i];
		} //set A = A^T
	int ProcessID[MATN+2][MATN+2];
	int envid;
	for(int i = 1; i <= MATN; i++) {
		envid = fork();
		if(envid == 0) {
			EAST();	
			return;
		}
		ProcessID[i][MATN+1] = envid;
	} //fork EAST
	for(int i = 1; i <= MATN; i++) {
		envid = fork();
		if(envid == 0) {
			SOUTH(i);
			return;
		}
		ProcessID[MATN+1][i] = envid;
	} //fork SOUTH
	for(int i = MATN; i >= 1; i--) {
		for(int j = MATN; j >= 1; j--) {
			envid = fork();
			if(envid == 0) {
				CENTER(ProcessID[i][j+1], ProcessID[i+1][j], B[i-1][j-1]);
				return;		
			}
			ProcessID[i][j] = envid;		
		}
	} //fork CENTER
	for(int i = 1; i <= MATN; i++) {
		envid = fork();
		if(envid == 0) {
			NORTH(ProcessID[1][i]);		
			return;
		}
		ProcessID[0][i] = envid;
	} //fork NORTH
	for(int i = 1; i <= MATN; i++) {
		envid = fork();
		if(envid == 0) {
			WEST(ProcessID[i][1], A[i-1]);	
			return;	
		}
		ProcessID[i][0] = envid;
	} //fork WEST
	return;
}


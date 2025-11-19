#ifndef DOS_H
#define DOS_H

/*
 * Header file for DOS-related utility functions.
 */

struct WORDREGS
	{
	unsigned int	ax, bx, cx, dx, si, di, cflag, flags;
	};

struct BYTEREGS
	{
	unsigned char	al, ah, bl, bh, cl, ch, dl, dh;
	};

union	REGS	{
	struct	WORDREGS x;
	struct	BYTEREGS h;
	};

struct	SREGS	{
	unsigned int	es;
	unsigned int	cs;
	unsigned int	ss;
	unsigned int	ds;
	};

struct	REGPACK
	{
	unsigned	r_ax, r_bx, r_cx, r_dx;
	unsigned	r_bp, r_si, r_di, r_ds, r_es, r_flags;
	};

int int86(int intno, union REGS *inregs, union REGS *outregs);
void outportb(int portid, char value);

void* MK_FP(int seg, int ofs);

#endif /* DOS_H */
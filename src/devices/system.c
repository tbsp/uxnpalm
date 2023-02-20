#include <PalmOS.h>

#include "../uxn.h"
#include "system.h"

/*
Copyright (c) 2022-2023 Devine Lu Linvega, Andrew Alderwick

Permission to use, copy, modify, and distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE.
*/

static const char *errors[] = {
	"underflow",
	"overflow",
	"division by zero"};

static void
system_print(Stack *s, char *name)
{
	Uint8 i;
	//fprintf(stderr, "<%s>", name);
	//for(i = 0; i < s->ptr; i++)
		//fprintf(stderr, " %02x", s->dat[i]);
	//f(!i)
		//fprintf(stderr, " empty");
	//fprintf(stderr, "\n");
}

static void
system_cmd(Uint8 *ram, Uint16 addr)
{
	/*if(ram[addr] == 0x01) {
		Uint16 i, length = PEEK16(ram + addr + 1);
		Uint16 a_page = PEEK16(ram + addr + 1 + 2), a_addr = PEEK16(ram + addr + 1 + 4);
		Uint16 b_page = PEEK16(ram + addr + 1 + 6), b_addr = PEEK16(ram + addr + 1 + 8);
		int src = (a_page % RAM_PAGES) * 0x10000, dst = (b_page % RAM_PAGES) * 0x10000;
		for(i = 0; i < length; i++)
			ram[dst + (Uint16)(b_addr + i)] = ram[src + (Uint16)(a_addr + i)];
	}*/
}

void
system_inspect(Uxn *u)
{
	system_print(u->wst, "wst");
	system_print(u->rst, "rst");
}

int
system_load(Uxn *u, char *filename)
{
    DmOpenRef       romDB;
    LocalID         romID;
    MemHandle       romRec;
    UInt8           *roms;

    // TODO: Use cardNo from rom listing
    romID = DmFindDatabase(0, filename);
    romDB = DmOpenDatabase(0, romID, dmModeReadOnly);
    romRec = DmQueryRecord(romDB, 0);
    roms = (UInt8 *) MemHandleLock(romRec);

    MemMove(&u->ram[PAGE_PROGRAM], roms, 0xf000 - PAGE_PROGRAM);

	return 1;
}

/* Error */

int
uxn_halt(Uxn *u, Uint8 instr, Uint8 err, Uint16 addr)
{
	Uint8 *d = &u->dev[0x00];
	Uint16 handler = GETVEC(d);
	if(handler) {
		u->wst->ptr = 4;
		u->wst->dat[0] = addr >> 0x8;
		u->wst->dat[1] = addr & 0xff;
		u->wst->dat[2] = instr;
		u->wst->dat[3] = err;
		return uxn_eval(u, handler);
	} else {
		system_inspect(u);
		//fprintf(stderr, "%s %s, by %02x at 0x%04x.\n", (instr & 0x40) ? "Return-stack" : "Working-stack", errors[err - 1], instr, addr);
	}
	return 0;
}

/* IO */

void
system_deo(Uxn *u, Uint8 *d, Uint8 port)
{
	switch(port) {
	case 0x2: u->wst = (Stack *)(u->ram + (d[port] ? (d[port] * 0x100) : 0x10000)); break;
	case 0x3: u->rst = (Stack *)(u->ram + (d[port] ? (d[port] * 0x100) : 0x10100)); break;
	case 0xe:
		if(u->wst->ptr || u->rst->ptr) system_inspect(u);
		break;
	}
}
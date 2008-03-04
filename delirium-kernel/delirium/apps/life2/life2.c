#include <delirium.h>

#include "delibrium/delibrium.h"

#include "vga/vga.h"
#include "vga/vga_modes.h"

/*
 * Implementation of Conway's Game of Life
 */

typedef unsigned char cell;
static cell *board;
static cell *neighbors;

static int w;
static int h;

#define Board(x,y) board[(y)*w+x]
#define Neighbors(x,y) neighbors[(y)*w+x]

void init_game(int width, int height) {
	void * pages[16];

	if (getpages(16, pages) != 16) return; // Mem leak!
	board = herdpages(16, pages);

	if (getpages(16, pages) != 16) return; // Mem leak!
	neighbors = herdpages(16, pages); 

	w = width;
	h = height;
	memset(neighbors, 0, w * h *sizeof(cell));
}

void calc_tick() {
	int x;
	int y;

	memset(neighbors, 0, w * h *sizeof(cell));
	for (y=0; y<h; y++) {
		for (x=0; x<w; x++)  {
			if (Board(x,y)) {
				if (x) {
					Neighbors(x-1,y)++;
					if (y) Neighbors(x-1,y-1)++;
					else Neighbors(x-1,h-1)++;
					if (y+1 != h) Neighbors(x-1,y+1)++;
					else Neighbors(x-1,0)++;
				} else {
					Neighbors(w-1,y)++;
					if (y) Neighbors(w-1,y-1)++;
					else Neighbors(w-1,h-1)++;
					if (y+1 != h) Neighbors(w-1,y+1)++;
					else Neighbors(w-1,0)++;
				}
				if (x+1 != w) {
					Neighbors(x+1,y)++;
					if (y) Neighbors(x+1,y-1)++;
					else Neighbors(x+1,h-1)++;
					if (y+1 != h) Neighbors(x+1,y+1)++;
					else Neighbors(x+1,0)++;
				} else {
					Neighbors(0,y)++;
					if (y) Neighbors(0,y-1)++;
					else Neighbors(0,h-1)++;
					if (y+1 != h) Neighbors(0,y+1)++;
					else Neighbors(0,0)++;
				}
				if (y) Neighbors(x,y-1)++;
				else Neighbors(x,h-1)++;
				if (y+1 != h) Neighbors(x,y+1)++;
				else Neighbors(x,0)++;
			}
		}
	}

	for (y=0; y<h; y++) {
		for (x=0; x<w; x++)  {
			if (Board(x,y)) {
				if (Neighbors(x,y) < 2 || Neighbors(x,y) > 3) {
					Board(x,y) = 0;
				} else
					Board(x,y) = Neighbors(x,y) + 2;
			}
			else {
				if (Neighbors(x,y) == 3) {
					Board(x,y) = Neighbors(x,y);
				}
			}
#if 0
			Board(x,y) = ((Neighbors(x,y) == 2) || 
				(Board(x,y) && Neighbors(x,y) == 3));
#endif
		}
	}
}

void cleanup_game() {
	freepage((void *)neighbors);
	freepage((void *)board);
}

void fill_life() {
#define A_(x,y) Board(x,y)++;
	        A_(25,10) A_(24,12) A_(25,12) A_(27,11) A_(28,12) A_(29,12) A_(30,12)
#undef A_
}


/*--------*/

void dream() {
	vga_openwindow();
	vga_loadregs(&vgaregs_320x200x256);

	init_game(320, 200);

#define LIFE_LIVE(x,y) Board(x,y)++;
	// Diehard
	LIFE_LIVE(115,105);
	LIFE_LIVE(116,105);
	LIFE_LIVE(116,106);
	LIFE_LIVE(120,106);
	LIFE_LIVE(121,106);
	LIFE_LIVE(122,106);
	LIFE_LIVE(121,104);

	fill_life();

	for(;;) {
		calc_tick();
	 	memcpy((void *)0xa0000, board, 320*200);
	}

	cleanup_game();

}




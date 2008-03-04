#include <delirium.h>

#include "delibrium/delibrium.h"

#include "vga/vga.h"
#include "vga/vga_modes.h"

/*
 * Implementation of Conway's Game of Life
 */

typedef char cell_t;

#define dead 0
#define alive 1

struct game {
	cell_t *cells;
	cell_t *shadow;
	size_t w;
	size_t h;
	size_t iter;
};


#define LIFE_CELL_REAL(g,x,y)  (((g)->cells)[((x)+(((g)->w) * (y)))])
#define BOUNDED_X(g,x)		( ((x)<0) ? ((x)+((g)->w)) :   \
				( ((x)>=((g)->w))  ? (x) - ((g)->w ) : (x) ) \
			)
#define BOUNDED_Y(g,y)		( ((y)<0) ? ((y)+((g)->h)) :   \
				( ((y)>=((g)->h))  ? (y) - ((g)->h ) : (y) ) \
			)
#define LIFE_CELL(g,x,y)	LIFE_CELL_REAL(g, BOUNDED_X(g,x), BOUNDED_Y(g,y))



#define LIFE_KILL(g,x,y)  (LIFE_CELL(g,x,y) = dead)
#define LIFE_LIVE(g,x,y)  (LIFE_CELL(g,x,y) = alive) 

#define LIFE_W(g,x,y)  LIFE_CELL(g,(x)-1,(y))
#define LIFE_E(g,x,y)  LIFE_CELL(g,(x)+1,(y))
#define LIFE_N(g,x,y)  LIFE_CELL(g,(x),(y)-1)
#define LIFE_S(g,x,y)  LIFE_CELL(g,(x),(y)+1)

#define LIFE_NW(g,x,y)  LIFE_CELL(g,(x)-1,(y)-1)
#define LIFE_NE(g,x,y)  LIFE_CELL(g,(x)+1,(y)-1)
#define LIFE_SW(g,x,y)  LIFE_CELL(g,(x)-1,(y)+1)
#define LIFE_SE(g,x,y)  LIFE_CELL(g,(x)+1,(y)+1)


void life_initGame(struct game *g) {
	int i;

	for (i=0; i<(g->w*g->h); i++)
		g->cells[i] = dead;
	g->iter = 0;
}

inline void life_tick(struct game *g) {
	int x;
	int y;
	struct game g2;


	g2 = *g;
	g2.cells = g2.shadow;
	memcpy(g2.cells, g->cells, g2.w * g2.h);

	for (y=0; y<(g->h); y++) {
		for (x=0; x<(g->w); x++) {
			int neighbors;

			neighbors = 
				LIFE_N(&g2,x,y) + LIFE_S(&g2,x,y) +
				LIFE_E(&g2,x,y) + LIFE_W(&g2,x,y) +
				LIFE_NE(&g2,x,y) + LIFE_NW(&g2,x,y) +
				LIFE_SE(&g2,x,y) + LIFE_SW(&g2,x,y);

			if (LIFE_CELL(&g2,x,y) == alive) {
				if (neighbors < 2) LIFE_CELL(g,x,y) = dead;
				if (neighbors > 3) LIFE_CELL(g,x,y) = dead;
			} else {
				if (neighbors == 3) LIFE_CELL(g,x,y) = alive;
			}
		}
	}

	g->iter++;
}

void dream() {
	struct game g;
	void * pages[16];

	if (getpages(16, pages) != 16) return; // Mem leak!
	g.cells = herdpages(16, pages);

	if (getpages(16, pages) != 16) return; // Mem leak!
	g.shadow = herdpages(16, pages); 

	vga_openwindow();
	vga_loadregs(&vgaregs_320x200x256);
	g.w = 320;
	g.h = 200;

	life_initGame(&g);

	// Diehard
	LIFE_LIVE(&g,115,105);
	LIFE_LIVE(&g,116,105);
	LIFE_LIVE(&g,116,106);
	LIFE_LIVE(&g,120,106);
	LIFE_LIVE(&g,121,106);
	LIFE_LIVE(&g,122,106);
	LIFE_LIVE(&g,121,104);

#define A_(x,y) LIFE_LIVE(&g,x,y);
	A_(25,10) A_(24,12) A_(25,12) A_(27,11) A_(28,12) A_(29,12) A_(30,12)
#undef A_
	for(;;) {
	 life_tick(&g);
	 memcpy((void *)0xa0000, g.cells, 320*200);
	}

	freepage((void *)g.cells);
	freepage((void *)g.shadow);
}

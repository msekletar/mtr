/*
    mtr  --  a network diagnostic tool
    Copyright (C) 1997,1998  Matt Kimball

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <config.h>

#ifndef NO_CURSES
#include <ctype.h>
#include <stdlib.h>

#if defined(HAVE_NCURSES_H)
#  include <ncurses.h>
#elif defined(HAVE_NCURSES_CURSES_H)
#  include <ncurses/curses.h>
#elif defined(HAVE_CURSES_H)
#  include <curses.h>
#else
#  error No curses header file available
#endif

#ifndef getmaxyx
#  define getmaxyx(win,y,x)	(y = (win)->_maxy + 1, x = (win)->_maxx + 1)
#endif

#include "mtr-curses.h"
#include "display.h"
#include "net.h"
#include "dns.h"
#endif

void pwcenter(char *str) {
  int maxx, maxy;
  int cx;

  getmaxyx(stdscr, maxy, maxx);
  cx = (signed)(maxx - strlen(str)) / 2;
  while(cx-- > 0)
    printw(" ");
  printw(str);
}

int mtr_curses_keyaction() {
  char c = getch();

  if(tolower(c) == 'q')
    return ActionQuit;
  if(c==3)
     return ActionQuit;
  if (c==12)
     return ActionClear;
  if (c==19)
     return ActionPause;
  if (c==17)
     return ActionResume;
  if(tolower(c) == 'r')
    return ActionReset;
  if (tolower(c) == 'd')
    return ActionDisplay;

  return 0;
}

void mtr_curses_hosts(int startstat) {
  int max;
  int at;
  int addr;
  int y, x;
  char *name;

  max = net_max();

  for(at = 0; at < max; at++) {
    printw("%2d. ", at + 1);
    addr = net_addr(at);

    if(addr != 0) {
      name = dns_lookup(addr);
      if(name != NULL) {
	printw("%s", name);
      } else {
	printw("%d.%d.%d.%d", (addr >> 24) & 0xff, (addr >> 16) & 0xff, 
	       (addr >> 8) & 0xff, addr & 0xff);
      }

      getyx(stdscr, y, x);
      move(y, startstat);

      printw("  %3d%% %4d %4d  %4d %4d %6d", 
             net_percent(at),
             net_returned(at), net_xmit(at),
             net_best(at), net_avg(at), net_worst(at));


    } else {
      printw("???");
    }

    printw("\n");
  }
}

static double factors[] = { 0.02, 0.05, 0.08, 0.15, 0.33, 0.50, 0.80, 1.00 };
static int scale[8];
static int low_ms, high_ms;

void mtr_gen_scale(void) {
	int *saved, i, max, at;
	int range;

	low_ms = 1000000;
	high_ms = -1;

	for (i = 0; i < 8; i++) {
		scale[i] = 0;
	}
	max = net_max();
	for (at = 0; at < max; at++) {
		saved = net_saved_pings(at);
		for (i = 0; i < SAVED_PINGS; i++) {
			if (saved[i] < 0) continue;
			if (saved[i] < low_ms) {
				low_ms = saved[i];
			}
			if (saved[i] > high_ms) {
				high_ms = saved[i];
			}
		}
	}
	range = high_ms - low_ms;
	for (i = 0; i < 8; i++) {
		scale[i] = low_ms + ((double)range * factors[i]);
	}
}

static const char* block_map = ".123abc>";

void mtr_print_scaled(int ms) {
	int i;

	for (i = 0; i < 8; i++) {
		if (ms <= scale[i]) {
			printw("%c", block_map[i]);
			return;
		}
	}
	printw(">");
}

void mtr_fill_graph(int at) {
	int* saved;
	int i;
	int val;

	saved = net_saved_pings(at);
	for (i = 0; i < SAVED_PINGS; i++) {
		if (saved[i] == -2) {
			printw(" ");
		} else if (saved[i] == -1) {
			attron(A_BOLD);
			printw("?");
			attroff(A_BOLD);
		} else {
			if (display_mode == 1) {
				if (saved[i] > scale[6]) {
					printw("%c", block_map[7]);
				} else {
					printw(".");
				}
			} else {
				mtr_print_scaled(saved[i]);
			}
		}
	}
}

void mtr_curses_graph(int startstat) {
	int max, at, addr, y, x;
	char* name;
	char blocks[50];

	max = net_max();
	for (at = 0; at < max; at++) {
		printw("%2d. ", at+1);

		addr = net_addr(at);
		if (!addr) {
			printw("???\n");
			continue;
		}

		name = dns_lookup(addr);
		if (name) {
			printw("%s", name);
		} else {
			printw("%d.%d.%d.%d", (addr >> 24) & 0xff, (addr >> 16) && 0xff, (addr >> 8) & 0xff, addr & 0xff);
		}

		getyx(stdscr, y, x);
		move(y, startstat);

		printw(" ");
		mtr_fill_graph(at);
		printw("\n");
	}
}

void mtr_curses_redraw() {
  int maxx, maxy;
  int startstat;
  int rowstat;
  int i;

  erase();
  getmaxyx(stdscr, maxy, maxx);

  rowstat = 5;

  attron(A_BOLD);
  move(0, 0);
  pwcenter("Matt's traceroute  [v" VERSION "]");
  printw("\n\n");
  attroff(A_BOLD);

  printw("Keys:  ");
  attron(A_BOLD);  printw("D");  attroff(A_BOLD);
  printw(" - Display mode    ");
  attron(A_BOLD);  printw("R");  attroff(A_BOLD);
  printw(" - Restart statistics    ");
  attron(A_BOLD);  printw("Q");  attroff(A_BOLD);
  printw(" - Quit\n");
  
  attron(A_BOLD);
  mvprintw(rowstat - 1, 0, "Hostname");

  if (display_mode == 0) {
    startstat = maxx - 40;

    /* Modified by Brian Casey December 1997 bcasey@imagiware.com */
    mvprintw(rowstat - 2, startstat, "    Packets            Pings");
    mvprintw(rowstat - 1, startstat, " %%Loss  Rcv  Snt  Best  Avg  Worst");

    attroff(A_BOLD);
    move(rowstat, 0);

    mtr_curses_hosts(startstat);
  } else {
    /* David Sward, Jan 1999 */
    startstat = maxx - 52;

    mvprintw(rowstat - 1, startstat, " Last 50 pings");

    attroff(A_BOLD);
    move(rowstat, 0);

    mtr_gen_scale();
    mtr_curses_graph(startstat);

    printw("\n");
    attron(A_BOLD);
    printw("Scale:");
    attroff(A_BOLD);
    
    for (i = 0; i < 7; i++) {
      printw("  %c:%d ms", block_map[i], scale[i]);
    }
  }

  refresh();
}

void mtr_curses_open() {
  initscr();
  raw();
  noecho(); 

  mtr_curses_redraw();
}

void mtr_curses_close() {  
  printw("\n");
  endwin();
}

void mtr_curses_clear() {
  mtr_curses_close();
  mtr_curses_open();
}

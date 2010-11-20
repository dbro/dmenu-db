/* See LICENSE file for copyright and license details. */
#include <locale.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>
#include "draw.h"

#define MAX(a, b)   ((a) > (b) ? (a) : (b))
#define MIN(a, b)   ((a) < (b) ? (a) : (b))
#define DEFFONT     "fixed"

static Bool loadfont(DC *dc, const char *fontstr);

void
drawrect(DC *dc, int x, int y, unsigned int w, unsigned int h, Bool fill, unsigned long color) {
	XRectangle r = { dc->x + x, dc->y + y, w, h };

	if(!fill) {
		r.width -= 1;
		r.height -= 1;
	}
	XSetForeground(dc->dpy, dc->gc, color);
	(fill ? XFillRectangles : XDrawRectangles)(dc->dpy, dc->canvas, dc->gc, &r, 1);
}


void
drawtext(DC *dc, const char *text, unsigned long col[ColLast]) {
	char buf[256];
	size_t n, mn;

	/* shorten text if necessary */
	n = strlen(text);
	for(mn = MIN(n, sizeof buf); textnw(dc, text, mn) > dc->w - (dc->xftfont.xft_font ? dc->xftfont.height : dc->font.height)/2; mn--)
		if(mn == 0)
			return;
	memcpy(buf, text, mn);
	if(mn < n)
		for(n = MAX(mn-3, 0); n < mn; buf[n++] = '.');

	drawrect(dc, 0, 0, dc->w, dc->h, True, BG(dc, col));
	drawtextn(dc, buf, mn, col);
}

void
drawtextn(DC *dc, const char *text, size_t n, unsigned long col[ColLast]) {
	int x, y;

	x = dc->x + (dc->xftfont.xft_font ? dc->xftfont.height : dc->font.height)/2;
	y = dc->y + (dc->xftfont.xft_font ? dc->xftfont.ascent : dc->font.ascent)+1;

	XSetForeground(dc->dpy, dc->gc, FG(dc, col));
    if(dc->xftfont.xft_font) {
        if (!dc->xftdraw)
            eprintf("error, creating xft drawable failed");
        if(dc->selected) {
            XftDrawStringUtf8(dc->xftdraw, &dc->xftselcolor, dc->xftfont.xft_font, x, y, (unsigned char*)text, n);
        } else {
            XftDrawStringUtf8(dc->xftdraw, &dc->xftcolor, dc->xftfont.xft_font, x, y, (unsigned char*)text, n);
        }
    } else if(dc->font.set)
		XmbDrawString(dc->dpy, dc->canvas, dc->font.set, dc->gc, x, y, text, n);
	else {
		XSetFont(dc->dpy, dc->gc, dc->font.xfont->fid);
		XDrawString(dc->dpy, dc->canvas, dc->gc, x, y, text, n);
	}
}

void
eprintf(const char *fmt, ...) {
	va_list ap;

	fprintf(stderr, "%s: ", progname);
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	exit(EXIT_FAILURE);
}

void
freedc(DC *dc) {
    if(dc->xftfont.xft_font) {
        int screen = DefaultScreen(dc->dpy);
        XftColorFree (dc->dpy, DefaultVisual(dc->dpy, screen), DefaultColormap(dc->dpy, screen), &dc->xftcolor);
        XftFontClose (dc->dpy, dc->xftfont.xft_font);
        XftDrawDestroy(dc->xftdraw);
    }
	if(dc->font.set)
		XFreeFontSet(dc->dpy, dc->font.set);
	if(dc->font.xfont)  // else if(!fontxft) [?]
		XFreeFont(dc->dpy, dc->font.xfont);
	if(dc->canvas)
		XFreePixmap(dc->dpy, dc->canvas);
	XFreeGC(dc->dpy, dc->gc);
	XCloseDisplay(dc->dpy);
	free(dc);
}

unsigned long
getcolor(DC *dc, const char *colstr) {
	Colormap cmap = DefaultColormap(dc->dpy, DefaultScreen(dc->dpy));
	XColor color;

	if(!XAllocNamedColor(dc->dpy, cmap, colstr, &color, &color))
		eprintf("cannot allocate color '%s'\n", colstr);
	return color.pixel;
}

DC *
initdc(void) {
	DC *dc;

	if(!setlocale(LC_CTYPE, "") || !XSupportsLocale())
		weprintf("no locale support\n");
	if(!(dc = malloc(sizeof *dc)))
		eprintf("cannot malloc %u bytes\n", sizeof *dc);
	if(!(dc->dpy = XOpenDisplay(NULL)))
		eprintf("cannot open display\n");

	dc->gc = XCreateGC(dc->dpy, DefaultRootWindow(dc->dpy), 0, NULL);
	XSetLineAttributes(dc->dpy, dc->gc, 1, LineSolid, CapButt, JoinMiter);
	dc->font.xfont = NULL;
	dc->font.set = NULL;
    dc->xftfont.xft_font = NULL;
	dc->canvas = None;
    dc->selected = False;
	return dc;
}

void
initfont(DC *dc, const char *fontstr) {
	if(!loadfont(dc, fontstr ? fontstr : DEFFONT)) {
		if(fontstr != NULL)
			weprintf("cannot load font '%s'\n", fontstr);
		if(fontstr == NULL || !loadfont(dc, DEFFONT))
			eprintf("cannot load font '%s'\n", DEFFONT);
	}
	dc->font.height = dc->font.ascent + dc->font.descent;
}

void
initxftfont(DC *dc, const char *fontstr) {
    int screen = DefaultScreen(dc->dpy);
//    if(!XftColorAllocName(dc->dpy, DefaultVisual(dc->dpy, screen), DefaultColormap(dc->dpy, screen), (const char*)normfgcolor, &dc->xftcolor))
//        eprintf("error, cannot allocate xft font color '%s'\n", normfgcolor);
//    if(!XftColorAllocName(dc->dpy, DefaultVisual(dc->dpy, screen), DefaultColormap(dc->dpy, screen), (const char*)selfgcolor, &dc->xftselcolor))
//        eprintf("error, cannot allocate xft font color '%s'\n", normfgcolor);
    if(!(dc->xftfont.xft_font = XftFontOpenName (dc->dpy, screen, fontstr)))
        eprintf("error, cannot load xft font\n" );
    if(!(dc->xftdraw = XftDrawCreate(dc->dpy, dc->canvas, DefaultVisual(dc->dpy,screen), DefaultColormap(dc->dpy,screen))));
        eprintf("error, cannot create xft drawable\n");
    dc->xftfont.ascent = dc->xftfont.xft_font->ascent;
    dc->xftfont.descent = dc->xftfont.xft_font->descent;
    dc->xftfont.height = dc->xftfont.ascent + dc->xftfont.descent;
}

Bool
loadfont(DC *dc, const char *fontstr) {
	char *def, **missing;
	int i, n;

	if(!*fontstr)
		return False;
	if((dc->font.set = XCreateFontSet(dc->dpy, fontstr, &missing, &n, &def))) {
		char **names;
		XFontStruct **xfonts;

		n = XFontsOfFontSet(dc->font.set, &xfonts, &names);
		for(i = dc->font.ascent = dc->font.descent = 0; i < n; i++) {
			dc->font.ascent = MAX(dc->font.ascent, xfonts[i]->ascent);
			dc->font.descent = MAX(dc->font.descent, xfonts[i]->descent);
		}
	}
	else if((dc->font.xfont = XLoadQueryFont(dc->dpy, fontstr))) {
		dc->font.ascent = dc->font.xfont->ascent;
		dc->font.descent = dc->font.xfont->descent;
	}
	if(missing)
		XFreeStringList(missing);
	return (dc->font.set || dc->font.xfont);
}

void
mapdc(DC *dc, Window win, unsigned int w, unsigned int h) {
	XCopyArea(dc->dpy, dc->canvas, win, dc->gc, 0, 0, w, h, 0, 0);
}

void
resizedc(DC *dc, unsigned int w, unsigned int h) {
	if(dc->canvas)
		XFreePixmap(dc->dpy, dc->canvas);
	dc->canvas = XCreatePixmap(dc->dpy, DefaultRootWindow(dc->dpy), w, h,
	                           DefaultDepth(dc->dpy, DefaultScreen(dc->dpy)));
	dc->x = dc->y = 0;
	dc->w = w;
	dc->h = h;
	dc->invert = False;
}

int
textnw(DC *dc, const char *text, size_t len) {
    if(dc->xftfont.xft_font) {
        XftTextExtentsUtf8(dc->dpy, dc->xftfont.xft_font, (const FcChar8*)text, len, &dc->gi);
        return dc->gi.width;
    } else if(dc->font.set) {
		XRectangle r;
		XmbTextExtents(dc->font.set, text, len, NULL, &r);
		return r.width;
	}
	return XTextWidth(dc->font.xfont, text, len);
}

int
textw(DC *dc, const char *text) {
//    if(dc->xftfont.xft_font)
//        return textnw(dc, text, strlen(text)) + dc->xftfont.height;
	return textnw(dc, text, strlen(text)) + (dc->xftfont.xft_font ? dc->xftfont.height : dc->font.height);
}

void
weprintf(const char *fmt, ...) {
	va_list ap;

	fprintf(stderr, "%s: warning: ", progname);
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}

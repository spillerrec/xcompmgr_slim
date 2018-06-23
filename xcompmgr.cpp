/*
 * Copyright © 2003 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */


/* Modified by Matthew Hawn. I don't know what to say here so follow what it
   says above. Not that I can really do anything about it
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xcomposite.h>
#include <unistd.h>


static int composite_opcode;


static int error( Display *dpy, XErrorEvent *ev ){
	if(    ev->request_code == composite_opcode
	    && ev->minor_code == X_CompositeRedirectSubwindows
	){
		fprintf (stderr, "Another composite manager is already running\n");
		exit (1);
	}

	//Get error message
	char buffer[256];
	buffer[0] = '\0';
	XGetErrorText (dpy, ev->error_code, buffer, sizeof (buffer));

	//Output error message
	fprintf (stderr, "error %d: %s request %d minor %d serial %lu\n",
			ev->error_code, (strlen (buffer) > 0) ? buffer : "unknown",
			ev->request_code, ev->minor_code, ev->serial);

	return 0;
}


static void print_error_cm_exists( Display* dpy, Window w ){
	XTextProperty tp;
	Atom winNameAtom = XInternAtom( dpy, "_NET_WM_NAME", false );

	//Try to get name of CM, print generic message if it fails
	if( !XGetTextProperty( dpy, w, &tp, winNameAtom ) &&
		!XGetTextProperty( dpy, w, &tp, XA_WM_NAME ) )
	{
		fprintf (stderr,
			"Another composite manager is already running (0x%lx)\n",
			(unsigned long) w);
		return;
	}
	
	//Print an error message with the naame of the CM
	char **strs;
	int count;
	if( XmbTextPropertyToTextList( dpy, &tp, &strs, &count ) == Success ){
		fputs( "Another composite manager is already running (", stderr );
		fputs( strs[0], stderr );
		fputs( ")\n", stderr );
		
		XFreeStringList (strs);
	}

	XFree( tp.value );
}

static Atom get_atom_VM_CM( Display* dpy, int scr ){
	char net_wm_cm[] = "_NET_WM_CM_Sxx";
	snprintf( net_wm_cm, sizeof(net_wm_cm), "_NET_WM_CM_S%d", scr );
	
	return XInternAtom( dpy, net_wm_cm, false );
}


int main ( int argc, char **argv ){
	Display* dpy = XOpenDisplay( argc == 2 ? argv[1] : nullptr );
	if( !dpy ){
		fputs( "Can't open display\n", stderr );
		exit (1);
	}
	
	XSetErrorHandler (error);
	int scr = DefaultScreen (dpy);
	
	int composite_event, composite_error;
	if (!XQueryExtension (dpy, COMPOSITE_NAME, &composite_opcode,
			  &composite_event, &composite_error))
	{
		fputs( "No composite extension\n", stderr );
		exit (1);
	}
	
	Atom a = get_atom_VM_CM( dpy, scr );

	//Check for existing composite manager
	Window w = XGetSelectionOwner (dpy, a);
	if( w != None ){
		print_error_cm_exists( dpy, w );
		exit (1);
	}

	//Register as the current composite maanager
	w = XCreateSimpleWindow( dpy, RootWindow( dpy, scr ), 0, 0, 1, 1, 0, None, None );
	Xutf8SetWMProperties( dpy, w, "xcompmgr", "xcompmgr", nullptr, 0, nullptr, nullptr, nullptr );
	XSetSelectionOwner( dpy, a, w, 0 );
	
	
	XGrabServer( dpy );
	XCompositeRedirectSubwindows( dpy, RootWindow( dpy, scr ), CompositeRedirectAutomatic );
	XUngrabServer( dpy );
	
	//Don't quit?
	for( ; ; ){
		XFlush( dpy );
		XEvent ev;
		XNextEvent( dpy, &ev );
	}
}

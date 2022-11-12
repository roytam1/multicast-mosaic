/* Please read copyright.tmpl. Don't remove next line */
#include "copyright.ncsa"

#ifndef __XMXP_H__
#define __XMXP_H__

/* System includes. */
#include <stdio.h>
#include <string.h>
#include <assert.h>

/* Main Xmx include (also X11 includes and Xm.h). */
#include "Xmx.h"

#include <Xm/XmAll.h>

#if 0

/* SGI-specific GL Widget inclues. */
#ifdef __sgi
#include <X11/Xirisw/GlxMDraw.h>
#endif

#ifdef _IBMR2
#include "/usr/lpp/GL/utilities/inc/Glib.h"
#endif /* _IBMR2 */

#endif /* if 0 */

/* Marc's defines. */
#undef  private
#define private static
#undef  public
#define public

#endif /* __XMXP_H__ */

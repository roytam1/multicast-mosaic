/*
                          C FILE WRITER
                                             
   It is useful to have both FWriter and Writer for environments in
   which fdopen() doesn't exist for example.

 */
#ifndef HTFWRITE_H
#define HTFWRITE_H

#include "HTStream.h"
#include <stdio.h>
#include "HTFormat.h"

extern HTStream * HTSaveAndExecute (
        HTPresentation *        pres,
        HTParentAnchor *        anchor, /* Not used */
        HTStream *              sink,
        HTFormat                format_in,
        int                     compressed,
	caddr_t			appd);

#endif

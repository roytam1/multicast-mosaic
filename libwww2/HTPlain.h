/*              Plain text object */

#ifndef HTPLAIN_H
#define HTPLAIN_H

#include "HTStream.h"
#include "HTAnchor.h"

extern HTStream* HTPlainPresent (
        HTPresentation *        pres,
        HTParentAnchor *        anchor,
        HTStream *              sink, 
        HTFormat                format_in,
        int                     compressed,
	caddr_t			appd);

#endif

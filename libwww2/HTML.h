#ifndef LIBWWW2_HTML_H
#define LIBWWW2_HTML_H

#include "HTUtils.h"
#include "HTAnchor.h"
#include "SGML.h"


extern HTStructured* HTML_new PARAMS((
        HTParentAnchor * anchor,
        HTFormat        format_out,
        HTStream *      target));

#endif

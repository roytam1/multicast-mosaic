/*      Hypertext "Anchor" Object                                    HTAnchor.h
**      ==========================
**
**      An anchor represents a region of a hypertext document which is linked
**      to another anchor in the same or a different document.
*/

#ifndef HTANCHOR_H
#define HTANCHOR_H

/* Version 0 (TBL) written in Objective-C for the NeXT browser */
/* Version 1 of 24-Oct-1991 (JFG), written in C, browser-independant */

#include "HTList.h"
#include "HTAtom.h"

/*                      Main definition of anchor */

typedef struct _HyperDoc HyperDoc;  /* Ready for forward references */
typedef struct _HTAnchor HTAnchor;
typedef struct _HTParentAnchor HTParentAnchor;

/*      After definition of HTFormat: */
#include "HTFormat.h"

typedef HTAtom HTLinkType;

typedef struct {
  HTAnchor *    dest;           /* The anchor to which this leads */
  HTLinkType *  type;           /* Semantics of this link */
} HTLink;

struct _HTAnchor {              /* Generic anchor : just links */
  HTLink        mainLink;       /* Main (or default) destination of this */
  HTList *      links;          /* List of extra links from this, if any */
  /* We separate the first link from the others to avoid too many small mallocs
     involved by a list creation. Most anchors only point to one place. */
  HTParentAnchor * parent;      /* Parent of this anchor (self for adults) */
};

struct _HTParentAnchor {
  /* Common part from the generic anchor structure */
  HTLink        mainLink;       /* Main (or default) destination of this */
  HTList *      links;          /* List of extra links from this, if any */
  HTParentAnchor * parent;      /* Parent of this anchor (self) */

  /* ParentAnchor-specific information */
  HTList *      children;       /* Subanchors of this, if any */
  HTList *      sources;        /* List of anchors pointing to this, if any */
  HyperDoc *    document;       /* The document within which this is an anchor */
  char *        address;        /* Absolute address of this node */
  HTFormat      format;         /* Pointer to node format descriptor */
  HT_BOOL          isIndex;        /* Acceptance of a keyword search */
  char *        title;          /* Title of document */

  HTList*       methods;        /* Methods available as HTAtoms */
  void *        protocol;       /* Protocol object */
  char *        physical;       /* Physical address */
};

typedef struct {
  /* Common part from the generic anchor structure */
  HTLink        mainLink;       /* Main (or default) destination of this */
  HTList *      links;          /* List of extra links from this, if any */
  HTParentAnchor * parent;      /* Parent of this anchor */

  /* ChildAnchor-specific information */
  char *        tag;            /* Address of this anchor relative to parent */
} HTChildAnchor;


/*      Create new or find old named anchor
**
**      This one is for a reference which is found in a document, and might
**      not be already loaded.
**      Note: You are not guaranteed a new anchor -- you might get an old one,
**      like with fonts.
*/

extern HTAnchor * HTAnchor_findAddress(WWW_CONST char * address);

#endif /* HTANCHOR_H */

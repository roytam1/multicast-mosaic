/*             The Stream class definition -- libwww
                                 STREAM OBJECT DEFINITION

   A Stream object is something which accepts a stream of text.

   The creation methods will vary on the type of Stream Object, but
   the methods used to write to it and close it are common.

 */
#ifndef HTSTREAM_H
#define HTSTREAM_H

#include "HTUtils.h"

typedef struct _HTStream HTStream;

/*
   These are the common methods of all streams.  They should be
   self-explanatory, except for end_document which must be called
   before free.  It should be merged with free in fact: it should be
   dummy for new streams.
   
   The put_block method was write, but this upset systems whiuch had
   macros for write().
 */
typedef struct _HTStreamClass {
        char*  name;                            /* Just for diagnostics */
        void (*free) ( HTStream*   me, caddr_t appd);
        void (*end_document) ( HTStream*  me, caddr_t appd);
        void (*put_character) ( HTStream* me, char ch, caddr_t appd);
        void (*put_string) ( HTStream*    me, char * str, caddr_t appd);
        void (*put_block) ( HTStream* me, char * str, int len, caddr_t appd);
        void (*handle_interrupt) ( HTStream* me, caddr_t appd);
}HTStreamClass;

#endif /* HTSTREAM_H */

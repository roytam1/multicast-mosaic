/* SGML AND STRUCTURED STREAMS
                                             
   The SGML parser is a state machine. It is called for every
   character of the input stream. The DTD data structure contains
   pointers to functions which are called to implement the actual
   effect of the text read. When these functions are called, the
   attribute structures pointed to by the DTD are valid, and the
   function is passed a pointer to the curent tag structure, and an
   "element stack" which represents the state of nesting within SGML
   elements.
   
   The following aspects are from Dan Connolly's suggestions: Binary
   search, Strcutured object scheme basically, SGML content enum type.
   
   (c) Copyright CERN 1991 - See Copyright.html
   
 */
#ifndef SGML_H
#define SGML_H

#include "HTUtils.h"
#include "HTStream.h"


/*              Structured Object definition
**
**      A structured object is something which can reasonably be
**      represented in SGML.  I'll rephrase that.  A structured
**      object is am ordered tree-structured arrangement of data
**      which is representable as text.
**
**      The SGML parer outputs to a Structured object.
**      A Structured object can output its contents
**      to another Structured Object.
**      It's a kind of typed stream.  The architecure
**      is largely Dan Conolly's.
**      Elements and entities are passed to the sob by number, implying
**      a knowledge of the DTD.
**      Knowledge of the SGML syntax is not here, though.
**
**      Superclass: HTStream
*/


/*      The creation methods will vary on the type of Structured Object.
**      Maybe the callerData is enough info to pass along.
*/

typedef struct _HTStructured HTStructured;

typedef struct _HTStructuredClass{
        char*  name;                            /* Just for diagnostics */
        void (*free) PARAMS(( HTStructured*   me));
        void (*end_document) PARAMS(( HTStructured*   me));
        void (*handle_interrupt) PARAMS(( HTStructured*   me));
        void (*put_character) PARAMS(( HTStructured*   me, char ch));
        void (*put_string) PARAMS(( HTStructured* me, WWW_CONST char * str));
        void (*write) PARAMS((
                HTStructured*   me,
                WWW_CONST char *    str,
                int             len));
        void (*put_entity) PARAMS((
                HTStructured*   me,
                int             entity_number));
}HTStructuredClass;

#endif  /* SGML_H */

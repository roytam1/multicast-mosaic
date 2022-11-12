#ifndef PLUGINM_H 
#define PLUGINM_H

#include <X11/Intrinsic.h>
#include "../pluglib/ERROR.h"
#include "../pluglib/com_struct.h"

/***********************************************/
/**** Les fonctions de la lib cote plugin    ***/
/*** MPP_GetValue : recuperation de donnees  ***/
/*** MPP_SetValue : positionnement de donnees **/
/*** MPP_MemAlloc : Allocation de memoire     **/
/*** MPP_FreeMem  : Liberation de la memoire  **/
/*** MPP_GetMem   : recuperation du pointeur  **/
/*** sur le premier pointeur cree             **/
/***********************************************/

void*     MPP_GetValue(MPX_INSTANCE *, MPX_SET, void *);
MP_ERROR   MPP_SetValue(MPX_INSTANCE *, MPX_SET, void *);
void *MPP_MemAlloc(MPX_Type type, MPX_MEM set, size_t size);
MP_ERROR MPP_FreeMem(MPX_INSTANCE *myinst, void *mymem);
void *MPP_GetMem(MPX_Type , MPX_MEM set);

#endif

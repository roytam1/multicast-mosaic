/**************************************************/
/***** Interface PLUGIN MOSAIC                 ****/
/***** Version 2.0                             ****/
/***** Date : le 15 juin 2000                  ****/
/***** Createurs : GEORGE LISE                 ****/
/*****             HOURS SIMON                 ****/
/*****             DEURVEILLER GILLES          ****/
/*****             FERLICOT FREDERIC           ****/
/***** Fichier public_struct entrees des       ****/
/***** methodes publiques pour mosaic          ****/
/*****                                         ****/
/**************************************************/

#ifndef PUBLIC_H
#define PUBLIC_H

#include "com_struct.h"
#include "ERROR.h"

/*********************************************************/
/**** memoire tampon de passage de parametres          ***/
/**** calcules a partir de l'elt OBJECT                ***/
/*********************************************************/

typedef struct _MPX_CONTEXT
{

  int ID; 

  char *classid;
  char *data;
  char *codebase;
  char *standby;
  
  ThePARAM *PARAM;
  
  int width,height;
  int x,y;
  int border;
 /*  char* type; */
  Widget MOSAICWin;



}MPX_CONTEXT;
  
/*********************************************************/
/*** Les fonction accessibles par mMOSAIC              ***/
/*** GetMINEDDescription : construction de la table    ***
/*** d'association                                     ***/
/*** MP_New : creation d'une instance                  ***/
/*** MP_SetValue : on positionne des valeurs           ***/
/*** MP_GetValue : on recupere des valeurs             ***/
/*** MP_Destroy  : Destructrion d'une instance         ***/
/*********************************************************/

MPX_PLUGIN MP_GetMINEDescription();
MPX_INSTANCE *MP_New(MPX_PLUGIN, MPX_CONTEXT *,char *type);
void MP_Destroy(MPX_PLUGIN, MPX_INSTANCE *);
MP_ERROR MP_SetValue(MPX_INSTANCE *,MPX_SET,void *);
void *MP_GetValue(MPX_INSTANCE *,MPX_SET);


#endif

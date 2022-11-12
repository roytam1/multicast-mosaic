/**************************************************/
/***** Interface PLUGIN MOSAIC                 ****/
/***** Version 2.0                             ****/
/***** Date : le 15 juin 2000                  ****/
/***** Createurs : GEORGE LISE                 ****/
/*****             HOURS SIMON                 ****/
/*****             DEURVEILLER GILLES          ****/
/*****             FERLICOT FREDERIC           ****/
/***** Fichier private.h contient les          ****/
/***** structures utilisees par la lib cote    ****/
/***** mMosaic                                 ****/
/**************************************************/

#ifndef PRIVATE_H
#define PRIVATE_H

/***** On inclus les structures partagees ******/
#include "com_struct.h"



/***********************************************************************/
/**** Les methodes associees a la structure MP_PLUG                  ***/
/**** MP_Create_PLUG    : creation de la pile de plugin              ***/
/**** MP__Del_All       : Suppression de la pile                     ***/
/**** MP_Add_ASSO       : ajout d'une association                    ***/
/**** MP_Add_PLUG_Inst  : ajout d'une instance                       ***/
/**** MP_Del_PLUG_Inst  : supprime une instance                      ***/
/**** MP_Getmyplug      : retourne le pointeur sur le plugin qui     ***/
/**** reconnait le type type                                         ***/
/***********************************************************************/

MP_PLUG *MP_Create_PLUG();
MP_ERROR MP_Del_All(MP_PLUG *);
MP_ERROR MP_Add_ASSO(MP_PLUG *,char *,char *);
MP_ERROR MP_Add_PLUG_Inst(MP_PLUG *,char *);
MP_ERROR MP_Del_PLUG_Inst(MP_PLUG *,char *);
MP_ASSO *MP_Getmyplug(MP_PLUG *myplug,char *type);

/*********************************************************/
/*** Les methodes associees a MP_Private_Inst          ***/
/*** MP_Create_Inst : creation d'une instance          ***/
/*** MP_Destroy_Inst : destruction d'une instance      ***/
/*********************************************************/

MP_Private_Inst *MP_Create_Inst(MP_PLUG *,char *type);
MP_ERROR MP_Destroy_Inst(MP_Private_Inst *);
MP_ERROR MP_Load(MP_ASSO *myasso);

#endif

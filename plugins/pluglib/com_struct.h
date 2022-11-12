/**************************************************/
/***** Interface PLUGIN MOSAIC                 ****/
/***** Version 2.0                             ****/
/***** Date : le 15 juin 2000                  ****/
/***** Createurs : GEORGE LISE                 ****/
/*****             HOURS SIMON                 ****/
/*****             DEURVEILLER GILLES          ****/
/*****             FERLICOT FREDERIC           ****/
/***** Fichier COM_struct.h contient les       ****/
/***** structures utilisees par les deux       ****/
/***** librairies                              ****/
/**************************************************/

#ifndef COM_STRUCT_H 
#define COM_STRUCT_H

#include <X11/Intrinsic.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <stdio.h>
#include "ERROR.h"

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <netdb.h>


/*********************************************************/
/*** structure publique d'instance                    ****/
/*** MOSAIC_Data : donnees privees pour la lib        ****/
/*** Plugin_Data : pointeur pour l'instance que le    ****/
/*** developpeur peut utiliser a sa guise             ****/
/*********************************************************/

typedef struct _MPX_INSTANCE
{
  void *MOSAIC_Data;
  void *Plugin_Data;
}MPX_INSTANCE;

/**********************************************************/
/*** MPX_PLUGIN est un pointeur de pile pour la gestion ***/
/*** des plugins                                        ***/
/**********************************************************/

typedef void* MPX_PLUGIN;
/*****************************************************************/
/*** La structure de memoire d'un plugin                      ****/
/*****************************************************************/

typedef struct _MP_MEMORY
{
  void *memory;
  struct _MP_MEMORY *next;

}MP_MEMORY;

/**********************************************************/
/**** Structure pour recuperer les parametres donnes   ****/
/**** par l'element OBJECT                             ****/
/**** le next est utilise pour pouvoir gerer ces PARAM ****/
/**** comme une liste chainee                          ****/
/**********************************************************/

typedef struct _PARAM
{
  char *NAME;         /**** Le nom du parametre ****/
  char *VALUE;        /**** Sa valeur           ****/
  char *VALUETYPE;    /**** Le type de valeur   ****/

  struct _PARAM *next; /*** pour la gestion de la pile ***/

}ThePARAM;

/*****************************************************************/
/*** Les definitions de type de fonctions                      ***/
/*** Les fonctions MPP_NEW, MPP_Destroy, MPP_Demande,          ***/
/*** MPP_Initialize sont obligatoires pour le plugin           ***/
/*** L'une des trois autres fonctions doit etre presente       ***/
/*****************************************************************/

typedef void (*_MPP_New)(MPX_INSTANCE *);
typedef void (*_MPP_Destroy)(MPX_INSTANCE *);
typedef void (*_MPP_Demande)(char *);
typedef void (*_MPP_Initialize)(MPX_PLUGIN); 
typedef void (*_MPP_NeedSocket)();
typedef void (*_MPP_NeedFile)();
typedef void (*_MPP_NeedNothing)();

/******************************************************************/
/*** La structure qui gere les plugin en memoire                ***/
/******************************************************************/

typedef struct _MP_ASSO 
{
  char *type;  /*** Le type de plugin ***/
  char *path;  /*** Le chemin d'acces ***/
  int typecom; /**** si = 0 pas de com, =1 par socke, =2 par fichier *****/
 
  int ID;      /*** L'indentificateur ***/
 
  int NB_INSTANCES; /*** Le nombre d'instances du plugin ***/
  
  void *Handle;     /*** Le pointeur sur le debut du plugin ***/
  _MPP_New MPP_New; /*** le pointeur sur la fonction MPP_New du plugin ***/
  _MPP_Destroy MPP_Destroy; /*** le pointeur sur la fonction MPP_Destroy ***/
  _MPP_Demande MPP_Demande; /*** le pointeur sur la fonction MPP_Demande ***/
  _MPP_Initialize MPP_Initialize; 
  _MPP_NeedSocket MPP_NeedSocket;
  _MPP_NeedFile   MPP_NeedFile;
  _MPP_NeedNothing MPP_NeedNothing;

  MP_MEMORY *plug_info; /*** Un pointeur donne au plugin ***/
  MP_MEMORY *plug_info_Queue;

  struct _MP_ASSO *next; /*** pointeur sur le prochain plugin ***/

}MP_ASSO;

typedef struct _MP_PLUG
{
  MP_ASSO *head;
  int ID_PLUG;

}MP_PLUG;

/***********************************************************************/
/**** La structure suivante est la structure d'une instance cote prive */
/***********************************************************************/

typedef struct MP_Private_Inst
{
  int ID;  /*** Le numero d'instance ***/
  int x,y; /*** Les coordonnees de la fenetre ***/
  
  int Socket; /***** numero de socket  *****/ 
  FILE *myfile; /**** Pointeur sur le fichier ****/
  char Nom_Fichier[100];/*** ce champ contiendra le nom du fichier dans    ***/
  /*** lequel on aura stocke les donnees de l'instance***/

  int ShutDown;     /*** si=1 l'instance est en mode pause ***/

  int width,height; /*** largeur, hauteur de la fenetre ***/
  int border;
  char *standby; /***texte du message a afficher au cours du chargement de l'objet s'il y a une attente***/

  Widget myWindow; /*** La fenetre du plugin ***/
  Widget MOSAICWin; /*** La widget de la fenetre HTML ***/

  ThePARAM *PARAM;     /*** Les parametres OBJECT; ***/
 

  char *classid;      /**** l'url definit par l'OBJECT ***/
  char *type;         /**** Mon type ****/
  char *codebase;   /****URL specifiant chemin d'acces de base en complement de classid ***/
  char *data;       /****URL de l'objet a charger***/
 

  MP_ASSO *myplug;   /*** Un pointeur sur le plugin associe ***/

}MP_Private_Inst;


typedef enum _MPX_SET {

  X,Y,
  HEIGHT,WIDTH,
  WINDOW,MOSAICwin,
  SOCKET,myFILE,
  TYPECOM,
  CLASSID, 
  TYPE,
  STANDBY,
  PARAM,
  ID,
  BORDER,
  CODEBASE,
  DATA 

}MPX_SET;

typedef enum _MPX_MEM
{
  PLUG,
  INSTANCE
}MPX_MEM;

typedef void* MPX_Type;
#endif

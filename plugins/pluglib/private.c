/**************************************************/
/***** Interface PLUGIN MOSAIC                 ****/
/***** Version 2.0                             ****/
/***** Date : le 15 juin 2000                  ****/
/***** Createurs : GEORGE LISE                 ****/
/*****             HOURS SIMON                 ****/
/*****             DEURVEILLER GILLES          ****/
/*****             FERLICOT FREDERIC           ****/
/***** Fichier private.c contient les          ****/
/***** methodes privees utilisees par le lib   ****/
/***** pour la gestion des plugins             ****/
/**************************************************/

#include "private.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>


/*****************************************************/
/*** Passage en mode DEBUG                         ***/
/*****************************************************/



#ifdef DEBUG

#define DEBUG_ASSO
#define DEBUG_PILE

#endif

/*****************************************************/
/*** Les methodes associees aux MP_ASSO            ***/
/*****************************************************/

/*****************************************************/
/*** La fonction MP_Create_ASSO                    ***/
/*** Creation d'une association type sur le path   ***/
/*** et de numero path                             ***/
/*** Cette fonction initialise le reste a NULL ou 0 **/
/*** La valeur retournee est l'association cree    ***/
/*****************************************************/

MP_ASSO *MP_Create_ASSO(char *type, char *path, int ID)
{

  MP_ASSO *myasso;

  /**** Creation de l'association ****/

  myasso=(MP_ASSO*)malloc(sizeof(MP_ASSO));
  if (myasso==NULL)
    {
      fprintf(stderr,"Dans MP_Create_ASSO : impossible de creer une association \n");
      return NULL;
    }
  else
    {
      myasso->ID=ID;          /*** On lui donne son numero ***/
      myasso->NB_INSTANCES=0; /*** Au depart il n'y a pas d'insatnce ***/
      myasso->Handle=NULL;    /*** Le plugin n'est pas charge ***/
      myasso->MPP_New=NULL;   /*** Si pas charge on n'a pas acces a la fonction MPP_NEW ***/
      myasso->MPP_Demande=NULL;
      myasso->MPP_Initialize=NULL;
      myasso->MPP_Destroy=NULL;
      myasso->MPP_NeedSocket=NULL;
      myasso->MPP_NeedFile=NULL;
      myasso->MPP_NeedNothing=NULL;

      myasso->next=NULL;

      myasso->plug_info=NULL; /*** On n'a pas de donnees utilisateur ***/

      myasso->plug_info_Queue=NULL;

      myasso->type=(char*)malloc(sizeof(char)*(strlen(type)+1));
      
      if (myasso->type==NULL) 
	{
	  fprintf(stderr,"Dans MP_Create_ASSO : impossible de creer type \n");
	  free(myasso);
	  return NULL;
	}
      
      myasso->path=(char*)malloc(sizeof(char)*(strlen(path)+1));
      if (myasso->path==NULL) 
	{
	  fprintf(stderr,"Dans MP_Create_ASSO : impossible de creer type \n");
	  free(myasso->type);
	  free(myasso);
	  return NULL;
	}
	
      strcpy(myasso->type,type);
      strcpy(myasso->path,path);

#ifdef DEBUG_ASSO
	  printf("L'association type = %s path = %s et d'ID = %d a ete creee \n",type,path,ID);
#endif
	
    }

  /**** On retourne l'association creee ****/

  return myasso;
}

/********************************************************/
/*** MP_Delete_ASSO : destruction d'une association   ***/
/*** Le parametre est l'association a detruire        ***/
/*** Si le nombre d'instances est non null il sera    ***/
/*** impoissible de detruire l'association            ***/
/*** dans ce cas MPERR_STILL_INSTANCE est retourne    ***/
/*** sinon c'est MPERR_NO_ERROR                       ***/
/********************************************************/

MP_ERROR MP_Delete_ASSO(MP_ASSO *myasso)
{
  if (myasso==NULL)
    {
      fprintf(stderr,"Dans MP_Delete_ASSO passage d'une association NULL \n");
      return MPERR_NO_ASSO;
    }
  else
    {
      if (myasso->NB_INSTANCES!=0)
	return MPERR_STILL_INSTANCE;
      else
	{
	  if (myasso->type!=NULL) free(myasso->type);
	  if (myasso->path!=NULL) free(myasso->path);
	  free(myasso);
	  return MPERR_NO_ERROR;
	}
      
    }
  
}

/*************************************************************/
/*** Fonction MP_Load : charge le plugin en memoire        ***/
/*** elle met a jour pour cette association les differents ***/
/*** pointeurs de fonctions                                ***/
/*** Si le nombre d'association est null il n'y a aucun    ***/
/*** chargement et la fonction retourne MPERR_NB_INST      ***/
/*** si tout est charge elle retourne MPERR_NO_ERROR       ***/
/*** si il manque une fonction obligatoire elle retourne   ***/
/*** MPERR_CANNOT_LOAD_PLUG                                ***/
/*************************************************************/

MP_ERROR MP_Load(MP_ASSO *myasso)
{
  /*** Le pointeur sur le plugin ***/

  void *ptr;

 /*** cette variable nous permet de verifier qu'il y a bien exactement une et   une seule des trois fonctions de recuperation de donnes  immplementee ***/

   int test=0;

#ifdef DEBUG_ASSO
  printf("IN LOAD \n");
  printf("In LOAD myasso path : %s \n",myasso->path);
#endif

  if (myasso==NULL)
    {
      fprintf(stderr,"Dans MP_Load : tentative de chargement d'un plugin Null \n");
      return MPERR_CANNOT_LOAD_PLUG;
    }
      /*** On verifie que l'on a le droit de charger le plugin ***/ 

  if (myasso->NB_INSTANCES==0)

#ifdef DEBUG_ASSO
    {
      printf("NB_INSTANCE=0 \n");
#endif
    return MPERR_NB_INST;

#ifdef DEBUG_ASSO
    }
#endif

  if ((myasso->path)==NULL)
    {
      fprintf(stderr,"Dans MP_Load : tentative de charger une association avec un path null \n");
      return MPERR_CANNOT_LOAD_PLUG;
    }

  /*** On charge le plugin ***/
  ptr=dlopen(myasso->path,RTLD_LAZY);

  /*** On verifie que le chargement c'est bien deroule ***/
	
  if (ptr==NULL)
#ifdef DEBUG_ASSO
    {
      printf("IN LOAD mais ptr=NULL \n");
#endif
      return MPERR_CANNOT_LOAD_PLUG;
#ifdef DEBUG_ASSO
    }
#endif
  else
    
    {  
      /*** si cela c'est bien passe on recupere les pointeurs de fonctions ***/
      myasso->Handle=ptr;

#ifdef DEBUG_ASSO
      printf("Dans MP_Load myasso->Handle OK \n");
      printf("On va charger les fonctions obligatoires \n");
#endif

      myasso->MPP_New=(_MPP_New)dlsym(ptr,"MPP_New");
      if (myasso->MPP_New==NULL)
	{
	  dlclose(ptr);
	  myasso->Handle=NULL;

#ifdef DEBUG_ASSO
	  printf("Dans MP_Load : Impossible de charger MPP_New du plugin \n");
#endif
	  return MPERR_CANNOT_LOAD_PLUG;
	}

#ifdef DEBUG_ASSO
      else
	{
	  printf("MPP_New Charge \n");
	}
#endif
      
      myasso->MPP_Initialize=(_MPP_Initialize)dlsym(ptr,"MPP_Initialize");
      if (myasso->MPP_Initialize==NULL)
	{
	  dlclose(ptr);
	  myasso->MPP_New=NULL;
	  myasso->Handle=NULL;

#ifdef DEBUG_ASSO
	  printf("Dans MP_Load : Impossible de charger MPP_Initialize du plugin \n");
#endif
	  return MPERR_CANNOT_LOAD_PLUG;
	}

#ifdef DEBUG_ASSO
      else
	{
	  printf("MPP_Initialize Charge \n");
	}
#endif

      myasso->MPP_Destroy=(_MPP_Destroy)dlsym(ptr,"MPP_Destroy");
      if (myasso->MPP_Destroy==NULL)
	{
	  dlclose(ptr);
	  myasso->MPP_New=NULL;
	  myasso->MPP_Initialize=NULL;
	  myasso->Handle=NULL;
	  
#ifdef DEBUG_ASSO
	  printf("Dans MP_Load : Impossible de charger MPP_Destroy du plugin \n");
#endif
	  return MPERR_CANNOT_LOAD_PLUG;
	}

#ifdef DEBUG_ASSO
      else
	{
	  printf("MPP_Destroy Charge \n");
	}
#endif

         myasso->MPP_Demande=(_MPP_Demande)dlsym(ptr,"MPP_Demande");
      if (myasso->MPP_Demande==NULL)
	{
	  dlclose(ptr);
	  myasso->MPP_New=NULL;
	  myasso->MPP_Initialize=NULL;
	  myasso->Handle=NULL;
	  
#ifdef DEBUG_ASSO
	  printf("Dans MP_Load : Impossible de charger MPP_Demande du plugin \n");
#endif
	  return MPERR_CANNOT_LOAD_PLUG;
	}

#ifdef DEBUG_ASSO
      else
	{
	  printf("MPP_Demande Charge \n");
	}
#endif
       /***** chargement des fonctions de recuperaration des donnees      ****/
      /***** Si test=1 alors le plugin est ok si test>1 alors il y a     ****/
      /*****   trop de fonctions implementees et si test=0 aucune        ****/
      /*****             des foctions n'a ete implementee                ****/
      /***** si typecom=0 pas de com, si =1 par socket si =2 par fichier ****/

      myasso->MPP_NeedNothing=(_MPP_NeedNothing)dlsym(ptr,"MPP_NeedNothing");       
      if (myasso->MPP_NeedNothing!=NULL) 
	test++;
      
      myasso->MPP_NeedSocket=(_MPP_NeedSocket)dlsym(ptr,"MPP_NeedSocket");
      if (myasso->MPP_NeedSocket!=NULL) 
	test++;
      
      myasso->MPP_NeedFile=(_MPP_NeedFile)dlsym(ptr,"MPP_NeedFile"); 
      if (myasso->MPP_NeedFile!=NULL) 
	test++;
      
      if(test==1) 
      {
	if (myasso->MPP_NeedNothing!=NULL) 
	  { 
#ifdef DEBUG_ASSO
	    fprintf(stdout,"Le plug n'a pas besoin de Mosaic pour recuperer ses donnees \n");
#endif
	    myasso->typecom=0;
	  }

	if (myasso->MPP_NeedSocket!=NULL)  
	  {
#ifdef DEBUG_ASSO
	    printf("Le plug recuperera les donnees dont il a besoin via une socket \n");	
#endif
	    myasso->typecom=1;
	  }

	if (myasso->MPP_NeedFile!=NULL)   
	  {
#ifdef DEBUG_ASSO
	    printf("Le plug recuperera les donnees dont il  a besoin via un fichier \n");
#endif
	    myasso->typecom=2;
	  }

      }
      else
	{
	  dlclose(ptr); 
	  myasso->MPP_New=NULL;
	  myasso->MPP_Initialize=NULL;
	  myasso->MPP_Destroy=NULL;
          myasso->Handle=NULL;
#ifdef DEBUG_ASSO
	  if (test==0) 
	    printf("aucune des trois fonctions de recuperation des donnees n'a ete impolementees !! \n");
          else 
	    printf("On ne peut choisir que un des trois modes offerts pour recuperer des donnees \n");
#endif
          return MPERR_CANNOT_LOAD_PLUG;

	}
#ifdef DEBUG_ASSO
      printf("My Handle est : 0x%x \n",myasso->Handle);
#endif
      return MPERR_NO_ERROR;
    }
}

/****************************************************/
/*** MP_UNLOAD : decharge le plugin de la memoire ***/
/*** Si NB_INSTANCES!=0 la fonction retourne      ***/
/*** MPERR_STILL_INSTANCE et ne decharge pas la   ***/
/*** memoire sinon retourne MPERR_NO_ERROR        ***/
/****************************************************/

MP_ERROR MP_UNLoad(MP_ASSO *myasso)
{
 MP_MEMORY *mem,*mem1;
  if (myasso==NULL)
    {
      fprintf(stderr,"Dans MP_UNLOAD : myasso =NULL \n");
      return MPERR_NO_ASSO;
    }

  if (myasso->NB_INSTANCES!=0) 
    return MPERR_STILL_INSTANCE;

  /**** Destruction du pluginfo ******/

  mem=myasso->plug_info;
 
  while(mem!=NULL)
     { 
        mem1=mem->next;
	if (mem->memory!=NULL)
	  free(mem->memory);
	free(mem);
	mem=mem1;
     }

#ifdef DEBUG_ASSO
      printf("IN MP_UNload \n");
      printf("My Handle = 0x%x \n",myasso->Handle);
#endif

      dlclose(myasso->Handle);
      myasso->Handle=NULL;
      myasso->MPP_New=NULL;
      myasso->MPP_Initialize=NULL;
      myasso->MPP_Demande=NULL;
      myasso->MPP_Destroy=NULL;
      myasso->MPP_NeedSocket=NULL;
      myasso->MPP_NeedFile=NULL;
      myasso->MPP_NeedNothing=NULL;
      
  return MPERR_NO_ERROR;
}

/******************************************************/
/*** MP_Is_In_ASSO : verifie si le type appartient  ***/
/*** a l'association si c'est le cas la fonction    ***/
/*** retourne MPERR_NO_ERROR sinon elle retourne    ***/
/*** MPERR_TYPE_NOT_FIND                            ***/
/******************************************************/

MP_ERROR MP_Is_In_ASSO(MP_ASSO *myasso,char *type)
{
  if (myasso==NULL) 
    {
      fprintf(stderr,"Dans MP_Is_In_ASSO myasso=NULL\n");
      return MPERR_NO_ASSO;
    }
  else
    {
#ifdef DEBUG_ASSO
      printf("myasso->type = %s et type recherche = %s \n",myasso->type,type);
#endif
      if (strstr(myasso->type,type)==NULL)
	return MPERR_TYPE_NOT_FIND;
      else
	return MPERR_NO_ERROR;  
    }
}

/**************************************************************/
/*** Les methodes associees a la structure MP_PLUG         ****/
/**************************************************************/

/**************************************************************/
/*** La methode MP_Create_PLUG : retourne le pointeur sur la **/
/*** pile de plugin                                         ***/
/**************************************************************/

MP_PLUG *MP_Create_PLUG()
{
  MP_PLUG *mypile;

  mypile=(MP_PLUG*)malloc(sizeof(MP_PLUG));
  
  if (mypile!=NULL)
    {
      mypile->head=NULL;
      mypile->ID_PLUG=0;
    
#ifdef DEBUG_PILE
  printf("OK pour la creation de la pile \n");
#endif
    }
  return mypile;
}

/**************************************************************/
/*** MP_Add_ASSO : ajoute a la pile de plugin une nouvelle  ***/
/*** association. si tout ce passe bien cette fonction      ***/
/*** retourne MPERR_NO_ERROR sinon  MPERR_CANNOT_CREATE_ASSO **/
/**************************************************************/

MP_ERROR MP_Add_ASSO(MP_PLUG *mypile,char *type, char *path)
{
  MP_ASSO *myasso;

  if ((mypile==NULL)||(type==NULL)||(path==NULL))
    {
      fprintf(stderr,"Dans MP_Add_ASSO : mypile=null\n");
      return MPERR_ERROR;
    }
  
  /*** Creation de l'association ***/

  myasso=MP_Create_ASSO(type,path,mypile->ID_PLUG);
  
  if (myasso==NULL) 
    return MPERR_CANNOT_CREATE_ASSO;
  else
    {
      mypile->ID_PLUG++;

       /*** On insere l'association dans la pile ***/

      if (mypile->head==NULL)
	mypile->head=myasso;
      else
	{
	  myasso->next=mypile->head;
	  mypile->head=myasso;	  
	}
  return MPERR_NO_ERROR;
    }
}

/******************************************************/
/****  MP_Add_PLUG_Inst : ajout d'une instance au   ***/
/**** plugin. si celui-ci n'est pas charge cette    ***/
/**** fonction le charge en memoire                 ***/
/**** Retour : MPERR_NO_ERROR si aucun probleme     ***/
/**** sinon MPERR_TYPE_NOT_FIND                     ***/
/******************************************************/
  
MP_ERROR MP_Add_PLUG_Inst(MP_PLUG *myplug,char *type)
{
  MP_ASSO *myasso;

  if ((myplug==NULL)||(type==NULL))
    {
      fprintf(stderr,"Dans MP_Add_PLUG_Inst : muplug=NULL \n");
      return MPERR_ERROR;
    }

  myasso=myplug->head;
  while (myasso!=NULL)
    {
      if ( MP_Is_In_ASSO(myasso,type)==MPERR_NO_ERROR)
	{
	  myasso->NB_INSTANCES++;

#ifdef DEBUG_PILE
	  printf("Dans MP_Add_PLUG_Inst Nombre d'instances de type %s = %d \n",myasso->type,myasso->NB_INSTANCES);
#endif
	  if (myasso->NB_INSTANCES==1)
	   if (MP_Load(myasso)==MPERR_CANNOT_LOAD_PLUG ) 
	      return MPERR_CANNOT_LOAD_PLUG;
	  else
	    {
	      /*** Apres le chargement il faut faire appel a la fonction MPP_Initialize *****/ 
	      /*** Il est necessaire de lui passer myasso en parametre                  *****/
	     
	      if (myasso->MPP_Initialize!=NULL)
		(*(myasso->MPP_Initialize))((void*)myasso);
	    }

	  break;
	}
      myasso=myasso->next;
    }
  if (myasso==NULL) 
    return MPERR_TYPE_NOT_FIND;
  
  return MPERR_NO_ERROR;
  
}

/*************************************************************/
/*** MP_Del_PLUG_Inst : suppression d'une instance         ***/
/*** Si le nombre d'instance est null on decharge le plugin **/
/*** retourne MPERR_NO_ERROR si ok sinon                   ***/
/*** MPERR_TYPE_NOT_FIND                                   ***/
/*************************************************************/

MP_ERROR MP_Del_PLUG_Inst(MP_PLUG *myplug,char *type)
{
  MP_ASSO *myasso;

  if (myplug==NULL)
    {
      fprintf(stderr,"Dans MP_Del_PLUG_Inst : muplug=NULL \n");
      return MPERR_ERROR;
    }

  myasso=myplug->head;
  while (myasso!=NULL)
    {
      if ( MP_Is_In_ASSO(myasso,type)==MPERR_NO_ERROR)
	{
	  myasso->NB_INSTANCES--;
#ifdef DEBUG_PILE
	  printf("dans MP_Del_PLUG_Inst Nombre d'instances de type %s = %d \n",myasso->type,myasso->NB_INSTANCES);
#endif

	  if (myasso->NB_INSTANCES==0)
	    MP_UNLoad(myasso);
	  break;
	}
      myasso=myasso->next;
    }
  if (myasso==NULL) 
    return MPERR_TYPE_NOT_FIND;

  else return MPERR_NO_ERROR;
  
}

/*************************************************************/
/****  la suppression de la pile                          ****/
/*************************************************************/

MP_ERROR MP_Del_All(MP_PLUG *myplug)
{
  MP_ASSO *p, *p1;


#ifdef DEBUG_PILE
  printf("In DEL_ALL \n");
#endif

 
  if (myplug==NULL)
    {
      fprintf(stderr,"In MP_Del_All myplug est a NULL \n");
      return MPERR_ERROR;
    }

  p=myplug->head;

  while(p!=NULL)
    {
      p1=p->next;
#ifdef DEBUG_PILE
      printf("On va detruire %s ",p->type);
#endif
      if(MP_Delete_ASSO(p)==MPERR_STILL_INSTANCE)
	{

#ifdef DEBUG_PILE
	  printf("dans MP_Del_All,ily a encore des instances");
#endif

	  myplug->head=p;
	  return MPERR_STILL_INSTANCE;
	}
      else
	{
#ifdef DEBUG_PILE  
	  printf("destruction Ok \n");
#endif

	  p=p1;
	}
    }
  return MPERR_NO_ERROR;
}


/*************************************************************/
/*** MP_Getmyplug retourne le pointeur sur le plugin associe */
/*** au type type                                          ***/
/*** Si il ne trouve pas le type il retourne NULL          ***/
/*************************************************************/

MP_ASSO *MP_Getmyplug(MP_PLUG *myplug,char *type)
{

  MP_ASSO *myasso;

  if (myplug!=NULL)
    {
      myasso=myplug->head;
      while (myasso!=NULL)
	{
	  if (MP_Is_In_ASSO(myasso,type)==MPERR_NO_ERROR)
	    break;
	  else
	    myasso=myasso->next;
	}

      return myasso;
    }
  else
    return NULL;
}

/******************************************************************/
/**** MP_Create_inst : creation d'une instance et initialisation **/
/**** des parametres relatifs au plugin                          **/
/******************************************************************/

MP_Private_Inst *MP_Create_Inst(MP_PLUG *myplug, char *type)
{
  MP_Private_Inst *myinst;
  char *mytype;

#ifdef DEBUG_PILE
  printf("In MP_Create_Inst on va creer une instance de type %s \n",type);
#endif 

  myinst=NULL;

  if (MP_Add_PLUG_Inst(myplug,type)==MPERR_NO_ERROR)
    {
      myinst=(MP_Private_Inst*)malloc(sizeof(MP_Private_Inst));
      
      if (myinst==NULL)
	{
	  fprintf(stderr,"In Create Inst : mytinst NULL\n");
	  return NULL;
	}
      else
	{
	  myinst->classid=NULL;
	  myinst->PARAM=NULL;
	  myinst->codebase=NULL;
	  myinst->data=NULL;
	  myinst->standby=NULL;
	  myinst->myWindow=NULL;
	  myinst->height=0;
	  myinst->width=0;
	  myinst->myfile=NULL;
	  myinst->Socket=-1;

	  mytype=(char*)malloc(sizeof(char)*(strlen(type)+1));
	  if (mytype==NULL)
	    {
	      fprintf(stderr,"In Create_Inst mytype null\n");
	      free(myinst);
	      return NULL;
	    }
	  else 
	    {
	      strcpy(mytype,type);
	      myinst->type=mytype;
	    }

	  myinst->myplug=MP_Getmyplug(myplug,type);
	  if (myinst->myplug==NULL)
	    {
	      free(myinst->type);
	      free(myinst);
	      return NULL;
	  
	    }
	}
    }
  return myinst;
  
}




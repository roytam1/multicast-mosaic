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

#include <unistd.h>

#include "com_struct.h"
#include "private.h"
#include "public_struct.h"


FILE* MP_Get_Fichier(MP_Private_Inst *mypinst, MPX_CONTEXT *cont);
int MP_Determine_Typecom(MP_Private_Inst *mypinst);
int MP_Create_Socket(MP_Private_Inst *mypinst, MPX_CONTEXT *cont);


/*****************************************************************************/
/*** Fonction qui construit la table d'association : la valeur retorunee  ****/
/*** est un pointeur sur la pile d'association                            ****/
/*** si cette valeur est NULL c'est qu'il n'y a pas de structure          ****/
/*****************************************************************************/

/* Cette version ne fonctionne pas */

MPX_PLUGIN MP_GetMINEDescription()
{
  MP_PLUG *myplug;
  MP_ASSO *myasso;
  char string[100]="";
  char Path[30]="../plugins/myplug/"; 
  char Patht[100]=" ";
  char fichier[100];
  FILE *stock_myplug;
  char commande[100];

  /**************************************************************************/
  /***** Path doit contenir le chemin pour acceder au repertoire ou   *******/
  /*****                  sont stockes les plugins                    *******/
  /**************************************************************************/
#ifdef DEBUG_PILE  
  printf("Vous etes dans MP_GetMINED...\n");
  printf("le Path est %s\n",Path);
#endif
  strcpy(commande,"ls ");
  
  strcat(commande,Path);
#ifdef DEBUG_PILE 
  printf("la commande est %s\n",commande); 
#endif

  myplug=MP_Create_PLUG(); 

   stock_myplug=popen(commande,"r");  
   if(stock_myplug == NULL) 
     { 
#ifdef DEBUG_PILE
       printf("je suis dans MP_GetMined et il y a eu un probleme d'ouverture de fichier\n"); 
#endif
    } 
   else  
     { 
       myasso=(MP_ASSO*)malloc(sizeof(MP_ASSO)); 
       myasso->NB_INSTANCES=-1; 
       myasso->Handle=NULL;   
       myasso->MPP_New=NULL;   
       myasso->MPP_Demande=NULL;
       myasso->MPP_Initialize=NULL;
       myasso->MPP_Destroy=NULL;
       myasso->MPP_NeedSocket=NULL;
       myasso->MPP_NeedFile=NULL;
       myasso->MPP_NeedNothing=NULL;
       myasso->next=NULL;
       myasso->plug_info=NULL; 
       myasso->plug_info_Queue=NULL;

       myasso->path=(char*)malloc(sizeof(char)*200); 

        while(fgets(fichier,100000,stock_myplug)) 
	  { 
	    if( strstr(fichier,".so") != NULL )
	      {
		strcpy(Patht,Path);
#ifdef DEBUG_PILE
		printf("MY patht :%s\n",Patht);
#endif
		strcat(Patht,fichier);
		strncpy(myasso->path,Patht,(strlen(Patht)-1));
		strcat( myasso->path,"");
#ifdef DEBUG_PILE
		printf("My path :%s\n",myasso->path);
#endif
	        if (MP_Load(myasso)==MPERR_NO_ERROR)
		  { 
		    if ((myasso->MPP_Demande)!=NULL)  
		      { 
			 (*(myasso->MPP_Demande))(string); 
		 	MP_Add_ASSO(myplug,string,myasso->path);
			
		       }
		    
		   }

	      }
	    
	  }
  
     }  
#ifdef DEBUG_PILE
   
   printf("Je sors du Get_MINED\n");
#endif
 
  return (MPX_PLUGIN)myplug;
}

/* MPX_PLUGIN MP_GetMINEDescription() */
/* { */
/*   MP_PLUG *myplug; */
/*   MP_ASSO *myasso; */
/*   char string[100]=""; */
/*   char Path[18]="../plugins/myplug/";  */
/*   FILE *stock_myplug; */
/*   char commande[100]; */

  /**************************************************************************/
  /***** Path doit contenir le chemin pour acceder au repertoire ou   *******/
  /*****                  sont stockes les plugins                    *******/
  /**************************************************************************/
  
 
/*   stock_myplug=popen(ls,"r");  */
/*   if(stock_myplug == NULL) */
/*     { */
/*       printf("je suis dans MP_GetMined et il y a eu un probleme d'ouverture de fichier\n"); */
/*     } */
/*   else  */
/*     { */
/*       while(fgets(fichier,100000,stock_myplug)) */
/* 	{ */
/* 	  if( strstr(fichier,".so") != NULL ) printf("Le fichier %s est un .so\n",fichier);  */
/* 	} */
/*   printf("In GET MINE\n"); */
/*   myasso=(MP_ASSO*)malloc(sizeof(MP_ASSO)); */
/*   myplug=MP_Create_PLUG(); */
/*   myasso->NB_INSTANCES=-1; */
  


/*   myasso->path=strdup("../plugins/myplug/myplug.so"); */

 
/*    if (MP_Load(myasso)==MPERR_NO_ERROR) */
/*      {  */
/*        if ((myasso->MPP_Demande)!=NULL)  */
/*  		(*(myasso->MPP_Demande))(&string);  */
       
/*        printf(" in GETMINE %s \n",string); */
/*      } */

/*    MP_Add_ASSO(myplug,"image/gif","../plugins/myplug/myplug.so");  */
   /**************************************************/

/*   myplug=MP_Create_PLUG(); */
/*   MP_Add_ASSO(myplug,"image/gif","../plugins/myplug/myplug.so");  */
/*  /*  MP_Add_ASSO(myplug,"image/jpeg","../plugins/myplug/myplug1.so");  */ 
/*  printf("Je sors du Get_MINED\n"); */
   
/*   return (MPX_PLUGIN)myplug; */
/* } */

/******************************************************************************/
/**** Fonction qui cree une nouvelle instance de type char. les parametres  ***/
/**** d'entrees sont la pile de plugin et le type. la valeur retournee est  ***/
/**** un pointeur sur l'instance.                                           ***/
/**** Cette fonction fait appel a la fonction MPP_New du plugin             ***/
/******************************************************************************/

MPX_INSTANCE *MP_New(MPX_PLUGIN myplug, MPX_CONTEXT *cont, char *type)
{
  MP_Private_Inst *mypinst;
  MPX_INSTANCE *myinst;
  MP_ASSO *myasso;

  ThePARAM *param;

  FILE *ad_fichier;
  int comm;
  int test_socket;
  myinst=NULL;
                       

  if (type!=NULL)
    {
      mypinst=MP_Create_Inst((MP_PLUG*)myplug,type);
      if (mypinst!=NULL)
	{
       
	  myinst=(MPX_INSTANCE*)malloc(sizeof(MPX_INSTANCE));
	  if (myinst!=NULL)
	    {

	      myinst->Plugin_Data=NULL;
	      myinst->MOSAIC_Data=(void*)mypinst;
	      myasso=mypinst->myplug;

	      /*********** Recuperation des donnees ***************/
	      
	      comm = MP_Determine_Typecom(mypinst);
              
	      if( comm == 2 ) /**recuperation par fichier**/
		{		
		  ad_fichier   = MP_Get_Fichier(mypinst,cont);
                  if(ad_fichier != NULL)
		    {
		      mypinst->myfile = ad_fichier; 
#ifdef DEBUG_PILE
		      if(ad_fichier==NULL) printf("je suis dans MP_New et le file descriptor du fichier dans lequel il y a les donnees est nul\n");
		      else 
			{
			  printf("je suis dans MP_New et ca y est j'ai passe le file descriptor a l'instance\n"); 
		      
			}
#endif
		    }
		  else
		    {
		      myasso->NB_INSTANCES=myasso->NB_INSTANCES-1;
		      free(myinst);
		      return NULL;
		    }
		}
	      else
		{
		  if(comm==1) /*** recuperation par socket  ***/
		    { 
		      
		      test_socket = MP_Create_Socket(mypinst,cont);
		      if(test_socket != -1 )
			{
			  mypinst->Socket=test_socket;
#ifdef DEBUG_PLUG
			  printf("\n Je suis dans MP_New et je dois passer la valeur de retour de la socket creee %d au plugin\n\n",mypinst->Socket);
#endif
			}
		      else
			{
			  myasso->NB_INSTANCES=myasso->NB_INSTANCES-1;
			  free(myinst);
			  return NULL;
			}
	     
		    }
                  else /*** besoin de rien**/;
                }
	    
	      /****** Avant d'appeller le plugin il faut tout initialiser ******/
	      
	      
	    if (cont->classid!=NULL) mypinst->classid  = strdup(cont->classid); 
	    if (cont->standby!=NULL) mypinst->standby   = strdup(cont->standby);
	    if (cont->codebase!=NULL) mypinst->codebase = strdup(cont->codebase);
	    if (cont->data!=NULL) mypinst->data = strdup(cont->data);

#ifdef DEBUG_PILE
	     if (mypinst->classid!=NULL) printf("MY class %s \n", mypinst->classid);	     
 	    if (mypinst->codebase!=NULL) printf("MY codebase %s \n", mypinst->codebase);
	     if (mypinst->data!=NULL) printf("MY data %s \n", mypinst->data);
#endif
	      
	  
	   /*  if (cont->type) mypinst->type = strdup(cont->type); */
	    
	    mypinst->MOSAICWin= cont->MOSAICWin;
	    mypinst->width    = cont->width;
	    mypinst->height   = cont->height;
	    mypinst->border   = cont->border;
	    mypinst->ID       = cont->ID;
	    mypinst->x        = cont->x;
	    mypinst->y        = cont->y;
	    mypinst->PARAM    = cont->PARAM;
	
	    
       
	   /*  printf("type lu par le plugin %s\n",cont->type); */
	    param=mypinst->PARAM;
	   
	       if ((myasso->MPP_New)!=NULL) 
 		(*(myasso->MPP_New))(myinst); 
 	    


	    }
	}
    }
  
  return myinst;
}

/*****************************************************************/
/*** FOnction MP_Destroy : Destruction d'une instance          ***/
/*** Parametre d'entree le pointeur sur l'instance a detruire  ***/
/*****************************************************************/

void MP_Destroy(MPX_PLUGIN myplug, MPX_INSTANCE *myinst)
{
  
  MP_ASSO *myasso; 
  MP_Private_Inst *mypinst; 
  ThePARAM *param,*param1;
/*  Widget frame; */
/*  XClientMessageEvent ev; */
  Atom delete_atom=0;
  Atom proto_atom=0;
  char path[256];
  if ( myinst == NULL ) 
    return ; 
  if ( myplug == NULL )
    return ;
  if (myinst!=NULL)  
    {  
       mypinst=(MP_Private_Inst*)(myinst->MOSAIC_Data);  
       myasso=mypinst->myplug;  
      
      /********* On fait d'abord appel a MPP_Destroy ****/ 
#ifdef DEBUG_PILE 
      printf("On va appeller MPP_Destroy du plugin \n"); 
#endif 

        
       if ((myasso->MPP_Destroy)!=NULL) 
	 (*(myasso->MPP_Destroy))(myinst); 
       
     /******** si l'instance a ouvert un fichier , il faut le fermer et le detruire  ****/
#ifdef DEBUG_PILE
       printf("je vais essayer de detruire les fichiers\n");
#endif
       if(myasso->typecom == 2 )
	  {
	    if( mypinst->myfile != NULL)
	      {
#ifdef DEBUG_PILE
		printf("je vais fermer le fichier Num %d\n", mypinst->myfile);
#endif
		fclose(mypinst->myfile);
#ifdef DEBUG_PILE
	        printf("J ai ferme le fichier");
#endif
                strcpy(path,"rm  ");
	        strcat(path,mypinst->Nom_Fichier); 
#ifdef DEBUG_PILE
                printf("IN MP_Destroy la commande pour detruire le fichier est %s\n",path);
#endif
		system(path);  
#ifdef DEBUG_PILE
	        printf("je suis dans MP_Destroy et je viens de detruire le fichier dont le nom est %s\n",mypinst->Nom_Fichier);
#endif
	          
	      } 
	  } 
       /******** si l'instance a necessite une socket il faut la fermer      ****/ 
	if(myasso->typecom == 1 ) {
#ifdef DEBUG_PILE
	   printf("je suis dans MP_Destroy et je veux la  fermer la socket %d\n",mypinst->Socket);
#endif
	   
	   if( mypinst->Socket != -1) {
	       close(mypinst->Socket);
#ifdef DEBUG_PILE
	       printf("je suis dans MP_Destroy et je viens de fermer la socket\n");
#endif
	   } 
	} 
       /***** On detruit la fenetre si elle exite encore ******/
	/*    frame=mypinst->myWindow; */
/*        if (frame!=NULL) */
/* 	 { */
/* 	   if (!delete_atom) */
/* 	     delete_atom=XInternAtom(XtDisplay(frame),"WM_DELETE_WINDOW",False); */
/* 	   if (!proto_atom) proto_atom= XInternAtom(XtDisplay(frame),"WM_PROTOCOLS",False); */
/* 	   ev.type=ClientMessage; */
/* 	   ev.window=XtWindow(frame); */
/* 	   ev.message_type=proto_atom; */
/* 	   ev.format=32; */
/* 	   ev.data.l[0]=delete_atom; */
/* 	   ev.data.l[1]=CurrentTime; */
/* 	   XSendEvent(XtDisplay(frame),XtWindow(frame),True,0x00ffffff,(XEvent *)&ev); */
/* 	   XFlush(XtDisplay(frame)); */
/* 	   XtSetMappedWhenManaged(frame,False); */
/* 	   XFlush(XtDisplay(frame)); */
/* 	   XtDestroyWidget(frame); */
/* 	 } */
     
       /******** Ne pas oublier de detruire le plugData **********/

       if (myinst->Plugin_Data!=NULL) free (myinst->Plugin_Data); 
       if (mypinst->classid!=NULL) free(mypinst->classid); 
       /*if (mypinst->type!=NULL) free(mypinst->type); */
       if (mypinst->codebase!=NULL) free(mypinst->codebase); 
       if (mypinst->data!=NULL) free(mypinst->data); 
       if (mypinst->standby!=NULL) free(mypinst->standby); 

       MP_Del_PLUG_Inst((MP_PLUG*)myplug,mypinst->type);
       free(mypinst->type);

       /******** reste a supprimer PARAM  ********/
       param=mypinst->PARAM;
       while (param!=NULL)
	 {
	   param1=param->next;
	   if (param->NAME!=NULL) free(param->NAME);
	   if (param->VALUE!=NULL) free(param->VALUE);
	   if (param->VALUETYPE!=NULL) free(param->VALUETYPE);
	   free(param);
	   param=param1;
	 }
       

       
       /******** Om supprime l'instance privee et la publique *****/

       free(mypinst);
       free(myinst);
     
     }  
}

/*******************************************************/
/**** MP_SetValue est la fonction de communication *****/
/*******************************************************/

MP_ERROR MP_SetValue(MPX_INSTANCE *myinst, MPX_SET set,void *val)
{
  MP_Private_Inst *mypinst;

 

  if (myinst==NULL)
    return MPERR_NO_INST ;
  else
    {
      mypinst=(MP_Private_Inst *)(myinst->MOSAIC_Data);
      switch (set)
	{
	case X : 
	  {
	    mypinst->x=*((int*)val);
	    break;
	  }
	case Y :
	  { 
	     mypinst->y=*((int*)val);
	    break;
	  }

	case WIDTH :
	  {
	    mypinst->width=*((int*)val);
	    break;
	  }
	case HEIGHT :
	  {
	    mypinst->height=*((int*)val);
	    break;
	  }

	default: 
	  return MPERR_NO_ACCESS;
	  

	}
      return MPERR_NO_ERROR;
    }
}

/****************************************************/

void * MP_GetValue(MPX_INSTANCE *myinst, MPX_SET set)
{
  MP_Private_Inst *mypinst;
  int *i;

  if (myinst==NULL) 
    return NULL;
 
  else
    {
      mypinst=(MP_Private_Inst*)myinst->MOSAIC_Data;
      switch (set) 
	{
	case  HEIGHT :
	  {
	    i=(int*)malloc(sizeof(int));
	    *i=(mypinst->height);
	    return (void*)i;
	    break;
	  }
	  case  X :
	  {
	    i=(int*)malloc(sizeof(int));
	    *i=(mypinst->x);
	    return (void*)i;
	    break;
	  }
	   case  Y :
	  {
	    i=(int*)malloc(sizeof(int));
	    *i=(mypinst->y);
	    return (void*)i;
	    break;
	  }
	case  WIDTH :
	  {  
	    i=(int*)malloc(sizeof(int));
	    *i=(mypinst->width);
	    return (void*)i;
	    break;
	  }
	 
	case   ID :
	  {
	    i=(int*)malloc(sizeof(int));
	    *i=(mypinst->ID);
	    return (void*)i;
	    break;
	  }
	 
	case WINDOW :
	  {
	    return mypinst->myWindow;
	    break;
	  }

	default: 
	  return NULL;
	  
	}
      return NULL;
    }
}
/*****************************************************************************//********** MP_Determine_Typecom determine le type de recuperation des *******/
/********** donnees                                                     ******/
/*****************************************************************************/
int MP_Determine_Typecom(MP_Private_Inst *mypinst)
{
 int i;
 
 i=(mypinst->myplug)->typecom;
#ifdef DEBUG_PILE
 printf(" la valeur de i est : % d \n",i);
 switch(i)
   {
   case 0 : 
     {
       printf("je suis dans MP_New et le plugin n a besoin de rien pour recuperer ses donnees\n"); 
     } 
     break;
   case 1 : 
     {
       printf("je suis dans MP_New et le plugin souhaite que mMosaic lui ouvre une socket\n");
     }
     break;
   case 2 : 
     {
       printf("je suis dans MP_New et le plugin souhaite que mMosaic lui donne ses donnees par fichier\n");
     }  break;
   }
#endif
 return i ;
}

/*****************************************************************************//********** MP_Get_Fichier permet de recuperer le fichier et le met dans******/
/********** le fichier temporaire identifie par temp_numeroID           ******/
/*****************************************************************************/

FILE* MP_Get_Fichier(MP_Private_Inst *mypinst, MPX_CONTEXT *cont)
{
 
  int test;
  char www[5]="www." ;
  FILE *fich_temp;
  char nom_fichier[100];
  char path[256];
  char *adresse_fichier;
  
  /******* determinons ou aller chercher le fichier contenant *******/
  /*******           les donnees pour l'instance              *******/ 

/*  mypinst->ID=111; */
  
  sprintf(nom_fichier,"temp_%d_%d",mypinst->myplug->ID,mypinst->myplug->NB_INSTANCES); /*** obtention du nom du fichier temporaire ou seront stockees les donnees***/
  
  strcpy(mypinst->Nom_Fichier,nom_fichier);
  
  adresse_fichier=cont->data; /** recuperation du champ data exple ens.fr/~mach                               in/truc.gif ****/ 
  if(adresse_fichier == NULL )
    {
#ifdef DEBUG_PILE
      printf(" Il n'est pas precise dans la page HTML ou aller chercher les donnees dont l'instance a besoin\n");
#endif
      return NULL;
    }
  else
    {
      strcat(www,adresse_fichier);
      adresse_fichier=www;
#ifdef DEBUG_PILE
      printf("les donnees a recuperer se trouvent a l'adresse %s\n",adresse_fichier);
#endif
      strcpy(path,"wget ");
      strcat(path,adresse_fichier);
      strcat(path," -O ");
      strcat(path,nom_fichier);
      test=system( path);  /*** commande system pour effectuer le wget du fichier rediriger dans le fichier temporaire***/
#ifdef DEBUG_PILE
      printf("la valeur de retour de system est %d\n\n",test);
      printf("je suis dans MP_New et j'ai mis les donnees dans le fichier dont le nom est %s \n",mypinst->Nom_Fichier);
#endif
      fich_temp=fopen(nom_fichier,"r");
      if(fich_temp == NULL )
	{
#ifdef DEBUG
	  printf("Il y a eu un probleme dans l'ouverture d'un fichier\n");
          printf("Je ne vais donc pas creer cette instance\n");
#endif
	  return NULL;
	}
      else
	{
#ifdef DEBUG_PILE
	  printf("Je suis dans MP_New et le file descripteur que je passe au plugin est : %d\n\n",fich_temp);
#endif
	  return(fich_temp); /*** on retourne le descripteur du fichier temporaire**/}
    }
}

/*******************************************************/
/****            creation d'une socket             *****/
/*******************************************************/

int MP_Create_Socket(MP_Private_Inst *mypinst, MPX_CONTEXT *cont)
{
  struct sockaddr_in Le_Serveur;
  struct hostent        *ph;
  int           Sock_Des;

  
  if (cont->codebase==NULL) 
    { /** recuperation du codebase exple www.enst.fr ****/
#ifdef DEBUG_PILE
      printf("codebase n'est pas implemente,on ne va donc pas creer cette instance\n");
#endif
	return -1;
       
    }
  /* Ouverture de la socket ****/

  if ((Sock_Des = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
#ifdef DEBUG_PILE
      printf("Erreur d'ouverture du socket\n");
#endif
      return -1;
  }

/*** on attache la socket au serveur i.e la machine distante ou se trrouvent les donnees a recuperer***/
  Le_Serveur.sin_family = AF_INET;
  if ((ph = gethostbyname(cont->codebase)) == (struct hostent *) 0) {
      fprintf(stderr, "%s: Hote inconnu\n", cont->codebase);
      return -1 ;
  } /**** recuperation de l'adresse IP de la machine distante ******/
  memcpy((char *) &Le_Serveur.sin_addr, (char *) ph->h_addr, ph->h_length);
  Le_Serveur.sin_port = htons(80);

/***mMosaic se connect au serveur et retourne le descripteur de socket *****/
  if (connect(Sock_Des,(struct sockaddr *)&Le_Serveur, sizeof(Le_Serveur)) < 0)
    {
      perror("Erreur de connexion sur le socket serveur"); 
       return -1 ;
    }
#ifdef DEBUG_PILE
 else printf("Je suis dans MP_New et je me suis CONNECTE a l'adresse ou se trouve le fichier\n\n");
#endif

  return Sock_Des;
}

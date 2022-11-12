
#include <stdio.h>
#include <stdlib.h>
#include "pluginM.h"



void *MPP_GetValue(MPX_INSTANCE *myinst, MPX_SET set , void *val)
{
  MP_Private_Inst *mypinst;

  if (myinst==NULL) 
    return NULL;

  mypinst=(MP_Private_Inst*)myinst->MOSAIC_Data;
  switch (set) {
    case X : 
      *((int*)val)=(mypinst->x);
      break;
    case Y : 
      *((int*)val)=(mypinst->y);
      break;
    case HEIGHT : 
      *((int*)val)=(mypinst->height);
      break;
    case WIDTH : 
      *((int*)val)=(mypinst->width);
      break;
   
    case myFILE :
     *((FILE **)val)=(mypinst->myfile);
      break;
      
    case SOCKET : 
      *((int*)val)=(mypinst->Socket);
      break;
    
    case BORDER :
      *((int*)val)=(mypinst->border);
      break;
    case ID :
      *((int*)val)=(mypinst->ID);
      break;
    case MOSAICwin :
      return (void*)(mypinst->MOSAICWin);
      break;
      
    case WINDOW :
      return (void*)(mypinst->myWindow);
      break;

    case PARAM :
      return (void*)(mypinst->PARAM);
      break;

    case STANDBY :
      return (void*)(mypinst->standby);
      break;  
      
    case DATA :
      return (void*)(mypinst->data);
      break;    

    case CODEBASE :
      return (void*)(mypinst->codebase);
      break;  

    case CLASSID :
      return (void*)(mypinst->classid);
      break;  

    case TYPE :
      return (void*)(mypinst->type);
      break;  
  
    default:
      return NULL;
    }
    return NULL;
}


MP_ERROR MPP_SetValue(MPX_INSTANCE *myinst, MPX_SET set , void *val)
{
  MP_Private_Inst *mypinst;

   if (myinst==NULL)
    return MPERR_NO_INST ;

   mypinst=(MP_Private_Inst *)myinst->MOSAIC_Data;
      switch (set)
	{
	case WINDOW :
	  {
	    mypinst->myWindow=(Widget)val;
	    return MPERR_NO_ERROR;
	    break;
	  }
	case HEIGHT :
	  {
	    mypinst->height=*((int*)(val));
	    break;
	  }
	case WIDTH :
	  {
	    mypinst->width=*((int*)(val));
	  }
	default:
	  MPERR_NO_ACCESS;

	}
}


/*************************************************************************/
/****** Allocation de memoire pour le plugin                          ****/
/*************************************************************************/


void *MPP_MemAlloc(MPX_Type type, MPX_MEM set, size_t size)
{
  
  MP_MEMORY *mem;
  MP_ASSO *myasso;
  MP_Private_Inst *mypinst;
  MPX_INSTANCE *myinst;

  if (type==NULL)
    {
      fprintf(stderr,"In MPP_MemAlloc le pointeur en parametre  est NULL");
      return NULL;
    }

  switch (set)
    {
    case PLUG : 
      myasso=(MP_ASSO*)type;
      break;

    case INSTANCE : 
      myinst=(MPX_INSTANCE*)type;
      mypinst=(MP_Private_Inst*)myinst->MOSAIC_Data;
      if(mypinst==NULL)
	{
	  fprintf(stderr,"In MPP_MemAlloc le pointeur mypinst est NULL");
	  return NULL;
	}
      myasso=mypinst->myplug; 
      if(myasso==NULL)
	{
	  fprintf(stderr,"In MPP_MemAlloc le pointeur myasso est NULL :le plugin n'a pas d'association dans la table");
	  return NULL;
	}
		    
    }
  
    
  mem=(MP_MEMORY *)malloc(sizeof(MP_MEMORY));

  if(mem==NULL)
       	 {
	   fprintf(stderr,"In MPP_MemAlloc :impossible d'allouer la memoire MP_MEMORY\n");
	   return NULL;
	 }
  mem->memory=malloc(size);

  if(mem->memory==NULL)
    {
      fprintf(stderr,"In MPP_MemAlloc_Plug :impossible d'allouer la memoire memory\n");
      free(mem);
      return NULL;
      
    }
	      
  if(myasso->plug_info==NULL)
    {
      myasso->plug_info=mem;
      myasso->plug_info_Queue=mem;
      mem->next=NULL;
    }
  else
    {
      mem->next=myasso->plug_info;
      myasso->plug_info=mem;
    }

   
  return mem->memory;
}
/*******************************************************/

void *MPP_GetMem(MPX_Type type ,MPX_MEM set)
{
  MP_MEMORY *mem;
  MP_ASSO *myasso;
  MP_Private_Inst *mypinst;
  MPX_INSTANCE *myinst;

  if (type==NULL)
    {
      fprintf(stderr,"In MPP_GetMem le pointeur en parametre  est NULL");
      return NULL;
    }

  switch (set)
    {
    case PLUG : 
      myasso=(MP_ASSO*)type;
      break;

    case INSTANCE : 
      myinst=(MPX_INSTANCE*)type;
      mypinst=(MP_Private_Inst*)myinst->MOSAIC_Data;
      if(mypinst==NULL)
	{
	  fprintf(stderr,"In MPP_GetMem le pointeur mypinst est NULL");
	  return NULL;
	}
      myasso=mypinst->myplug; 
      if(myasso==NULL)
	{
	  fprintf(stderr,"In MPP_GetMem le pointeur myasso est NULL :le plugin n'a pas d'association dans la table");
	  return NULL;
	}
		    
    }
  
  mem=myasso->plug_info_Queue;

  if (mem!=NULL)

    return mem->memory;
  else
    return NULL;

  
}

/*****************************************************/

MP_ERROR MPP_FreeMem(MPX_INSTANCE *myinst, void *mymem)
{
  MP_MEMORY *mem, *mem1;
  MP_ASSO *myasso;
  MP_Private_Inst *mypinst;

  if ((myinst==NULL)||(mymem==NULL))
    {
      fprintf(stderr,"In MPP_FreeMem,l'un des pointeurs entre en parametre de la fonction est NULL\n");
      return MPERR_ERROR;
    }
     
  mypinst=(MP_Private_Inst*)myinst->MOSAIC_Data;
  myasso=mypinst->myplug;	  
  if(myasso==NULL)
    {
      fprintf(stderr,"In MPP_MemAlloc_Plug le pointeur myasso est NULL :le plugin n'a pas d'association dans la table");
      return MPERR_ERROR;
    }
  mem=myasso->plug_info;
  mem1=NULL;
  if (mem==NULL)
    return MPERR_NO_MEMORY_TO_FREE;

 
  while(mem!=NULL)
    { 
  
       if(mymem==mem->memory)
	 {  
	  
	   free(mem->memory); 
	   if (mem1==NULL)
	     myasso->plug_info=mem->next;
	   else 
	     mem1->next=mem->next;
	  free(mem);
	  return MPERR_NO_ERROR;
	}
  
      mem1=mem;
      mem=mem->next;
    }
  return MPERR_MEMORY_NOT_FIND;
  
}



  

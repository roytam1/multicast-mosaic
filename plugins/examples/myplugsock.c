
/************************************************/
/**** Un exemple de plugin                    ***/
/************************************************/

#include <stdio.h>
#include <unistd.h>

#include "pluginM.h"
#include <string.h>
#include <Xm/PushB.h>
#include <Xm/Label.h>
#include <time.h>

#include <X11/StringDefs.h>

  
#define DEBUG_PLUG

typedef struct {
  XtIntervalId id;
  MPX_INSTANCE *myinst;
  XtAppContext app;
  int i;
  char *string;
  XtInputId *idI;
  int index;
}TimeClientData;


int Traite_Donnee(int sock,TimeClientData *time);


void MPP_Demande (char *type) 
{ 
  strcpy(type,"image/jpeg");
}

void mise(XtPointer cl,XtIntervalId *id)
{

  char *tmp2; 
  char TOURNE[1000]="";
  char tmp1[1000]="";
  Widget myWin;
  Widget MOSAIC;
  MPX_INSTANCE *p;
  TimeClientData *time1;
  XmString compose;
  int i;
  Arg args[10];
  char *chaine,*tmp;
  char thechaine[1000];
  long nsec,nsec_a,minute_suiv;

  time(&nsec);
  nsec_a=nsec;
  chaine=ctime(&nsec_a);
   
 
 
 
  minute_suiv=(60-nsec%nsec)*1000;
 
  time1=(TimeClientData*)cl;
  i=time1->index; 
  p=time1->myinst;
  tmp=time1->string;
  strcpy(thechaine,tmp);
  strcat(thechaine,chaine);

  if (i>=strlen(thechaine)) i=0;

  strncpy(tmp1,thechaine,i);
  tmp2=&(thechaine[i]);
  strncpy(TOURNE,tmp2,(strlen(thechaine)-i-1));
  strcat(TOURNE," ");
  strcat(TOURNE,tmp1);
  strcat(TOURNE,"\0");
  i++;

  time1->index=i;
  compose=XmStringLtoRCreate(TOURNE,XmSTRING_DEFAULT_CHARSET);
  myWin=(Widget)MPP_GetValue(p,WINDOW,NULL);
  MOSAIC=(Widget)MPP_GetValue(p,MOSAICwin,NULL);
  (time1->i)++;
  XtSetArg(args[0],XmNlabelString,compose);
  XtSetValues(myWin,args,1);
  time1->id=XtAppAddTimeOut(time1->app,500, mise,time1 );  
  XFlush(XtDisplay(MOSAIC));
}

void MPP_New(MPX_INSTANCE *p)
{
  int s;
  XtAppContext app; 
  int x,y,h,w;
  Widget MOSAIC; 
  Widget frame;
  ThePARAM *myparam, *tmp1;
  char mystring[500],mystring1[500],*tmp;
  Arg args[10];
  TimeClientData *time;
  int n;

  n=0;
  
  MPP_GetValue(p,X,&x);
  MPP_GetValue(p,Y,&y);
  MPP_GetValue(p,WIDTH,&w);
  MPP_GetValue(p,HEIGHT,&h);
  MOSAIC=(Widget)MPP_GetValue(p,MOSAICwin,NULL);

 
  strcpy(mystring1," ");
  
 
  myparam=(ThePARAM *) MPP_GetValue(p,PARAM,NULL); 
  tmp1=myparam;
  while (tmp1!=NULL)
    {
      if (tmp1->VALUE!=NULL)
	{
	
	 strcpy(mystring,tmp1->VALUE); 
         strcat(mystring1,mystring);
	 strcat(mystring1," ");
	 
	}
      tmp1=tmp1->next;
      
     }

 
   
   tmp=(char*)malloc(sizeof(char)*(strlen(mystring1)+1));
   strcpy(tmp,mystring1);
 
   
  XtSetArg(args[n],XmNx,x); n++;
  XtSetArg(args[n],XmNy,y); n++;
  XtSetArg(args[n],XmNwidth,w); n++;
  XtSetArg(args[n],XmNheight,h); n++;

  frame=XmCreateLabel(MOSAIC,mystring,args,n);
  MPP_SetValue(p, WINDOW,frame);

  app=XtWidgetToApplicationContext(MOSAIC);
  time=(TimeClientData*)malloc(sizeof(TimeClientData));
  time->myinst=p;
  time->app=app;
  time->i=1;
  time->id=XtAppAddTimeOut(app ,1000L, mise, (XtPointer)time); 
  time->string=tmp;
  p->Plugin_Data=time;
  time->index=0;
  XtSetMappedWhenManaged(frame,False);
  XtManageChild(frame);
  MPP_GetValue(p,SOCKET,&s);
   Traite_Donnee(s,time); 
}

void MPP_Initialize(MPX_PLUGIN myplug)
{
}


void MPP_Destroy(MPX_INSTANCE *p)
{
  Widget frame;
  TimeClientData *time;
  XClientMessageEvent ev;
  Atom delete_atom=0;
  Atom proto_atom=0;

  if (p==NULL) return;
  time=(TimeClientData*)(p->Plugin_Data);
  XtRemoveTimeOut(time->id); 
  

  frame= (Widget) MPP_GetValue(p,WINDOW,NULL);

 if (frame!=NULL)
	 {
	   if (!delete_atom)
	     delete_atom=XInternAtom(XtDisplay(frame),"WM_DELETE_WINDOW",False);
	   if (!proto_atom) proto_atom= XInternAtom(XtDisplay(frame),"WM_PROTOCOLS",False);
	   ev.type=ClientMessage;
	   ev.window=XtWindow(frame);
	   ev.message_type=proto_atom;
	   ev.format=32;
	   ev.data.l[0]=delete_atom;
	   ev.data.l[1]=CurrentTime;
	   XSendEvent(XtDisplay(frame),XtWindow(frame),True,0x00ffffff,(XEvent *)&ev);
	   XFlush(XtDisplay(frame));
	   XtSetMappedWhenManaged(frame,False);
	   XFlush(XtDisplay(frame));
	   XtDestroyWidget(frame);
	   MPP_SetValue(p,WINDOW,NULL);
	 }
 

}





void MPP_NeedSocket()
{
 
}

int Traite_Donnee(int sock,TimeClientData *time)
{ 
  printf("*************************************\n");
  printf("**  InTraite_Donnee par socket     **\n");
  printf("*************************************\n");

 
  if( sock < 0 )
    {
      printf("La socket descriptor est negative \n");
      return -1 ;
    }
  
  printf("j'ai bien recu le descripteur de socket qui me permettra de recuperer   les donnees a copier et le socket descriptor est %d\n", sock);
 
  {
	char * msg = "GET /ecole.html HTTP/1.0\n\n";

  	if (write(sock,msg,strlen(msg))<0) { 
  	    /* 	printf("ca ne marche pas le truc de socket avec HTTP\n");  */
  	} else { 
      	/* 	printf ("ca marche pour le write\n "); */
  	} 
  }

 return(0);
}

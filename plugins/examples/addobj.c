/* ********************************************************************* *
 *
 *  addobj.c : 
 *  [Eric Lecolinet - 1996/97].
 *
 * ********************************************************************* */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <Xm/XmAll.h>	     /* XmAll.h => inclut tous les headers Motif */

/* ********************************************************************* */
/* ******************************************* Fonctions Utilitaires *** */

/* Fonction pour creer un PushButton, le manager et lui ajouter 
 * une fonction de callback
 */
Widget CreatePushButton(Widget parent, String name, 
			XtCallbackProc callback, XtPointer data)
{
  Widget w = XmCreatePushButton(parent, name, NULL, 0);
  if (!w) return NULL;
  /* ajouter la fonction de callback si elle est definie */
  if (callback) XtAddCallback(w, XmNactivateCallback, callback, data);
  /* manager et retourner le widget cree */
  XtManageChild(w);
  return w;
}

/* ===================================================================== */
/* Fonction pour creer un ToggleButton, le manager et lui ajouter
 * une fonction de callback
 */
Widget CreateToggleButton(Widget parent, String name, 
			XtCallbackProc callback, XtPointer data)
{
  Widget w = XmCreateToggleButton(parent, name, NULL, 0);
  if (!w) return NULL;
  /* Attention: pas le meme type de callback que pour PushButton ! */
  if (callback) XtAddCallback(w, XmNvalueChangedCallback, callback, data);
  XtManageChild(w);
  return w;
}

/* ===================================================================== */
/* Fonction de callback pour sortir d'une application
 */
void ExitCB(Widget w, XtPointer client_data, XtPointer call_data)
{
  exit(0);
}

/* ===================================================================== */
/* Fonction de callback standard pour detruire un widget
 */
void DestroyCB(Widget w, XtPointer widget_to_destroy, XtPointer call_data)
{
  Widget w_to_destroy = (Widget)widget_to_destroy;
  /* si widget_to_destroy est NULL detruire le widget qui a appele DestroyCB*/
  if (! w_to_destroy) w_to_destroy = w;
  XtDestroyWidget(w_to_destroy);
}

/* ********************************************************************* */
/* ******************************************* Fonctions Specifiques *** */

/* recuperer la ressource la fonte (= police de caracteres) du Toggle
 * qui a appele cette fonction et l'affecter au widget "message"
 */
void ChangeFontCB(Widget toggle, XtPointer message, XtPointer call_data)
{
  XmFontList fontlist;
  XtVaGetValues(toggle, XmNfontList, &fontlist, NULL);
  XtVaSetValues((Widget)message, XmNfontList, fontlist, NULL);
}

/* ===================================================================== */
/* creer dynamiquement un PushButton en l'ajoutant au RowColumn "mainbox"
 * et rajouter un callback qui detruira ce bouton quand on cliquera dessus
 */
void AddButtonCB(Widget w, XtPointer mainbox, XtPointer call_data)
{
  CreatePushButton((Widget)mainbox, "remove", DestroyCB, NULL);
}

/* ********************************************************************* */
/* *********************************************************** Main **** */

int main(int argc, char *argv[])
{
  /* les "fallbacks" permettent de specifier des ressources a la maniere
   * des Fichiers de Ressources externes
   * (mais attention, il faut redoubler les \, par exemple \\n au lieu de \n)
   */
  String fallbacks[] = {
    "*background: grey",
    "*foreground: blue",
    "*message.background: red",
    "*message.foreground: white",
    "*message.labelString: Hello \\nBrave\\nNew World ...!",
    "*fontList: -*-helvetica-medium-r-normal-*-10-*-*-*-*-*-*-*",
    "*radiobox.small.fontList: -*-helvetica-medium-o-normal-*-9-*-*-*-*-*-*-*",
    "*radiobox.big.fontList: -*-helvetica-bold-r-normal-*-15-*-*-*-*-*-*-*",
    NULL,			/* toujours terminer par NULL !!! */
  };
  XtAppContext app;
  Widget toplevel, mainbox, message, radiobox;

  toplevel = XtVaAppInitialize(&app, "XMdemos", NULL, 0, &argc, argv,
			       fallbacks,
			       /* cette ressource permet de modifier */
			       /* la taille de la fenetre principale */
			       XmNallowShellResize, (XtArgVal) True, NULL);

  /* manager principal "mainbox" */
  mainbox = XmCreateRowColumn(toplevel, "mainbox", NULL, 0);
  XtManageChild(mainbox);

  /* zone de message en haut de "mainbox" */
  message = XmCreateLabel(mainbox, "message",  NULL, 0);
  XtManageChild(message);

  /* RadioBox (= Manager variante de RowColumn) CONTENU dans mainbox */
  radiobox = XmCreateRadioBox(mainbox, "radiobox", NULL, 0);
  XtManageChild(radiobox);

  /* ToggleButtons contenus dans "radiobox" */
  CreateToggleButton(radiobox, "small", ChangeFontCB, message);
  CreateToggleButton(radiobox, "big", ChangeFontCB, message);

  /* PushButtons contenus dans "mainbox" */
  CreatePushButton(mainbox, "add", AddButtonCB, mainbox);
  CreatePushButton(mainbox, "quit", ExitCB, NULL);

  /* Realisation et Lancement */
  XtRealizeWidget(toplevel);
  XtAppMainLoop(app);
}

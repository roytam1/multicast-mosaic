/* Please read copyright.ncsa. Don't remove next line */
#include "copyright.ncsa"

#include "../libhtmlw/HTML.h"
#include "mosaic.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>
#include <pwd.h>

#include "libnut/system.h"

#ifdef HAVE_AUDIO_ANNOTATIONS

/* Check and make sure that the recording command we named
 * in the resource is actually there and executable. */

mo_status mo_audio_capable (void)
{
	char *filename = get_pref_string(eRECORD_COMMAND_LOCATION);
	struct stat buf;
	int r;

	r = stat (filename, &buf); 
	if (r != -1 && buf.st_mode & S_IXOTH)
		return mo_succeed;
	else
		return mo_fail;
}

static XmxCallback (start_cb)
{
	mo_window *win = (mo_window*) client_data;

/* Take ourselves out of circulation. */
	XmxSetSensitive (win->audio_start_button, XmxNotSensitive);

/* Come up with a new tmpnam. */
	win->record_fnam = mo_tmpnam(win->current_node->url);

/* Fork off the recording process. */
	if ((win->record_pid = fork ()) < 0) {
/* Uh oh, no process. */
		XmxMakeErrorDialog (win->audio_annotate_win, 
			"Unable to start audio recording process." , 
			"Audio Annotation Error" );
		XtManageChild (Xmx_w);
	} else if (win->record_pid == 0) {
/* We're in the child. */
		execl (get_pref_string(eRECORD_COMMAND_LOCATION),
		get_pref_string(eRECORD_COMMAND),
		win->record_fnam, (char *)0);
	}
/* Let the user stop the record process. */
	XmxSetSensitive (win->audio_stop_button, XmxSensitive);
}

static XmxCallback (stop_cb)
{
	mo_window *win = (mo_window*) client_data;

/* Take ourselves out of circulation. */
	XmxSetSensitive (win->audio_stop_button, XmxNotSensitive);
/* Stop the record process.  This works for both SGI recordaiff
 * and Sun record, apparently. */
	kill (win->record_pid, SIGINT);
/* Wait until the process is dead. */
	waitpid (win->record_pid, NULL, 0);
/* No more process. */
	win->record_pid = 0;
/* Let the user make another recording. */
	XmxSetSensitive (win->audio_start_button, XmxSensitive);
}

static XmxCallback (buttons_cb1)
{
	mo_window *win = (mo_window*)client_data;

	if (!win->current_node) {
		XtUnmanageChild (win->audio_annotate_win);
		return;
	}
	if (win->record_pid) {
		kill (win->record_pid, SIGINT); /* Stop the record process. */
		waitpid(win->record_pid,NULL,0);/*Wait until process is dead. */
		win->record_pid = 0;
		win->record_fnam = 0;
	}
	XtUnmanageChild (win->audio_annotate_win);
}

static XmxCallback (buttons_cb2)
{
	mo_window *win = (mo_window*)client_data;

	if (!win->current_node) {
		XtUnmanageChild (win->audio_annotate_win);
		return;
	}
	mo_open_another_window (win, mo_assemble_help_url(
			"help-on-audio-annotate.html"),
		NULL, NULL);
}

static XmxCallback (buttons_cb0)
{
	mo_window *win = (mo_window*)client_data;

	if (!win->current_node) {
		XtUnmanageChild (win->audio_annotate_win);
		return;
	}

	if (win->record_pid) {	 /* Stop the record process. */
		kill (win->record_pid, SIGINT);
		waitpid(win->record_pid, NULL, 0);/*Wait until process is dead. */
	}
	if (win->record_fnam) {
		if (win->audio_pubpri == mo_annotation_workgroup) {
			char namestr[200], titlestr[200];
			unsigned char *data;
			FILE *fp;
			int len;
			extern char *machine;

			sprintf (namestr, "%s <%s>",
				get_pref_string(eDEFAULT_AUTHOR_NAME),
				get_pref_string(eDEFAULT_AUTHOR_EMAIL));
			sprintf (titlestr, "%s %s",
				"Audio Annotation by" ,
				get_pref_string(eDEFAULT_AUTHOR_NAME));
			len = 0;
			fp = fopen(win->record_fnam, "r");
			if (fp != NULL) {
/* Fine the length of the file the really cheesy way! */
				fseek(fp, 0L, 2);
				len = ftell(fp);
				fseek(fp, 0L, 0);
				data = (unsigned char *)malloc(len);
				if (data != NULL) {
					len = fread(data, sizeof(char), len, fp);
				} else {
					len = 0;
				}
				fclose(fp);
			}
			if (len <= 0) {
				XmxMakeErrorDialog(win->base, 
					"Unable to complete audio annotation." , 
					"Audio Annotation Error" );
				XtManageChild (Xmx_w);
			}
			mo_audio_grpan(win->current_node->url, titlestr, namestr,
					(char *)data, len);
			if (data)
				free((char *)data);
				mo_set_win_current_node (win, win->current_node);
		} else {	 /* Do the right thing. */
			int pan_id = mo_next_pan_id ();
			char *default_directory = 
				get_pref_string(ePRIVATE_ANNOTATION_DIRECTORY);
			char filename[500], namestr[200], textstr[500], titlestr[200];
			extern char *machine;
			char retBuf[BUFSIZ];

#ifdef __sgi
			sprintf (filename, "%s/%s/%s%d.aiff", getenv ("HOME"), 
				default_directory, "PAN-", pan_id);
#else /* sun or hp */
			sprintf (filename, "%s/%s/%s%d.au", getenv ("HOME"), 
				default_directory, "PAN-", pan_id);
#endif
/*SWP -- New "mv" fucntion to take care of these /bin/mv things*/
			if(my_move(win->record_fnam,filename,retBuf,BUFSIZ,1)!=SYS_SUCCESS) {
				application_user_info_wait(retBuf);
				return;
			}
			sprintf (titlestr, "%s %s", "Audio Annotation by" ,
				get_pref_string(eDEFAULT_AUTHOR_NAME));
			sprintf (namestr, "%s <%s>",
				get_pref_string(eDEFAULT_AUTHOR_NAME),
				get_pref_string(eDEFAULT_AUTHOR_EMAIL));
			sprintf (textstr, "This is an audio annotation. <P>\n\nTo hear the annotation, click <A HREF=\"file:%s\">here</A>. <P>\n" ,
				 filename);
			mo_new_pan (win->current_node->url, titlestr, namestr,
				textstr);
/* Inefficient, but safe. */
			mo_write_pan_list ();
			mo_set_win_current_node (win, win->current_node);
		}
	}
	win->record_pid = 0;
	win->record_fnam = 0;
	XtUnmanageChild (win->audio_annotate_win);
}

extern XmxOptionMenuStruct *pubpri_opts;

mo_status mo_post_audio_annotate_win (mo_window *win)
{
	Widget dialog_frame;
	Widget dialog_sep, buttons_form;
	Widget audio_annotate_form, yap_label;

	if (!win->current_node)
		return mo_fail;

	if (!win->audio_annotate_win) {
/* Create it for the first time. */
		XmxSetArg (XmNresizePolicy, XmRESIZE_GROW);
		win->audio_annotate_win = XmxMakeFormDialog (win->base,
			"Mosaic: Audio Annotate Window" );
		dialog_frame =XmxMakeFrame(win->audio_annotate_win, XmxShadowOut);
/* Constraints for base. */
		XmxSetConstraints (dialog_frame, XmATTACH_FORM, XmATTACH_FORM, 
			XmATTACH_FORM, XmATTACH_FORM, NULL, NULL, NULL, NULL);
/* Main form. */
		XmxSetArg (XmNfractionBase, 2);
		audio_annotate_form = XmxMakeForm (dialog_frame);
      
		yap_label = XmxMakeLabel(audio_annotate_form, 
			"Press Start to start recording; Stop to stop recording.\nRepeat until you're satisfied with the annotation.\nThen either Commit or Dismiss the annotation." );
 
		win->audio_start_button = XmxMakePushButton (audio_annotate_form,
			"Start" , start_cb, (XtPointer)win);
		win->audio_stop_button = XmxMakePushButton(
			audio_annotate_form,"Stop",stop_cb,(XtPointer)win);
		pubpri_opts[0].win = win;
		pubpri_opts[1].win = win;
		pubpri_opts[2].win = win;
		win->audio_pubpri_menu = XmxRMakeOptionMenu(audio_annotate_form,
			"", pubpri_opts);
		XmxRSetSensitive(win->audio_pubpri_menu, mo_annotation_public, 
			XmxNotSensitive);

		if (! get_pref_string(eANNOTATION_SERVER))
			XmxRSetSensitive(win->audio_pubpri_menu,
				(XtPointer) mo_annotation_workgroup, 
				XmxNotSensitive);

		win->audio_pubpri = mo_annotation_private;
		dialog_sep = XmxMakeHorizontalSeparator (audio_annotate_form);
      
		buttons_form = XmxMakeFormAndThreeButtons(audio_annotate_form, 
			"Commit" , "Dismiss" , "Help..." , 
			buttons_cb0, buttons_cb1, buttons_cb2,(XtPointer)win);
      
/* Constraints for audio_annotate_form. */
		XmxSetOffsets (yap_label, 10, 10, 10, 10);
		XmxSetConstraints(yap_label, XmATTACH_FORM, XmATTACH_NONE, 
			XmATTACH_FORM, XmATTACH_FORM,
			NULL, NULL, NULL, NULL);
		XmxSetOffsets (win->audio_start_button, 10, 10, 10, 10);
		XmxSetConstraints(win->audio_start_button, 
			XmATTACH_WIDGET, XmATTACH_NONE,
			XmATTACH_FORM, XmATTACH_NONE,
			yap_label, NULL, NULL, NULL);
		XmxSetOffsets (win->audio_stop_button, 10, 10, 10, 10);
		XmxSetConstraints(win->audio_stop_button, 
			XmATTACH_WIDGET, XmATTACH_NONE,
			XmATTACH_WIDGET, XmATTACH_NONE,
			yap_label, NULL, win->audio_start_button, NULL);
		XmxSetOffsets (win->audio_pubpri_menu->base, 10, 10, 10, 10);
		XmxSetConstraints(win->audio_pubpri_menu->base, 
			XmATTACH_WIDGET, XmATTACH_NONE,
			XmATTACH_WIDGET, XmATTACH_FORM,
			yap_label, NULL, win->audio_stop_button, NULL);
		XmxSetArg (XmNtopOffset, 10);
		XmxSetConstraints(dialog_sep, 
			XmATTACH_WIDGET, XmATTACH_NONE, 
			XmATTACH_FORM, XmATTACH_FORM,
			win->audio_pubpri_menu->base, NULL, NULL, NULL);
		XmxSetConstraints(buttons_form, 
			XmATTACH_WIDGET, XmATTACH_NONE, 
			XmATTACH_FORM, XmATTACH_FORM,
			dialog_sep, NULL, NULL, NULL);
	}
	XmxManageRemanage (win->audio_annotate_win);
	XmxSetSensitive (win->audio_stop_button, XmxNotSensitive);
	XmxSetSensitive (win->audio_start_button, XmxSensitive);
	return mo_succeed;
}
#endif /* HAVE_AUDIO_ANNOTATIONS */

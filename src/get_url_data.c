/* get_url_data.c
 * Author: Gilles Dauphin
 * Version 2.7b4m9 [Sept96]
 *
 * Copyright (C) 1996 - G.Dauphin, P.Dax
 * See the file "license.mMosaic" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES. 
 *
 * Bug report :  dauphin@sig.enst.fr dax@inf.enst.fr
 */

#include "../libmc/mc_defs.h"
#include "libhtmlw/HTML.h"
#include "libhtmlw/HTMLP.h"
#include "libhtmlw/HTMLPutil.h"
#include "mosaic.h"
#include "../libmc/mc_dispatch.h"
#include "../libmc/mc_misc.h"

/* Defined in gui.c */
extern char *cached_url;
extern mo_window *current_win;

/* Defined in gui-documents.c */
extern int interrupted;
extern int loading_inlined_images;
extern int installed_colormap;
extern Colormap installed_cmap;

void GetUrlData(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window *) clid;
	EODataStruct * eods = (EODataStruct *) calld;
	char * src = NULL;
	char *fnam;
	int rc;
	McMoWType wtype;
        int internal_numeo;

	eods->ret_filename = NULL;
	if (w == NULL){
		printf("[GetUrlData] NoWidget !!!!\n");
		return;
	}
	if(eods->src)
		src = strdup(eods->src);
	if (!src)
		return ;

	wtype = win->mc_type;
	internal_numeo = eods->num_eo;

	mo_set_current_cached_win (win);
	cached_url = win->cached_url ? win->cached_url : "lose";
	win->cached_url = cached_url;

#ifdef MULTICAST
        /* if it is a receiver do something . Reassemble data if possible*/
        /* the data is update in McActionAllData when it's ready to do it */
        if (wtype == MC_MO_TYPE_RCV_ALL){
                McUser * u = win->mc_user;      /*#####*/
                int num_eo = internal_numeo + 2;
		char * eo_file_name=NULL;
        
                if (u->neo < (num_eo+1))        /* GASP###### */
                        return ;
                if (u->eos[num_eo] == NULL)
                        return;
        
                /*####################*/
                eo_file_name= McGetEOFileData(w, u->eos[num_eo],u->seo[num_eo],u,
				num_eo);
		eods->ret_filename = eo_file_name;
                return ;
        }
#endif
	src = mo_url_canonicalize (src, cached_url);

	if ( interrupted) {	 /* Return if interrupted is high. */
		free (src);
		eods->ret_filename = NULL;
		return ;
	}

	/* We have to load the data. */
	fnam = mo_tmpnam(src);
	interrupted = 0;
	rc = mo_pull_er_over_virgin (src, fnam);
	if (!rc) {
		free (fnam);
		return;
	}
	eods->ret_filename = fnam;
#ifdef MULTICAST
	/* if this is a sender widget, save the original data for futur send*/
	if ((wtype == MC_MO_TYPE_MAIN) && mc_send_enable){
		McGlobalEo * geo;
		if (!eods->cw_only){
			geo = McGetEmptyGlobalObject(); /* a structure to fill */
			geo->orig_data = McReadEo(fnam,&(geo->len_eo));
			/* and that's all. Work is done in McFillData */
		}
	}
	/* continue if wtype == MC_TYPE_NONE */
#endif
/*############
        unlink(fnam);
        free (fnam);
*/
}

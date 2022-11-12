#include "libhtmlw/HTML.h"
#include "libhtmlw/HTMLP.h"
#include "libhtmlw/HTMLPutil.h"
#include "mosaic.h"
#include "../libmc/mc_dispatch.h"
#include "../libmc/mc_misc.h"


/* Defined in gui-documents.c */

void GetUrlData(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window *) clid;
	EODataStruct * eods = (EODataStruct *) calld;
	char * src = NULL;
	char *fnam;
	McMoWType wtype;
        int internal_numeo;

	eods->ret_filename = NULL;
	if(eods->src)
		src = strdup(eods->src);
	if (!src)
		return ;

#ifdef MULTICAST
	wtype = win->mc_type;
#endif

#ifdef MULTICAST
        /* if it is a receiver do something . Reassemble data if possible*/
        /* the data is update in McActionAllData when it's ready to do it */
        if (wtype == MC_MO_TYPE_RCV_ALL){
                /*####################*/
                eo_file_name= McGetEOFileData(w, u->eos[num_eo],u->seo[num_eo],u,
				num_eo);
		eods->ret_filename = eo_file_name;
                return ;
        }
#endif
	src = mo_url_canonicalize (src, win->current_node->base_url);


	/* We have to load the data. */
	fnam = tempnam (mMosaicTmpDir,"mMo");
	fprintf(stderr, "GetUrlData: implement... Abort... Please report\n");
	abort();
#ifdef MULTICAST
	/* if this is a sender widget, save the original data for futur send*/
	if ((wtype == MC_MO_TYPE_MAIN) && mc_send_enable){
		if (!eods->cw_only){
			geo = McGetEmptyGlobalObject(); /* a structure to fill */
			geo->orig_data = McReadEo(fnam,&(geo->len_eo));
			/* and that's all. Work is done in McFillData */
		}
	}
	/* continue if wtype == MC_TYPE_NONE */
#endif
}

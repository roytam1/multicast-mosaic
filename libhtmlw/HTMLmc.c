#ifdef MULTICAST

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "HTML.h"
#include "../src/mosaic.h"
#include "HTMLP.h"
#include "HTMLPutil.h"

/*
 * Convenience function to return the ele_rec of all images in the
 * document.
 * Function returns an array of ele_rec and fills num_ele_rec passed.
 * If there are no ele_rec NULL returned.
 */
/*struct ele_rec ** HTMLGetEoEleRec(Widget w, int *num_ele_rec)
{
        HTMLWidget hw = (HTMLWidget)w;
        struct ele_rec *eptr;
        int cnt;
        struct ele_rec **earray;

        cnt = 0;
        eptr = hw->html.formatted_elements;
        while (eptr != NULL) {
                if (eptr->type == E_IMAGE)
                        cnt++;
                eptr = eptr->next;
        }
        if (cnt == 0) {
                *num_ele_rec = 0;
                return(NULL);
        }
        *num_ele_rec = cnt;
        earray = (struct ele_rec  **)malloc(sizeof(struct ele_rec * ) * cnt);
        eptr = hw->html.formatted_elements;
        cnt = 0;
        while (eptr != NULL) {
                if (eptr->type == E_IMAGE) {
                        earray[cnt] = eptr;
                        cnt++;
                }
                eptr = eptr->next;
        }
        return(earray);
}
*/

/* image converter fonction (from ImageResolve in img.c) */

/*void McUpdateWidgetObject(Widget w,int num_eo,char * data,int len_data)
/*{
/*	HTMLWidget hw = (HTMLWidget)w;
/*	struct ele_rec **earray;
/*	int neo_source;	/* the number of eo in html text source code */
/*	struct ele_rec *eptr;
/*
/*	earray = HTMLGetEoEleRec(w, &neo_source);
/*	num_eo = num_eo - 3;	/* 0 1 2 is busy */
/*
/*	if (neo_source == 0)
/*		return;
/*	if (neo_source  < (num_eo +1) ){
/*		printf("probleme getting embedded object \n");
/*		printf(" neo_source/num_eo = %d/%d\n",neo_source,num_eo);
/*		return;
/*	}
/*	
/*	eptr = earray[num_eo];
/*
/*        /*
/*       * we have to do a full reformat.
/*      */
/*#ifdef TO_DO
/*	/*####### C'est a optimiser ########## */
/*#endif
/*	ReformatWindow(hw,True);
/*	ScrollWidgets(hw);
/*	ViewClearAndRefresh(hw);
/*}

/* return the file name or NULL */

/*
char * McGetEOFileData(Widget w, char * buf, int len_buf, McUser *u,int num_eo)
{
	char * fnam = NULL;
	int tmpfd;

	if (u->filename[num_eo])
		return u->filename[num_eo];

	fnam = tempnam (mMosaicTmpDir,"mMo");
	tmpfd = open(fnam,O_WRONLY|O_CREAT,0644);
	write(tmpfd,buf,len_buf);
	close(tmpfd);
	u->filename[num_eo] = fnam;

	return fnam;
}
*/
#endif

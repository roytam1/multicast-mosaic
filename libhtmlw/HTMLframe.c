
#include "../libmc/mc_defs.h"
#include "HTML.h"
#include "HTMLP.h"
#include "HTMLPutil.h"

#define FRAME_CHECK_SIZE 2048


#if 0
        if(get_pref_boolean(eFRAME_HACK))
                frame_hack();

/* Basically we show the urls that appear within the frameset tag
   as urls and add some text explaining that these are the urls they
   were supposed to see as frames. We also show the NOFRAMES stuff. */

static void frame_hack()
{
	char *start, *new_src, *place,*tmp, *url, *frame_anchors[25];
	int num_frames=0, new_size=0, i;
	char *ptr;

	start=NULL;
	for(i=0,ptr=HTMainText->htmlSrc; ptr && i<FRAME_CHECK_SIZE; ptr++,i++) {
		if (*ptr=='<' && (*(ptr+1)=='f' || *(ptr+1)=='F')) {
			if (!strncasecmp("rameset",ptr+2,7)) {
				start=ptr;
				break;
			}
		}
	}
  
	if(!start)
		return;
	place = start;

	while((tmp=strstr(place, "<frame ")) || (tmp=strstr(place,"<FRAME "))) {
		url = ParseMarkTag(tmp, "FRAME", "SRC");
		if (url) {
			frame_anchors[num_frames] = (char*)malloc(strlen(url)*2 + 
				strlen("<LI> <A HREF=  > </A> ")+4);
			sprintf(frame_anchors[num_frames], 
				"<LI> <A HREF=\"%s\"> %s </A>", url, url);
			new_size += strlen(frame_anchors[num_frames])+1;
			num_frames++;
		}
		place = tmp+6;
	}
	new_src = (char*)malloc(new_size+strlen(HTMainText->htmlSrc) + 
				strlen(" <HR> ") +
				strlen("<H2> Frame Index: </H2> <UL> </UL>")+6);

	/* copy everything up to first frameset tag to new_src */
	strncpy(new_src, HTMainText->htmlSrc, strlen(HTMainText->htmlSrc) -
			strlen(start));
	new_src[strlen(HTMainText->htmlSrc) - strlen(start)]='\0';
	sprintf(new_src, "%s <H2> Frame Index: </H2> <UL>", new_src);
	for(i=0;i<num_frames;i++) {
		sprintf(new_src, "%s%s", new_src, frame_anchors[i]);
		free(frame_anchors[i]);
	}
	sprintf(new_src, "%s </UL> <HR>", new_src); /* end list */
	strcat(new_src, start);			 /* add in rest of document */
	free(HTMainText->htmlSrc);
	HTMainText->htmlSrc = new_src;
}

#endif

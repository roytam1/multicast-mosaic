
#ifndef LIBHTMLW_HTMLP_UTIL_H
#define LIBHTMLW_HTMLP_UTIL_H


extern void 		FreeMarkUpList(struct mark_up *List);
extern void 		FreeLineList(struct ele_rec *, HTMLWidget );
extern int 		ElementLessThan(struct ele_rec *s, struct ele_rec *e,
				int start_pos, int end_pos);
extern int 		SwapElements( struct ele_rec *start, struct ele_rec *end,
				int start_pos, int end_pos);
extern char *		MaxTextWidth(char *txt, int *cnt);


 
extern void 		PartOfTextPlace(HTMLWidget hw, struct mark_up *mptr,
				PhotoComposeContext * pcc);
extern void 		PartOfPreTextPlace(HTMLWidget hw, struct mark_up *mptr,
				PhotoComposeContext * pcc);
extern void 		LinefeedPlace(HTMLWidget hw, struct mark_up *mptr, 
				PhotoComposeContext * pcc);
extern void 		HRulePlace(HTMLWidget hw, struct mark_up *mptr, 
				PhotoComposeContext * pcc);
extern void 		BulletPlace(HTMLWidget hw, struct mark_up *mptr, 
				PhotoComposeContext * pcc);
extern void 		Set_E_TEXT_Element(HTMLWidget hw, struct ele_rec *eptr,
				char *text, PhotoComposeContext *pcc);



extern Dimension	HbarHeight(HTMLWidget hw);



extern struct ele_rec * CreateElement( HTMLWidget hw, ElementType type, XFontStruct *fp,
        			int x,int y, int w, int h, int baseline,
				PhotoComposeContext * pcc);
extern void 		AdjustBaseLine(HTMLWidget hw,struct ele_rec *eptr,
				PhotoComposeContext * pcc);
extern void 		FormatChunk( HTMLWidget hw, struct mark_up * start_mark,
        			struct mark_up * end_mark,
        			PhotoComposeContext * pcc, Boolean save_obj);
extern void 		PlaceLine( HTMLWidget hw, int line);
extern struct ele_rec * LocateElement( HTMLWidget hw, int x, int y, int *pos);
extern char * 		ParseTextToString(
				struct ele_rec *startp, struct ele_rec *endp,
        			int start_pos, int end_pos,
        			int space_width, int lmargin);
extern char * 		ParseTextToPrettyString( HTMLWidget hw,
        			struct ele_rec *startp,
				struct ele_rec *endp,
        			int start_pos, int end_pos,
        			int space_width, int lmargin);
extern int 		DocumentWidth(HTMLWidget hw, struct mark_up *list);
extern void 		TextRefresh( HTMLWidget hw, struct ele_rec *eptr,
        			int start_pos, int end_pos);
extern void 		ImageRefresh(HTMLWidget hw, struct ele_rec *eptr);
extern void 		LinefeedRefresh( HTMLWidget hw, struct ele_rec *eptr);
extern void 		BulletRefresh( HTMLWidget hw, struct ele_rec *eptr);
extern void 		HRuleRefresh( HTMLWidget hw, struct ele_rec *eptr);


extern void 		FreeColors(Display *dsp, Colormap colormap);
extern void 		FindColor(Display *dsp, Colormap colormap, XColor *colr);
extern XImage * 	MakeImage( Display *dsp, unsigned char *data,
        			int width, int height,
        			int depth, ImageInfo *img_info, int clip);
extern int 		AnchoredHeight(HTMLWidget hw);
extern char * 		IsMapForm(HTMLWidget hw);
extern int 		IsIsMapForm(HTMLWidget hw, char *href);
extern ImageInfo * 	NoImageData( HTMLWidget hw);
extern Pixmap 		InfoToImage( HTMLWidget hw, ImageInfo *img_info,int clip);
extern void 		ImagePlace(HTMLWidget hw, struct mark_up *mptr, 
				PhotoComposeContext * pcc);


extern void 		ImageSubmitForm(FormInfo *fptr, XEvent *event,
				int x, int y);
extern void 		HideWidgets( HTMLWidget hw);
extern char * 		ComposeCommaList( char **list, int cnt);
extern void		FreeCommaList( char **list, int cnt);
extern void		WidgetPlace(HTMLWidget hw, struct mark_up *mptr,
				PhotoComposeContext *pcc);
extern WidgetInfo *	MakeWidget(HTMLWidget hw, char *text,
				PhotoComposeContext *pcc, 
				int id);
extern void 		AddNewForm( HTMLWidget hw, FormInfo *fptr);
extern void		WidgetRefresh( HTMLWidget hw, struct ele_rec *eptr);



extern String 		ParseTextToPSString(HTMLWidget hw, struct ele_rec *el,
                           	struct ele_rec *startp, struct ele_rec *endp,
                           	int start_pos, int end_pos, int space_width,
                           	int lmargin, int fontfamily,
                           	char *url, char *time_str);



extern void		hw_do_bg(HTMLWidget, PhotoComposeContext *,
				struct mark_up *mptr);
extern void		hw_do_color(HTMLWidget,char*,char*,PhotoComposeContext * pcc);

extern void 		TablePlace(HTMLWidget hw, struct mark_up **mptr, 
				PhotoComposeContext * pcc, Boolean save_obj);
extern void 		TableRefresh( HTMLWidget hw, struct ele_rec *eptr);


extern void 		AprogPlace(HTMLWidget hw, struct mark_up **mptr, 
				PhotoComposeContext * pcc, Boolean save);
extern void 		AprogRefresh(HTMLWidget hw, struct ele_rec *eptr);
extern void 		AppletPlace(HTMLWidget hw, struct mark_up **mptr, 
				PhotoComposeContext * pcc, Boolean save);
extern void 		AppletRefresh(HTMLWidget hw, struct ele_rec *eptr);

extern void 		ViewClearAndRefresh( HTMLWidget hw);
extern void		ScrollWidgets(HTMLWidget hw);
#endif

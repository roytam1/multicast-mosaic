
#ifndef LIBHTMLW_HTMLP_UTIL_H
#define LIBHTMLW_HTMLP_UTIL_H


extern void 		FreeLineList(struct ele_rec *, HTMLWidget );
extern int 		ElementLessThan(struct ele_rec *s, struct ele_rec *e,
				int start_pos, int end_pos);
extern int 		SwapElements( struct ele_rec *start, struct ele_rec *end,
				int start_pos, int end_pos);


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
extern char * 		ParseTextToPrettyString(struct ele_rec *startp,
				struct ele_rec *endp,
        			int start_pos, int end_pos,
        			int space_width, int lmargin);
extern void 		TextRefresh( HTMLWidget hw, struct ele_rec *eptr,
        			int start_pos, int end_pos);
extern void 		ImageRefresh(HTMLWidget hw, struct ele_rec *eptr);
extern void 		LinefeedRefresh( HTMLWidget hw, struct ele_rec *eptr);
extern void 		BulletRefresh( HTMLWidget hw, struct ele_rec *eptr);
extern void 		HRuleRefresh( HTMLWidget hw, struct ele_rec *eptr);


extern char * 		IsMapForm(HTMLWidget hw);
extern int 		IsIsMapForm(HTMLWidget hw, char *href);
extern ImageInfo * 	NoImageData( HTMLWidget hw);
extern Pixmap 		InfoToImage( HTMLWidget hw, ImageInfo *img_info,int clip);
extern void 		ImagePlace(HTMLWidget hw, struct mark_up *mptr, 
				PhotoComposeContext * pcc);


extern void 		ImageSubmitForm(FormInfo *fptr, XEvent *event,
				int x, int y);
extern void 		InputImageSubmitForm(FormInfo *fptr, XEvent *event,
				HTMLWidget hw);
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


extern String 		ParseTextToPSString(HTMLWidget hw,
                           	struct ele_rec *startp, struct ele_rec *endp,
                           	int start_pos, int end_pos, int lmargin,
                           	int fontfamily,
                           	char *url, char *time_str);

extern void		hw_do_body_bgima(HTMLWidget, struct mark_up *mptr);
extern void		hw_do_body_color(HTMLWidget,char*,char*,PhotoComposeContext * pcc);

extern void 		TablePlace(HTMLWidget hw, struct mark_up **mptr, 
				PhotoComposeContext * pcc, Boolean save_obj);
extern void 		TableRefresh( HTMLWidget hw, struct ele_rec *eptr,
				int win_x, int win_y, Dimension win_w,
				Dimension win_h);

extern void 		ViewClearAndRefresh( HTMLWidget hw);
extern void		ScrollWidgets(HTMLWidget hw);
#endif

#ifdef MULTICAST

typedef struct _GuiEntry {
	Widget form;
	Widget label;
	Widget toggle;
	int nu;
	struct _Source * source;
} GuiEntry;

typedef struct _SDesStruct {
	char * cname;
	char * tool;
	char * email;
} SDesStruct;

typedef struct _Source {
	int 		mute;	/* displayed ? */
	mo_window 	*win;	/* winodw for sender */

	u_int32_t	srcid;	/* id of the source */
	IPAddr		addr;	/* IPV4 adresse */
	char *		s_srcid; /* unique string */
	int		is_sender; /* is emetting */

	struct _Source 	*hlink;	/* hashcode scrid link */
	struct _Source 	*next;	/* list of source */
	int first_seq;		/* very first seqn */
	int cur_seq;		/* seqno */

	SDesStruct * sdes;	/* session description */

	GuiEntry * gui_ent;	/* graphique interface for this source */
				/* only for the user list */

	int huid;		/* highter url_id we hear for this source */
	int cwuid;		/* current wanted url_id doc */
	int cduid;		/* current display url_id doc */

	DocEntry ** doc;	/* store the completed data here */
	ChunkedDocEntry ** chkdoc; /* assemble packet here */
} Source;

extern unsigned int mc_local_srcid;	/* SSRC for me */

extern void McRtcpWriteSdesCb(XtPointer clid, XtIntervalId * time_id);
extern void UcRtcpWriteSdesCb(XtPointer clid, XtIntervalId * time_id);

extern void McNewSrcid(IPAddr addr);
extern Source* demux(u_int32_t srcid, IPAddr addr, u_int16_t seq);
extern void McUpdateDataSource(Source *s, int is_end, u_int16_t seqn,
        u_int32_t rtp_ts, u_int32_t ssrc,
        u_int16_t url_id, u_int16_t o_id, u_int32_t offset, char *d,
        u_int32_t d_len);
extern void ProcessRtcpData(unsigned char *buf, int len, IPAddr addr_from);

#endif

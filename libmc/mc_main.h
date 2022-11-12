#ifdef MULTICAST

extern int mc_local_url_id;
extern int mc_local_object_id;
extern int mc_state_report_url_id;
extern int mc_state_report_o_id;
extern int mc_state_report_len;

extern char		*local_ip_addr_string;
extern unsigned short	uc_rtp_addr_port;
extern unsigned short	uc_rtcp_addr_port;

extern void McStartSender(mo_window * main_win);
extern void McInit(mo_window * win);

#endif

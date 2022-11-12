#ifdef MULTICAST

extern char		*local_ip_addr_string;
extern unsigned short	uc_rtp_addr_port;
extern unsigned short	uc_rtcp_addr_port;

extern void McStartSender(mo_window * main_win);
extern void McInit(mo_window * win);
extern void McEmitCursor(mo_window * mc_send_win, XEvent * ev);

#endif

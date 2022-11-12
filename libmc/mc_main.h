#ifdef MULTICAST

                        /* sockets descriptor */
extern int             mc_fd_rtp_w;
extern int             mc_fd_rtp_r;
extern int             mc_fd_rtcp_w;
extern int             mc_fd_rtcp_r;
extern int             uc_fd_rtp_w;          
extern int             uc_fd_rtp_r;          
extern int             uc_fd_rtcp_w;         
extern int             uc_fd_rtcp_r;

extern int mc_local_url_id;

extern XtIntervalId	mc_rtcp_w_sdes_timer_id;
extern unsigned long	mc_rtcp_w_sdes_time;

extern char		*local_ip_addr_string;
extern unsigned short	uc_rtp_addr_port;
extern unsigned short	uc_rtcp_addr_port;

extern void McStartSender(mo_window * main_win);
extern void McInit(mo_window * win);

#endif

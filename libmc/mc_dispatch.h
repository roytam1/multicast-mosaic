/* mc_dispatch.c
 * Author: Gilles Dauphin
 * Version 2.7b4m3 [May96]
 *
 * Copyright (C) 1996 - G.Dauphin, P.Dax
 * THIS PROGRAM IS DISTRIBUTED WITHOUT ANY WARRANTY
 * COMMERCIAL USE IS FORBIDDEN WITHOUT EXPLICIT AUTHORIZATION
 *
 * Bug report :  dauphin@sig.enst.fr dax@inf.enst.fr
 */

#ifndef MC_DISPATCH_H
#define MC_DISPATCH_H

#ifdef MULTICAST

extern int		mc_multicast_enable;
extern int		mc_send_enable;
extern mo_window * 	mc_send_win;
extern XtIntervalId    	mc_send_rtcp_sdes_cname_time_out_id;
extern XtIntervalId    	mc_check_rcvstime_time_out_id;
extern XtIntervalId    	mc_check_senderstime_time_out_id;
extern McSendDataStruct 	mc_data_send_data_struct;
extern unsigned int 	mc_global_eo_count;

extern void McInit(mo_window * win);
extern void McFillData(McSendDataStruct * d, mo_window *win);
extern void McReadSocketCb(XtPointer clid, int * fd, XtInputId * in_id);
extern void McReadRtcpSocketCb(XtPointer clid, int * fd, XtInputId * in_id);
extern void McStartSendHyperText(mo_window * main_win);
extern void McStopSendHyperText(mo_window * win);
extern void McSendFastAllEoData(int r_num_eo);
extern void McSendFastPacketEoData(int r_num_eo,int r_fpno);
extern void McSendAllDataOnlyOnce(McSendDataStruct * d);
extern McGlobalEo * McGetEmptyGlobalObject();
extern void McSetCursorPos(Widget w, int x, int y);
extern void McSetHtmlTexte( char *txt);

#endif /* MULTICAST */

#endif /* MC_DISPATCH_H */

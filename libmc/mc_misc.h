/*
 * mc_sockio.h
 * Author: G.Dauphin
 * Version 1.0 [May96]
 *
 * Copyright (C) 1996 - G.Dauphin, P.Dax (ENST)
 *
 * THIS PROGRAM IS DISTRIBUTED WITHOUT ANY WARRANTY
 * COMMERCIAL USE IS FORBIDDEN WITHOUT EXPLICIT AUTHORIZATION
 *
 * Bug report :
 * 
 * dax@inf.enst.fr
 * dauphin@sig.enst.fr
 */


extern time_t McDate();
extern void SendGotoDataTimeOutCb(XtPointer clid, XtIntervalId * id);
extern void McSendUserInfoTimeOutCb(XtPointer clid, XtIntervalId * id);
extern char * McReadEo( char* fnam, unsigned int * len_ret);

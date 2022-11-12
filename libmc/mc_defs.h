/*
 * mc_defs.h
 * Author: Gilles Dauphin
 * Version 1.0 [May96]
 *
 * Copyright (C) - 1996 G.Dauphin, P.Dax (ENST)
 *
 * THIS PROGRAM IS DISTRIBUTED WITHOUT ANY WARRANTY
 * COMMERCIAL USE IS FORBIDDEN WITHOUT EXPLICIT AUTHORIZATION
 * 
 * Bug report :
 * 
 * dax@inf.enst.fr
 * dauphin@sig.enst.fr
 */

#ifndef MC_DEFS_H
#define MC_DEFS_H


#define MC_GOTODATA_TIME_OUT (5000U)	/*(5 * 1000) milli-sec. For sending*/
#define MC_PURGE_TIME_OUT (60000U)	/*(60 * 1000) milli-sec. */
#define MC_SEND_ALL_DATA_TIME_OUT (4000U)	/* in milli-sec. For sending*/
#define MC_SEND_RTCP_SDES_CNAME_TIME_OUT (5000U) /* in milli-sec. For sending*/
#define MC_SEND_HEAR_BEAT_TIME_OUT (1000U)	/* in milli-sec. For sending*/
#define MC_SEND_GOTO_ID_TIME_OUT   (4000U)	/* scroll bar update */
#define MC_CHECK_RCVSTIME_TIME_OUT    (300000U) /* is member still alive? */
#define MC_CHECK_SENDERSTIME_TIME_OUT (120000U)	/* is member alive as sender?*/

#define MC_USERS_TIME_OUT (300U)	/* en secondes */

#define MCR_GOTODATA		0x00	/* goto URL  */
					/* Send it often to stay alive */
#define MCR_ALLDATA		128	/* the real multicast mode */
#define MCR_HEARBEAT		129	/* hearbeat: I am alive */
#define MCR_HTML_GOTO_ID 	130	/* Goto to id (parser id) Scrollbar pos*/
#define MCR_CURSOR_POS		131	/* Cursor position x.y */

#define MC_MAX_BUF_SIZE		32767	/* max io buf size for socket */
#define MC_MAX_ALIAS_SIZE 	126	/* max len of an alias name */
#define MC_MAX_URL_SIZE		500	/* max len of an url name */
#define MC_PACKET_SIZE		500	/* max len of data multicast packet */

typedef struct _McRtcpLrmpNackAllDataStruct {
	unsigned short rh_flags; /* (0x8500 + RTCP_PT_LRMP) */
	unsigned int ipaddr;	/* user's ip addr */
	unsigned short pid;	/* user's pid */
	unsigned short unused;  /* unused */
	unsigned int s_ipaddr;  /* the sender source of url_id */
	unsigned short s_pid;
	unsigned int s_ssrc;	/* sender SSRC */
	unsigned int url_id;	/* url_id to recover */
	unsigned short num_eo;  /* numero du eo a recover */
} McRtcpLrmpNackAllDataStruct;

typedef struct _McRtcpLrmpNackDataStruct {
	unsigned short rh_flags; /* (0x8300 + RTCP_PT_LRMP) */
	unsigned int ipaddr;	/* user's ip addr */
	unsigned short pid;	/* user's pid */
	unsigned short unused;  /* unused */
	unsigned int s_ipaddr;  /* the sender source of url_id */
	unsigned short s_pid;
	unsigned int s_ssrc;	/* sender SSRC */
	unsigned int url_id;	/* url_id to recover */
	unsigned short num_eo;  /* numero du eo a recover */
	unsigned short fpno;	/* first packet number to recover in num_eo */
	unsigned short blp;	/* bitmask of following lost packets. */
} McRtcpLrmpNackDataStruct;

typedef struct _McRtcpByeDataStruct {
	unsigned short rh_flags; /* (0x8000 + RTCP_PT_BYE) */
	unsigned int ipaddr;	/* user's ip addr */
	unsigned int ssrc;	/* SSRC */
	unsigned short pid;	/* user's pid */
} McRtcpByeDataStruct;

typedef struct _McRtcpSdesCnameDataStruct {
	unsigned short rh_flags; /* (0x8000 + RTCP_PT_SDES) */
	unsigned int ipaddr;	/* user's ip addr */
	unsigned int ssrc;	/* SSRC */
	unsigned short pid;	/* user's pid */
	unsigned char code;	/* RTCP_SDES_CNAME */
	unsigned char len_alias;	/* length of alias name */
	char alias[MC_MAX_ALIAS_SIZE+1]; /* alias de l'utilisateur  */
} McRtcpSdesCnameDataStruct;

typedef struct _mcs_gotodata {
	unsigned char code;	/* MCR_GOTODATA */
	unsigned int gmt_init_time; /* Player run games at this time */
	unsigned int gmt_send_time; /* Player send this message at this time */
	unsigned int send_cnt;	/* counter */
	unsigned int ip_addr;	/* user's ip addr */
	unsigned short pid;	/* user's pid */
	unsigned int url_id;	/* the id of the url sent */
	unsigned char len_alias;	/* length of alias name */
	char alias[MC_MAX_ALIAS_SIZE+1]; /* alias de l'utilisateur  */
	unsigned char len_url;	/* len of URL */
        char url[MC_MAX_URL_SIZE+1]; /* url that we rec. or send */
} Mcs_gotodata;

/* c'est le header du packet 'alldata' suivent ensuite les donnees */
typedef struct _mcs_alldata {
	unsigned short rh_flags;	/* 0x8030 */
	unsigned char code;	/* MCR_ALLDATA */
	unsigned int ipaddr;	/* user's ip addr */
	unsigned int ssrc;	/* 8bit SSRC */
	unsigned short pid;	/* user's pid */
	unsigned int url_id;    /* the id of the url sent */
	unsigned int gmt_send_time; /* date the user send */
	unsigned int nombre_eo;	/* nombre d'eo */
	unsigned int num_eo;	/* numero du eo */
	unsigned int seo;	/* size of eo */
	unsigned int nombre_packet;	/* nombre de packet total du eo */
	unsigned int num_packet;	/* numero du  packet */
	char data[MC_PACKET_SIZE +1]; /* les datas */
	unsigned int len_data;	/* taille des donnees */
} Mcs_alldata;

typedef struct _McRtpGotoIdDataStruct {
	unsigned short rh_flags;	/* 0x8030 */
	unsigned char code;	/* MCR_HTML_GOTO_ID */
	unsigned int ipaddr;	/* user's ip addr */
	unsigned int ssrc;	/* 8bit SSRC */
	unsigned short pid;	/* user's pid */
	unsigned int url_id;    /* the id of the url sent */
	unsigned int gmt_send_time; /* date the user send */
	unsigned int html_goto_id; /* HTML id (get by parser ) Set the Scollbar*/
} McRtpGotoIdDataStruct;

typedef struct _McRtpCursorPosDataStruct {
	unsigned short rh_flags;	/* 0x8030 */
	unsigned char code;	/* MCR_CURSOR_POS */
	unsigned int ipaddr;	/* user's ip addr */
	unsigned int ssrc;	/* 8bit SSRC */
	unsigned short pid;	/* user's pid */
	unsigned int url_id;    /* the id of the url sent */
	unsigned int gmt_send_time; /* date the user send */
	short posx; 		/* x pos of virtual cursor*/
	short posy;		/* y pos of virtual cursor*/
} McRtpCursorPosDataStruct;


#include "../libhtmlw/HTML.h"
#include "../src/mosaic.h"

typedef struct _mc_global_eo{
	unsigned int num_eo;	/* numero du eo */
	unsigned int len_eo;	/* len of data */
	char * orig_data;	/* original data not converted */
} McGlobalEo;


typedef struct _McSendDataStruct {
	unsigned int 	id;	/* the id is unique */
	unsigned int	is_send; /* is send Once ? */
	char *		alias;	/* alias_name */
	mo_window *	win;
	char * 		url;	/* the url of actual hypertext */
	char * 		text;	/* the hyper text to send */
	unsigned int 	neo;	/* number of embedded objects */
				/* essentially IMG */
	unsigned int *	seo;	/* taille de chaque eo . size of eo */
	char ** 	eos;	/* eo[neo]. Each element is data of eo*/
				/* but :				*/
				/*    eo[0] <=> alias			*/
				/*    eo[1] <=> URL			*/
				/*    eo[1] <=> text 			*/
} McSendDataStruct;

typedef struct _McPacketData {
	unsigned int nalloc;	/* nombre de packet qu'il faut alloues */
				/* pour avoir la completude */
	unsigned int nbre_rcv;	/* nombre de packet recu */
	char ** pdata;		/* tableau de packet pdata[d->nombre_packet] */
				/* si pdata[i] == NULL le packet manque */
				/* si nbre_rcv == d->nombre_packet alors */
				/* on a recu tous les packets, on reconstitue */
				/* eos[x] */
}McPacketData;
	
typedef struct _McRcvDataStruct {
	unsigned int 	id;	/* the id is unique */
	char *		alias;	/* alias_name */
	char * 		url;	/* the url of actual hypertext */
	char * 		text;	/* the hyper text to send */
	unsigned int 	neo;	/* number of embedded objects */
				/* essentially IMG */
	unsigned int *	seo;	/* taille de chaque eo . size of eo */
	char ** 	eos;	/* eo[neo]. Each element is data of eo*/
				/* but :				*/
				/*    eo[0] <=> alias			*/
				/*    eo[1] <=> URL			*/
				/*    eo[2] <=> text 			*/
				/* NULL if all data not here		*/
	McPacketData *  pds;	/* pds[neo] */
} McRcvDataStruct;

typedef struct _mc_user {
	unsigned int ip_addr;	/* ip address of user */
	pid_t pid ;             /* user's pid  */  
	char url[MC_MAX_URL_SIZE+1]; /* url send by this user */
	unsigned int url_id;    /* the id of the url sent */
	time_t last_rcv_time;  	/* last time i see him (my time) as rcver */
	time_t last_sender_time;/* last time i see him (my time) as sender*/
	time_t player_goto_time;/* Player's last upd time (gmt time)*/
	time_t my_goto_time;	/* Last msg receive by this user (my gmt time)*/
	time_t player_fdata_time;/* Player's last upd time (gmt time)*/
	time_t my_fdata_time;	/* Last msg receive by this user (my gmt time) */
	unsigned char len_alias;
	char alias[MC_MAX_ALIAS_SIZE+1]; /* alias de l'utilisateur  */
	int mc_list_number;	/* entry Number in mc_list */
	int rcv_enable;		/* 0 no, 1 yes */
	char *          text;   /* the hyper text i receive */
	unsigned int    neo;    /* number of embedded objects */
                                /* essentially IMG */
        unsigned int *  seo;    /* taille de chaque eo . size of eo */
        char **         eos;    /* eo[neo]. Each element is data of eo*/
                                /* but :                                */
                                /*    eo[0] <=> alias                   */
                                /*    eo[1] <=> URL                     */
                                /*    eo[2] <=> text                    */
                                /* NULL if all data not here            */
	char ** filename;	/* associated filename to eos */
        McPacketData *  pds;    /* pds[neo] */
	mo_window * win ;	/* window for this user */
	int oldx;		/* oldx cursor position */
	int oldy;		/* oldy cursor position */
	struct _mc_user * next; /* liste pointeur */
	struct _mc_user * prev; /* back pointer */
} McUser, *McUserList;

#endif /* MC_DEFS_H */

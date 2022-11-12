/* mplugin.h
 * Version 3.6.9 [Oct2000]
 *
 * Author: 	Gilles Dauphin
 *		George Lise
 *		Hours Simon
 *		Deurveiller Gilles
 *		Ferlicot FrederiC
 *
 * Copyright (C) 2000 - ENST
 * See the file "license.mMosaic" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#ifndef MPLUGIN_H
#define MPLUGIN_H

#define MP_PLUGIN_RELEASE (0x0001000)   /* 1.0 */

typedef struct _HtmlObjectStruct *HtmlObjectPtr;

typedef int (*MPPInitializeProc)(HtmlObjectPtr); 
typedef void (*MPPNewProc)       (HtmlObjectPtr);        
typedef void (*MPPDestroyProc)   (HtmlObjectPtr);    

/* typedef void (*_MPP_Demande)(char *);                */
/* typedef void (*_MPP_NeedSocket)();                   */
/* typedef void (*_MPP_NeedFile)();                     */
/* typedef void (*_MPP_NeedNothing)();			*/

 
typedef struct _MosaicPlugjectClassRec
{
        int release;            /* release of mmosaic plugin */
        int i_cid;              /* internal id */
        char * class_id;        /* name of plugin */
        char * bin_path;        /* abs path to binary */
        char * codebase;        /* ??? */
        void * dl_handle;       /* Le pointeur sur le debut du plugin ***/
        MPPInitializeProc MPP_Initialize;
        MPPNewProc MPP_New;       /* pointeur de fonction MPP_New du plugin ***/
        MPPDestroyProc MPP_Destroy; /* pointeur de fonction MPP_Destroy ***/

/*  _MPP_Demande MPP_Demande;  */
/*  _MPP_NeedSocket MPP_NeedSocket; 		*/
/*  _MPP_NeedFile   MPP_NeedFile;                 */
/*  _MPP_NeedNothing MPP_NeedNothing;             */
                                                     
        void *plugin_ext;       /* if needed by plugin */
        void *ext;              /* futur use */      
} MosaicPlugjectClassRec, * MosaicPlugjectClass;

typedef enum { 
	MPP_URI_TO_FILE=1,
	MPP_URI_TO_FD,
	MPP_URI_TO_AURI,
	MPP_URI_TO_AS_IS
} MPPLoadDataMethod ;

typedef struct _ObjectParamStruct {
	char *name;            
        char *value;           
        char *valuetype;
} ObjectParamStruct, *ObjectParamPtr;

typedef struct _HtmlObjectStruct {
        int x;
        int y;
        int width;
        int height;
        int border_width;
        int valignment;

	char * class_id;	/* name of ... */
        char * bin_path;	/* classidPtr donne le bin_path */
	char *content_type;
	char *codebase;
	char *data_url;

	MPPLoadDataMethod load_data_method;
	char * fname_for_data;	/* in case of MPP_URI_TO_FILE */

/*	MPPLoadDataMethod load_valueref_method;   */

        int param_count;		/* number of params */
        ObjectParamPtr *param_t;	/* table of params */
        int cw_only;              	/* Boolean */
	void * frame;			/* XtWidget frame, xmRowColumn */
	MosaicPlugjectClass mp_class;
	int i_oid;	/* internal id */
	struct _HtmlObjectStruct *mp_rec;
	void *plugin_data;
} HtmlObjectStruct;                   


#endif /* MPLUGIN_H */


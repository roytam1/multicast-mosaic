/* This file is Copyright (C) 1996 - G.Dauphin
 * See the file "license.mMosaic" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */



extern	void EndForm(HTMLWidget hw, struct mark_up ** mptr,
        PhotoComposeContext * pcc, Boolean save_obj);
extern	void BeginForm(HTMLWidget hw, struct mark_up ** mptr,
        PhotoComposeContext * pcc, Boolean save_obj);

extern	void FormInputField(HTMLWidget hw, struct mark_up ** mptr,
        PhotoComposeContext * pcc, Boolean save_obj);

extern	void FormTextAreaBegin(HTMLWidget hw, struct mark_up ** mptr,
        PhotoComposeContext * pcc, Boolean save_obj);
extern	void FormTextAreaEnd(HTMLWidget hw, struct mark_up ** mptr,
        PhotoComposeContext * pcc, Boolean save_obj);
extern	void FormSelectOptionField(HTMLWidget hw, struct mark_up ** mptr,
        PhotoComposeContext * pcc, Boolean save_obj);
extern	void FormSelectBegin(HTMLWidget hw, struct mark_up ** mptr,
        PhotoComposeContext * pcc, Boolean save_obj);
extern	void FormSelectEnd(HTMLWidget hw, struct mark_up ** mptr,
        PhotoComposeContext * pcc, Boolean save_obj);




#ifdef MULTICAST

extern void McCreateMemberlist(void);
extern GuiEntry* CreateMemberGuiEntry(Source *s);
extern ChunkedDocEntry * GetChkDocAndCotab(Source *s, unsigned int url_id, unsigned int o_id);
extern void McDoWindowText(Source *s, unsigned int url_id);

#endif

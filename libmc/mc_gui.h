
#ifdef MULTICAST

extern GuiEntry * mc_gui_member_list;

extern void McCreateMemberlist(void);
extern GuiEntry* CreateMemberGuiEntry(Source *s);
extern void McDoWindowText(Source *s, unsigned int url_id);

#endif

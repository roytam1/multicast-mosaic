
#ifndef MO_WWW_H
#define MO_WWW_H

/* Bare minimum. */
struct _HText {
  char *expandedAddress;
  char *simpleAddress;

  /* This is what we should parse and display; it is *not* safe to free. */
  char *htmlSrc;
  /* This is what we should free. */
  char *htmlSrcHead;
  int srcalloc;    /* amount of space allocated */
  int srclen;      /* amount of space used , and is the len of htmlSrc*/
};

typedef struct _HText HText;    /* Normal Library */

extern HText * HTMainText;              /* Pointer to current main text */

extern void application_error(char *str, char *title);
extern int prompt_for_yes_or_no (char *questionstr);
extern char *mo_post_pull_er_over (char *url, char *content_type, char *data,
                            char **texthead);

#endif

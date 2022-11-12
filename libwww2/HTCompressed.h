#ifndef HTCOMPRESSED_H
#define HTCOMPRESSED_H

extern void HTCompressedFileToFile (char *fnam, int compressed,caddr_t appd);
extern void HTCompressedHText (HText *text, int compressed, int plain,caddr_t appd);

#endif /* not HTCOMPRESSED_H */

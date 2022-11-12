/*       File access in libwww
                              FILE ACCESS
                                             
   These are routines for local file access used by WWW browsers and
   servers. Implemented by HTFile.c.
   
   If the file is not a local file, then we pass it on to HTFTP in
   case it can be reached by FTP.

 */
#ifndef HTFILE_H
#define HTFILE_H

#include "HTFormat.h"
#include "HTAccess.h"
#include "HTML.h"               /* SCW */

/* Controlling globals

   These flags control how directories and files are represented as
   hypertext, and are typically set by the application from command
   line options, etc.
   
 */
extern int HTDirAccess; /* Directory access level */

#define HT_DIR_FORBID           0       /* Altogether forbidden */
#define HT_DIR_SELECTIVE        1       /* If HT_DIR_ENABLE_FILE exists */
#define HT_DIR_OK               2       /* Any accesible directory */

#define HT_DIR_ENABLE_FILE      ".www_browsable" /* If exists, can browse */

extern HTList *HTSuffixes;

/* Convert filenames between local and WWW formats */
extern char * HTLocalName PARAMS((WWW_CONST char * name));

/*

HTSetSuffix: Define the representation for a file suffix

   This defines a mapping between local file suffixes and file content
   types and encodings.
   
  ON ENTRY,
  
  suffix includes the "." if that is important (normally, yes!)
                         
  representation is MIME-style content-type
                         
  encoding is MIME-style content-transfer-encoding (8bit, 7bit, etc)
                         
  quality an a priori judgement of the quality of such files
  (0.0..1.0)
                         
 */
/* Example:   HTSetSuffix(".ps", "application/postscript", "8bit", 1.0);
**
*/

extern void HTSetSuffix PARAMS((
        WWW_CONST char *    suffix,
        WWW_CONST char *    representation,
        WWW_CONST char *    encoding,
        float           quality));
        

/*

HTFileFormat: Get Representation and Encoding from file name

  ON EXIT,
  
  return                 The represntation it imagines the file is in
                         
  *pEncoding             The encoding (binary, 7bit, etc). See HTSetSuffix.
                         
 */

#define COMPRESSED_NOT    0
#define COMPRESSED_BIGZ   1
#define COMPRESSED_GNUZIP 2

extern HTFormat HTFileFormat PARAMS((
                char *    filename,
                HTAtom **       pEncoding,
                HTAtom *,
                int *compressed));
extern char * HTFileMimeType PARAMS((
                WWW_CONST char *    filename,
                WWW_CONST char *    default_type));
extern char *HTDescribeURL (char *);

/* Determine file value from file name */

extern float HTFileValue PARAMS(( WWW_CONST char * filename));


/* Determine write access to a file

  ON EXIT,
  
  return value YES if file can be accessed and can be written to.
                         
  BUGS
   Isn't there a quicker way?
 */

extern HT_BOOL HTEditable PARAMS((WWW_CONST char * filename));

/* Determine a suitable suffix, given the representation

  ON ENTRY,
  rep                     is the atomized MIME style representation
                         
  ON EXIT,
  returns a pointer to a suitable suffix string if one has been found,
                         else NULL.
*/
extern WWW_CONST char * HTFileSuffix PARAMS((
                HTAtom* rep));



/* The Protocols */
extern HTProtocol HTFTP, HTFile;

#endif /* HTFILE_H */

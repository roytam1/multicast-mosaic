/*
 * Author: Gilles Dauphin
 * Version 3.1.1 [May97]
 *
 * Copyright (C) 1997 - G.Dauphin, P.Dax
 *
 * See the file "license.mMosaic" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * Bug report : dauphin@sig.enst.fr dax@inf.enst.fr
 *
 * useful types
 */

#ifndef mosaic_types_h
#define mosaic_types_h

#if defined(sgi) || defined(__bsdi__) || defined(__FreeBSD__) || defined(ultrix) || defined(linux) || defined(netBSD)
#include <sys/types.h>
#else
typedef signed char int8_t;
typedef unsigned char u_int8_t;        
typedef short int16_t;                 
typedef unsigned short u_int16_t;      
typedef unsigned int u_int32_t;
typedef int int32_t;
#endif

#endif /* mosaic_types_h */

/* Please read copyright.ncsa. Don't remove next line */
#include "copyright.ncsa"

/* Author: DXP 

 A lot of this is copied from the PNGLIB file example.c

 Modified:

    August   1995 - Glenn Randers-Pehrson <glennrp@arl.mil>
                    Changed dithering to use a 6x6x6 color cube.

    March 21 1996 - DXP
                    Fixed some interlacing problems.
                  
*/

#ifdef HAVE_PNG

#include <stdio.h>
#include <X11/Intrinsic.h>
#include <setjmp.h>

#include "../libhtmlw/HTML.h"
#include "mosaic.h"
#include "readPNG.h"

#define MAX(x,y)  (((x) > (y)) ? (x) : (y))

unsigned char * ReadPNG(FILE *infile,int *width, int *height, XColor *colrs)
{
	unsigned char *pixmap;
	unsigned char *p;
	png_byte *q;
	png_struct *png_ptr;
	png_info *info_ptr;
	double screen_gamma;
	png_byte *png_pixels=NULL, **row_pointers=NULL;
	int i, j;
	unsigned int packets;
	png_color std_color_cube[216];
	int ret;
	png_byte buf[8];

    
        /* first check to see if its a valid PNG file. If not, return. */
        /* we assume that infile is a valid filepointer */

	ret = fread(buf, 1, 8, infile);
	if(ret != 8)
		return 0;
	ret = png_check_sig(buf, 8);
	if(!ret)
		return(0);

        /* OK, it is a valid PNG file, so let's rewind it, and start 
           decoding it */
	rewind(infile);

        /* allocate the structures */
	png_ptr = (png_struct *)malloc(sizeof(png_struct));
	if(!png_ptr)
		return 0;

	info_ptr = (png_info *)malloc(sizeof(png_info));
	if(!info_ptr) {
		free(png_ptr);
		return 0;
	}

        /* Establish the setjmp return context for png_error to use. */
	if (setjmp(png_ptr->jmpbuf)) {
        
        	if (mMosaicSrcTrace) {
            		fprintf(stderr, "\n!!!libpng read error!!!\n");
        	}
		png_read_destroy(png_ptr, info_ptr, (png_info *)0); 

		if(png_pixels != NULL)
			free((char *)png_pixels);
		if(row_pointers != NULL)
			free((png_byte **)row_pointers);
		free((char *)png_ptr);
		free((char *)info_ptr);
		return 0;
	}

    /* SWP -- Hopefully to fix cores on bad PNG files */
    /*####png_set_message_fn(png_ptr,png_get_msg_ptr(png_ptr),NULL,NULL); */

        /* initialize the structures */
	png_info_init(info_ptr);
	png_read_init(png_ptr);
    
        /* set up the input control */
	png_init_io(png_ptr, infile);
    
        /* read the file information */
	png_read_info(png_ptr, info_ptr);
    
        /* setup other stuff using the fields of png_info. */
    
	*width = (int)png_ptr->width;
	*height = (int)png_ptr->height;

	if (mMosaicSrcTrace) {
		fprintf(stderr,"\n\nBEFORE\nheight = %d\n", (int)png_ptr->width);
		fprintf(stderr,"width = %d\n", (int)png_ptr->height);
		fprintf(stderr,"bit depth = %d\n", info_ptr->bit_depth);
		fprintf(stderr,"color type = %d\n", info_ptr->color_type);
		fprintf(stderr,"compression type = %d\n", info_ptr->compression_type);
		fprintf(stderr,"filter type = %d\n", info_ptr->filter_type);
		fprintf(stderr,"interlace type = %d\n", info_ptr->interlace_type);
		fprintf(stderr,"num colors = %d\n",info_ptr->num_palette);
		fprintf(stderr,"rowbytes = %d\n", info_ptr->rowbytes);
	}
#if 0
        /* This handles alpha and transparency by replacing it with 
           a background value. */
        /* its #if'ed out for now cause I don't have anything to 
           test it with */
	{
		png_color_16 my_background;
		if (info_ptr->valid & PNG_INFO_bKGD)
			png_set_background(png_ptr, &(info_ptr->background),
					PNG_GAMMA_FILE, 1, 1.0);
		else
			png_set_background(png_ptr, &my_background,
					PNG_GAMMA_SCREEN, 0, 1.0);
	}
#endif

        /* strip pixels in 16-bit images down to 8 bits */
	if (info_ptr->bit_depth == 16)
		png_set_strip_16(png_ptr);

/* If it is a color image then check if it has a palette. If not
then dither the image to 256 colors, and make up a palette */
	if (info_ptr->color_type==PNG_COLOR_TYPE_RGB ||
	    info_ptr->color_type==PNG_COLOR_TYPE_RGB_ALPHA) {

		if(! (info_ptr->valid & PNG_INFO_PLTE)) {
	            	if (mMosaicSrcTrace) {
                		fprintf(stderr,"dithering (RGB->palette)...\n");
            		}
                /* if there is is no valid palette, then we need to make
                   one up */
			for(i=0;i<216;i++) {	 /* 255.0/5 = 51 */
				std_color_cube[i].red=(i%6)*51;
				std_color_cube[i].green=((i/6)%6)*51;
				std_color_cube[i].blue=(i/36)*51;
			}

                /* this should probably be dithering to 
                   Rdata.colors_per_inlined_image colors */
			png_set_dither(png_ptr, std_color_cube, 216, 216, NULL, 1);
		} else {
            		if (mMosaicSrcTrace) {
                		fprintf(stderr,"dithering (RGB->file supplied palette)...\n");
            		}
			png_set_dither(png_ptr, info_ptr->palette, 
					info_ptr->num_palette,
					mMosaicAppData.colors_per_inlined_image,
					info_ptr->hist, 1);
		}
	}
/* PNG files pack pixels of bit depths 1, 2, and 4 into bytes as
           small as they can. This expands pixels to 1 pixel per byte, and
           if a transparency value is supplied, an alpha channel is
           built.*/
	if (info_ptr->bit_depth < 8)
		png_set_packing(png_ptr);

/* have libpng handle the gamma conversion */

	if (mMosaicAppData.use_screen_gamma) { /*SWP*/
		if (info_ptr->bit_depth != 16) {  /* temporary .. glennrp */
			screen_gamma=(double)(mMosaicAppData.screen_gamma);
            
            if (mMosaicSrcTrace) {
                fprintf(stderr,"screen gamma=%f\n",screen_gamma);
            }
			if (info_ptr->valid & PNG_INFO_gAMA) {
				if (mMosaicSrcTrace) {
					printf("setting gamma=%f\n",info_ptr->gamma);
				}
				png_set_gamma(png_ptr, screen_gamma, (double)info_ptr->gamma);
			} else {
				if (mMosaicSrcTrace) {
					fprintf(stderr,"setting gamma=%f\n",0.45);
				}
				png_set_gamma(png_ptr, screen_gamma, (double)0.45);
			}
		}
	}
    
	if (info_ptr->interlace_type)
		png_set_interlace_handling(png_ptr);

	png_read_update_info(png_ptr, info_ptr);
    
	if (mMosaicSrcTrace) {
		fprintf(stderr,"\n\nAFTER\nheight = %d\n", (int)png_ptr->width);
		fprintf(stderr,"width = %d\n", (int)png_ptr->height);
		fprintf(stderr,"bit depth = %d\n", info_ptr->bit_depth);
		fprintf(stderr,"color type = %d\n", info_ptr->color_type);
		fprintf(stderr,"compression type = %d\n", info_ptr->compression_type);
		fprintf(stderr,"filter type = %d\n", info_ptr->filter_type);
		fprintf(stderr,"interlace type = %d\n", info_ptr->interlace_type);
		fprintf(stderr,"num colors = %d\n",info_ptr->num_palette);
		fprintf(stderr,"rowbytes = %d\n", info_ptr->rowbytes);
	}
        /* allocate the pixel grid which we will need to send to 
           png_read_image(). */
	png_pixels = (png_byte *)malloc(info_ptr->rowbytes * 
			(*height) * sizeof(png_byte));
    
	row_pointers = (png_byte **) malloc((*height) * sizeof(png_byte *));
	for (i=0; i < *height; i++)
		row_pointers[i]=png_pixels+(info_ptr->rowbytes*i);

/* FINALLY - read the darn thing. */
	png_read_image(png_ptr, row_pointers);
    
/* now that we have the (transformed to 8-bit RGB) image, we have
to copy the resulting palette to our colormap. */
	if (info_ptr->color_type & PNG_COLOR_MASK_COLOR) {
		if (info_ptr->valid & PNG_INFO_PLTE) {
			for (i=0; i < info_ptr->num_palette; i++) {
				colrs[i].red = info_ptr->palette[i].red << 8;
				colrs[i].green = info_ptr->palette[i].green << 8;
				colrs[i].blue = info_ptr->palette[i].blue << 8;
				colrs[i].pixel = i;
				colrs[i].flags = DoRed|DoGreen|DoBlue;
			}
		} else {
			for (i=0; i < 216; i++) {
				colrs[i].red = std_color_cube[i].red << 8;
				colrs[i].green = std_color_cube[i].green << 8;
				colrs[i].blue = std_color_cube[i].blue << 8;
				colrs[i].pixel = i;
				colrs[i].flags = DoRed|DoGreen|DoBlue;
			}	    
		}
	} else {	 /* grayscale image */
		for(i=0; i < 256; i++ ) {
			colrs[i].red = i << 8;
			colrs[i].green = i << 8; 	    
			colrs[i].blue = i << 8;
			colrs[i].pixel = i;
			colrs[i].flags = DoRed|DoGreen|DoBlue;    
		}
	}
/* Now copy the pixel data from png_pixels to pixmap */
	pixmap = (png_byte *)malloc((*width) * (*height) * sizeof(png_byte));
	p = pixmap; q = png_pixels;
/* if there is an alpha channel, we have to get rid of it in the
pixmap, since I don't do anything with it yet */
	if (info_ptr->color_type & PNG_COLOR_MASK_ALPHA) {
	        if (mMosaicSrcTrace) {
            		fprintf(stderr,"Getting rid of alpha channel\n");
        	}
		for(i=0; i<*height; i++) {
			q = row_pointers[i];
			for(j=0; j<*width; j++) {
				*p++ = *q++; /*palette index*/
				q++; /* skip the alpha pixel */
			}
		}
		free((char *)png_pixels);
	} else {
		if (mMosaicSrcTrace) {
			fprintf(stderr,"No alpha channel\n");
		}
		for(i=0; i<*height; i++) {
			q = row_pointers[i];
			for(j=0; j<*width; j++) {
				*p++ = *q++; /*palette index*/
			}
		}
		free((char *)png_pixels);
	}
	free((png_byte **)row_pointers);
/* clean up after the read, and free any memory allocated */
	png_read_destroy(png_ptr, info_ptr, (png_info *)0);
/* free the structures */
	free((char *)png_ptr);
	free((char *)info_ptr);
	return pixmap;
}
#endif

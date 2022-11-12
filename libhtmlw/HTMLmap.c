#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>

#include "HTMLmiscdefs.h"
#include "HTMLparse.h"
#include "HTMLP.h"
#include "HTMLPutil.h"

/* #define DEBUG_MAP */

static int IfInArea (struct ele_rec *eptr, AreaRec *area,Dimension xpos, Dimension ypos)
{
	int *coord = area->coords;
	int n_coord = area->n_coords;
	Dimension lx, rx, top_y, bot_y;

	lx = eptr->x; top_y = eptr->y;
	rx = lx + eptr->width; bot_y = top_y + eptr->height;

	switch(area->shape) {
	case AREA_RECT: {
		Dimension X1=0,Y1=0,X2=0,Y2=0;

		X1 = coord[0];
  		Y1 = coord[1];
 		X2 = coord[2];
		Y2 = coord[3];
		return (    xpos >= (X1 + lx) 
			&&  xpos <= (X2 + lx)
			&&  ypos >= (Y1 + top_y)
			&&  ypos <= (Y2 + top_y));
		}
	case AREA_CIRCLE: {
		int x_center,y_center,radius,r_max,x_dist,y_dist;
		Dimension x_coord,y_coord;

		x_coord = eptr->width>>1; y_coord = eptr->height>>1;
		r_max = y_coord;
		if(x_coord < y_coord)
	  		r_max = x_coord;
		radius = r_max;

   		x_coord = coord[0];
  		y_coord = coord[1];
		r_max = coord[2];
		if(r_max < 0) /* relative value */
	  		r_max = ((-r_max*radius)/100);

		x_center = lx + x_coord; y_center = top_y + y_coord;
		x_dist = xpos - x_center;
		y_dist = ypos - y_center;
		radius = sqrt(x_dist*x_dist + y_dist*y_dist);
		return (radius <= r_max);
	}

	case AREA_POLYGON: {
		XPoint *points;
		int i,j,nr_points = 0;
		Region regio;

		nr_points =n_coord/2;
		points = (XPoint *) calloc(nr_points+1,sizeof(XPoint));
		i = 0; j=0;
		while( j < n_coord) {
			points[i].x = coord[j] + lx;
			points[i].y = coord[j+1] + top_y;
			i++;
			j = j+2;
   		}
		if(points[0].x != points[i-1].x && points[0].y != points[i-1].y)
   		{++nr_points;
			points[i].x = points[0].x;
			points[i].y = points[0].y;
   		}
		regio = XPolygonRegion(points,nr_points,WindingRule);
		i = XPointInRegion(regio,xpos,ypos);
		XDestroyRegion(regio);
		free(points);
		return i;
	}

	case AREA_DEFAULT:
		return (   xpos >= lx && xpos <= rx 
				&& ypos >= top_y && ypos <= bot_y);
	default: break;
   	}
	return FALSE;
}

int MapAreaFound(HTMLWidget hw, struct ele_rec *eptr,
	Dimension xpos, Dimension ypos, char** href )
{
	MapRec *map = eptr->pic_data->map;
	AreaRec *area;
	int n_area,i;

#ifdef DEBUG_MAP	
fprintf(stderr,"start MapAreaFound(%d,%d)\n",xpos,ypos);
#endif
	xpos += hw->html.scroll_x;
	ypos += hw->html.scroll_y;

	n_area = map->n_area;

	for(i=0; i<n_area; i++) {
		area = map->areas[i];
		if(IfInArea(eptr, area,xpos,ypos)) {
			*href = area->href;
			if (*href)
				return True;
			else
				return False;
		}
  	}
	return FALSE;

#ifdef DEBUG_MAP
fprintf(stderr,"IfMapAreaFound(): HREF:%s, ALT:%s\n", area->href,area->alt);
#endif	
}


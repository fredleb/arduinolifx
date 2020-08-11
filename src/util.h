#ifndef UTIL_H
#define UTIL_H

#if 0
//#define htons(x) ((uint16_t)( ((x)<<8) | (((x)>>8)&0xFF) ))
#define htons(x) (x)
#define ntohs(x) htons(x)

/*
#define htonl(x) ( ((x)<<24 & 0xFF000000UL) | \
                   ((x)<< 8 & 0x00FF0000UL) | \
                   ((x)>> 8 & 0x0000FF00UL) | \
                   ((x)>>24 & 0x000000FFUL) )
*/
#define htonl(x) (x)
#define ntohl(x) htonl(x)
#endif

#endif

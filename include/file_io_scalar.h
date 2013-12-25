/******************************************************************************

            Copyright (c) 1996 - 2004  --  University of Washington

******************************************************************************/

#ifndef __FILE_IO_SCALAR_H_
#define __FILE_IO_SCALAR_H_

#ifndef fscanf0
#define fscanf0 fscanf
#endif

#ifndef fscanf2
#define fscanf2 fscanf
#endif

#define _SCALAR_TEST (_INDEX==0 || _PRIV_ACCESS(_io_in_ens))

#define _SCALAR_FPRINTF0(str,ctl) (_SCALAR_TEST ? \
				   fprintf((str)->fptr,ctl), \
				   fflush((str)->fptr) : 0)
#define _SCALAR_FPRINTF1(str,ctl,a1) (_SCALAR_TEST ? \
				   fprintf((str)->fptr,ctl,a1), \
				   fflush((str)->fptr) : 0)
#define _SCALAR_FPRINTF2(str,ctl,a1,a2) (_SCALAR_TEST ? \
				   fprintf((str)->fptr,ctl,a1,a2), \
				   fflush((str)->fptr) : 0)
#define _SCALAR_FPRINTF3(str,ctl,a1,a2,a3) (_SCALAR_TEST ? \
				   fprintf((str)->fptr,ctl,a1,a2,a3), \
				   fflush((str)->fptr) : 0)
#define _SCALAR_FPRINTF4(str,ctl,a1,a2,a3,a4) (_SCALAR_TEST ? \
				   fprintf((str)->fptr,ctl,a1,a2,a3,a4), \
				   fflush((str)->fptr) : 0)
#define _SCALAR_FPRINTF5(str,ctl,a1,a2,a3,a4,a5) (_SCALAR_TEST ? \
				   fprintf((str)->fptr,ctl,a1,a2,a3,a4,a5), \
				   fflush((str)->fptr) : 0)
#define _SCALAR_FPRINTF6(str,ctl,a1,a2,a3,a4,a5,a6) (_SCALAR_TEST ? \
				   fprintf((str)->fptr,ctl,a1,a2,a3,a4,a5,a6), \
				   fflush((str)->fptr) : 0)

#define _SCALAR_FSCANF0(str,ctl) (_SCALAR_TEST ? fscanf0((str)->fptr,ctl) : 0)

#define _SCALAR_FSCANF1(str,ctl,a1) (_SCALAR_TEST ? \
				     fscanf((str)->fptr,ctl,a1) : 0)

#define _SCALAR_FSCANF2(str,ctl,a1,a2) (_SCALAR_TEST ? \
                                        fscanf2((str)->fptr,ctl,a1,a2) : 0)

#define _SCALAR_FWRITE(ptr,siz,num,str) (_SCALAR_TEST ? \
					 fwrite(ptr,siz,num,(str)->fptr) : 0)

#define _SCALAR_FREAD(ptr,siz,num,str) (_SCALAR_TEST ? \
					fread(ptr,siz,num,(str)->fptr) : 0)

#define _SCALAR_LINEFEED(str) (_SCALAR_TEST ? fprintf((str)->fptr,"\n"), \
			                      fflush((str)->fptr) : 0)

#define _SCALAR_FPRINTF1_bool(str,a1) (_SCALAR_TEST ? \
                                        fprintf((str)->fptr,"%s", \
						(a1 ? "true" : "false")), \
				        fflush((str)->fptr) : 0)

#define _SCALAR_FSCANF1_sbyte(str,a1) if (_SCALAR_TEST) { \
                                        int _byteval; \
					fscanf((str)->fptr,"%d",&_byteval); \
					*(a1) = _byteval; \
				       }

#define _SCALAR_FSCANF1_ubyte(str,a1) if (_SCALAR_TEST) { \
					unsigned int _byteval; \
					fscanf((str)->fptr,"%u",&_byteval); \
					*(a1) = _byteval; \
				      }

#define _SCALAR_BCAST(ptr,sz) if (!_PRIV_ACCESS(_io_in_ens)) { \
                                    _BroadcastSimple(_DefaultGrid, ptr,sz,0,0); \
                                  }

#endif

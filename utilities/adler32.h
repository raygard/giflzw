/* adler32.h */
/* This implementation Copyright 1998 Raymond D. Gardner  */

unsigned long update_adler32(unsigned long adler, unsigned char *buf, int len);
unsigned long adler32(unsigned char *buf, int len);

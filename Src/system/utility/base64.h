#ifndef __BASE64_H__
#define __BASE64_H__

extern int base64_encode(const unsigned char * sourcedata, char * base64);
extern int base64_decode(const char * base64, unsigned char * dedata);

#endif

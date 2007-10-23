#ifndef _TEA_
#define _TEA_

void encrypt(unsigned long* v, unsigned long* k); 
void encrypt_cbc(unsigned long *in, unsigned long len, unsigned long *key, unsigned long *out); 
void decrypt(unsigned long* v, unsigned long* k); 
void decrypt_cbc(unsigned long *in, unsigned long len, unsigned long *key, unsigned long *out); 

#endif


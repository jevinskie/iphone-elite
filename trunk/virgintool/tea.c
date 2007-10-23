#include <stdio.h>
#include <stdlib.h>

void encrypt(unsigned long* v, unsigned long* k) {
  unsigned long v0, v1, sum, i, delta, k0, k1, k2, k3;
  delta=0X9E3779B9;                       /* a key schedule constant */
  k0=k[0]; k1=k[1]; k2=k[2]; k3=k[3];     /* cache key */
  v0 = v[0]; v1 = v[1]; sum=0;            /* set up */

  for (i=0; i < 32; i++) {                /* basic cycle start */
    sum += delta;
    v0 += ((v1<<4) + k0) ^ (v1 + sum) ^ ((v1>>5) + k1);
    v1 += ((v0<<4) + k2) ^ (v0 + sum) ^ ((v0>>5) + k3); /* end cycle */
  }
  v[0]=v0; v[1]=v1;
}

/* See http://upload.wikimedia.org/wikipedia/en/d/d3/Cbc_encryption.png
   for a depiction of general CBC encryption */
void encrypt_cbc(unsigned long *in, unsigned long len, unsigned long *key, unsigned long *out) {
  unsigned long x[2], tea[2], offset;

  /* initialization vector */
  x[0] = x[1] = 0xffffffff;

  for (offset=0; offset<len; offset+=8) {  /* block size of TEA is 8 bytes */
    int blen = 8;
    if ((len-offset)<8)
      blen = len-offset;

    int i;
    for (i=0; i<(blen>>2); i++)
      tea[i] = in[(offset>>2)+i];

    tea[0] ^= x[0];    tea[1] ^= x[1];
    encrypt(tea, key);
    x[0]    = tea[0];    x[1]  = tea[1];
    
    /* Ciphertext */
    for (i=0; i<blen; i++)
      ((unsigned char *)out)[offset+i] = ((unsigned char *)tea)[i];
  }
}

void decrypt(unsigned long* v, unsigned long* k) {
  unsigned long v0, v1, sum, i, delta, k0, k1, k2, k3;
  delta=0X9E3779B9;                       /* a key schedule constant */
  k0=k[0]; k1=k[1]; k2=k[2]; k3=k[3];     /* cache key */
  v0 = v[0]; v1 = v[1]; sum=0xC6EF3720;   /* set up */

  for (i=0; i<32; i++) {                               /* basic cycle start */
    v1 -= ((v0<<4) + k2) ^ (v0 + sum) ^ ((v0>>5) + k3);
    v0 -= ((v1<<4) + k0) ^ (v1 + sum) ^ ((v1>>5) + k1);
    sum -= delta;                                      /* end cycle */
  }
  v[0]=v0; v[1]=v1;
}

/* See http://upload.wikimedia.org/wikipedia/en/6/66/Cbc_decryption.png 
   for a depiction of general CBC decryption */
void decrypt_cbc(unsigned long *in, unsigned long len, unsigned long *key, unsigned long *out) {
  unsigned long x[2], nx[2], dec[2], tea[2], offset;

  /* initialization vector */
  /* Note: this is referenced at 0xa02e8320+0x30 by sub_A0234BB8 4.0.1 */
  x[0] = x[1] = 0xffffffff;

  for (offset=0; offset<len; offset+=8) {  /* block size of TEA is 8 bytes */
    int blen = 8;
    if ((len-offset)<8)
      blen = len-offset;

    int i;
    for (i=0; i<(blen>>2); i++)
      tea[i] = in[(offset>>2)+i];

    nx[0]  = tea[0];  nx[1]  = tea[1];  // save these for next CBC round
    decrypt(tea, key);
    dec[0] = tea[0] ^ x[0];
    dec[1] = tea[1] ^ x[1];
    x[0]   = nx[0];   x[1]   = nx[1];
    
    /* Plaintext */
    for (i=0; i<blen; i++)
      ((unsigned char *)out)[offset+i] = ((unsigned char *)dec)[i];
  }
}



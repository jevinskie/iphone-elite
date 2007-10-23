char *
memcpy(char *destaddr, const char *srcaddr, int len)
{
    char *dest = destaddr;
    
    while (len-- > 0)
	*destaddr++ = *srcaddr++;
    
    return dest;
}

void *
memset(char *s, int c, int len)
{
    char *se = s + len;
    
    while(s < se)
	*s++ = c;

    return s;
}

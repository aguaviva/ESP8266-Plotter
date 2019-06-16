#if defined (__cplusplus)
extern "C" {
#endif

void my_twi_setClock(unsigned int freq);
void my_twi_setClockStretchLimit(uint32_t limit);
void my_twi_init(unsigned char sda, unsigned char scl);
void my_twi_setAddress(uint8_t address);
unsigned char my_twi_writeTo(unsigned char address, unsigned char * buf, unsigned int len, unsigned char sendStop);
unsigned char my_twi_readFrom(unsigned char address, unsigned char* buf, unsigned int len, unsigned char sendStop);

#if defined (__cplusplus)
}
#endif

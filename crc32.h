#ifndef CRC32_H_INCLUDED
#define CRC32_H_INCLUDED

#include <stdint.h>


#define CUSTOM_NAME	"user"

typedef
	uint32_t
	crc32_t;

extern int crc32_update( crc32_t *crc, void *data, size_t sz );
extern int crc32_init( crc32_t *crc );
extern int crc32_final( crc32_t *crc );

extern int crc32_setalgorithm( const char *name );
extern int crc32_setcustom( crc32_t initval, crc32_t finalxor, crc32_t polynom, int lsb );
extern void crc32_dumpalgos( void );

extern void crc32_dumpparam( void );
extern void crc32_dumptable( void );

#endif // CRC32_H_INCLUDED

/* EOF */

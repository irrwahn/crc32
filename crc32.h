#ifndef CRC32_H_INCLUDED
#define CRC32_H_INCLUDED

#include <stdint.h>


#define CUSTOM_NAME	"user"

/*
	Flag values
	Currently either none or both reflect flags must be set!
*/
#define CRC_RFIN	1						// reflect input
#define CRC_RFOUT	2						// reflect output
#define CRC_RFIO	(CRC_RFIN|CRC_RFOUT)	// reflect both, input and output

#define CRC_ROBYT	4						// reverse output byte(!) order


typedef
	uint32_t
	crc32_t;

extern int crc32_update( crc32_t *crc, void *data, size_t sz );
extern int crc32_init( crc32_t *crc );
extern int crc32_final( crc32_t *crc );
extern int crc32_calc( crc32_t *crc, void *data, size_t sz );

extern int crc32_setalgorithm( const char *name );
extern int crc32_setcustom( crc32_t initval, crc32_t finalxor, crc32_t polynom, unsigned reflect );
extern void crc32_dumpalgos( void );

#ifdef DEBUG
extern void crc32_dumpparam( void );
extern int crc32_dumptable( void );
extern int crc32_selftest( int dumpparm, int dumptbl );
#endif

#endif // CRC32_H_INCLUDED

/* EOF */

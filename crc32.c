/*
  crc32.c

  Prametrized 32bit cyclic redundancy check computation.
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "crc32.h"


#ifdef DEBUG
#define DPRINT(...)	fprintf(stderr,__VA_ARGS__)
#else
#define DPRINT(...)
#endif


typedef
	struct {
		const char *name;
		crc32_t poly;
		crc32_t init;
		crc32_t final;
		int lsb;
	}
	crc32_algo_t;

static crc32_algo_t algo[] = {
	{ "crc32", 		0x04C11DB7, 0xFFFFFFFF, 0xFFFFFFFF, 1 },	// ok
	{ "crc32c",		0x1edc6f41, 0xFFFFFFFF, 0xFFFFFFFF, 1 },	// not verified!
	{ "crc32d",		0xa833982b, 0xFFFFFFFF, 0xFFFFFFFF, 1 },	// not verified!
	{ "jamcrc32",	0x04C11DB7, 0xFFFFFFFF, 0x00000000, 1 },	// not verified!
	{ CUSTOM_NAME, 	0x04C11DB7, 0xFFFFFFFF, 0xFFFFFFFF, 1 },
	{ NULL, 0, 0, 0, 0 },
};

static crc32_algo_t *a = &algo[0];
static crc32_t table[256];
static int table_valid = 0;

static int find_algo( const char *name )
{
	if ( NULL == name )
		return errno = EINVAL, -1;
	for ( int i = 0; NULL != algo[i].name; ++i )
		if ( 0 == strcmp( name, algo[i].name ) )
			return i;
	return errno = EINVAL, -1;
}

static inline crc32_t bmirr( crc32_t val, unsigned int n )
{
	crc32_t ref = 0;

	while ( n-- )
	{
		ref = ( ref << 1 ) | ( val & 1 );
		val >>= 1;
	}
	return ref;
}

static void calc_table( crc32_t *table, crc32_t poly, int lsb )
{
	crc32_t pom = lsb ? bmirr( poly, 32 ) : poly;	// bit order!
	crc32_t rem;

	for ( int i = 0; i <= 0xFF; ++i )
	{
		rem = i;
		for ( int j = 0; j < 8; ++j )
			rem = (rem & 1) ? ( rem >> 1 ) ^ pom : rem >> 1;
		table[i] = rem;
	}
	table_valid = 1;
}

int crc32_update( crc32_t *crc, void *data, size_t sz )
{
	if ( NULL == crc )
		return errno = EINVAL, -1;

	crc32_t ncrc = *crc;
	uint8_t *p = data;

	while ( sz-- )
		ncrc = ( ncrc >> 8 ) ^ table[ ( ncrc & 0xFF ) ^ *p++ ];
	*crc = ncrc;
	return 0;
}

int crc32_init( crc32_t *crc )
{
	if ( NULL == crc )
		return errno = EINVAL, -1;
	if ( !table_valid )
		calc_table( table, a->poly, a->lsb );
	*crc = a->init;
	return 0;
}

int crc32_final( crc32_t *crc )
{
	if ( NULL == crc )
		return errno = EINVAL, -1;
	*crc ^= a->final;
	return 0;
}

int crc32_setalgorithm( const char *name )
{
	int i;

	if ( 0 > ( i = find_algo( name ) ) )
		return -1;
	table_valid = 0;
	a = &algo[i];
	return 0;
}

int crc32_setcustom( crc32_t initval, crc32_t finalxor, crc32_t polynom, int lsb )
{
	int i;

	if ( 0 > ( i = find_algo( CUSTOM_NAME ) ) )
		return -1;
	table_valid = 0;
	a = &algo[i];
	a->init = initval;
	a->final = finalxor;
	a->poly = polynom;
	a->lsb = lsb;
	return 0;
}


#define DUMPPARM(P)	\
	printf( "name=\"%s\" poly=0x%08X init=0x%08X final=0x%08X lsb=%d\n", \
			(P)->name, (P)->poly, (P)->init, (P)->final, (P)->lsb );


void crc32_dumpalgos( void )
{
	for ( int i = 0; NULL != algo[i].name; ++i )
		DUMPPARM( &algo[i] );
}

void crc32_dumpparam( void )
{
	DUMPPARM( a );
}

void crc32_dumptable( void )
{
	if ( !table_valid )
		calc_table( table, a->poly, a->lsb );
	printf( "crc32_t table[] = {" );
	for ( int i = 0; i <= 0xFF; ++i )
	{
		if ( 0 == i % 8 )
			printf( "\n\t" );
		printf( "0x%08X, ", table[i] );
	}
	printf( "\n};\n" );
}

/* EOF */

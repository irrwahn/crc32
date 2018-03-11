/*
 * This file is part of the crc32 project.
 *
 * Copyright (c) 2013 Urban Wallasch <irrwahn35@freenet.de>
 * See LICENSE file for more details.
 */
/*
  crc32.c

  Parametrized 32bit cyclic redundancy check computation.

  ISSUES:
  - The whole raffle is inherently thread-unsafe, unless it is guaranteed that
      a) the same algorithm is used by all threads, AND
      b) the first calls to crc32_setalgorithm() and crc32_init() are mutexed.
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "crc32.h"


#ifdef DEBUG
#define DPRINT(...)		fprintf(stderr,__VA_ARGS__)
#define TESTSTRING		"123456789"
#else
#define DPRINT(...)
#endif


typedef
	struct {
		const char *name;
		crc32_t poly;
		crc32_t init;
		crc32_t final;
		unsigned flags;
		crc32_t check;	// precalculated TESTSTRING crc, used in crc32_selftest()
	}
	crc32_algo_t;

/*
  Check values represent the CRC for the test string "123456789",
  used in self test procedure.
*/
static crc32_algo_t algo[] = {
	// name			polynomial  init val    final XOR   flags      check
	{ "CRC32", 		0x04C11DB7, 0xFFFFFFFF, 0xFFFFFFFF,  CRC_RFIO, 0xCBF43926 },	// ok, equivalent to PHP hash('crc32b', 'string')
	{ "BZIP2",		0x04C11DB7, 0xFFFFFFFF, 0xFFFFFFFF,         0, 0xFC891918 },	// ok
	{ "ETHER",		0x04C11DB7, 0xFFFFFFFF, 0xFFFFFFFF, CRC_ROBYT, 0x181989FC },	// ok, equivalent to PHP hash('crc32', 'string')
	{ "MPEG2",		0x04C11DB7, 0xFFFFFFFF, 0x00000000,         0, 0x0376E6E7 },	// ok
	{ "JAMCRC",		0x04C11DB7, 0xFFFFFFFF, 0x00000000,  CRC_RFIO, 0x340BC6D9 },	// ok
	{ "POSIX",		0x04C11DB7, 0x00000000, 0xFFFFFFFF,         0, 0x765E7680 },	// ok

	{ "CRC32C",		0x1EDC6F41, 0xFFFFFFFF, 0xFFFFFFFF,  CRC_RFIO, 0xE3069283 },	// ok (Castagnoli)
	{ "CRC32D",		0xA833982B, 0xFFFFFFFF, 0xFFFFFFFF,  CRC_RFIO, 0x87315576 },	// ok
	{ "CRC32K",		0x741B8CD7, 0xFFFFFFFF, 0xFFFFFFFF,         0, 0xD14EB786 },	// ok (Koopman) init/final/flags not stipulated!
	{ "CRC32Q",		0x814141AB, 0x00000000, 0x00000000,         0, 0x3010BF7F },	// ok
	{ "XFER",		0x000000AF, 0x00000000, 0x00000000,         0, 0xBD0BE338 },	// ok

	{ "sick",		0x04C11DB7, 0xFFFFFFFF, 0xFFFFFFFF, CRC_RFOUT, 0x1898913F },	// bzip2 w/ reflect out

	// add more presets above
	{ CUSTOM_NAME, 	0x04C11DB7, 0xFFFFFFFF, 0xFFFFFFFF,        1, 0x00000000 },
	{ NULL, 0, 0, 0, 0, 0 },
};

static crc32_algo_t *a = &algo[0];
static crc32_t table[256];
static int table_valid = 0;


static int str_icmp( const char *a, const char *b )
{
	while ( *a && *b && tolower( (unsigned char)*a ) == tolower( (unsigned char)*b ) )
		++a, ++b;
	return *a - *b;
}

static int find_algo( const char *name )
{
	if ( NULL == name )
		return errno = EINVAL, -1;
	for ( int i = 0; NULL != algo[i].name; ++i )
		if ( 0 == str_icmp( name, algo[i].name ) )
			return i;
	return errno = EINVAL, -1;
}

static inline crc32_t bit_reflect( crc32_t val, unsigned int n )
{
	crc32_t reflect = 0;

	while ( n-- )
	{
		reflect = ( reflect << 1 ) | ( val & 1 );
		val >>= 1;
	}
	return reflect;
}

static int init_table( crc32_t *table, crc32_t poly, unsigned flags )
{
	crc32_t i, rem;
	int j;

	if ( flags & CRC_RFIN )
	{
		poly = bit_reflect( poly, 32 );	// reverse bit order (lsb left)
		for ( i = 0; i <= 0xFF; ++i )
		{
			rem = i;
			for ( j = 0; j < 8; ++j )
				rem = (rem & 1) ? ( rem >> 1 ) ^ poly : rem >> 1;
			table[i] = rem;
		}
	}
	else
	{
		for ( i = 0; i <= 0xFF; ++i )
		{
			rem = i << 24;
			for ( j = 0; j < 8; ++j )
				rem = ( rem & 0x80000000UL ) ? ( rem << 1 ) ^ poly : rem << 1;
			table[i] = rem;
		}
	}
	table_valid = 1;
	return 0;
}

int crc32_update( crc32_t *crc, void *data, size_t sz )
{
	if ( NULL == crc )
		return errno = EINVAL, -1;

	crc32_t ncrc = *crc;
	uint8_t *p = data;

	if ( CRC_RFIN & a->flags )
	{
		while ( sz-- )
			ncrc = ( ncrc >> 8 ) ^ table[ ( ncrc & 0xFF ) ^ *p++ ];
	}
	else
	{
		while ( sz-- )
			ncrc = ( ncrc << 8 ) ^ table[ ( ncrc >> 24 ) ^ *p++ ];
	}

	if ( !(CRC_RFOUT & a->flags) != !(CRC_RFIN & a->flags) ) // mind the logical xor!
		ncrc = bit_reflect( ncrc, 32 );
	*crc = ncrc;
	return 0;
}

int crc32_init( crc32_t *crc )
{
	if ( NULL == crc )
		return errno = EINVAL, -1;
	if ( !table_valid && 0 != init_table( table, a->poly, a->flags ) )
		return -1;
	*crc = a->init;
	return 0;
}

int crc32_final( crc32_t *crc )
{
	if ( NULL == crc )
		return errno = EINVAL, -1;
	*crc ^= a->final;
	if ( a->flags & CRC_ROBYT )
	{
		crc32_t r = *crc << 24 | ( *crc & 0xff00 ) << 8 | ( *crc & 0xff0000 ) >> 8 | *crc >> 24;
		*crc = r;
	}
	return 0;
}

int crc32_calc( crc32_t *crc, void *data, size_t sz )
{
	if ( 0 != crc32_init( crc ) )
		return -1;
	if ( 0 != crc32_update( crc, data, sz ) )
		return -1;
	return crc32_final( crc );
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

int crc32_setcustom( crc32_t initval, crc32_t finalxor, crc32_t polynom, unsigned flags )
{
	int i;

	if ( 0 > ( i = find_algo( CUSTOM_NAME ) ) )
		return -1;
	table_valid = 0;
	a = &algo[i];
	a->init = initval;
	a->final = finalxor;
	a->poly = polynom;
	a->flags = flags;
	return 0;
}

#define DUMPPARM(P)	\
	printf( "name=\"%s\"\tpoly=0x%08X init=0x%08X final=0x%08X flags=0x%08X\n", \
			(P)->name, (P)->poly, (P)->init, (P)->final, (P)->flags );

void crc32_dumpalgos( void )
{
	for ( int i = 0; NULL != algo[i].name; ++i )
		DUMPPARM( &algo[i] );
}

#ifdef DEBUG

void crc32_dumpparam( void )
{
	DUMPPARM( a );
}

int crc32_dumptable( void )
{
	if ( !table_valid && 0 != init_table( table, a->poly, a->flags ) )
		return -1;
	printf( "crc32_t table[] = {" );
	for ( int i = 0; i <= 0xFF; ++i )
	{
		if ( 0 == i % 8 )
			printf( "\n\t" );
		printf( "0x%08X, ", table[i] );
	}
	printf( "\n};\n" );
	return 0;
}

int crc32_selftest( int dumpparm, int dumptbl )
{
	int err = 0;
	crc32_t crc;

	for ( int i = 0; NULL != algo[i].name; ++i )
	{
		if (  0 == strcmp( CUSTOM_NAME, algo[i].name ) )
			continue;	// do not check custom algo
		crc32_setalgorithm( algo[i].name );
		if ( 0 != crc32_init( &crc ) )
			return -1;
		if ( 0 != crc32_update( &crc, TESTSTRING, strlen( TESTSTRING ) ) )
			return -1;
		if ( 0 != crc32_final( &crc ) )
			return -1;
		printf( "%-8s %-5s got:%08X  expected:%08X\n", a->name, crc == a->check ? "OK" : "FAIL", crc, a->check );
		if ( dumpparm )
			printf( "\t" ), DUMPPARM( a );
		if ( dumptbl )
			crc32_dumptable();
		err += ( crc != a->check );
	}
	return err;
}

#endif

/* EOF */

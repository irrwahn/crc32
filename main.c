/*
  crc32

  Print parametrized CRC32 for files.

*/

// support large files > 2 GB
#define _FILE_OFFSET_BITS 64


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>


#include "crc32.h"


#define CHUNK_SIZE	( 4 * 1024 )


static size_t printsize = 0;


static int computeCRC32( FILE *fp, crc32_t *crc, size_t *sz )
{
	uint8_t buf[CHUNK_SIZE];
	off_t size = 0;
	off_t bread = 0;
	unsigned int n;

	if ( 0 != crc32_init( crc ) )
		return -1;
	if ( fp != stdin )
	{
		if ( 0 > fseeko( fp, 0, SEEK_END ) )
			return -1;
		if ( 0 > ( size = ftello( fp ) ) )
			return -1;
		rewind( fp );
		// printf( "size:%ld\n", size );
	}
	while ( 0 < ( n = fread( buf, 1, sizeof buf, fp ) ) )
	{
		bread += n;
		crc32_update( crc, buf, n );
	}
	// printf( "bread:%ld\n", bread );
	crc32_final( crc );
	*sz = bread;
	if ( ferror( fp ) )
		return -1;
	if ( fp != stdin && size != bread )
		return errno = ETXTBSY, -1;	// actually the file size changed during operation
	return 0;
}


static void usage( const char *argv0, int show_algo )
{
	const char *progname = strrchr( argv0, '/' );

    progname = progname ? progname + 1 : argv0;
	printf(
		"%s - compute CRC32 message digest\n"
		"Usage:\n"
		"  %s [OPTION] ... [FILE] ...\n"
		"Options:\n"
		"  -h       display this help and exit\n"
		"  -s|S		print number of processed bytes, S: human readable\n"
		"  -a NAME  use predefined algorithm NAME\n"
		"            \"-a help\" prints a list of preset algorithms.\n"
		"  -i HEX   initial CRC value, default: 0xFFFFFFFF\n"
		"  -x HEX   final XOR value, default: 0xFFFFFFFF\n"
		"  -p HEX   generator polynom, defaut: 0x04C11DB7\n"
		"  -f HEX   bitwise OR of flags, default: 3\n"
		"            1:reflect input\n"
		"            2:reflect output\n"
		"            4:reverse output byte order\n"
#ifdef DEBUG
		"  -d       dump current parameter set\n"
		"  -D       dump resulting remainder table\n"
		"  -T       execute self test and exit (can be combined with d and D)\n"
#endif
		"Description:\n"
		"  Compute and print CRC32 checksums for files.\n"
		"  With no FILE, or when FILE is -, read standard input.\n"
		, progname, progname
	);
	if ( show_algo )
	{
		printf( "\nPreset algorithms:\n" );
		crc32_dumpalgos();
		printf( "\n" );
	}
}


static int do_config( int argc, char *argv[] )
{
	int opt;
	const char *ostr = "+:hsSa:f:i:p:x:"
#ifdef DEBUG
		"dDT";
	int dumpparam = 0;
	int dumptable = 0;
	int selftest = 0;
#else
		;
#endif
	const char *algo = "crc32";
	int custom = 0;
	crc32_t init = 0xFFFFFFFF;
	crc32_t final = 0xFFFFFFFF;
	crc32_t poly = 0x04C11DB7;
	unsigned flags = CRC_RFIO;

	while ( -1 != ( opt = getopt( argc, argv, ostr ) ) )
	{
		switch ( opt )
		{
#ifdef DEBUG
		case 'd':
			dumpparam = 1;
			break;
		case 'D':
			dumptable = 1;
			break;
		case 'T':
			selftest = 1;
			break;
#endif
		case 'a':
			algo = optarg;
			custom = 0;
			break;
		case 'f':
			flags = strtoul( optarg, NULL, 16 );
			++custom;
			break;
		case 'i':
			init = strtoul( optarg, NULL, 16 );
			++custom;
			break;
		case 'p':
			poly = strtoul( optarg, NULL, 16 );
			++custom;
			break;
		case 's':
			printsize = 1;
			break;
		case 'S':
			printsize = 2;
			break;
		case 'x':
			final = strtoul( optarg, NULL, 16 );
			++custom;
			break;
		case ':':
			fprintf( stderr, "Missing argument for option '%c'\n", optopt );
			return -1;
			break;
		case '?':
		default:
			fprintf( stderr, "Unrecognized option '%c'\n", optopt );
		case 'h':
			return -1;
			break;
		}
	}

	if ( 0 < custom )
	{
		crc32_setcustom( init, final, poly, flags );
	}
	else if ( 0 != crc32_setalgorithm( algo ) )
	{
		if ( 0 != strcmp( algo, "help" ) )
			fprintf( stderr, "Unrecognized algorithm '%s'.\n", algo );
		return -2;
	}
#ifdef DEBUG
	if ( selftest )
	{
		int e;
		printf( "Executing selftest...\n" );
		if ( 0 > ( e = crc32_selftest( dumpparam, dumptable ) ) )
			perror( "self test" );
		else
			printf( "Unexpected failures: %d\n", e );
		exit( e ? EXIT_FAILURE : EXIT_SUCCESS );
	}
	if ( dumpparam )
		crc32_dumpparam();
	if ( dumptable && 0 != crc32_dumptable() )
		return perror( "dumptable" ), -1;
#endif
	return optind;
}


int main( int argc, char *argv[] )
{
	int i, err = 0;
	uint32_t crc = 0;
	size_t sz;
	FILE *fp;
	char *files[] = { "-", NULL };

	if ( 0 > ( i = do_config( argc, argv ) ) )
	{
		usage( argv[0], -1 > i );
		exit( EXIT_FAILURE );
	}
	if ( NULL == argv[optind] )
	{	// no input files specified, fake '-' option
		argv = files;
		optind = 0;
	}

	for ( i = optind; NULL != argv[i]; i++ )
	{
		if ( 0 == strcmp( "-", argv[i] ) )
			fp = stdin;
		else if ( NULL == ( fp = fopen( argv[i], "r" ) ) )
		{
			perror( argv[i] );
			++err;
			continue;
		}
		if ( 0 != computeCRC32( fp, &crc, &sz ) )
		{
			perror( argv[i] );
			++err;
		}
		else
		{
			const char *name;
			name = stdin == fp ? "" : argv[i];
			if ( printsize )
			{
				int pfx = 0;
				const char *spfx = " KMGTPEZY";
				while ( 1 < printsize && 1024 < sz )
					sz /= 1024, ++pfx;
				printf( "%08X %3zu%c %s\n", crc, sz, spfx[pfx], name );
			}
			else
				printf( "%08X %s\n", crc, name );
		}
		if ( stdin != fp )
			fclose( fp );
    }
	exit( err ? EXIT_FAILURE : EXIT_SUCCESS );
}

/* EOF */

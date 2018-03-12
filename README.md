# crc32

Crc32 - print parametrized CRC32 for files.

## What

The crc32 utility calculates and prints the CRC32 checksum for each
input file, using one of the predefined CRC parameter sets or user
supplied custom parameters.

## Build

Simply run:

        make

## Install

Running

        make install

will install `crc32` to `$PREFIX/bin/` (by default /usr/local/bin).

## Usage

The `crc32` utility is typically used like this:

        crc32 [-a PRESET] [-|file ...]

to read input from stdin or file(s) and calculate the CRC32, where
`PRESET` specifies one of the following:

        Preset   Polynomial   Comment
        --------------------------------------------------------
        CRC32    0x04C11DB7   default; equiv. to PHP hash crc32b
        BZIP2    0x04C11DB7
        ETHER    0x04C11DB7   equiv. to PHP hash crc32
        MPEG2    0x04C11DB7
        JAMCRC   0x04C11DB7
        POSIX    0x04C11DB7
        CRC32C   0x1EDC6F41   Castagnoli
        CRC32D   0xA833982B
        CRC32K   0x741B8CD7   Koopman
        CRC32Q   0x814141AB
        XFER     0x000000AF

Alternatively the individual CRC parameters can be specified on
the command line; `crc32 -h` prints a short help text listing all
available options.


## License

Crc32 is distributed under the Modified ("3-clause") BSD License. See
`LICENSE` file for more information.

----------------------------------------------------------------------

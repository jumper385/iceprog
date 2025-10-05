/*
 *  iceprog -- simple programming tool for FTDI-based Lattice iCE programmers
 *
 *  Copyright (C) 2015  Claire Xenia Wolf <claire@clairexen.net>
 *  Copyright (C) 2018  Piotr Esden-Tempski <piotr@esden.net>
 *
 *  Permission to use, copy, modify, and/or distribute this software for any
 *  purpose with or without fee is hereby granted, provided that the above
 *  copyright notice and this permission notice appear in all copies.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 *  Relevant Documents:
 *  -------------------
 *  http://www.latticesemi.com/~/media/Documents/UserManuals/EI/icestickusermanual.pdf
 *  http://www.micron.com/~/media/documents/products/data-sheet/nor-flash/serial-nor/n25q/n25q_32mb_3v_65nm.pdf
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <io.h> /* _setmode() */
#include <fcntl.h> /* _O_BINARY */
#endif

#include "iceprog_fn.h"

static bool verbose = false;

int main(int argc, char **argv)
{
	/* used for error reporting */
	const char *my_name = argv[0];
	for (size_t i = 0; argv[0][i]; i++)
		if (argv[0][i] == '/')
			my_name = argv[0] + i + 1;

	int read_size = 256 * 1024;
	int erase_block_size = 64;
	int erase_size = 0;
	int rw_offset = 0;

	bool read_mode = false;
	bool check_mode = false;
	bool erase_mode = false;
	bool bulk_erase = false;
	bool dont_erase = false;
	bool prog_sram = false;
	int  test_mode = 0;
	bool slow_clock = false;
	bool disable_protect = false;
	bool disable_verify = false;
	bool disable_powerdown = false;
	const char *filename = NULL;
	const char *devstr = NULL;
	int ifnum = 0;

#ifdef _WIN32
	_setmode(_fileno(stdin), _O_BINARY);
	_setmode(_fileno(stdout), _O_BINARY);
#endif

	static struct option long_options[] = {
		{"help", no_argument, NULL, -2},
		{NULL, 0, NULL, 0}
	};

	/* Decode command line parameters */
	int opt;
	char *endptr;
	while ((opt = getopt_long(argc, argv, "d:i:I:rR:e:o:cbnStQvspXk", long_options, NULL)) != -1) {
		switch (opt) {
		case 'd': /* device string */
			devstr = optarg;
			break;
		case 'i': /* block erase size */
			if (!strcmp(optarg, "4"))
				erase_block_size = 4;
			else if (!strcmp(optarg, "32"))
				erase_block_size = 32;
			else if (!strcmp(optarg, "64"))
				erase_block_size = 64;
			else {
				fprintf(stderr, "%s: `%s' is not a valid erase block size (must be `4', `32' or `64')\n", my_name, optarg);
				return EXIT_FAILURE;
			}
			break;
		case 'I': /* FTDI Chip interface select */
			if (!strcmp(optarg, "A"))
				ifnum = 0;
			else if (!strcmp(optarg, "B"))
				ifnum = 1;
			else if (!strcmp(optarg, "C"))
				ifnum = 2;
			else if (!strcmp(optarg, "D"))
				ifnum = 3;
			else {
				fprintf(stderr, "%s: `%s' is not a valid interface (must be `A', `B', `C', or `D')\n", my_name, optarg);
				return EXIT_FAILURE;
			}
			break;
		case 'r': /* Read 256 bytes to file */
			read_mode = true;
			break;
		case 'R': /* Read n bytes to file */
			read_mode = true;
			read_size = strtol(optarg, &endptr, 0);
			if (*endptr == '\0')
				/* ok */;
			else if (!strcmp(endptr, "k"))
				read_size *= 1024;
			else if (!strcmp(endptr, "M"))
				read_size *= 1024 * 1024;
			else {
				fprintf(stderr, "%s: `%s' is not a valid size\n", my_name, optarg);
				return EXIT_FAILURE;
			}
			break;
		case 'e': /* Erase blocks as if we were writing n bytes */
			erase_mode = true;
			erase_size = strtol(optarg, &endptr, 0);
			if (*endptr == '\0')
				/* ok */;
			else if (!strcmp(endptr, "k"))
				erase_size *= 1024;
			else if (!strcmp(endptr, "M"))
				erase_size *= 1024 * 1024;
			else {
				fprintf(stderr, "%s: `%s' is not a valid size\n", my_name, optarg);
				return EXIT_FAILURE;
			}
			break;
		case 'o': /* set address offset */
			rw_offset = strtol(optarg, &endptr, 0);
			if (*endptr == '\0')
				/* ok */;
			else if (!strcmp(endptr, "k"))
				rw_offset *= 1024;
			else if (!strcmp(endptr, "M"))
				rw_offset *= 1024 * 1024;
			else {
				fprintf(stderr, "%s: `%s' is not a valid offset\n", my_name, optarg);
				return EXIT_FAILURE;
			}
			break;
		case 'c': /* do not write just check */
			check_mode = true;
			break;
		case 'b': /* bulk erase before writing */
			bulk_erase = true;
			break;
		case 'n': /* do not erase before writing */
			dont_erase = true;
			break;
		case 'S': /* write to sram directly */
			prog_sram = true;
			break;
		case 't': /* just read flash id */
			test_mode = 1;
			break;
		case 'Q': /* just read flash id */
			test_mode = 2;
			break;
		case 'v': /* provide verbose output */
			verbose = true;
			break;
		case 's': /* use slow SPI clock */
			slow_clock = true;
			break;
		case 'p': /* disable flash protect before erase/write */
			disable_protect = true;
			break;
		case 'X': /* disable verification */
			disable_verify = true;
			break;
		case 'k': /* disable power down command */
			disable_powerdown = true;
			break;
		case -2:
			help(argv[0]);
			return EXIT_SUCCESS;
		default:
			/* error message has already been printed */
			fprintf(stderr, "Try `%s --help' for more information.\n", argv[0]);
			return EXIT_FAILURE;
		}
	}

	/* Make sure that the combination of provided parameters makes sense */

	if (read_mode + erase_mode + check_mode + prog_sram + !!test_mode > 1) {
		fprintf(stderr, "%s: options `-r'/`-R', `-e`, `-c', `-S', and `-t' are mutually exclusive\n", my_name);
		return EXIT_FAILURE;
	}

	if (bulk_erase && dont_erase) {
		fprintf(stderr, "%s: options `-b' and `-n' are mutually exclusive\n", my_name);
		return EXIT_FAILURE;
	}

	if (disable_protect && (read_mode || check_mode || prog_sram || test_mode)) {
		fprintf(stderr, "%s: option `-p' only valid in programming mode\n", my_name);
		return EXIT_FAILURE;
	}

	if (bulk_erase && (read_mode || check_mode || prog_sram || test_mode)) {
		fprintf(stderr, "%s: option `-b' only valid in programming mode\n", my_name);
		return EXIT_FAILURE;
	}

	if (dont_erase && (read_mode || check_mode || prog_sram || test_mode)) {
		fprintf(stderr, "%s: option `-n' only valid in programming mode\n", my_name);
		return EXIT_FAILURE;
	}

	if (rw_offset != 0 && prog_sram) {
		fprintf(stderr, "%s: option `-o' not supported in SRAM mode\n", my_name);
		return EXIT_FAILURE;
	}

	if (rw_offset != 0 && test_mode) {
		fprintf(stderr, "%s: option `-o' not supported in test mode\n", my_name);
		return EXIT_FAILURE;
	}

	if (optind + 1 == argc) {
		if (test_mode) {
			fprintf(stderr, "%s: test mode doesn't take a file name\n", my_name);
			fprintf(stderr, "Try `%s --help' for more information.\n", argv[0]);
			return EXIT_FAILURE;
		}
		filename = argv[optind];
	} else if (optind != argc) {
		fprintf(stderr, "%s: too many arguments\n", my_name);
		fprintf(stderr, "Try `%s --help' for more information.\n", argv[0]);
		return EXIT_FAILURE;
	} else if (bulk_erase || disable_protect) {
		filename = "/dev/null";
	} else if (!test_mode && !erase_mode && !disable_protect) {
		fprintf(stderr, "%s: missing argument\n", my_name);
		fprintf(stderr, "Try `%s --help' for more information.\n", argv[0]);
		return EXIT_FAILURE;
	}

	/* open input/output file in advance
	   so we can fail before initializing the hardware */

	FILE *f = NULL;
	long file_size = -1;

	if (test_mode) {
		/* nop */;
	} else if (erase_mode) {
		file_size = erase_size;
	} else if (read_mode) {
		f = (strcmp(filename, "-") == 0) ? stdout : fopen(filename, "wb");
		if (f == NULL) {
			fprintf(stderr, "%s: can't open '%s' for writing: ", my_name, filename);
			perror(0);
			return EXIT_FAILURE;
		}
	} else {
		f = (strcmp(filename, "-") == 0) ? stdin : fopen(filename, "rb");
		if (f == NULL) {
			fprintf(stderr, "%s: can't open '%s' for reading: ", my_name, filename);
			perror(0);
			return EXIT_FAILURE;
		}

		/* For regular programming, we need to read the file
		   twice--once for programming and once for verifying--and
		   need to know the file size in advance in order to erase
		   the correct amount of memory.

		   See if we can seek on the input file.  Checking for "-"
		   as an argument isn't enough as we might be reading from a
		   named pipe, or contrarily, the standard input may be an
		   ordinary file. */

		if (!prog_sram && !check_mode) {
			if (fseek(f, 0L, SEEK_END) != -1) {
				file_size = ftell(f);
				if (file_size == -1) {
					fprintf(stderr, "%s: %s: ftell: ", my_name, filename);
					perror(0);
					return EXIT_FAILURE;
				}
				if (fseek(f, 0L, SEEK_SET) == -1) {
					fprintf(stderr, "%s: %s: fseek: ", my_name, filename);
					perror(0);
					return EXIT_FAILURE;
				}
			} else {
				FILE *pipe = f;

				f = tmpfile();
				if (f == NULL) {
					fprintf(stderr, "%s: can't open temporary file\n", my_name);
					return EXIT_FAILURE;
				}
				file_size = 0;

				while (true) {
					static unsigned char buffer[4096];
					size_t rc = fread(buffer, 1, 4096, pipe);
					if (rc <= 0)
						break;
					size_t wc = fwrite(buffer, 1, rc, f);
					if (wc != rc) {
						fprintf(stderr, "%s: can't write to temporary file\n", my_name);
						return EXIT_FAILURE;
					}
					file_size += rc;
				}
				fclose(pipe);

				/* now seek to the beginning so we can
				   start reading again */
				fseek(f, 0, SEEK_SET);
			}
		}
	}

	// ---------------------------------------------------------
	// Initialize USB connection to FT2232H
	// ---------------------------------------------------------

	fprintf(stderr, "init..\n");

	mpsse_init(ifnum, devstr, slow_clock);

	fprintf(stderr, "cdone: %s\n", get_cdone() ? "high" : "low");

	flash_release_reset();
	usleep(100000);

	if (test_mode)
	{
		fprintf(stderr, "reset..\n");

		flash_chip_deselect();
		usleep(250000);

		fprintf(stderr, "cdone: %s\n", get_cdone() ? "high" : "low");

		flash_reset();
		flash_power_up();

		if (test_mode == 1)
			flash_read_id();
		else
			flash_enable_quad();

		flash_power_down();

		flash_release_reset();
		usleep(250000);

		fprintf(stderr, "cdone: %s\n", get_cdone() ? "high" : "low");
	}
	else if (prog_sram)
	{
		// ---------------------------------------------------------
		// Reset
		// ---------------------------------------------------------

		fprintf(stderr, "reset..\n");

		sram_reset();
		usleep(100);

		sram_chip_select();
		usleep(2000);

		fprintf(stderr, "cdone: %s\n", get_cdone() ? "high" : "low");


		// ---------------------------------------------------------
		// Program
		// ---------------------------------------------------------

		fprintf(stderr, "programming..\n");
		while (1) {
			static unsigned char buffer[4096];
			int rc = fread(buffer, 1, 4096, f);
			if (rc <= 0)
				break;
			if (verbose)
				fprintf(stderr, "sending %d bytes.\n", rc);
			mpsse_send_spi(buffer, rc);
		}

		mpsse_send_dummy_bytes(6);
		mpsse_send_dummy_bit();

		fprintf(stderr, "cdone: %s\n", get_cdone() ? "high" : "low");
	}
	else /* program flash */
	{
		// ---------------------------------------------------------
		// Reset
		// ---------------------------------------------------------

		fprintf(stderr, "reset..\n");

		flash_chip_deselect();
		usleep(250000);

		fprintf(stderr, "cdone: %s\n", get_cdone() ? "high" : "low");

		flash_reset();
		flash_power_up();

		flash_read_id();


		// ---------------------------------------------------------
		// Program
		// ---------------------------------------------------------

		if (!read_mode && !check_mode)
		{
			if (disable_protect)
			{
				flash_write_enable();
				flash_disable_protection();
			}
			
			if (!dont_erase)
			{
				if (bulk_erase)
				{
					flash_write_enable();
					flash_bulk_erase();
					flash_wait();
				}
				else
				{
					fprintf(stderr, "file size: %ld\n", file_size);

					int block_size = erase_block_size << 10;
					int block_mask = block_size - 1;
					int begin_addr = rw_offset & ~block_mask;
					int end_addr = (rw_offset + file_size + block_mask) & ~block_mask;

					for (int addr = begin_addr; addr < end_addr; addr += block_size) {
						flash_write_enable();
						switch(erase_block_size) {
							case 4:
								flash_4kB_sector_erase(addr);
								break;
							case 32:
								flash_32kB_sector_erase(addr);
								break;
							case 64:
								flash_64kB_sector_erase(addr);
								break;
						}
						if (verbose) {
							fprintf(stderr, "Status after block erase:\n");
							flash_read_status();
						}
						flash_wait();
					}
				}
			}

			if (!erase_mode)
			{
				fprintf(stderr, "programming..\n");

				for (int rc, addr = 0; true; addr += rc) {
					uint8_t buffer[256];
					int page_size = 256 - (rw_offset + addr) % 256;
					rc = fread(buffer, 1, page_size, f);
					if (rc <= 0)
						break;
					fprintf(stderr, "                      \r");
					fprintf(stderr, "addr 0x%06X %3ld%%\r", rw_offset + addr, 100 * addr / file_size);
					flash_write_enable();
					flash_prog(rw_offset + addr, buffer, rc);
					flash_wait();
				}
				fprintf(stderr, "                      \r");
				fprintf(stderr, "done.\n");

				/* seek to the beginning for second pass */
				fseek(f, 0, SEEK_SET);
			}
		}

		// ---------------------------------------------------------
		// Read/Verify
		// ---------------------------------------------------------

		if (read_mode) {
			fprintf(stderr, "reading..\n");
			for (int addr = 0; addr < read_size; addr += 256) {
				uint8_t buffer[256];
				fprintf(stderr, "                      \r");
				fprintf(stderr, "addr 0x%06X %3d%%\r", rw_offset + addr, 100 * addr / read_size);
				flash_read(rw_offset + addr, buffer, 256);
				fwrite(buffer, read_size - addr > 256 ? 256 : read_size - addr, 1, f);
			}
			fprintf(stderr, "                      \r");
			fprintf(stderr, "done.\n");
		} else if (!erase_mode && !disable_verify) {
			fprintf(stderr, "reading..\n");
			for (int addr = 0; true; addr += 256) {
				uint8_t buffer_flash[256], buffer_file[256];
				int rc = fread(buffer_file, 1, 256, f);
				if (rc <= 0)
					break;
				fprintf(stderr, "                      \r");
				fprintf(stderr, "addr 0x%06X %3ld%%\r", rw_offset + addr, 100 * addr / file_size);
				flash_read(rw_offset + addr, buffer_flash, rc);
				if (memcmp(buffer_file, buffer_flash, rc)) {
					fprintf(stderr, "Found difference between flash and file!\n");
					if (!disable_powerdown)
					  flash_power_down();
					flash_release_reset();
					usleep(250000);
					mpsse_error(3);
				}
			}

			fprintf(stderr, "                      \r");
			fprintf(stderr, "VERIFY OK\n");
		}


		// ---------------------------------------------------------
		// Reset
		// ---------------------------------------------------------

		if (!disable_powerdown)
			flash_power_down();

		flash_release_reset();
		usleep(250000);

		fprintf(stderr, "cdone: %s\n", get_cdone() ? "high" : "low");
	}

	if (f != NULL && f != stdin && f != stdout)
		fclose(f);

	// ---------------------------------------------------------
	// Exit
	// ---------------------------------------------------------

	fprintf(stderr, "Bye.\n");
	mpsse_close();
	return 0;
}

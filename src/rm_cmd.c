/*
 * @file        rm_cmd.c
 * @brief       Commandline tool for sending requests
 *              to rsyncme daemon.
 * @author      Piotr Gregor <piotrek.gregor at gmail.com>
 * @version     0.1.2
 * @date        05 Jan 2016 11:15 PM
 * @copyright   LGPLv2.1
 */


#include "rm_defs.h"
#include "rm_tx.h"


#include <fcntl.h>
#include <sys/mman.h>
#include <getopt.h>


#define RM_CMD_F_LEN_MAX 100

void rsyncme_usage(const char *name)
{
	if (!name)
		return;

	fprintf(stderr, "\nusage:\t %s [push <-x file> <[-i ip]|[-y sync_file]><-s raw_bytes_send_threshold>]\n", name);
	fprintf(stderr, "\n      \t [--f(orce)]\n");
	fprintf(stderr, "     \t -x			: local file to synchronize\n");
	fprintf(stderr, "     \t -i			: IPv4 if syncing with remote file\n");
	fprintf(stderr, "     \t -y			: file to sync with (local if [ip]\n"
			"				: was not given, remote otherwise)\n");
	fprintf(stderr, "     \t -l			: block size in bytes, if not given\n"
			"				: default value 512 is used\n");
	fprintf(stderr, "     \t -s			: raw bytes send threshold in bytes, if not given\n"
			"				: default value used is equal size of the block\n");
	fprintf(stderr, "\nExamples:\n");
	fprintf(stderr, "	rsyncme push -x /tmp/foo.tar -i 245.298.125.22 -y /tmp/bar.tar\n"
			"		This will sync local /tmp/foo.tar with remote\n"
			"		file /tmp/bar.tar (remote becomes same as local is).\n");
	fprintf(stderr, "	rsyncme push -x foo.txt -i 245.298.125.22 -y bar.txt -l 2048\n"
			"		This will sync local foo.txt with remote\n"
			"		file bar.txt (remote becomes same as local is) using\n"
			"		increased block size - good for big files.\n");
	fprintf(stderr, "	rsyncme push -x /tmp/foo.tar -i 245.298.125.22\n"
			"		This will sync local /tmp/foo.tar with remote\n"
			"		file with same name (remote becomes same as local is).\n");
	fprintf(stderr, "	rsyncme push -x foo.tar -y bar.tar\n"
			"		This will sync local foo.tar with local bar.tar\n"
			"		(bar.tar becomes same as foo.tar is).\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "For more information please consult documentation.\n");
	fprintf(stderr, "\n");
}

static void
rsyncme_range_error(char argument, unsigned long value)
{
	fprintf(stderr, "argument [%c] too big [%lu]\n", argument, value);
}

int
main( int argc, char *argv[])
{
	int     c, idx, res;
	char	x[RM_CMD_F_LEN_MAX] = {0};
	char	y[RM_CMD_F_LEN_MAX] = {0};
	uint8_t	flags = 0;		/* bits		meaning
                             * 0		cmd (0 RM_MSG_PUSH,
                             *              1 RM_MSG_PULLi)
                             * 1		x
                             * 2		y
                             * 3		ip
                             * 4        force creation of @y if it doesn't exist */
	char                *pCh;
	unsigned long       helper;
    size_t              send_threshold = 0;
	struct sockaddr_in  remote_addr = {0};
	size_t              L = RM_DEFAULT_L;
    rm_push_flags       push_flags = 0;

	if (argc < 4)
	{
		rsyncme_usage(argv[0]);
		exit(EXIT_FAILURE);
	}

	int option_index = 0;
	struct option long_options[] = {
		{ "type", required_argument, 0, 17 },
		{ "push", no_argument, 0, 1 },
		{ "pull", no_argument, 0, 2 },
		{ "force", no_argument, 0, 3 },
		{ 0 }
	};

	// parse optional command line arguments
	while ((c = getopt_long(argc, argv, "x:y:i:l:s", long_options, &option_index)) != -1)
	{
		switch (c)
		{

		case 0:
			// If this option set a flag, turn flag on in ioctl struct
			if (long_options[option_index].flag != 0)
				break;
			fprintf(stderr, "Long option [%s]", long_options[option_index].name);
			if (optarg)
				fprintf(stderr, " with arg [%s]", optarg);
			fprintf(stderr, "\n");
			break; 

		case 1:
			// push request
			flags &= ~RM_BIT_0;
			break;

		case 2:
			// pull request
			flags |= RM_BIT_0;
			break;

		case 3:
			// --force
			flags |= RM_BIT_4;
			break;

		case 'x':
			strncpy(x, optarg, RM_CMD_F_LEN_MAX);
			flags |= RM_BIT_1;
			break;

		case 'y':
			strncpy(y, optarg, RM_CMD_F_LEN_MAX);
			flags |= RM_BIT_2;
			break;
		case 'i':
			if (inet_aton(optarg, &remote_addr.sin_addr) == 0)
			{
				fprintf(stderr, "Invalid IPv4 address\n");
				rsyncme_usage(argv[0]);
				exit(EXIT_FAILURE);
			}
			flags |= RM_BIT_3;
			break;

		case 'l':
			helper = strtoul(optarg, &pCh, 10);
			if (helper > 0x10000 - 1)
			{
				rsyncme_range_error(c, helper);
				exit(EXIT_FAILURE);
			}
			// check
			if ((pCh == optarg) || (*pCh != '\0'))
			{
				fprintf(stderr, "Invalid argument\n");
				fprintf(stderr, "Parameter conversion error, nonconvertible part is: [%s]\n", pCh);
				rsyncme_usage(argv[0]);
				exit(EXIT_FAILURE);
			}
			L = helper;
		case 's':
			helper = strtoul(optarg, &pCh, 10);
			if (helper > 0x10000 - 1)
			{
				rsyncme_range_error(c, helper);
				exit(EXIT_FAILURE);
			}
			// check
			if ((pCh == optarg) || (*pCh != '\0'))
			{
				fprintf(stderr, "Invalid argument\n");
				fprintf(stderr, "Parameter conversion error, nonconvertible part is: [%s]\n", pCh);
				rsyncme_usage(argv[0]);
				exit(EXIT_FAILURE);
			}
			send_threshold = helper;
		case '?':
			// check optopt
			if (optopt == 'x' || optopt == 'y' || optopt == 'i')
				fprintf(stderr, "Option -%c requires an argument.\n", optopt);
			else if (isprint(optopt))
				fprintf(stderr,"Unknown option '-%c'.\n", optopt);
			else {
				fprintf(stderr, "Unknown option character '\\x%x'.\n", optopt);
				fprintf(stderr, "Are there any long options? "
					"Please check that you have typed them correctly.\n");
			}

			rsyncme_usage(argv[0]);
			exit(EXIT_FAILURE);

		default:

			rsyncme_usage(argv[0]);
			exit(EXIT_FAILURE);
		}
	}

	// parse non-optional arguments
	for (idx = optind; idx < argc; idx++)
		fprintf(stderr, "Non-option argument[ %s]\n", argv[idx]);

	// validation
	if ((argc - optind) != 1)
	{
		fprintf(stderr, "\nInvalid number of non-option arguments."
				"\nThere should be 1 non-option arguments: "
						"<push|pull>\n");
		rsyncme_usage(argv[0]);
		exit(EXIT_FAILURE);
	}

	if (strcmp(argv[optind], "push") == 0)
	{
		// RM_MSG_PUSH
		flags &= ~RM_BIT_0;
	}
	else if (strcmp(argv[optind], "pull") == 0)
	{
		// RM_MSG_PULL
		flags |= RM_BIT_0;
	}
	else {
		fprintf(stderr, "\nUnknown command.\nCommand should be one of: "
						"<push|pull>\n");
		rsyncme_usage(argv[0]);
		exit(EXIT_FAILURE);
	}

	// if -x not set report error
	if ((flags & RM_BIT_1) == 0u)
	{
		fprintf(stderr, "\n-x option not set.\n"
			"What is the file you want to sync?\n");
		rsyncme_usage(argv[0]);
		exit(EXIT_FAILURE);
	}

    /* TODO set thresholds */
    if (send_threshold == 0) {
        send_threshold = L;
    }

	// if -i is set
	if ((flags & RM_BIT_3) != 0u)
	{
		/* remote request */
        /* push? */
		if ((flags & RM_BIT_0) == 0u)
		{
			/* remote push request */
			fprintf(stderr, "\nRemote push.\n");
			res = rm_tx_remote_push(x, y, &remote_addr, L);
			if (res < 0)
			{
				// report failure
			}
		} else {
			/* local pull request */
			fprintf(stderr, "\nRemote pull.\n");
		}
	
	} else {
		/* local sync */
        /* push? */
		if ((flags & RM_BIT_0) == 0u)
		{
			/* local push request */
			fprintf(stderr, "\nLocal push.\n");
            /* setup push flags */
            push_flags |= ((flags & RM_BIT_4) >> 4);
			res = rm_tx_local_push(x, y, L, send_threshold, push_flags);
			if (res < 0)
			{
                switch (res)    /* report failure */
                {
                    case -1:
                        fprintf(stderr, "Error. Couldn't open @x file [%s]\n", x);
                        goto fail;
                    case -2:
                        fprintf(stderr, "Error. @y [%s] doesn't exist "
                                "and couldn't create @y\n", y);
                        goto fail;
                    case -3:
                       fprintf(stderr, "Error. Couldn't open @y [%s] and --force "
                               "flag not set, which file should I use?\nPlease "
                               "check that file exists or add --force flag to force "
                               "creation of @y file if it doesn't exist.\n", y);
                       goto fail;
                    case -4:
                      fprintf(stderr, "Error. Couldn't stat @x [%s]\n", x);
                      goto fail;
                    case -5:
                      fprintf(stderr, "Error. Couldn't stat @x [%s]\n", x);
                      goto fail;
                    default:
                        return -1;
                }
			}
		} else {
			/* local pull request */
			fprintf(stderr, "\nLocal pull.\n");
		}
	}

	// OK
	fprintf(stderr, "\nOK.\n");
	return 0;

fail:
    return res;
}

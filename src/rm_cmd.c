/// @file	rm_cmd.c
/// @brief	Commandline tool for sending requests
///		to rsyncme daemon.
/// @author	peterg
/// @version	0.1.1
/// @date	05 Jan 2016 11:15 PM
/// @copyright	LGPLv2.1


#include "rm_defs.h"


#include <fcntl.h>
#include <sys/mman.h>
#include <getopt.h>


#define RM_CMD_F_LEN_MAX 100

void usage(const char *name)
{
	if (!name)
		return;

	fprintf(stderr, "\nusage:\t %s [push <-x file> <[-i ip]|[-y sync_file]>\n", name);
	fprintf(stderr, "     \t -x			: local file to synchronize\n");
	fprintf(stderr, "     \t -i			: IPv4 if syncing with remote file\n");
	fprintf(stderr, "     \t -y			: file to sync with (local if [ip]\n"
			"				: was not given, remote otherwise)\n");
	fprintf(stderr, "\nPossible options:\n");
	fprintf(stderr, "	rsyncme push -x /tmp/dir.tar -i 245.298.125.22 -y /tmp/dir2.tar\n"
			"		This will sync local /tmp/dir.tar with remote\n"
			"		file /tmp/dir2.tar.\n");
	fprintf(stderr, "	rsyncme push -x /tmp/dir.tar -i 245.298.125.22\n"
			"		This will sync local /tmp/dir.tar with remote\n"
			"		file with same name.\n");
	fprintf(stderr, "	rsyncme push -x /tmp/dir1.tar -y /tmp/dir2.tar\n"
			"		This will sync local /tmp/dir1.tar with local\n"
			"		file /tmp/dir2.tar.\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "For more information please consult documentation.\n");
	fprintf(stderr, "\n");
}


int
main( int argc, char *argv[])
{
	int	c, idx;
	char	x[RM_CMD_F_LEN_MAX] = {0};
	char	y[RM_CMD_F_LEN_MAX] = {0};
	uint8_t	flags = 0;		// bits		meaning
					// 0		cmd (0 RM_MSG_PUSH_OUT,
					//		     1 RM_MSG_PULL_OUT)
					// 1		x
					// 2		y
					// 3		ip	
	struct sockaddr_in	server_addr = {0};

	if (argc < 4)
	{
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}

	int option_index = 0;
	struct option long_options[] = {
		{ "type", required_argument, 0, 17 },
		{ "push", no_argument, 0, 1 },
		{ "pull", no_argument, 0, 2 },
		{ "file", required_argument, 0, 0 },
		{ 0 }
	};

	// parse optional command line arguments
	while ((c = getopt_long(argc, argv, "x:y:i:", long_options, &option_index)) != -1)
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

		case 'x':
			strncpy(x, optarg, RM_CMD_F_LEN_MAX);
			flags |= RM_BIT_1;
			break;

		case 'y':
			strncpy(y, optarg, RM_CMD_F_LEN_MAX);
			flags |= RM_BIT_2;
			break;
		case 'i':
			if (inet_aton(optarg, &server_addr.sin_addr) == 0)
			{
				fprintf(stderr, "Invalid IPv4 address\n");
				usage(argv[0]);
				exit(EXIT_FAILURE);
			}
			flags |= RM_BIT_3;
			break;
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

			usage(argv[0]);
			exit(EXIT_FAILURE);

		default:

			usage(argv[0]);
			exit(EXIT_FAILURE);
		}
	}

	// parse non-optional arguments
	for (idx = optind; idx < argc; idx++)
		//fprintf(stderr, "Non-option argument[ %s]\n", argv[idx]);

	// validation
	if ((argc - optind) != 1)
	{
		fprintf(stderr, "\nInvalid number of non-option arguments."
				"\nThere should be 1 non-option arguments: "
						"<push|pull>\n");
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}

	if (strcmp(argv[optind], "push") == 0)
	{
		// RM_MSG_PUSH_OUT
		flags &= ~1;
	}
	else if (strcmp(argv[optind], "pull") == 0)
	{
		// RM_MSG_PULL_OUT;
		flags |= 1;;
	}
	else {
		fprintf(stderr, "\nUnknown command.\nCommand should be one of: "
						"<push|pull>\n");
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}

	// if -x not set report error
	if (((flags >> 1) & 1) == 0)
	{
		fprintf(stderr, "\n-x option not set.\n"
			"What is the file you want to sync?\n");
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}
	// if -i is set
	if (((flags >> 3) & 1) == 1)
	{
		// remote request
		if (((flags >> 0) & 1) == 0)
		{
			// push
			fprintf(stdout, "\nRemote push.\n");
		} else {
			// pull
			fprintf(stdout, "\nRemote pull.\n");
		}
	
	} else {
		// local sync
		if (((flags >> 0) & 1) == 0)
		{
			// push
			fprintf(stdout, "\nLocal push.\n");
		} else {
			// pull
			fprintf(stdout, "\nLocal pull.\n");
		}
	}

	return 0;
}

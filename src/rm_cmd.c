/* @file        rm_cmd.c
 * @brief       Commandline tool for sending requests to rsyncme daemon.
 * @author      Piotr Gregor <piotrgregor@rsyncme.org>
 * @date        05 Jan 2016 11:15 PM
 * @copyright   LGPLv2.1 */


#include "rm_defs.h"
#include "rm_config.h"
#include "rm_rx.h"
#include "rm_tx.h"


#include <fcntl.h>
#include <sys/mman.h>
#include <getopt.h>


static void rsyncme_usage(const char *name)
{
	if (name == NULL) {
		return;
	}
	fprintf(stderr, "\nusage:\t %s push <-x file> <[-i IPv4 [-p port]]|[-y file]> [-z file] [-a threshold] [-t threshold] [-s threshold]\n\n", name);
	fprintf(stderr, "      \t               [-l block_size] [--f(orce)] [--l(eave)] [--help] [--version] [--loglevel level]\n\n");
	fprintf(stderr, "     \t -x           : file to synchronize\n");
	fprintf(stderr, "     \t -i           : IP address or domain name of the receiver of file\n");
	fprintf(stderr, "     \t -p           : receiver's port (defaults to %u)\n", RM_DEFAULT_PORT);
	fprintf(stderr, "     \t -y           : reference file used for syncing (local if [ip]\n"
			"     \t                was not given, remote otherwise) or result file if it doesn't\n"
			"     \t                exist and -z is not set and --force is set\n");
	fprintf(stderr, "     \t -z           : synchronization result file name [optional]\n");
	fprintf(stderr, "     \t -a           : copy all threshold. Send file as raw bytes if it's size\n"
			"     \t                is less or equal this threshold [optional]\n");
	fprintf(stderr, "     \t -t           : copy tail threshold. Send remaining bytes as raw elements\n"
			"     \t                instead of performing rolling match algorithm if the number\n"
			"     \t                of bytes to process is less or equal this threshold [optional]\n");
	fprintf(stderr, "     \t -s           : send threshold. Raw bytes are not sent until\n"
			"     \t                that many bytes have been accumulated. Default value used\n"
			"     \t                is equal to size of the block\n");
	fprintf(stderr, "     \t -l           : block size in bytes, if it is not given then\n"
			"     \t                default value of 512 bytes is used\n");
	fprintf(stderr, "     \t --force      : force creation of result (@y or @z if given) in case the reference file @y doesn't exist\n");
	fprintf(stderr, "     \t --leave      : leave @y after @z has been reconstructed\n");
	fprintf(stderr, "     \t --timeout_s  : seconds part of timeout limit on connect\n");
	fprintf(stderr, "     \t --timeout_us : microseconds part of timeout limit on connect\n");
	fprintf(stderr, "     \t --help       : display this help and exit\n");
	fprintf(stderr, "     \t --version    : output version information and exit\n");
	fprintf(stderr, "     \t --loglevel   : set log verbosity (defalut is NORMAL)\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "     \t If no option is specified, --help is assumed.\n");

	fprintf(stderr, "\nExamples:\n");
	fprintf(stderr, "	rsyncme push -x /tmp/foo.tar -i 245.218.125.22 -y /tmp/bar.tar -z /tmp/result\n"
			"		This will sync local /tmp/foo.tar with remote\n"
			"		file /tmp/bar.tar and save result in remote /tmp/result file (remote /tmp/bar.tar\n"
			"		is saved as /tmp/result, /tmp/bar.tar no longer exists).\n");
	fprintf(stderr, "	rsyncme push -x /tmp/foo.tar -i 245.218.125.22 -y /tmp/bar.tar -z /tmp/result --l\n"
			"		Same as above but remote /tmp/bar.tar is left intact.\n");
	fprintf(stderr, "	rsyncme push -x /tmp/foo.tar -i 245.218.125.22 -y /tmp/bar.tar\n"
			"		This will sync local /tmp/foo.tar with remote\n"
			"		file /tmp/bar.tar (remote becomes same as local is).\n");
	fprintf(stderr, "	rsyncme push -x foo.txt -i 245.218.125.22 -y bar.txt -l 2048\n"
			"		This will sync local foo.txt with remote\n"
			"		file bar.txt (remote becomes same as local is) using\n"
			"		increased block size - good for big files.\n");
	fprintf(stderr, "	rsyncme push -x /tmp/foo.tar -i 245.218.125.22\n"
			"		This will sync local /tmp/foo.tar with remote\n"
			"		file with same name (remote becomes same as local is).\n");
	fprintf(stderr, "	rsyncme push -x foo.tar -y bar.tar\n"
			"		This will sync local foo.tar with local bar.tar\n"
			"		(bar.tar becomes same as foo.tar is).\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "For more information please consult documentation.\n");
	fprintf(stderr, "\n");
}

static void rsyncme_range_error(char argument, unsigned long value)
{
	fprintf(stderr, "\nERR, argument [%c] too big [%lu]\n\n", argument, value);
}

static void help_hint(const char *name)
{
	if (name == NULL)
		return;
	fprintf(stderr, "\nTry %s --help for more information.\n", name);
	return;
}

int main(int argc, char *argv[])
{
	int             c;
	enum rm_error   res;
	char	x[RM_FILE_LEN_MAX] = {0};
	char	y[RM_FILE_LEN_MAX] = {0};
	char	z[RM_FILE_LEN_MAX] = {0};
	char    *xp = NULL, *yp = NULL, *zp = NULL;
	rm_push_flags   push_flags = 0;      /* bits    meaning
										  * 0		cmd (0 RM_MSG_PUSH, 1 RM_MSG_PULL)
										  * 1		x
										  * 2		y
										  * 3		z
										  * 4       force creation of @y if it doesn't exist
										  * 5		ip
										  * 6       delete @y after @z has been reconstructed (or created) */
	char                *pCh, *y_copy;
	unsigned long long  helper;
	struct rm_delta_reconstruct_ctx rec_ctx = {0};
	size_t              copy_all_threshold = 0;
	size_t              copy_tail_threshold = 0;
	size_t              send_threshold = 0;
	uint8_t             send_threshold_set = 0;
	const char          *addr = NULL, *err_str = NULL;
	uint16_t            port = RM_DEFAULT_PORT;
	size_t              L = RM_DEFAULT_L;
	uint16_t            timeout_s = 0, timeout_us = 0;

	char					y_dirname[PATH_MAX];
	char					*y_dname = NULL;
	char					z_dirname[PATH_MAX];
	char					*z_dname = NULL;

	struct rm_tx_options	opt = { .loglevel = RM_LOGLEVEL_NORMAL };


	if (argc < 2) {
		rsyncme_usage(argv[0]);
		exit(EXIT_FAILURE);
	}

	int option_index = 0;
	struct option long_options[] = {
		{ "type", required_argument, 0, 17 },
		{ "push", no_argument, 0, 1 },
		{ "pull", no_argument, 0, 2 },
		{ "force", no_argument, 0, 3 },
		{ "leave", no_argument, 0, 4 },
		{ "help", no_argument, 0, 5 },
		{ "version", no_argument, 0, 6 },
		{ "timeout_s", required_argument, 0, 7 },
		{ "timeout_us", required_argument, 0, 8 },
		{ "loglevel", required_argument, 0, 9 },
		{ 0 }
	};

	while ((c = getopt_long(argc, argv, "x:y:z:i:p:l:a:t:s:", long_options, &option_index)) != -1) { /* parse optional command line arguments */
		switch (c) {

			case 0:
				if (long_options[option_index].flag != 0) { /* If this option set a flag, turn flag on in ioctl struct */
					break;
				}
				fprintf(stderr, "Long option [%s]", long_options[option_index].name);
				if (optarg) {
					fprintf(stderr, " with arg [%s]", optarg);
				}
				fprintf(stderr, "\n");
				break;

			case 1:
				push_flags &= ~RM_BIT_0; /* push request */
				break;

			case 2:
				push_flags |= RM_BIT_0; /* pull request */
				break;

			case 3:
				push_flags |= RM_BIT_4; /* --force */
				break;

			case 4:
				push_flags |= RM_BIT_6; /* --leave */
				break;

			case 5:
				rsyncme_usage(argv[0]); /* --help */
				exit(EXIT_SUCCESS);
				break;

			case 6:
				fprintf(stderr, "\nversion [%s]\n", RM_VERSION); /* --version */
				exit(EXIT_SUCCESS);
				break;

			case 7:
				helper = strtoul(optarg, &pCh, 10);
				if (helper > 0x10000 - 1) {
					rsyncme_range_error(c, helper);
					exit(EXIT_FAILURE);
				}
				if ((pCh == optarg) || (*pCh != '\0')) {    /* check */
					fprintf(stderr, "Invalid argument\n");
					fprintf(stderr, "Parameter conversion error, nonconvertible part is: [%s]\n", pCh);
					help_hint(argv[0]);
					exit(EXIT_FAILURE);
				}
				timeout_s = helper;
				break;

			case 8:
				helper = strtoul(optarg, &pCh, 10);
				if (helper > 0x10000 - 1) {
					rsyncme_range_error(c, helper);
					exit(EXIT_FAILURE);
				}
				if ((pCh == optarg) || (*pCh != '\0')) {    /* check */
					fprintf(stderr, "Invalid argument\n");
					fprintf(stderr, "Parameter conversion error, nonconvertible part is: [%s]\n", pCh);
					help_hint(argv[0]);
					exit(EXIT_FAILURE);
				}
				timeout_us = helper;
				break;

			case 9:																												/* loglevel */
				helper = strtoul(optarg, &pCh, 10);
				if (helper > 0x10 - 1) {
					rsyncme_range_error(c, helper);
					exit(EXIT_FAILURE);
				}
				if ((pCh == optarg) || (*pCh != '\0')) {    /* check */
					fprintf(stderr, "Invalid argument\n");
					fprintf(stderr, "Parameter conversion error, nonconvertible part is: [%s]\n", pCh);
					help_hint(argv[0]);
					exit(EXIT_FAILURE);
				}
				opt.loglevel = helper;
				break;

			case 'x':
				if (strlen(optarg) > RM_FILE_LEN_MAX - 1) {
					fprintf(stderr, "-x name too long\n");
					exit(EXIT_FAILURE);
				}
				strncpy(x, optarg, RM_FILE_LEN_MAX);
				push_flags |= RM_BIT_1;
				xp = &x[0];
				break;

			case 'y':
				if (strlen(optarg) > RM_FILE_LEN_MAX - 1) {
					fprintf(stderr, "-y name too long\n");
					exit(EXIT_FAILURE);
				}
				strncpy(y, optarg, RM_FILE_LEN_MAX);
				push_flags |= RM_BIT_2;
				yp = &y[0];
				break;

			case 'z':
				if (strlen(optarg) > RM_FILE_LEN_MAX - 1) {
					fprintf(stderr, "-z name too long\n");
					exit(EXIT_FAILURE);
				}
				strncpy(z, optarg, RM_FILE_LEN_MAX);
				push_flags |= RM_BIT_3;
				zp = &z[0];
				break;

			case 'i':
				addr = optarg;
				push_flags |= RM_BIT_5; /* remote */
				break;

			case 'p':
				helper = strtoul(optarg, &pCh, 10);
				if (helper > 0x10000 - 1) {
					rsyncme_range_error(c, helper);
					exit(EXIT_FAILURE);
				}
				if ((pCh == optarg) || (*pCh != '\0')) {    /* check */
					fprintf(stderr, "Invalid argument\n");
					fprintf(stderr, "Parameter conversion error, nonconvertible part is: [%s]\n", pCh);
					help_hint(argv[0]);
					exit(EXIT_FAILURE);
				}
				port = helper;
				break;

			case 'l':
				helper = strtoul(optarg, &pCh, 10);
				if (helper > 0x100000000 - 1) {
					rsyncme_range_error(c, helper);
					exit(EXIT_FAILURE);
				}
				if ((pCh == optarg) || (*pCh != '\0')) {    /* check */
					fprintf(stderr, "Invalid argument\n");
					fprintf(stderr, "Parameter conversion error, nonconvertible part is: [%s]\n", pCh);
					help_hint(argv[0]);
					exit(EXIT_FAILURE);
				}
				L = helper;
				break;

			case 'a':
				helper = strtoul(optarg, &pCh, 10);
				if (helper > 0x100000000 - 1) {
					rsyncme_range_error(c, helper);
					exit(EXIT_FAILURE);
				}
				if ((pCh == optarg) || (*pCh != '\0')) {    /* check */
					fprintf(stderr, "Invalid argument\n");
					fprintf(stderr, "Parameter conversion error, nonconvertible part is: [%s]\n", pCh);
					help_hint(argv[0]);
					exit(EXIT_FAILURE);
				}
				copy_all_threshold = helper;
				break;

			case 't':
				helper = strtoul(optarg, &pCh, 10);
				if (helper > 0x100000000 - 1) {
					rsyncme_range_error(c, helper);
					exit(EXIT_FAILURE);
				}
				if ((pCh == optarg) || (*pCh != '\0')) {    /* check */
					fprintf(stderr, "Invalid argument\n");
					fprintf(stderr, "Parameter conversion error, nonconvertible part is: [%s]\n", pCh);
					help_hint(argv[0]);
					exit(EXIT_FAILURE);
				}
				copy_tail_threshold = helper;
				break;

			case 's':
				helper = strtoul(optarg, &pCh, 10);
				if (helper > 0x100000000 - 1) {
					rsyncme_range_error(c, helper);
					exit(EXIT_FAILURE);
				}
				if ((pCh == optarg) || (*pCh != '\0')) {    /* check */
					fprintf(stderr, "Invalid argument\n");
					fprintf(stderr, "Parameter conversion error, nonconvertible part is: [%s]\n", pCh);
					help_hint(argv[0]);
					exit(EXIT_FAILURE);
				}
				send_threshold = helper;
				send_threshold_set = 1;
				break;

			case '?':
				if (optopt == 'x' || optopt == 'y' || optopt == 'i')
					fprintf(stderr, "Option -%c requires an argument.\n", optopt);
				else if (isprint(optopt))
					fprintf(stderr,"Unknown option '-%c'.\n", optopt);
				else {
					fprintf(stderr, "Are there any long options? "
							"Please check that you have typed them correctly.\n");
				}
				rsyncme_usage(argv[0]);
				exit(EXIT_FAILURE);

			default:
				help_hint(argv[0]);
				exit(EXIT_FAILURE);
		}
	}

	/*for (idx = optind; idx < argc; idx++) {  parse non-optional arguments
	  fprintf(stderr, "Non-option argument[ %s]\n", argv[idx]);
	  }*/
	if ((argc - optind) != 1) {
		fprintf(stderr, "\nInvalid number of non-option arguments.\nThere should be 1 non-option arguments: <push|pull>\n");
		help_hint(argv[0]);
		exit(EXIT_FAILURE);
	}
	if (strcmp(argv[optind], "push") == 0) {
		push_flags &= ~RM_BIT_0; /* RM_MSG_PUSH */
	}
	else if (strcmp(argv[optind], "pull") == 0) {
		push_flags |= RM_BIT_0; /* RM_MSG_PULL */
	}
	else {
		fprintf(stderr, "\nUnknown command.\nCommand should be one of: <push|pull>\n");
		rsyncme_usage(argv[0]);
		exit(EXIT_FAILURE);
	}
	if ((push_flags & RM_BIT_0) == 0u) { /* push request? */
		if ((push_flags & RM_BIT_1) == 0u) { /* if -x not set report error */
			fprintf(stderr, "\n-x file not set.\nWhat is the file you want to sync?\n");
			exit(EXIT_FAILURE);
		}
		if ((push_flags & RM_BIT_6) && (z == NULL || (strcmp(y, z) == 0))) { /* if do not delete @y after @z has been synced, but @z name is not given or is same as @y - error */
			fprintf(stderr, "\n--leave option set but @z name not given.\nWhat is the file name you want to use as the result of sync (must be different than @y)?\n");
			help_hint(argv[0]);
			exit(EXIT_FAILURE);
		}
	} else { /* pull */
		if ((push_flags & RM_BIT_2) == 0u) { /* if -y not set report error */
			fprintf(stderr, "\n-y file not set.\nWhat is the file you want to sync?\n");
			exit(EXIT_FAILURE);
		}
		if ((push_flags & RM_BIT_6) && (z == NULL || (strcmp(x, z) == 0))) { /* if do not delete @y after @z has been synced, but @z name is not given or is same as @x - error */
			fprintf(stderr, "\n--leave option set but @z name not given.\nWhat is the file name you want to use as the result of sync (must be different than @x)?\n");
			help_hint(argv[0]);
			exit(EXIT_FAILURE);
		}
	}
	if (L == 0) {
		fprintf(stderr, "\nBlock size can't be 0.\nConsider block size of more than zero.\n");
		exit(EXIT_FAILURE);
	}
	if (send_threshold == 0) {
		if (send_threshold_set == 1) {
			fprintf(stderr, "\nSend threshold can't be 0.\nConsider send threshold of more than zero.\n");
			return RM_ERR_BAD_CALL;
		} else {
			send_threshold = L;
		}
	}
	if ((timeout_s == 0) && (timeout_us == 0)) {
		timeout_s = 10;
	}
	if (push_flags & RM_BIT_5) { /* remote */
		if (yp == NULL) {
			strncpy(y, x, RM_FILE_LEN_MAX);
			yp = &y[0];
		}
	} else { /* local */
		if ((push_flags & RM_BIT_0) == 0u) { /* push request? */
			if (yp == NULL) {
				fprintf(stderr, "\n-y file not set.\n");
				if (push_flags & RM_BIT_4) { /* if --force set */
					fprintf(stderr, "What is the name of result file?\n");
				} else {
					fprintf(stderr, "What is the file you want to sync with?\n");
				}
				help_hint(argv[0]);
				exit(EXIT_FAILURE);
			}
		} else { /* local pull */
			if (xp == NULL) {
				fprintf(stderr, "\n-x file not set.\n");
				if (push_flags & RM_BIT_4) { /* if --force set */
					fprintf(stderr, "What is the name of result file?\n");
				} else {
					fprintf(stderr, "What is the file you want to sync with?\n");
				}
				help_hint(argv[0]);
				exit(EXIT_FAILURE);
			}
		}
	}
	if (L <= sizeof(struct rm_ch_ch)) { /* warn there is no performance benefit in using rsyncme when block size is less than checksums overhead (apart from nonuniform distribution of byte stream transmitted) */
		fprintf(stderr, "\nWarning: block size [%zu] disables possibility of improvement. Consider block bigger than [%zu].\n", L, sizeof(struct rm_ch_ch));
	}

	if ((push_flags & RM_BIT_5) != 0u) { /* remote request if -i is set */
		if ((push_flags & RM_BIT_0) == 0u) { /* remote push request? */
			fprintf(stderr, "\nRemote push.\n");
			res = rm_tx_remote_push(xp, yp, zp, L, copy_all_threshold, copy_tail_threshold, send_threshold, push_flags, &rec_ctx, addr, port, timeout_s, timeout_us, &err_str, &opt);
			if (res != RM_ERR_OK) {
				fprintf(stderr, "\n");
				switch (res) {
					case RM_ERR_OPEN_X:
						fprintf(stderr, "Error. @x [%s] doesn't exist\n", x);
						goto fail;
					case RM_ERR_FSTAT_X:
						fprintf(stderr, "Error. Couldn't stat @x [%s]\n", x);
						goto fail;
					case RM_ERR_FSTAT_Y:
						fprintf(stderr, "Error. Couldn't stat @y [%s]\n", y);
						goto fail;
					case RM_ERR_FSTAT_TMP:
						fprintf(stderr, "Error. Receiver failed (couldn't stat intermediate tmp file)\n");
						goto fail;
					case RM_ERR_X_ZERO_SIZE:
						fprintf(stderr, "Error. @x [%s] size is zero\n", x);
						goto fail;
					case RM_ERR_CREATE_SESSION:
						fprintf(stderr, "Error. Session failed\n");
						goto fail;
					case RM_ERR_GETADDRINFO:
						if (err_str != NULL) {
							fprintf(stderr, "Error. can't get receiver's address, [%s]\n", err_str);
						} else {
							fprintf(stderr, "Error. can't get receiver's address\n");
						}
						goto fail;
					case RM_ERR_CONNECT_TIMEOUT:
						if (err_str != NULL) {
							fprintf(stderr, "Error. Timeout occurred while connecting to [%s] on port [%u] (%s).\nIs the rsyncme receiver running on that host?\n", addr, port, err_str);
						} else {
							fprintf(stderr, "Error. Timeout occurred while connecting to [%s] on port [%u].\nIs the rsyncme receiver running on that host?\n", addr, port);
						}
						goto fail;
					case RM_ERR_CONNECT_REFUSED:
						if (err_str != NULL) {
							fprintf(stderr, "Error. Connection refused while connecting to [%s] on port [%u] (%s).\nIs the rsyncme receiver running on that host?\n", addr, port, err_str);
						} else {
							fprintf(stderr, "Error. Connection refused while connecting to [%s] on port [%u].\nIs the rsyncme receiver running on that host?\n", addr, port);
						}
						goto fail;
					case RM_ERR_CONNECT_HOSTUNREACH:
						if (err_str != NULL) {
							fprintf(stderr, "Error. Host unreachable while connecting to [%s] on port [%u] (%s).\nIs the rsyncme receiver running on that host?\n", addr, port, err_str);
						} else {
							fprintf(stderr, "Error. Host unreachable while connecting to [%s] on port [%u].\nIs the rsyncme receiver running on that host?\n", addr, port);
						}
						goto fail;
					case RM_ERR_CONNECT_GEN_ERR:
						if (err_str != NULL) {
							fprintf(stderr, "Error. Connection failed while connecting to [%s] on port [%u] (%s).\nIs the rsyncme receiver running on that host?\n", addr, port, err_str);
						} else {
							fprintf(stderr, "Error. Connection failed while connecting to [%s] on port [%u].\nIs the rsyncme receiver running on that host?\n", addr, port);
						}
						goto fail;
					case RM_ERR_AUTH:
						fprintf(stderr, "Error. Authentication failure. Is this IP allowed by receiver running on [%s]?\nSkip --auth flag when starting the receiver to disable authentication.\n", addr);
						goto fail;
					case RM_ERR_CHDIR_Y:
						strncpy(y_dirname, y, PATH_MAX);
						y_dname = dirname(y_dirname);
						fprintf(stderr, "Error. receiver running on [%s] cannot change working directory to @y's dir [%s]\n", addr, y_dname);
						y_dname = NULL;
						goto fail;
					case RM_ERR_CHDIR_Z:
						strncpy(z_dirname, z, PATH_MAX);
						z_dname = dirname(z_dirname);
						fprintf(stderr, "Error. Receiver running on [%s] cannot change working directory to @z's dir [%s]\n", addr, z_dname);
						z_dname = NULL;
						goto fail;
					case RM_ERR_CH_CH_RX_THREAD_LAUNCH:
						fprintf(stderr, "Error. Checksums rx thread launch failed\n");
						goto fail;
					case RM_ERR_DELTA_TX_THREAD_LAUNCH:
						fprintf(stderr, "Error. Delta tx thread launch failed\n");
						goto fail;
					case RM_ERR_DELTA_RX_THREAD_LAUNCH:
						fprintf(stderr, "Error. Delta rx thread launch failed\n");
						goto fail;
					case RM_ERR_CH_CH_RX_THREAD:
						fprintf(stderr, "Error. Checksums rx thread failed\n");
						goto fail;
					case RM_ERR_DELTA_TX_THREAD:
						fprintf(stderr, "Error. Delta tx thread failed\n");
						goto fail;
					case RM_ERR_DELTA_RX_THREAD:
						fprintf(stderr, "Error. Delta rx thread failed\n");
						goto fail;
					case RM_ERR_MEM:
						fprintf(stderr, "Error. Not enough memory\n");
						exit(EXIT_FAILURE);
					case RM_ERR_Y_Z_SYNC:
						fprintf(stderr, "Error, request can't be handled by receiver");
						fprintf(stderr, " (do not delete @y after @z has been synced, but @z name is not given or is same as @y)\n");
						goto fail;
					case RM_ERR_Y_NULL:
						fprintf(stderr, "Error, request can't be handled by receiver");
						fprintf(stderr, " (@y name empty)\n");
						goto fail;
					case RM_ERR_OPEN_Z:
						fprintf(stderr, "Error, request can't be handled by receiver");
						fprintf(stderr, " (can't open @z file)\n");
						goto fail;
					case RM_ERR_OPEN_Y:
						fprintf(stderr, "Error, request can't be handled by receiver");
						fprintf(stderr, " (can't open @y file)\nDoes reference file [%s] exist on the receiver [%s]?\nPlease set --force (--f) option if you want to force the creation of result file in case the reference file doesn't exist.\n", y, addr);
						goto fail;
					case RM_ERR_OPEN_TMP:
						fprintf(stderr, "Error, request can't be handled by receiver");
						fprintf(stderr, " (can't open temporary file)\n");
						goto fail;
					case RM_ERR_Y_ZERO_SIZE:
						fprintf(stderr, "Error. @y [%s] size is zero\n", x);
						goto fail;
					case RM_ERR_Z_ZERO_SIZE:
						fprintf(stderr, "Error. @z [%s] size is zero\n", x);
						goto fail;
					case RM_ERR_BAD_CALL:
						fprintf(stderr, "Error. Receiver considers arguments as misconfigured conception\n");
						goto fail;
					case RM_ERR_BLOCK_SIZE:
						fprintf(stderr, "Error. Block size can't be zero\n");
						goto fail;
					case RM_ERR_RENAME_TMP_Y:
						fprintf(stderr, "Error. Receiver can't rename result file to @y [%s]\n", y);
						goto fail;
					case RM_ERR_RENAME_TMP_Z:
						fprintf(stderr, "Error. Receiver can't rename result file to @z [%s]\n", z);
						goto fail;
					default:
						fprintf(stderr, "\nError.\n");
						return -1;
				}
			}
		} else { /* remote pull request */
			fprintf(stderr, "\nRemote pull.\n");
		}

	} else { /* local sync */
		if ((res = rm_util_chdir_umask_openlog(NULL, 1, NULL, 0)) != RM_ERR_OK) { /* just set umask to -rw-r--r-- */
			fprintf(stderr, "Error. Can't set umask\n");
			goto fail;
		}
		if ((push_flags & RM_BIT_0) == 0u) { /* local push? */
			fprintf(stderr, "\nLocal push.\n");
			res = rm_tx_local_push(xp, yp, zp, L, copy_all_threshold, copy_tail_threshold, send_threshold, push_flags, &rec_ctx, &opt);
			if (res != RM_ERR_OK) {
				fprintf(stderr, "\n");
				switch (res) {
					case RM_ERR_OPEN_X:
						fprintf(stderr, "Error. @x [%s] doesn't exist\n", x);
						goto fail;
					case RM_ERR_OPEN_Y:
						fprintf(stderr, "Error. @y [%s] doesn't exist and --force flag is not set. What file should I use?\nPlease "
								"check that @y file exists or add --force flag to force creation of target file if it doesn't exist.\n", y);
						goto fail;
					case RM_ERR_OPEN_Z:
						fprintf(stderr, "Error. @z [%s] can't be opened\n", (z != NULL ? z : y));
						goto fail;
					case RM_ERR_COPY_BUFFERED:
						fprintf(stderr, "Error. Copy buffered failed\n");
						goto fail;
					case RM_ERR_FSTAT_X:
						fprintf(stderr, "Error. Couldn't stat @x [%s]\n", x);
						goto fail;
					case RM_ERR_FSTAT_Y:
						fprintf(stderr, "Error. Couldn't stat @y [%s]\n", y);
						goto fail;
					case RM_ERR_FSTAT_Z:
						fprintf(stderr, "Error. Couldn't stat @z [%s]\n", z);
						goto fail;
					case RM_ERR_OPEN_TMP:
						fprintf(stderr, "Error. Temporary @z can't be opened\n");
						goto fail;
					case RM_ERR_CREATE_SESSION:
						fprintf(stderr, "Error. Session failed\n");
						goto fail;
					case RM_ERR_DELTA_TX_THREAD_LAUNCH:
						fprintf(stderr, "Error. Delta tx thread launch failed\n");
						goto fail;
					case RM_ERR_DELTA_RX_THREAD_LAUNCH:
						fprintf(stderr, "Error. Delta rx thread launch failed\n");
						goto fail;
					case RM_ERR_DELTA_TX_THREAD:
						fprintf(stderr, "Error. Delta tx thread failed\n");
						goto fail;
					case RM_ERR_DELTA_RX_THREAD:
						fprintf(stderr, "Error. Delta rx thread failed\n");
						goto fail;
					case RM_ERR_FILE_SIZE:
						fprintf(stderr, "Error. Bad size of result file\n");
						goto fail;
					case RM_ERR_FILE_SIZE_REC_MISMATCH:
						fprintf(stderr, "Error. Bad size of result file (reconstruction error)\n");
						goto fail;
					case RM_ERR_UNLINK_Y:
						fprintf(stderr, "Error. Cannot unlink @y\n");
						goto fail;
					case RM_ERR_RENAME_TMP_Y:
						fprintf(stderr, "Error. Cannot rename temporary file to @y\n");
						goto fail;
					case RM_ERR_RENAME_TMP_Z:
						fprintf(stderr, "Error. Cannot rename temporary file to @z\n");
						goto fail;
					case RM_ERR_MEM:
						fprintf(stderr, "Error. Not enough memory\n");
						exit(EXIT_FAILURE);
					case RM_ERR_CHDIR:
						y_copy = strdup(y);
						if (y_copy == NULL) {
							fprintf(stderr, "Error. Not enough memory\n");
							exit(EXIT_FAILURE);
						}
						fprintf(stderr, "Error. Cannot change directory to [%s]\n", dirname(y_copy));
						free(y_copy);
						goto fail;
					case RM_ERR_BAD_CALL:
					default:
						fprintf(stderr, "\nInternal error.\n");
						return -1;
				}
			}
		} else { /* local pull request */
			fprintf(stderr, "\nLocal pull.\n");
			res = rm_tx_local_push(yp, xp, zp, L, copy_all_threshold, copy_tail_threshold, send_threshold, push_flags, &rec_ctx, &opt);
			if (res != RM_ERR_OK) {
				fprintf(stderr, "\n");
				switch (res) {
					case RM_ERR_OPEN_X:
						fprintf(stderr, "Error. @y [%s] doesn't exist\n", y);
						goto fail;
					case RM_ERR_OPEN_Y:
						fprintf(stderr, "Error. @x [%s] doesn't exist and --force flag is not set. What file should I use?\nPlease "
								"check that @x file exists or add --force flag to force creation of target file if it doesn't exist.\n", x);
						goto fail;
					case RM_ERR_OPEN_Z:
						fprintf(stderr, "Error. @z [%s] can't be opened\n", (z != NULL ? z : x));
						goto fail;
					case RM_ERR_COPY_BUFFERED:
						fprintf(stderr, "Error. Copy buffered failed\n");
						goto fail;
					case RM_ERR_FSTAT_X:
						fprintf(stderr, "Error. Couldn't stat @y [%s]\n", y);
						goto fail;
					case RM_ERR_FSTAT_Y:
						fprintf(stderr, "Error. Couldn't stat @x [%s]\n", x);
						goto fail;
					case RM_ERR_FSTAT_Z:
						fprintf(stderr, "Error. Couldn't stat @z [%s]\n", z);
						goto fail;
					case RM_ERR_OPEN_TMP:
						fprintf(stderr, "Error. Temporary @z can't be opened\n");
						goto fail;
					case RM_ERR_CREATE_SESSION:
						fprintf(stderr, "Error. Session failed\n");
						goto fail;
					case RM_ERR_DELTA_TX_THREAD_LAUNCH:
						fprintf(stderr, "Error. Delta tx thread launch failed\n");
						goto fail;
					case RM_ERR_DELTA_RX_THREAD_LAUNCH:
						fprintf(stderr, "Error. Delta rx thread launch failed\n");
						goto fail;
					case RM_ERR_DELTA_TX_THREAD:
						fprintf(stderr, "Error. Delta tx thread failed\n");
						goto fail;
					case RM_ERR_DELTA_RX_THREAD:
						fprintf(stderr, "Error. Delta rx thread failed\n");
						goto fail;
					case RM_ERR_FILE_SIZE:
						fprintf(stderr, "Error. Bad size of result file\n");
						goto fail;
					case RM_ERR_FILE_SIZE_REC_MISMATCH:
						fprintf(stderr, "Error. Bad size of result file (reconstruction error)\n");
						goto fail;
					case RM_ERR_UNLINK_Y:
						fprintf(stderr, "Error. Cannot unlink @x\n");
						goto fail;
					case RM_ERR_RENAME_TMP_Y:
						fprintf(stderr, "Error. Cannot rename temporary file to @x\n");
						goto fail;
					case RM_ERR_RENAME_TMP_Z:
						fprintf(stderr, "Error. Cannot rename temporary file to @z\n");
						goto fail;
					case RM_ERR_MEM:
						fprintf(stderr, "Error. Not enough memory\n");
						exit(EXIT_FAILURE);
					case RM_ERR_CHDIR:
						y_copy = strdup(x);
						if (y_copy == NULL) {
							fprintf(stderr, "Error. Not enough memory\n");
							exit(EXIT_FAILURE);
						}
						fprintf(stderr, "Error. Cannot change directory to [%s]\n", dirname(y_copy));
						free(y_copy);
						goto fail;
					case RM_ERR_BAD_CALL:
					default:
						fprintf(stderr, "\nInternal error.\n");
						return -1;
				}
			}
		}
	}

	rm_rx_print_stats(rec_ctx);
	fprintf(stderr, "\nOK.\n");
	return RM_ERR_OK;

fail:
	fprintf(stderr, "\n");
	return res;
}

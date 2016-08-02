/* @file        rm_cmd.c
 * @brief       Commandline tool for sending requests to rsyncme daemon.
 * @author      Piotr Gregor <piotrek.gregor at gmail.com>
 * @version     0.1.2
 * @date        05 Jan 2016 11:15 PM
 * @copyright   LGPLv2.1 */


#include "rm_defs.h"
#include "rm_tx.h"


#include <fcntl.h>
#include <sys/mman.h>
#include <getopt.h>


#define RM_CMD_F_LEN_MAX 100

static void
rsyncme_usage(const char *name) {
    if (name == NULL) {
        return;
    }
    fprintf(stderr, "\nusage:\t %s push <-x file> <[-i IPv4]|[-y file]> [-z file] [-a threshold] [-t threshold] [-s threshold]\n\n", name);
    fprintf(stderr, "      \t             [-l block_size] [--f(orce)] [--l(eave)] [--help] [--version]\n\n");
    fprintf(stderr, "     \t -x         : file to synchronize\n");
    fprintf(stderr, "     \t -i         : IPv4 if syncing with remote file\n");
    fprintf(stderr, "     \t -y         : reference file used for syncing (local if [ip]\n"
                    "     \t            : was not given, remote otherwise)\n");
    fprintf(stderr, "     \t -z         : synchronization result file name [optional]\n");
    fprintf(stderr, "     \t -a         : copy all threshold. Send file as raw bytes if it's size\n"
                    "     \t              is less or equal this threshold [optional]\n");
    fprintf(stderr, "     \t -t         : copy tail threshold. Send remaining bytes as raw elements\n"
                    "     \t              instead of performing rolling match algorithm if the number\n"
                    "     \t              of bytes to process is less or equal this threshold [optional]\n");
    fprintf(stderr, "     \t -s         : send threshold. Raw bytes are not sent until\n"
                    "     \t              that many bytes have been accumulated. Default value used\n"
                    "     \t              is equal to size of the block\n");
    fprintf(stderr, "     \t -l         : block size in bytes, if it is not given then\n"
                    "     \t              default value of 512 bytes is used\n");
    fprintf(stderr, "     \t --force    : force creation of @y if it doesn't exist\n");
    fprintf(stderr, "     \t --leave    : leave @y after @z has been reconstructed\n");
    fprintf(stderr, "     \t --help     : display this help and exit\n");
    fprintf(stderr, "     \t --version  : output version information and exit\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "     \t If no option is specified, --help is assumed.\n");

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
rsyncme_range_error(char argument, unsigned long value) {
    fprintf(stderr, "\nERR, argument [%c] too big [%lu]\n\n", argument, value);
}

static void
print_stats(struct rm_delta_reconstruct_ctx rec_ctx) {
    enum rm_reconstruct_method method;
    double                  real_time, cpu_time;
    size_t                  bytes;

    bytes = rec_ctx.rec_by_raw + rec_ctx.rec_by_ref;
    real_time = rec_ctx.time_real.tv_sec + (double) rec_ctx.time_real.tv_nsec / RM_NANOSEC_PER_SEC;
    cpu_time = rec_ctx.time_cpu;

    method = rec_ctx.method;
    switch (method) {

        case RM_RECONSTRUCT_METHOD_COPY_BUFFERED:
            fprintf(stderr, "\nmethod      : COPY_BUFFERED");
            fprintf(stderr, "\nbytes       : [%zu]", bytes);
            break;

        case RM_RECONSTRUCT_METHOD_DELTA_RECONSTRUCTION:
            fprintf(stderr, "\nmethod      : DELTA_RECONSTRUCTION (block [%zu])", rec_ctx.L);
            fprintf(stderr, "\nbytes       : [%zu] (by raw [%zu], by refs [%zu])", bytes, rec_ctx.rec_by_raw, rec_ctx.rec_by_ref);
            if (rec_ctx.rec_by_zero_diff != 0) {
                fprintf(stderr, " (zero difference)");
            }
            fprintf(stderr, "\ndeltas      : [%zu] (raw [%zu], refs [%zu])", rec_ctx.delta_raw_n + rec_ctx.delta_ref_n, rec_ctx.delta_raw_n, rec_ctx.delta_ref_n);
            fprintf(stderr, "\ncollisions  : 1st level [%zu], 2nd level [%zu])", rec_ctx.collisions_1st_level, rec_ctx.collisions_2nd_level);
            break;

        default:
            fprintf(stderr, "\nERR, unknown delta reconstruction method\n");
            exit(EXIT_FAILURE);
            break;
    }
    fprintf(stderr, "\ntime        : real [%lf]s, cpu [%lf]s", real_time, cpu_time);
    fprintf(stderr, "\nbandwidth   : [%lf]MB/s\n", ((double) bytes / 1000000) / real_time);
}

int
main( int argc, char *argv[]) {
    int             c;
    enum rm_error   res;
    char	x[RM_CMD_F_LEN_MAX] = {0};
    char	y[RM_CMD_F_LEN_MAX] = {0};
    char	z[RM_CMD_F_LEN_MAX] = {0};
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
    unsigned long       helper;
    struct rm_delta_reconstruct_ctx rec_ctx = {0};
    size_t              copy_all_threshold = 0;
    size_t              copy_tail_threshold = 0;
    size_t              send_threshold = 0;
    struct sockaddr_in  remote_addr = {0};
    size_t              L = RM_DEFAULT_L;

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
        { 0 }
    };

    while ((c = getopt_long(argc, argv, "x:y:z:i:l:a:t:s:", long_options, &option_index)) != -1) { /* parse optional command line arguments */
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

            case 'x':
                if (strlen(optarg) > RM_CMD_F_LEN_MAX - 1) {
                    fprintf(stderr, "-x name too long\n");
                    rsyncme_usage(argv[0]);
                    exit(EXIT_FAILURE);
                }
                strncpy(x, optarg, RM_CMD_F_LEN_MAX);
                push_flags |= RM_BIT_1;
                xp = &x[0];
                break;

            case 'y':
                if (strlen(optarg) > RM_CMD_F_LEN_MAX - 1) {
                    fprintf(stderr, "-y name too long\n");
                    rsyncme_usage(argv[0]);
                    exit(EXIT_FAILURE);
                }
                strncpy(y, optarg, RM_CMD_F_LEN_MAX);
                push_flags |= RM_BIT_2;
                yp = &y[0];
                break;

            case 'z':
                if (strlen(optarg) > RM_CMD_F_LEN_MAX - 1) {
                    fprintf(stderr, "-z name too long\n");
                    rsyncme_usage(argv[0]);
                    exit(EXIT_FAILURE);
                }
                strncpy(z, optarg, RM_CMD_F_LEN_MAX);
                push_flags |= RM_BIT_3;
                zp = &z[0];
                break;

            case 'i':
                if (inet_pton(AF_INET, optarg, &remote_addr.sin_addr) == 0) {
                    fprintf(stderr, "Invalid IPv4 address\n");
                    rsyncme_usage(argv[0]);
                    exit(EXIT_FAILURE);
                }
                push_flags |= RM_BIT_5; /* remote */
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
                    rsyncme_usage(argv[0]);
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
                    rsyncme_usage(argv[0]);
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
                    rsyncme_usage(argv[0]);
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
                    rsyncme_usage(argv[0]);
                    exit(EXIT_FAILURE);
                }
                send_threshold = helper;
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
                rsyncme_usage(argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    /*for (idx = optind; idx < argc; idx++) {  parse non-optional arguments
      fprintf(stderr, "Non-option argument[ %s]\n", argv[idx]);
      }*/
    if ((argc - optind) != 1) {
        fprintf(stderr, "\nInvalid number of non-option arguments.\nThere should be 1 non-option arguments: <push|pull>\n");
        rsyncme_usage(argv[0]);
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
    if ((push_flags & RM_BIT_1) == 0u) { /* if -x not set report error */
        fprintf(stderr, "\n-x file not set.\nWhat is the file you want to sync?\n");
        rsyncme_usage(argv[0]);
        exit(EXIT_FAILURE);
    }
    if ((push_flags & RM_BIT_6) && (z == NULL || (strcmp(y, z) == 0))) { /* if do not delete @y after @z has been synced, but @z name is not given or is same as @y - error */
        fprintf(stderr, "\n--leave option set but @z name not given.\nWhat is the file name you want to use as the result of sync (must be different than @y)?\n");
        rsyncme_usage(argv[0]);
        exit(EXIT_FAILURE);
    }
    if (push_flags & RM_BIT_5) { /* remote */
        if (y == NULL) {
            strncpy(y, x, RM_CMD_F_LEN_MAX);
        }
    } else { /* local */
        if (y == NULL) {
            fprintf(stderr, "\n-y file not set.\nWhat is the file you want to sync with?\n");
            rsyncme_usage(argv[0]);
            exit(EXIT_FAILURE);
        }
    }
    if (send_threshold == 0) { /* TODO set thresholds */
        send_threshold = L;
    }
    if (L <= sizeof(struct rm_ch_ch)) { /* warn there is no performance benefit in using rsyncme when block size is less than checksums overhead (apart from nonuniform distribution of byte stream transmitted) */
        fprintf(stderr, "\nWarning: block size [%zu] disables possibility of improvement. Consider morre than [%zu].\n", L, sizeof(struct rm_ch_ch));
    }

    if ((push_flags & RM_BIT_5) != 0u) { /* remote request if -i is set */
        if ((push_flags & RM_BIT_0) == 0u) { /* remote push request? */
            fprintf(stderr, "\nRemote push.\n");
            res = rm_tx_remote_push(xp, yp, &remote_addr, L);
            if (res < 0) {
                /* TODO */
            }
        } else { /* local pull request */
            fprintf(stderr, "\nRemote pull.\n");
        }

    } else { /* local sync */
        if ((push_flags & RM_BIT_0) == 0u) { /* local push? */
            fprintf(stderr, "\nLocal push.\n");
            res = rm_tx_local_push(xp, yp, zp, L, copy_all_threshold, copy_tail_threshold, send_threshold, push_flags, &rec_ctx);
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
        }
    }

    print_stats(rec_ctx);
    fprintf(stderr, "\nOK.\n");
    return 0;

fail:
    fprintf(stderr, "\n");
    return res;
}

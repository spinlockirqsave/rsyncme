/*
 * @file        rm_rx.c
 * @brief       Definitions used by rsync receiver (B).
 * @author      Piotr Gregor <piotrek.gregor at gmail.com>
 * @version     0.1.2
 * @date        2 Jan 2016 11:18 AM
 * @copyright   LGPLv2.1
 */


#include "rm_rx.h"


/*
int
rm_rx_insert_nonoverlapping_ch_ch(FILE *f, const char *fname,
		struct twhlist_head *h, uint32_t L,
		int (*f_tx_ch_ch)(const struct rm_ch_ch *),
        size_t limit, size_t *blocks_n)
{
    int         fd, res;
    struct stat fs;
    uint32_t	file_sz, read_left, read_now, read;
    size_t      entries_n;
    struct rm_ch_ch	*e;
    unsigned char	*buf;

    assert(f != NULL);
    assert(fname != NULL);
    assert(L > 0);

    / get file size /
    fd = fileno(f);
    res = fstat(fd, &fs);
    if (res != 0)
    {
        RM_LOG_PERR("Can't fstat file [%s]", fname);
        return -1;
    }
    file_sz = fs.st_size;

    / read L bytes chunks /
    read_left = file_sz;
    read_now = rm_min(L, read_left);
    buf = malloc(read_now);
    if (buf == NULL)
    {
        RM_LOG_ERR("Malloc failed, L [%u], "
                "read_now [%u]", L, read_now);
        return -2;
    }

    entries_n = 0;
    do
    {
        read = fread(buf, 1, read_now, f);
        if (read != read_now)
        {
            RM_LOG_PERR("Error reading file [%s]", fname);
            free(buf);
            return -3;
        }
        / alloc new table entry /
        e = malloc(sizeof (*e));
        if (e == NULL)	
        {
            RM_LOG_PERR("Can't allocate table "
                    "entry, malloc failed");
            return -4;
        }

        / compute checksums /
        e->f_ch = rm_fast_check_block(buf, read);
        rm_md5(buf, read, e->s_ch.data);

        if (h != NULL)
        {
            / insert into hashtable, hashing fast checksum /
            TWINIT_HLIST_NODE(&e->hlink);
            twhash_add_bits(h, &e->hlink, e->f_ch,
                                RM_NONOVERLAPPING_HASH_BITS);
        }
        entries_n++;

        / tx checksums to remote A ? /
        if (f_tx_ch_ch != NULL)
        {
            res = f_tx_ch_ch(e);
            if (res < 0)
                return -5;
        }
        read_left -= read;
        read_now = rm_min(L, read_left);
    } while (read_now > 0 && entries_n < limit);

    if (blocks_n != NULL)
        *blocks_n = entries_n;
    return	0;
}*/

int
rm_rx_f_tx_ch_ch_ref_1(const struct f_tx_ch_ch_ref_arg_1 arg)
{
    const struct rm_ch_ch_ref       *e;
    const struct rm_session         *s;
    enum rm_session_type            s_type;
    struct rm_session_push_rx       *rm_push_rx;
    struct rm_session_pull_rx       *rm_pull_rx;

    e = arg.e;
    s = arg.s;
    if (e == NULL || s == NULL)
        return -1;
    s_type = s->type;
    if (s_type != RM_PUSH_RX && s_type != RM_PULL_RX)
        return -2;
    switch (s_type)
    {
        case RM_PUSH_RX:
            rm_push_rx = (struct rm_session_push_rx*) s->prvt;
            if (rm_push_rx == NULL)
                return -3;
            if (rm_tcp_tx_ch_ch_ref(rm_push_rx->fd, e) < 0)
                return -4;

            break;
        case RM_PULL_RX:
            rm_pull_rx = (struct rm_session_pull_rx*) s->prvt;
            if (rm_pull_rx == NULL)
                return -5;
            if (rm_tcp_tx_ch_ch_ref(rm_pull_rx->fd, e) < 0)
                return -6;
            break;
        default:
            return -7;
    }

    return 0;
}
int
rm_rx_insert_nonoverlapping_ch_ch_ref(FILE *f, const char *fname,
		struct twhlist_head *h, uint32_t L,
		int (*f_tx_ch_ch_ref)(const struct f_tx_ch_ch_ref_arg_1),
        size_t limit, size_t *blocks_n)
{
    int                 fd, res;
    struct stat         fs;
    uint32_t	        file_sz, read_left, read_now, read;
    size_t              entries_n;
    struct rm_ch_ch_ref_hlink	*e;
    unsigned char	    *buf;
    struct f_tx_ch_ch_ref_arg_1 arg;

    assert(f != NULL);
    assert(fname != NULL);
    assert(L > 0);

    /* get file size */
    fd = fileno(f);
    res = fstat(fd, &fs);
    if (res != 0)
    {
        RM_LOG_PERR("Can't fstat file [%s]", fname);
        return -1;
    }
    file_sz = fs.st_size;

    /* read L bytes chunks */
    read_left = file_sz;
    read_now = rm_min(L, read_left);
    buf = malloc(read_now);
    if (buf == NULL)
    {
        RM_LOG_ERR("Malloc failed, L [%u], "
                "read_now [%u]", L, read_now);
        return -2;
    }

    entries_n = 0;
    do
    {
        read = fread(buf, 1, read_now, f);
        if (read != read_now)
        {
            RM_LOG_PERR("Error reading file [%s]", fname);
            free(buf);
            return -3;
        }
        /* alloc new table entry */
        e = malloc(sizeof (*e));
        if (e == NULL)	
        {
            RM_LOG_PERR("Can't allocate table "
                    "entry, malloc failed");
            return -4;
        }

        /* compute checksums */
        e->data.ch_ch.f_ch = rm_fast_check_block(buf, read);
        rm_md5(buf, read, e->data.ch_ch.s_ch.data);

        /* assign offset */
        e->data.ref = entries_n;

        if (h != NULL)
        {
            /* insert into hashtable, hashing fast checksum */
            TWINIT_HLIST_NODE(&e->hlink);
            twhash_add_bits(h, &e->hlink, e->data.ch_ch.f_ch,
                                RM_NONOVERLAPPING_HASH_BITS);
        }
        entries_n++;

        /* tx checksums to remote A ? */
        if (f_tx_ch_ch_ref != NULL)
        {
            arg.e = &e->data;
            arg.s = NULL;
            res = f_tx_ch_ch_ref(arg);
            if (res < 0)
                return -5;
        }
        read_left -= read;
        read_now = rm_min(L, read_left);
    } while (read_now > 0 && entries_n < limit);

    if (blocks_n != NULL)
        *blocks_n = entries_n;
    return	0;
}

int
rm_rx_insert_nonoverlapping_ch_ch_array(FILE *f, const char *fname,
		struct rm_ch_ch *checksums, uint32_t L,
		int (*f_tx_ch_ch)(const struct rm_ch_ch *),
        size_t limit, size_t *blocks_n)
{
    int         fd, res;
    struct stat fs;
    uint32_t	file_sz, read_left, read_now, read;
    size_t      entries_n;
    struct rm_ch_ch	*e;
    unsigned char	*buf;

    assert(f != NULL);
    assert(fname != NULL);
    assert(L > 0);
    assert(checksums != NULL);

    /* get file size */
    fd = fileno(f);
    res = fstat(fd, &fs);
    if (res != 0)
    {
        RM_LOG_PERR("Can't fstat file [%s]", fname);
        return -1;
    }
    file_sz = fs.st_size;

    /* read L bytes chunks */
    read_left = file_sz;
    read_now = rm_min(L, read_left);
    buf = malloc(read_now);
    if (buf == NULL)
    {
        RM_LOG_ERR("Malloc failed, L [%u], "
                "read_now [%u]", L, read_now);
        return -2;
    }

    entries_n = 0;
    e = &checksums[0];
    do
    {
        read = fread(buf, 1, read_now, f);
        if (read != read_now)
        {
            RM_LOG_PERR("Error reading file [%s]", fname);
            free(buf);
            return -3;
        }

        /* compute checksums */
        e->f_ch = rm_fast_check_block(buf, read);
        rm_md5(buf, read, e->s_ch.data);

        /* tx checksums to remote A ? */
        if (f_tx_ch_ch != NULL)
        {
            res = f_tx_ch_ch(e);
            if (res < 0)
                return -5;
        }
        ++entries_n;
        ++e;

        read_left -= read;
        read_now = rm_min(L, read_left);
    } while (read_now > 0 && entries_n < limit);

    if (blocks_n != NULL)
        *blocks_n = entries_n;
    return 0;
}

int
rm_rx_insert_nonoverlapping_ch_ch_ref_link(FILE *f, const char *fname,
		struct twlist_head *l, uint32_t L,
        size_t limit, size_t *blocks_n)
{
    int                     fd, res;
    struct stat             fs;
    uint32_t	            file_sz, read_left, read_now, read;
    size_t                  entries_n;
    struct rm_ch_ch_ref_link *e;
    unsigned char           *buf;

    assert(f != NULL);
    assert(fname != NULL);
    assert(l != NULL);
    assert(L > 0);

    /* get file size */
    fd = fileno(f);
    res = fstat(fd, &fs);
    if (res != 0)
    {
        RM_LOG_PERR("Can't fstat file [%s]", fname);
        return -1;
    }
    file_sz = fs.st_size;

    /* read L bytes chunks */
    read_left = file_sz;
    read_now = rm_min(L, read_left);
    buf = malloc(read_now);
    if (buf == NULL)
    {
        RM_LOG_ERR("Malloc failed, L [%u], "
                "read_now [%u]", L, read_now);
        return -2;
    }

    entries_n = 0;
    do
    {
        read = fread(buf, 1, read_now, f);
        if (read != read_now)
        {
            RM_LOG_PERR("Error reading file [%s]", fname);
            free(buf);
            return -3;
        }
        /* alloc new table entry */
        e = malloc(sizeof (*e));
        if (e == NULL)	
        {
            RM_LOG_PERR("Can't allocate list "
                    "entry, malloc failed");
            return -4;
        }

        /* compute checksums */
        e->data.ch_ch.f_ch = rm_fast_check_block(buf, read);
        rm_md5(buf, read, e->data.ch_ch.s_ch.data);

        /* insert into list */
        TWINIT_LIST_HEAD(&e->link);
        e->data.ref = entries_n;
        twlist_add_tail(&e->link, l);

        entries_n++;

        read_left -= read;
        read_now = rm_min(L, read_left);
    } while (read_now > 0 && entries_n < limit);

    *blocks_n = entries_n;
    return	0;
}
/*
int
rm_rx_delta_e_reconstruct_f_1(void *arg)
{
	struct rm_rx_delta_e_reconstruct_arg *delta_pack =
			(struct rm_rx_delta_e_reconstruct_arg*) arg;
	assert(delta_pack != NULL);
    (void)delta_pack;
	return NULL;
}
*/

int
rm_rx_process_delta_element(const struct rm_delta_e *delta_e, FILE *f_y, FILE *f,
        struct rm_delta_reconstruct_ctx *ctx)
{
    assert(delta_e != NULL && f_y != NULL && f != NULL && ctx != NULL);
    if (delta_e == NULL || f_y == NULL || f == NULL || ctx == NULL) {
        return -1;
    }
    switch (delta_e->type)
    {
        case RM_DELTA_ELEMENT_REFERENCE:
            /* TODO copy referenced bytes from @f_y to @f */
            ctx->rec_by_ref += delta_e->raw_bytes_n;    /* L == delta_e->raw_bytes_n for REFERNECE delta elements*/
            ++ctx->delta_ref_n;
            break;
        case RM_DELTA_ELEMENT_RAW_BYTES:
            /* TODO copy raw bytes to @f directly */
            ctx->rec_by_raw += delta_e->raw_bytes_n;
            ++ctx->delta_raw_n;
            break;
        case RM_DELTA_ELEMENT_ZERO_DIFF:
            /* TODO copy all bytes from @f_y to @f */
            ctx->rec_by_ref += delta_e->raw_bytes_n; /* delta ZERO_DIFF has raw_bytes_n set to indicate bytes that matched
                                                        (whole file) so we can nevertheless check here at receiver that is correct */
            ++ctx->delta_ref_n;

            ctx->rec_by_zero_diff += delta_e->raw_bytes_n;
            ++ctx->delta_zero_diff_n;
            break;
        case RM_DELTA_ELEMENT_TAIL:
            /* TODO copy referenced bytes from @f_y to @f */
            ctx->rec_by_ref += delta_e->raw_bytes_n; /* delta TAIL has raw_bytes_n set to indicate bytes that matched
                                                        (that tail) so we can nevertheless check here at receiver there is no error */
            ++ctx->delta_ref_n;

            ctx->rec_by_tail += delta_e->raw_bytes_n;
            ++ctx->delta_tail_n;
            break;
        default:
            assert(1 == 0 && "Unknown delta element type!");
            return -2;
    }
    return 0;
}

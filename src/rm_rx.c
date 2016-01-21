/// @file       rm_rx.c
/// @brief      Definitions used by rsync receiver (B).
/// @author     Piotr Gregor piotrek.gregor at gmail.com
/// @version    0.1.1
/// @date       2 Jan 2016 11:18 AM
/// @copyright  LGPLv2.1


#include "rm_rx.h"


long long int
rm_rx_insert_nonoverlapping_ch_ch(FILE *f, char *fname,
		struct twhlist_head *h, uint32_t L)
{
	int		fd;
	struct stat	fs;
	uint32_t	file_sz, read_left, read_now, read;
	long long int	entries_n;
	struct rm_ch_ch	*e;
	unsigned char	*buf;

	assert(f != NULL);
	assert(fname != NULL);
	assert(h != NULL);
	assert(L > 0);

	// get file size
	fd = fileno(f);
	if (fstat(fd, &fs) != 0)
	{
		RM_LOG_PERR("Can't fstat file [%s]", fname);
		return -1;
	}
	file_sz = fs.st_size;

	// read L bytes chanks
	read_left = file_sz;
	read_now = rm_min(L, read_left);
	buf = malloc(read_now);
	if (buf == NULL)	
	{
		RM_LOG_PERR("Malloc failed, L [%u], "
		"read_now [%u]", L, read_now);
		return -2;
	}

	entries_n = 0;
	do
	{
		read = fread(buf, 1, read_now, f);
		if (read != read_now)
		{
			RM_LOG_PERR("Error reading file [%s]",
					fname);
			free(buf);
			return -3;
		}
		// alloc new table entry
		e = malloc(sizeof (*e));
		if (e == NULL)	
		{
			RM_LOG_PERR("Can't allocate table "
				"entry, malloc failed");
			return -4;
		}
		TWINIT_HLIST_NODE(&e->hlink);
		// compute checksums
		e->f_ch = rm_fast_check_block(buf, read);
		rm_md5(buf, read, e->s_ch.data);
		// insert, hashing fast checksum
		twhash_add_bits(h, &e->hlink, e->f_ch, RM_NONOVERLAPPING_HASH_BITS);
		entries_n++;

		read_left -= read;
		read_now = rm_min(L, read_left);
	} while (read_now > 0);
	
	return	entries_n;
}

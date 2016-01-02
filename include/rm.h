/// @file       rm.h
/// @brief      Simple rsync algorithm implementation
///		as described in Tridgell A., "Efficient Algorithms
///		for Sorting and Synchronization", 1999.
/// @details	Common definitions used by sender and receiver.
/// @author     Piotr Gregor piotrek.gregor at gmail.com
/// @version    0.1.1
/// @date       1 Jan 2016 07:50 PM
/// @copyright  LGPL


#ifndef RSYNCME_H
#define RSYNCME_H


/// Implementation is based on Tridgell A.,
/// "Efficient Algorithms for Sorting and
/// Synchronization", 1999.
/// 
/// Notation:
///
/// sender(A)
/// receiver(B)
/// F'_A - changed file F in A
/// F'_B - changed file F in B (result of sync
///		so that F'_B == F'_A)
/// F_A  - previous version of file F in A
/// F_B  - previous version of file F in B
///
/// When sender (A) made a change to current
/// versiion of file F (F_A),
/// it has produced new file F'_A and now
/// it will synchronize this new file with
/// other version F_B (maybe same as F_A,
/// maybe not) stored in receiver (B).
///
/// ------------------------------------
/// A[sender]     ->    B[receiver]
/// 1. produces F'_A
///			2. produces ch_ch_h vec
/// 3. produces delta_vec
///			4. reconstructs F'_A
///			   with  F'_B 
/// F_A, F'_A           F_B,F'_B=f(delta_vec,F_B)
/// ------------------------------------
///
/// Complete flow:
///
/// 1.	A made a change to F_A, produced F'_A
/// 	and sent sync request to B.
/// ------------------------------------
/// A[sender]     ->    B[receiver]
/// F_A, F'A    sync_req       F_B
/// ------------------------------------
/// 2.	B calculates ch_ch_h vector.
///	and sends ch_ch_h vector to A.
/// ------------------------------------
/// A[sender]     <-    B[receiver]
/// F_A, F'A    ch_ch_h        F_B
/// ------------------------------------
/// 3.	A uses ch_ch_h vector to produce delta
/// 	vector of token matches and/or literal
///	bytes using rolling checksum and sends
///	delta vector to B.
/// A[sender]     ->    B[receiver]
/// F_A, F'A    delta_vec      F_B
/// ------------------------------------
/// 4.	B reconstructs F'_A with F'_B from
///	delta_vec and previous version F_B
///	of file F.
/// ------------------------------------
/// A[sender]     ->    B[receiver]
/// F_A, F'_A           F_B,F'_B=f(delta_vec,F_B)
/// ------------------------------------


#include "rm_defs.h"


typedef int		BLOCKSIZE;
typedef uint32_t	FASTCHECKSUM;
typedef uint32_t	DIVISOR;

/// @brief	Strong checksum struct.
struct rm_strong_ch
{
	char data[16];
};

/// @brief	Checksum checksum struct.
struct rm_ch_ch
{
	struct rm_strong_ch	s_ch;		// strong checksum
        uint32_t		f_ch;		// fast checksum
        int			index_in_b;
};

/// @brief	Checksum checksum hash struct.
struct rm_ch_ch_h
{
	struct rm_strong_ch	strong_ch;
	uint32_t		fast_ch;
	int			index_in_b;
	uint16_t		h;
};

/// @brief	Checksum hash struct.
struct rm_ch_h
{
	uint32_t	fast_ch;
	BLOCKSIZE	size;
	int		index_in_a;
	uint16_t	h;
};


#endif	// RSYNCME_H

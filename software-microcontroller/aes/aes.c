/******************************************************************************
* Fixsliced implementations of AES-128  and AES-256 (encryption-only) in C.
* Fully-fixsliced implementation should run faster than the semi-fixsliced
* version at the cost of a bigger code size.
*
* See the paper at https://eprint.iacr.org/2020/1123.pdf for more details.
*
* @author 	Alexandre Adomnicai, Nanyang Technological University, Singapore
*			alexandre.adomnicai@ntu.edu.sg
*
* @date		October 2020
******************************************************************************/
#include "aes.h"
#include <string.h> 	// for memcpy

#define ROR(x,y) 		(((x) >> (y)) | ((x) << (32 - (y))))

#define BYTE_ROR_6(x) 										\
	((((x) >> 6) & 0x03030303)	| (((x) & 0x3f3f3f3f) << 2))

#define BYTE_ROR_4(x) 										\
	((((x) >> 4) & 0x0f0f0f0f)	| (((x) & 0x0f0f0f0f) << 4))

#define BYTE_ROR_2(x) 										\
	((((x) >> 2) & 0x3f3f3f3f)	| (((x) & 0x03030303) << 6))

#define SWAPMOVE(a, b, mask, n) \
	tmp = (b ^ (a >> n)) & mask; \
	b ^= tmp; \
	a ^= (tmp << n);

#define LE_LOAD_32(x) 										\
    ((((uint32_t)((x)[3])) << 24) | 						\
     (((uint32_t)((x)[2])) << 16) | 						\
     (((uint32_t)((x)[1])) << 8) | 							\
      ((uint32_t)((x)[0])))

#define LE_STORE_32(x, y)									\
	(x)[0] = (y) & 0xff; 									\
	(x)[1] = ((y) >> 8) & 0xff; 							\
	(x)[2] = ((y) >> 16) & 0xff; 							\
	(x)[3] = (y) >> 24;


#ifndef AES_ASSEMBLY
/******************************************************************************
* Packs two 128-bit input blocs in0, in1 into the 256-bit internal state out
* where the bits are packed as follows:
* out[0] = b_24 b_56 b_88 b_120 || ... || b_0 b_32 b_64 b_96
* out[1] = b_25 b_57 b_89 b_121 || ... || b_1 b_33 b_65 b_97
* out[2] = b_26 b_58 b_90 b_122 || ... || b_2 b_34 b_66 b_98
* out[3] = b_27 b_59 b_91 b_123 || ... || b_3 b_35 b_67 b_99
* out[4] = b_28 b_60 b_92 b_124 || ... || b_4 b_36 b_68 b_100
* out[5] = b_29 b_61 b_93 b_125 || ... || b_5 b_37 b_69 b_101
* out[6] = b_30 b_62 b_94 b_126 || ... || b_6 b_38 b_70 b_102
* out[7] = b_31 b_63 b_95 b_127 || ... || b_7 b_39 b_71 b_103
******************************************************************************/
static void packing(uint32_t* out, const unsigned char* in0,
		const unsigned char* in1) {
	uint32_t tmp;
	out[0] = LE_LOAD_32(in0);
	out[1] = LE_LOAD_32(in1);
	out[2] = LE_LOAD_32(in0 + 4);
	out[3] = LE_LOAD_32(in1 + 4);
	out[4] = LE_LOAD_32(in0 + 8);
	out[5] = LE_LOAD_32(in1 + 8);
	out[6] = LE_LOAD_32(in0 + 12);
	out[7] = LE_LOAD_32(in1 + 12);
	SWAPMOVE(out[1], out[0], 0x55555555, 1);
	SWAPMOVE(out[3], out[2], 0x55555555, 1);
	SWAPMOVE(out[5], out[4], 0x55555555, 1);
	SWAPMOVE(out[7], out[6], 0x55555555, 1);
	SWAPMOVE(out[2], out[0], 0x33333333, 2);
	SWAPMOVE(out[3], out[1], 0x33333333, 2);
	SWAPMOVE(out[6], out[4], 0x33333333, 2);
	SWAPMOVE(out[7], out[5], 0x33333333, 2);
	SWAPMOVE(out[4], out[0], 0x0f0f0f0f, 4);
	SWAPMOVE(out[5], out[1], 0x0f0f0f0f, 4);
	SWAPMOVE(out[6], out[2], 0x0f0f0f0f, 4);
	SWAPMOVE(out[7], out[3], 0x0f0f0f0f, 4);
}

/******************************************************************************
* Unpacks the 256-bit internal state in two 128-bit blocs out0, out1.
******************************************************************************/
static void unpacking(unsigned char* out0, unsigned char* out1, uint32_t* in) {
	uint32_t tmp;
	SWAPMOVE(in[4], in[0], 0x0f0f0f0f, 4);
	SWAPMOVE(in[5], in[1], 0x0f0f0f0f, 4);
	SWAPMOVE(in[6], in[2], 0x0f0f0f0f, 4);
	SWAPMOVE(in[7], in[3], 0x0f0f0f0f, 4);
	SWAPMOVE(in[2], in[0], 0x33333333, 2);
	SWAPMOVE(in[3], in[1], 0x33333333, 2);
	SWAPMOVE(in[6], in[4], 0x33333333, 2);
	SWAPMOVE(in[7], in[5], 0x33333333, 2);
	SWAPMOVE(in[1], in[0], 0x55555555, 1);
	SWAPMOVE(in[3], in[2], 0x55555555, 1);
	SWAPMOVE(in[5], in[4], 0x55555555, 1);
	SWAPMOVE(in[7], in[6], 0x55555555, 1);
	LE_STORE_32(out0, in[0]);
	LE_STORE_32(out0 + 4, in[2]);
	LE_STORE_32(out0 + 8, in[4]);
	LE_STORE_32(out0 + 12, in[6]);
	LE_STORE_32(out1, in[1]);
	LE_STORE_32(out1 + 4, in[3]);
	LE_STORE_32(out1 + 8, in[5]);
	LE_STORE_32(out1 + 12, in[7]);
}

/******************************************************************************
* XOR the round key to the internal state. The round keys are expected to be
* pre-computed and to be packed in the fixsliced representation.
******************************************************************************/
static void ark(uint32_t* state, const uint32_t* rkey) {
	for(int i = 0; i < 8; i++)
		state[i] ^= rkey[i];
}

/******************************************************************************
* Bitsliced implementation of the AES Sbox based on Boyar, Peralta and Calik.
* See http://www.cs.yale.edu/homes/peralta/CircuitStuff/SLP_AES_113.txt
* Note that the 4 NOT (^= 0xffffffff) are moved to the key schedule.
******************************************************************************/
static void sbox(uint32_t* state) {
	uint32_t t0, t1, t2, t3, t4, t5,
		t6, t7, t8, t9, t10, t11, t12,
		t13, t14, t15, t16, t17;
	t0			= state[3] ^ state[5];
	t1			= state[0] ^ state[6];
	t2			= t1 ^ t0;
	t3			= state[4] ^ t2;
	t4			= t3 ^ state[5];
	t5			= t2 & t4;
	t6			= t4 ^ state[7];
	t7			= t3 ^ state[1];
	t8			= state[0] ^ state[3];
	t9			= t7 ^ t8;
	t10			= t8 & t9;
	t11			= state[7] ^ t9;
	t12			= state[0] ^ state[5];
	t13			= state[1] ^ state[2];
	t14			= t4 ^ t13;
	t15			= t14 ^ t9;
	t16			= t0 & t15;
	t17			= t16 ^ t10;
	state[1]	= t14 ^ t12;
	state[2]	= t12 & t14;
	state[2] 	^= t10;
	state[4]	= t13 ^ t9;
	state[5]	= t1 ^ state[4];
	t3			= t1 & state[4];
	t10			= state[0] ^ state[4];
	t13 		^= state[7];
	state[3] 	^= t13;
	t16			= state[3] & state[7];
	t16 		^= t5;
	t16 		^= state[2];
	state[1] 	^= t16;
	state[0] 	^= t13;
	t16			= state[0] & t11;
	t16 		^= t3;
	state[2] 	^= t16;
	state[2] 	^= t10;
	state[6] 	^= t13;
	t10			= state[6] & t13;
	t3 			^= t10;
	t3 			^= t17;
	state[5] 	^= t3;
	t3			= state[6] ^ t12;
	t10			= t3 & t6;
	t5 			^= t10;
	t5 			^= t7;
	t5 			^= t17;
	t7			= t5 & state[5];
	t10			= state[2] ^ t7;
	t7 			^= state[1];
	t5 			^= state[1];
	t16			= t5 & t10;
	state[1] 	^= t16;
	t17			= state[1] & state[0];
	t11			= state[1] & t11;
	t16			= state[5] ^ state[2];
	t7 			&= t16;
	t7 			^= state[2];
	t16			= t10 ^ t7;
	state[2] 	&= t16;
	t10 		^= state[2];
	t10 		&= state[1];
	t5 			^= t10;
	t10			= state[1] ^ t5;
	state[4] 	&= t10;
	t11 		^= state[4];
	t1 			&= t10;
	state[6] 	&= t5;
	t10			= t5 & t13;
	state[4] 	^= t10;
	state[5] 	^= t7;
	state[2] 	^= state[5];
	state[5]	= t5 ^ state[2];
	t5			= state[5] & t14;
	t10			= state[5] & t12;
	t12			= t7 ^ state[2];
	t4 			&= t12;
	t2 			&= t12;
	t3 			&= state[2];
	state[2] 	&= t6;
	state[2] 	^= t4;
	t13			= state[4] ^ state[2];
	state[3] 	&= t7;
	state[1] 	^= t7;
	state[5] 	^= state[1];
	t6			= state[5] & t15;
	state[4] 	^= t6;
	t0 			&= state[5];
	state[5]	= state[1] & t9;
	state[5] 	^= state[4];
	state[1] 	&= t8;
	t6			= state[1] ^ state[5];
	t0 			^= state[1];
	state[1]	= t3 ^ t0;
	t15			= state[1] ^ state[3];
	t2 			^= state[1];
	state[0]	= t2 ^ state[5];
	state[3]	= t2 ^ t13;
	state[1]	= state[3] ^ state[5];
	//state[1] 	^= 0xffffffff;
	t0 			^= state[6];
	state[5]	= t7 & state[7];
	t14			= t4 ^ state[5];
	state[6]	= t1 ^ t14;
	state[6] 	^= t5;
	state[6] 	^= state[4];
	state[2]	= t17 ^ state[6];
	state[5]	= t15 ^ state[2];
	state[2] 	^= t6;
	state[2] 	^= t10;
	//state[2] 	^= 0xffffffff;
	t14 		^= t11;
	t0 			^= t14;
	state[6] 	^= t0;
	//state[6] 	^= 0xffffffff;
	state[7]	= t1 ^ t0;
	//state[7] 	^= 0xffffffff;
	state[4]	= t14 ^ state[3];
}

/******************************************************************************
* Applies the ShiftRows transformation twice (i.e. SR^2) on the internal state.
******************************************************************************/
static void double_shiftrows(uint32_t* state) {
    uint32_t tmp;
	for(int i = 0; i < 8; i++) {
    SWAPMOVE(state[i], state[i], 0x0f000f00, 4);
  }

}

/******************************************************************************
* Computation of the MixColumns transformation in the fixsliced representation.
* For fully-fixsliced implementations, it is used for rounds i s.t. (i%4) == 0.
* For semi-fixsliced implementations, it is used for rounds i s.t. (i%2) == 0.
******************************************************************************/
static void mixcolumns_0(uint32_t* state) {
	uint32_t t0, t1, t2, t3, t4;
	t3 = ROR(BYTE_ROR_6(state[0]),8);
	t0 = state[0] ^ t3;
	t1 = ROR(BYTE_ROR_6(state[7]),8);
	t2 = state[7] ^ t1;
	state[7] = ROR(BYTE_ROR_4(t2),16) ^ t1 ^ t0;
	t1 = ROR(BYTE_ROR_6(state[6]),8);
	t4 = t1 ^ state[6];
	state[6] = t2 ^ t0 ^ t1 ^ ROR(BYTE_ROR_4(t4),16);
	t1 = ROR(BYTE_ROR_6(state[5]),8);
	t2 = t1 ^ state[5];
	state[5] = t4 ^ t1 ^ ROR(BYTE_ROR_4(t2),16);
	t1 = ROR(BYTE_ROR_6(state[4]),8);
	t4 = t1 ^ state[4];
	state[4] = t2 ^ t0 ^ t1 ^ ROR(BYTE_ROR_4(t4),16);
	t1 = ROR(BYTE_ROR_6(state[3]),8);
	t2 = t1 ^ state[3];
	state[3] = t4 ^ t0 ^ t1 ^ ROR(BYTE_ROR_4(t2),16);
	t1 = ROR(BYTE_ROR_6(state[2]),8);
	t4 = t1 ^ state[2];
	state[2] = t2 ^ t1 ^ ROR(BYTE_ROR_4(t4),16);
	t1 = ROR(BYTE_ROR_6(state[1]),8);
	t2 = t1 ^ state[1];
	state[1] = t4 ^ t1 ^ ROR(BYTE_ROR_4(t2),16);
	state[0] = t2 ^ t3 ^ ROR(BYTE_ROR_4(t0),16);
}

/******************************************************************************
* Computation of the MixColumns transformation in the fixsliced representation.
* For fully-fixsliced implementations only, for round i s.t. (i%4) == 1.
******************************************************************************/
static void mixcolumns_1(uint32_t* state) {
	uint32_t t0, t1, t2;
	t0 = state[0] ^ ROR(BYTE_ROR_4(state[0]),8);
	t1 = state[7] ^ ROR(BYTE_ROR_4(state[7]),8);
	t2 = state[6];
	state[6] = t1 ^ t0;
	state[7] ^= state[6] ^ ROR(t1,16);
	t1 =  ROR(BYTE_ROR_4(t2),8);
	state[6] ^= t1;
	t1 ^= t2;
	state[6] ^= ROR(t1,16);
	t2 = state[5];
	state[5] = t1;
	t1 =  ROR(BYTE_ROR_4(t2),8);
	state[5] ^= t1;
	t1 ^= t2;
	state[5] ^= ROR(t1,16);
	t2 = state[4];
	state[4] = t1 ^ t0;
	t1 =  ROR(BYTE_ROR_4(t2),8);
	state[4] ^= t1;
	t1 ^= t2;
	state[4] ^= ROR(t1,16);
	t2 = state[3];
	state[3] = t1 ^ t0;
	t1 =  ROR(BYTE_ROR_4(t2),8);
	state[3] ^= t1;
	t1 ^= t2;
	state[3] ^= ROR(t1,16);
	t2 = state[2];
	state[2] = t1;
	t1 = ROR(BYTE_ROR_4(t2),8);
	state[2] ^= t1;
	t1 ^= t2;
	state[2] ^= ROR(t1,16);
	t2 = state[1];
	state[1] = t1;
	t1 = ROR(BYTE_ROR_4(t2),8);
	state[1] ^= t1;
	t1 ^= t2;
	state[1] ^= ROR(t1,16);
	t2 = state[0];
	state[0] = t1;
	t1 = ROR(BYTE_ROR_4(t2),8);
	state[0] ^= t1;
	t1 ^= t2;
	state[0] ^= ROR(t1,16);
}

/******************************************************************************
* Computation of the MixColumns transformation in the fixsliced representation.
* For fully-fixsliced implementations only, for rounds i s.t. (i%4) == 2.
******************************************************************************/
static void mixcolumns_2(uint32_t* state) {
	uint32_t t0, t1, t2, t3, t4;
	t3 = ROR(BYTE_ROR_2(state[0]),8);
	t0 = state[0] ^ t3;
	t1 = ROR(BYTE_ROR_2(state[7]),8);
	t2 = state[7] ^ t1;
	state[7] = ROR(BYTE_ROR_4(t2),16) ^ t1 ^ t0;
	t1 = ROR(BYTE_ROR_2(state[6]),8);
	t4 = t1 ^ state[6];
	state[6] = t2 ^ t0 ^ t1 ^ ROR(BYTE_ROR_4(t4),16);
	t1 = ROR(BYTE_ROR_2(state[5]),8);
	t2 = t1 ^ state[5];
	state[5] = t4 ^ t1 ^ ROR(BYTE_ROR_4(t2),16);
	t1 = ROR(BYTE_ROR_2(state[4]),8);
	t4 = t1 ^ state[4];
	state[4] = t2 ^ t0 ^ t1 ^ ROR(BYTE_ROR_4(t4),16);
	t1 = ROR(BYTE_ROR_2(state[3]),8);
	t2 = t1 ^ state[3];
	state[3] = t4 ^ t0 ^ t1 ^ ROR(BYTE_ROR_4(t2),16);
	t1 = ROR(BYTE_ROR_2(state[2]),8);
	t4 = t1 ^ state[2];
	state[2] = t2 ^ t1 ^ ROR(BYTE_ROR_4(t4),16);
	t1 = ROR(BYTE_ROR_2(state[1]),8);
	t2 = t1 ^ state[1];
	state[1] = t4 ^ t1 ^ ROR(BYTE_ROR_4(t2),16);
	state[0] = t2 ^ t3 ^ ROR(BYTE_ROR_4(t0),16);
}

/******************************************************************************
* Computation of the MixColumns transformation in the fixsliced representation.
* For fully-fixsliced implementations, it is used for rounds i s.t. (i%4) == 3.
* For semi-fixsliced implementations, it is used for rounds i s.t. (i%2) == 1.
* Based on Käsper-Schwabe, similar to https://github.com/Ko-/aes-armcortexm.
******************************************************************************/
static void mixcolumns_3(uint32_t* state) {
	uint32_t t0, t1, t2;
	t0 = state[7] ^ ROR(state[7],8);
	t2 = state[0] ^ ROR(state[0],8);
	state[7] = t2 ^ ROR(state[7], 8) ^ ROR(t0, 16);
	t1 = state[6] ^ ROR(state[6],8);
	state[6] = t0 ^ t2 ^ ROR(state[6], 8) ^ ROR(t1,16);
	t0 = state[5] ^ ROR(state[5],8);
	state[5] = t1 ^ ROR(state[5],8) ^ ROR(t0,16);
	t1 = state[4] ^ ROR(state[4],8);
	state[4] = t0 ^ t2 ^ ROR(state[4],8) ^ ROR(t1,16);
	t0 = state[3] ^ ROR(state[3],8);
	state[3] = t1 ^ t2 ^ ROR(state[3],8) ^ ROR(t0,16);
	t1 = state[2] ^ ROR(state[2],8);
	state[2] = t0 ^ ROR(state[2],8) ^ ROR(t1,16);
	t0 = state[1] ^ ROR(state[1],8);
	state[1] = t1 ^ ROR(state[1],8) ^ ROR(t0,16);
	state[0] = t0 ^ ROR(state[0],8) ^ ROR(t2,16);
}
#endif

/******************************************************************************
* Fully-fixsliced AES-128 encryption (the ShiftRows is completely omitted).
* Two 128-bit blocks ptext0, ptext1 are encrypted into ctext0, ctext1 without
* any operating mode. The round keys are assumed to be pre-computed.
* Note that it can be included in serial operating modes since ptext0, ptext1
* can refer to the same block. Moreover ctext parameters can be the same as
* ptext parameters.
******************************************************************************/
void aes128_encrypt_ffs(unsigned char* ctext0, unsigned char * ctext1,
					const unsigned char* ptext0, const unsigned char* ptext1,
					const rkey_t* rkeys) {
	#ifndef AES_ASSEMBLY
	const uint32_t *rkeys_ffs = rkeys->rkeys;

	uint32_t state[8]; 					// 256-bit internal state
	if(ptext1 != NULL) {
		packing(state, ptext0, ptext1);		// packs into bitsliced representation
	}else{
		packing(state, ptext0, ptext0);		// packs into bitsliced representation
	}
	ark(state, rkeys_ffs); 				// key whitening
	sbox(state); 						// 1st round
	mixcolumns_0(state); 				// 1st round
	ark(state, rkeys_ffs + 8); 			// 1st round
	sbox(state); 						// 2nd round
	mixcolumns_1(state); 				// 2nd round
	ark(state, rkeys_ffs + 16); 		// 2nd round
	sbox(state); 						// 3rd round
	mixcolumns_2(state); 				// 3rd round
	ark(state, rkeys_ffs + 24); 		// 3rd round
	sbox(state); 						// 4th round
	mixcolumns_3(state); 				// 4th round
	ark(state, rkeys_ffs + 32); 		// 4th round
	sbox(state); 						// 5th round
	mixcolumns_0(state); 				// 5th round
	ark(state, rkeys_ffs + 40); 		// 5th round
	sbox(state);						// 6th round
	mixcolumns_1(state); 				// 6th round
	ark(state, rkeys_ffs + 48); 		// 6th round
	sbox(state); 						// 7th round
	mixcolumns_2(state); 				// 7th round
	ark(state, rkeys_ffs + 56); 		// 7th round
	sbox(state); 						// 8th round
	mixcolumns_3(state); 				// 8th round
	ark(state, rkeys_ffs + 64); 		// 8th round
	sbox(state); 						// 9th round
	mixcolumns_0(state); 				// 9th round
	ark(state, rkeys_ffs + 72); 		// 9th round
	sbox(state); 						// 10th round
	double_shiftrows(state); 			// 10th round (resynchronization)
	ark(state, rkeys_ffs + 80); 		// 10th round
	unpacking(ctext0, ctext1, state);	// unpacks the state to the output

	#else
	AES_128_encrypt(rkeys->rkeys, ptext0, ctext0);
	if(ptext1 != NULL) {
		AES_128_encrypt(rkeys->rkeys, ptext1, ctext1);
	}
	#endif
}

#ifndef AES_ASSEMBLY
/******************************************************************************
* Applies ShiftRows^(-1) on a round key to match the fixsliced representation.
******************************************************************************/
static void inv_shiftrows_1(uint32_t* rkey) {
	uint32_t tmp;
	for(int i = 0; i < 8; i++) {
		SWAPMOVE(rkey[i], rkey[i], 0x0c0f0300, 4);
		SWAPMOVE(rkey[i], rkey[i], 0x33003300, 2);
	}
}
/******************************************************************************
* Applies ShiftRows^(-2) on a round key to match the fixsliced representation.
******************************************************************************/
static void inv_shiftrows_2(uint32_t* rkey) {
	uint32_t tmp;
	for(int i = 0; i < 8; i++) {
    SWAPMOVE(rkey[i], rkey[i], 0x0f000f00, 4);
  }

}

/******************************************************************************
* Applies ShiftRows^(-3) on a round key to match the fixsliced representation.
******************************************************************************/
static void inv_shiftrows_3(uint32_t* rkey) {
	uint32_t tmp;
	for(int i = 0; i < 8; i++) {
		SWAPMOVE(rkey[i], rkey[i], 0x030f0c00, 4);
		SWAPMOVE(rkey[i], rkey[i], 0x33003300, 2);
	}
}

/******************************************************************************
* XOR the columns after the S-box during the key schedule round function.
* Note that the NOT omitted in the S-box calculations have to be applied t
* ensure output correctness.
* The idx_ror parameter refers to the index of the previous round key that is
* involved in the XOR computation (should be 8 and 16 for AES-128 and AES-256,
* respectively).
* The idx_ror parameter refers to the rotation value. When a Rotword is applied
* the value should be 2, 26 otherwise.
******************************************************************************/
static void xor_columns(uint32_t* rkeys, int idx_xor, int idx_ror) {
	rkeys[1] ^= 0xffffffff; 			// NOT that are omitted in S-box
	rkeys[2] ^= 0xffffffff; 			// NOT that are omitted in S-box
	rkeys[6] ^= 0xffffffff; 			// NOT that are omitted in S-box
	rkeys[7] ^= 0xffffffff; 			// NOT that are omitted in S-box
	for(int i = 0; i < 8; i++) {
		rkeys[i] = (rkeys[i-idx_xor] ^ ROR(rkeys[i], idx_ror))  & 0xc0c0c0c0;
		rkeys[i] |= ((rkeys[i-idx_xor] ^ rkeys[i] >> 2) & 0x30303030);
		rkeys[i] |= ((rkeys[i-idx_xor] ^ rkeys[i] >> 2) & 0x0c0c0c0c);
		rkeys[i] |= ((rkeys[i-idx_xor] ^ rkeys[i] >> 2) & 0x03030303);
	}
}
#endif

/******************************************************************************
* Fully bitsliced AES-128 key schedule to match the fully-fixsliced (ffs)
* representation. Note that it is possible to pass two different keys as input
* parameters if one wants to encrypt 2 blocks with two different keys.
******************************************************************************/
void aes128_keyschedule_ffs(rkey_t* rkeys_ffs, const unsigned char* key) {
	#ifndef AES_ASSEMBLY
	uint32_t *rkeys = rkeys_ffs->rkeys;
	packing(rkeys, key, key); 	// packs the keys into the bitsliced state
	memcpy(rkeys+8, rkeys, 32);
	sbox(rkeys+8);
	rkeys[15] ^= 0x00000300; 		// 1st rconst
	xor_columns(rkeys+8, 8, 2); 	// Rotword and XOR between the columns
	memcpy(rkeys+16, rkeys+8, 32);
	sbox(rkeys+16);
	rkeys[22] ^= 0x00000300;		// 2nd rconst
	xor_columns(rkeys+16, 8, 2); 	// Rotword and XOR between the columns
	inv_shiftrows_1(rkeys+8); 		// to match fixslicing
	memcpy(rkeys+24, rkeys+16, 32);
	sbox(rkeys+24);
	rkeys[29] ^= 0x00000300;		// 3rd rconst
	xor_columns(rkeys+24, 8, 2); 	// Rotword and XOR between the columns
	inv_shiftrows_2(rkeys+16); 		// to match fixslicing
	memcpy(rkeys+32, rkeys+24, 32);
	sbox(rkeys+32);
	rkeys[36] ^= 0x00000300; 		// 4th rconst
	xor_columns(rkeys+32, 8, 2); 	// Rotword and XOR between the columns
	inv_shiftrows_3(rkeys+24); 		// to match fixslicing
	memcpy(rkeys+40, rkeys+32, 32);
	sbox(rkeys+40);
	rkeys[43] ^= 0x00000300; 		// 5th rconst
	xor_columns(rkeys+40, 8, 2); 	// Rotword and XOR between the columns
	memcpy(rkeys+48, rkeys+40, 32);
	sbox(rkeys+48);
	rkeys[50] ^= 0x00000300;		// 6th rconst
	xor_columns(rkeys+48, 8, 2); 	// Rotword and XOR between the columns
	inv_shiftrows_1(rkeys+40); 		// to match fixslicing
	memcpy(rkeys+56, rkeys+48, 32);
	sbox(rkeys+56);
	rkeys[57] ^= 0x00000300;		// 7th rconst
	xor_columns(rkeys+56, 8, 2); 	// Rotword and XOR between the columns
	inv_shiftrows_2(rkeys+48); 		// to match fixslicing
	memcpy(rkeys+64, rkeys+56, 32);
	sbox(rkeys+64);
	rkeys[64] ^= 0x00000300;		// 8th rconst
	xor_columns(rkeys+64, 8, 2); 	// Rotword and XOR between the columns
	inv_shiftrows_3(rkeys+56); 		// to match fixslicing
	memcpy(rkeys+72, rkeys+64, 32);
	sbox(rkeys+72);
	rkeys[79] ^= 0x00000300; 		// 9th rconst
	rkeys[78] ^= 0x00000300; 		// 9th rconst
	rkeys[76] ^= 0x00000300; 		// 9th rconst
	rkeys[75] ^= 0x00000300; 		// 9th rconst
	xor_columns(rkeys + 72, 8, 2); 	// Rotword and XOR between the columns
	memcpy(rkeys+80, rkeys+72, 32);
	sbox(rkeys+80);
	rkeys[86] ^= 0x00000300; 		// 10th rconst
	rkeys[85] ^= 0x00000300; 		// 10th rconst
	rkeys[83] ^= 0x00000300;		// 10th rconst
	rkeys[82] ^= 0x00000300; 		// 10th rconst
	xor_columns(rkeys+80, 8, 2); 	// Rotword and XOR between the columns
	inv_shiftrows_1(rkeys+72);
	for(int i = 1; i < 11; i++) {
		rkeys[i*8 + 1] ^= 0xffffffff; 	// NOT to speed up SBox calculations
		rkeys[i*8 + 2] ^= 0xffffffff; 	// NOT to speed up SBox calculations
		rkeys[i*8 + 6] ^= 0xffffffff; 	// NOT to speed up SBox calculations
		rkeys[i*8 + 7] ^= 0xffffffff; 	// NOT to speed up SBox calculations
	}
	#else
	memcpy(rkeys_ffs->rkeys, key, 16);
	AES_128_keyschedule(key, rkeys_ffs->rkeys + 16);
	#endif
}

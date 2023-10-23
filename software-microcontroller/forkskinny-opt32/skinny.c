#include "skinny.h"
#include "internal-skinnyutil.h"

static unsigned char const RC[SKINNY_128_256_ROUNDS] = {
  0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3E, 0x3D, 0x3B, 0x37, 0x2F,
  0x1E, 0x3C, 0x39, 0x33, 0x27, 0x0E, 0x1D, 0x3A, 0x35, 0x2B,
  0x16, 0x2C, 0x18, 0x30, 0x21, 0x02, 0x05, 0x0B, 0x17, 0x2E,
  0x1C, 0x38, 0x31, 0x23, 0x06, 0x0D, 0x1B, 0x36, 0x2D, 0x1A,
  0x34, 0x29, 0x12, 0x24, 0x08, 0x11, 0x22, 0x04
};

void skinny_128_256_init_tk1(skinny_128_256_tweakey_schedule_t *tks, const unsigned char key[16], uint8_t nb_rounds) {
  uint32_t TK[4];
	unsigned round;
  uint8_t rc;

	/* Load first Tweakey */
	TK[0] = le_load_word32(key);
	TK[1] = le_load_word32(key + 4);
	TK[2] = le_load_word32(key + 8);
	TK[3] = le_load_word32(key + 12);
	/* Initiate key schedule with permutations of TK1 */
	for(round = 0; round<nb_rounds; round++){
    rc = RC[round];
		tks->row0[round] = TK[0] ^ (rc & 0x0F);
		tks->row1[round] = TK[1] ^ (rc >> 4);

		skinny128_permute_tk(TK);
	}
}
void skinny_128_256_init_tk2(skinny_128_256_tweakey_schedule_t *tks, const unsigned char key[16], uint8_t nb_rounds) {
  uint32_t TK[4];
	unsigned round;

  /* Load second Tweakey */
	TK[0] = le_load_word32(key);
	TK[1] = le_load_word32(key + 4);
	TK[2] = le_load_word32(key + 8);
	TK[3] = le_load_word32(key + 12);
	/* Process second Tweakey and add it to the key schedule */
	for(round = 0; round<nb_rounds; round++){
		tks->row0[round] = TK[0];
		tks->row1[round] = TK[1];

		skinny128_permute_tk(TK);
		skinny128_LFSR2(TK[0]);
		skinny128_LFSR2(TK[1]);
	}
}

void skinny_128_256_init_tks(skinny_128_256_tweakey_schedule_t *tks1, skinny_128_256_tweakey_schedule_t *tks2, const unsigned char key[32], uint8_t nb_rounds)
{
  skinny_128_256_init_tk1(tks1, key, nb_rounds);
  skinny_128_256_init_tk2(tks2, key + 16, nb_rounds);
}

void skinny_128_256_rounds
    (skinny_128_256_state_t *state, const skinny_128_256_tweakey_schedule_t *tks1, const skinny_128_256_tweakey_schedule_t *tks2, unsigned first, unsigned last)
{
    uint32_t s0, s1, s2, s3, temp;

    /* Load the state into local variables */
    s0 = state->S[0];
    s1 = state->S[1];
    s2 = state->S[2];
    s3 = state->S[3];

    /* Perform all requested rounds */
    for (; first < last; ++first) {
        /* Apply the S-box to all cells in the state */
        skinny128_sbox(s0);
        skinny128_sbox(s1);
        skinny128_sbox(s2);
        skinny128_sbox(s3);

        /* XOR the round constant and the subkey for this round */

        s0 ^= tks1->row0[first] ^ tks2->row0[first]; //^ 0x00020000;
        s1 ^= tks1->row1[first] ^ tks2->row1[first];
        s2 ^= 0x02;

        /* Shift the cells in the rows right, which moves the cell
         * values up closer to the MSB.  That is, we do a left rotate
         * on the word to rotate the cells in the word right */
        s1 = leftRotate8(s1);
        s2 = leftRotate16(s2);
        s3 = leftRotate24(s3);

        /* Mix the columns */
        s1 ^= s2;
        s2 ^= s0;
        temp = s3 ^ s2;
        s3 = s2;
        s2 = s1;
        s1 = s0;
        s0 = temp;
    }

    /* Save the local variables back to the state */
    state->S[0] = s0;
    state->S[1] = s1;
    state->S[2] = s2;
    state->S[3] = s3;
}

void skinny_128_256_encrypt_with_tks(const skinny_128_256_tweakey_schedule_t *tks1, const skinny_128_256_tweakey_schedule_t *tks2, unsigned char *output, const unsigned char *input) {
    skinny_128_256_state_t state;

    /* Unpack the input */
    state.S[0] = le_load_word32(input);
    state.S[1] = le_load_word32(input + 4);
    state.S[2] = le_load_word32(input + 8);
    state.S[3] = le_load_word32(input + 12);

    /* Run all of the rounds */
    skinny_128_256_rounds(&state, tks1, tks2, 0, SKINNY_128_256_ROUNDS);

    le_store_word32(output,      state.S[0]);
    le_store_word32(output + 4,  state.S[1]);
    le_store_word32(output + 8,  state.S[2]);
    le_store_word32(output + 12, state.S[3]);
}

void skinny_128_256_encrypt(const unsigned char key[32], unsigned char *output, const unsigned char *input) {
    skinny_128_256_tweakey_schedule_t tks1, tks2;

    /* Iterate the tweakey schedule */
    skinny_128_256_init_tks(&tks1, &tks2, key, SKINNY_128_256_ROUNDS);
    skinny_128_256_encrypt_with_tks(&tks1, &tks2, output, input);
}

void skinny_64_192_init_tks(skinny_64_192_tweakey_schedule_t *tks, const unsigned char key[24], uint8_t nb_rounds)
{
	uint16_t TK[4];
	unsigned round;

	/* Load first Tweakey */
	TK[0] = be_load_word16(key);
	TK[1] = be_load_word16(key + 2);
	TK[2] = be_load_word16(key + 4);
	TK[3] = be_load_word16(key + 6);
	/* Initiate key schedule with permutations of TK1 */
	for(round = 0; round<nb_rounds; round++){
		tks->row0[round] = TK[0];
		tks->row1[round] = TK[1];

		skinny64_permute_tk(TK);
	}

	/* Load second Tweakey */
	TK[0] = be_load_word16(key + 8);
	TK[1] = be_load_word16(key + 10);
	TK[2] = be_load_word16(key + 12);
	TK[3] = be_load_word16(key + 14);

	/* Process second Tweakey and add it to the key schedule */
	for(round = 0; round<nb_rounds; round++){
		tks->row0[round] ^= TK[0];
		tks->row1[round] ^= TK[1];

		skinny64_permute_tk(TK);
		skinny64_LFSR2(TK[0]);
		skinny64_LFSR2(TK[1]);
	}

	/* Load third Tweakey */
	TK[0] = be_load_word16(key + 16);
	TK[1] = be_load_word16(key + 18);
	TK[2] = be_load_word16(key + 20);
	TK[3] = be_load_word16(key + 22);
	/* Process third Tweakey and add it to the key schedule */
	for(round = 0; round<nb_rounds; round++){
		tks->row0[round] ^= TK[0];
		tks->row1[round] ^= TK[1];

		skinny64_permute_tk(TK);
		skinny64_LFSR3(TK[0]);
		skinny64_LFSR3(TK[1]);
	}
}

void skinny_64_192_rounds(skinny_64_192_state_t *state, const skinny_64_192_tweakey_schedule_t *tks, unsigned first, unsigned last)
{
    uint16_t s0, s1, s2, s3, temp;
    uint8_t rc;

    /* Load the state into local variables */
    s0 = state->S[0];
    s1 = state->S[1];
    s2 = state->S[2];
    s3 = state->S[3];

    /* Perform all requested rounds */
    for (; first < last; ++first) {
        /* Apply the S-box to all cells in the state */
        skinny64_sbox(s0);
        skinny64_sbox(s1);
        skinny64_sbox(s2);
        skinny64_sbox(s3);

        /* XOR the round constant and the subkey for this round */
        rc = RC[first];
        s0 ^= tks->row0[first] ^ ((rc & 0x0F) << 12);// ^ 0x0020;
        s1 ^= tks->row1[first] ^ ((rc & 0x70) << 8);
        s2 ^= 0x2000;

        /* Shift the cells in the rows right */
        s1 = rightRotate4_16(s1);
        s2 = rightRotate8_16(s2);
        s3 = rightRotate12_16(s3);

        /* Mix the columns */
        s1 ^= s2;
        s2 ^= s0;
        temp = s3 ^ s2;
        s3 = s2;
        s2 = s1;
        s1 = s0;
        s0 = temp;
    }

    /* Save the local variables back to the state */
    state->S[0] = s0;
    state->S[1] = s1;
    state->S[2] = s2;
    state->S[3] = s3;
}

void skinny_64_192_encrypt(const unsigned char key[24], unsigned char *output, const unsigned char *input) {
  skinny_64_192_state_t state;
  skinny_64_192_tweakey_schedule_t tks;

  skinny_64_192_init_tks(&tks, key, SKINNY_64_192_ROUNDS);

  /* Unpack the input */
  state.S[0] = be_load_word16(input);
  state.S[1] = be_load_word16(input + 2);
  state.S[2] = be_load_word16(input + 4);
  state.S[3] = be_load_word16(input + 6);

  skinny_64_192_rounds(&state, &tks, 0, SKINNY_64_192_ROUNDS);

  be_store_word16(output,     state.S[0]);
  be_store_word16(output + 2, state.S[1]);
  be_store_word16(output + 4, state.S[2]);
  be_store_word16(output + 6, state.S[3]);

}

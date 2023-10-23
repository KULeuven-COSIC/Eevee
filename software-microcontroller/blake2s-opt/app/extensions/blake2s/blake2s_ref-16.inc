typedef struct blake2s_uint32 {
	uint16_t lo, hi;
} blake2s_uint32;

static void
U8TO32(const unsigned char *p, blake2s_uint32 *v) {
	v->lo = 
		((uint16_t)p[0]      ) |
		((uint16_t)p[1] <<  8);
	v->hi =
		((uint16_t)p[2]      ) |
		((uint16_t)p[3] <<  8);
}

static void
U32TO8(unsigned char *p, blake2s_uint32 *v) {
	p[0] = (v->lo      ) & 0xff;
	p[1] = (v->lo >>  8) & 0xff;
	p[2] = (v->hi      ) & 0xff;
	p[3] = (v->hi >>  8) & 0xff;
}

static void
XOR32(blake2s_uint32 *x, const blake2s_uint32 *y) {
	x->lo ^= y->lo;
	x->hi ^= y->hi;
}

static void
ADD32(blake2s_uint32 *x, const blake2s_uint32 *y) {
	uint16_t u0 = x->lo;
	unsigned char c;
	x->lo += y->lo;
	c = (x->lo < u0);
	x->hi += c + y->hi;
}

static void
ADD32_BYTE(blake2s_uint32 *x, unsigned char y) {
	uint16_t u0 = x->lo;
	x->lo += y;
	x->hi += (x->lo < u0);
}

static void
ROTR32_16(blake2s_uint32 *x) {
	uint16_t u0 = x->lo;
	x->lo = x->hi;
	x->hi = u0;
}

static void
ROTR32_12(blake2s_uint32 *x) {
	uint16_t u0 = (x->lo << 4);
	uint16_t u1 = (x->hi << 4);
	x->lo = (x->lo >> 12) | u1;
	x->hi = (x->hi >> 12) | u0;
}

static void
ROTR32_8(blake2s_uint32 *x) {
	uint16_t u0 = (x->lo << 8);
	uint16_t u1 = (x->hi << 8);
	x->lo = (x->lo >> 8) | u1;
	x->hi = (x->hi >> 8) | u0;
}

static void
ROTR32_7(blake2s_uint32 *x) {
	uint16_t u0 = (x->lo << 9);
	uint16_t u1 = (x->hi << 9);
	x->lo = (x->lo >> 7) | u1;
	x->hi = (x->hi >> 7) | u0;
}

static int
IS_ZERO32(const blake2s_uint32 *x) {
	return (!x->lo && !x->hi);
}

static int
IS_LT32_BYTE(const blake2s_uint32 *x, unsigned char y) {
	return ((x->hi == 0) && (x->lo < y));
}

static void
blake2s_blocks_ref(blake2s_state_internal *S, const unsigned char *in, size_t bytes, size_t stride) {
	static const blake2s_uint32 w[8] = {
		{0xe667, 0x6a09},
		{0xae85, 0xbb67},
		{0xf372, 0x3c6e},
		{0xf53a, 0xa54f},
		{0x527f, 0x510e},
		{0x688c, 0x9b05},
		{0xd9ab, 0x1f83},
		{0xcd19, 0x5be0}
	};
	static const unsigned char indices[32] = {
		0,4,8,12,1,5,9,13,2,6,10,14,3,7,11,15,
		0,5,10,15,1,6,11,12,2,7,8,13,3,4,9,14
	};
	static const unsigned char sigma[10][16] = {
		{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
		{14, 10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3},
		{11, 8, 12, 0, 5, 2, 15, 13, 10, 14, 3, 6, 7, 1, 9, 4},
		{7, 9, 3, 1, 13, 12, 11, 14, 2, 6, 5, 10, 4, 0, 15, 8},
		{9, 0, 5, 7, 2, 4, 10, 15, 14, 1, 11, 12, 6, 8, 3, 13},
		{2, 12, 6, 10, 0, 11, 8, 3, 4, 13, 7, 5, 15, 14, 1, 9},
		{12, 5, 1, 15, 14, 13, 4, 10, 0, 7, 6, 3, 9, 2, 8, 11},
		{13, 11, 7, 14, 12, 1, 3, 9, 5, 0, 15, 4, 8, 6, 2, 10},
		{6, 15, 14, 9, 11, 3, 0, 8, 12, 2, 13, 7, 1, 4, 10, 5},
		{10, 2, 8, 4, 7, 6, 1, 5, 15, 11, 9, 14, 3, 12, 13, 0}
	};

	const unsigned char inc = (bytes >= 64) ? 64 : (unsigned char)bytes;
	blake2s_uint32 tf[4];
	blake2s_uint32 h[8];
	unsigned char buffer[64];
	size_t i, j;

	for (i = 0; i < 2; i++) U8TO32(S->t + (i * 4), tf + i + 0);
	for (i = 0; i < 2; i++) U8TO32(S->f + (i * 4), tf + i + 2);

	/* if (f0) */
	if (!IS_ZERO32(tf + 2)) {
		memset(buffer, 0, sizeof(buffer));
		memcpy(buffer, in, bytes);
		in = buffer;
	}

	for (i = 0; i < 8; i++) U8TO32(S->h + (i * 4), h + i);

	while (1) {
		blake2s_uint32 m[16];
		blake2s_uint32 v[16];

		/*
			t0 += inc
			if (t0 < inc)
				t1 += 1
		 */
		ADD32_BYTE(tf + 0, inc);
		if (IS_LT32_BYTE(tf + 0, inc))
			ADD32_BYTE(tf + 1, 1);

		for (i = 0; i < 16; i++) U8TO32(in + (i * 4), m + i);
		for (i = 0; i < 8; i++) v[i] = h[i];
		for (i = 0; i < 8; i++)	v[i + 8] = w[i];
		for (i = 0; i < 4; i++)	XOR32(v + i + 12, tf + i);

		for (i = 0; i < 10; i++) {
			for (j = 0; j < 8; j++) {
				size_t basei = (j * 4);
				unsigned char a = indices[basei + 0];
				unsigned char b = indices[basei + 1];
				unsigned char c = indices[basei + 2];
				unsigned char d = indices[basei + 3];
				size_t bases = (j * 2);

				ADD32(v + a, m + sigma[i][bases + 0]);
				ADD32(v + a, v + b);
				XOR32(v + d, v + a);
				ROTR32_16(v + d);
				ADD32(v + c, v + d);
				XOR32(v + b, v + c);
				ROTR32_12(v + b);
				ADD32(v + a, m + sigma[i][bases + 1]);
				ADD32(v + a, v + b);
				XOR32(v + d, v + a);
				ROTR32_8(v + d);
				ADD32(v + c, v + d);
				XOR32(v + b, v + c);
				ROTR32_7(v + b);
			}
		}

		for (i = 0; i < 8; i++) {
			XOR32(v + i, v + i + 8);
			XOR32(h + i, v + i);
		}

		if (bytes <= 64)
			break;
		in += stride;
		bytes -= 64;
	}

	for (i = 0; i < 8; i++) U32TO8(S->h + (i * 4), h + i);
	for (i = 0; i < 2; i++) U32TO8(S->t + (i * 4), tf + i);
}

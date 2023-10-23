#if defined(HAVE_INT8)
	typedef uint8_t blake2s_uint8;
	#define MASK8(x) (x)
#else
	typedef unsigned char blake2s_uint8;
	#define MASK8(x) ((x) & 0xff)
#endif

typedef struct blake2s_uint32 {
	blake2s_uint8 w[4];
} blake2s_uint32;

static void
U8TO32(const unsigned char *p, blake2s_uint32 *v) {
	size_t i;
	for (i = 0; i < 4; i++)
		v->w[i] = p[i];
}

static void
U32TO8(unsigned char *p, const blake2s_uint32 *v) {
	size_t i;
	for (i = 0; i < 4; i++)
		p[i] = (unsigned char)v->w[i];
}

static void
XOR32(blake2s_uint32 *x, const blake2s_uint32 *y) {
	size_t i;
	for (i = 0; i < 4; i++)
		x->w[i] ^= y->w[i];
}

static void
ADD32(blake2s_uint32 *x, const blake2s_uint32 *y) {
	size_t i;
	blake2s_uint8 cp = 0;
	for (i = 0; i < 4; i++) {
		blake2s_uint8 u = MASK8(x->w[i] + y->w[i]);
		blake2s_uint8 c = (u < x->w[i]);
		x->w[i] = MASK8(u + cp);
		cp = (x->w[i] < u) | c;
	}
}

static void
ADD32_BYTE(blake2s_uint32 *x, blake2s_uint8 y) {
	size_t i;
	blake2s_uint8 u = x->w[0];
	x->w[0] = MASK8(x->w[0] + y);
	for (i = 1; i < 4; i++) {
		blake2s_uint8 ui = x->w[i];
		x->w[i] = MASK8(x->w[i] + (x->w[i - 1] < u));
		u = ui;
	}
}

static void
ROTR32_16(blake2s_uint32 *x) {
	size_t i;
	for (i = 0; i < 2; i++) {
		blake2s_uint8 u = x->w[i];
		x->w[i] = x->w[i + 2];
		x->w[i + 2] = u;
	}
}

static void
ROTR32_12(blake2s_uint32 *x) {
	blake2s_uint8 u0 = x->w[0];
	blake2s_uint8 u1 = x->w[1];
	x->w[0] = MASK8((x->w[2] << 4) | (u1      >> 4));
	x->w[1] = MASK8((x->w[3] << 4) | (x->w[2] >> 4));
	x->w[2] = MASK8((u0      << 4) | (x->w[3] >> 4));
	x->w[3] = MASK8((u1      << 4) | (u0      >> 4));
}

static void
ROTR32_8(blake2s_uint32 *x) {
	blake2s_uint8 u0 = x->w[0];
	x->w[0] = x->w[1];
	x->w[1] = x->w[2];
	x->w[2] = x->w[3];
	x->w[3] = u0;
}

static void
ROTR32_7(blake2s_uint32 *x) {
	size_t i;
	blake2s_uint8 u0 = x->w[0];
	x->w[0] = MASK8((x->w[0] >> 7) | (x->w[1] << 1));
	x->w[1] = MASK8((x->w[1] >> 7) | (x->w[2] << 1));
	x->w[2] = MASK8((x->w[2] >> 7) | (x->w[3] << 1));
	x->w[3] = MASK8((x->w[3] >> 7) | (u0      << 1));
}

static int
IS_ZERO32(const blake2s_uint32 *x) {
	size_t i;
	for (i = 0; i < 4; i++) {
		if (x->w[i])
			return 0;
	}
	return 1;
}

static int
IS_LT32_BYTE(const blake2s_uint32 *x, blake2s_uint8 y) {
	size_t i;
	for (i = 1; i < 4; i++) {
		if (x->w[i])
			return 0;
	}
	return (x->w[0] < y);
}

static void
blake2s_blocks_ref(blake2s_state_internal *S, const unsigned char *in, size_t bytes, size_t stride) {
	static const blake2s_uint32 w[8] = {
		{{0x67, 0xe6, 0x09, 0x6a}},
		{{0x85, 0xae, 0x67, 0xbb}},
		{{0x72, 0xf3, 0x6e, 0x3c}},
		{{0x3a, 0xf5, 0x4f, 0xa5}},
		{{0x7f, 0x52, 0x0e, 0x51}},
		{{0x8c, 0x68, 0x05, 0x9b}},
		{{0xab, 0xd9, 0x83, 0x1f}},
		{{0x19, 0xcd, 0xe0, 0x5b}}
	};
	static const blake2s_uint8 indices[32] = {
		0,4,8,12,1,5,9,13,2,6,10,14,3,7,11,15,
		0,5,10,15,1,6,11,12,2,7,8,13,3,4,9,14
	};
	static const blake2s_uint8 sigma[10][16] = {
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
	const blake2s_uint8 inc = (bytes >= 64) ? 64 : (blake2s_uint8)bytes;
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
				blake2s_uint8 a = indices[basei + 0];
				blake2s_uint8 b = indices[basei + 1];
				blake2s_uint8 c = indices[basei + 2];
				blake2s_uint8 d = indices[basei + 3];
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

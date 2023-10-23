#if defined(HAVE_INT32)
	typedef uint32_t blake2s_uint32;
	#define MASK32(x) (x)
#else
	typedef unsigned long blake2s_uint32;
	#define MASK32(x) (x & 0xfffffffful)
#endif

static blake2s_uint32
ROTR32(blake2s_uint32 x, int k) {
	x = (x >> k) | (x << (32 - k));
	return MASK32(x);
}

static blake2s_uint32
U8TO32(const unsigned char *p) {
	return
		((blake2s_uint32)p[0]      ) |
		((blake2s_uint32)p[1] <<  8) |
		((blake2s_uint32)p[2] << 16) |
		((blake2s_uint32)p[3] << 24);
}

static void
U32TO8(unsigned char *p, blake2s_uint32 v) {
	p[0] = (v      ) & 0xff;
	p[1] = (v >>  8) & 0xff;
	p[2] = (v >> 16) & 0xff;
	p[3] = (v >> 24) & 0xff;
}

static void
blake2s_blocks_ref(blake2s_state_internal *S, const unsigned char *in, size_t bytes, size_t stride) {
	const blake2s_uint32 f0 = U8TO32(&S->f[0]);
	const blake2s_uint32 f1 = U8TO32(&S->f[4]);

	const blake2s_uint32  w8 = 0x6a09e667ul;
	const blake2s_uint32  w9 = 0xbb67ae85ul;
	const blake2s_uint32 w10 = 0x3c6ef372ul;
	const blake2s_uint32 w11 = 0xa54ff53aul;
	const blake2s_uint32 w12 = 0x510e527ful;
	const blake2s_uint32 w13 = 0x9b05688cul;
	const blake2s_uint32 w14 = 0x1f83d9abul ^ f0;
	const blake2s_uint32 w15 = 0x5be0cd19ul ^ f1;

	const size_t inc = (bytes >= 64) ? 64 : bytes;

	blake2s_uint32 t0 = U8TO32(&S->t[0]);
	blake2s_uint32 t1 = U8TO32(&S->t[4]);

	blake2s_uint32 h0 = U8TO32(&S->h[0]);
	blake2s_uint32 h1 = U8TO32(&S->h[4]);
	blake2s_uint32 h2 = U8TO32(&S->h[8]);
	blake2s_uint32 h3 = U8TO32(&S->h[12]);
	blake2s_uint32 h4 = U8TO32(&S->h[16]);
	blake2s_uint32 h5 = U8TO32(&S->h[20]);
	blake2s_uint32 h6 = U8TO32(&S->h[24]);
	blake2s_uint32 h7 = U8TO32(&S->h[28]);

	blake2s_uint32 v0,v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11,v12,v13,v14,v15;
	unsigned char buffer[64];

	if (f0) {
		memset(buffer, 0, sizeof(buffer));
		memcpy(buffer, in, bytes);
		in = buffer;
	}

	while (1) {
		const blake2s_uint32 m0 = U8TO32(in + 0);
		const blake2s_uint32 m1 = U8TO32(in + 4);
		const blake2s_uint32 m2 = U8TO32(in + 8);
		const blake2s_uint32 m3 = U8TO32(in + 12);
		const blake2s_uint32 m4 = U8TO32(in + 16);
		const blake2s_uint32 m5 = U8TO32(in + 20);
		const blake2s_uint32 m6 = U8TO32(in + 24);
		const blake2s_uint32 m7 = U8TO32(in + 28);
		const blake2s_uint32 m8 = U8TO32(in + 32);
		const blake2s_uint32 m9 = U8TO32(in + 36);
		const blake2s_uint32 m10 = U8TO32(in + 40);
		const blake2s_uint32 m11 = U8TO32(in + 44);
		const blake2s_uint32 m12 = U8TO32(in + 48);
		const blake2s_uint32 m13 = U8TO32(in + 52);
		const blake2s_uint32 m14 = U8TO32(in + 56);
		const blake2s_uint32 m15 = U8TO32(in + 60);

		t0 += inc;
		if (t0 < inc)
			t1 += 1;

		v0 = h0;
		v1 = h1;
		v2 = h2;
		v3 = h3;
		v4 = h4;
		v5 = h5;
		v6 = h6;
		v7 = h7;
		v8 = w8;
		v9 = w9;
		v10 = w10;
		v11 = w11;
		v12 = w12 ^ t0;
		v13 = w13 ^ t1;
		v14 = w14;
		v15 = w15;

		#define Gs(v0,v1,v2,v3,m,r1,r2) \
			v0 += m + v1; \
			v3 ^= v0; \
			v3 = ROTR32(v3,r1); \
			v2 += v3; \
			v1 ^= v2; \
			v1 = ROTR32(v1,r2);

		#define ROUND(m0,m1,m2,m3,m4,m5,m6,m7,m8,m9,m10,m11,m12,m13,m14,m15) \
			Gs(v0,v4, v8,v12, m0, 16,12) \
			Gs(v1,v5, v9,v13, m2, 16,12) \
			Gs(v0,v4, v8,v12, m1,  8, 7) \
			Gs(v1,v5, v9,v13, m3,  8, 7) \
			Gs(v2,v6,v10,v14, m4, 16,12) \
			Gs(v3,v7,v11,v15, m6, 16,12) \
			Gs(v2,v6,v10,v14, m5,  8, 7) \
			Gs(v3,v7,v11,v15, m7,  8, 7) \
			Gs(v0,v5,v10,v15, m8, 16,12) \
			Gs(v1,v6,v11,v12,m10, 16,12) \
			Gs(v0,v5,v10,v15, m9,  8, 7) \
			Gs(v1,v6,v11,v12,m11,  8, 7) \
			Gs(v2,v7, v8,v13,m12, 16,12) \
			Gs(v3,v4, v9,v14,m14, 16,12) \
			Gs(v2,v7, v8,v13,m13,  8, 7) \
			Gs(v3,v4, v9,v14,m15,  8, 7)

		ROUND( m0, m1, m2, m3, m4, m5, m6, m7, m8, m9,m10,m11,m12,m13,m14,m15)
		ROUND(m14,m10, m4, m8, m9,m15,m13, m6, m1,m12, m0, m2,m11, m7, m5, m3)
		ROUND(m11, m8,m12, m0, m5, m2,m15,m13,m10,m14, m3, m6, m7, m1, m9, m4)
		ROUND( m7, m9, m3, m1,m13,m12,m11,m14, m2, m6, m5,m10, m4, m0,m15, m8)
		ROUND( m9, m0, m5, m7, m2, m4,m10,m15,m14, m1,m11,m12, m6, m8, m3,m13)
		ROUND( m2,m12, m6,m10, m0,m11, m8, m3, m4,m13, m7, m5,m15,m14, m1, m9)
		ROUND(m12, m5, m1,m15,m14,m13, m4,m10, m0, m7, m6, m3, m9, m2, m8,m11)
		ROUND(m13,m11, m7,m14,m12, m1, m3, m9, m5, m0,m15, m4, m8, m6, m2,m10)
		ROUND( m6,m15,m14, m9,m11, m3, m0, m8,m12, m2,m13, m7, m1, m4,m10, m5)
		ROUND(m10, m2, m8, m4, m7, m6, m1, m5,m15,m11, m9,m14, m3,m12,m13, m0)

		h0 ^= v0 ^ v8;
		h1 ^= v1 ^ v9;
		h2 ^= v2 ^ v10;
		h3 ^= v3 ^ v11;
		h4 ^= v4 ^ v12;
		h5 ^= v5 ^ v13;
		h6 ^= v6 ^ v14;
		h7 ^= v7 ^ v15;

		if (bytes <= 64)
			break;
		in += stride;
		bytes -= 64;
	}

	U32TO8(&S->h[0], h0);
	U32TO8(&S->h[4], h1);
	U32TO8(&S->h[8], h2);
	U32TO8(&S->h[12], h3);
	U32TO8(&S->h[16], h4);
	U32TO8(&S->h[20], h5);
	U32TO8(&S->h[24], h6);
	U32TO8(&S->h[28], h7);
	U32TO8(&S->t[0], t0);
	U32TO8(&S->t[4], t1);
}

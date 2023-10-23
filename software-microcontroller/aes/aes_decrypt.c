
/// Note that the 4 bitwise NOT (^= 0xffffffff) are accounted for here so that it is a true
/// inverse of 'sub_bytes'.
static void inv_sub_bytes(uint32_t* state) {

    // Scheduled using https://github.com/Ko-/aes-armcortexm/tree/public/scheduler
    // Inline "stack" comments reflect suggested stores and loads (ARM Cortex-M3 and M4)

    uint32_t u7 = state[0];
    uint32_t u6 = state[1];
    uint32_t u5 = state[2];
    uint32_t u4 = state[3];
    uint32_t u3 = state[4];
    uint32_t u2 = state[5];
    uint32_t u1 = state[6];
    uint32_t u0 = state[7];

    uint32_t t23 = u0 ^ u3;
    uint32_t t8 = u1 ^ t23;
    uint32_t m2 = t23 & t8;
    uint32_t t4 = u4 ^ t8;
    uint32_t t22 = u1 ^ u3;
    uint32_t t2 = u0 ^ u1;
    uint32_t t1 = u3 ^ u4;
    // t23 -> stack
    uint32_t t9 = u7 ^ t1;
    // t8 -> stack
    uint32_t m7 = t22 & t9;
    // t9 -> stack
    uint32_t t24 = u4 ^ u7;
    // m7 -> stack
    uint32_t t10 = t2 ^ t24;
    // u4 -> stack
    uint32_t m14 = t2 & t10;
    uint32_t r5 = u6 ^ u7;
    // m2 -> stack
    uint32_t t3 = t1 ^ r5;
    // t2 -> stack
    uint32_t t13 = t2 ^ r5;
    uint32_t t19 = t22 ^ r5;
    // t3 -> stack
    uint32_t t17 = u2 ^ t19;
    // t4 -> stack
    uint32_t t25 = u2 ^ t1;
    uint32_t r13 = u1 ^ u6;
    // t25 -> stack
    uint32_t t20 = t24 ^ r13;
    // t17 -> stack
    uint32_t m9 = t20 & t17;
    // t20 -> stack
    uint32_t r17 = u2 ^ u5;
    // t22 -> stack
    uint32_t t6 = t22 ^ r17;
    // t13 -> stack
    uint32_t m1 = t13 & t6;
    uint32_t y5 = u0 ^ r17;
    uint32_t m4 = t19 & y5;
    uint32_t m5 = m4 ^ m1;
    uint32_t m17 = m5 ^ t24;
    uint32_t r18 = u5 ^ u6;
    uint32_t t27 = t1 ^ r18;
    uint32_t t15 = t10 ^ t27;
    // t6 -> stack
    uint32_t m11 = t1 & t15;
    uint32_t m15 = m14 ^ m11;
    uint32_t m21 = m17 ^ m15;
    // t1 -> stack
    // t4 <- stack
    uint32_t m12 = t4 & t27;
    uint32_t m13 = m12 ^ m11;
    uint32_t t14 = t10 ^ r18;
    uint32_t m3 = t14 ^ m1;
    // m2 <- stack
    uint32_t m16 = m3 ^ m2;
    uint32_t m20 = m16 ^ m13;
    // u4 <- stack
    uint32_t r19 = u2 ^ u4;
    uint32_t t16 = r13 ^ r19;
    // t3 <- stack
    uint32_t t26 = t3 ^ t16;
    uint32_t m6 = t3 & t16;
    uint32_t m8 = t26 ^ m6;
    // t10 -> stack
    // m7 <- stack
    uint32_t m18 = m8 ^ m7;
    uint32_t m22 = m18 ^ m13;
    uint32_t m25 = m22 & m20;
    uint32_t m26 = m21 ^ m25;
    uint32_t m10 = m9 ^ m6;
    uint32_t m19 = m10 ^ m15;
    // t25 <- stack
    uint32_t m23 = m19 ^ t25;
    uint32_t m28 = m23 ^ m25;
    uint32_t m24 = m22 ^ m23;
    uint32_t m30 = m26 & m24;
    uint32_t m39 = m23 ^ m30;
    uint32_t m48 = m39 & y5;
    uint32_t m57 = m39 & t19;
    // m48 -> stack
    uint32_t m36 = m24 ^ m25;
    uint32_t m31 = m20 & m23;
    uint32_t m27 = m20 ^ m21;
    uint32_t m32 = m27 & m31;
    uint32_t m29 = m28 & m27;
    uint32_t m37 = m21 ^ m29;
    // m39 -> stack
    uint32_t m42 = m37 ^ m39;
    uint32_t m52 = m42 & t15;
    // t27 -> stack
    // t1 <- stack
    uint32_t m61 = m42 & t1;
    uint32_t p0 = m52 ^ m61;
    uint32_t p16 = m57 ^ m61;
    // m57 -> stack
    // t20 <- stack
    uint32_t m60 = m37 & t20;
    // p16 -> stack
    // t17 <- stack
    uint32_t m51 = m37 & t17;
    uint32_t m33 = m27 ^ m25;
    uint32_t m38 = m32 ^ m33;
    uint32_t m43 = m37 ^ m38;
    uint32_t m49 = m43 & t16;
    uint32_t p6 = m49 ^ m60;
    uint32_t p13 = m49 ^ m51;
    uint32_t m58 = m43 & t3;
    // t9 <- stack
    uint32_t m50 = m38 & t9;
    // t22 <- stack
    uint32_t m59 = m38 & t22;
    // p6 -> stack
    uint32_t p1 = m58 ^ m59;
    uint32_t p7 = p0 ^ p1;
    uint32_t m34 = m21 & m22;
    uint32_t m35 = m24 & m34;
    uint32_t m40 = m35 ^ m36;
    uint32_t m41 = m38 ^ m40;
    uint32_t m45 = m42 ^ m41;
    // t27 <- stack
    uint32_t m53 = m45 & t27;
    uint32_t p8 = m50 ^ m53;
    uint32_t p23 = p7 ^ p8;
    // t4 <- stack
    uint32_t m62 = m45 & t4;
    uint32_t p14 = m49 ^ m62;
    state[1] = p14 ^ p23;
    // t10 <- stack
    uint32_t m54 = m41 & t10;
    uint32_t p2 = m54 ^ m62;
    uint32_t p22 = p2 ^ p7;
    state[7] = p13 ^ p22;
    uint32_t p17 = m58 ^ p2;
    uint32_t p15 = m54 ^ m59;
    // t2 <- stack
    uint32_t m63 = m41 & t2;
    // m39 <- stack
    uint32_t m44 = m39 ^ m40;
    // p17 -> stack
    // t6 <- stack
    uint32_t m46 = m44 & t6;
    uint32_t p5 = m46 ^ m51;
    // p23 -> stack
    uint32_t p18 = m63 ^ p5;
    uint32_t p24 = p5 ^ p7;
    // m48 <- stack
    uint32_t p12 = m46 ^ m48;
    state[4] = p12 ^ p22;
    // t13 <- stack
    uint32_t m55 = m44 & t13;
    uint32_t p9 = m55 ^ m63;
    // p16 <- stack
    state[0] = p9 ^ p16;
    // t8 <- stack
    uint32_t m47 = m40 & t8;
    uint32_t p3 = m47 ^ m50;
    uint32_t p19 = p2 ^ p3;
    state[2] = p19 ^ p24;
    uint32_t p11 = p0 ^ p3;
    uint32_t p26 = p9 ^ p11;
    // t23 <- stack
    uint32_t m56 = m40 & t23;
    uint32_t p4 = m48 ^ m56;
    // p6 <- stack
    uint32_t p20 = p4 ^ p6;
    uint32_t p29 = p15 ^ p20;
    state[6] = p26 ^ p29;
    // m57 <- stack
    uint32_t p10 = m57 ^ p4;
    uint32_t p27 = p10 ^ p18;
    // p23 <- stack
    state[3] = p23 ^ p27;
    uint32_t p25 = p6 ^ p10;
    uint32_t p28 = p11 ^ p25;
    // p17 <- stack
    state[5] = p17 ^ p28;

    // state[0] = s7;
    // state[1] = s6;
    // state[2] = s5;
    // state[3] = s4;
    // state[4] = s3;
    // state[5] = s2;
    // state[6] = s1;
    // state[7] = s0;
}

#define ror_distance(rows,cols) ((rows << 3) + (cols << 1))

static uint32_t rotate_rows_and_columns_1_1(uint32_t x) {
    return (ROR(x, ror_distance(1, 1)) & 0x3f3f3f3f) |
    (ROR(x, ror_distance(0, 1)) & 0xc0c0c0c0);
}

static void inv_mix_columns_1(uint32_t* state) {
            let (a0, a1, a2, a3, a4, a5, a6, a7) = (
                state[0], state[1], state[2], state[3], state[4], state[5], state[6], state[7]
            );
  uint32_t b0, b1, b2, b3, b4, b5, b6, b7;
  b0 = rotate_rows_and_columns_1_1(state[0]),
                rotate_rows_and_columns_1_1(a1),
                rotate_rows_and_columns_1_1(a2),
                rotate_rows_and_columns_1_1(a3),
                $first_rotate(a4),
                $first_rotate(a5),
                $first_rotate(a6),
                $first_rotate(a7),
            );
            let (c0, c1, c2, c3, c4, c5, c6, c7) = (
                a0 ^ b0,
                a1 ^ b1,
                a2 ^ b2,
                a3 ^ b3,
                a4 ^ b4,
                a5 ^ b5,
                a6 ^ b6,
                a7 ^ b7,
            );
            let (d0, d1, d2, d3, d4, d5, d6, d7) = (
                a0      ^ c7,
                a1 ^ c0 ^ c7,
                a2 ^ c1,
                a3 ^ c2 ^ c7,
                a4 ^ c3 ^ c7,
                a5 ^ c4,
                a6 ^ c5,
                a7 ^ c6,
            );
            let (e0, e1, e2, e3, e4, e5, e6, e7) = (
                c0      ^ d6,
                c1      ^ d6 ^ d7,
                c2 ^ d0      ^ d7,
                c3 ^ d1 ^ d6,
                c4 ^ d2 ^ d6 ^ d7,
                c5 ^ d3      ^ d7,
                c6 ^ d4,
                c7 ^ d5,
            );
            state[0] = d0 ^ e0 ^ $second_rotate(e0);
            state[1] = d1 ^ e1 ^ $second_rotate(e1);
            state[2] = d2 ^ e2 ^ $second_rotate(e2);
            state[3] = d3 ^ e3 ^ $second_rotate(e3);
            state[4] = d4 ^ e4 ^ $second_rotate(e4);
            state[5] = d5 ^ e5 ^ $second_rotate(e5);
            state[6] = d6 ^ e6 ^ $second_rotate(e6);
            state[7] = d7 ^ e7 ^ $second_rotate(e7);
}

// adapted from https://github.com/RustCrypto/block-ciphers/blob/736671fe85c23b29ff085b17820e47a609145b36/aes/src/soft/fixslice32.rs
/// Fully-fixsliced AES-128 decryption (the InvShiftRows is completely omitted).
///
/// Decrypts four blocks in-place and in parallel.
void aes128_decrypt_ffs(unsigned char* ptext0, unsigned char * ptext1,
					const unsigned char* ctext0, const unsigned char* ctext1,
					const uint32_t* rkeys_ffs) {
  //rkeys: &FixsliceKeys128, blocks: &BatchBlocks) -> BatchBlocks {
    //let mut state = State::default();
    uint32_t state[8]; 					// 256-bit internal state
  	packing(state, ctext0, ctext1);		// packs into bitsliced representation
    //bitslice(&mut state, &blocks[0], &blocks[1]);

    ark(state, rkeys_ffs + 80);
    //add_round_key(&mut state, &rkeys[80..]);

    inv_sub_bytes(state);

    inv_shiftrows_2(state);
    // inv_shift_rows_2(&mut state);

    int rk_off = 72;
    for(; rk_off > 0; ) {

        ark(state, rkeys_ffs + rk_off);
        // add_round_key(&mut state, &rkeys[rk_off..(rk_off + 8)]);
        inv_mix_columns_1(&mut state);
        inv_sub_bytes(&mut state);
        rk_off -= 8;

        if rk_off == 0 {
            break;
        }

        add_round_key(&mut state, &rkeys[rk_off..(rk_off + 8)]);
        inv_mix_columns_0(&mut state);
        inv_sub_bytes(&mut state);
        rk_off -= 8;

        #[cfg(not(aes_compact))]
        {
            add_round_key(&mut state, &rkeys[rk_off..(rk_off + 8)]);
            inv_mix_columns_3(&mut state);
            inv_sub_bytes(&mut state);
            rk_off -= 8;

            add_round_key(&mut state, &rkeys[rk_off..(rk_off + 8)]);
            inv_mix_columns_2(&mut state);
            inv_sub_bytes(&mut state);
            rk_off -= 8;
        }
    }

    add_round_key(&mut state, &rkeys[..8]);

    inv_bitslice(&state)
}

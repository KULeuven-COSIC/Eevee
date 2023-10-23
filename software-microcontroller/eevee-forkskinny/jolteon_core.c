// included in jolteon.c

static void JOLTEON_PREFIX(_ad)(unsigned char *t_a, unsigned char *tweakey, const JOLTEON_TKS_T *tks1, JOLTEON_TKS_T *tks2, const unsigned char *ad, unsigned long long adlen, unsigned char no_m) {
    assert(adlen > 0);
    unsigned char ad_in[JOLTEON_BLOCKSIZE];
    unsigned char ad_out[JOLTEON_BLOCKSIZE];
    unsigned long long n_ad_blocks = adlen/JOLTEON_BLOCKSIZE;
    bool complete_block = adlen % JOLTEON_BLOCKSIZE == 0;
    if(complete_block) {
        // complete block case
        n_ad_blocks -= 1;
    }
    for(unsigned long long i=0; i< n_ad_blocks; i++) {
        // set tweak
        // 15 bit counter:  0 | i_14 | i_13 | ... | i_0
        tweakey[JOLTEON_TKSIZE-2] = ((i+4) >> 8) & 0x7F;
        tweakey[JOLTEON_TKSIZE-1] = (i+4) & 0xFF;
        JOLTEON_INIT_VAR_TKS_ONE_LEG(tks2, tweakey);

        memcpy(ad_in, ad + i*JOLTEON_BLOCKSIZE, JOLTEON_BLOCKSIZE);
        JOLTEON_FK_FORWARD(tks1, tks2, ad_out, NULL, ad_in);
        #ifdef EEVEE_VERBOSE
        printf("\nAD Block %lld\n", i);
        printf("In: ");
        print_n(ad_in, JOLTEON_BLOCKSIZE);
        printf("\nTK: ");
        print_n(tweakey, JOLTEON_TKSIZE);
        printf("\nOut: ");
        print_n(ad_out, JOLTEON_BLOCKSIZE);
        printf("\n");
        #endif
        JOLTEON_BLOCK_XOR(t_a, ad_out);
    }
    // last AD block
    // copy partial AD block
    for(unsigned int i=0; i< adlen - JOLTEON_BLOCKSIZE*n_ad_blocks; i++)
        ad_in[i] = ad[JOLTEON_BLOCKSIZE*n_ad_blocks+i];
    if(!complete_block) {
        // add padding
        ad_in[adlen % JOLTEON_BLOCKSIZE] = 0x80;
        for(unsigned int i=adlen % JOLTEON_BLOCKSIZE +1; i<JOLTEON_BLOCKSIZE; i++)
            ad_in[i] = 0;
    }
    tweakey[JOLTEON_TKSIZE-2] = 0;
    tweakey[JOLTEON_TKSIZE-1] = no_m + (complete_block) ? 0 : 2;
    JOLTEON_INIT_VAR_TKS_ONE_LEG(tks2, tweakey);
    JOLTEON_FK_FORWARD(tks1, tks2, ad_out, NULL, ad_in);

    #ifdef EEVEE_VERBOSE
    printf("\nAD Block %lld\n", n_ad_blocks);
    printf("In: ");
    print_n(ad_in, JOLTEON_BLOCKSIZE);
    printf("\nTK: ");
    print_n(tweakey, JOLTEON_TKSIZE);
    printf("\nOut: ");
    print_n(ad_out, JOLTEON_BLOCKSIZE);
    printf("\n");
    #endif

    JOLTEON_BLOCK_XOR(t_a, ad_out);
}

int JOLTEON_PREFIX(_encrypt)(unsigned char *c, unsigned char *tag, const unsigned char *m, unsigned long long mlen, const unsigned char *ad, unsigned long long adlen, const unsigned char *npub, const unsigned char *k) {
    if(mlen > JOLTEON_MAX_MESSAGE_LENGTH) {
        printf("Message too long!\n");
        return 1;
    }
    if(adlen > JOLTEON_MAX_MESSAGE_LENGTH) {
        printf("AD too long!\n");
        return 1;
    }
    if(adlen == 0 && mlen == 0) {
      return 1;
    }

    unsigned char tweakey[JOLTEON_TKSIZE];
    unsigned char t_a[JOLTEON_BLOCKSIZE];
    memset(t_a, 0, JOLTEON_BLOCKSIZE);

    // load key
    memcpy(tweakey, k, JOLTEON_KEYSIZE);
    // load nonce (JOLTEON_NONCESIZE byte)
    memcpy(tweakey + JOLTEON_KEYSIZE, npub, JOLTEON_NONCESIZE);

    JOLTEON_TKS_T tks1, tks2;
    JOLTEON_INIT_FIXED_TKS(&tks1, tweakey);

    // AD
    if(adlen > 0)
      JOLTEON_PREFIX(_ad)(t_a, tweakey, &tks1, &tks2, ad, adlen, mlen == 0);

    #ifdef EEVEE_VERBOSE
    printf("\nT_A after AD: ");
    print_n(t_a, JOLTEON_BLOCKSIZE);
    printf("\n");
    #endif

    // message
    if(mlen == 0) {
        // output T_a as tag
        memcpy(tag, t_a, JOLTEON_BLOCKSIZE);
        return 0;
    }

    unsigned long long n_m_blocks = mlen/JOLTEON_BLOCKSIZE;
    bool complete_block = mlen % JOLTEON_BLOCKSIZE == 0;
    if(complete_block) {
        // complete block case
        n_m_blocks -= 1;
    }
    const unsigned char *m_in = m;
    unsigned char *c_out = c;
    for(unsigned long long i=0; i<n_m_blocks; i++) {
        // set tweak
        // 15 bit counter:  1 | i_14 | i_13 | ... | i_0
        tweakey[JOLTEON_TKSIZE-2] = (((i+2) >> 8) & 0x7F) | 0x80;
        tweakey[JOLTEON_TKSIZE-1] = (i+2) & 0xFF;

        // for(unsigned k=0; k<JOLTEON_BLOCKSIZE; k++) {
        //   m_in[k] = m[i*JOLTEON_BLOCKSIZE + k]; ^ last_c[k];
        // }
        JOLTEON_BLOCK_XOR(t_a, m_in);
        // if(i == 0) {
        //   last_c = c;
        //   memcpy(t_a, m_in, JOLTEON_BLOCKSIZE);
        // }else{
        //   last_c += JOLTEON_BLOCKSIZE;
        //   JOLTEON_BLOCK_XOR(t_a, m_in);
        // }
        JOLTEON_INIT_VAR_TKS_ONE_LEG(&tks2, tweakey);
        JOLTEON_FK_FORWARD(&tks1, &tks2, c_out, NULL, m_in);

        JOLTEON_BLOCK_XOR(t_a, c_out);

        #ifdef EEVEE_VERBOSE
        printf("\nM Block %lld\nm_in: ", i);
        print_n(m_in, JOLTEON_BLOCKSIZE);
        printf("\nTK: ");
        print_n(tweakey, JOLTEON_TKSIZE);
        printf("\nC0: ");
        print_n(&c[i*JOLTEON_BLOCKSIZE], JOLTEON_BLOCKSIZE);
        printf("\nT_A: ");
        print_n(t_a, JOLTEON_BLOCKSIZE);
        printf("\n");
        #endif
        m_in += JOLTEON_BLOCKSIZE;
        c_out += JOLTEON_BLOCKSIZE;
    }

    unsigned char last_m[JOLTEON_BLOCKSIZE];

    // last message block
    // copy partial message
    memcpy(last_m, m + n_m_blocks * JOLTEON_BLOCKSIZE, mlen - JOLTEON_BLOCKSIZE*n_m_blocks);
    if(!complete_block) {
        // add padding
        last_m[mlen % JOLTEON_BLOCKSIZE] = 0x80;
        for(unsigned int i=mlen%JOLTEON_BLOCKSIZE+1; i<JOLTEON_BLOCKSIZE; i++)
            last_m[i] = 0;
    }
    JOLTEON_BLOCK_XOR(last_m, t_a);
    // set tweak to 1 | 0 | ... | if(complete_block) 0 else 1
    tweakey[JOLTEON_TKSIZE-2] = 0x80;
    tweakey[JOLTEON_TKSIZE-1] = (complete_block) ? 0 : 1;
    unsigned char last_block[JOLTEON_BLOCKSIZE];
    unsigned int last_block_length = (complete_block) ? JOLTEON_BLOCKSIZE : mlen % JOLTEON_BLOCKSIZE;
    JOLTEON_INIT_VAR_TKS_TWO_LEG(&tks2, tweakey);
    JOLTEON_FK_FORWARD(&tks1, &tks2, tag, last_block, last_m);
    for(unsigned int i=0; i<last_block_length; i++)
            c[JOLTEON_BLOCKSIZE*n_m_blocks + i] = m[n_m_blocks * JOLTEON_BLOCKSIZE + i]  ^ last_block[i];

    #ifdef EEVEE_VERBOSE
    printf("\nM Block %lld\nm_in: ", n_m_blocks);
    print_n(last_m, JOLTEON_BLOCKSIZE);
    printf("\nTK: ");
    print_n(tweakey, JOLTEON_TKSIZE);
    printf("\nTag: ");
    print_n(tag, JOLTEON_BLOCKSIZE);
    printf("\nC1*: ");
    print_n(last_block, last_block_length);
    printf("\nC*: ");
    print_n(&c[JOLTEON_BLOCKSIZE*n_m_blocks], last_block_length);
    printf("\n");
    #endif
    return 0;
}

int JOLTEON_PREFIX(_decrypt)(unsigned char *m, const unsigned char *c, unsigned long long clen, const unsigned char *tag, const unsigned char *ad, unsigned long long adlen, const unsigned char *npub, const unsigned char *k) {
    if(clen > JOLTEON_MAX_MESSAGE_LENGTH) {
        printf("Ciphertext too long!\n");
        return 1;
    }
    if(adlen > JOLTEON_MAX_MESSAGE_LENGTH) {
        printf("AD too long!\n");
        return 1;
    }
    if(adlen == 0 && clen == 0) {
      return 1;
    }

    unsigned char tweakey[JOLTEON_TKSIZE];
    unsigned char t_a[JOLTEON_BLOCKSIZE];
    memset(t_a, 0, JOLTEON_BLOCKSIZE);

    // load key
    memcpy(tweakey, k, JOLTEON_KEYSIZE);
    // load nonce (JOLTEON_NONCESIZE byte)
    memcpy(tweakey + JOLTEON_KEYSIZE, npub, JOLTEON_NONCESIZE);

    JOLTEON_TKS_T tks1, tks2;
    JOLTEON_INIT_FIXED_TKS(&tks1, tweakey);

    // AD
    if(adlen > 0)
      JOLTEON_PREFIX(_ad)(t_a, tweakey, &tks1, &tks2, ad, adlen, clen == 0);

    #ifdef EEVEE_VERBOSE
    printf("\nT_A after AD: ");
    print_n(t_a, JOLTEON_BLOCKSIZE);
    printf("\n");
    #endif

    if(clen == 0) {
        // no ciphertext, just a tag
        // check tag (THIS IS NOT CONSTANT TIME!), see commented code below for constant time implementation
        for(unsigned int i=0; i<JOLTEON_BLOCKSIZE; i++) {
            if(tag[i] != t_a[i]) return 1;
        }
        return 0;
        /*int fail = 0;
        for(unsigned int i=0; i<JOLTEON_BLOCKSIZE; i++)
            fail |= tag[i] != t_a[i];
        return fail;*/
    }

    long long n_m_blocks = clen/JOLTEON_BLOCKSIZE;
    bool complete_block = clen % JOLTEON_BLOCKSIZE == 0;
    if(complete_block) {
        n_m_blocks -= 1;
    }
    const unsigned char *c_in = c;
    unsigned char *m_out = m;
    for(long long i=0; i<n_m_blocks; i++) {
        // set tweak
        // 15 bit counter:  1 | i_14 | i_13 | ... | i_0
        tweakey[JOLTEON_TKSIZE-2] = (((i+2) >> 8) & 0x7F) | 0x80;
        tweakey[JOLTEON_TKSIZE-1] = (i+2) & 0xFF;
        JOLTEON_INIT_VAR_TKS_ONE_LEG(&tks2, tweakey);
        JOLTEON_FK_INVERT(&tks1, &tks2, m_out, NULL, c_in);
        // for(unsigned k=0; k<JOLTEON_BLOCKSIZE; k++) {
        //   m[i*JOLTEON_BLOCKSIZE + k] = out[k] ^ last_c[k];
        // }
        JOLTEON_BLOCK_XOR(t_a, c_in);
        JOLTEON_BLOCK_XOR(t_a, m_out);

        // if(i == 0) {
        //   last_c = c;
        //   memcpy(t_a, out, JOLTEON_BLOCKSIZE);
        // }else{
        //   last_c += JOLTEON_BLOCKSIZE;
        //   JOLTEON_BLOCK_XOR(t_a, out);
        // }

        #ifdef EEVEE_VERBOSE
        printf("\nC Block %lld\n", i);
        printf("In: ");
        print_n(c_in, JOLTEON_BLOCKSIZE);
        printf("\nTK: ");
        print_n(tweakey, JOLTEON_TKSIZE);
        printf("\nM Block %lld: ", i);
        print_n(m_out, JOLTEON_BLOCKSIZE);
        printf("\nT_A: ");
        print_n(t_a, JOLTEON_BLOCKSIZE);
        printf("\n");
        #endif

        c_in += JOLTEON_BLOCKSIZE;
        m_out += JOLTEON_BLOCKSIZE;
    }

    // last message block
    // set tweak to 1 | 0 | ... | if(complete_block) 0 else 1
    tweakey[JOLTEON_TKSIZE-2] = 0x80;
    tweakey[JOLTEON_TKSIZE-1] = (complete_block) ? 0 : 1;
    unsigned int last_block_length = (complete_block) ? JOLTEON_BLOCKSIZE : clen % JOLTEON_BLOCKSIZE;

    unsigned char c_out[JOLTEON_BLOCKSIZE];
    unsigned char out[JOLTEON_BLOCKSIZE];
    JOLTEON_INIT_VAR_TKS_TWO_LEG(&tks2, tweakey);
    JOLTEON_FK_INVERT(&tks1, &tks2, out, c_out, tag);

    #ifdef EEVEE_VERBOSE
    printf("\nC Block %lld\n", n_m_blocks);
    printf("In: ");
    print_n(tag, JOLTEON_BLOCKSIZE);
    printf("\nTK: ");
    print_n(tweakey, JOLTEON_TKSIZE);
    printf("\nout: ");
    print_n(out, JOLTEON_BLOCKSIZE);
    printf("\nc_out: ");
    print_n(c_out, JOLTEON_BLOCKSIZE);
    printf("\n");
    #endif

    JOLTEON_BLOCK_XOR(out, t_a);
    // JOLTEON_BLOCK_XOR(out, last_c);

    // output partial message
    memcpy(m + n_m_blocks*JOLTEON_BLOCKSIZE, out, last_block_length);

    #ifdef EEVEE_VERBOSE
    printf("M*: ");
    print_n(out, last_block_length);
    printf("\n");
    // check tag (THIS IS NOT CONSTANT TIME!), see commented code below for a constant time implementation
    printf("\nCheck padding: ");
    print_n(&out[last_block_length], JOLTEON_BLOCKSIZE-last_block_length);
    printf("\n");
    #endif

    if(last_block_length < JOLTEON_BLOCKSIZE && out[last_block_length] != 0x80) return 1;
    for(unsigned int i=last_block_length +1; i<JOLTEON_BLOCKSIZE; i++) {
        if(out[i] != 0x00) return 1;
    }

    #ifdef EEVEE_VERBOSE
    printf("\nC*': ");
    for(unsigned int i=0; i< last_block_length; i++)
        printf("%02x", out[i] ^ c_out[i]);
    printf(" == ");
    for(unsigned int i=0; i< last_block_length; i++)
        printf("%02x", c[JOLTEON_BLOCKSIZE*n_m_blocks+i]);
    printf("\n");
    #endif

    for(unsigned int i=0; i< last_block_length; i++) {
        if((out[i] ^ c_out[i]) != c[JOLTEON_BLOCKSIZE*n_m_blocks+i]) return 1;
    }
    return 0;
    /**
    int fail = 0;
    if(last_block_length < JOLTEON_BLOCKSIZE)
        fail |= c1_old[JOLTEON_BLOCKSIZE*n_m_blocks + last_block_length] != 0x80;
    for(unsigned int i=last_block_length +1; i<JOLTEON_BLOCKSIZE; i++) {
        fail |= c1_old[i] != 0x00;
    }

    printf("C*': ");
    for(unsigned int i=0; i< last_block_length; i++)
        printf("%02x", c1_out[i] ^ c1_old[i]);
    printf(" == ");
    for(unsigned int i=0; i< last_block_length; i++)
        printf("%02x", c[JOLTEON_BLOCKSIZE*n_m_blocks+i]);
    printf("\n");
    for(unsigned int i=0; i< last_block_length; i++) {
        fail |= (c1_out[i] ^ c1_old[i]) != c[JOLTEON_BLOCKSIZE*n_m_blocks+i];
    }
    return fail;
    **/
}

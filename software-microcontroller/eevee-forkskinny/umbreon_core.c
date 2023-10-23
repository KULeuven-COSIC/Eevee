// included in eevee.c


static void EEVEE_PREFIX(_ad)(unsigned char *t_a, unsigned char *tweakey, const EEVEE_TKS_T *tks1, EEVEE_TKS_T *tks2, const unsigned char *ad, unsigned long long adlen, unsigned char no_m) {
    assert(adlen > 0);
    unsigned char ad_in[EEVEE_BLOCKSIZE];
    unsigned char ad_out[EEVEE_BLOCKSIZE];
    unsigned long long n_ad_blocks = adlen/EEVEE_BLOCKSIZE;
    bool complete_block = adlen % EEVEE_BLOCKSIZE == 0;
    if(complete_block) {
        // complete block case
        n_ad_blocks -= 1;
    }
    for(unsigned long long i=0; i< n_ad_blocks; i++) {
        // set tweak
        // 15 bit counter:  0 | i_14 | i_13 | ... | i_0
        tweakey[EEVEE_TKSIZE-2] = ((i+4) >> 8) & 0x7F;
        tweakey[EEVEE_TKSIZE-1] = (i+4) & 0xFF;

        memcpy(ad_in, ad + i*EEVEE_BLOCKSIZE, EEVEE_BLOCKSIZE);
        EEVEE_INIT_VAR_TKS_ONE_LEG(tks2, tweakey);
        EEVEE_FK_FORWARD(tks1, tks2, ad_out, NULL, ad_in);
        #ifdef EEVEE_VERBOSE
        printf("\nAD Block %lld\n", i);
        printf("In: ");
        print_n(ad_in, EEVEE_BLOCKSIZE);
        printf("\nTK: ");
        print_n(tweakey, EEVEE_TKSIZE);
        printf("\nOut: ");
        print_n(ad_out, EEVEE_BLOCKSIZE);
        printf("\n");
        #endif
        EEVEE_BLOCK_XOR(t_a, ad_out);
    }
    // last AD block
    // copy partial AD block
    for(unsigned int i=0; i< adlen - EEVEE_BLOCKSIZE*n_ad_blocks; i++)
        ad_in[i] = ad[EEVEE_BLOCKSIZE*n_ad_blocks+i];
    if(!complete_block) {
        // add padding
        ad_in[adlen % EEVEE_BLOCKSIZE] = 0x80;
        for(unsigned int i=adlen % EEVEE_BLOCKSIZE +1; i<EEVEE_BLOCKSIZE; i++)
            ad_in[i] = 0;
    }
    tweakey[EEVEE_TKSIZE-2] = 0;
    tweakey[EEVEE_TKSIZE-1] = no_m + (complete_block) ? 0 : 2;
    EEVEE_INIT_VAR_TKS_ONE_LEG(tks2, tweakey);
    EEVEE_FK_FORWARD(tks1, tks2, ad_out, NULL, ad_in);

    #ifdef EEVEE_VERBOSE
    printf("\nAD Block %lld\n", n_ad_blocks);
    printf("In: ");
    print_n(ad_in, EEVEE_BLOCKSIZE);
    printf("\nTK: ");
    print_n(tweakey, EEVEE_TKSIZE);
    printf("\nOut: ");
    print_n(ad_out, EEVEE_BLOCKSIZE);
    printf("\n");
    #endif

    EEVEE_BLOCK_XOR(t_a, ad_out);
}

int EEVEE_PREFIX(_encrypt)(unsigned char *c, unsigned char *tag, const unsigned char *m, unsigned long long mlen, const unsigned char *ad, unsigned long long adlen, const unsigned char *npub, const unsigned char *k) {
    if(mlen > EEVEE_MAX_MESSAGE_LENGTH) {
        printf("Message too long!\n");
        return 1;
    }
    if(adlen > EEVEE_MAX_MESSAGE_LENGTH) {
        printf("AD too long!\n");
        return 1;
    }
    if(adlen == 0 && mlen == 0) {
      return 1;
    }

    unsigned char tweakey[EEVEE_TKSIZE];
    EEVEE_TKS_T tks1, tks2;

    // load key
    memcpy(tweakey, k, EEVEE_KEYSIZE);
    // load nonce (EEVEE_NONCESIZE byte)
    memcpy(tweakey + EEVEE_KEYSIZE, npub, EEVEE_NONCESIZE);

    EEVEE_INIT_FIXED_TKS(&tks1, tweakey);

    unsigned char t_a[EEVEE_BLOCKSIZE];
    memset(t_a, 0, EEVEE_BLOCKSIZE);

    // AD
    if(adlen > 0)
      EEVEE_PREFIX(_ad)(t_a, tweakey, &tks1, &tks2, ad, adlen, mlen == 0);

    #ifdef EEVEE_VERBOSE
    printf("\nT_A after AD: ");
    print_n(t_a, EEVEE_BLOCKSIZE);
    printf("\n");
    #endif

    // message
    if(mlen == 0) {
        // output T_a as tag
        memcpy(tag, t_a, EEVEE_BLOCKSIZE);
        return 0;
    }
    unsigned char c1[EEVEE_BLOCKSIZE];
    unsigned char m_in[EEVEE_BLOCKSIZE];
    memcpy(c1, t_a, EEVEE_BLOCKSIZE);

    unsigned long long n_m_blocks = mlen/EEVEE_BLOCKSIZE;
    bool complete_block = mlen % EEVEE_BLOCKSIZE == 0;
    if(complete_block) {
        // complete block case
        n_m_blocks -= 1;
    }
    for(unsigned long long i=0; i<n_m_blocks; i++) {
        // set tweak
        // 15 bit counter:  1 | i_14 | i_13 | ... | i_0
        tweakey[EEVEE_TKSIZE-2] = (((i+2) >> 8) & 0x7F) | 0x80;
        tweakey[EEVEE_TKSIZE-1] = (i+2) & 0xFF;

        memcpy(m_in, m + i*EEVEE_BLOCKSIZE, EEVEE_BLOCKSIZE);
        EEVEE_BLOCK_XOR(m_in, c1);

        EEVEE_INIT_VAR_TKS_TWO_LEG(&tks2, tweakey);
        EEVEE_FK_FORWARD(&tks1, &tks2, &c[EEVEE_BLOCKSIZE*i], c1, m_in);

        #ifdef EEVEE_VERBOSE
        printf("\nM Block %lld\nm_in: ", i);
        print_n(m_in, EEVEE_BLOCKSIZE);
        printf("\nTK: ");
        print_n(tweakey, EEVEE_TKSIZE);
        printf("\nC0: ");
        print_n(&c[8*i], EEVEE_BLOCKSIZE);
        printf("\nC1: ");
        print_n(c1, EEVEE_BLOCKSIZE);
        #endif

        EEVEE_BLOCK_XOR(t_a, c1);

        #ifdef EEVEE_VERBOSE
        printf("\nT_A: ");
        print_n(t_a, EEVEE_BLOCKSIZE);
        printf("\n");
        #endif
    }

    // last message block
    // copy partial message
    memcpy(m_in, m + n_m_blocks * EEVEE_BLOCKSIZE, mlen - EEVEE_BLOCKSIZE*n_m_blocks);
    if(!complete_block) {
        // add padding
        m_in[mlen % EEVEE_BLOCKSIZE] = 0x80;
        for(unsigned int i=mlen%EEVEE_BLOCKSIZE+1; i<EEVEE_BLOCKSIZE; i++)
            m_in[i] = 0;
    }
    EEVEE_BLOCK_XOR(m_in, t_a);
    // set tweak to 1 | 0 | ... | if(complete_block) 0 else 1
    tweakey[EEVEE_TKSIZE-2] = 0x80;
    tweakey[EEVEE_TKSIZE-1] = (complete_block) ? 0 : 1;

    unsigned char last_block[EEVEE_BLOCKSIZE];
    unsigned int last_block_length = (complete_block) ? EEVEE_BLOCKSIZE : mlen % EEVEE_BLOCKSIZE;
    EEVEE_INIT_VAR_TKS_TWO_LEG(&tks2, tweakey);
    EEVEE_FK_FORWARD(&tks1, &tks2, tag, last_block, m_in);
    for(unsigned int i=0; i<last_block_length; i++)
            c[EEVEE_BLOCKSIZE*n_m_blocks + i] = m_in[i] ^ t_a[i] ^ last_block[i];

    #ifdef EEVEE_VERBOSE
    printf("\nM Block %lld\nm_in: ", n_m_blocks);
    print_n(m_in, EEVEE_BLOCKSIZE);
    printf("\nTK: ");
    print_n(tweakey, EEVEE_TKSIZE);
    printf("\nTag: ");
    print_n(tag, EEVEE_BLOCKSIZE);
    printf("\nC1*: ");
    print_n(last_block, last_block_length);
    printf("\nC*: ");
    print_n(&c[EEVEE_BLOCKSIZE*n_m_blocks], last_block_length);
    printf("\n");
    #endif
    return 0;
}

int EEVEE_PREFIX(_decrypt)(unsigned char *m, const unsigned char *c, unsigned long long clen, const unsigned char *tag, const unsigned char *ad, unsigned long long adlen, const unsigned char *npub, const unsigned char *k) {
    if(clen > EEVEE_MAX_MESSAGE_LENGTH) {
        printf("Ciphertext too long!\n");
        return 1;
    }
    if(adlen > EEVEE_MAX_MESSAGE_LENGTH) {
        printf("AD too long!\n");
        return 1;
    }
    if(adlen == 0 && clen == 0) {
      return 1;
    }

    unsigned char tweakey[EEVEE_TKSIZE];
    EEVEE_TKS_T tks1, tks2;
    unsigned char t_a[EEVEE_BLOCKSIZE];
    memset(t_a, 0, EEVEE_BLOCKSIZE);

    // load key
    memcpy(tweakey, k, EEVEE_KEYSIZE);
    // load nonce (EEVEE_NONCESIZE byte)
    memcpy(tweakey + EEVEE_KEYSIZE, npub, EEVEE_NONCESIZE);

    EEVEE_INIT_FIXED_TKS(&tks1, tweakey);

    // AD
    if(adlen > 0)
      EEVEE_PREFIX(_ad)(t_a, tweakey, &tks1, &tks2, ad, adlen, clen == 0);

    #ifdef EEVEE_VERBOSE
    printf("\nT_A after AD: ");
    print_n(t_a, EEVEE_BLOCKSIZE);
    printf("\n");
    #endif

    if(clen == 0) {
        // no ciphertext, just a tag
        // check tag (THIS IS NOT CONSTANT TIME!), see commented code below for constant time implementation
        for(unsigned int i=0; i<EEVEE_BLOCKSIZE; i++) {
            if(tag[i] != t_a[i]) return 1;
        }
        return 0;
        /*int fail = 0;
        for(unsigned int i=0; i<EEVEE_BLOCKSIZE; i++)
            fail |= tag[i] != t_a[i];
        return fail;*/
    }

    unsigned char c1_out[EEVEE_BLOCKSIZE];
    unsigned char c1_old[EEVEE_BLOCKSIZE];
    for(unsigned int i=0; i<EEVEE_BLOCKSIZE; i++)
        c1_old[i] = t_a[i];
    long long n_m_blocks = clen/EEVEE_BLOCKSIZE;
    bool complete_block = clen % EEVEE_BLOCKSIZE == 0;
    if(complete_block) {
        n_m_blocks -= 1;
    }

    for(long long i=0; i<n_m_blocks; i++) {
        // set tweak
        // 15 bit counter:  1 | i_14 | i_13 | ... | i_0
        tweakey[EEVEE_TKSIZE-2] = (((i+2) >> 8) & 0x7F) | 0x80;
        tweakey[EEVEE_TKSIZE-1] = (i+2) & 0xFF;
        EEVEE_INIT_VAR_TKS_TWO_LEG(&tks2, tweakey);
        EEVEE_FK_INVERT(&tks1, &tks2, &m[EEVEE_BLOCKSIZE*i], c1_out, &c[EEVEE_BLOCKSIZE*i]);
        // forkInvert(&m[EEVEE_BLOCKSIZE*i], c1_out, &c[EEVEE_BLOCKSIZE*i], tweakey, 0, INV_BOTH);

        #ifdef EEVEE_VERBOSE
        printf("\nC Block %lld\n", i);
        printf("In: ");
        print_n(&c[EEVEE_BLOCKSIZE*i], EEVEE_BLOCKSIZE);
        printf("\nTK: ");
        print_n(tweakey, EEVEE_TKSIZE);
        printf("\nM' Block %lld: ", i);
        print_n(&m[EEVEE_BLOCKSIZE*i], EEVEE_BLOCKSIZE);
        printf("\nC1 out: ");
        print_n(c1_out, EEVEE_BLOCKSIZE);
        #endif

        EEVEE_BLOCK_XOR(m + i*EEVEE_BLOCKSIZE, c1_old);
        memcpy(c1_old, c1_out, EEVEE_BLOCKSIZE);
        EEVEE_BLOCK_XOR(t_a, c1_out);

        #ifdef EEVEE_VERBOSE
        printf("\nM Block %lld: ", i);
        print_n(&m[EEVEE_BLOCKSIZE*i], EEVEE_BLOCKSIZE);
        printf("\n");
        #endif
    }

    // last message block
    // set tweak to 1 | 0 | ... | if(complete_block) 0 else 1
    tweakey[EEVEE_TKSIZE-2] = 0x80;
    tweakey[EEVEE_TKSIZE-1] = (complete_block) ? 0 : 1;
    unsigned int last_block_length = (complete_block) ? EEVEE_BLOCKSIZE : clen % EEVEE_BLOCKSIZE;
    EEVEE_INIT_VAR_TKS_TWO_LEG(&tks2, tweakey);
    EEVEE_FK_INVERT(&tks1, &tks2, c1_old, c1_out, tag);
    // forkInvert(c1_old, c1_out, tag, tweakey, 0, INV_BOTH);

    #ifdef EEVEE_VERBOSE
    printf("\nC Block %lld\n", n_m_blocks);
    printf("In: ");
    print_n(tag, EEVEE_BLOCKSIZE);
    printf("\nTK: ");
    print_n(tweakey, EEVEE_TKSIZE);
    printf("\nC0 out: ");
    print_n(c1_old, EEVEE_BLOCKSIZE);
    printf("\nC1 out: ");
    print_n(c1_out, EEVEE_BLOCKSIZE);
    printf("\n");
    #endif

    EEVEE_BLOCK_XOR(c1_old, t_a);

    // output partial message
    for(unsigned int i=0; i<last_block_length; i++)
        m[EEVEE_BLOCKSIZE*n_m_blocks + i] = c1_old[i];

    #ifdef EEVEE_VERBOSE
    printf("M*: ");
    print_n(c1_old, last_block_length);
    printf("\n");
    // check tag (THIS IS NOT CONSTANT TIME!), see commented code below for a constant time implementation
    printf("\nCheck padding: ");
    print_n(&c1_old[last_block_length], EEVEE_BLOCKSIZE-last_block_length);
    #endif

    if(last_block_length < EEVEE_BLOCKSIZE && c1_old[last_block_length] != 0x80) return 1;
    for(unsigned int i=last_block_length +1; i<EEVEE_BLOCKSIZE; i++) {
        if(c1_old[i] != 0x00) return 1;
    }

    #ifdef EEVEE_VERBOSE
    printf("\nC*': ");
    for(unsigned int i=0; i< last_block_length; i++)
        printf("%02x", c1_out[i] ^ c1_old[i]);
    printf(" == ");
    for(unsigned int i=0; i< last_block_length; i++)
        printf("%02x", c[EEVEE_BLOCKSIZE*n_m_blocks+i]);
    printf("\n");
    #endif

    for(unsigned int i=0; i< last_block_length; i++) {
        if((c1_out[i] ^ c1_old[i]) != c[EEVEE_BLOCKSIZE*n_m_blocks+i]) return 1;
    }
    return 0;
    /**
    int fail = 0;
    if(last_block_length < EEVEE_BLOCKSIZE)
        fail |= c1_old[EEVEE_BLOCKSIZE*n_m_blocks + last_block_length] != 0x80;
    for(unsigned int i=last_block_length +1; i<EEVEE_BLOCKSIZE; i++) {
        fail |= c1_old[i] != 0x00;
    }

    printf("C*': ");
    for(unsigned int i=0; i< last_block_length; i++)
        printf("%02x", c1_out[i] ^ c1_old[i]);
    printf(" == ");
    for(unsigned int i=0; i< last_block_length; i++)
        printf("%02x", c[EEVEE_BLOCKSIZE*n_m_blocks+i]);
    printf("\n");
    for(unsigned int i=0; i< last_block_length; i++) {
        fail |= (c1_out[i] ^ c1_old[i]) != c[EEVEE_BLOCKSIZE*n_m_blocks+i];
    }
    return fail;
    **/
}

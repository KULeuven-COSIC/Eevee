#include "espeon.h"
#include "eevee_common.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#ifdef EEVEE_VERBOSE
static void print_n(const unsigned char *buf, unsigned int n) {
    for(unsigned int i=0; i<n; i++)
        printf("%02x", buf[i]);
}
#endif

// 16-bit counter => 2^16 * 16 byte
#define ESPEON_MAX_MESSAGE_LENGTH 1048576

#define ESPEON_BLOCKSIZE 16
#define ESPEON384_TKSIZE 48
#define ESPEON256_TKSIZE 32
#define ESPEON_KEYSIZE 16
#define ESPEON384_NONCESIZE 16
#define ESPEON256_NONCESIZE 13

#define ESPEON384_TKS_ONE_LEG_ROUNDS (FORKSKINNY_128_384_ROUNDS_BEFORE + FORKSKINNY_128_384_ROUNDS_AFTER)
#define ESPEON384_TKS_TWO_LEG_ROUNDS (FORKSKINNY_128_384_ROUNDS_BEFORE + 2*FORKSKINNY_128_384_ROUNDS_AFTER)

#define ESPEON256_TKS_ONE_LEG_ROUNDS (FORKSKINNY_128_256_ROUNDS_BEFORE + FORKSKINNY_128_256_ROUNDS_AFTER)
#define ESPEON256_TKS_TWO_LEG_ROUNDS (FORKSKINNY_128_256_ROUNDS_BEFORE + 2*FORKSKINNY_128_256_ROUNDS_AFTER)

static void espeon_forkskinny_128_384_ad(unsigned char *t_a, unsigned char *tweakey,
  eevee_forkskinny_128_384_tks_t *tks1, eevee_forkskinny_128_384_tks_t *tks2,
  const eevee_forkskinny_128_384_tks_t *tks3,
  const unsigned char *npub,
  const unsigned char *ad, unsigned long long adlen, unsigned char no_m) {
    assert(adlen > 0);
    unsigned char ad_in[ESPEON_BLOCKSIZE];
    unsigned char ad_out[ESPEON_BLOCKSIZE];
    unsigned long long n_ad_blocks = adlen/ESPEON_BLOCKSIZE;
    bool complete_block = adlen % ESPEON_BLOCKSIZE == 0;
    if(complete_block) {
        // complete block case
        n_ad_blocks -= 1;
    }

    memcpy(tweakey + ESPEON_KEYSIZE, npub, ESPEON384_NONCESIZE);
    memset(tweakey + ESPEON_KEYSIZE + ESPEON384_NONCESIZE, 0, ESPEON384_TKSIZE - ESPEON_KEYSIZE - ESPEON384_NONCESIZE);
    eevee_common_forkskinny_128_384_init_tk2(tks2, tweakey + ESPEON_KEYSIZE, ESPEON384_TKS_ONE_LEG_ROUNDS);
    tweakey[ESPEON384_TKSIZE-1] = 0x00;

    for(unsigned long long i=0; i< n_ad_blocks; i++) {
        // set tweak
        // 16 bit counter:  i_15 | i_14 | ... | i_0
        tweakey[ESPEON384_TKSIZE-3] = ((i+4) >> 8) & 0xFF;
        tweakey[ESPEON384_TKSIZE-2] = (i+4) & 0xFF;

        memcpy(ad_in, ad + i*ESPEON_BLOCKSIZE, ESPEON_BLOCKSIZE);
        eevee_common_forkskinny_128_384_init_tk1(tks1, tweakey + ESPEON_KEYSIZE + ESPEON384_NONCESIZE, ESPEON384_TKS_ONE_LEG_ROUNDS);
        eevee_common_forkskinny_128_384_encrypt(tks1, tks2, tks3, NULL, ad_out, ad_in);
        // JOLTEON_FK_FORWARD(tks1, tks2, ad_out, NULL, ad_in);
        #ifdef EEVEE_VERBOSE
        printf("\nAD Block %lld\n", i);
        printf("In: ");
        print_n(ad_in, ESPEON_BLOCKSIZE);
        printf("\nTK: ");
        print_n(tweakey, ESPEON_BLOCKSIZE);
        printf("\nOut: ");
        print_n(ad_out, ESPEON_BLOCKSIZE);
        printf("\n");
        #endif
        block_xor128(t_a, ad_out);
    }
    // last AD block
    // copy partial AD block
    if(adlen > 0) {
      for(unsigned int i=0; i< adlen - ESPEON_BLOCKSIZE*n_ad_blocks; i++)
          ad_in[i] = ad[ESPEON_BLOCKSIZE*n_ad_blocks+i];
    }
    if(!complete_block) {
        // add padding
        ad_in[adlen % ESPEON_BLOCKSIZE] = 0x80;
        for(unsigned int i=adlen % ESPEON_BLOCKSIZE +1; i<ESPEON_BLOCKSIZE; i++)
            ad_in[i] = 0;
    }
    tweakey[ESPEON384_TKSIZE-3] = 0;
    tweakey[ESPEON384_TKSIZE-2] = (no_m << 2) + (complete_block) ? 0 : 0xc;
    eevee_common_forkskinny_128_384_init_tk1(tks1, tweakey + ESPEON_KEYSIZE + ESPEON384_NONCESIZE, ESPEON384_TKS_ONE_LEG_ROUNDS);
    eevee_common_forkskinny_128_384_encrypt(tks1, tks2, tks3, NULL, ad_out, ad_in);

    #ifdef EEVEE_VERBOSE
    printf("\nAD Block %lld\n", n_ad_blocks);
    printf("In: ");
    print_n(ad_in, ESPEON_BLOCKSIZE);
    printf("\nTK: ");
    print_n(tweakey, ESPEON_BLOCKSIZE);
    printf("\nOut: ");
    print_n(ad_out, ESPEON_BLOCKSIZE);
    printf("\n");
    #endif

    block_xor128(t_a, ad_out);
}

int espeon_forkskinny_128_384_encrypt(unsigned char *c, unsigned char *tag, const unsigned char *m, unsigned long long mlen, const unsigned char *ad, unsigned long long adlen, const unsigned char *npub, const unsigned char *k) {
    if(adlen > ESPEON_MAX_MESSAGE_LENGTH) {
        printf("AD too long!\n");
        return 1;
    }
    if(adlen == 0 && mlen == 0) {
      return 1;
    }

    unsigned char tweakey[ESPEON384_TKSIZE];
    unsigned char t_a[ESPEON_BLOCKSIZE];
    memset(t_a, 0, ESPEON_BLOCKSIZE);

    // load key
    memcpy(tweakey, k, ESPEON_KEYSIZE);

    eevee_forkskinny_128_384_tks_t tks1, tks2, tks3;
    eevee_common_forkskinny_128_384_init_tk3(&tks3, tweakey, ESPEON384_TKS_TWO_LEG_ROUNDS);

    // AD
    if(adlen > 0)
      espeon_forkskinny_128_384_ad(t_a, tweakey, &tks1, &tks2, &tks3, npub, ad, adlen, mlen == 0);

    #ifdef EEVEE_VERBOSE
    printf("\nT_A after AD: ");
    print_n(t_a, ESPEON_BLOCKSIZE);
    printf("\n");
    #endif

    // message
    if(mlen == 0) {
        // output T_a as tag
        memcpy(tag, t_a, ESPEON_BLOCKSIZE);
        return 0;
    }
    unsigned char m_in[ESPEON_BLOCKSIZE];

    unsigned long long n_m_blocks = mlen/ESPEON_BLOCKSIZE;
    bool complete_block = mlen % ESPEON_BLOCKSIZE == 0;
    if(complete_block) {
        // complete block case
        n_m_blocks -= 1;
    }


    const unsigned char *input = t_a;
    unsigned char *output = c;
    memcpy(tweakey + ESPEON_KEYSIZE, npub, ESPEON384_NONCESIZE);
    memset(tweakey + ESPEON_KEYSIZE + ESPEON384_NONCESIZE, 0, ESPEON384_TKSIZE - ESPEON_KEYSIZE - ESPEON384_NONCESIZE-2);
    tweakey[ESPEON384_TKSIZE-2] = 0x4;
    tweakey[ESPEON384_TKSIZE-1] = 0x00;
    for(unsigned long long i=0; i<n_m_blocks; i++) {
        if(i == 0) {
          block_xor128(t_a, m);
        }else{
          block_xor128(t_a, input);
        }

        eevee_common_forkskinny_128_384_init_tk1(&tks1, tweakey + ESPEON_KEYSIZE, ESPEON384_TKS_ONE_LEG_ROUNDS);
        eevee_common_forkskinny_128_384_init_tk2(&tks2, tweakey + ESPEON_KEYSIZE + ESPEON_BLOCKSIZE, ESPEON384_TKS_ONE_LEG_ROUNDS);
        eevee_common_forkskinny_128_384_encrypt(&tks1, &tks2, &tks3, NULL, output, input);
        #ifdef EEVEE_VERBOSE
        printf("M Block %lld\nIn: ", i);
        print_n(input, ESPEON_BLOCKSIZE);
        printf("\nTK ");
        print_n(tweakey, ESPEON384_TKSIZE);
        printf("\nOut: ");
        print_n(output, ESPEON_BLOCKSIZE);
        printf("\nT_A: ");
        print_n(t_a, ESPEON_BLOCKSIZE);
        printf("\n");
        #endif

        if(i==0) {
          input = m;
          memcpy(tweakey + ESPEON_KEYSIZE, output, ESPEON_BLOCKSIZE);
          memcpy(tweakey + ESPEON_KEYSIZE + ESPEON_BLOCKSIZE, npub, ESPEON384_TKSIZE - ESPEON_KEYSIZE - ESPEON384_NONCESIZE-1);
          tweakey[ESPEON384_TKSIZE-1] = 0x01;
        } else {
          memcpy(tweakey + ESPEON_KEYSIZE, output, ESPEON_BLOCKSIZE);
          memcpy(tweakey + ESPEON_KEYSIZE + ESPEON_BLOCKSIZE, output - ESPEON_BLOCKSIZE, ESPEON_BLOCKSIZE-1);
        }

        input += ESPEON_BLOCKSIZE;
        output += ESPEON_BLOCKSIZE;
    }

    // last message block
    // copy partial message
    memcpy(m_in, m + n_m_blocks * ESPEON_BLOCKSIZE, mlen - ESPEON_BLOCKSIZE*n_m_blocks);
    if(!complete_block) {
        // add padding
        m_in[mlen % ESPEON_BLOCKSIZE] = 0x80;
        for(unsigned int i=mlen%ESPEON_BLOCKSIZE+1; i<ESPEON_BLOCKSIZE; i++)
            m_in[i] = 0;
    }

    block_xor128(m_in, t_a);

    // set tweak to  0 | ... | if(complete_block) 10 else 11
    tweakey[ESPEON384_TKSIZE-1] = (complete_block) ? 0x2 : 0x3;

    unsigned char last_block[ESPEON_BLOCKSIZE];
    unsigned int last_block_length = (complete_block) ? ESPEON_BLOCKSIZE : mlen % ESPEON_BLOCKSIZE;
    eevee_common_forkskinny_128_384_init_tk1(&tks1, tweakey + ESPEON_KEYSIZE, ESPEON384_TKS_TWO_LEG_ROUNDS);
    eevee_common_forkskinny_128_384_init_tk2(&tks2, tweakey + ESPEON_KEYSIZE + ESPEON_BLOCKSIZE, ESPEON384_TKS_TWO_LEG_ROUNDS);
    eevee_common_forkskinny_128_384_encrypt(&tks1, &tks2, &tks3, last_block, tag, m_in);

    for(unsigned int i=0; i<last_block_length; i++)
            c[ESPEON_BLOCKSIZE*n_m_blocks + i] = m[n_m_blocks * ESPEON_BLOCKSIZE + i] ^ last_block[i];

    #ifdef EEVEE_VERBOSE
    printf("\nM Block %lld\nm_in: ", n_m_blocks);
    print_n(m_in, ESPEON_BLOCKSIZE);
    printf("\nTK: ");
    print_n(tweakey, ESPEON384_TKSIZE);
    printf("\nTag: ");
    print_n(tag, ESPEON_BLOCKSIZE);
    printf("\nC1*: ");
    print_n(last_block, last_block_length);
    printf("\nC*: ");
    print_n(&c[ESPEON_BLOCKSIZE*n_m_blocks], last_block_length);
    printf("\n");
    #endif

    return 0;
}

int espeon_forkskinny_128_384_decrypt(unsigned char *m, const unsigned char *c, unsigned long long clen, const unsigned char *tag, const unsigned char *ad, unsigned long long adlen, const unsigned char *npub, const unsigned char *k) {
    if(adlen > ESPEON_MAX_MESSAGE_LENGTH) {
        printf("AD too long!\n");
        return 1;
    }
    if(adlen == 0 && clen == 0) {
      return 1;
    }

    unsigned char tweakey[ESPEON384_TKSIZE];
    unsigned char t_a[ESPEON_BLOCKSIZE];
    memset(t_a, 0, ESPEON_BLOCKSIZE);

    // load key
    memcpy(tweakey, k, ESPEON_KEYSIZE);

    eevee_forkskinny_128_384_tks_t tks1, tks2, tks3;
    eevee_common_forkskinny_128_384_init_tk3(&tks3, tweakey, ESPEON384_TKS_TWO_LEG_ROUNDS);

    // AD
    if(adlen > 0)
      espeon_forkskinny_128_384_ad(t_a, tweakey, &tks1, &tks2, &tks3, npub, ad, adlen, clen == 0);

    #ifdef EEVEE_VERBOSE
    printf("\nT_A after AD: ");
    print_n(t_a, ESPEON_BLOCKSIZE);
    printf("\n");
    #endif

    if(clen == 0) {
        // no ciphertext, just a tag
        // check tag (THIS IS NOT CONSTANT TIME!), see commented code below for constant time implementation
        for(unsigned int i=0; i<ESPEON_BLOCKSIZE; i++) {
            if(tag[i] != t_a[i]) return 1;
        }
        return 0;
        /*int fail = 0;
        for(unsigned int i=0; i<ESPEON_BLOCKSIZE; i++)
            fail |= tag[i] != t_a[i];
        return fail;*/
    }

    long long n_m_blocks = clen/ESPEON_BLOCKSIZE;
    bool complete_block = clen % ESPEON_BLOCKSIZE == 0;
    if(complete_block) {
        n_m_blocks -= 1;
    }

    tweakey[ESPEON384_TKSIZE-1] = 0x01;
    const unsigned char *input = c;
    unsigned char *output = m;
    memcpy(tweakey + ESPEON_KEYSIZE, npub, ESPEON384_NONCESIZE);
    memset(tweakey + ESPEON_KEYSIZE + ESPEON384_NONCESIZE, 0, ESPEON384_TKSIZE - ESPEON_KEYSIZE - ESPEON384_NONCESIZE-2);
    tweakey[ESPEON384_TKSIZE-2] = 0x4;
    tweakey[ESPEON384_TKSIZE-1] = 0x00;
    for(long long i=0; i<n_m_blocks; i++) {

      eevee_common_forkskinny_128_384_init_tk1(&tks1, tweakey + ESPEON_KEYSIZE, ESPEON384_TKS_ONE_LEG_ROUNDS);
      eevee_common_forkskinny_128_384_init_tk2(&tks2, tweakey + ESPEON_KEYSIZE + ESPEON_BLOCKSIZE, ESPEON384_TKS_ONE_LEG_ROUNDS);
      eevee_common_forkskinny_128_384_decrypt(&tks1, &tks2, &tks3, NULL, output, input);

      if(i == 0) {
        block_xor128(output, t_a);
      }
      block_xor128(t_a, output);

      #ifdef EEVEE_VERBOSE
      printf("C Block %lld\nIn: ", i);
      print_n(input, ESPEON_BLOCKSIZE);
      printf("\nTK ");
      print_n(tweakey, ESPEON384_TKSIZE);
      printf("\nOut: ");
      print_n(output, ESPEON_BLOCKSIZE);
      printf("\nT_A: ");
      print_n(t_a, ESPEON_BLOCKSIZE);
      printf("\n");
      #endif

      if(i==0) {
        memcpy(tweakey + ESPEON_KEYSIZE, input, ESPEON_BLOCKSIZE);
        memcpy(tweakey + ESPEON_KEYSIZE + ESPEON_BLOCKSIZE, npub, ESPEON384_TKSIZE - ESPEON_KEYSIZE - ESPEON384_NONCESIZE-1);
        tweakey[ESPEON384_TKSIZE-1] = 0x01;
      } else {
        memcpy(tweakey + ESPEON_KEYSIZE, input, ESPEON_BLOCKSIZE);
        memcpy(tweakey + ESPEON_KEYSIZE + ESPEON_BLOCKSIZE, input - ESPEON_BLOCKSIZE, ESPEON_BLOCKSIZE-1);
      }

      input += ESPEON_BLOCKSIZE;
      output += ESPEON_BLOCKSIZE;
    }

    // last message block
    // set tweak to C_{i-2} || C_{i-1} | if(complete_block) 10 else 11
    tweakey[ESPEON384_TKSIZE-1] = (complete_block) ? 0x2 : 0x3;
    unsigned int last_block_length = (complete_block) ? ESPEON_BLOCKSIZE : clen % ESPEON_BLOCKSIZE;

    unsigned char out[ESPEON_BLOCKSIZE];
    unsigned char c_out[ESPEON_BLOCKSIZE];
    eevee_common_forkskinny_128_384_init_tk1(&tks1, tweakey + ESPEON_KEYSIZE, ESPEON384_TKS_TWO_LEG_ROUNDS);
    eevee_common_forkskinny_128_384_init_tk2(&tks2, tweakey + ESPEON_KEYSIZE + ESPEON_BLOCKSIZE, ESPEON384_TKS_TWO_LEG_ROUNDS);
    eevee_common_forkskinny_128_384_decrypt(&tks1, &tks2, &tks3, c_out, out, tag);

    #ifdef EEVEE_VERBOSE
    printf("\nC Block %lld\n", n_m_blocks);
    printf("In: ");
    print_n(tag, ESPEON_BLOCKSIZE);
    printf("\nTK: ");
    print_n(tweakey, ESPEON384_TKSIZE);
    printf("\nM*': ");
    print_n(out, ESPEON_BLOCKSIZE);
    printf("\nC*': ");
    print_n(c_out, ESPEON_BLOCKSIZE);
    printf("\n");
    #endif

    block_xor128(out, t_a);

    // output partial message
    memcpy(m + n_m_blocks*ESPEON_BLOCKSIZE, out, last_block_length);

    #ifdef EEVEE_VERBOSE
    printf("M*: ");
    print_n(m + n_m_blocks*ESPEON_BLOCKSIZE, last_block_length);
    printf("\n");
    // check tag (THIS IS NOT CONSTANT TIME!), see commented code below for a constant time implementation
    printf("\nCheck padding: ");
    print_n(&out[last_block_length], ESPEON_BLOCKSIZE-last_block_length);
    printf("\n");
    #endif

    if(last_block_length < ESPEON_BLOCKSIZE && out[last_block_length] != 0x80) return 1;
    for(unsigned int i=last_block_length +1; i<ESPEON_BLOCKSIZE; i++) {
        if(out[i] != 0x00) return 1;
    }

    #ifdef EEVEE_VERBOSE
    printf("\nC*': ");
    for(unsigned int i=0; i< last_block_length; i++)
        printf("%02x", out[i] ^ c_out[i]);
    printf(" == ");
    for(unsigned int i=0; i< last_block_length; i++)
        printf("%02x", c[ESPEON_BLOCKSIZE*n_m_blocks+i]);
    printf("\n");
    #endif

    for(unsigned int i=0; i< last_block_length; i++) {
        if((out[i] ^ c_out[i]) != c[ESPEON_BLOCKSIZE*n_m_blocks+i]) return 1;
    }
    return 0;
    /**
    int fail = 0;
    if(last_block_length < ESPEON_BLOCKSIZE)
        fail |= c1_old[ESPEON_BLOCKSIZE*n_m_blocks + last_block_length] != 0x80;
    for(unsigned int i=last_block_length +1; i<ESPEON_BLOCKSIZE; i++) {
        fail |= c1_old[i] != 0x00;
    }

    printf("C*': ");
    for(unsigned int i=0; i< last_block_length; i++)
        printf("%02x", c1_out[i] ^ c1_old[i]);
    printf(" == ");
    for(unsigned int i=0; i< last_block_length; i++)
        printf("%02x", c[ESPEON_BLOCKSIZE*n_m_blocks+i]);
    printf("\n");
    for(unsigned int i=0; i< last_block_length; i++) {
        fail |= (c1_out[i] ^ c1_old[i]) != c[ESPEON_BLOCKSIZE*n_m_blocks+i];
    }
    return fail;
    **/
}

#undef ESPEON384_TKSIZE
#undef ESPEON384_NONCESIZE

#undef ESPEON384_TKS_ONE_LEG_ROUNDS
#undef ESPEON384_TKS_TWO_LEG_ROUNDS

static void espeon_forkskinny_128_256_ad(unsigned char *t_a, unsigned char *tweakey,
  eevee_forkskinny_128_256_tks_t *tks1, const eevee_forkskinny_128_256_tks_t *tks2,
  const unsigned char *npub,
  const unsigned char *ad, unsigned long long adlen, unsigned char no_m) {
    assert(adlen > 0);
    unsigned char ad_in[ESPEON_BLOCKSIZE];
    unsigned char ad_out[ESPEON_BLOCKSIZE];
    unsigned long long n_ad_blocks = adlen/ESPEON_BLOCKSIZE;
    bool complete_block = adlen % ESPEON_BLOCKSIZE == 0;
    if(complete_block) {
        // complete block case
        n_ad_blocks -= 1;
    }

    memcpy(tweakey + ESPEON_KEYSIZE, npub, ESPEON256_NONCESIZE);
    // memset(tweakey + ESPEON_KEYSIZE + ESPEON_NONCESIZE, 0, ESPEON256_TKSIZE - ESPEON_KEYSIZE - ESPEON_NONCESIZE);
    // forkskinny_128_256_init_tks_part(tks1, tweakey + ESPEON_KEYSIZE, ESPEON256_TKS_ONE_LEG_ROUNDS, 1);
    tweakey[ESPEON256_TKSIZE-1] = 0x00;

    for(unsigned long long i=0; i< n_ad_blocks; i++) {
        // set tweak
        // 16 bit counter:  i_15 | i_14 | ... | i_0
        tweakey[ESPEON256_TKSIZE-3] = ((i+4) >> 8) & 0xFF;
        tweakey[ESPEON256_TKSIZE-2] = (i+4) & 0xFF;

        memcpy(ad_in, ad + i*ESPEON_BLOCKSIZE, ESPEON_BLOCKSIZE);
        eevee_common_forkskinny_128_256_init_tk1(tks1, tweakey + ESPEON_KEYSIZE, ESPEON256_TKS_ONE_LEG_ROUNDS);
        eevee_common_forkskinny_128_256_encrypt(tks1, tks2, NULL, ad_out, ad_in);

        #ifdef EEVEE_VERBOSE
        printf("\nAD Block %lld\n", i);
        printf("In: ");
        print_n(ad_in, ESPEON_BLOCKSIZE);
        printf("\nTK: ");
        print_n(tweakey, ESPEON_BLOCKSIZE);
        printf("\nOut: ");
        print_n(ad_out, ESPEON_BLOCKSIZE);
        printf("\n");
        #endif
        block_xor128(t_a, ad_out);
    }
    // last AD block
    // copy partial AD block
    if(adlen > 0) {
      for(unsigned int i=0; i< adlen - ESPEON_BLOCKSIZE*n_ad_blocks; i++)
          ad_in[i] = ad[ESPEON_BLOCKSIZE*n_ad_blocks+i];
    }
    if(!complete_block) {
        // add padding
        ad_in[adlen % ESPEON_BLOCKSIZE] = 0x80;
        for(unsigned int i=adlen % ESPEON_BLOCKSIZE +1; i<ESPEON_BLOCKSIZE; i++)
            ad_in[i] = 0;
    }
    tweakey[ESPEON256_TKSIZE-3] = 0;
    tweakey[ESPEON256_TKSIZE-2] = (no_m << 2) + (complete_block) ? 0 : 0xc;
    eevee_common_forkskinny_128_256_init_tk1(tks1, tweakey + ESPEON_KEYSIZE, ESPEON256_TKS_ONE_LEG_ROUNDS);
    eevee_common_forkskinny_128_256_encrypt(tks1, tks2, NULL, ad_out, ad_in);

    #ifdef EEVEE_VERBOSE
    printf("\nAD Block %lld\n", n_ad_blocks);
    printf("In: ");
    print_n(ad_in, ESPEON_BLOCKSIZE);
    printf("\nTK: ");
    print_n(tweakey, ESPEON_BLOCKSIZE);
    printf("\nOut: ");
    print_n(ad_out, ESPEON_BLOCKSIZE);
    printf("\n");
    #endif

    block_xor128(t_a, ad_out);
}

int espeon_forkskinny_128_256_encrypt(unsigned char *c, unsigned char *tag, const unsigned char *m, unsigned long long mlen, const unsigned char *ad, unsigned long long adlen, const unsigned char *npub, const unsigned char *k) {
    if(adlen > ESPEON_MAX_MESSAGE_LENGTH) {
        printf("AD too long!\n");
        return 1;
    }
    if(adlen == 0 && mlen == 0) {
      return 1;
    }

    unsigned char tweakey[ESPEON256_TKSIZE];
    unsigned char t_a[ESPEON_BLOCKSIZE];
    memset(t_a, 0, ESPEON_BLOCKSIZE);

    // load key
    memcpy(tweakey, k, ESPEON_KEYSIZE);

    eevee_forkskinny_128_256_tks_t tks1, tks2;
    eevee_common_forkskinny_128_256_init_tk2(&tks2, tweakey, ESPEON256_TKS_TWO_LEG_ROUNDS);

    // AD
    if(adlen > 0)
      espeon_forkskinny_128_256_ad(t_a, tweakey, &tks1, &tks2, npub, ad, adlen, mlen == 0);

    #ifdef EEVEE_VERBOSE
    printf("\nT_A after AD: ");
    print_n(t_a, ESPEON_BLOCKSIZE);
    printf("\n");
    #endif

    // message
    if(mlen == 0) {
        // output T_a as tag
        memcpy(tag, t_a, ESPEON_BLOCKSIZE);
        return 0;
    }
    unsigned char m_in[ESPEON_BLOCKSIZE];

    unsigned long long n_m_blocks = mlen/ESPEON_BLOCKSIZE;
    bool complete_block = mlen % ESPEON_BLOCKSIZE == 0;
    if(complete_block) {
        // complete block case
        n_m_blocks -= 1;
    }


    const unsigned char *input = t_a;
    unsigned char *output = c;
    memcpy(tweakey + ESPEON_KEYSIZE, npub, ESPEON256_NONCESIZE);
    tweakey[ESPEON256_TKSIZE-3] = 0x0;
    tweakey[ESPEON256_TKSIZE-2] = 0x0;
    tweakey[ESPEON256_TKSIZE-1] = 0x01;
    for(unsigned long long i=0; i<n_m_blocks; i++) {
        if(i == 0) {
          block_xor128(t_a, m);
        }else{
          block_xor128(t_a, input);
        }

        eevee_common_forkskinny_128_256_init_tk1(&tks1, tweakey + ESPEON_KEYSIZE, ESPEON256_TKS_ONE_LEG_ROUNDS);
        eevee_common_forkskinny_128_256_encrypt(&tks1, &tks2, NULL, output, input);
        #ifdef EEVEE_VERBOSE
        printf("M Block %lld\nIn: ", i);
        print_n(input, ESPEON_BLOCKSIZE);
        printf("\nTK ");
        print_n(tweakey, ESPEON256_TKSIZE);
        printf("\nOut: ");
        print_n(output, ESPEON_BLOCKSIZE);
        printf("\nT_A: ");
        print_n(t_a, ESPEON_BLOCKSIZE);
        printf("\n");
        #endif

        if(i==0) {
          input = m;
        }
        memcpy(tweakey + ESPEON_KEYSIZE, output, ESPEON_BLOCKSIZE-1);
        input += ESPEON_BLOCKSIZE;
        output += ESPEON_BLOCKSIZE;
    }

    // last message block
    // copy partial message
    memcpy(m_in, m + n_m_blocks * ESPEON_BLOCKSIZE, mlen - ESPEON_BLOCKSIZE*n_m_blocks);
    if(!complete_block) {
        // add padding
        m_in[mlen % ESPEON_BLOCKSIZE] = 0x80;
        for(unsigned int i=mlen%ESPEON_BLOCKSIZE+1; i<ESPEON_BLOCKSIZE; i++)
            m_in[i] = 0;
    }

    block_xor128(m_in, t_a);

    // set tweak to  0 | ... | if(complete_block) 10 else 11
    tweakey[ESPEON256_TKSIZE-1] = (complete_block) ? 0x2 : 0x3;

    unsigned char last_block[ESPEON_BLOCKSIZE];
    unsigned int last_block_length = (complete_block) ? ESPEON_BLOCKSIZE : mlen % ESPEON_BLOCKSIZE;
    eevee_common_forkskinny_128_256_init_tk1(&tks1, tweakey + ESPEON_KEYSIZE, ESPEON256_TKS_TWO_LEG_ROUNDS);
    eevee_common_forkskinny_128_256_encrypt(&tks1, &tks2, last_block, tag, m_in);

    for(unsigned int i=0; i<last_block_length; i++)
            c[ESPEON_BLOCKSIZE*n_m_blocks + i] = m[n_m_blocks * ESPEON_BLOCKSIZE + i] ^ last_block[i];

    #ifdef EEVEE_VERBOSE
    printf("\nM Block %lld\nm_in: ", n_m_blocks);
    print_n(m_in, ESPEON_BLOCKSIZE);
    printf("\nTK: ");
    print_n(tweakey, ESPEON256_TKSIZE);
    printf("\nTag: ");
    print_n(tag, ESPEON_BLOCKSIZE);
    printf("\nC1*: ");
    print_n(last_block, last_block_length);
    printf("\nC*: ");
    print_n(&c[ESPEON_BLOCKSIZE*n_m_blocks], last_block_length);
    printf("\n");
    #endif

    return 0;
}

int espeon_forkskinny_128_256_decrypt(unsigned char *m, const unsigned char *c, unsigned long long clen, const unsigned char *tag, const unsigned char *ad, unsigned long long adlen, const unsigned char *npub, const unsigned char *k) {
    if(adlen > ESPEON_MAX_MESSAGE_LENGTH) {
        printf("AD too long!\n");
        return 1;
    }
    if(adlen == 0 && clen == 0) {
      return 1;
    }

    unsigned char tweakey[ESPEON256_TKSIZE];
    unsigned char t_a[ESPEON_BLOCKSIZE];
    memset(t_a, 0, ESPEON_BLOCKSIZE);

    // load key
    memcpy(tweakey, k, ESPEON_KEYSIZE);

    eevee_forkskinny_128_256_tks_t tks1, tks2;
    eevee_common_forkskinny_128_256_init_tk2(&tks2, tweakey, ESPEON256_TKS_TWO_LEG_ROUNDS);

    // AD
    if(adlen > 0)
      espeon_forkskinny_128_256_ad(t_a, tweakey, &tks1, &tks2, npub, ad, adlen, clen == 0);

    #ifdef EEVEE_VERBOSE
    printf("\nT_A after AD: ");
    print_n(t_a, ESPEON_BLOCKSIZE);
    printf("\n");
    #endif

    if(clen == 0) {
        // no ciphertext, just a tag
        // check tag (THIS IS NOT CONSTANT TIME!), see commented code below for constant time implementation
        for(unsigned int i=0; i<ESPEON_BLOCKSIZE; i++) {
            if(tag[i] != t_a[i]) return 1;
        }
        return 0;
        /*int fail = 0;
        for(unsigned int i=0; i<ESPEON_BLOCKSIZE; i++)
            fail |= tag[i] != t_a[i];
        return fail;*/
    }

    long long n_m_blocks = clen/ESPEON_BLOCKSIZE;
    bool complete_block = clen % ESPEON_BLOCKSIZE == 0;
    if(complete_block) {
        n_m_blocks -= 1;
    }

    const unsigned char *input = c;
    unsigned char *output = m;
    memcpy(tweakey + ESPEON_KEYSIZE, npub, ESPEON256_NONCESIZE);
    tweakey[ESPEON256_TKSIZE-3] = 0x0;
    tweakey[ESPEON256_TKSIZE-2] = 0x0;
    tweakey[ESPEON256_TKSIZE-1] = 0x01;
    for(long long i=0; i<n_m_blocks; i++) {

      eevee_common_forkskinny_128_256_init_tk1(&tks1, tweakey + ESPEON_KEYSIZE, ESPEON256_TKS_ONE_LEG_ROUNDS);
      eevee_common_forkskinny_128_256_decrypt(&tks1, &tks2, NULL, output, input);

      if(i == 0) {
        block_xor128(output, t_a);
      }
      block_xor128(t_a, output);

      #ifdef EEVEE_VERBOSE
      printf("C Block %lld\nIn: ", i);
      print_n(input, ESPEON_BLOCKSIZE);
      printf("\nTK ");
      print_n(tweakey, ESPEON256_TKSIZE);
      printf("\nOut: ");
      print_n(output, ESPEON_BLOCKSIZE);
      printf("\nT_A: ");
      print_n(t_a, ESPEON_BLOCKSIZE);
      printf("\n");
      #endif

      memcpy(tweakey + ESPEON_KEYSIZE, input, ESPEON_BLOCKSIZE-1);

      input += ESPEON_BLOCKSIZE;
      output += ESPEON_BLOCKSIZE;
    }

    // last message block
    // set tweak to C_{i-2} || C_{i-1} | if(complete_block) 10 else 11
    tweakey[ESPEON256_TKSIZE-1] = (complete_block) ? 0x2 : 0x3;
    unsigned int last_block_length = (complete_block) ? ESPEON_BLOCKSIZE : clen % ESPEON_BLOCKSIZE;

    unsigned char out[ESPEON_BLOCKSIZE];
    unsigned char c_out[ESPEON_BLOCKSIZE];
    eevee_common_forkskinny_128_256_init_tk1(&tks1, tweakey + ESPEON_KEYSIZE, ESPEON256_TKS_TWO_LEG_ROUNDS);
    eevee_common_forkskinny_128_256_decrypt(&tks1, &tks2, c_out, out, tag);

    #ifdef EEVEE_VERBOSE
    printf("\nC Block %lld\n", n_m_blocks);
    printf("In: ");
    print_n(tag, ESPEON_BLOCKSIZE);
    printf("\nTK: ");
    print_n(tweakey, ESPEON256_TKSIZE);
    printf("\nM*': ");
    print_n(out, ESPEON_BLOCKSIZE);
    printf("\nC*': ");
    print_n(c_out, ESPEON_BLOCKSIZE);
    printf("\n");
    #endif

    block_xor128(out, t_a);

    // output partial message
    memcpy(m + n_m_blocks*ESPEON_BLOCKSIZE, out, last_block_length);

    #ifdef EEVEE_VERBOSE
    printf("M*: ");
    print_n(m + n_m_blocks*ESPEON_BLOCKSIZE, last_block_length);
    printf("\n");
    // check tag (THIS IS NOT CONSTANT TIME!), see commented code below for a constant time implementation
    printf("\nCheck padding: ");
    print_n(&out[last_block_length], ESPEON_BLOCKSIZE-last_block_length);
    printf("\n");
    #endif

    if(last_block_length < ESPEON_BLOCKSIZE && out[last_block_length] != 0x80) return 1;
    for(unsigned int i=last_block_length +1; i<ESPEON_BLOCKSIZE; i++) {
        if(out[i] != 0x00) return 1;
    }

    #ifdef EEVEE_VERBOSE
    printf("\nC*': ");
    for(unsigned int i=0; i< last_block_length; i++)
        printf("%02x", out[i] ^ c_out[i]);
    printf(" == ");
    for(unsigned int i=0; i< last_block_length; i++)
        printf("%02x", c[ESPEON_BLOCKSIZE*n_m_blocks+i]);
    printf("\n");
    #endif

    for(unsigned int i=0; i< last_block_length; i++) {
        if((out[i] ^ c_out[i]) != c[ESPEON_BLOCKSIZE*n_m_blocks+i]) return 1;
    }
    return 0;
    /**
    int fail = 0;
    if(last_block_length < ESPEON_BLOCKSIZE)
        fail |= c1_old[ESPEON_BLOCKSIZE*n_m_blocks + last_block_length] != 0x80;
    for(unsigned int i=last_block_length +1; i<ESPEON_BLOCKSIZE; i++) {
        fail |= c1_old[i] != 0x00;
    }

    printf("C*': ");
    for(unsigned int i=0; i< last_block_length; i++)
        printf("%02x", c1_out[i] ^ c1_old[i]);
    printf(" == ");
    for(unsigned int i=0; i< last_block_length; i++)
        printf("%02x", c[ESPEON_BLOCKSIZE*n_m_blocks+i]);
    printf("\n");
    for(unsigned int i=0; i< last_block_length; i++) {
        fail |= (c1_out[i] ^ c1_old[i]) != c[ESPEON_BLOCKSIZE*n_m_blocks+i];
    }
    return fail;
    **/
}

#ifndef MIMC_GMP_H
#define MIMC_GMP_H

#include "mimc128_gmp.h"
#include "gmp_utils.h"
#include "htmac128_gmp.h"
#include "ppmac128_gmp.h"

unsigned long long clen_mimc128_ctr(unsigned long long mlen) {
  if(mlen <= 15) {
    return 16;
  }else if(mlen % 15 != 0){
    return mlen/15 * 16 + 16;
  }else{
    return mlen/15 * 16;
  }
}

#endif // MIMC_GMP_H

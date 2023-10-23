#include "mimc128_gmp.h"
#include "assert.h"

static char* hex_constants[MIMC128_GMP_CONSTANTS] = {
  "67cb30ec12123b00bba3498728310135",
  "34aaf0b6f625334b99135f6330026f84",
  "65848431bafbd09160a75a9c633817ac",
  "17e3acdb6ad2f59041ec41f24e31f1ef",
  "6f1e643ab1c73e008c74452e217e8b16",
  "517cd517652038a8495ee6b9946cfc0f",
  "79a2abaf479d15e0cf5a1d592123cf5b",
  "4d71262c00c840482557e692b88172fa",
  "666c10514f1ea5dfc6215c91312fa5fd",
  "7bfa41088b3e587afd0ac98883ae13aa",
  "33579f539bd10aa829b0c674d330b1f2",
  "1de23123a29c8c8419785afe62bc9bdb",
  "69025ffc6ed405c54f224e9234853459",
  "1508ee5633fdc806160f3ce3ae0ffe7f",
  "7188ed3942bb0f2521673b9598d331d0",
  "505eb0a9549819ef332e8b2ea5a2f26b",
  "c3962837ea70db22c702b2af81895e2",
  "49691d1805a35c626f86f1011c6f87db",
  "5387ff0a158f0010962104a978f7fd97",
  "4c1dd3a7670f211c035530ace9fdb2",
  "61a2902cfbaa4bb3841738bd6b58eb87",
  "23f80dc982460cf71781e86403c69f44",
  "773acd9dea4a52400550baf7679c20f2",
  "e2177bfb8a00855dfc3cd07a1f2da01",
  "3391ee8eac52d7659fd1804790a7b26a",
  "671442da969886274f15d684a1b2ee50",
  "6130536be1b904d2e1c4db8f3c7377fa",
  "a4e8c0ed6f16d3524a7025e23a44b37",
  "47cbc6d6cffb962e25500dfd841d8876",
  "2e375b257b74f678c94f2711a7f7e397",
  "2a27d7c3716372d47fca47ff8299c510",
  "41a79914c70df3ee391040641707a49",
  "56b3ec596cec9bf3e3a8aaafb413d1fd",
  "12fdc050e4639d71e49fa6992763cbcf",
  "4aff4ecd5084d50554dc02cf2e182f9",
  "1a477aacbd34b98381ac9a1e336dc8dc",
  "5e6b745376c12a977fbfe2665b28ac03",
  "2ba951a2d141fc5df8e3e06f91680d69",
  "31fa6800ca9cc0d678c26c2a5bad8fca",
  "721c97dc25dc4e03b6fb504f30867654",
  "3fc3a73bfa88d9c5ab3a0040090f9d05",
  "1c7db39511fe157a0e1b7aa515d59d5f",
  "674e14fdf0c4269c696a6cfc92d9fd5a",
  "44dbdbb82d5c225fc094f8d4dde2b5fb",
  "4452956d4f4d2edce061d66c5648645e",
  "6d27f2d79fa0388ca30e252b520e848a",
  "5fe9f4eedd2b20ee1814c76049b2a753",
  "6e9d8f65ecd211a4f9d73a1ed89f855c",
  "6f17aa0d54e4b130766ba012d9bff786",
  "22944625fafd92ea2502c4dae5c53c24",
  "46c2dc42c1cbcef2493d38ef9af9536d",
  "76832870b66c3654d07d00886ae324e",
  "66280359e25461d07a1146dd5960869b",
  "266175997a599e60bada30deb60b1546",
  "5d7a2969bc0529c317b1ae8cc8f2f3a",
  "4dbeec12c416174e88162c6a5ac023fd",
  "5e4eef57c1554759fc9d3d64a64a4009",
  "3f3ecad7faa2ac895de6da12ec8b9ab6",
  "4a8297713e4b7b01aea6dee5c10a681b",
  "545b02a4421c3a6bceb11117cf5671b",
  "1b4dc665d277c4c8893c2e6e4dd1ba3d",
  "1cf5204a5827fbb6ad7b910f80a9686e",
  "76a6b3ec9049e5c311f9c9deb1fc2e84",
  "106815d16de13df042ba071652e0eb5c",
  "7a2c77b1177fc6caebcf903a8563f3d2",
  "5824ed2eacbf7e816e8c448e659b24bf",
  "63210b7031b934d04285eafd31bef72c",
  "7bc5ad36f6b4535edd4087483a24bc44",
  "41c51c2f24dc5091a2687c79360bb833",
  "df27e8d6c2f0d96e5bf8fc00750788f",
  "38a9f537751ac48691ad930ece6c5469",
  "323a62abdda5de786f5b031c231a015",
  "541e7a7d0e02b4f889ce8eea210065a",
};

void mimc128_init(mimc128 *instance) {
  CHECK_RET(mpz_init_set_str(instance->prime, "800000000000000000000000001b8001", 16));
  for(int i=0; i<MIMC128_GMP_CONSTANTS; i++) {
    CHECK_RET(mpz_init_set_str(instance->constants[i], hex_constants[i], 16));
  }
}

void mimc128_clear(mimc128 *instance) {
  mpz_clear(instance->prime);
  for(int i=0; i<MIMC128_GMP_CONSTANTS; i++) {
    mpz_clear(instance->constants[i]);
  }
}

void mimc128_enc(const mimc128 *instance, mpz_t ciphertext, const mpz_t key, const mpz_t message) {
  mod_add(ciphertext, message, key, instance->prime);
  mpz_t tmp;
  mpz_init(tmp);
  for(int i=0; i<MIMC128_GMP_CONSTANTS; i++) {
    //x³
    mpz_set(tmp, ciphertext);
    mpz_mul(ciphertext, ciphertext, ciphertext);
    mpz_mod(ciphertext, ciphertext, instance->prime);
    mpz_mul(ciphertext, ciphertext, tmp);
    mpz_mod(ciphertext, ciphertext, instance->prime);
    mod_add(ciphertext, ciphertext, key, instance->prime);
    mod_add(ciphertext, ciphertext, instance->constants[i], instance->prime);
  }
  mpz_clear(tmp);
  mod_add(ciphertext, ciphertext, key, instance->prime);
}

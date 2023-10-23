#include "./common/stm32wrapper.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#if defined(EEVEE_FORKSKINNY_BACKEND_OPT32)
#include "forkskinny-opt32/internal-forkskinny.h"
#elif defined(EEVEE_FORKSKINNY_BACKEND_C)
#include "forkskinny-c/forkskinny64-cipher.h"
#else
#error Undefined SKINNY backend
#endif

#define EEVEE_BEGIN_EXP_HDR1 0x00
#define EEVEE_BEGIN_EXP_HDR2 0xff
#define EEVEE_BEGIN_EXP_HDR3 0x00

#define KEY_LEN 24
#define MAX_MESSAGE_LEN 8

int main(void)
{
    clock_setup();
    gpio_setup();
    usart_setup(115200);

    //cycle counter
    SCS_DEMCR |= SCS_DEMCR_TRCENA;
    DWT_CYCCNT = 0;
    DWT_CTRL |= DWT_CTRL_CYCCNTENA;
    char cyclebuffer[40];

    uint8_t buffer[3];

    unsigned char key[KEY_LEN];

    unsigned char *message = malloc(MAX_MESSAGE_LEN * sizeof(unsigned char));
    if(message == NULL) {
      // stop with error
      while(1) {
        sprintf(cyclebuffer, "Cannot allocate for message");
        send_USART_str(cyclebuffer);
      }
    }
    unsigned char *ciphertext = malloc(2*MAX_MESSAGE_LEN * sizeof(unsigned char));
    if(ciphertext == NULL) {
      free(message);
      // stop with error
      while(1) {
        sprintf(cyclebuffer, "Cannot allocate for ciphertext");
        send_USART_str(cyclebuffer);
      }
    }

    sprintf(cyclebuffer, "Ready!");
    send_USART_str(cyclebuffer);

    while(1){

        // This is a blocking read of 3-byte
        recv_USART_bytes(buffer, 3);
        // sprintf(cyclebuffer, "Received");
        // send_USART_str(cyclebuffer);
        // send_USART_bytes(buffer, 3);
        if(buffer[0] != EEVEE_BEGIN_EXP_HDR1 || buffer[1] != EEVEE_BEGIN_EXP_HDR2 || buffer[2] != EEVEE_BEGIN_EXP_HDR3)
          continue;

        // sprintf(cyclebuffer, "Ready!");
        // send_USART_str(cyclebuffer);

        // receive message length
        recv_USART_bytes(buffer, 2);
        unsigned int message_len = buffer[0] | (buffer[1] << 8);
        if(message_len > MAX_MESSAGE_LEN) {
          sprintf(cyclebuffer, "Message too large!");
          send_USART_str(cyclebuffer);
          continue;
        }

        // receive message test vector
        recv_USART_bytes(message, message_len);
        // receive key
        recv_USART_bytes(key, KEY_LEN);
        // receive nonce
        // recv_USART_bytes(nonce, NONCE_LEN);

        /////// MEASURE CYCLES
        DWT_CYCCNT = 0;
        unsigned int oldcount = DWT_CYCCNT;
        unsigned int tk1_count, tk23_count, encrypt_count;

        // Whatever it is you want to measure the cycle count of should go here
        #if defined(EEVEE_FORKSKINNY_BACKEND_OPT32)
        forkskinny_64_192_tweakey_schedule_t tks1, tks2;

        forkskinny_64_192_init_tks_keypart(&tks1, key, FORKSKINNY_64_192_ROUNDS_BEFORE + 2*FORKSKINNY_64_192_ROUNDS_AFTER);
        tk23_count = DWT_CYCCNT-oldcount;
        DWT_CYCCNT = 0;
        oldcount = DWT_CYCCNT;
        forkskinny_64_192_init_tks_tweakpart(&tks2, key + 16, FORKSKINNY_64_192_ROUNDS_BEFORE + 2*FORKSKINNY_64_192_ROUNDS_AFTER);
        tk1_count = DWT_CYCCNT-oldcount;
        DWT_CYCCNT = 0;
        oldcount = DWT_CYCCNT;

        forkskinny_64_192_encrypt_with_tks(&tks1, &tks2, ciphertext, ciphertext + 8, message);
        #elif defined(EEVEE_FORKSKINNY_BACKEND_C)
        ForkSkinny64Key_t tks1, tks2;
        forkskinny_c_64_192_init_tk1(&tks1, key, FORKSKINNY64_MAX_ROUNDS);
        tk1_count = DWT_CYCCNT-oldcount;
        DWT_CYCCNT = 0;
        oldcount = DWT_CYCCNT;
        forkskinny_c_64_192_init_tk2_tk3(&tks2, key + 8, FORKSKINNY64_MAX_ROUNDS);
        tk23_count = DWT_CYCCNT-oldcount;
        DWT_CYCCNT = 0;
        oldcount = DWT_CYCCNT;
        forkskinny_c_64_192_encrypt(&tks1, &tks2, ciphertext, ciphertext + 8, message);
        #endif

        encrypt_count = DWT_CYCCNT-oldcount;
        /////// STOP MEASURE CYCLES

        // send cyclecount
        sprintf(cyclebuffer, "%d", tk1_count);
        send_USART_str(cyclebuffer); // send cyclecount
        sprintf(cyclebuffer, "%d", tk23_count);
        send_USART_str(cyclebuffer); // send cyclecount
        sprintf(cyclebuffer, "%d", encrypt_count);
        send_USART_str(cyclebuffer); // send cyclecount
        // send ciphertext
        send_USART_bytes(ciphertext, 2*message_len);
        // send tag
        send_USART_bytes(ciphertext, 0);
    }

    return 0;
}

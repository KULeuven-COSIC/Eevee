#include "./common/stm32wrapper.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#if defined(EEVEE_FORKSKINNY_BACKEND_OPT32)
#include "forkskinny-opt32/skinny.h"
#elif defined(EEVEE_FORKSKINNY_BACKEND_C)
#include "skinny-c/include/skinny128-cipher.h"
#else
#error Undefined SKINNY backend
#endif

#define EEVEE_BEGIN_EXP_HDR1 0x00
#define EEVEE_BEGIN_EXP_HDR2 0xff
#define EEVEE_BEGIN_EXP_HDR3 0x00

#define KEY_LEN 32
#define MAX_MESSAGE_LEN 16

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
    unsigned char *ciphertext = malloc(MAX_MESSAGE_LEN * sizeof(unsigned char));
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

        // Whatever it is you want to measure the cycle count of should go here
        #if defined(EEVEE_FORKSKINNY_BACKEND_OPT32)
          skinny_128_256_encrypt(key, ciphertext, message);
        #elif defined(EEVEE_FORKSKINNY_BACKEND_C)
          Skinny128Key_t tks;
          skinny128_set_key(&tks, key, KEY_LEN);
          skinny128_ecb_encrypt(ciphertext, message, &tks);
        #endif

        unsigned int cyclecount = DWT_CYCCNT-oldcount;
        /////// STOP MEASURE CYCLES

        // send cyclecount
        sprintf(cyclebuffer, "%d", cyclecount);
        send_USART_str(cyclebuffer); // send cyclecount
        // send ciphertext
        send_USART_bytes(ciphertext, message_len);
        // send tag
        send_USART_bytes(ciphertext, 0);
    }

    return 0;
}

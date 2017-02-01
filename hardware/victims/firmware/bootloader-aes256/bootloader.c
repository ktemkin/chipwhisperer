/*
    This file is part of the ChipWhisperer Example Targets
    Copyright (C) 2014-2016 NewAE Technology Inc 
	
	Authors:
		Colin O'Flynn <coflynn@newae.com>
		Greg d'Eon    <gdeon@newae.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "hal.h"
#include "aes256.h"

// Nobody will ever find this, right?
#include "supersecret.h"	

//If building on something besides avr-gcc target, need to replace this
#include <util/crc16.h>

// CRC check replies
#define COMM_BADCHECKSUM 0xA1
#define COMM_OK 0xA4

int main
	(
	void
	)
	{
    platform_init();
	init_uart();	
	trigger_setup();
	
 	/* Uncomment this to get a HELLO message for debug */
	/*
	putch('h');
	putch('e');
	putch('l');
	putch('l');
	putch('o');
	putch('\n');
	*/
	    
	//Load Super-Secret key
    aes256_context ctx; 
    uint8_t tmp32[32] = SECRET_KEY;
    aes256_init(&ctx, tmp32);

    //Load super-secret IV
    uint8_t iv[16] = IV;
       
    //Do this forever (we don't actually have a jumper to bootloader)
    uint8_t i;
    uint16_t crc;
    uint8_t c;
    while(1){
        c = (uint8_t)getch();
        if (c == 0){
        
            //Initial Value
            crc = 0x0000;
    
            //Read 16 Bytes now            
            for(i = 0; i < 16; i++){
                c = (uint8_t)getch();
                crc = _crc_xmodem_update(crc, c);
                //Save two copies, as one used for IV
                tmp32[i] = c;                
                tmp32[i+16] = c;
            }
            
            //Validate CRC-16
            uint16_t inpcrc = (uint16_t)getch() << 8;
            inpcrc |= (uint8_t)getch();
            
            if (inpcrc == crc){                  
                
                //CRC is OK - continue with decryption
                trigger_high();                
				aes256_decrypt_ecb(&ctx, tmp32); /* encrypting the data block */
				trigger_low();
                             
                //Apply IV (first 16 bytes)
                for (i = 0; i < 16; i++){
                    tmp32[i] ^= iv[i];
                }

                /* Comment this block out to always use initial IV, instead
                   of performing actual CBC mode operation */
                //Save IV for next time from original ciphertext                
                for (i = 0; i < 16; i++){
                    iv[i] = tmp32[i+16];
                }
                

                //Always send OK, done before checking
                //signature to ensure there is no timing
                //attack. This does mean user needs to
                //add some delay before sending next packet
                putch(COMM_OK);
                putch(COMM_OK);

                //Check the signature
                if ((tmp32[0] == SIGNATURE1) &&
                   (tmp32[1] == SIGNATURE2) &&
                   (tmp32[2] == SIGNATURE3) &&
                   (tmp32[3] == SIGNATURE4)){
                   
                   //We now have 12 bytes of useful data to write to flash memory.
                   //We don't actually write anything here though in real life would
                   //probably do more than just delay a moment
                   _delay_ms(1);
                }   
            } else {
                putch(COMM_BADCHECKSUM);
                putch(COMM_BADCHECKSUM);
            }            
        }         
    }
	 
	return 1;
	}
	
	
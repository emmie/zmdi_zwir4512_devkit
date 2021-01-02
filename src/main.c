/* ***************************************************************************\
 *
 * This is a simple example program, toggling a DevKit LED1 and printing the
 *  string "Hello World %second%" to UART1 once per second.
 *  
\* ***************************************************************************/

#include <stdio.h>
#include "ZWIR45xx-6LoWPAN.h"
#include "ZWIR45xx-UART1.h"
#include "ZWIR451x-Devkit.h"


// provide printf over UART1
ZWIR_UART1_PRINTF


/*****************************************************************************\
 This hook is called every second. Here we print our string and toggle the LED
\*****************************************************************************/
void ZWIR_Main1000ms ( void ) {

  static int i = 0;

  ZWIR_ToggleLED1 ( );
  printf ( "Hello World %d\n", i++ );
}

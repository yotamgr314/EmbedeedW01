#include <stdlib.h>
#include <stdio.h>

#include "System/system.h"
#include "System/delay.h"
#include "oledDriver/oledC.h"
#include "oledDriver/oledC_colors.h"
#include "oledDriver/oledC_shapes.h"

void User_Initialize(void)
{
  //Configure A/D Control Registers (ANSB & AD1CONx SFRs)

  //Configure S1/S2 and LED1/LED2 IO directions (TRISA)
}

/*
                         Main application
 */
int main(void)
{

    // initialize the system
    SYSTEM_Initialize();
    User_Initialize();

    //Set OLED Background color and Clear the display
    
    //Main loop
    while(1)
    {

	//Count S1 hits and display (Light LED1 when S1 pressed)
	//Toggle background color with S2 hits (Light LED2 when S2 pressed)
	//Get potentiometer position and display decimal value
    }
  return 1;
}


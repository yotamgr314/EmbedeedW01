#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>  // For abs()
#include "xc.h"

#ifndef FCY
#define FCY 4000000UL
#endif
#include <libpic30.h>

// Global variables
uint8_t displayMode = 0;      // 0 = blended display, 1 = single-color display
uint8_t activeColor = 0;      // 0 = RED, 1 = GREEN, 2 = BLUE
uint16_t savedRed = 0, savedGreen = 0, savedBlue = 0;  // Brightness values for each color
uint8_t potentiometerSynced = 0;  // Flag to track if potentiometer matches saved value

// Function Prototypes
void GPIO_Init(void);
void PWM_Init(void);
uint16_t Read_PotValue(void);
void Process_Buttons(void);
void Switch_DisplayMode(void);
void Switch_To_NextColor(void);
void Adjust_Composite_Brightness(uint16_t potValue);
void Adjust_Color_Brightness(uint16_t potValue);
void Update_LED_Output(uint8_t led, uint8_t state);
void Save_Current_Brightness(void);

// Main Function
int main(void) {
    GPIO_Init();
    PWM_Init();

    while (1) {
        uint16_t potValue = Read_PotValue();  // Read potentiometer value

        if (displayMode == 0) {  // Blended color display mode
            Adjust_Composite_Brightness(potValue);
        } else {  // Single-color display mode
            Adjust_Color_Brightness(potValue);
        }

        Process_Buttons();  // Handle S1 and S2 button presses
    }

    return 0;
}


void GPIO_Init(void) {
    TRISA |= (1 << 11) | (1 << 12);  // S1, S2 as input
    TRISA &= ~((1 << 8) | (1 << 9));  // LED1, LED2 as output
    ANSB |= (1 << 12);  // Potentiometer (AN8) as analog input

    AD1CON1 = 0x00;
    AD1CON1bits.MODE12 = 0;
    AD1CON1bits.ASAM = 0;
    AD1CON1bits.SSRC = 0b111;
    AD1CON1bits.ADON = 1;

    AD1CON2 = 0x00;
    AD1CON3 = 0x00;
    AD1CON3bits.ADCS = 0xFF;
    AD1CON3bits.SAMC = 0x10;
    AD1CHS = 0x08;  // AN8
}

void PWM_Init(void) {
    RPOR13bits.RP26R = 13;
    RPOR13bits.RP27R = 14;
    RPOR11bits.RP23R = 15;

    OC1CON1 = OC2CON1 = OC3CON1 = 0;
    OC1CON2 = OC2CON2 = OC3CON2 = 0;
    OC1CON1bits.OCTSEL = OC2CON1bits.OCTSEL = OC3CON1bits.OCTSEL = 0b111;
    OC1CON1bits.OCM = OC2CON1bits.OCM = OC3CON1bits.OCM = 0b110;
    OC1CON2bits.SYNCSEL = OC2CON2bits.SYNCSEL = OC3CON2bits.SYNCSEL = 0x1F;
    OC1RS = OC2RS = OC3RS = 1023;
    OC1R = OC2R = OC3R = 0;  // Start with 0 intensity
}

uint16_t Read_PotValue(void) {
    AD1CON1bits.SAMP = 1;  // Start sampling
    __delay_us(10);        // Allow sampling time
    AD1CON1bits.SAMP = 0;  // End sampling, start conversion
    while (!AD1CON1bits.DONE);  // Wait for conversion
    return ADC1BUF0;  // Return 10-bit result
}

void Process_Buttons(void) {
    if (!(PORTA & (1 << 11))) {  // S1 is pressed
        __delay_ms(20);  // Debounce delay
        Switch_DisplayMode();
        while (!(PORTA & (1 << 11)));  // Wait for release
    }

    if (!(PORTA & (1 << 12))) {  // S2 is pressed
        __delay_ms(20);  // Debounce delay
        Switch_To_NextColor();
        while (!(PORTA & (1 << 12)));  // Wait for release
    }
}

void Switch_DisplayMode(void) {
    displayMode ^= 1;  // Toggle between blended and single-color modes
    potentiometerSynced = 0;  // Reset potentiometer sync
}

void Switch_To_NextColor(void) {
    if (displayMode == 1) {  // Only switch colors in single-color mode
        Save_Current_Brightness();

        // Cycle through the colors
        activeColor = (activeColor + 1) % 3;

        // Reset sync flag to require potentiometer match
        potentiometerSynced = 0;
    }
}

void Save_Current_Brightness(void) {
    switch (activeColor) {
        case 0: savedRed = OC1R; break;
        case 1: savedGreen = OC2R; break;
        case 2: savedBlue = OC3R; break;
    }
}

void Adjust_Composite_Brightness(uint16_t potValue) {
    float scale = potValue / 1023.0;  // Scale factor as a percentage

    // Apply the scale factor to the saved values
    OC1R = (uint16_t)(savedRed * scale);   // Red LED intensity
    OC2R = (uint16_t)(savedGreen * scale); // Green LED intensity
    OC3R = (uint16_t)(savedBlue * scale);  // Blue LED intensity
}

void Adjust_Color_Brightness(uint16_t potValue) {
    uint16_t *currentBrightness = NULL;

    switch (activeColor) {
        case 0:  // RED
            currentBrightness = &savedRed;
            OC1R = potentiometerSynced ? potValue : *currentBrightness;
            OC2R = OC3R = 0;
            break;
        case 1:  // GREEN
            currentBrightness = &savedGreen;
            OC2R = potentiometerSynced ? potValue : *currentBrightness;
            OC1R = OC3R = 0;
            break;
        case 2:  // BLUE
            currentBrightness = &savedBlue;
            OC3R = potentiometerSynced ? potValue : *currentBrightness;
            OC1R = OC2R = 0;
            break;
    }

    // Sync the potentiometer if it matches the saved value
    if (!potentiometerSynced && abs(potValue - *currentBrightness) < 5) {
        potentiometerSynced = 1;
    }

    // If it's the first time, set the saved value
    if (*currentBrightness == 0) {
        *currentBrightness = potValue;
    }
}

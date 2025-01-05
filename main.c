#include <stdint.h>
#include <stdio.h>
#include "xc.h"
#define FCY 4000000UL
#include <libpic30.h>

// Global variables
uint8_t mode = 0;          // 0 = composite, 1 = individual color mode
uint8_t currentColor = 0;  // 0 = RED, 1 = GREEN, 2 = BLUE
uint16_t storedRed = 0, storedGreen = 0, storedBlue = 0;
uint8_t potSynced = 1;     // Potentiometer synchronization flag

// Function Prototypes
void SYSTEM_Initialize(void);
void User_Initialize(void);
void PWM_Initialize(void);
uint16_t Read_Potentiometer(void);
void Handle_Switches(void);
void Toggle_Mode(void);
void Cycle_Color(void);
void Update_Composite_Mode(uint16_t adcValue);
void Update_Individual_Mode(uint16_t adcValue);
void Set_LED(uint8_t led, uint8_t state);

// Main Function
int main(void) {
    SYSTEM_Initialize();
    User_Initialize();
    PWM_Initialize();

    while (1) {
        uint16_t adcValue = Read_Potentiometer(); // Read potentiometer value
        
        if (mode == 0) { // Composite mode
            Update_Composite_Mode(adcValue);
        } else { // Individual color mode
            Update_Individual_Mode(adcValue);
        }

        Handle_Switches(); // Handle S1 and S2 logic
    }

    return 0;
}

// Initialization Functions
void User_Initialize(void) {
    TRISA |= (1 << 11) | (1 << 12); // S1, S2 as input
    TRISA &= ~((1 << 8) | (1 << 9)); // LED1, LED2 as output
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
    AD1CHS = 0x08; // AN8
}

void PWM_Initialize(void) {
    RPOR13bits.RP26R = 13;
    RPOR13bits.RP27R = 14;
    RPOR11bits.RP23R = 15;

    OC1CON1 = OC2CON1 = OC3CON1 = 0;
    OC1CON2 = OC2CON2 = OC3CON2 = 0;
    OC1CON1bits.OCTSEL = OC2CON1bits.OCTSEL = OC3CON1bits.OCTSEL = 0b111;
    OC1CON1bits.OCM = OC2CON1bits.OCM = OC3CON1bits.OCM = 0b110;
    OC1CON2bits.SYNCSEL = OC2CON2bits.SYNCSEL = OC3CON2bits.SYNCSEL = 0x1F;
    OC1RS = OC2RS = OC3RS = 1023;
    OC1R = OC2R = OC3R = 0;
}

// Input Handling
uint16_t Read_Potentiometer(void) {
    AD1CON1bits.SAMP = 1;      // Start sampling
    __delay_us(10);            // Allow sampling time
    AD1CON1bits.SAMP = 0;      // End sampling, start conversion
    while (!AD1CON1bits.DONE); // Wait for conversion
    return ADC1BUF0;           // Return 10-bit result
}

void Handle_Switches(void) {
    if (!(PORTA & (1 << 11))) { // S1 is pressed
        Set_LED(1, 1); // Turn on LED1
        __delay_ms(20); // Debounce delay
        while (!(PORTA & (1 << 11))); // Wait for release
        Toggle_Mode();
    } else {
        Set_LED(1, 0); // Turn off LED1
    }

    if (!(PORTA & (1 << 12))) { // S2 is pressed
        Set_LED(2, 1); // Turn on LED2
        __delay_ms(20); // Debounce delay
        while (!(PORTA & (1 << 12))); // Wait for release
        Cycle_Color();
    } else {
        Set_LED(2, 0); // Turn off LED2
    }
}

void Toggle_Mode(void) {
    mode ^= 1; // Toggle mode
    potSynced = 1; // Reset sync
}

void Cycle_Color(void) {
    currentColor = (currentColor + 1) % 3; // Cycle colors
    potSynced = 0; // Reset sync for new color
}

void Update_Composite_Mode(uint16_t adcValue) {
    float scale = adcValue / 1023.0;  // Scale factor as a percentage

    // Scale stored values by the potentiometer reading
    OC1R = (uint16_t)(storedRed * scale);   // Red LED intensity
    OC2R = (uint16_t)(storedGreen * scale); // Green LED intensity
    OC3R = (uint16_t)(storedBlue * scale);  // Blue LED intensity
}

void Update_Individual_Mode(uint16_t adcValue) {
    switch (currentColor) {
        case 0: // RED
            if (!potSynced && abs(adcValue - storedRed) < 5) potSynced = 1;
            if (potSynced) storedRed = adcValue;
            OC1R = potSynced ? storedRed : adcValue;
            OC2R = 0; OC3R = 0;
            break;

        case 1: // GREEN
            if (!potSynced && abs(adcValue - storedGreen) < 5) potSynced = 1;
            if (potSynced) storedGreen = adcValue;
            OC1R = 0; OC2R = potSynced ? storedGreen : adcValue; OC3R = 0;
            break;

        case 2: // BLUE
            if (!potSynced && abs(adcValue - storedBlue) < 5) potSynced = 1;
            if (potSynced) storedBlue = adcValue;
            OC1R = 0; OC2R = 0; OC3R = potSynced ? storedBlue : adcValue;
            break;
    }
}

void Set_LED(uint8_t led, uint8_t state) {
    if (led == 1) {
        LATAbits.LATA8 = state; // Control LED1
    } else if (led == 2) {
        LATAbits.LATA9 = state; // Control LED2
    }
}
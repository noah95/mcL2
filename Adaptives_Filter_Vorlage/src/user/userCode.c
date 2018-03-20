/*******************************************************************************
 * Project:
 *  "Neuentwicklung DSP-Board fuer das Labor MikroComm"
 *
 * Version / Date:
 *  0.9 / August 2016
 *
 * Authors:
 *  Simon Gerber, Belinda Kneubuehler
 *
 * File Name:
 *  userCode.c
 *
 * Description:
 *  This file contains the user specific instructions that will be called at
 *  the correct location in the main loop.
 *******************************************************************************/

#include <dsp.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "userConfig.h"
#include "../globals.h"
#include "../tlv320aic.h"
#include "../IOconfig.h"
#include "../pwm.h"
#include "userCode.h"


/**************************************************************************
 * User Global Definitions
 ***************************************************************************/

volatile int8_t mode = 0;


bool updateMode(void);
fractional dds(unsigned int);

#define N_TAPS  32
#define DELAY   64// 0.008*CODEC_SAMPLE_RATE
#define MU      5000

// Buffer mit delay
fractional   autonotchBuffer[BUFFERLENGTH_DIV_2 + DELAY];
fractional * autonotchSrcSamps = &autonotchBuffer[0];
fractional * autonotchRefSamps = &autonotchBuffer[DELAY];

// coefficients
fractional __attribute__((space(xmemory), far, aligned(256)))
    coefficients[N_TAPS];

// delay elements
fractional __attribute__((space(ymemory), far, aligned(256)))
    delayBuffer[N_TAPS];

FIRStruct FIRfilter;

/**************************************************************************
 * User Public Functions
 ***************************************************************************/

/**
 * This user function is called once in the begining. It can be used to
 * initialise user buffers ect.
 */
void user_init()
{
    codec_setInput(LINE);
    FIRStructInit(&FIRfilter, N_TAPS, coefficients, COEFFS_IN_DATA, delayBuffer);    
    FIRDelayInit(&FIRfilter);
}

/**
 * This user function is called once in the main loop.
 */
void user_mainLoop()
{
    if (updateMode()) {
        led_setColour(LED2, mode & 0x01 ? 255 : 0, 0, 0);
        led_setColour(LED1, (mode >> 1) & 0x01 ? 255 : 0, 0, 0);
    }

}

/**
 * This user function is called when new data is ready from the DMA.
 * @param sourceBuffer  is the buffer that contains the available input data
 * @param targetBuffer  is the buffer that the processed data has to be written into
 */
void user_processData(fractional *sourceBuffer, fractional *targetBuffer)
{
    uint16_t i, index1, index2;

    static fractional rxLeft[BUFFERLENGTH_DIV_2], rxRight[BUFFERLENGTH_DIV_2];
    static fractional txLeft[BUFFERLENGTH_DIV_2], txRight[BUFFERLENGTH_DIV_2];

    // copy sourceBuffer to leftSignalBuffer and rightSignalBuffer
    for (index1 = 0, index2 = 0; index1 < BUFFERLENGTH_DIV_2; index1++) {
        rxLeft[index1] = sourceBuffer[index2++];
        rxRight[index1] = sourceBuffer[index2++];
    }

    switch (mode) {
        case 0:
            //talk through: just copy input (rx) into output (tx) 
            for (i = 0; i < BUFFERLENGTH_DIV_2; i++) {
                txLeft[i] = rxLeft[i];
            }
            for (i = 0; i < BUFFERLENGTH_DIV_2; i++) {
                txRight[i] = rxRight[i];
            }
            break;
        case 1:
            //IIR filter
// extern fractional* FIRLMS (
//   int numSamps,
//   fractional* dstSamps,
//   fractional* srcSamps,
//   FIRStruct* filter,
//   fractional* refSamps,
//);
            // store delay samples
            for (i = 0; i < DELAY; i++)
            {
                autonotchSrcSamps[i] = autonotchRefSamps[BUFFERLENGTH_DIV_2 + i - DELAY];
            }
            // Copy input samples
            for (i = 0; i < BUFFERLENGTH_DIV_2; i++)
            {
                autonotchRefSamps[i] = rxLeft[i];
            }
            FIRLMS(BUFFERLENGTH_DIV_2, rxLeft, autonotchSrcSamps, &FIRfilter, autonotchRefSamps, MU);
            VectorSubtract(BUFFERLENGTH_DIV_2, txLeft, rxLeft, autonotchSrcSamps);
            
            for (i = 0; i < BUFFERLENGTH_DIV_2; i++) {
                txRight[i] = txLeft[i];
            }
            break;
        case 2:
            

            break;
    }

    //copy left and right txBuffer into targetBuffer
    for (index1 = 0, index2 = 0; index1 < BUFFERLENGTH_DIV_2; index1++) {
        targetBuffer[index2++] = txLeft[index1];
        targetBuffer[index2++] = txRight[index1]; //txRight[index1];
    }
}

/**************************************************************************
 * User Private Functions
 ***************************************************************************/

/**
 * This function changes between four different modes whenever a switch is
 * pushed. The switches are used to count the mode up or down.
 * @return a bool that is true whenever the mode has changed (a switch has been
 * pushed)
 */
bool updateMode(void)
{
    bool modeChanged = false;
    if (switchStatesChanged) // Any changes in the state of the switches?
    {
        if (switchStatesChanged & SW1_MASK) // Has state of switch 1 changed?
        {
            switchStatesChanged &= ~SW1_MASK;
            if ((switchStates & SW1_MASK) == 0) // Is new state = 0?
            {
                mode++;
                if (mode > 3) {
                    mode = 0;
                }
            }
        }
        if (switchStatesChanged & SW2_MASK) // Has state of switch 2 changed?
        {
            switchStatesChanged &= ~SW2_MASK;
            if ((switchStates & SW2_MASK) == 0) // Is new state = 0?
            {
                mode--;
                if (mode < 0) {
                    mode = 3;
                }
            }
        }
        modeChanged = true;
    }
    return modeChanged;
}

fractional dds(unsigned int deltaPsiRegister){
    static unsigned int psiRegister;
    
    return psiRegister; //just to make the compiler happy!
}
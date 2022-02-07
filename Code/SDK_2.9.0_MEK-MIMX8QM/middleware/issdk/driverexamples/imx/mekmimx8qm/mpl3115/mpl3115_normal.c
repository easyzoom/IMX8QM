/*
 * Copyright 2018 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file mpl3115_normal.c
 * @brief The mpl3115_normal.c file implements the ISSDK MPL3115 sensor driver
 *        example demonstration with polling mode.
 */

//-----------------------------------------------------------------------
// SDK Includes
//-----------------------------------------------------------------------
#include "pin_mux.h"
#include "clock_config.h"
#include "board.h"
#include "fsl_debug_console.h"
#include "fsl_irqsteer.h"

/* CMSIS Includes */
#include "Driver_I2C.h"

/* ISSDK Includes */
#include "issdk_hal.h"
#include "mpl3115_drv.h"

//-----------------------------------------------------------------------
// Macros
//-----------------------------------------------------------------------
#define MPL3115_DATA_SIZE (5) /* 3 byte Pressure/Altitude and 2 byte Temperature. */
/*! In MPL3115 the Auto Acquisition Time Step (ODR) can be set only in powers of 2 (i.e. 2^x, where x is the
 *  SAMPLING_EXPONENT).
 *  This gives a range of 1 second to 2^15 seconds (9 hours). */
#define MPL3115_SAMPLING_EXPONENT (1) /* 2 seconds */

//-----------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------
/*! @brief Register settings for Normal (non buffered) mode. */
const registerwritelist_t cMpl3115ConfigNormal[] = {
    /* Enable Data Ready and Event flags for Pressure, Temperature or either. */
    {MPL3115_PT_DATA_CFG,
     MPL3115_PT_DATA_CFG_TDEFE_ENABLED | MPL3115_PT_DATA_CFG_PDEFE_ENABLED | MPL3115_PT_DATA_CFG_DREM_ENABLED,
     MPL3115_PT_DATA_CFG_TDEFE_MASK | MPL3115_PT_DATA_CFG_PDEFE_MASK | MPL3115_PT_DATA_CFG_DREM_MASK},
    /* Set Over Sampling Ratio to 128. */
    {MPL3115_CTRL_REG1, MPL3115_CTRL_REG1_OS_OSR_128, MPL3115_CTRL_REG1_OS_MASK},
    /* Set Auto acquisition time step. */
    {MPL3115_CTRL_REG2, MPL3115_SAMPLING_EXPONENT, MPL3115_CTRL_REG2_ST_MASK},
    __END_WRITE_DATA__};

/*! @brief Address of Status Register. */
const registerreadlist_t cMpl3115Status[] = {{.readFrom = MPL3115_STATUS, .numBytes = 1}, __END_READ_DATA__};

/*! @brief Address and size of Raw Pressure+Temperature Data in Normal Mode. */
const registerreadlist_t cMpl3115OutputNormal[] = {{.readFrom = MPL3115_OUT_P_MSB, .numBytes = MPL3115_DATA_SIZE},
                                                   __END_READ_DATA__};

//-----------------------------------------------------------------------
// Functions
//-----------------------------------------------------------------------
/*!
 * @brief Main function
 */
int main(void)
{
    int16_t tempInDegrees;
    uint32_t pressureInPascals;
    int32_t status;
    uint8_t dataReady;
    uint8_t data[MPL3115_DATA_SIZE];
    mpl3115_pressuredata_t rawData;
    sc_ipc_t ipc;
    uint32_t freq;

    ARM_DRIVER_I2C *I2Cdrv = &I2C_S_DRIVER; // Now using the shield.h value!!!
    mpl3115_i2c_sensorhandle_t mpl3115Driver;

    ipc = BOARD_InitRpc();
    BOARD_InitMemory();

    BOARD_InitPins();
    BOARD_BootClockRUN();

    BOARD_InitDebugConsole();

    PRINTF("\r\n ----------------------------------------------------------------\r\n");
    PRINTF("\r\n ISSDK MPL3115 sensor driver example demonstration with POLL mode\r\n");
    PRINTF("\r\n ----------------------------------------------------------------\r\n");

    /* Power on LPI2C. */
    if (sc_pm_set_resource_power_mode(ipc, SC_R_I2C_0, SC_PM_PW_MODE_ON) != SC_ERR_NONE)
    {
        PRINTF("Error: Failed to power on LPI2C\r\n");
    }

    /* Set LPI2C clock */
    freq = CLOCK_SetIpFreq(kCLOCK_DMA_Lpi2c0, SC_24MHZ);
    if (freq == 0)
    {
        PRINTF("Error: Failed to set LPI2C frequency\r\n");
    }

    /* Power on IRQSTEER. */
#if (MIMX8QM_CM4_CORE0)
    if (sc_pm_set_resource_power_mode(ipc, SC_R_IRQSTR_M4_0, SC_PM_PW_MODE_ON) != SC_ERR_NONE)
    {
        PRINTF("Error: Failed to power on IRQSTEER M4_0\r\n");
    }
#elif (MIMX8QM_CM4_CORE1)
    if (sc_pm_set_resource_power_mode(ipc, SC_R_IRQSTR_M4_1, SC_PM_PW_MODE_ON) != SC_ERR_NONE)
    {
        PRINTF("Error: Failed to power on IRQSTEER M4_1\r\n");
    }
#endif

    /* Enable interrupt in irqsteer */
    IRQSTEER_Init(IRQSTEER);
    IRQSTEER_EnableInterrupt(IRQSTEER, DMA_I2C0_INT_IRQn);


    /*! Initialize the I2C driver. */
    status = I2Cdrv->Initialize(I2C_S_SIGNAL_EVENT);
    if (ARM_DRIVER_OK != status)
    {
        PRINTF("\r\n I2C Initialization Failed\r\n");
        return -1;
    }

    /*! Set the I2C Power mode. */
    status = I2Cdrv->PowerControl(ARM_POWER_FULL);
    if (ARM_DRIVER_OK != status)
    {
        PRINTF("\r\n I2C Power Mode setting Failed\r\n");
        return -1;
    }

    /*! Set the I2C bus speed. */
    status = I2Cdrv->Control(ARM_I2C_BUS_SPEED, ARM_I2C_BUS_SPEED_FAST);
    if (ARM_DRIVER_OK != status)
    {
        PRINTF("\r\n I2C Control Mode setting Failed\r\n");
        return -1;
    }

    /*! Initialize MPL3115 sensor driver. */
    status = MPL3115_I2C_Initialize(&mpl3115Driver, &I2C_S_DRIVER, I2C_S_DEVICE_INDEX, MPL3115_I2C_ADDR,
                                    MPL3115_WHOAMI_VALUE);
    if (SENSOR_ERROR_NONE != status)
    {
        PRINTF("\r\n Sensor Initialization Failed\r\n");
        return -1;
    }
    PRINTF("\r\n Successfully Initiliazed MPL3115 Sensor\r\n");
    PRINTF("\r\n Device Address is 0x%x and WHOAMI Value is 0x%x\r\n", MPL3115_I2C_ADDR, MPL3115_WHOAMI_VALUE);

    /*! Configure the MPL3115 sensor. */
    status = MPL3115_I2C_Configure(&mpl3115Driver, cMpl3115ConfigNormal);
    if (SENSOR_ERROR_NONE != status)
    {
        PRINTF("\r\nMPL3115 configuration failed...\r\n");
        return -1;
    }
    PRINTF("\r\n Successfully Applied MPL3115 Sensor Configuration\r\n");

	PRINTF("\r\n Reading MPL3115 Pressure & Temperature Values\r\n");

    for (;;) /* Forever loop */
    {
        /*! Wait for data ready from the MPL3115. */
        status = MPL3115_I2C_ReadData(&mpl3115Driver, cMpl3115Status, &dataReady);
        if (0 == (dataReady & MPL3115_DR_STATUS_PTDR_MASK))
        { /* Loop, if new sample is not available. */
            continue;
        }

        /*! Read new raw sensor data from the MPL3115. */
        status = MPL3115_I2C_ReadData(&mpl3115Driver, cMpl3115OutputNormal, data);
        if (ARM_DRIVER_OK != status)
        {
            PRINTF("\r\n Read Failed. \r\n");
            return -1;
        }

        /*! Process the sample and convert the raw sensor data. */
        rawData.pressure = (uint32_t)((data[0]) << 16) | ((data[1]) << 8) | ((data[2]));
        pressureInPascals = rawData.pressure / MPL3115_PRESSURE_CONV_FACTOR;

        rawData.temperature = (int16_t)((data[3]) << 8) | (data[4]);
        tempInDegrees = rawData.temperature / MPL3115_TEMPERATURE_CONV_FACTOR;

        PRINTF("\r\n Pressure    = %d Pa", pressureInPascals);
        PRINTF("\t Temperature = %d degC\r\n", tempInDegrees);
        ASK_USER_TO_RESUME(8); /* Ask for user input after processing 8 samples. */
    }
}

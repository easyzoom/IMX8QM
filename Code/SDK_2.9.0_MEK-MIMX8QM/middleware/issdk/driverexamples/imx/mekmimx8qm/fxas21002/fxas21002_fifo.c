/*
 * Copyright 2018 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file fxas21002_fifo.c
 * @brief The fxas21002_fifo.c file implements the ISSDK FXAS21002 sensor driver
 *        example demonstration with FIFO mode.
 */

/*  SDK Includes */
#include "pin_mux.h"
#include "clock_config.h"
#include "board.h"
#include "fsl_debug_console.h"
#include "fsl_irqsteer.h"

/* CMSIS Includes */
#include "Driver_I2C.h"

/* ISSDK Includes */
#include "issdk_hal.h"
#include "fxas21002_drv.h"

/*******************************************************************************
 * Macro Definitions
 ******************************************************************************/
/*---------------------------------------------------------------------------*/
/*! @def    FIFO_SIZE
 *  @brief  The watermark value configured for FXAS21002 FIFO Buffer.
 */
#define FIFO_SIZE 4

/*******************************************************************************
 * Constants
 ******************************************************************************/
/*! Prepare the register write list to configure FXAS21002 in FIFO mode. */
const registerwritelist_t fxas21002_Config_with_Fifo[] = {
    /*! Configure CTRL_REG1 register to put FXAS21002 to 12.5Hz sampling rate. */
    {FXAS21002_CTRL_REG1, FXAS21002_CTRL_REG1_DR_12_5HZ, FXAS21002_CTRL_REG1_DR_MASK},
    /*! Configure F_SETUP register to set FIFO in Stop mode, Watermark of FIFO_SIZE. */
    {FXAS21002_F_SETUP, FXAS21002_F_SETUP_F_MODE_STOP_MODE | (FIFO_SIZE << FXAS21002_F_SETUP_F_WMRK_SHIFT),
     FXAS21002_F_SETUP_F_MODE_MASK | FXAS21002_F_SETUP_F_WMRK_MASK},
    {FXAS21002_CTRL_REG3, FXAS21002_CTRL_REG3_WRAPTOONE_ROLL_DATA, FXAS21002_CTRL_REG3_WRAPTOONE_MASK},
    __END_WRITE_DATA__};

/*! Prepare the register read list to read FXAS21002 FIFO event. */
const registerreadlist_t fxas21002_Fifo_Event[] = {{.readFrom = FXAS21002_F_EVENT, .numBytes = 1}, __END_READ_DATA__};

/*! Prepare the register read list to read the raw gyro data from the FXAS21002. */
const registerreadlist_t fxas21002_Output_Values[] = {
    {.readFrom = FXAS21002_OUT_X_MSB, .numBytes = FXAS21002_GYRO_DATA_SIZE * FIFO_SIZE}, __END_READ_DATA__};

/*******************************************************************************
 * Code
 ******************************************************************************/
/*!
 * @brief Main function
 */
int main(void)
{
    int32_t status;
    uint8_t fifoEvent;
    uint8_t data[FXAS21002_GYRO_DATA_SIZE * FIFO_SIZE];
    fxas21002_gyrodata_t rawData;
    sc_ipc_t ipc;
    uint32_t freq;

    ARM_DRIVER_I2C *I2Cdrv = &I2C_S_DRIVER;
    fxas21002_i2c_sensorhandle_t FXAS21002drv;

    /*! Initialize the MCU hardware. */
    ipc = BOARD_InitRpc();
    BOARD_InitMemory();
    BOARD_InitPins();
    BOARD_BootClockRUN();

    BOARD_InitDebugConsole();

    PRINTF("\r\n ------------------------------------------------------------------\r\n");
    PRINTF("\r\n ISSDK FXAS21002 sensor driver example demonstration with FIFO mode\r\n");
    PRINTF("\r\n ------------------------------------------------------------------\r\n");

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

    /*! Initialize the FXAS21002 sensor driver. */
    status = FXAS21002_I2C_Initialize(&FXAS21002drv, &I2C_S_DRIVER, I2C_S_DEVICE_INDEX, FXAS21002_I2C_ADDR,
                                      FXAS21002_WHO_AM_I_WHOAMI_PROD_VALUE);
    if (SENSOR_ERROR_NONE != status)
    {
        PRINTF("\r\n Sensor Initialization Failed\r\n");
        return -1;
    }
    PRINTF("\r\n Successfully Initiliazed FXAS21002 Sensor\r\n");
    PRINTF("\r\n Device Address is 0x%x and WHOAMI Value is 0x%x\r\n", FXAS21002_I2C_ADDR, FXAS21002_WHO_AM_I_WHOAMI_PROD_VALUE);

    /*! Configure the FXAS21002 sensor driver. */
    status = FXAS21002_I2C_Configure(&FXAS21002drv, fxas21002_Config_with_Fifo);
    if (SENSOR_ERROR_NONE != status)
    {
        PRINTF("\r\n FXAS21002 Sensor Configuration Failed, Err = %d\r\n", status);
        return -1;
    }
    PRINTF("\r\n Successfully Applied FXAS21002 Sensor Configuration\r\n");

	PRINTF("\r\n Reading FXAS21002 Gyro Values in FIFO mode, FIFO width=%d\r\n", FIFO_SIZE);

    for (;;) /* Forever loop */
    {
        /*! Wait for the FIFO watermark event. */
        status = FXAS21002_I2C_ReadData(&FXAS21002drv, fxas21002_Fifo_Event, &fifoEvent);
        if (0 == (fifoEvent & FXAS21002_F_EVENT_F_EVENT_MASK))
        {
            continue;
        }

        /*! Read the raw sensor data from the FXAS21002. */
        status = FXAS21002_I2C_ReadData(&FXAS21002drv, fxas21002_Output_Values, data);
        if (ARM_DRIVER_OK != status)
        {
            PRINTF("\r\n Read Failed. \r\n");
            return -1;
        }

        for (uint8_t i = 0; i < FIFO_SIZE; i++)
        {
            /*! Convert the raw sensor data to signed 16-bit container for display to the debug port. */
            rawData.gyro[0] =
                ((int16_t)data[i * FXAS21002_GYRO_DATA_SIZE + 0] << 8) | data[i * FXAS21002_GYRO_DATA_SIZE + 1];
            rawData.gyro[1] =
                ((int16_t)data[i * FXAS21002_GYRO_DATA_SIZE + 2] << 8) | data[i * FXAS21002_GYRO_DATA_SIZE + 3];
            rawData.gyro[2] =
                ((int16_t)data[i * FXAS21002_GYRO_DATA_SIZE + 4] << 8) | data[i * FXAS21002_GYRO_DATA_SIZE + 5];
        }
        /* NOTE: PRINTF is relatively expensive in terms of CPU time, specially when used with-in execution loop. */
        PRINTF("\r\n Gyro X = %d  Y = %d  Z = %d\r\n", rawData.gyro[0], rawData.gyro[1], rawData.gyro[2]);
        ASK_USER_TO_RESUME(100 / FIFO_SIZE); /* Ask for user input after processing 100 samples. */
    }
}

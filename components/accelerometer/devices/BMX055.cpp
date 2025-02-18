#include "include/BMX055.h"
#include "BMX055.h"

//************************/
//*      VARIABLES       */
//************************/
static const char *TAG = "BMX055";

//?^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^/
//?         FUNCTIONS DEFINITION        /
//?^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^/
accel_error_e BMX055::init(spi_host_device_t spi_bus, gpio_num_t miso, gpio_num_t mosi, gpio_num_t sck, gpio_num_t cs, int32_t speed_mhz)
{
    /* Initialize the communication bus */
    spi_bus_config_t buscfg =
        {
            .mosi_io_num = mosi,
            .miso_io_num = miso,
            .sclk_io_num = sck,
            .quadwp_io_num = -1,
            .quadhd_io_num = -1};

    spi_device_interface_config_t devcfg =
        {
            .command_bits = 0,
            .address_bits = 8,
            .dummy_bits = 0,
            .mode = 0, // SPI mode 0
            .cs_ena_pretrans = 0,
            .cs_ena_posttrans = 0,
            .clock_speed_hz = speed_mhz * 1000 * 1000, // Clock out at 10 MHz
            .spics_io_num = cs,                        // CS pin
            .queue_size = 1,
            .pre_cb = NULL,
            .post_cb = NULL,
        };

    ESP_ERROR_CHECK(spi_bus_initialize(spi_bus, &buscfg, SPI_DMA_CH_AUTO));
    ESP_LOGI(TAG, "Bus initialized");

    // Attach the Accelerometer to the SPI bus
    ESP_ERROR_CHECK(spi_bus_add_device(spi_bus, &devcfg, &_spi));
    ESP_LOGI(TAG, "Device added to the spi bus");

    _xSpiSemaphore = xSemaphoreCreateMutex();

    return set_default_config();
}

accel_error_e BMX055::set_default_config()
{
    int8_t status = 0;
    uint8_t dummy_value;
    uint32_t l_dummy_value;
    bmx_int_scr_u int_src_active;
    bmx_power_mode_e pwr_mode;

    if (_get_chipID(&dummy_value) == ACC_OK)
    {
        printf("Chip id : 0x%x\n", dummy_value);
        if (dummy_value == BMX_CHIPID)
        {
            status += soft_reset();
            if (status != 0)
            {
                printf("* ACCEL SOFT-RESET FAILED *\n");
            }

            /* Get accelerometer actual configuration */
            status += _get_accel_conf(&_acc_conf);
            if (status != 0)
            {
                printf("* ACCEL CONFIG READ FAILED *\n");
            }
            else
            {
                printf("******************************\n");
                printf("* ACCEL CONFIG DEFAULT VALUE *\n");
                printf("******************************\n");
                printf("Range: 0x0%x\n", _acc_conf.range);
                printf("Bandwidth: 0x0%x\n", _acc_conf.bw);
                printf("Shadowing mode: ");
                printf("%s\n", _acc_conf.shadow_dis ? "true" : "false");
                printf("Data type: ");
                printf("%s\n", _acc_conf.data_high_bw ? "Unfiltered" : "filtered");
            }

            /* Set accelerometer configuration required value */
            _acc_conf.bw = BMX_ACC_BW_1000_HZ;
            _acc_conf.range = BMX_ACC_RANGE_2G;

            /* Set accelerometer actual configuration */
            status += _set_accel_conf(&_acc_conf);
            if (status != 0)
            {
                printf("* ACCEL CONFIG FAILED *\n");
            }

            /* Get accelerometer actual configuration */
            status += _get_accel_conf(&_acc_conf);
            if (status != 0)
            {
                printf("* ACCEL CONFIG READ FAILED *\n");
            }
            else
            {
                printf("******************************\n");
                printf("*   ACCEL CONFIG APP VALUE   *\n");
                printf("******************************\n");
                printf("Range: 0x0%x\n", _acc_conf.range);
                printf("Bandwidth: 0x0%x\n", _acc_conf.bw);
                printf("Shadowing mode: ");
                printf("%s\n", _acc_conf.shadow_dis ? "true" : "false");
                printf("Data type: ");
                printf("%s\n", _acc_conf.data_high_bw ? "Unfiltered" : "filtered");
            }

            /* Set accelerometer offsets using fast compensation feature */
            // TODO

            /* Get FIFO actual configuration */
            status += _get_fifo_config(&_fifo_conf);
            if (status != 0)
            {
                printf("* FIFO CONFIG READ FAILED *\n");
            }

            /* Set FIFO configuration required value*/
            _fifo_conf.fifo_data_select = BMX_XYZ_AXES;
            _fifo_conf.fifo_mode_select = BMX_MODE_BYPASS;
            _fifo_conf.length = BMX_FIFO_BUFFER_SIZE;
            _fifo_conf.wm_level = BMX_FIFO_WM_LEVEL;
            status += _set_fifo_config(&_fifo_conf);
            if (status != 0)
            {
                printf("* FIFO CONFIG FAILED *\n");
            }
            else
            {
                printf("*************************\n");
                printf("* FIFO CONFIG APP VALUE *\n");
                printf("*************************\n");
                printf("FIFO data type: %x\n", _fifo_conf.fifo_data_select);
                printf("FIFO mode: %x\n", _fifo_conf.fifo_mode_select);
                printf("FIFO watermark level: %d\n", _fifo_conf.wm_level);
                printf("FIFO frame count: %d\n", _fifo_conf.fifo_frame_count);
                printf("FIFO overrun? ");
                printf("%s\n", _fifo_conf.fifo_overrun ? "YES" : "NO");
            }

            /* Enable desired interrupts */
            status += set_interrupt_source(BMX_INT_EN_DATA_READY);
            if (status != 0)
            {
                printf("* ACTIVATION OF FIFO WM and FULL INTERRUPT FAILED *\n");
            }
            status += get_interrupt_source(&int_src_active);
            if (status != 0)
            {
                printf("* INTERRUPT SOURCES READ FAILED *\n");
            }
            else
            {
                printf("**********************\n");
                printf("* INTERRUPTS ENABLED *\n");
                printf("**********************\n");
                printf("Flat: ");
                printf("%s\n", int_src_active.flat ? "YES" : "NO");
                printf("Orientation: ");
                printf("%s\n", int_src_active.orient ? "YES" : "NO");
                printf("Single tap: ");
                printf("%s\n", int_src_active.s_tap ? "YES" : "NO");
                printf("Double tap: ");
                printf("%s\n", int_src_active.d_tap ? "YES" : "NO");
                printf("Slope X: ");
                printf("%s\n", int_src_active.slope_x ? "YES" : "NO");
                printf("Slope Y: ");
                printf("%s\n", int_src_active.slope_y ? "YES" : "NO");
                printf("Slope Z: ");
                printf("%s\n", int_src_active.slope_z ? "YES" : "NO");
                printf("FIFO wm: ");
                printf("%s\n", int_src_active.fifo_wm ? "YES" : "NO");
                printf("FIFO full: ");
                printf("%s\n", int_src_active.fifo_full ? "YES" : "NO");
                printf("Data ready: ");
                printf("%s\n", int_src_active.data_rdy ? "YES" : "NO");
            }

            /* Get actual power mode */
            status += get_power_mode(&pwr_mode);
            if (status != 0)
            {
                printf("* POWER MODE READ FAILED *\n");
            }
            else
            {
                printf("*********************\n");
                printf("* ACTUAL POWER MODE *\n");
                printf("*********************\n");
                printf("Power mode: ");
                if (pwr_mode == BMX_NORMAL_MODE)
                {
                    printf("NORMAL\n");
                }
                else if (pwr_mode == BMX_DEEP_SUSPEND_MODE)
                {
                    printf("DEEP SUSPEND\n");
                }
                else if (pwr_mode == BMX_LOW_POWER_MODE_1)
                {
                    printf("LOW POWER 2\n");
                }
                else if (pwr_mode == BMX_SUSPEND_MODE)
                {
                    printf("SUSPEND\n");
                }
                else if (pwr_mode == BMX_LOW_POWER_MODE_2)
                {
                    printf("LOW POWER 2\n");
                }
                else if (pwr_mode == BMX_STANDBY_MODE)
                {
                    printf("STANDBY\n");
                }
                else
                {
                    printf("UNKNOWN\n");
                }
            }

            /* Set power mode to NORMAL */
            status += set_power_mode(BMX_NORMAL_MODE);
            if (status != 0)
            {
                printf("* POWER MODE WRITE FAILED *\n");
            }
        }
        else
        {
            return ACC_ERR_DEV_NOT_FOUND;
        }
    }
    else
    {
        return ACC_ERR_DEV_NOT_FOUND;
    }

    if (status == 0)
    {
        printf("\nAccelerometer initialization COMPLETED\n");
        return ACC_OK;
    }
    else
    {
        printf("\nAccelerometer initialization FAILED\n");
        return ACC_ERR_INIT;
    }
}

accel_error_e BMX055::get_acc_settings(accel_settings_t &settings)
{
    bmx_acc_conf_t config;

    if (_get_accel_conf(&config) != ACC_OK)
    {
        return ACC_ERR_RD;
    }

    settings.range = _reg_value_2_range(config.range);
    settings.band = _reg_value_2_bw(config.bw);

    return ACC_OK;
}

accel_error_e BMX055::set_acc_settings(const accel_settings_t &settings)
{
    bmx_acc_conf_t config;

    if (_get_accel_conf(&config) != ACC_OK)
    {
        return ACC_ERR_RD;
    }

    config.range = _range_2_reg_value(settings.range);
    config.bw = _bw_2_reg_value(settings.band);

    return ACC_OK;
}

uint8_t BMX055::set_range_reg(uint8_t range)
{
    bmx_acc_conf_t config;

    if (_get_accel_conf(&config) != ACC_OK)
    {
        return ACC_ERR_RD;
    }

    config.range = _range_2_reg_value(range);

    if (_set_accel_conf(&config) != ACC_OK)
    {
        return ACC_ERR_WR;
    }
    
    return ACC_OK;
}

uint8_t BMX055::get_range_value(void)
{
    bmx_acc_conf_t config;

    if (_get_accel_conf(&config) != ACC_OK)
    {
        return ACC_ERR_WR;
    }
    
    return _reg_value_2_range(config.range);
}

uint8_t BMX055::set_bandwidth_reg(uint8_t bandwidth)
{
    bmx_acc_conf_t config;

    if (_get_accel_conf(&config) != ACC_OK)
    {
        return ACC_ERR_RD;
    }
    
    config.bw = _bw_2_reg_value(bandwidth);

    if (_set_accel_conf(&config) != ACC_OK)
    {
        return ACC_ERR_WR;
    }
    
    return ACC_OK;
}

uint8_t BMX055::get_bandwidth_value(void)
{
    bmx_acc_conf_t config;

    if (_get_accel_conf(&config) != ACC_OK)
    {
        return ACC_ERR_RD;
    }
    
    return _reg_value_2_bw(config.bw);
}

accel_error_e BMX055::get_int_status(uint8_t &int_status)
{
    return ACC_OK;
}

accel_error_e BMX055::read_accelerations_packet(acc_packet_t &dataBuffer)
{
    sensor_3D_data_t data;
    if (read_acc_data(&data) != ACC_OK)
    {
        return ACC_ERR_RD;
    }

    dataBuffer.acc_x = data.x;
    dataBuffer.acc_y = data.y;
    dataBuffer.acc_z = data.z;

    return ACC_OK;
};

/*!
 * @details This API reads the chip ID value of the connected device
 *
 * @param[out] chipID_value  : Pointer to data buffer to store the read chip id
 *
 * @return Result of API execution status
 * @return 0 -> Success
 * @return != 0 -> Fail
 */
accel_error_e BMX055::_get_chipID(uint8_t *chipID_value)
{
    return _read_regs(BMX_REG_CHIPID, chipID_value, 1);
}

/*!
 * @details This API read the acc conf registers and store them into a struct
 *
 * @param[out] acc_conf_struct  : Pointer to bmx_acc_conf struct to store the actual conf
 *
 * @return Result of API execution status
 * @return 0 -> Success
 * @return != 0 -> Fail
 */
accel_error_e BMX055::_get_accel_conf(bmx_acc_conf_t *acc_conf_struct)
{
    uint8_t reg_data[2];

    if (acc_conf_struct != NULL)
    {
        /* Get the values in the accel configuration registers */
        if (_read_regs(BMX_REG_PMU_RANGE, reg_data, 2) == ACC_OK)
        {
            acc_conf_struct->range = BMX_GET_BITS_POS_0(reg_data[0], BMX_RANGE);
            acc_conf_struct->bw = BMX_GET_BITS_POS_0(reg_data[1], BMX_BW);

            if (_read_regs(BMX_REG_ACCD_HBW, &reg_data[0], 1) == ACC_OK)
            {
                acc_conf_struct->shadow_dis = BMX_GET_BITS(reg_data[0], BMX_SHADOW_DIS);
                acc_conf_struct->data_high_bw = BMX_GET_BITS(reg_data[0], BMX_DATA_HIGH_BW);
            }
            else
            {
                return ACC_ERR_RD;
            }
        }
        else
        {
            return ACC_ERR_RD;
        }
    }
    else
    {
        return ACC_ERR_NULL_POINTER;
    }

    return ACC_OK;
}

/*!
 * @details This API read an acc conf data struct and write the acc conf registers according to the strut value
 *
 * @param[in] acc_conf_struct  : Pointer to bmx_acc_conf struct which is to be written
 *
 * @return Result of API execution status
 * @return 0 -> Success
 * @return != 0 -> Fail
 */
accel_error_e BMX055::_set_accel_conf(bmx_acc_conf_t *acc_conf_struct)
{
    uint8_t reg_data[2];

    if (acc_conf_struct != NULL)
    {
        if (_read_regs(BMX_REG_PMU_RANGE, reg_data, 2) == ACC_OK)
        {
            reg_data[0] = BMX_SET_BITS_POS_0(reg_data[0], BMX_RANGE, acc_conf_struct->range);
            reg_data[1] = BMX_SET_BITS_POS_0(reg_data[1], BMX_BW, acc_conf_struct->bw);

            /* Set the values in the accel configuration registers */
            if (_write_regs(BMX_REG_PMU_RANGE, reg_data, 2) == ACC_OK)
            {
                reg_data[0] = 0;
                reg_data[0] = BMX_SET_BITS_POS_0(reg_data[0], BMX_SHADOW_DIS, acc_conf_struct->shadow_dis);
                reg_data[0] = BMX_SET_BITS_POS_0(reg_data[0], BMX_DATA_HIGH_BW, acc_conf_struct->data_high_bw);

                if (_write_regs(BMX_REG_ACCD_HBW, &reg_data[0], 1) != ACC_OK)
                {
                    return ACC_ERR_WR;
                }
            }
            else
            {
                return ACC_ERR_WR;
            }
        }
        else
        {
            return ACC_ERR_RD;
        }
    }
    else
    {
        return ACC_ERR_NULL_POINTER;
    }

    return ACC_OK;
}

/*!
 *  @brief This API is used to get fifo configuration of the sensor.
 */
accel_error_e BMX055::_get_fifo_config(bmx_fifo_conf_t *fifo_conf_struct)
{
    uint8_t reg_data;

    if (fifo_conf_struct != NULL)
    {
        if (_read_regs(BMX_REG_FIFO_STATUS, &reg_data, 1) == ACC_OK)
        {
            fifo_conf_struct->fifo_frame_count = BMX_GET_BITS_POS_0(reg_data, BMX_FIFO_FRAME_COUNT);
            fifo_conf_struct->fifo_overrun = BMX_GET_BITS(reg_data, BMX_FIFO_OVERRUN);

            if (_read_regs(BMX_REG_FIFO_CONFIG_0, &reg_data, 1) == ACC_OK)
            {
                fifo_conf_struct->wm_level = BMX_GET_BITS_POS_0(reg_data, BMX_FIFO_WATER_MARK);
            }
            else
            {
                return ACC_ERR_RD;
            }

            if (_read_regs(BMX_REG_FIFO_CONFIG_1, &reg_data, 1) == ACC_OK)
            {
                fifo_conf_struct->fifo_data_select = BMX_GET_BITS_POS_0(reg_data, BMX_FIFO_DATA_SELECT);
                fifo_conf_struct->fifo_mode_select = BMX_GET_BITS(reg_data, BMX_FIFO_MODE_SELECT);
            }
            else
            {
                return ACC_ERR_RD;
            }
        }
        else
        {
            return ACC_ERR_RD;
        }
    }
    else
    {
        return ACC_ERR_NULL_POINTER;
    }

    return ACC_OK;
}

/*!
 *  @brief This API is used to get fifo configuration of the sensor.
 */
accel_error_e BMX055::_set_fifo_config(bmx_fifo_conf_t *fifo_conf_struct)
{
    uint8_t reg_data;

    if (fifo_conf_struct != NULL)
    {
        if (_read_regs(BMX_REG_FIFO_CONFIG_0, &reg_data, 1) == ACC_OK)
        {
            reg_data = BMX_SET_BITS_POS_0(reg_data, BMX_FIFO_WATER_MARK, fifo_conf_struct->wm_level);
        }
        else
        {
            return ACC_ERR_RD;
        }

        if (_write_regs(BMX_REG_FIFO_CONFIG_0, &reg_data, 1) == ACC_OK)
        {
            _fifo_conf.wm_level = fifo_conf_struct->wm_level;

            if (_read_regs(BMX_REG_FIFO_CONFIG_1, &reg_data, 1) == ACC_OK)
            {
                reg_data = BMX_SET_BITS_POS_0(reg_data, BMX_FIFO_DATA_SELECT, fifo_conf_struct->fifo_data_select);
                reg_data = BMX_SET_BITS(reg_data, BMX_FIFO_MODE_SELECT, fifo_conf_struct->fifo_mode_select);

                if (_write_regs(BMX_REG_FIFO_CONFIG_1, &reg_data, 1) != ACC_OK)
                {
                    return ACC_ERR_WR;
                }

                _fifo_conf.fifo_data_select = fifo_conf_struct->fifo_data_select;
                _fifo_conf.fifo_mode_select = fifo_conf_struct->fifo_mode_select;
            }
            else
            {
                return ACC_ERR_RD;
            }
        }
        else
        {
            return ACC_ERR_WR;
        }
    }
    else
    {
        return ACC_ERR_NULL_POINTER;
    }

    return ACC_OK;
}

/*!
 * @brief This API is used to enable the various interrupts
 */
accel_error_e BMX055::set_interrupt_source(uint32_t int_source_to_en)
{
    uint8_t reg_data[3];

    if (_read_regs(BMX_REG_INT_EN_0, reg_data, 3) == ACC_OK)
    {
        reg_data[0] = BMX_SET_BITS_POS_0(reg_data[0], BMX_INT_EN_0, int_source_to_en);
        reg_data[1] = (uint8_t)(BMX_SET_BITS_POS_0(reg_data[1], BMX_INT_EN_1, int_source_to_en) >> 8);
        reg_data[2] = (uint8_t)(BMX_SET_BITS_POS_0(reg_data[2], BMX_INT_EN_2, int_source_to_en) >> 16);
        printf("Desired Interrupt source bytes value: 0x%x, 0x%x, 0x%x\n", reg_data[0], reg_data[1], reg_data[2]);
        if (_write_regs(BMX_REG_INT_EN_0, &reg_data[0], 1) != ACC_OK)
        {
            return ACC_ERR_WR;
        }
        if (_write_regs(BMX_REG_INT_EN_1, &reg_data[1], 1) != ACC_OK)
        {
            return ACC_ERR_WR;
        }
        if (_write_regs(BMX_REG_INT_EN_2, &reg_data[2], 1) != ACC_OK)
        {
            return ACC_ERR_WR;
        }
    }
    else
    {
        return ACC_ERR_RD;
    }

    return ACC_OK;
}

/*!
 * @brief This API is used to get the various interrupts
 * which are enabled in the sensor
 */
accel_error_e BMX055::get_interrupt_source(bmx_int_scr_u *int_en)
{
    uint8_t reg_data[3];
    uint32_t d_dummy_var;

    if (int_en != NULL)
    {
        if (_read_regs(BMX_REG_INT_EN_0, reg_data, 3) == ACC_OK)
        {
            d_dummy_var = (uint32_t)((uint32_t)(reg_data[2] << 16) | (uint16_t)(reg_data[1] << 8) | reg_data[0]);
            memcpy(int_en, &d_dummy_var, sizeof(d_dummy_var));
        }
        else
        {
            return ACC_ERR_RD;
        }
    }
    else
    {
        return ACC_ERR_NULL_POINTER;
    }

    return ACC_OK;
}

/*!
 * @brief This API is used to get the power mode of the sensor
 */
accel_error_e BMX055::get_power_mode(bmx_power_mode_e *power_mode)
{
    uint8_t reg_data[2];
    uint8_t power_mode_val;

    if (power_mode != NULL)
    {
        if (_read_regs(BMX_REG_PMU_LPW, reg_data, 2) == ACC_OK)
        {
            reg_data[0] = BMX_GET_BITS(reg_data[0], BMX_POWER_MODE_CTRL);
            reg_data[1] = BMX_GET_BITS(reg_data[1], BMX_LOW_POWER_MODE);

            /* Power_mode has the following bit arrangement
             *      {BIT3 : BIT2 : BIT1 : BIT0} =
             *{lowpower_mode : suspend : lowpower_en : deep_suspend}
             */
            power_mode_val = ((uint8_t)(reg_data[1] << 3)) | reg_data[0];

            /* Check if deep suspend bit is enabled. If enabled then assign powermode as deep suspend */
            if (power_mode_val & BMX_DEEP_SUSPEND_MODE)
            {
                /* Device is in deep suspend mode */
                power_mode_val = BMX_DEEP_SUSPEND_MODE;
            }

            *power_mode = (bmx_power_mode_e)power_mode_val;

            /* Store the actual power mode in class value */
            _power_mode = *power_mode;
        }
        else
        {
            return ACC_ERR_RD;
        }
    }
    else
    {
        return ACC_ERR_NULL_POINTER;
    }

    return ACC_OK;
}

/*!
 * @brief This API is used to set the power mode of the sensor
 */
accel_error_e BMX055::set_power_mode(bmx_power_mode_e power_mode)
{
    accel_error_e rslt = ACC_ERR_WR;
    uint8_t reg_data[2];
    uint8_t low_power_mode;

    /* Read the power control registers */
    if (_read_regs(BMX_REG_PMU_LPW, reg_data, 2) == ACC_OK)
    {
        switch (power_mode)
        {
        case BMX_NORMAL_MODE:
        case BMX_DEEP_SUSPEND_MODE:
            rslt = _set_normal_mode();
            break;

        case BMX_LOW_POWER_MODE_1:
        case BMX_SUSPEND_MODE:
            if ((_power_mode == BMX_LOW_POWER_MODE_2) || (_power_mode == BMX_STANDBY_MODE) ||
                (_power_mode == BMX_DEEP_SUSPEND_MODE))
            {
                rslt = _set_normal_mode();
            }

            break;
        case BMX_LOW_POWER_MODE_2:
        case BMX_STANDBY_MODE:
            if ((_power_mode == BMX_LOW_POWER_MODE_1) || (_power_mode == BMX_SUSPEND_MODE) ||
                (_power_mode == BMX_DEEP_SUSPEND_MODE))
            {
                rslt = _set_normal_mode();
            }

            break;
        default:
            rslt = ACC_ERR_INVALID_POWERMODE;
            break;
        }

        if (rslt == ACC_OK)
        {
            /* To overcome invalid powermode state a delay is provided. Since
             * 2 registers are accessed to set powermode */
            vTaskDelay(pdMS_TO_TICKS(1));

            low_power_mode = BMX_GET_BITS(power_mode, BMX_POWER_MODE_EXTRACT);
            reg_data[1] = BMX_SET_BITS(reg_data[1], BMX_LOW_POWER_MODE, low_power_mode);

            power_mode = (bmx_power_mode_e)((uint8_t)power_mode & BMX_POWER_MODE_MASK);
            reg_data[0] = BMX_SET_BITS(reg_data[0], BMX_POWER_MODE_CTRL, power_mode);

            if (_write_regs(BMX_REG_PMU_LPW, &reg_data[0], 1) == ACC_OK)
            {
                /* To overcome invalid powermode state a delay of 450us is provided. Since
                 * 2 registers are accessed to set powermode */
                vTaskDelay(pdMS_TO_TICKS(1));

                if (_write_regs(BMX_REG_LOW_NOISE, &reg_data[1], 1) == ACC_OK)
                {
                    _power_mode = power_mode;
                }
                else
                {
                    return ACC_ERR_WR;
                }
            }
            else
            {
                return ACC_ERR_WR;
            }
        }
        else
        {
            return rslt;
        }
    }
    else
    {
        return ACC_ERR_RD;
    }

    return ACC_OK;
}

/*!
 * @brief This internal API is used to set the powermode as normal.
 */
accel_error_e BMX055::_set_normal_mode()
{
    uint8_t reg_data[2];

    /* Read the power control registers */
    if (_read_regs(BMX_REG_PMU_LPW, reg_data, 2) == ACC_OK)
    {
        if (_power_mode == BMX_DEEP_SUSPEND_MODE)
        {
            /* Soft reset is performed to return to normal mode from deepsuspend mode.
             * Since no read or write operation is possible in deepsuspend mode */
            if (soft_reset() != ACC_OK)
            {
                return ACC_ERR_WR;
            }
        }
        else
        {
            reg_data[0] = BMX_SET_BIT_VAL_0(reg_data[0], BMX_POWER_MODE_CTRL);
            reg_data[1] = BMX_SET_BIT_VAL_0(reg_data[1], BMX_LOW_POWER_MODE);

            if (_write_regs(BMX_REG_PMU_LPW, &reg_data[0], 1) == ACC_OK)
            {
                /* To overcome invalid powermode state a delay is provided. Since
                 * 2 registers are accessed to set powermode */
                vTaskDelay(pdMS_TO_TICKS(1));

                if (_write_regs(BMX_REG_LOW_NOISE, &reg_data[1], 1) != ACC_OK)
                {
                    return ACC_ERR_WR;
                }

                vTaskDelay(pdMS_TO_TICKS(1));
            }
            else
            {
                return ACC_ERR_WR;
            }
        }
    }
    else
    {
        return ACC_ERR_RD;
    }

    return ACC_OK;
}

/*!
 * @brief This API is used to perform soft-reset of the sensor
 * where all the registers are reset to their default values
 */
accel_error_e BMX055::soft_reset()
{
    uint8_t soft_rst_cmd = BMX_SOFT_RESET_CMD;

    /* Soft-reset is done by writing soft-reset command into the register */
    if (_write_regs(BMX_REG_BGW_SOFT_RESET, &soft_rst_cmd, 1) == ACC_OK)
    {
        /* Delay for soft-reset */
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    else
    {
        return ACC_ERR_WR;
    }

    return ACC_OK;
}

/*!
 *  @brief This API gets the interrupt status from the registers.
 */
accel_error_e BMX055::get_int_status(bmx_int_status_t *int_status)
{
    uint8_t reg_data[4];

    if (int_status != NULL)
    {
        if (_read_regs(BMX_REG_INT_STATUS_0, reg_data, 4) == ACC_OK)
        {
            int_status->int_status_0 = reg_data[0];
            int_status->int_status_1 = reg_data[1];
            int_status->int_status_2 = reg_data[2];
            int_status->int_status_3 = reg_data[3];
        }
        else
        {
            return ACC_ERR_RD;
        }
    }
    else
    {
        return ACC_ERR_NULL_POINTER;
    }

    return ACC_OK;
}

accel_error_e BMX055::read_acc_data_DMA(sensor_3D_data_t *accel_data)
{
    uint8_t reg_data[6];
    uint8_t new_data_xyz;
    uint8_t new_data_bit;

    if (accel_data == NULL)
    {
        return ACC_ERR_NULL_POINTER;
    }

    if (_read_regs_dma(BMX_REG_ACCD_X_LSB, reg_data, 6) != ACC_OK)
    {
        return ACC_ERR_RD;
    }

    new_data_bit = BMX_GET_BITS_POS_0(reg_data[4], BMX_NEW_DATA);
    new_data_xyz = (uint8_t)(new_data_bit << 2);
    new_data_bit = BMX_GET_BITS_POS_0(reg_data[2], BMX_NEW_DATA);
    new_data_xyz |= (uint8_t)(new_data_bit << 1);
    new_data_bit = BMX_GET_BITS_POS_0(reg_data[0], BMX_NEW_DATA);
    new_data_xyz |= new_data_bit;

    if (new_data_xyz != BMX_NEW_DATA_XYZ)
    {
        return ACC_ERR_NO_NEW_AVAILABLE;
    }

    _convert_reg_data_to_accel(accel_data, reg_data);

    return ACC_OK;
}

accel_error_e BMX055::read_acc_data(sensor_3D_data_t *accel_data)
{
    uint8_t reg_data[6];
    uint8_t new_data_xyz;
    uint8_t new_data_bit;

    if (accel_data == NULL)
    {
        return ACC_ERR_NULL_POINTER;
    }

    if (_read_regs(BMX_REG_ACCD_X_LSB, reg_data, 6) != ACC_OK)
    {
        return ACC_ERR_RD;
    }

    new_data_bit = BMX_GET_BITS_POS_0(reg_data[4], BMX_NEW_DATA);
    new_data_xyz = (uint8_t)(new_data_bit << 2);
    new_data_bit = BMX_GET_BITS_POS_0(reg_data[2], BMX_NEW_DATA);
    new_data_xyz |= (uint8_t)(new_data_bit << 1);
    new_data_bit = BMX_GET_BITS_POS_0(reg_data[0], BMX_NEW_DATA);
    new_data_xyz |= new_data_bit;

    if (new_data_xyz != BMX_NEW_DATA_XYZ)
    {
        return ACC_ERR_NO_NEW_AVAILABLE;
    }

    _convert_reg_data_to_accel(accel_data, reg_data);

    return ACC_OK;
}

/*!
 *  @brief This API is used to read the FIFO data from FIFO data register
 */
accel_error_e BMX055::read_fifo_data()
{
    uint16_t fifo_data_byte_count = 0;

    if (get_fifo_frame_count() == ACC_OK)
    {
        if (_fifo_conf.fifo_data_select == BMX_XYZ_AXES)
        {
            fifo_data_byte_count = (uint16_t)(_fifo_conf.fifo_frame_count * BMX_FIFO_XYZ_AXIS_FRAME_SIZE);
        }
        else
        {
            fifo_data_byte_count = (uint16_t)(_fifo_conf.fifo_frame_count * BMX_FIFO_SINGLE_AXIS_FRAME_SIZE);
        }
    }
    else
    {
        return ACC_ERR_READ_FIFO_CONFIG;
    }

    if (_fifo_conf.length > fifo_data_byte_count)
    {
        /* Handling the case where user requests
         * more data than available in FIFO */
        _fifo_conf.length = fifo_data_byte_count;
    }

    /* Read only the filled bytes in the FIFO Buffer */
    if (_read_regs_dma(BMX_REG_FIFO_DATA, _FIFO_data, _fifo_conf.length) != ACC_OK)
    {
        return ACC_ERR_RD_DMA;
    }

    return ACC_OK;
}

accel_error_e BMX055::get_fifo_frame_count()
{
    uint8_t reg_data;

    if (_read_regs(BMX_REG_FIFO_STATUS, &reg_data, 1) == ACC_OK)
    {
        _fifo_conf.fifo_frame_count = BMX_GET_BITS_POS_0(reg_data, BMX_FIFO_FRAME_COUNT);
        _fifo_conf.fifo_overrun = BMX_GET_BITS(reg_data, BMX_FIFO_OVERRUN);
    }
    else
    {
        return ACC_ERR_RD;
    }

    return ACC_OK;
}

accel_error_e BMX055::fifo_extract_frames(sensor_3D_data_t *accel_data, uint16_t *acc_index)
{
    int8_t rslt = 0;
    uint16_t data_index = 0;
    uint16_t index = 0;

    if ((accel_data != NULL) && (acc_index != NULL))
    {
        for (; (data_index < _fifo_conf.length) && (rslt != ACC_ERR_FIFO_FRAME_EMPTY) && (rslt != ACC_ERR_INVALID_CONFIG);)
        {
            rslt = decode_fifo_frames(&accel_data[index], &data_index);

            index++;

            /* Update the valid frame count */
            *acc_index = index;
        }
    }
    else
    {
        return ACC_ERR_NULL_POINTER;
    }

    return ACC_OK;
}

/*!
 *  @brief This internal API is used to unpack the accel data.
 */
accel_error_e BMX055::decode_fifo_frames(sensor_3D_data_t *accel_data, uint16_t *data_index)
{
    accel_error_e rslt;

    switch (_fifo_conf.fifo_data_select)
    {
    case BMX_XYZ_AXES:
        if (!((_FIFO_data[*data_index] == 0) && (_FIFO_data[*data_index + 1] == 0) &&
              (_FIFO_data[*data_index + 2] == 0) && (_FIFO_data[*data_index + 3] == 0) &&
              (_FIFO_data[*data_index + 4] == 0) && (_FIFO_data[*data_index + 5] == 0)))
        {
            /* Accel x data */
            accel_data->x = (int16_t)(((uint16_t)_FIFO_data[*data_index + 1] << 8) | _FIFO_data[*data_index]);

            /* Accel y data */
            accel_data->y = (int16_t)(((uint16_t)_FIFO_data[*data_index + 3] << 8) | _FIFO_data[*data_index + 2]);

            /* Accel z data */
            accel_data->z = (int16_t)(((uint16_t)_FIFO_data[*data_index + 5] << 8) | _FIFO_data[*data_index + 4]);

            *data_index += BMX_ACCEL_DATA_XYZ_AXES_LEN;
            rslt = ACC_OK;
        }
        else
        {
            rslt = ACC_ERR_FIFO_FRAME_EMPTY;

            /* Move the data index to the last byte to mark completion */
            *data_index = _fifo_conf.length;
        }

        break;
    case BMX_X_AXIS:
        rslt = fifo_frame_empty_check(&(*data_index));

        if (rslt == ACC_OK)
        {
            /* Accel x data */
            accel_data->x = (int16_t)(((uint16_t)_FIFO_data[*data_index + 1] << 8) | _FIFO_data[*data_index]);

            /* Accel y data */
            accel_data->y = 0;

            /* Accel z data */
            accel_data->z = 0;

            *data_index += BMX_ACCEL_DATA_SINGLE_AXIS_LEN;
            rslt = ACC_OK;
        }

        break;
    case BMX_Y_AXIS:
        rslt = fifo_frame_empty_check(&(*data_index));

        if (rslt == ACC_OK)
        {
            /* Accel x data */
            accel_data->x = 0;

            /* Accel y data */
            accel_data->y = (int16_t)(((uint16_t)_FIFO_data[*data_index + 1] << 8) | _FIFO_data[*data_index]);

            /* Accel z data */
            accel_data->z = 0;

            *data_index += BMX_ACCEL_DATA_SINGLE_AXIS_LEN;
            rslt = ACC_OK;
        }

        break;
    case BMX_Z_AXIS:
        rslt = fifo_frame_empty_check(&(*data_index));

        if (rslt == ACC_OK)
        {
            /* Accel x data */
            accel_data->x = 0;

            /* Accel y data */
            accel_data->y = 0;

            /* Accel z data */
            accel_data->z = (int16_t)(((uint16_t)_FIFO_data[*data_index + 1] << 8) | _FIFO_data[*data_index]);

            *data_index += BMX_ACCEL_DATA_SINGLE_AXIS_LEN;
            rslt = ACC_OK;
        }

        break;
    default:
        accel_data->x = 0;
        accel_data->y = 0;
        accel_data->z = 0;
        rslt = ACC_ERR_INVALID_CONFIG;
    }

    return rslt;
}

/*!
 *  @brief This internal API is check if the fifo frame is empty.
 */
accel_error_e BMX055::fifo_frame_empty_check(uint16_t *data_index)
{
    if ((_FIFO_data[*data_index] == 0) && (_FIFO_data[*data_index + 1] == 0) && (_FIFO_data[*data_index + 2] == 0) &&
        (_FIFO_data[*data_index + 3] == 0))
    {
        /* Move the data index to the last byte to mark completion */
        *data_index = _fifo_conf.length;

        return ACC_ERR_FIFO_FRAME_EMPTY;
    }
    else
    {
        return ACC_OK;
    }
}

/*!
 * @details This API reads the data from the given register
 * address of the sensor
 *
 * @param[in] reg_addr  : Register address from where the data is to be read
 * @param[out] data_rd  : Pointer to data buffer to store the read data
 * @param[in] length    : No of bytes of data to be read
 *
 * @return Result of API execution status
 * @return 0 -> Success
 * @return != 0 -> Fail
 */
accel_error_e BMX055::_read_regs(uint8_t reg_addr, uint8_t *data_rd, uint32_t length)
{
    esp_err_t ret = ESP_FAIL;

    uint8_t dummy_buffer[length];
    memset(dummy_buffer, 0, sizeof(dummy_buffer[0] * length));

    spi_transaction_t t =
        {
            .addr = (uint64_t)(reg_addr | BMX_SPI_RD_MASK),
            .length = 8 * length,
            .rxlength = 8 * length,
            .tx_buffer = dummy_buffer, // Dummy data buffer
            .rx_buffer = data_rd,
        };

    if ((length > 0) && (data_rd != NULL))
    {
        if (xSemaphoreTake(_xSpiSemaphore, portMAX_DELAY) == pdTRUE)
        {
            ret = spi_device_polling_transmit(_spi, &t);
            xSemaphoreGive(_xSpiSemaphore);
        }
    }
    else
    {
        return ACC_ERR_NULL_POINTER;
    }

    if (ret == 0)
    {
        return ACC_OK;
    }
    else
    {
        return ACC_ERR_RD;
    }
}

/*!
 * @details This API writes the given data to the register address
 * of the sensor.
 *
 * @param[in] reg_addr : Register address where the reg_data is to be written
 * @param[in] data_wr  : Pointer to data buffer which is to be written
 *                       in the reg_addr of sensor
 * @param[in] length   : No of bytes of data to be written
 *
 * @return Result of API execution status
 * @return 0 -> Success
 * @return != 0 -> Fail
 */
accel_error_e BMX055::_write_regs(uint8_t reg_addr, uint8_t *data_wr, uint32_t length)
{
    esp_err_t ret = ESP_FAIL;

    spi_transaction_t t =
        {
            .addr = (uint64_t)(reg_addr & BMX_SPI_WR_MASK),
            .length = 8,
            .tx_buffer = data_wr // Buffer of data to write
        };
    // ! there is a bug inside the spi_device_polling_transmit which cause no data transmit when (t.length > 8)
    if ((length > 0) && (data_wr != NULL))
    {
        for (uint32_t i = 0; i < length; i++)
        {
            t.addr = (uint64_t)((reg_addr + i) & BMX_SPI_WR_MASK),
            t.tx_buffer = &data_wr[i];
            if (xSemaphoreTake(_xSpiSemaphore, portMAX_DELAY) == pdTRUE)
            {
                ret = spi_device_polling_transmit(_spi, &t);
                xSemaphoreGive(_xSpiSemaphore);
            }
        }
    }
    else
    {
        return ACC_ERR_NULL_POINTER;
    }

    if (ret == 0)
    {
        return ACC_OK;
    }
    else
    {
        return ACC_ERR_WR;
    }
}

/*!
 * @details This API reads the data from the given register
 * address of the sensor without interrupt the execution of the task.
 *
 * @param[in] reg_addr  : Register address from where the data is to be read
 * @param[out] data_rd  : Pointer to data buffer to store the read data
 * @param[in] length    : No of bytes of data to be read
 *
 * @return Result of API execution status
 * @return 0 -> Success
 * @return != 0 -> Fail
 */
accel_error_e BMX055::_read_regs_dma(uint8_t reg_addr, uint8_t *data_rd, uint32_t length)
{
    esp_err_t ret = ESP_FAIL;

    uint8_t dummy_buffer[length];
    memset(dummy_buffer, 0, sizeof(dummy_buffer[0] * length));

    spi_transaction_t t =
        {
            .addr = (uint64_t)(reg_addr | BMX_SPI_RD_MASK),
            .length = 8 * length,
            .rxlength = 8 * length,
            .tx_buffer = dummy_buffer, // Dummy data buffer
            .rx_buffer = data_rd,
        };

    if ((length > 0) && (data_rd != NULL))
    {
        if (xSemaphoreTake(_xSpiSemaphore, portMAX_DELAY) == pdTRUE)
        {
            ret = spi_device_transmit(_spi, &t);
            xSemaphoreGive(_xSpiSemaphore);
        }
    }
    else
    {
        return ACC_ERR_NULL_POINTER;
    }

    if (ret == 0)
    {
        return ACC_OK;
    }
    else
    {
        return ACC_ERR_RD;
    }
}

int16_t BMX055::_reg_value_2_range(uint8_t reg_value)
{
    switch (reg_value)
    {
    case BMX_ACC_RANGE_2G:
        return 2;
        break;
    case BMX_ACC_RANGE_4G:
        return 4;
        break;
    case BMX_ACC_RANGE_8G:
        return 8;
        break;
    case BMX_ACC_RANGE_16G:
        return 16;
        break;
    default:
        return 0;
        break;
    }
}

uint8_t BMX055::_range_2_reg_value(int16_t range)
{
    switch (range)
    {
    case 2:
        return BMX_ACC_RANGE_2G;
        break;
    case 4:
        return BMX_ACC_RANGE_4G;
        break;
    case 8:
        return BMX_ACC_RANGE_8G;
        break;
    case 16:
        return BMX_ACC_RANGE_16G;
        break;
    default:
        return 0;
        break;
    }
}

int16_t BMX055::_reg_value_2_bw(uint8_t bw_reg_value)
{
    switch (bw_reg_value)
    {
    case BMX_ACC_BW_7_81_HZ:
        return 8;
        break;
    case BMX_ACC_BW_15_63_HZ:
        return 16;
        break;
    case BMX_ACC_BW_31_25_HZ:
        return 31;
        break;
    case BMX_ACC_BW_62_5_HZ:
        return 62;
        break;
    case BMX_ACC_BW_125_HZ:
        return 125;
        break;
    case BMX_ACC_BW_250_HZ:
        return 250;
        break;
    case BMX_ACC_BW_500_HZ:
        return 500;
        break;
    case BMX_ACC_BW_1000_HZ:
        return 1000;
        break;
    default:
        return 0;
        break;
    }
}

uint8_t BMX055::_bw_2_reg_value(int16_t range)
{
    //TODO
    return 0;
}

void BMX055::_convert_reg_data_to_accel(sensor_3D_data_t *accel, uint8_t *data_array)
{
    uint32_t reg_data;

    /* Accel X axis data */
    reg_data = (uint32_t)(((uint16_t)(data_array[1] << 8) | data_array[0]) >> BMX_RES_SHIFT_VAL);

    if (reg_data > BMX_RES_MAX_VAL)
    {
        /* Computing accel data negative value */
        accel->x = (int16_t)(reg_data - BMX_RES_NEG_VAL);
    }
    else
    {
        accel->x = (int16_t)reg_data;
    }

    /* Accel Y axis data */
    reg_data = (uint32_t)(((uint16_t)(data_array[3] << 8) | data_array[2]) >> BMX_RES_SHIFT_VAL);

    if (reg_data > BMX_RES_MAX_VAL)
    {
        /* Computing accel data negative value */
        accel->y = (int16_t)(reg_data - BMX_RES_NEG_VAL);
    }
    else
    {
        accel->y = (int16_t)reg_data;
    }

    /* Accel Z axis data */
    reg_data = (uint32_t)(((uint16_t)(data_array[5] << 8) | data_array[4]) >> BMX_RES_SHIFT_VAL);

    if (reg_data > BMX_RES_MAX_VAL)
    {
        /* Computing accel data negative value */
        accel->z = (int16_t)(reg_data - BMX_RES_NEG_VAL);
    }
    else
    {
        accel->z = (int16_t)reg_data;
    }
}

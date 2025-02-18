#ifndef INC_APP_H
#define INC_APP_H

#include "../common_def.h"
#include "../DPBshared.hpp"
#include "SignalProcessing.hpp"
#include "motor.hpp"
#include "RPMTracker.hpp"
#include "accelerometer.hpp"
#include "ADXL345_def.h"
#include "ADXL345.h"
#include "BMX055_defs.h"
#include "BMX055.h"

//************************/
//*      DEFINES         */
//************************/
#define INIT_ISR_DONE 1U << 1
#define INIT_ESC_DONE 1U << 2
#define INIT_ACCEL_DONE 1U << 3
#define INIT_RPM_S_DONE 1U << 4
#define READ_SETTINGS_DONE 1U << 5

//*******************************/
//*      CLASS DECLARATION      */
//*******************************/
class DPB
{
public:
    DPB(gpio_num_t esc_gpio_num, gpio_num_t ir_gpio_num);

    int16_t init(QueueHandle_t xQueueSysInput_handle, QueueHandle_t xQueueSysOutput_handle, DPBShared *sharedData);

    void loop(void);
    void loop_rpm(void);
    void loop_accel(void);
    void exe(command_data_t command);
    void ask_acc_charts_update(void);
    void ask_fft_chart_update(void);
    void ask_unbalance_arrow_update(void);
    void ask_unbalance_step_1(void);
    void ask_unbalance_step_2(void);
    void ask_unbalance_step_3(void);
    void ask_unbalance_step_4(void);
    void ask_main_page(void);
    void ask_setting_update(sys_command_e request, int64_t value);

    void set_step(app_steps_e v);
private:
    RPMTracker* _rpmSensor;
    Motor* _motor;
    Accel* _accel;
    QueueHandle_t _xQueueSysInput;
    QueueHandle_t _xQueueSysOutput;
    DPBShared &_xShared = DPBShared::getInstance();
    
    SignalProcessing _analyzer;

    uint8_t _init_done;
    app_steps_e _app_step;
    TimerHandle_t _motorStartupTimer;
    size_t _XpeakCount;
    size_t _YpeakCount;
    app_search_type_e _search_type;
    float_t _steps_amplitude[4] = {0};
    uint16_t _sample_cnt_360;

    void _init_esc(void);
    void _init_rpm(void);
    void _init_accel(void);

    void _exe_start(void);
    void _exe_reset(void);
    void _exe_filter(void);
    void _exe_analyze(void);
    void _exe_fft(uint8_t data_type);
    void _exe_bpf(uint8_t data_type);
    void _exe_step_managment(app_steps_e requested_step);
    void _exe_get_settings(sys_command_e request);
    void _exe_set_settings(command_data_t commnad);
    void _exe_store_settings(void);

    void _reset(void);
    void _unbalance_magnitude_calc(void);
    int16_t _filter_data_iir_zero(data_orig_e data_type);
    int16_t _fft_calc(data_orig_e data_type);
    void _signal_peak_finder(void);
    void _fft_peak_finder(void);
    void _dummy_data_remove(void);

    void _set_searchType(app_search_type_e type);

    void _log_acc_data(void);
    void _log_acc_data_filtered(void);

    void _standard_peaks_analisys(void);
    void _z_scores_peaks_analisys(void);
    void _unbalance_finder_optical(void);
    void _unbalance_finder_steps(void);
    void _unbalance_step_1(void);
    void _unbalance_step_2(void);
    void _unbalance_step_3(void);
    void _unbalance_step_4(void);

    void _settings_read(void);
    int16_t _settings_load(app_settings_t *settings);
    void _settings_apply(void);
    void _settings_write(void);
    int16_t _settings_save(app_settings_t *settings);

    float_t _get_fundamental_freq(uint16_t sample_freq, size_t fft_length, app_unbalance_source_e axis);
    float_t _get_vibe_vector_mod(void);
    int8_t _range_2_gui_value_convert(uint8_t range);

    static void __motorStartupTimerCallback_static(TimerHandle_t pxTimer);
    void __motorStartupTimerCallback(TimerHandle_t pxTimer);
    void __initNVS(void);
};

#endif
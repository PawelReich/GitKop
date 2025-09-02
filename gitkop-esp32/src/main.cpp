#include <Arduino.h>
#include "esp_adc/adc_continuous.h"

const int PULSE_OUT = 2;

hw_timer_t* pulseTimer = NULL;

uint pulseTickCtr = 0;

// Request 1 MSPS
#define SAMPLE_FREQ_HZ     100000 
// One sample per frame
#define CONV_FRAME_SIZE    sizeof(adc_digi_output_data_t)
// DMA buffer holds 1024 samples
#define DMA_BUF_SIZE       (CONV_FRAME_SIZE * 1024)

static const char *TAG = "ADC_MAX_SPEED";
#define PULSE_IN_GPIO     GPIO_NUM_2    // Change to your desired GPIO
#define PULSE_IN_CHANNEL ADC_CHANNEL_1
#define ADC_UNIT_NUM     ADC_UNIT_1

static adc_continuous_handle_t handle = NULL;
static uint8_t *dma_buf = nullptr;
static volatile size_t bytes_avail = 0;

// IRAM buffer pointer and state
static volatile uint16_t IRAM_ATTR latest_sample_value = 0;

// Callback called when new DMA data is ready
static bool IRAM_ATTR on_dma_done(adc_continuous_handle_t h, 
                                  const adc_continuous_evt_data_t *edata, 
                                  void *user_ctx) {
    bytes_avail = edata->size;

    // edata->size = number of bytes just filled into your dma_buf
    // dma_buf contains contiguous adc_digi_output_data_t entries
    size_t bytes = edata->size;
    size_t count = bytes / sizeof(adc_digi_output_data_t);
    // Point to the last sample's data field in IRAM
    auto *data = (adc_digi_output_data_t*)dma_buf;
    // Compute address of the 12-bit raw value
    latest_sample_value = data[count - 1].type2.data & 0x0FFF;
    return false;
}

static volatile bool IRAM_ATTR rdy = false;
static volatile uint16_t IRAM_ATTR val = 0;

void IRAM_ATTR pulseTimerFired()
{
    if (pulseTickCtr == 0)
    {
        digitalWrite(PULSE_OUT, HIGH);
    }

    if (pulseTickCtr == 10)
    {
        digitalWrite(PULSE_OUT, LOW);
    }

    pulseTickCtr++;

    if (pulseTickCtr == 20)
    {
        // val = latest_sample_value;
        // rdy = true;
    }

    if (pulseTickCtr == 50)
    {
        pulseTickCtr = 0;
    }

}

void timer_setup() {
    pulseTimer = timerBegin(10000);
    timerAlarm(pulseTimer, 1, true, 0);
    timerAttachInterrupt(pulseTimer, &pulseTimerFired);
    timerStart(pulseTimer);
}

void adc_setup() {
    // Allocate DMA buffer
    dma_buf = (uint8_t*)heap_caps_malloc(DMA_BUF_SIZE, MALLOC_CAP_DMA);
    ESP_ERROR_CHECK(dma_buf ? ESP_OK : ESP_ERR_NO_MEM);

    // ADC continuous handle
    adc_continuous_handle_cfg_t h_cfg = {
        .max_store_buf_size = DMA_BUF_SIZE,
        .conv_frame_size    = CONV_FRAME_SIZE,
    };
    ESP_ERROR_CHECK(adc_continuous_new_handle(&h_cfg, &handle));

    // Pattern: single channel
    adc_digi_pattern_config_t pattern = {
        .atten     = ADC_ATTEN_DB_12,
        .channel   = PULSE_IN_CHANNEL,
        .unit      = ADC_UNIT_NUM,
        .bit_width = SOC_ADC_RTC_MAX_BITWIDTH,
    };

    adc_continuous_config_t cfg = {
        .pattern_num    = 1,
        .adc_pattern    = &pattern,
        .sample_freq_hz = SAMPLE_FREQ_HZ,
        .conv_mode      = ADC_CONV_SINGLE_UNIT_1,
        .format         = ADC_DIGI_OUTPUT_FORMAT_TYPE2,
    };
    ESP_ERROR_CHECK(adc_continuous_config(handle, &cfg));

    // Register DMA-done callback
    adc_continuous_evt_cbs_t cbs = {
        .on_conv_done = on_dma_done,
    };
    ESP_ERROR_CHECK(adc_continuous_register_event_callbacks(handle, &cbs, NULL));

    // Start ADC continuous conversion
    ESP_ERROR_CHECK(adc_continuous_start(handle));
    ESP_LOGI(TAG, "Started ADC continuous at %d Hz", SAMPLE_FREQ_HZ);
}

void setup()
{
    Serial.begin(250000);

    // Pulse generator setup
    pinMode(PULSE_OUT, OUTPUT);
    timer_setup();
    adc_setup();
}

void loop()
{
    // if (rdy)
    // {
    //     Serial.printf("val: %u\n", val);
    //     rdy = false;
    // }
}

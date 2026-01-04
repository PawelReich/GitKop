#pragma once

typedef struct {
    float alpha;  // Smoothing factor (0.0 to 1.0). Lower = slower/smoother.
    float out;    // Current internal state
} EMA_t;

/**
 * @brief Initialize the EMA filter.
 * * @param filt Pointer to the EMA object.
 * @param alpha Smoothing factor (0.0 to 1.0).
 * 1.0 = no smoothing (output = input).
 * 0.1 = heavy smoothing.
 * @param initial_val Starting value to prevent startup glitches.
 */
static inline void EMA_Init(EMA_t *filt, float alpha, float initial_val) {
    filt->alpha = alpha;
    filt->out = initial_val;
}

/**
 * @brief Update the filter with a new sample.
 * Formula: Out = (Alpha * In) + ((1 - Alpha) * Old_Out)
 * * @param filt Pointer to the EMA object.
 * @param input The new raw sensor value.
 * @return The filtered float value.
 */
static inline float EMA_Update(EMA_t *filt, float input) {
    filt->out = (filt->alpha * input) + ((1.0f - filt->alpha) * filt->out);
    return filt->out;
}

/**
 * @brief Reset the filter output to a specific value immediately.
 * Useful if you detect a large jump and want to reset the average.
 */
static inline void EMA_Reset(EMA_t *filt, float new_val) {
    filt->out = new_val;
}

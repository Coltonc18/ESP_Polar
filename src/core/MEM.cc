#include "./MEM.h"

// Comparison function for qsort
int compare_float(const void* a, const void* b) {
  float fa = *(const float*)a;
  float fb = *(const float*)b;
  return (fa > fb) - (fa < fb);
}

// Cubic spline interpolation
float Interpolate(float* buffer, float t) {
  // Convert t from [0,1] to buffer index range
  float idx = t * (NUM_SAMPLES - 1);
  int i = (int)idx;
  float u = idx - i;

  // Get 4 points for cubic interpolation
  float p0 = buffer[(i - 1 + NUM_SAMPLES) % NUM_SAMPLES];
  float p1 = buffer[i];
  float p2 = buffer[(i + 1) % NUM_SAMPLES];
  float p3 = buffer[(i + 2) % NUM_SAMPLES];

  // Catmull-Rom spline coefficients
  float c0 = -0.5f * p0 + 1.5f * p1 - 1.5f * p2 + 0.5f * p3;
  float c1 = p0 - 2.5f * p1 + 2.0f * p2 - 0.5f * p3;
  float c2 = -0.5f * p0 + 0.5f * p2;
  float c3 = p1;

  // Evaluate cubic polynomial
  return ((c0 * u + c1) * u + c2) * u + c3;
}

// 1. Initialization
void MEM_Init(MEM_Context* ctx) {
  ctx->index = 0;
  ctx->samples_processed = 0;
  memset(ctx->buffer, 0, NUM_SAMPLES * sizeof(float));
  memset(ctx->ar_coeff, 0, MODEL_ORDER * sizeof(float));
  memset(ctx->psd, 0, FREQ_BINS * sizeof(float));
}

// 2. Preprocessing (optimized median filter)
void PreprocessPPI(MEM_Context* ctx, uint16_t measurement) {
  // Add new sample to circular buffer
  ctx->buffer[ctx->index] = measurement;
  ctx->index = (ctx->index + 1) % NUM_SAMPLES;

  // Median filter (5-point window) to remove artifacts
  static float window[5];
  static float filtered[NUM_SAMPLES];

  for (int i = 0; i < NUM_SAMPLES; i++) {
    // Extract 5-point window centered at current sample
    for (int j = -2; j <= 2; j++) {
      int idx = (i + j + NUM_SAMPLES) % NUM_SAMPLES;
      window[j + 2] = ctx->buffer[idx];
    }

    // Sort window to find median
    qsort(window, 5, sizeof(float), compare_float);

    // Replace outlier if it deviates more than 20% from median
    float median = window[2];
    if (ctx->buffer[i] > 1.2f * median || ctx->buffer[i] < 0.8f * median) {
      filtered[i] = median;
    }
    else {
      filtered[i] = ctx->buffer[i];
    }
  }

  // Copy filtered data back to buffer
  memcpy(ctx->buffer, filtered, NUM_SAMPLES * sizeof(float));

  // Resample using cubic spline interpolation
  static float resampled[NUM_SAMPLES];
  for (int i = 0; i < NUM_SAMPLES; i++) {
    float t = (float)i / (NUM_SAMPLES - 1);  // Normalized time [0,1]
    resampled[i] = Interpolate(ctx->buffer, t);
  }

  // Copy resampled data back to buffer
  memcpy(ctx->buffer, resampled, NUM_SAMPLES * sizeof(float));
}

// 3. Burg's Method (optimized for fixed-point)
void BurgsMethod(MEM_Context* ctx) {
  // Initialize arrays for forward/backward errors and reflection coefficients
  static float f_error[NUM_SAMPLES] = { 0 };  // Forward prediction errors
  static float b_error[NUM_SAMPLES] = { 0 };  // Backward prediction errors
  static float k[MODEL_ORDER] = { 0 };        // Reflection coefficients
  static float a[MODEL_ORDER] = { 0 };        // AR coefficients
  static float a_prev[MODEL_ORDER] = { 0 };   // Previous AR coefficients

  // Initialize errors with input data
  memcpy(f_error, ctx->buffer, NUM_SAMPLES * sizeof(float));
  memcpy(b_error, ctx->buffer, NUM_SAMPLES * sizeof(float));

  // Initialize AR coefficients
  memset(a, 0, MODEL_ORDER * sizeof(float));
  memset(a_prev, 0, MODEL_ORDER * sizeof(float));

  // Main Burg recursion
  for (int m = 0; m < MODEL_ORDER; m++) {
    float numerator = 0.0f;
    float denominator = 0.0f;

    // Compute reflection coefficient
    for (int t = m; t < NUM_SAMPLES - 1; t++) {
      numerator += 2.0f * f_error[t] * b_error[t];
      denominator += f_error[t] * f_error[t] + b_error[t + 1] * b_error[t + 1];
    }
    k[m] = -numerator / (denominator + 1e-9f);  // Avoid division by zero

    // Update AR coefficients using Levinson recursion
    a[m] = k[m];
    for (int i = 0; i < m; i++) {
      a[i] = a_prev[i] + k[m] * a_prev[m - 1 - i];
    }

    // Update forward/backward prediction errors
    for (int t = NUM_SAMPLES - 1; t > m; t--) {
      float temp = f_error[t];
      f_error[t] = f_error[t] + k[m] * b_error[t - 1];
      b_error[t] = b_error[t - 1] + k[m] * temp;
    }

    // Save current AR coefficients for next iteration
    memcpy(a_prev, a, (m + 1) * sizeof(float));
  }

  // Store final AR coefficients in context
  memcpy(ctx->ar_coeff, a, MODEL_ORDER * sizeof(float));
}

// Helper function to read from PROGMEM
inline float read_float(const float* addr) {
  float value;
  memcpy_P(&value, addr, sizeof(float));
  return value;
}

// Helper function to read complex exponential from exp_table
inline void read_complex_exp(int i, int f, float* real, float* imag) {
  *real = read_float(&exp_table[i].real[f]);
  *imag = read_float(&exp_table[i].imag[f]);
}

// 4. PSD Calculation (with precomputed exponents)
void ComputePSD(MEM_Context* ctx) {
  const float freq_step = (FREQ_END - FREQ_START) / FREQ_BINS;

  for (int f = 0; f < FREQ_BINS; f++) {
    float denominator_real = 1.0f;
    float denominator_imag = 0.0f;

    // Compute denominator: 1 - sum(a_i * e^(-j2πfi))
    for (int i = 0; i < MODEL_ORDER; i++) {
      float exp_real, exp_imag;
      read_complex_exp(i, f, &exp_real, &exp_imag);

      // Complex multiplication: a_i * e^(-j2πfi)
      float temp_real = ctx->ar_coeff[i] * exp_real;
      float temp_imag = ctx->ar_coeff[i] * exp_imag;

      denominator_real -= temp_real;
      denominator_imag -= temp_imag;
    }

    // Compute power spectrum: 1/|denominator|^2
    float magnitude_squared = denominator_real * denominator_real +
      denominator_imag * denominator_imag;
    ctx->psd[f] = 1.0f / (magnitude_squared + 1e-9f);  // Avoid division by zero by adding a small epsilon
  }
}

// Helper function to integrate PSD over a frequency range
float IntegratePSD(const float* psd, float freq_start, float freq_end) {
  // Convert frequency range to bin indices
  int start_bin = (int)((freq_start - FREQ_START) * FREQ_BINS / (FREQ_END - FREQ_START));
  int end_bin = (int)((freq_end - FREQ_START) * FREQ_BINS / (FREQ_END - FREQ_START));

  // Clamp indices to valid range
  start_bin = (start_bin < 0) ? 0 : (start_bin >= FREQ_BINS ? FREQ_BINS - 1 : start_bin);
  end_bin = (end_bin < 0) ? 0 : (end_bin >= FREQ_BINS ? FREQ_BINS - 1 : end_bin);

  // Ensure start_bin is less than end_bin
  if (start_bin > end_bin) {
    int temp = start_bin;
    start_bin = end_bin;
    end_bin = temp;
  }

  // Calculate frequency step size
  float freq_step = (FREQ_END - FREQ_START) / FREQ_BINS;

  // Integrate using trapezoidal rule
  float integral = 0.0f;
  for (int i = start_bin; i < end_bin; i++) {
    integral += (psd[i] + psd[i + 1]) * 0.5f * freq_step;
  }

  return integral;
}

// 5. Real-Time Update Handler
void ProcessNewPPI(MEM_Context* ctx, uint16_t measurement) {
  PreprocessPPI(ctx, measurement);

  if (++ctx->samples_processed >= NUM_SAMPLES) {
    BurgsMethod(ctx);
    ComputePSD(ctx);

    // Optional: VO2 prediction using LF/HF ratios
    float lf = IntegratePSD(ctx->psd, 0.04f, 0.15f);
    float hf = IntegratePSD(ctx->psd, 0.15f, 0.4f);
    float ratio = lf / hf;
  }
} 
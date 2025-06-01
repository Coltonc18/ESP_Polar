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

  // // Median filter (5-point window) to remove artifacts
  // static float window[5];
  // static float filtered[NUM_SAMPLES];

  // for (int i = 0; i < NUM_SAMPLES; i++) {
  //   // Extract 5-point window centered at current sample
  //   for (int j = -2; j <= 2; j++) {
  //     int idx = (i + j + NUM_SAMPLES) % NUM_SAMPLES;
  //     window[j + 2] = ctx->buffer[idx];
  //   }

  //   // Sort window to find median
  //   qsort(window, 5, sizeof(float), compare_float);

  //   // Replace outlier if it deviates more than 20% from median
  //   float median = window[2];
  //   if (ctx->buffer[i] > 1.2f * median || ctx->buffer[i] < 0.8f * median) {
  //     filtered[i] = median;
  //   }
  //   else {
  //     filtered[i] = ctx->buffer[i];
  //   }
  // }

  // // Copy filtered data back to buffer
  // memcpy(ctx->buffer, filtered, NUM_SAMPLES * sizeof(float));

  // // Resample using cubic spline interpolation
  // static float resampled[NUM_SAMPLES];
  // for (int i = 0; i < NUM_SAMPLES; i++) {
  //   float t = (float)i / (NUM_SAMPLES - 1);  // Normalized time [0,1]
  //   resampled[i] = Interpolate(ctx->buffer, t);
  // }

  // // Copy resampled data back to buffer
  // memcpy(ctx->buffer, resampled, NUM_SAMPLES * sizeof(float));
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
      numerator += f_error[t] * b_error[t-1];
      denominator += f_error[t] * f_error[t] + b_error[t-1] * b_error[t-1];
    }
    k[m] = -2.0f * numerator / (denominator + 1e-9f);  // Avoid division by zero

    // Update AR coefficients using Levinson recursion
    a[m] = k[m];
    for (int i = 0; i < m; i++) {
      a[i] = a_prev[i] + k[m] * a_prev[m - 1 - i];
    }

    // Update forward/backward prediction errors
    for (int t = NUM_SAMPLES - 1; t > m; t--) {
      float temp = f_error[t];
      f_error[t] = f_error[t] + k[m] * b_error[t-1];
      b_error[t] = b_error[t-1] + k[m] * temp;
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
  // Calculate the actual signal variance
  float mean = 0.0f;
  float variance = 0.0f;
  
  // First pass: calculate mean
  for (int i = 0; i < NUM_SAMPLES; i++) {
    mean += ctx->buffer[i];
  }
  mean /= NUM_SAMPLES;
  
  // Second pass: calculate variance
  for (int i = 0; i < NUM_SAMPLES; i++) {
    float diff = ctx->buffer[i] - mean;
    variance += diff * diff;
  }
  variance /= (NUM_SAMPLES - 1);

  // Calculate frequency step for normalization
  const float freq_step = (FREQ_END - FREQ_START) / FREQ_BINS;
  const float norm_factor = 1.0f / (2.0f * M_PI * freq_step);
  
  // Scale factor to convert to ms² and normalize to expected range
  const float scale_factor = 1000.0f / variance;  // Adjust this value based on expected range

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

    // Compute power spectrum: variance / |denominator|^2
    float magnitude_squared = denominator_real * denominator_real + denominator_imag * denominator_imag;
    
    // Normalize the PSD and convert to ms² with scaling
    ctx->psd[f] = (variance * norm_factor * scale_factor) / (magnitude_squared + 1e-9f);
  }
}

// Helper function to integrate PSD over a frequency range
float IntegratePSD(const float* psd, float freq_start, float freq_end) {  
  if (freq_start >= freq_end) {
    return 0.0f;
  }

  // Calculate exact bin positions (can be fractional)
  float start_pos = (freq_start - FREQ_START) * (FREQ_BINS - 1) / (FREQ_END - FREQ_START);
  float end_pos = (freq_end - FREQ_START) * (FREQ_BINS - 1) / (FREQ_END - FREQ_START);
  
  // Get integer bin indices
  int start_bin = (int)start_pos;
  int end_bin = (int)end_pos;
  
  // Clamp to valid range
  start_bin = fmaxf(0, fminf(start_bin, FREQ_BINS - 2));
  end_bin = fmaxf(0, fminf(end_bin, FREQ_BINS - 2));

  // Calculate frequency step size
  float freq_step = (FREQ_END - FREQ_START) / (FREQ_BINS - 1);
  
  // Initialize integral
  float integral = 0.0f;
  
  // Handle fractional start bin
  if (start_pos > start_bin) {
    float frac = start_pos - start_bin;
    float p0 = psd[start_bin];
    float p1 = psd[start_bin + 1];
    integral += (p0 + (p1 - p0) * frac) * (1.0f - frac) * freq_step * 0.5f;
    start_bin++;
  }
  
  // Handle fractional end bin
  if (end_pos < end_bin + 1) {
    float frac = end_pos - end_bin;
    float p0 = psd[end_bin];
    float p1 = psd[end_bin + 1];
    integral += (p0 + (p1 - p0) * frac) * frac * freq_step * 0.5f;
    end_bin--;
  }
  
  // Integrate over complete bins using trapezoidal rule
  for (int i = start_bin; i <= end_bin; i++) {
    integral += (psd[i] + psd[i + 1]) * 0.5f * freq_step;
  }

  return integral;
}

// 5. Real-Time Update Handler
void ProcessNewPPI(MEM_Context* ctx, uint16_t measurement) {
  const float MIN_POWER = 1e-8f;  // Reduced minimum power threshold
  PreprocessPPI(ctx, measurement);

  if (++ctx->samples_processed >= NUM_SAMPLES) {
    BurgsMethod(ctx);
    ComputePSD(ctx);

    // Calculate total power for normalization
    ctx->total_power = IntegratePSD(ctx->psd, 0.003f, 0.4f);  // Full HRV range
    
    // VO2 prediction using LF/HF ratios
    ctx->LF = IntegratePSD(ctx->psd, 0.04f, 0.15f);
    ctx->HF = IntegratePSD(ctx->psd, 0.15f, 0.4f);
    
    // Normalize powers to percentage of total
    if (ctx->total_power > MIN_POWER) {
      ctx->LF = (ctx->LF / ctx->total_power) * 100.0f;
      ctx->HF = (ctx->HF / ctx->total_power) * 100.0f;
    } else {
      ctx->LF = MIN_POWER;
      ctx->HF = MIN_POWER;
    }
    
    // Calculate ratio only if both powers are significant
    if (ctx->LF > MIN_POWER && ctx->HF > MIN_POWER) {
      ctx->LF_HF_Ratio = ctx->LF / ctx->HF;
    } else {
      ctx->LF_HF_Ratio = 1.0f;  // Default to 1.0 if powers are too small
    }
  } else {
    // Initialize values to small non-zero numbers before we have enough samples
    ctx->LF = MIN_POWER;
    ctx->HF = MIN_POWER;
    ctx->LF_HF_Ratio = 1.0f;
  }
} 
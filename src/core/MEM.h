#ifndef MEM_H
#define MEM_H

#include "../utils/Constants.h"
#include "../utils/BoundedQueue.hpp"
#include "../utils/exp_table.h"
#include "../utils/MEM_Types.h"

// Function declarations
int compare_float(const void* a, const void* b);
float Interpolate(float* buffer, float t);
void MEM_Init(MEM_Context* ctx);
void PreprocessPPI(MEM_Context* ctx, uint16_t measurement);
void BurgsMethod(MEM_Context* ctx);
void ComputePSD(MEM_Context* ctx);
float IntegratePSD(const float* psd, float freq_start, float freq_end);
void ProcessNewPPI(MEM_Context* ctx, uint16_t measurement);

#endif // MEM_H

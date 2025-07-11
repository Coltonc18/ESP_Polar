# Heart Rate Variability (HRV) Parameters and Equations

## Important Values:
1. Low Frequency Power
	- Correlated to sympathetic influence $$LF = \int_{0.04}^{0.15} P(f)\;df$$
2. High Frequency Power
	- Correlated to parasympathetic influence $$HF = \int_{0.15}^{0.4} P(f)\;df$$
3. LF/HF Ratio
	- Correlated to global sympathovagal balance
	- $Ratio = LF/HF$
4. Total Power
	-  $TP = \int P(f)\;df$

## 1. Time-Domain Parameters
These are calculated directly from the intervals between consecutive heartbeats (RR intervals).
### a) Mean RR Interval

Equation: $$Mean\;RR = (\sum RR_i) / N$$Where:
$RR_i$ = i-th RR interval
N = Total number of intervals
### b) Standard Deviation of NN Intervals (SDNN)
Equation: $$SDNN = \sqrt{[(1/N) * \sum (RR_i - Mean RR)^2]}$$Represents the overall HRV.
### c) Root Mean Square of Successive Differences (RMSSD)
Equation: $$RMSSD = \sqrt{[(1/(N-1)) * \sum(RR_{(i+1)} - RR_i)^2]}$$Reflects parasympathetic activity.
### d) Percentage of Successive NN Intervals Greater than 50 ms (pNN50)
Equation: $$pNN50 = [Number\;of\;|RR_{(i+1)} - RR_i| > 50\;ms / (N-1)] * 100$$
## 2. Frequency-Domain Parameters
These are derived from the power spectral density (PSD) of RR intervals.
### a) Total Power (TP)
Equation: $$TP = \int P(f)\;df$$over the entire frequency range.
### b) Very Low Frequency (VLF) Power
Equation: $$VLF = \int_{0.003}^{0.04} P(f)\;df$$from 0.003 to 0.04 Hz.
### c) Low Frequency (LF) Power
Equation: $$LF = \int_{0.04}^{0.15} P(f)\;df$$from 0.04 to 0.15 Hz.
### d) High Frequency (HF) Power
Equation: $$HF = \int_{0.15}^{0.4} P(f)\;df$$from 0.15 to 0.4 Hz.
### e) LF/HF Ratio
Equation: $$\text{LF/HF Ratio = LF Power / HF Power}$$
## 3. Nonlinear Parameters
### a) SD1 (Short-Term Variability)
Equation: $$SD1 = \sqrt{[1/2 * Var(\Delta RR)]}$$
### b) SD2 (Long-Term Variability)
Equation: $$SD2 = \sqrt{[2 * SDNN^2 - SD1^2]}$$
### c) Approximate Entropy (ApEn)
Equation: $$ApEn(m, r) = \phi^m(r) - \phi^{(m+1)}(r)$$
### d) Sample Entropy (SampEn)
Equation: $$SampEn(m, r) = -\ln(A/B)$$Where:
$A$ = Number of matches for $m+1$
$B$ = Number of matches for $m$
### e) Detrended Fluctuation Analysis (DFA)
Equation: $$F(n) = \sqrt{[(1/N) \sum (y(i) - y_n(i))^2]}$$
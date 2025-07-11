> Goal: Output analog voltages from the ESP32 to the powerlab which correlate to the most recent PPI measurement.
 
- Handle erraneous measurements from the Polar device by throwing them out and continuing to transmit the previous value until it is time to transmit the next one
- Use a FIFO buffer to handle the flow of data from the polar to the microcontroller
- Eventually implement a rolling average to store the most recent $n$ measurements and transmit that value instead
	- This will help with error values to transmit the average continuously

## Hardware Implementation
The ESP32-S3 does **not** have an onboard DAC like the ESP32 does. This means that we have two options: 
1. Use an external DAC connected over $I^2 C$ or $SPI$.
2. Use PWM output from the ESP through an RC Filter to simulate a DC voltage
#### Exploring Option 2: PWM to RC Filter
PWM works by rapidly switching between **HIGH (3.3V)** and **LOW (0V)**. The **duty cycle** (percentage of time the signal is HIGH) determines the average output voltage.
$$V_{out}=Duty\;Cycle\times V_{max}$$
For ESP32-S3 (3.3V max voltage):
$$V_{out}=\frac{D}{2^{res}-1} \times 3.3V$$
Since PWM is a square wave, we need an **RC (Resistor-Capacitor) filter** to smooth it into a stable DC voltage.
A simple **RC Low-Pass Filter** consists of $V_{out}$ connected between:
- **Resistor (R)**: 10kΩ
- **Capacitor (C)**: 10 $\mu F$
	- Increasing this (like to 47 $\mu F$) will smooth the output further
	- At the expense of increasing delay of output (which is irrelevant in this project)
**Connection path**: GPIO $\rightarrow$ resistor $\rightarrow$ $V_{out} \rightarrow$ capacitor $\rightarrow$ GND
```c++
const int pwmPin = 21;          // Use any PWM-capable GPIO
const int pwmChannel = 0;       // ESP32-S3 supports 0-15 pwmChannels
const int pwmResolution = 12;   // 12-bit resolution (0-4095) (theoretical 0.81 mV accuracy(!) )
const int pwmFrequency = 5000;  // 5 kHz. Increase up to 10 kHz if ripple observed. At 12-bit resolution, max is 19 kHz.

void setup() {
  ledcSetup(pwmChannel, pwmFrequency, pwmResolution);  // setup freq, res
  ledcAttachPin(pwmPin, pwmChannel);  // Assign PWM 0 to channel 21
}

void loop() {
  int maxValue = (1 << pwmResolution) - 1;
  // ramp up from 0V to 3.3V and then back down
  for (int i = 0; i <= maxValue; i++) {
    ledcWrite(pwmChannel, i);
    delay(10);  // Adjust delay based on response time needed
  }
  delay(1000);
  for (int i = maxValue; i >= 0; i--) {
    ledcWrite(pwmChannel, i);
    delay(10);
  }
  delay(1000);
}
```

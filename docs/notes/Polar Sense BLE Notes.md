## Workflow
ESP32 device scans nearby bluetooth devices until one which has a name containing "Polar Sense" is found. It then attempts to establish a BLE connection with this device.
- Creates a BLE Client which is used to connect to the server
- Increases the MTU (Maximum Transmission Unit) from default 23 bytes to 232 bytes 
	- necessary for SDK mode to work correctly
- Searches for the UUID associated with the Control Service and Characteristic
	- this is the "port" used to send commands to the Sense
	- and the "port" from which we will receive responses and data transmission from sensors

| Description                 | UUID                                 |
| --------------------------- | ------------------------------------ |
| PMD Service                 | FB005C80-02E7-F387-1CAD-8ACD2D8DF0C8 |
| PMD Control Point           | FB005C81-02E7-F387-1CAD-8ACD2D8DF0C8 |
| PMD Data MTU Characteristic | FB005C82-02E7-F387-1CAD-8ACD2D8DF0C8 |
- Registers for notifications from the data characteristic
	- assigns a callback function to parse the received packed upon reception
	- this port is where sensor data is received
- Registers for indications from the control characteristic
	- assigns a callback function to handle received packets
	- these packets are the responses from the device after we send a command
- Device is now ready to receive commands over the BLE protocol
### Command Breakdown
> All commands are sent to the PMD Control Point

| Index | Byte                                 | Meaning                                                                                                                                                                     |
| ----- | ------------------------------------ | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 0     | 0x01                                 | Get measurement settings                                                                                                                                                    |
|       | 0x02                                 | Start streaming                                                                                                                                                             |
|       | 0x03                                 | End streaming                                                                                                                                                               |
| 1     | 0x00                                 | ECG                                                                                                                                                                         |
|       | 0x01                                 | PPG                                                                                                                                                                         |
|       | 0x02                                 | Acceleration                                                                                                                                                                |
|       | 0x03                                 | PPI                                                                                                                                                                         |
|       | 0x05                                 | Gyroscope                                                                                                                                                                   |
|       | 0x06                                 | Magnetometer                                                                                                                                                                |
|       | 0x09                                 | SDK Mode                                                                                                                                                                    |
|       | 0x0A                                 | Location                                                                                                                                                                    |
|       | 0x0B                                 | Pressure                                                                                                                                                                    |
|       | 0x0C                                 | Temperature                                                                                                                                                                 |
| 2     | 0x00<br>0x01<br>0x02<br>0x03<br>0x04 | **2 bytes** - Sample Rate (Hz)<br>**2 bytes** - Resolution (bits)<br>**2 bytes** - Range (+/- Unit)<br>**2 bytes** - Range (milliUnit)<br>**1 byte**   - Number of Channels |
| 3, 4? | Data                                 | Indicates value to write to setting indicated by byte 2                                                                                                                     |
| 4?    | Setting                              | Another setting if byte 2 was 0x04 (width of data 1 byte)                                                                                                                   |
| 5...  |                                      | Continue assigning desired settings to start a stream                                                                                                                       |
## Parsing PPG Data
Data is sent as a byte array over BLE in the form of notifications on the PMD Data MTU Characteristic. Can be received as a String, giving length information for easier parsing.

| Description                   | Location in Packet                                       | Expected Value for PPG                                                                                                                 |
| ----------------------------- | -------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------- |
| Measurement Type              | data[0] (Accel, HR, PPG, etc.)                           | 0x01                                                                                                                                   |
| Timestamp                     | data[1:8]                                                | nanoseconds since epoch (Jan 1st 2000)                                                                                                 |
| FrameType                     | data[9]                                                  | 0x80 (compressed with type 0)<br>0x00 (uncompressed type 0)                                                                            |
| Frame data                    | data[10:12]<br>data[13:15]<br>data[16:18]<br>data[19:21] | 16 bit int for PPG0 (green light)<br>16 bit int for PPG1 (red light)<br>16 bit int for PPG2 (infrared light)<br>16 bit int for Ambient |
| More Frame Data if Compressed | data[22:size-1]                                          | delta compressed values                                                                                                                |
Determine data frame type from FrameType value (data[9]) by bitwise AND operation with mask 0x7F. (All but MSB)

The data frame type describes the bit size of the data as well as the data type. 

Find out if the frame is compressed or uncompressed frame by bitwise AND operation with FrameType byte and mask 0x80.

Measurement type can be parsed from data[0] using bitwise AND with bitmask 0x3F. PPG should yield 0x01.

Frame data is the remaining payload starting from index 10 unil the end of the payload. 
>Note: when parsing samples remember to multiply each sample value by the conversion factor

Data will likely be compressed. Compressed data has been constructed as delta compressed data where the previous value in data is used to calculate the next value as a sum of the two adjacent values (previous + next). The first value is the reference value.
Delta frame has a size determined by the device. The Delta frame size may differ and the number of samples in the frame may differ.

Find both the delta frame size and sample count in front of the delta frame. Initially, the delta frame size is at index $(channels \times ceil(resolution / 8.0)) + 2$. Where resolution is always in full bytes.

First, calculate the reference sample. Define a mask for PMD data field encoding of signed int as $mask = -0x01 << resolution$ where resolution is $ceil(resolution\;bits/8.0)$.
Then take a chunk of values where number of values is determined by the "Resolution in bytes". Number of chunks is equal to the number of used channels. For PPG, there are 4 channels, so $4\times resolution$ = number of values to use in the reference sample calculation.

Next, convert each chunk to signed integer:
Bitwise OR together with the result of current iteration value as unsigned integer (convert value first to unsigned byte) sift left by the current iteration index multiplied by 8. If resulted sample value bitwise AND bitmask is less than zero, then the resulted sample is result of bitwise OR (0xFFFFFFFFu << chunkSize * 8).

Each delta frame must go through operation where each value in frame is being transformed into binary format. Then each sample in binary format is iterated through. If the value currently in iteration 1 is the sample value (initially 0) is ((value bitwise OR 0x01) << 1). (zero remains zero, one remains one.) Finally, if the resulted value is not zero, the resulted value is pushed thorugh bitwise OR operation (Int.MAX_VALUE << bitWidth-1). Now, calculate the actual value by summing up the two adjacent values.

	I now have the SDK mode working correctly on the Verity with PPG at maximum speed and the trace is fantastic. In the end, the best signal seemed to be the sum of the three PPG channels (not ambient) and then throw it through a high pass filter (timeconstant 1s with no low pass filtering required). The delta frame parsing routine is a bit convoluted with the logic being a conversion into an unsigned half-nibble array which can be flexibly parsed with even-value bit lengths.

#### Example Measurement Packet
01 3A 99 80 B6 FA 71 F2 0A 80 52 B9 F7 C4 BD F7 70 C7 F7 49 6B F5 0A 27 88 AB 9E 3D FF A4 1F AD B8 0F B5 2B CF 7B F6 89 FB 8E 38 03 A2 97 FE 7D F4 78 23 3F F7 FF CE FB FE FD FA 9F 47 2F 3D 05 8D 03 3F 7B 02 D6 FF CE 7D FF D9 1F 80 3F 13 91 F3 5E FC EB CE BF EF BF 07 B1 F3 2E FA FB C2 7B 3E BC F5 F6 13 D0 81 0F E2 B3 7F B9 FB 7B 0F AF BE F0 2D 68 0F 00 08 18 4C EF FC 02 85 0F B0 BE E7 24 48 F0 7C 1E CE E3 1E 3F 04 D7 3F C0 C0 EF D1 47 7E 77 03 E3 FB 7F C1 06 F9 4F D0 3F FE 84 6F BF BE FF BF 1B 4F 7C 03 20 7C 8F 7E 07 E8 C3 CF FD 00 C1 BB 0F 3E 06 B5 5F DF BD FD D2 77 EF FC EF F2 6F 2F 3C 02 CB 57 3F 43 0F F0 7B 0F 7B 04 FA 07 B0 3F F9 A8 73 DF 3F F2



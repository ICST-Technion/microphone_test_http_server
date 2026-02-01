this test records audio from I2S microphone model INMP441 , and allows you to download the file for testing on PC or phone

**Uses no audio library, only the built-in I2S library**. The audio is encoded as WAV file not as MP3, there is a limit on the maximum recording time, about 35 seconds. Recordings of up to 10 seconds are recommended for testing.

dependencies -install ESP Async WebServer by ESP32ASYNC
https://github.com/ESP32Async/ESPAsyncWebServer

tested with ESP32 SDK V3.3.6
In the included test file you can hear me speaking at 3 distances - 50cm, 25cm and 5cm from the microphone.

How to use - 
 - Enter your WIFI SSID and password in the code
 - Run the code, when connected the ESP32 will display IP address
 - Open this address with a web browser on a device conencted to the same network as ESP32
 - click "start recording" ONCE
 - click "stop recording" ONCE - it will take 2-3 seconds to update GUI
 - click "download file"


Connections - 
- WS to GPIO32
- SD to GPIO25
- SCK to GPIO33
- GND to GND
- VCC to 3.3V
- L/R pin keep unconnected or connect to 3.3V (left channel mode)

Notes:

Bugs can lead to a corrupted WAV header. Audio data is recorded correctly, but file will not open correctly by your default music player software.

To play a file with corrupted header, open the file in Audacity (open source software) using File-->Import-->Raw Data , and choose  the following settings (or change according to your settings)- 

- Encoding: Signed 16-bit PCM
- Byte order: Default endianness
- Channels: 1 Channel (Mono)
- Start offset: 0
- Amount to import: 100%
- Sample rate: 16000 Hz




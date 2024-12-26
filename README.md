this test records audio from I2S microphone model INMP441 , and allows you to download the file for testing on PC or phone

Uses no audio library, only the built-in I2S library. The audio is encoded as WAV file not as MP3 , there is a limit on the maximum recording time, about 35 seconds. recordings of up to 10 seconds are recommended for testing.

install ESPAsyncWebServer by lacamera V3.1.0
https://github.com/lacamera/ESPAsyncWebServer

use ESP32 SDK version 2.0.17 (because of incompatibility of ESPAsyncWebServer with newer SDK versions)

TODO: There is some bug with corrupted WAV header. After recording and downloading an audio file, if you want to repeat, unplug ESP32 , wait 3 seconds and plug back in.
If your file still cannot be opened in phone/Windows due to corrupted header, open the file in Audacity (open source software) using File-->Import-->Raw Data , and choose  the following settings - 

Encoding:Signed 16-bit PCM
Byte order:Default endianness
Channels:1 Channel (Mono)
Start offset: 0
Amount to import: 100%
Sample rate: 16000 Hz




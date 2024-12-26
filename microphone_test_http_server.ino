#include <driver/i2s.h>
#include <LittleFS.h>
#include <FS.h>
// #include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "html_page.h"  // Include the HTML file

// I2S pins
#define SAMPLE_RATE (16000)
#define I2S_WS 32
#define I2S_SD 25
#define I2S_SCK 33
#define I2S_PORT I2S_NUM_0
#define bufferLen 256

// WiFi credentials
const char *ssid = "XXXXXXXX";
const char *password = "XXXXXXXX";

// Global variables
int32_t sBuffer[bufferLen];
int16_t outputBuffer[bufferLen];
File wavFile;
const int sampleRate = 16000;
const int bitsPerSample = 32;
const int numChannels = 1;
unsigned long recordingStartTime = 0;
bool isRecording = false;

unsigned long lastPrint = 0;
unsigned int totalWritten = 0;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(100); }
  Serial.println("\nSetup starting...");

  // Initialize WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Initialize LittleFS
  Serial.println("Initializing LittleFS...");
  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS Mount Failed");
  }
  if (LittleFS.format()) {
    Serial.println("LittleFS formatted successfully");
  }
  

  // Add debug info about available space
  Serial.printf("Total space: %lu bytes\n", LittleFS.totalBytes());
  Serial.printf("Used space: %lu bytes\n", LittleFS.usedBytes());

  // Initialize I2S
  esp_err_t err = i2s_install();
  if (err != ESP_OK) {
    Serial.printf("Failed to install I2S driver: %d\n", err);
    return;
  }

  err = i2s_setpin();
  if (err != ESP_OK) {
    Serial.printf("Failed to set I2S pins: %d\n", err);
    return;
  }

  err = i2s_start(I2S_PORT);
  if (err != ESP_OK) {
    Serial.printf("Failed to start I2S: %d\n", err);
    return;
  }

  // Configure web server routes
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", index_html);
  });

  server.on("/recording.wav", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/recording.wav", "audio/wav", true);
  });

  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    String json = "{\"isRecording\":" + String(isRecording ? "true" : "false");
    json += ",\"filename\":\"" + String(isRecording ? "recording in progress" : "recording.wav") + "\"}";
    request->send(200, "application/json", json);
  });

  server.on("/start-recording", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!isRecording) {
      startRecording();
      request->send(200, "application/json", "{\"success\":true}");
    } else {
      request->send(200, "application/json", "{\"success\":false,\"error\":\"Already recording\"}");
    }
  });

  server.on("/stop-recording", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (isRecording) {
      stopRecording();
      request->send(200, "application/json", "{\"success\":true}");
    } else {
      request->send(200, "application/json", "{\"success\":false,\"error\":\"Not recording\"}");
    }
  });

  server.begin();
}

void startRecording() {
  if (LittleFS.exists("/recording.wav")) {
    Serial.println("Removing old recording");
    LittleFS.remove("/recording.wav");
  }

  wavFile = LittleFS.open("/recording.wav", "w");
  if (!wavFile) {
    Serial.println("Failed to create file!");
    return;
  }
  Serial.println("File created successfully");

  writeWavHeader(wavFile, sampleRate, 16, numChannels);
  Serial.println("WAV header written");

  delay(50);

  recordingStartTime = millis();
  isRecording = true;
  Serial.println("Recording started...");
}

void stopRecording() {
  if (isRecording && wavFile) {
    unsigned long fileSize = wavFile.size();
    Serial.printf("Recording stopped. File size before header update: %lu bytes\n", fileSize);

    updateWavHeader(wavFile, fileSize);
    wavFile.close();

    // Verify the file
    File readFile = LittleFS.open("/recording.wav", "r");
    if (readFile) {
      Serial.printf("Final file size: %lu bytes\n", readFile.size());
      readFile.close();
    } else {
      Serial.println("Failed to verify final file");
    }

    isRecording = false;
    Serial.println("Recording stopped");
  }
}


void loop() {
  if (isRecording) {
    size_t bytesIn = 0;
    esp_err_t result = i2s_read(I2S_PORT, &sBuffer, bufferLen * sizeof(int32_t), &bytesIn, portMAX_DELAY);

    if (result == ESP_OK) {
      // Convert samples and verify we're getting real data
      static bool dataWarningPrinted = false;
      bool hasNonZeroData = false;

      for (int i = 0; i < bufferLen; i++) {
        int32_t sample = sBuffer[i];

        // Right-align the 24-bit sample from I2S
        sample = sample >> 11;
        // sample = sample / 2048;
        // Clamp to 16-bit range
        // if (sample > 32767) sample = 32767;
        // if (sample < -32768) sample = -32768;

        if (sample & 0x8000) {
          sample |= 0xFFFF0000;
        }

        outputBuffer[i] = (int16_t)sample;

        if (sample != 0) hasNonZeroData = true;
      }

      // Debug output for first loop iteration
      static bool firstTime = true;
      if (firstTime) {
        Serial.println("First 5 samples:");
        for (int i = 0; i < 5; i++) {
          Serial.printf("Raw: %ld, Converted: %d\n", sBuffer[i], outputBuffer[i]);
        }
        firstTime = false;
      }

      if (!hasNonZeroData && !dataWarningPrinted) {
        Serial.println("Warning: All samples are zero - check microphone connection");
        dataWarningPrinted = true;
      }

      size_t bytesWritten = wavFile.write((const uint8_t *)outputBuffer, bufferLen * sizeof(int16_t));
      totalWritten+=bytesWritten;
      if (millis() - lastPrint > 500) {
        Serial.printf("Bytes written: %d\n", totalWritten);
        lastPrint = millis();
        totalWritten=0;
      }
    }
  }
}

esp_err_t i2s_install() {

  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
    .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
    .intr_alloc_flags = 0,
    .dma_buf_count = 8,
    .dma_buf_len = bufferLen,
    .use_apll = false
  };

  return i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
}

esp_err_t i2s_setpin() {
  const i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_SD
  };

  return i2s_set_pin(I2S_PORT, &pin_config);
}

void writeWavHeader(File file, int sampleRate, int bitsPerSample, int numChannels) {
  byte header[44] = { 0 };  // Initialize all to 0

  // RIFF chunk
  header[0] = 'R';
  header[1] = 'I';
  header[2] = 'F';
  header[3] = 'F';
  // Size will be updated later, leave as 0 for now

  // WAVE identifier
  header[8] = 'W';
  header[9] = 'A';
  header[10] = 'V';
  header[11] = 'E';

  // fmt subchunk
  header[12] = 'f';
  header[13] = 'm';
  header[14] = 't';
  header[15] = ' ';
  header[16] = 16;  // Size of fmt chunk
  header[20] = 1;   // Audio format (1 = PCM)
  header[22] = numChannels;

  // Sample rate
  uint32_t sr = sampleRate;
  header[24] = sr & 0xFF;
  header[25] = (sr >> 8) & 0xFF;
  header[26] = (sr >> 16) & 0xFF;
  header[27] = (sr >> 24) & 0xFF;

  // Byte rate = sample rate * num channels * bytes per sample
  uint32_t byteRate = sampleRate * numChannels * (bitsPerSample / 8);
  header[28] = byteRate & 0xFF;
  header[29] = (byteRate >> 8) & 0xFF;
  header[30] = (byteRate >> 16) & 0xFF;
  header[31] = (byteRate >> 24) & 0xFF;

  // Block align = num channels * bytes per sample
  header[32] = numChannels * (bitsPerSample / 8);
  header[34] = bitsPerSample;

  // Data subchunk
  header[36] = 'd';
  header[37] = 'a';
  header[38] = 't';
  header[39] = 'a';
  // Size will be updated later, leave as 0 for now

  file.write(header, sizeof(header));
}

void updateWavHeader(File file, unsigned long fileSize) {
  if (!file) return;

  uint32_t dataSize = fileSize - 44;
  uint32_t riffSize = fileSize - 8;
  byte sizeBuf[4];

  // Update RIFF chunk size
  sizeBuf[0] = riffSize & 0xFF;
  sizeBuf[1] = (riffSize >> 8) & 0xFF;
  sizeBuf[2] = (riffSize >> 16) & 0xFF;
  sizeBuf[3] = (riffSize >> 24) & 0xFF;

  file.seek(4);
  file.write(sizeBuf, 4);

  // Update data chunk size
  sizeBuf[0] = dataSize & 0xFF;
  sizeBuf[1] = (dataSize >> 8) & 0xFF;
  sizeBuf[2] = (dataSize >> 16) & 0xFF;
  sizeBuf[3] = (dataSize >> 24) & 0xFF;

  file.seek(40);
  file.write(sizeBuf, 4);
}
#include <driver/i2s.h>
#include <SD.h>
#include <SPI.h>  // Needed for SD

#define SD_CS 5

#define I2S_WS 15
#define I2S_SD 4
#define I2S_SCK 2
#define I2S_PORT I2S_NUM_0
#define I2S_SAMPLE_RATE   (16000)
#define I2S_SAMPLE_BITS   (16)
#define I2S_READ_LEN      (16 * 1024)
#define RECORD_TIME       (20) //Seconds
#define I2S_CHANNEL_NUM   (1)
#define FLASH_RECORD_SIZE (I2S_CHANNEL_NUM * I2S_SAMPLE_RATE * I2S_SAMPLE_BITS / 8 * RECORD_TIME)

File file;
const char filename[] = "/recording300hzSDmic2.wav";
const int headerSize = 44;
byte header[headerSize];
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  // SPIFFSInit();

  if (!SD.begin(SD_CS, SPI, 4000000)){
    Serial.println("Sd card init failed");
    while(1);
  }
  Serial.println("Sd card init success");

  SD.remove(filename);
  file = SD.open(filename, FILE_WRITE);
  if (!file){
    Serial.println("Failed to create file!");
    while(1);
  }
  wavHeader(header, FLASH_RECORD_SIZE);
  file.write(header, headerSize);
  i2sInit();
  xTaskCreate(i2s_adc, "i2s_adc", 1024 * 6, NULL, 1, NULL);
}

void loop() {
  // put your main code here, to run repeatedly:

}

// void SPIFFSInit(){
//   if(!SPIFFS.begin(true)){
//     Serial.println("SPIFFS initialisation failed!");
//     while(1) yield();
//   }

//   SPIFFS.remove(filename);
//   file = SPIFFS.open(filename, FILE_WRITE);
//   if(!file){
//     Serial.println("File is not available!");
//   }

//   byte header[headerSize];
//   wavHeader(header, FLASH_RECORD_SIZE);

//   file.write(header, headerSize);
//   listSPIFFS();
// }

void i2sInit(){
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = I2S_SAMPLE_RATE,
    .bits_per_sample = i2s_bits_per_sample_t(I2S_SAMPLE_BITS),
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
    .intr_alloc_flags = 0,
    .dma_buf_count = 64,
    .dma_buf_len = 1024,
    .use_apll = 1
  };

  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);

  const i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = -1,
    .data_in_num = I2S_SD
  };

  i2s_set_pin(I2S_PORT, &pin_config);
}


void i2s_adc(void *arg)
{
    
    int i2s_read_len = I2S_READ_LEN;
    int written_bytes = 0;
    size_t bytes_read;

    char* i2s_read_buff = (char*) calloc(i2s_read_len, sizeof(char));

    i2s_read(I2S_PORT, (void*) i2s_read_buff, i2s_read_len, &bytes_read, portMAX_DELAY);
    i2s_read(I2S_PORT, (void*) i2s_read_buff, i2s_read_len, &bytes_read, portMAX_DELAY);
    
    Serial.println(" *** Recording Start *** ");
    while (written_bytes < FLASH_RECORD_SIZE) {
        //read data from I2S bus, in this case, from ADC.
        i2s_read(I2S_PORT, (void*) i2s_read_buff, i2s_read_len, &bytes_read, portMAX_DELAY);
        //example_disp_buf((uint8_t*) i2s_read_buff, 64);
        //save original data from I2S(ADC) into flash.
        // i2s_adc_data_scale(flash_write_buff, (uint8_t*)i2s_read_buff, i2s_read_len);
        // file.write((const byte*) flash_write_buff, i2s_read_len);
        file.write((const byte*) i2s_read_buff, bytes_read);

        written_bytes += bytes_read;
        ets_printf("Sound recording %u%%\n", written_bytes * 100 / FLASH_RECORD_SIZE);
        ets_printf("Never Used Stack Size: %u\n", uxTaskGetStackHighWaterMark(NULL));
    }
    file.close();

    free(i2s_read_buff);
    i2s_read_buff = NULL;

    
    // listSPIFFS();
    vTaskDelete(NULL);
}

void example_disp_buf(uint8_t* buf, int length)
{
    printf("======\n");
    for (int i = 0; i < length; i++) {
        printf("%02x ", buf[i]);
        if ((i + 1) % 8 == 0) {
            printf("\n");
        }
    }
    printf("======\n");
}

void wavHeader(byte* header, int wavSize){
  header[0] = 'R';
  header[1] = 'I';
  header[2] = 'F';
  header[3] = 'F';
  unsigned int fileSize = wavSize + headerSize - 8;
  header[4] = (byte)(fileSize & 0xFF);
  header[5] = (byte)((fileSize >> 8) & 0xFF);
  header[6] = (byte)((fileSize >> 16) & 0xFF);
  header[7] = (byte)((fileSize >> 24) & 0xFF);
  header[8] = 'W';
  header[9] = 'A';
  header[10] = 'V';
  header[11] = 'E';
  header[12] = 'f';
  header[13] = 'm';
  header[14] = 't';
  header[15] = ' ';
  header[16] = 0x10;
  header[17] = 0x00;
  header[18] = 0x00;
  header[19] = 0x00;
  header[20] = 0x01;
  header[21] = 0x00;
  header[22] = 0x01;
  header[23] = 0x00;
  header[24] = 0x80;
  header[25] = 0x3E;
  header[26] = 0x00;
  header[27] = 0x00;
  header[28] = 0x00;
  header[29] = 0x7D;
  header[30] = 0x00;
  header[31] = 0x00;
  header[32] = 0x02;
  header[33] = 0x00;
  header[34] = 0x10;
  header[35] = 0x00;
  header[36] = 'd';
  header[37] = 'a';
  header[38] = 't';
  header[39] = 'a';
  header[40] = (byte)(wavSize & 0xFF);
  header[41] = (byte)((wavSize >> 8) & 0xFF);
  header[42] = (byte)((wavSize >> 16) & 0xFF);
  header[43] = (byte)((wavSize >> 24) & 0xFF);
  
}


// void listSPIFFS(void) {
//   Serial.println(F("\r\nListing SPIFFS files:"));
//   static const char line[] PROGMEM =  "=================================================";

//   Serial.println(FPSTR(line));
//   Serial.println(F("  File name                              Size"));
//   Serial.println(FPSTR(line));

//   fs::File root = SPIFFS.open("/");
//   if (!root) {
//     Serial.println(F("Failed to open directory"));
//     return;
//   }
//   if (!root.isDirectory()) {
//     Serial.println(F("Not a directory"));
//     return;
//   }

//   fs::File file = root.openNextFile();
//   while (file) {

//     if (file.isDirectory()) {
//       Serial.print("DIR : ");
//       String fileName = file.name();
//       Serial.print(fileName);
//     } else {
//       String fileName = file.name();
//       Serial.print("  " + fileName);
//       // File path can be 31 characters maximum in SPIFFS
//       int spaces = 33 - fileName.length(); // Tabulate nicely
//       if (spaces < 1) spaces = 1;
//       while (spaces--) Serial.print(" ");
//       String fileSize = (String) file.size();
//       spaces = 10 - fileSize.length(); // Tabulate nicely
//       if (spaces < 1) spaces = 1;
//       while (spaces--) Serial.print(" ");
//       Serial.println(fileSize + " bytes");
//     }

//     file = root.openNextFile();
//   }

//   Serial.println(FPSTR(line));
//   Serial.println();
//   delay(1000);
// }

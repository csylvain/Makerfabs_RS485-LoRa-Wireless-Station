// Vincent Fix 2023/5/24
// Need Choice Sensor Type !

// #define SENSOR_5_PIN
#define SENSOR_3_PIN

#include <RadioLib.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <HardwareSerial.h>

#include "pin_config.h"

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
HardwareSerial MySerial(1);

#define FREQUENCY 915.0 // the rest of the LoRa defaults are in pin_config.h

SX1276 radio = new Module(LORA_CS, DIO0, LORA_RST, DIO1, SPI, SPISettings());

unsigned char resp[80] = {0};

int humidity = 0;
int tem = 0;
int ph = 0;

float humidity_value = 0.0;
float tem_value = 0.0;

float ph_value = 0.0;

int P_value = 0;
int N_value = 0;
int K_value = 0;

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(4800);
  MySerial.begin(4800, SERIAL_8N1, 23, 22);

  pinMode(Display_LED, OUTPUT);
  pinMode(Display_power, OUTPUT);
  pinMode(RS485_power, OUTPUT);
  delay(50);
  digitalWrite(Display_LED, HIGH);
  digitalWrite(Display_power, HIGH);
  digitalWrite(RS485_power, HIGH);
  delay(1000);
  digitalWrite(Display_LED, LOW);

  Serial.println(F("Serial Begin!"));

  Wire.begin(ESP32_SDA, ESP32_SCL);
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ; // Don't proceed, loop forever
  }
  Serial.println("SSD1306 found");

  delay(1000);
  digitalWrite(Display_LED, HIGH);

  show_logo();

  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
  Serial.println(F("[SX1276] Initialization"));

  int state = radio.begin(FREQUENCY, BANDWIDTH, SPREADING_FACTOR, CODING_RATE, SX127X_SYNC_WORD, OUTPUT_POWER, PREAMBLE_LEN, GAIN);
  if (state == ERR_NONE)
  {
    Serial.println(F("success!"));
  }
  else
  {
    Serial.print(F("failed, code "));
    Serial.println(state);
    for (;;)
      ; // Don't proceed, loop forever
  }
  Serial.println(F("[SX1276] Ready"));
}

void show_logo()
{
  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2); // Draw 2X-scale text
  display.setCursor(10, 0);
  display.println(F("Makerfabs"));
  display.setTextSize(1);
  display.setCursor(10, 16);
  display.println(F("RS485-LoRa"));
  display.display(); // Show initial text
  delay(500);

  // Scroll in various directions, pausing in-between:
  display.startscrollright(0x00, 0x01);
  delay(2500);
  display.startscrolldiagright(0x00, 0x07);
  delay(1500);
  display.startscrolldiagleft(0x00, 0x07);
  delay(1500);
  display.stopscroll();
}

void loop()
{
  Serial.println(F("read and display"));

#ifdef SENSOR_5_PIN
  sensor_read_5pin();
  value_log();
  value_show_5pin(humidity_value, tem_value, ph_value);
  delay(3000);
  NPK_Show(N_value, P_value, K_value);
#endif

#ifdef SENSOR_3_PIN
  sensor_read_3pin();
  value_log();
  value_show_3pin(humidity_value, tem_value);
#endif

  LoRa_transmit();

  digitalWrite(Display_LED, HIGH); // LED Off
  delay(3000);
  digitalWrite(Display_LED, LOW);  // LED On (disable/skip to save power)
}

void LoRa_transmit()
{
  char temp[32];
  sprintf(temp, "    = T:%.1fC H:%.1f%%", tem_value, humidity_value);
  int state = radio.transmit(temp);
  
  if (state == ERR_NONE)
  {
    // the packet was successfully transmitted
    Serial.println(F("TX okay!"));

    // print measured data rate
    Serial.print(F("[SX1276] Datarate:\t"));
    Serial.print(radio.getDataRate());
    Serial.println(F(" bps"));
  }
  else if (state == ERR_PACKET_TOO_LONG)
  {
    // the supplied packet was longer than 256 bytes
    Serial.println(F("too long!"));
  }
  else if (state == ERR_TX_TIMEOUT)
  {
    // timeout occurred while transmitting packet
    Serial.println(F("timeout!"));
  }
  else
  {
    // some other error occurred
    Serial.print(F("failed, code "));
    Serial.println(state);
  }
}
void sensor_read_3pin()
{
  unsigned char ask_cmd[8] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x02, 0xC4, 0x0B};
  MySerial.write(ask_cmd, 8);
  int i = 0;

  while (MySerial.available() > 0 && i < 80)
  {
    resp[i] = MySerial.read();
    i++;

    yield();
  }

  Serial.print(F("Answer Length:"));
  Serial.println(i);

  char temp[20];
  for (int j = 0; j < 19; j++)
  {
    sprintf(temp, "%02X ", (int)resp[j]);
    Serial.printf(temp);
  }
  Serial.print("\n");

  humidity = CaculateValue((int)resp[3], (int)resp[4]);
  humidity_value = humidity * 0.1;
  tem = CaculateValue((int)resp[5], (int)resp[6]);
  tem_value = tem * 0.1;
}

void sensor_read_5pin()
{
  unsigned char ask_cmd[8] = {0x01, 0x04, 0x00, 0x00, 0x00, 0x07, 0xB1, 0xC8};
  MySerial.write(ask_cmd, 8);
  int i = 0;

  while (MySerial.available() > 0 && i < 80)
  {
    resp[i] = MySerial.read();
    i++;

    yield();
  }

  Serial.print(F("Answer Length:"));
  Serial.println(i);

  char temp[20];
  for (int j = 0; j < 19; j++)
  {
    sprintf(temp, "%02X ", (int)resp[j]);
    Serial.printf(temp);
  }
  Serial.print("\n");

  humidity = CaculateValue((int)resp[3], (int)resp[4]);
  humidity_value = humidity * 0.1;
  tem = CaculateValue((int)resp[5], (int)resp[6]);
  tem_value = tem * 0.1;
  ph = CaculateValue((int)resp[9], (int)resp[10]);
  ph_value = ph * 0.1;
  N_value = CaculateValue((int)resp[11], (int)resp[12]);
  P_value = CaculateValue((int)resp[13], (int)resp[14]);
  K_value = CaculateValue((int)resp[15], (int)resp[16]);
}

int CaculateValue(int x, int y)
{
  int t = 0;
  t = x * 256;
  t = t + y;
  return t;
}

void value_show_3pin(float h, float t)
{
  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2); // Draw 2X-scale text
  display.setCursor(2, 0);
  display.print(F("T:"));
  display.print(t, 1);
  display.print(F("C"));

  display.setCursor(2, 18);
  display.print(F("H:"));
  display.print(h, 1);
  display.print(F("%"));

  display.display(); // Show initial text
}

void value_show_5pin(float h, float t, float ph_f)
{
  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1); // Draw 2X-scale text
  display.setCursor(2, 0);
  display.print(F("T:"));
  display.print(t, 1);
  display.print(F("C"));

  display.setCursor(66, 0);
  display.print(F("H:"));
  display.print(h, 1);
  display.print(F("%"));

  display.setCursor(2, 16);
  display.print(F("PH:"));
  display.print(ph_f, 1);

  display.display(); // Show initial text
}

void NPK_Show(int N, int P, int K)
{
  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1); // Draw 2X-scale text
  display.setCursor(2, 0);
  display.print(F("N:"));
  display.print(N);
  // display.print(F("C"));

  display.setCursor(66, 0);
  display.print(F("P:"));
  display.print(P);
  // display.print(F("%"));

  display.setCursor(2, 16);
  display.print(F("K:"));
  display.print(K);
  display.print(F(" mg/kg"));

  display.display(); // Show initial text
}

void value_log()
{
  Serial.print(F("humidity:"));
  Serial.println(humidity);

  Serial.print(F("humidity_value:"));
  Serial.println(humidity_value);
  Serial.print(F("therm_value:"));
  Serial.println(tem_value);

#ifdef SENSOR_5_PIN
  Serial.print(F("ph_value:"));
  Serial.println(ph_value);

  Serial.print(F("N= "));
  Serial.print(N_value);
  Serial.println(F(" mg/kg"));
  Serial.print(F("P= "));
  Serial.print(P_value);
  Serial.println(F(" mg/kg"));
  Serial.print(F("K= "));
  Serial.print(K_value);
  Serial.println(F(" mg/kg"));
#endif
}
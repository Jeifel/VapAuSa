//-------------- DS18B20 Temperature sensor ----------
#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 8 // connect center pin of DS18B20 to this pin
#define TEMPOFFSET 0.4

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

//------------- SSD1360 Display ---------------------
#include <Wire.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// Arduino UNO/Nano/ProMini:       A4(SDA), A5(SCL)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet: 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//--------------------- setup -------------------
void setup() {
  Serial.begin(9600);

  // ---- DS18B20 Temperature sensor ----
  sensors.begin();

  // ---- SSD1306 OLED Diplay ----
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // lower display brightness
  display.ssd1306_command(SSD1306_SETCONTRAST);
  display.ssd1306_command(1); 
  
  // clear display buffer
  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.setTextSize(3);
  display.println("Ahoi!");
  display.display();
}

//---------------------------------------

float get_temperature(){
  sensors.requestTemperatures();
  float tempC = sensors.getTempCByIndex(0);
  if(tempC == DEVICE_DISCONNECTED_C){
    tempC = -99;
  }else{
    tempC = tempC-TEMPOFFSET;
  }
  return tempC;
  
}

void display_temperature(){
  display.clearDisplay();
  display.setCursor(0,0);
  display.print(get_temperature());
  display.print((char)247);
  display.println('C');
  display.display();
  
}

void loop() {
  display_temperature();
  delay(1000);
}

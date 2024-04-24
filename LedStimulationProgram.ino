#include <SPI.h>
#include <FastLED.h>
#include <esp_now.h>
#include <WiFi.h>

#define FASTLED_ESP32_RAW_PIN_ORDER
#define FASTLED_ALLOW_INTERRUPTS 0

CRGB leds[12];


struct control_message_to_LED
{
  bool lightActive=true;
  int R, G, B;
  //R defines how much red
  //G defines how much green
  //B defines how much blue
  //CRGB lightColor = CRGB(R, G, B);
  float lightFrequency;
  int minDuration;
  int maxDuration;
  int a1, a2, a3, b1, b2, b3, c1, c2, c3;
  //a defines the first color
  //b defines the 2nd color
  //c defines the 3rd color
  //1 defines how much red
  //2 defines how much green
  //3 defines how much blue
  //char CRGB defaultPattern[3] = 
  //{
  //  CRGB(a1, a2, a3),  //a1, a2, a3
  //  CRGB(b1, b2, b3),   //b1, b2, b3,
  //  CRGB(c1, c2, c3)    //c1, c2, c3
  //};
  int patternDuration;
  int mode;//color=0, pattern=1, breathing=2, random=3 or other number
  bool ISREnabled;
};

control_message_to_LED message; // Define an instance of the struct
//callback function that will be executed when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) 
{
  // Process the received message
  memcpy(&message, incomingData, sizeof(message));
  Serial.println(message.lightActive);
  Serial.println(message.R);
  Serial.println(message.G);
  Serial.println(message.B);
  Serial.println(message.lightFrequency);
  Serial.println(message.minDuration);
  Serial.println(message.maxDuration);
  Serial.println(message.a1);
  Serial.println(message.a2);
  Serial.println(message.a3);
  Serial.println(message.b1);
  Serial.println(message.b2);
  Serial.println(message.b3);
  Serial.println(message.c1);
  Serial.println(message.c2);
  Serial.println(message.c3);
  Serial.println(message.patternDuration);
  Serial.println(message.mode);
  Serial.println(message.ISREnabled);
  // Now you can use the received data as needed
  CRGB lightColor = CRGB(message.R, message.G, message.B);
  CRGB defaultPattern[3] = 
  {
    CRGB(message.a1, message.a2, message.a3),  //a1, a2, a3
    CRGB(message.b1, message.b2, message.b3),   //b1, b2, b3
    CRGB(message.c1, message.c2, message.c3)    //c1, c2, c3
  };
  //If ISR is Enabled 
  if(message.ISREnabled)
  {
    message.lightActive=!message.lightActive;
  }
  //attachInterrupt(ISREnabled, toggleLight(lightActive), CHANGE); message.ISREnabled when tr
  if (message.patternDuration == 0) 
  {
    message.patternDuration = 999999999999999;
  }
  if (message.maxDuration == 0) 
  {
    message.maxDuration = 999999999999999;
  }
    if (message.lightActive) 
    {
      //color is enabled
      if (message.mode==0) 
      {
        singleFrequency(message.lightFrequency, lightColor, message.minDuration, message.maxDuration);
      } 
      //pattern is enabled
      else if(message.mode==1)
      {
        pattern(defaultPattern, message.patternDuration);
      }
      //breathing is enabled
      else if(message.mode==2)
      {
        breathing(message.lightFrequency, lightColor, message.minDuration, message.maxDuration);
      }
      //random mode is enabled
      else
      {
        int time = random(4, 12);
        int randomLED= random(1, 12); 
        int red=random(20, 220);
        int green=random(20, 220);
        int blue=random(20, 220);
        for (int z; z<time; z++)
        {
          leds[randomLED] = CRGB(red, green, blue);
          FastLED.setBrightness(100);
          FastLED.show();
          delay(1000);
          randomLED= random(1, 12); 
        }
      }
    }
    message.lightActive=false;//because done with light pattern;
}
//Setups everything defines the hardware SPI pins explicitly for ESP32 and which ports are in used.
//This SHOULD not be changed unless the ports change for the ESP32 board.
void setup() 
{
  Serial.begin(115200);//may need to change and add more
    //Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  //Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packer info
  esp_now_register_recv_cb(OnDataRecv);
  FastLED.addLeds<WS2812B, 26, GRB>(leds, 12);
}

void loop() 
{
}

void singleFrequency(float frequency, CRGB color, int minDuration, int maxDuration) 
{
  int duration = random(minDuration, maxDuration);
  int period = 1000 / frequency;
  unsigned long startTime = millis();
  while (millis() - startTime < duration-500) 
  {
    for (int i = 0; i < 12 && millis() - startTime < duration-500; i++) 
    {
      leds[i] = color;
      FastLED.setBrightness(100);
      FastLED.show();
      delay(period / 2);
      fill_solid(leds, 12, CRGB::Black);
      FastLED.show();
      delay(period / 2);
    }
  }
  // Turn off LEDs after the color duration
  fill_solid(leds, 12, CRGB::Black);
  FastLED.show();
  delay(1000); //Delay to show lights off.
  return;//exits function
}

void pattern(CRGB defaultPattern[], int patternDuration) 
{
  unsigned long startTime = millis();
  int patternSize = sizeof(defaultPattern)-1; 
  unsigned long elapsedTime = 0;
  FastLED.setBrightness(100);
  int j=0;//Holds the color array element

  while (millis() - startTime < patternDuration) 
  {
      for (int i = 0; i < 12; i++) 
      {
        leds[i] = defaultPattern[j];
      }
      FastLED.show();
      //There is just enough seconds to go through each color in 1 second
      if(patternDuration>(1000*patternSize))
      {
        delay(1000); // Long delay
      }
      else//Goes through the colors and end instantly 
      {
        delay(patternDuration/patternSize); // quick delay
      }
    //If the pattern array is at the end
    if(j<patternSize-1)
    {
      j++;
    }
    //Reset the pattern array 
    else
    {
      j=0;
    }
  }
  // Turn off LEDs after the pattern duration
  fill_solid(leds, 12, CRGB::Black);
  FastLED.show();
  delay(1000); //Delay to show lights off.
  return;//exits function
}

void breathing(float F, CRGB color, int minDuration, int maxDuration) 
{
  //F is the frequency of the signal
  int Fs = 500;                                                //sampling frequency
  int n = 500;                                                 //number of samples
  float t;                                                     //Time instance
  int sampling_interval;
  byte samples[500];                                           // to store the samples
  for (int n = 0; n < 500; n++)
  {
    t = (float) n / Fs;                                       //creating time isntance to find the 500 samples
    samples[n] = (byte) (127.0 * sin(2 * 3.14 * F* t) + 127.0 ); //calculating the sin value at each time instance
    //samples[n] = 
  }
  //sampling_interval = 1000 / F;
  sampling_interval = 1000000 / (F * n);                      
  //sampling interval Ts = 1/frequency x number of sample (Ts = 1/Fn or Ts = T/n)x1000000 to convert it in µS
  int duration = random(minDuration, maxDuration);
  unsigned long startTime = millis();
  fill_solid(leds, 12, color);
  while(millis() - startTime < duration-500)
  {
    for (int j = 0; j < 500&&millis() - startTime < duration-500; j++) 
    {
      FastLED.setBrightness(samples[j]);
      FastLED.show();
      delayMicroseconds(sampling_interval);                      //time interval
    }
  }
  // Turn off LEDs after the color duration
  fill_solid(leds, 12, CRGB::Black);
  FastLED.show();
  delay(1000); //Delay to show lights off.
  return;//exits function
}
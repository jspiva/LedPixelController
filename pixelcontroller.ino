// Includes
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include "PixelBuffer.h"
#include "PixelWriterAsync.h"
#include "e131.h"
#include <ArduinoOTA.h>

// #define DEBUG // turn on debugging

// Allow analog voltage to be read
ADC_MODE(ADC_VCC);

// Constants

// E131 Packet Parser
E131 e131;

// Networking
const char* ssid     = "...";           // Replace with WiFi SSID
const char* password = "...";           // Replace with WiFi Password
const char* domain = "esp8266.local";   // Replace with local domain name
char name[11] = "LED_xxxxxx";

// State Machine
const uint8_t STATE_STARTING = 0X00;
const uint8_t STATE_CONNECTING = 0X01;
const uint8_t STATE_WAITING = 0X02;
const uint8_t STATE_PROCESSING = 0X03;
const uint8_t STATE_REFRESHING = 0X04;
uint8_t state = STATE_STARTING;

// Settings
const bool LED_ON = LOW;
const bool LED_OFF = HIGH;

const uint8_t UNIVERSE_MAXCOUNT = 255;
const uint8_t CHANNEL_COUNT = 8;
const uint8_t PIXEL_SIZE = 3;
const uint8_t PIXEL_COUNT = 150;
const uint8_t PINS[CHANNEL_COUNT] = {D0, D1, D2, D3, D5, D6, D7, D8};
uint8_t universeToBuffer[UNIVERSE_MAXCOUNT];

// Colors (RGB)
uint8_t RED[PIXEL_SIZE]= {0x20, 0x00, 0x00};
uint8_t ORANGE[PIXEL_SIZE]= {0x20, 0x10, 0x00};
uint8_t YELLOW[PIXEL_SIZE]= {0x20, 0x20, 0x00};
uint8_t GREEN[PIXEL_SIZE]= {0x00, 0x20, 0x00};
uint8_t CYAN[PIXEL_SIZE]= {0x00, 0x20, 0x20};
uint8_t BLUE[PIXEL_SIZE]= {0x00, 0x00, 0x20};
uint8_t PURPLE[PIXEL_SIZE]= {0x20, 0x00, 0x20};
uint8_t BLACK[PIXEL_SIZE]= {0x00, 0x00, 0x00};
uint8_t WHITE[PIXEL_SIZE]= {0x20, 0x20, 0x20};

// Create the pixel Writer
PixelWriterAsync pixelWriter(PIXEL_COUNT, PIXEL_SIZE, PINS, CHANNEL_COUNT);

// Declare some variables
PixelBuffer buffers[CHANNEL_COUNT + 1]{
  {&pixelWriter, PIXEL_SIZE, PIXEL_COUNT, PINS, CHANNEL_COUNT},
  {&pixelWriter, PIXEL_SIZE, PIXEL_COUNT, &PINS[0], 1},
  {&pixelWriter, PIXEL_SIZE, PIXEL_COUNT, &PINS[1], 1},
  {&pixelWriter, PIXEL_SIZE, PIXEL_COUNT, &PINS[2], 1},
  {&pixelWriter, PIXEL_SIZE, PIXEL_COUNT, &PINS[3], 1},
  {&pixelWriter, PIXEL_SIZE, PIXEL_COUNT, &PINS[4], 1},
  {&pixelWriter, PIXEL_SIZE, PIXEL_COUNT, &PINS[5], 1},
  {&pixelWriter, PIXEL_SIZE, PIXEL_COUNT, &PINS[6], 1},
  {&pixelWriter, PIXEL_SIZE, PIXEL_COUNT, &PINS[7], 1}
};

bool wifiDisconnected = false;

void setup() {

  Serial.begin(500000); 
  delay(100);

  // Clear the screen
  Serial.write(12);
  Serial.println(F("*** LED PIXEL CONTROLLER V1.0 ***"));

  // Set a hostname
  sprintf(name, "LED_%06X", ESP.getChipId());
  WiFi.hostname(name);
  Serial.print(F("Hostname: "));
  Serial.println(name);

  // Read/display some data
  Serial.print(F("Free Heap:"));
  Serial.println(ESP.getFreeHeap());
  Serial.print(F("Flash Size(bytes):"));
  Serial.println(ESP.getFlashChipSize());
  Serial.print(F("Flash Speed(Hz):"));
  Serial.println(ESP.getFlashChipSpeed());
  Serial.print(F("Supply Voltage:"));
  Serial.println(ESP.getVcc());

  Serial.println("");

}

// The main program loop
void loop() {

  // State Machine
  switch (state) {

    case STATE_WAITING: {

        state = Wait();
      }
      break;
    case STATE_PROCESSING: {

        state = Process();
      }
      break;
    case STATE_REFRESHING: {

        state = Refresh();
      }
      break;
    case STATE_CONNECTING : {

        state = Connect();
      }
      break;

    case STATE_STARTING : {

        state = Start();
      }
      break;
  }
}

// Start the process
uint8_t Start() {

  Serial.println(F("Starting..."));

  Serial.println(F("Starting PixelWriter service..."));
  
  // Initialize the pixel writer
  pixelWriter.Initialize();

  // Write the blanks out to all channels to clear them
  buffers[0].Clear();
  buffers[0].Show();

  // Set startup sequence
  buffers[0].SetRepeat(BLUE, PIXEL_SIZE);
  buffers[0].Show();

  Serial.println(F("Calculating the universe to buffer lookup..."));

  // Create the universe to channel lookup array
  uint8_t universeIndex;
  for (universeIndex = 0; universeIndex < UNIVERSE_MAXCOUNT; universeIndex++){

    // Calculate the possible buffer index
    uint8_t bufferIndex = universeIndex % 10;

    // Set the broadcast channel if we don't know where it goes.
    if (bufferIndex > CHANNEL_COUNT) { bufferIndex = 0; }

    // Save the value in the array for easy lookup
    universeToBuffer[universeIndex] = bufferIndex;   
  }

  // Start OTA server.
  Serial.println(F("Starting OTA update serivce..."));
  startOTA(name);

  // Start the e131 server
  Serial.println(F("Starting E131 serivce..."));
  e131.begin();

  Serial.println(F("Startup complete"));
  Serial.println("");

  // Update the state
  return STATE_CONNECTING;
}


// Connect to the wifi
uint8_t Connect() {

  // Connect the wifi using DHCP
  Serial.println(F("Connecting..."));
  Serial.print(F("Connecting to "));
  Serial.print(ssid);

  // Make sure we are disconnected.
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  
  if (password != NULL){
    WiFi.begin(ssid, password);
  }else{
    WiFi.begin(ssid);
  }

  // Wait for a connection
  uint8_t waitIndex = 0;
  const uint8_t pixelCount = 5;
  const uint8_t dataLength = pixelCount * PIXEL_SIZE;
  uint8_t repeatData[dataLength];
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(250);

    // Set all blue pixels
    uint8_t i;
    for (i = 0; i < pixelCount; i++){
        memcpy(repeatData + (i * PIXEL_SIZE), BLUE, PIXEL_SIZE);
    }
    
    // Set the Black Pixel
    uint8_t pixelIndex = waitIndex % pixelCount;
    memcpy(repeatData + (pixelIndex * PIXEL_SIZE), BLACK, PIXEL_SIZE);
    
    buffers[0].SetRepeat(repeatData, dataLength); 
    buffers[0].Show();

    waitIndex++;
  }
  
  Serial.println("");
  buffers[0].Clear();
  buffers[0].Show();
  
#ifdef DEBUG
  WiFi.printDiag(Serial);
#endif

  // Write the connection details
  Serial.println(F("WiFi connected"));
  Serial.print(F("MAC address: "));
  Serial.println(WiFi.macAddress());
  Serial.print(F("IP address: "));
  Serial.println(WiFi.localIP());
  Serial.print(F("Gateway: "));
  Serial.println(WiFi.gatewayIP());
  Serial.print(F("Channel: "));
  Serial.println(WiFi.channel());
  Serial.print(F("Signal Strength(dB): "));
  Serial.println(WiFi.RSSI());

  // Set up mDNS responder:
  // - first argument is the domain name, in this example
  //   the fully-qualified domain name is "esp8266.local"
  // - second argument is the IP address to advertise
  //   we send our IP address on the WiFi network
  if (!MDNS.begin(domain)) {
    Serial.println(F("Error setting up mDNS responder!"));
  }
  Serial.println(F("mDNS responder started"));

  Serial.println(F("Connection complete"));
  Serial.println("");

  // Update the state
  return STATE_WAITING;
}


// Waits for a network command
uint8_t Wait() {
  
  // Check if data has been received
  uint16_t size = e131.parsePacket();
  if (size) {
    
    // Skip preview packets
    if ((e131.packet.options & 0x80) != 0x80){
        
        // Transision to the Processing state
        return STATE_PROCESSING;   
    }
  } else {
    
    // Log a disconnection, ESP should reconnect on it's own.
    if (WiFi.status() != WL_CONNECTED) {

      // If we were not already disconnected, log it.
      if (!wifiDisconnected){Serial.println(F("WiFi disconnected..."));}

      // Save the value so we don't relog it.
      wifiDisconnected = true;
      
    } else {

      // If we are not already connected, log it.
      if (wifiDisconnected){Serial.println(F("WiFi connected..."));}
      wifiDisconnected = false;
    }
  
    // Check for any code updates
    ArduinoOTA.handle();   
  }
  
  return STATE_WAITING;
}


// Process a network command
uint8_t Process() {

  // Determine the buffer index
  uint8_t bufferIndex = universeToBuffer[e131.universe];

 #ifdef DEBUG
  Serial.print("Processing Buffer: ");
  Serial.println(bufferIndex);
 #endif

  // Read the start code
  uint8_t startCode = e131.packet.property_values[0];

  // Start code drives the action to process
  switch (startCode) {
      
    default: 
      // if nothing else matches, do the default
      // default is optional

      uint16_t byteCount = htons(e131.packet.property_value_count) - 1;

      // Write the entire set of data to the buffer
      buffers[bufferIndex].SetBuffer(e131.data, byteCount);
      
    break;
  }

  // Update the state
  return STATE_REFRESHING;
}


// Refresh the channel
uint8_t Refresh() {

  // Determine the channel index
  uint8_t bufferIndex = universeToBuffer[e131.universe];

 #ifdef DEBUG
  Serial.print("Refreshing Buffer: ");
  Serial.println(bufferIndex);
 #endif

  // Show the channel
  buffers[bufferIndex].Show();

  // Transistion
  return STATE_WAITING;
}


// Start OTA
void startOTA(const char *hostname){

  ArduinoOTA.onStart([]() {
    
    Serial.println(F("OTA Update Starting..."));
    
    buffers[0].SetRepeat(YELLOW, PIXEL_SIZE); 
    buffers[0].Show();   
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {

    // Clear the page, so the progress overwrites itself
    Serial.write(12);
  
    // Provide update status.
    Serial.println(F("OTA Update Running..."));
    Serial.print(F("Progress: "));
    Serial.print(progress);
    Serial.print(F(" of  "));
    Serial.println(total);
       
  });
 
  ArduinoOTA.onEnd([]() { // do a fancy thing with our board led at end

    Serial.println(F("OTA Update Complete, Restarting..."));
    
    buffers[0].SetRepeat(GREEN, PIXEL_SIZE); 
    buffers[0].Show();
  });

  ArduinoOTA.setHostname(hostname);
  //ArduinoOTA.setPassword((const char *)"test");

  ArduinoOTA.begin();

}

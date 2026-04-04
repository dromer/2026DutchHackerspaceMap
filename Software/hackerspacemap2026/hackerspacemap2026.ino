/**********************************************************************************
 * Copyright (c) 2026, Theo Borm
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// FOR WIFI CREDENTIALS SEE SETUP ROUTINE
// should fix this by setting up an access point an letting user enter their wifi credentials instead of this hack

/*******************************************************************************************************************
* Arduino sucks.
*
* Really, The Arduino IDE is a multi-level clusterfuck for everything but the simplest projects.
*
* This could (or maybe should) be a multi-page rant about what is wrong with ARDUINO, and there
* might be some entertainment value in that, but I simply don't have the time for it, so I'll
* be short.
*
* There is NO project management in ARDUINO, and if you split source across multiple files
* you end up in dependency hell because the ARDUINO IDE breaks C/C++ programming standards.
* Splitting complex source into multiple units along functional bounfaries is a way to keep
* code understandable and manageable, and is something programmers deal with on a daily basis.
* Not in ARDUINO IDE, though. The ARDUINO IDE adds some "Magic" to the compilation process which
* (I guess) makes it slightly easier to get started, but which breaks normal dependency management.
*
* While it may very well be possible to work around the limitations and create a multi-source-
* file project, with sources split in a way that makes some sense from a software maintainance
* point of view, I simply don't have the time nor energy to jump through these hoops, So I
* restructured everything so that it at least works: one big file.
*
*******************************************************************************************************************/



/* This map of hackerspaces in NL displays info about the open/closed state obtained INDIRECTLY through the
* SpaceAPI. There is no guarantee whatsoever that this  information is accurate. The necessary information is
* obtained by a server once every ~minute, compacted and presented as a single string - one ASCII character per
* hackerspace at the URL https://hackwinkel.nl/hsm2026/api
* The ASCII code of each character in the string is obtained by adding 32 to a 6 bit number encoding
* the colour of the corresponding LED. The following 6 bit binary RGB colours, numbers, ASCII codes and
* meanings are currently defined:
* 0b000000   0 -> 32=" " black   defunct space
* 0b110000  48 -> 80="P" red     closed
* 0b001100  12 -> 44="," green   open
* 0b101000  40 -> 72="H" yellow  indeterminate - JSON contains state->open, but neither true nor false
* 0b010100  20 -> 52="4" brown  indeterminate - JSON contains state, but not state->open
* 0b000101  05 -> 37="%" magenta  indeterminate - JSON contains no state
* 0b000011  03 -> 35="#" blue    no JSON or SpaceAPI unreachable
*
* //https://directory.spaceapi.io/ is a list of spaceapi providers
* with state->open->true/false in a json structure
*
* The following is a (at time of writing) list of SpaceAPI URLs of  Dutch hackerspaces that the
* server uses to generate the string that is ret
* https://maakplek.nl/api/
* https://mqtt.hackerspace-drenthe.nl/spaceapi
* https://spaceapi.tkkrlab.nl/
* https://hack42.nl/spacestate/json.php
* https://state.hackerspacenijmegen.nl/state.json
* https://spaceapi.tdvenlo.nl/spaceapi.json
* https://ackspace.nl/spaceAPI/
* https://hackalot.nl/statejson
* https://services.pi4dec.nl/space/spaceapi.json
* https://spaceapi.pixelbar.nl/
* https://revspace.nl/status/status.php
* https://portal.spaceleiden.nl/api/public/status.json
* http://techinc.nl/space/spacestate.json
* https://state.awesomespace.nl
* https://randomdata.sandervankasteel.nl/index.json
* https://hermithive.nl/state.json
* https://space.nurdspace.nl/spaceapi/status.json
* https://bitlair.nl/spaceapi.json
*
*/



/*******************************************************************************************************************
* INCLUDES section for external header files
*******************************************************************************************************************/
#include <Arduino.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_heap_caps.h"
// includes and objects for networking
#include <NetworkClientSecure.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include "esp_mac.h"


/*******************************************************************************************************************
* TYPEDEFINITIONS section here because ARDUINO IDE does weird stuff with them
*******************************************************************************************************************/
// data needs to be stored in a special format to be able to send it to the LED string efficiently.
// this format is somewhat wasteful as it uses one 32 bit word per byte of information.
// struct holding the 3 32 bit encoded colour values in the required order. this struct represents one pixel
typedef struct {
  uint32_t G;
  uint32_t R;
  uint32_t B;
} SpiLedStringPixel;

// info about a hackerspace - state, led "phase" and "speed" - is stored in a structure for each space
typedef struct {
  uint8_t state;
  uint8_t phase;
  uint8_t speed;
  uint8_t count;
} SpaceInfo;

/*******************************************************************************************************************
* NETWORKING - CLIENT
*******************************************************************************************************************/
// FOR WIFI CREDENTIALS SEE SETUP ROUTINE
// should fix this by setting up an access point an letting user enter their wifi credentials instead of this hack

NetworkClientSecure client;
char ssid[40];
char password[40];

int16_t reconnect=false; // was the module ever connected?

const char *server = "hackwinkel.nl";
const char *path = "hsm2026/api";
char tempstr[100]; // for constructing strings

// try to (re-)connect to the wifi network, returns true on succes
void ConnectWifi(void) {
  if (reconnect) { WiFi.reconnect(); } else { WiFi.begin(ssid, password); }
}

int IsWifiConnected(void) {
    if (WiFi.status() == WL_CONNECTED) return(true); else return (false);
}

// try to get data from the server. wifi MUST be connected
int GetData(char * data, int maxlength) {
  int ok=true;
  data[0]=0; // clears out string
  if (WiFi.status() != WL_CONNECTED) { return(false); }
  //client.setCACert(rootCACertificate);
  client.setInsecure();
  if (!client.connect(server, 443)) {
    Serial.println("Failed to connect to server");
    return(false);
  } else {
    sprintf(tempstr,"GET https://%s/%s HTTP/1.1\r\n",server,path);
    client.print(tempstr);
    sprintf(tempstr,"Host: %s\r\n",server);
    client.print(tempstr);
    client.print("Connection: close\r\n\r\n");
    // simply ignore response headers
    while (client.connected()) {
      String line = client.readStringUntil('\n');
      if (line == "\r") {
        break;
      }
    }
    // get response content if available
    int cnt=0;
    while (client.available() && cnt<maxlength) {
      data[cnt] = client.read();
      cnt++;
      data[cnt] = 0; // zero terminate response
    }
    client.stop();
    if (cnt>0) return(true); // some content was obtained
  }
  return(false);
}


/*******************************************************************************************************************
* TIMER section - to maintain a 10 ms tick timer and tick flag and 60s tock timer and tock flag
*******************************************************************************************************************/
hw_timer_t * times = NULL; // a timer structure

uint32_t tick=0; // 10 ms each
uint32_t tickflag=0;
uint32_t tockticks=0; // 6000 ticks is one tock
uint32_t tock=0; // 60s each;
uint32_t tockflag=0;

void IRAM_ATTR HandleTimer(void) {
  tick++;
  tickflag=1;
  tockticks++;
  if (tockticks>=6000) {
    tock++;
    tockticks=0;
    tockflag=1;
  }
}

int InitializeTimer(void) {
  times = timerBegin(1000000); // 1Mhz
  if (times == NULL) {
    return(false);
  } else {
    timerAttachInterrupt(times, &HandleTimer);
    timerAlarm(times, 10000, true, 0); // Alarm 10ms
    return(true);
  }
}

/*******************************************************************************************************************
* SPILEDSTRING section -  a basic SPI+DMA hardware LED string driver.
*******************************************************************************************************************/
#define LEDSTRINGPIN 10
#define LEDSTRINGSIZE 38 // 18 spaces + 20 backlight LEDs
/*
 * This code uses SPI to send data to the ws2812 LED(s).
 * The WS2812 expects each bit to last ~ 1.2 us. A "1" is transmitted as an 800ns
 * high level on a pin, followed by a 400ns low level. A "0" is transmitted as a 400ns
 * high level on a pin, followed by an 800ns low level. The data rate is 800 kbit/s
 * If we operate the SPI at 3.2 MHz, we can "emulate" this on-off pattern with 4 bits
 * per bit that must be sent to the panel. A "0" bit is emulated using the pattern
 * 1000, a "1" bit is emulated using the pattern 1110. This means that a "0" bit is
 * sent as a 312.5 ns high, followed by a 937.5 ns low signal, and that a "1" bit is
 * sent as a 837.5 ns high, followed by a 312.5 ns low signal. This is OFF-SPEC, but
 * it works. Main benefit is that one byte of data to be transmitted fits into a 32
 * bit pattern, using "only" 4 times more memory than the "real" data, as opposed to
 * 32 times as much data for the RMT perhipheral. This also makes manipulating
 * display data much more efficient. Instead of shifting colour bits and then turning
 * colour bits into 32 bits in memory, we can write a (translated) colour byte to
 * memory as a single 32 bit word.
 * It would be possible to reduce the memory consumption by a quarter by using patterns
 * of three bits per "0" or "1", but that would require a lot more processing and
 * memory transfer cycles.
 *
 * The RGB values must be encoded in 3 consecutive 32 bit words in the right (GRB)
 * order. Each bit of each RGB value byte must be translated in a 4 bit pattern. The
 * storage order of these patterns in the 32 bit words depends on the endianness of
 * the ESP32, and is (for each bit 0..7, with 7 the most significant in the byte):
 * 11110000333322225555444477776666
 *
 * This encoding is hanldled  by the SpiLedStringSetPixel() function, which calls the
 * SpiLedStringEncodeValue() function for each (R, G, B) value.
 */

//default #define for the pin to which the string is connected
#define LEDSTRINGBITS LEDSTRINGSIZE * 24 * 4

// encoded RGB string memory
DMA_ATTR SpiLedStringPixel ledDMAbuffer[LEDSTRINGSIZE];

// Encode an 8 bit value into a 32 bit value
uint32_t SpiLedStringEncodeValue(uint8_t v) {
  uint32_t r=0b10001000100010001000100010001000; //encoding of all zero bits
  if (v&  1) r |= 0b10001110100010001000100010001000;
  if (v&  2) r |= 0b11101000100010001000100010001000;
  if (v&  4) r |= 0b10001000100011101000100010001000;
  if (v&  8) r |= 0b10001000111010001000100010001000;
  if (v& 16) r |= 0b10001000100010001000111010001000;
  if (v& 32) r |= 0b10001000100010001110100010001000;
  if (v& 64) r |= 0b10001000100010001000100010001110;
  if (v&128) r |= 0b10001000100010001000100011101000;
  return(r);
 }

 // fill a single LED string pixel with the appropriate encoded values
  void SpiLedStringSetPixel(int p, uint8_t R, uint8_t G, uint8_t B) {
    ledDMAbuffer[p].R=SpiLedStringEncodeValue(R);
    ledDMAbuffer[p].G=SpiLedStringEncodeValue(G);
    ledDMAbuffer[p].B=SpiLedStringEncodeValue(B);
  }

// control structures for the spi interface attached to the led string
static spi_device_handle_t ledStringSpiDevice;
static spi_device_interface_config_t ledStringSpiInterfaceCfg;
static spi_bus_config_t ledStringSpiBusCfg;
static spi_transaction_t ledStringSpiTransaction;

// initialize the hardware to drive a led string
void SpiLedStringInitialize(void) {
  ledStringSpiBusCfg = {
  	.mosi_io_num = LEDSTRINGPIN,
  	.miso_io_num = -1,
  	.sclk_io_num = -1,
  	.quadwp_io_num = -1,
  	.quadhd_io_num = -1,
  	.max_transfer_sz = LEDSTRINGBITS
	};
  ledStringSpiInterfaceCfg = {
    .command_bits = 0,
    .address_bits = 0,
    .mode = 0,
    .clock_speed_hz = 3200000,
  	.spics_io_num = -1,
  	.queue_size = 1 
	};
  ledStringSpiTransaction = {
    .flags=0,
    .cmd=0,
    .addr=0,
    .length=LEDSTRINGBITS,// in bits
    .rxlength=0,
    .user=(void*)0,
    .tx_buffer=(void *)ledDMAbuffer,
    .rx_buffer=(void*)0
  };

	esp_err_t err;
	err = spi_bus_initialize(SPI2_HOST, &ledStringSpiBusCfg, SPI_DMA_CH_AUTO);
	ESP_ERROR_CHECK(err);
	err = spi_bus_add_device(SPI2_HOST, &ledStringSpiInterfaceCfg, &ledStringSpiDevice);
	ESP_ERROR_CHECK(err);
}

// update the actual screen by starting the data transfer
void SpiLedStringTransmit(void) {
  esp_err_t err;
  err = spi_device_queue_trans(ledStringSpiDevice, &ledStringSpiTransaction,1);
}

/*******************************************************************************************************************
* RENDERING and DISPLAY section -  hackerspace map specific
*******************************************************************************************************************/
/*
 * The led string is split in two parts: The first 18 LEDs are used to reflect the actual space states.
 * The (optional) other LEDs are used as a backlight to the PCB.
 *
 * The space state LEDs get their colors from 3 arrays: "states", "phases" and "speeds".
 * The "states" determine the colours of the LEDs, and are directly related to the reported space state.
 * The "phases" determine the brightness of the LEDs, and are used to impart a (sort of) "breathing" effect
 * The "speeds: determine the speed at which the "breathing" happens.
 *
 * the backlight will have half the LEDs show one colour and the other half the "oposite" colour.
 * the colour wil change slowly, and the halves will also rotate (more) slowly.
 * the current state of the backlight is therefore governed by two variables: blcol and blphase
 * bgcol is an index into two arrays of colour-values:
 * 0 -> 60,00,00 / 00,30,30
 * 1 -> 40,20,00 / 10,20,30
 * 2 -> 20,40,00 / 20,10,30
 * 3 -> 00,60,00 / 30,00,30
 * 4 -> 00,40,20 / 30,10,20
 * 5 -> 00,20,40 / 30,20,10
 * 6 -> 00,00,60 / 30,30,00
 * 7 -> 20,00,40 / 20,30,10
 * 8 -> 40,00,20 / 10,30,20
 */
uint32_t foreground[]={ 0x3c0000, 0x281400, 0x142800, 0x003c00, 0x002814, 0x001428, 0x00002c, 0x140028, 0x280014 };
uint32_t background[]={ 0x001e1e, 0x0a141e, 0x140a1e, 0x1e001e, 0x1e0a14, 0x1e1e0a, 0x1e1e00, 0x141e0a, 0x0a1e14 };
int blcol=0;
int blcolcount=0;
int blcolspeed=2353;
int blphase=0;
int blphasecount=0;
int blphasespeed=91;

 #define SPACELEDS 18
 #define BACKLIGHTLEDS 20

SpaceInfo Spaces[SPACELEDS]; ///stores info for the LED of each space

void ClearSpaceInfo(void) {
  for (int i=0; i<SPACELEDS; i++) {
    Spaces[i].state=21; // 64 possible values 0 is black
    Spaces[i].phase=0; // count from 0..30, corresponding to brightness 1..16..1
    Spaces[i].speed=7; // speed for updating phase
    Spaces[i].count=0; // counter for updating phase
  }
}

void FillSpaceInfo(char * d) {
  for (int i=0; i<SPACELEDS; i++) {
    int t=2;
    if ((d[i]-32)&48) t+=7;
    if ((d[i]-32)&12) t+=3;
    if ((d[i]-32)&3) t+=9;
    Spaces[i].state=d[i]-32; // 64 possible values 0 is black
    Spaces[i].speed=7+random(5); // speed for updating phase
    Spaces[i].count=Spaces[i].count % Spaces[i].speed; // counter for updating phase
  }

}
// takes the info from the space state and backlight and renders this info into SpiLedString DMA memory encoded as colors ready to be sent to the LEDs
// called every 10 ms
void Render(void) {
  for (int i=0; i<SPACELEDS; i++) {
    uint8_t brightness=Spaces[i].phase+1;
    if (brightness>16) brightness=32-brightness;
    SpiLedStringSetPixel(i, ((Spaces[i].state & 48)>>4)*brightness, ((Spaces[i].state & 12)>>2)*brightness, (Spaces[i].state & 3)*brightness);
  }
  for (int i=0; i<BACKLIGHTLEDS; i++) {
    int l=(i+blphase) % BACKLIGHTLEDS;
    if (l<(BACKLIGHTLEDS>>1)) {
      SpiLedStringSetPixel(SPACELEDS+i,(foreground[blcol]>>16)&0xff, (foreground[blcol]>>8)&0xff, (foreground[blcol])&0xff);
    } else {
     SpiLedStringSetPixel(SPACELEDS+i,(background[blcol]>>16)&0xff, (background[blcol]>>8)&0xff, (background[blcol])&0xff);
    }
  }
}

// update the states of the space LEDs and background. 
// called every 10 ms
void Update(void) {
  for (int i=0; i<SPACELEDS; i++) {
    Spaces[i].count++;
    if ((Spaces[i].count) >= (Spaces[i].speed)) {
      Spaces[i].count=0;
      int t=Spaces[i].phase+1;
      if (t>30) t=0;
      Spaces[i].phase=t;
    }
  }
  blphasecount++;
  if (blphasecount >= blphasespeed) {
    blphasecount=0;
    int t=blphase+1;
    if (t >= BACKLIGHTLEDS) t=0;
    blphase=t;
  }
  blcolcount++;
  if (blcolcount >= blcolspeed) {
    blcolcount=0;
    int t=blcol+1;
       if (t >= 9) t=0;
    blcol=t;
  }
}



int wifimode=0; // wifi unconfigured
uint32_t prev; // to timeout wifi stuff
uint32_t now;

void setup(void) {

  pinMode(8,OUTPUT); // attached to the ESP32 on-board LED - debugging wifi connection
  digitalWrite(8,HIGH); // wifi disconnected - LED on

  Serial.begin(115200);
  delay(100);
  Serial.println("Start");

  if (InitializeTimer()) {
    Serial.println("Timer started");
  } else {
     Serial.println("Timer failed to start");
  }
  SpiLedStringInitialize();
  Serial.println("LED string initialized");
  ClearSpaceInfo();


  strcpy(ssid,"mywifi"); // your default network SSID (name of wifi network)
  strcpy(password,"mywifipassword"); // your default network password

  ConnectWifi();

  prev=millis();
  now=prev;
  while (((now-prev)<15000) && (IsWifiConnected() == false) ) {
    delay(100);
    now=millis();
    digitalWrite(8,LOW);
  }

  if (IsWifiConnected()) {
    Serial.println("wifi is connected");
    digitalWrite(8,HIGH);
    wifimode=1;
  } else {
    Serial.println("wifi is NOT connected");
    digitalWrite(8,LOW);
    // maybe set up access point etc...
  }
}

int tockstate=0; // state machine

void loop(void) {
  if (tickflag) {
    Render();
    SpiLedStringTransmit();
    Update();
    tickflag=0;
    if (tockstate && wifimode) {
      switch (tockstate) {
        case 1:
          if (IsWifiConnected()) {
            tockstate=3;
          } else { 
            ConnectWifi();
            prev=millis();
            tockstate=2;
          }
          break;
        case 2:
          if (IsWifiConnected()) {
            digitalWrite(8,HIGH);
            tockstate=3; 
          } else {
              digitalWrite(8,LOW);
            now=millis();
            if ((now-prev) > 30000) {
              tockstate=0;
              Serial.println("WIFI timeout");
            }
          }
          break;
        case 3:
          char data[101];
          if (GetData(data,100)) {
            if (strlen(data) == SPACELEDS) {
              FillSpaceInfo(data);
            } else {
              Serial.println("incorrect data length");
            }
          } else {
            Serial.println("fail");
          }
          tockstate=0;
        break;
      }
    }
  }
  if (tockflag) {
    tockstate=1;
    Serial.println(tock);
    tockflag=0;
  }
}

/*
  Door Sensor
  Author: Kevin Watters  
*/

#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
// #include <WiFi.h>

// char ssid[] = "wifishield"; //  your network SSID (name) 
// char pass[] = "secretPassword";    // your network password (use for WPA, or use as key for WEP)
// int keyIndex = 0;            // your network key Index number (needed only for WEP)

// int status = WL_IDLE_STATUS;
// if you don't want to use DNS (and reduce your sketch size)
// use the numeric IP instead of the name for the server:
IPAddress server(10,0,0,192);  // numeric IP for Google (no DNS)
//char server[] = "www.google.com";    // name address for Google (using DNS)

IPAddress timeServer(132, 163, 4, 101); // time-a.timefreq.bldrdoc.gov NTP server
const int NTP_PACKET_SIZE= 48; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets 

// A UDP instance to let us send and receive packets over UDP
EthernetUDP Udp;

unsigned int localPort = 8888;      // local port to listen for UDP packets


// Initialize the Ethernet client library
// with the IP address and port of the server 
// that you want to connect to (port 80 is default for HTTP):
EthernetClient client;

byte mac[] = { 0x90, 0xA2, 0xDA, 0x0D, 0xD6, 0x3F };

// door sensor stuff.
// pins 5, 6, 8 and 9 work..
int sensor[] = {6,8,9};
int state[] = {0,0,0};
unsigned long openTimestamp[] = {0,0,0};
int numSensors = 3;

int led = 11;
int time_threshold = 100; 
int val = 0;
int time = 0;
int isOpen = false;
// unsigned long openTime = 0;

void setup() {

  initBoard();
  initNetwork();
  initSensors();
 
}

void initBoard() {
    //Initialize serial and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  Serial.println("Serial Port Connected.");
  // check for the presence of the shield:

}

void initNetwork() {
  // attempt to connect to Wifi network:
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP.");
    // no point in carrying on, so do nothing forevermore:
    // while(true);
  } else {
    Serial.println("Connected to ethernet");
    printEthernetStatus();
    updateClockViaNTP(server);   
  }
}

void initSensors() {
    for (int i = 0 ; i < numSensors; i++ ) {
      pinMode(sensor[i],INPUT_PULLUP);  
      state[i] = digitalRead(sensor[i]);
    }
}

void loop() {  
  int newVal = 0;
  Serial.print("STATE : ");
  Serial.print(state[0]);
  Serial.print(" ");
  Serial.print(state[1]);
  Serial.print(" ");
  Serial.println(state[1]);
  
  for (int i = 0 ; i < numSensors; i++) {
    newVal = digitalRead(sensor[i]);
    Serial.print("Sensor : ");
    Serial.print(i);
    Serial.print(" value: ");
    Serial.println(newVal);
    if (state[i] != newVal) { 
      Serial.print("State change: " );
      Serial.print(i);
      Serial.print(" ");
      Serial.println(newVal);
      // a state changed.
      if (newVal == 1) {        
        // did it really open?
        Serial.println("Test if it really opened...");
        delay(time_threshold);
        int tval = digitalRead(sensor[i]);
        Serial.print("TVAL:");
        Serial.println(tval);
        if (newVal == tval) {
          // it was a blip.. it didn't stay in this state for more than 100 ms..        
          Serial.print("Sensor : ");
          Serial.print(i);
          Serial.println(" opened.");
          openTimestamp[i] = millis();
          alertIO(i, "opened", 0);
        } else {
          Serial.println("Not properly detected...");
        }
         
      } else {
        delay(time_threshold);
        if (newVal == digitalRead(sensor[i])) {
          // it was a blip.. it didn't stay in this state for more than 100 ms..
          unsigned long delta = millis() - openTimestamp[i];
          Serial.print("Sensor : ");
          Serial.print(i);
          Serial.print(" closed. It was opened for ");
          Serial.print(delta);
          Serial.println(" ms.");           
          alertIO(i,"closed", delta);
        }
      }
    }
    state[i] = newVal;
  }
  // just to keep power consumption down?
  delay(100);
}
  /*
  val = digitalRead(sensor[0]);
  // while closed the value is low (the arduino has pins as pullup inputs.
  while(val == LOW){
    val = digitalRead(sensor[0]);  
    if (!val) {
      if (isOpen) {
        unsigned long delta = millis() - openTime;
        Serial.print("Door just closed : ");
        Serial.println(delta);
        alertIO(1,"closed", delta);
      }
      isOpen = false;
    }
  }
  
  time = 0;
  while((val == HIGH)&&(time < time_threshold)){
    time = time + 100;
    val = digitalRead(sensor[0]);
    delay(100);
    digitalWrite(led,LOW);
    // Serial.print("Waiting for threshold: ");
    // Serial.println(val);
  }
  
  if (time >= time_threshold){
    if (!isOpen) {
      isOpen = true;
      digitalWrite(led,LOW);
      openTime = millis();
      Serial.print("Alerting IO - opened  millis: ");
      Serial.println(openTime);
      alertIO(1, "opened", 0);
    } else {
      // Serial.println("Was already alerted... skipping alert. door still open.");
    }
  }
}
  */


void printEthernetStatus() {
  // print the SSID of the network you're attached to:
  //Serial.print("SSID: ");
  //Serial.println(WiFi.SSID());
  // print your WiFi shield's IP address:
  IPAddress ip = Ethernet.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
  // print the received signal strength:
  //long rssi = WiFi.RSSI();
  //Serial.print("signal strength (RSSI):");
  //Serial.print(rssi);
  //Serial.println(" dBm");
}

void alertIO(int id, char* action, unsigned long time) {
  if (client.connect(server, 80)) {
    // Serial.println("connected to server");
    // Make a HTTP request:
    client.print("GET /alert.php?id=");
    client.print(id);
    client.print("&action=");
    client.print(action);
    client.print("&time=");
    client.print(time);
    client.println(" HTTP/1.1");
    client.println("Host: phobos");
    client.println("Connection: close");
    client.println();
    client.flush();
    client.stop();    
    Serial.println("Alerted IO."); 
  } else {
    Serial.println("Error alerting IO."); 
  }
  // delay(5000);
}




// send an NTP request to the time server at the given address 
unsigned long updateClockViaNTP(IPAddress& address)
{
  Serial.println("Updating clock..");
  // set all bytes in the buffer to 0
  
  Udp.begin(localPort);
  
  memset(packetBuffer, 0, NTP_PACKET_SIZE); 
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49; 
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp: 		   
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer,NTP_PACKET_SIZE);
  Udp.endPacket(); 
  
  Serial.println("done with packet");
  // wait for the response 
  // todo. this is lame.need this to be more async..
  delay(1000); 
  
    if ( Udp.parsePacket() ) {  
    // We've received a packet, read the data from it
    Udp.read(packetBuffer,NTP_PACKET_SIZE);  // read the packet into the buffer

    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);  
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;  
    Serial.print("Seconds since Jan 1 1900 = " );
    Serial.println(secsSince1900);               

    // now convert NTP time into everyday time:
    Serial.print("Unix time = ");
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;     
    // subtract seventy years:
    unsigned long epoch = secsSince1900 - seventyYears;  
    // print Unix time:
    Serial.println(epoch);                               


    // print the hour, minute and second:
    Serial.print("The UTC time is ");       // UTC is the time at Greenwich Meridian (GMT)
    Serial.print((epoch  % 86400L) / 3600); // print the hour (86400 equals secs per day)
    Serial.print(':');  
    if ( ((epoch % 3600) / 60) < 10 ) {
      // In the first 10 minutes of each hour, we'll want a leading '0'
      Serial.print('0');
    }
    Serial.print((epoch  % 3600) / 60); // print the minute (3600 equals secs per minute)
    Serial.print(':'); 
    if ( (epoch % 60) < 10 ) {
      // In the first 10 seconds of each minute, we'll want a leading '0'
      Serial.print('0');
    }
    Serial.println(epoch %60); // print the second
  }
  
}


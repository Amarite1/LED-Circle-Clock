/*
This program contains code adapted from the NTPClient example
*/
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

char ssid[] = "ssid";  //  your network SSID (name)
char pass[] = "pass";       // your network password

IPAddress timeServerIP; //do not touch
const char* ntpServerName = "t3kbau5.com"; //set timeserver domain name (leave blank for manual IP)
//IPAddress timeServerIP(129, 6, 15, 28); //set timeserver ip (uncomment if domain name is blank)

IPAddress weatherServerIP;
const char* weatherServerHost = "t3kbau5.com"; //ser the weather server host (leave blank for IP)
//IPAddress weatherServerIP(129, 6, 15, 28); //set weather server ip (uncomment if domain name is blank)
const char* weatherServerPage = "/stuff/weather.php"; //set the path to the weather script on the above host

unsigned int localPort = 2390;      // local port to listen for UDP packets
const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets
WiFiUDP udp; // A UDP instance to let us send and receive packets over UDP

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  udp.begin(localPort);
  
  if(ntpServerName != ""){
    WiFi.hostByName(ntpServerName, timeServerIP); 
  }
  if(weatherServerHost != ""){
    WiFi.hostByName(ntpServerName, timeServerIP); 
  }
  Serial.println("READY");
}

void loop() {
  if(Serial.available()){
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if(cmd.equalsIgnoreCase("TIME")){
      sendNTPpacket(timeServerIP);
      Serial.println("OK");
    }else{
      Serial.println("ICMD+'" + cmd+"'");
    }
  }

  int cb = udp.parsePacket();
  if(cb){
    unsigned long t = getUnixTime();
    char buf[256];
    sprintf(buf, "T+%lu", t);
    Serial.println(buf);
  }
  
}

// send an NTP request to the time server at the given address
unsigned long sendNTPpacket(IPAddress& address)
{
  // set all bytes in the buffer to 0
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
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}

unsigned long getUnixTime(){
  udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

  //the timestamp starts at byte 40 of the received packet and is four bytes,
  // or two words, long. First, esxtract the two words:
  unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
  unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
  // combine the four bytes (two words) into a long integer
  // this is NTP time (seconds since Jan 1 1900):
  unsigned long secsSince1900 = highWord << 16 | lowWord;
  // now convert NTP time into everyday time:
  // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
  const unsigned long seventyYears = 2208988800UL;
  // subtract seventy years:
  unsigned long epoch = secsSince1900 - seventyYears;
  return(epoch);
}


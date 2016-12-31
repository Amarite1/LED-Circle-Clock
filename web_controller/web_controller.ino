/*
This program contains code adapted from the NTPClient example
*/
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <EEPROM.h>

#define ADDRESS_TOFFSET 0

#define HOUR_SECONDS 3600

char ssid[] = "ssid";  //  your network SSID (name)
char pass[] = "pass";       // your network password

IPAddress timeServerIP; //do not touch
const char* ntpServerName = "t3kbau5.com"; //set timeserver domain name (leave blank for manual IP)
//IPAddress timeServerIP(129, 6, 15, 28); //set timeserver ip (uncomment if domain name is blank)

IPAddress weatherServerIP;
const char* weatherServerHost = "t3kbau5.com"; //ser the weather server host (leave blank for IP)
//IPAddress weatherServerIP(129, 6, 15, 28); //set weather server ip (uncomment if domain name is blank)
const char* weatherServerPage = "/stuff/weather.php"; //set the path to the weather script on the above host

WiFiServer server(80);

unsigned int localPort = 2390;      // local port to listen for UDP packets
const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets
WiFiUDP udp; // A UDP instance to let us send and receive packets over UDP
WiFiClient webClient;
WiFiClient serverClient;
unsigned long timeout;
bool wreq = false;
int timeOffset = 0;

void setup() {
  Serial.begin(9600);

  //WiFi Init
  WiFi.begin(ssid, pass);

  pinMode(2, OUTPUT);

  bool ledOn = false;
  
  while (WiFi.status() != WL_CONNECTED) {
    ledOn = !ledOn;
    digitalWrite(2, ledOn);
    delay(250);
  }
  digitalWrite(2, 0);
  udp.begin(localPort);
  server.begin();
  
  if(ntpServerName != ""){
    WiFi.hostByName(ntpServerName, timeServerIP); 
  }
  if(weatherServerHost != ""){
    WiFi.hostByName(ntpServerName, timeServerIP); 
  }

  //EEPROM Init
  EEPROM.begin(512);
  byte b = EEPROM.read(ADDRESS_TOFFSET);
  timeOffset = b-12;
  
  Serial.println("READY");
}

void loop() {
  if(Serial.available()){
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if(cmd.equalsIgnoreCase("TIME")){
      sendNTPpacket(timeServerIP);
      Serial.println("OK");
    }else if(cmd.equalsIgnoreCase("WEATHER")){
      sendWeatherRequest();
      Serial.println("OK");
    }else if(cmd.equalsIgnoreCase("IP")){
      Serial.print("DEBUG+");
      Serial.println(WiFi.localIP());
    }else if(cmd.equalsIgnoreCase("CLEAR")){
      clearSettings();
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
  if(webClient.available() > 0) {
    char buf[webClient.available()];
    for(int i=0;i<webClient.available();i++){
      buf[i] = webClient.read();
    }
    webClient.stop();
    Serial.print("DEBUG+");
    Serial.println(buf);
  }else if(wreq){
    if (millis() - timeout > 5000) {
      Serial.println("DEBUG+'Weather Request Timed Out'");
      webClient.stop();
    }
  }

  serverClient = server.available();
  if(serverClient){
    String req = serverClient.readStringUntil('\r');
    Serial.println("DEBUG+'" + req + "'");
    serverClient.flush();
    int index = req.indexOf("/config/");
    if(index != -1){
      index += 8;
      if(req.indexOf("?offset=") != -1){
       index += 8;
       int endIndex = req.indexOf(' ', index);
       String val = req.substring(index, endIndex);
       timeOffset = val.toInt();
       saveTimeOffset(timeOffset);
       serverOK(timeOffset);
       sendNTPpacket(timeServerIP);
      }else{
        serverBad();
      }
    }else{
      serverBad();
    }
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
  epoch = epoch + HOUR_SECONDS*timeOffset; //do timezone conversion
  return(epoch);
}

void sendWeatherRequest(){
  const int httpPort = 80;
  if (!webClient.connect(weatherServerHost, httpPort)) {
    Serial.println("DEBUG+'Weather Connection Failed'");
    return;
  }
  
  // This will send the request to the server
  webClient.print(String("GET ") + weatherServerPage + " HTTP/1.1\r\n" +
               "Host: " + weatherServerHost + "\r\n" + 
               "Connection: close\r\n\r\n");
  timeout = millis();
}

void clearSettings(){
  for(int i=0; i<512; i++){
    EEPROM.write(i, 0);
  }
  EEPROM.commit();
}

void saveTimeOffset(int offset){
  offset = offset+12;
  EEPROM.write(ADDRESS_TOFFSET, offset);
  EEPROM.commit();
}

void serverOK(int val){
  String s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\nValue set! val: ";
  s += val;
  s += "</html>\n";

  serverClient.print(s);
  serverClient.stop();
}

void serverBad(){
  String s = "HTTP/1.1 400 BAD REQUEST\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\nInvalid Request. Please check your syntax and try again.</html>\n";

  serverClient.print(s);
  serverClient.stop();
}


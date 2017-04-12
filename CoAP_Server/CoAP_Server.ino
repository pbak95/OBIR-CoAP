/**
   Implementation of CoAP Server
*/
#include <RF24Network.h>
#include <RF24.h>
#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>

//RADIO PART
RF24 radio(7, 8);               // nRF24L01(+) creating of RF24 object
RF24Network network(radio);      // Network uses that radio
const uint16_t this_node = 00;    // Address of our node in Octal format ( 04,031, etc)
const uint16_t other_node = 01;   // Address of the other node in Octal format

//Structure of payload sending to smart object
struct payload_t {
  unsigned long ms;
  unsigned long counter;
  unsigned short adc;
};
//packets counter for debug
unsigned long packets_sent;

//ETHERNET PART
byte mac[] = {0x00, 0xaa, 0xbb, 0xcc, 0xde, 0xf4}; //MAC address of ehernet shield
const int MAX_BUFFER = 80;
byte packetBuffer[MAX_BUFFER];
byte sendBuffer[MAX_BUFFER];
EthernetUDP Udp;    //initializing UDP object
short localPort = 1238;   //UDP port  


//SERIAL
String inString = "";

void setup(void)
{
  Serial.begin(115200);
  Serial.println("Starting CoAP Server:");
  Serial.println("Radio channel: 110");
  Serial.println("Radio address: 00");
  SPI.begin();
  radio.begin();
  network.begin(/*channel*/ 110, /*node address*/ this_node);
  Ethernet.begin(mac);
  Serial.println("IP: ");
  Serial.println(Ethernet.localIP());
  Serial.println("Port: 1238");
  Udp.begin(localPort);

}
void loop(void) {

  network.update();                  // Checking the network regularly

  //MESSAGE FROM SMART OBJECT
  while ( network.available() ) {     // Checking if any data is avaliable
    RF24NetworkHeader header;        // header struct, which is send with each message
    payload_t payload;              // payload initialization
    network.read(header, &payload, sizeof(payload));
    Serial.println("Received value #");
    Serial.print(payload.counter);
    Serial.print(" at ");
    Serial.print(payload.ms);
    Serial.println(" IN HEX: ");
    Serial.print(payload.adc, HEX);
    Serial.println(" /////////");

  }


  // SETTING LIGHT INTENSITY
  if (Serial.available()) {
    inString = Serial.readString();
    // if you get a newline, print the string,
    // then the string's value:
    unsigned short valueToSend = 0;
    uint8_t first = inString[0];
    uint8_t second = inString[1];
    valueToSend = (first - 48) * 8 + (second - 48) * 1;
    Serial.print("Value from Serial:");
    Serial.println(valueToSend, DEC);
    Serial.print("String: ");
    Serial.println(inString);
    unsigned long now = millis();
    payload_t payload;
    payload.ms = now;
    payload.counter = packets_sent++;
    payload.adc  = valueToSend;
    RF24NetworkHeader header(/*to node*/ other_node);
    //radio.stopListening();
    bool ok = network.write(header, &payload, sizeof(payload));
    if (ok)
      Serial.println("Sending frequency OK.");
    else
      Serial.println("Sending frequency FAILED.");
    // clear the string for new input:
    //radio.startListening();
    inString = "";

  }

  //ETHERNET PART
  int packetSize = Udp.parsePacket();
  if (packetSize) {
    Udp.read(packetBuffer, MAX_BUFFER);
    Serial.println(packetSize);

//ZOSTAWIŁEM ZEBY JUŻ NIE SKAKAĆ I BYŁ WZÓR Z LABY

//    byte data[packetSize - 5];
//    int j = 0;
//    for (int i = 5; i < packetSize; i++)
//    {
//      data[j] = packetBuffer[i];
//      j++;
//    }
//    if (data[0] == 71 && sended == false) {
//      //get
//      sended = true;
//      Serial.println("get");
//      Serial.println(level);
//      Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
//      sprintf(sendBuffer, "%x", level);
//      int r = Udp.write(sendBuffer, sizeof(sendBuffer));
//      Serial.println(r);
//      Udp.endPacket();
//    }
//    if (data[0] == 83) {
//      //set
//      level = 0;
//      Serial.println("set");
//      for (int i = 0; i < packetSize - 6; i++) {
//        if (data[packetSize - 6 - i] != 10) {
//          Serial.println(i);
//          int temp = (data[packetSize - 6 - i] - 48) * pow(10, i);
//          Serial.println(temp);
//          level += temp;
//        }
//      }
//      level += 1;
//      level = level / 10;
//      Serial.println(level);
//      analogWrite(3, level);
//    }
  }
}

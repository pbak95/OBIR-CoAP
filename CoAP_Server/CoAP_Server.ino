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
  uint8_t header;
  byte payload[5];
};

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
 if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // no point in carrying on, so do nothing forevermore:
    for(;;)
      ;
  }
  Serial.println("IP: ");
  Serial.println(Ethernet.localIP());
  Serial.println("Port: 1238");
  Udp.begin(localPort);

}
void loop(void) {
  network.update();                  // Checking the network regularly

  //MESSAGE FROM SMART OBJECT
  if ( network.available() ) {     // Checking if any data is avaliable
    RF24NetworkHeader header;        // header struct, which is send with each message
    payload_t payload;              // payload initialization
    network.read(header, &payload, sizeof(payload));
   
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
    RF24NetworkHeader header(/*to node*/ other_node);
    //radio.stopListening();
    payload_t payload;
    bool ok = network.write(header, &payload, sizeof(payload));
    if (ok)
      Serial.println("Sending payload OK.");
    else
      Serial.println("Sending payload FAILED.");
    // clear the string for new input:
    //radio.startListening();
    inString = "";
  }

  //ETHERNET PART
  int packetSize = Udp.parsePacket();
  if (packetSize) {
    Udp.read(packetBuffer, MAX_BUFFER);
    Serial.println(packetSize);

    /*0                   1                   2                   3
        0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |Ver| T |  TKL  |      Code     |          Message ID           |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |   Token (if any, TKL bytes) ...
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |   Options (if any) ...
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |1 1 1 1 1 1 1 1|    Payload (if any) ...
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    */

    //flag to indicate if coap field is handled
    bool flag = false;
    //flag to send back to client diagnostic payload
    bool diagnostic_payload = false;

    //header is first 4B of coap message
    int header_length = 4;

    //header analysis
    //field version no. value=01
    uint8_t ver = packetBuffer[0] >> 6;

    //field message type CON(0) NON(1) ACK(2) RST(3)
    uint8_t T = (packetBuffer[0] >> 4)& 3;
    if(T == 1)
    {
      Serial.println("Type: NON");
      flag = true;
    }
    else
    {
      Serial.println("Type: other");
      flag = true;
      diagnostic_payload = true;
    }

    //field token length 0-255
    Serial.println(packetBuffer[0], BIN);
    uint8_t TKL = packetBuffer[0] & 15;
    Serial.print("TKL: ");
    Serial.println(TKL, BIN);

    //field code class request(0) success response(010) client error response(100) server error response(101)
    uint8_t code_class = packetBuffer[1] >> 5;

    //field code detail empty(00000) GET(00001) POST(00010) PUT(00011) DELETE(00100) (dd) for response
    uint8_t code_detail = packetBuffer[1] & 31;

    Serial.print("Code: ");
    Serial.print(code_class, DEC);
    Serial.print(".");
    Serial.println(code_detail, DEC);

    //type of request
    int request_type;

    switch(code_class)
    {
      case 0:
      {
        //request
       if(code_detail == 1)
       {
        //GET
        Serial.println("GET request");
        request_type = 1;
       }
       else if(code_detail == 3)
       {
        //PUT
        Serial.println("PUT request");
        request_type = 3;
       }
       else
       {
        Serial.println("Different request");
        diagnostic_payload = true;
       }
       break;
      }
      case 2:
      {
        //success response
        Serial.println("success response");
        flag = false;
        break;
      }
      case 4:
      {
        //client error response
        Serial.println("client error response");
        flag = true;
        diagnostic_payload = true;
        break;
      }
      case 5:
      {
        //server error response
        Serial.println("server error response");
        flag = true;
        diagnostic_payload = true;
        break;
      }
    }

    //field messageID
    uint16_t MessageID = ((uint16_t)packetBuffer[2] << 8) | packetBuffer[3];
    Serial.println(MessageID, HEX);

    //if exists read token with TKL Bytes
    if(TKL != 0)
    {
      byte token[TKL];
      for(int i=0; i< TKL; ++i)
      {
        token[i] = packetBuffer[i + header_length];
      }
    }

    //iterator
    int iter = header_length + TKL;

    //until byte is not flag 11111111
    byte options[MAX_BUFFER];
    char * uri_path_option;
    int resource_id;

    //number that indicates option
    uint8_t option_no = 0;
    //loop until flag 11111111 that ends header
    while(packetBuffer[iter] != 255)
    {
      Serial.println("debug2");
      uint8_t opt_delta = packetBuffer[iter] >> 4;
      Serial.println(opt_delta, BIN);
      uint8_t opt_length = packetBuffer[iter] & 15;
      Serial.println(opt_length, BIN);
      byte opt_value[opt_length];
      for(int i = 0;i < opt_length; ++i)
      {
        ++iter;
        opt_value[i] = packetBuffer[iter];
        Serial.println(opt_value[i], BIN);
      }
      //TODO check whick option and do stuff
      option_no += opt_delta;
      switch(option_no)
      {
        case 4:
        {
          Serial.println("ETag option");
          byte etag_option[opt_length];
          for(int i=0;i<opt_length;++i)
          {
            etag_option[i]=opt_value[i];
          }
          break;
        }
        case 11:
        {
          Serial.println("Uri-Path option");
          uri_path_option = (char*) malloc (opt_length);
          for(int i=0;i<opt_length;++i)
          {
            uri_path_option[i]=opt_value[i];
          }

          if(uri_path_option == "lamp")
          {
            resource_id = 0;
          }
          if(uri_path_option == "button")
          {
            resource_id = 1;
          }
          break;
        }
        case 12:
        {
          Serial.println("Content-Format option");
          byte content_format_option[opt_length];
          for(int i=0;i<opt_length;++i)
          {
            content_format_option[i]=opt_value[i];
          }
          break;
        }
        case 17:
        {
          Serial.println("Accept option");
          byte acept_option[opt_length];
          for(int i=0;i<opt_length;++i)
          {
            acept_option[i]=opt_value[i];
          }
          break;
        }
        case 23:
        {
          Serial.println("Block2 option");
          byte byte2_option[opt_length];
          for(int i=0;i<opt_length;++i)
          {
            byte2_option[i]=opt_value[i];
          }
          break;
        }
        case 28:
        {
          Serial.println("Size2 option");
          byte size2_option[opt_length];
          for(int i=0;i<opt_length;++i)
          {
            size2_option[i]=opt_value[i];
          }
          break;
        }
      }
      ++iter;
    }

    ++iter;
    //rest is payload
    byte payload[packetSize - iter];
    for(int j = 0; j < packetSize - iter; ++j)
    {
      payload[j] = packetBuffer[iter + j];
      Serial.println(payload[j]);
    }

    //if flag=false do nothing, if flag=true send smth
    if(flag)
    {
      if(diagnostic_payload)
      {
        //send diagnostic payload to client
      }
      else
      {
        if(uri_path_option == NULL)
        {
          if(resource_id != 2)
          {
            sendToObject(request_type, resource_id, payload);
          }
          else
          {    sendToClient(resource_id);
          }
        }
      }
    }
  }
}

void sendToObject(int request_type, int resource_id, byte payload[])
{
  //function to send message to smart object
  Serial.println("sending message to object");
}

void sendToClient(int resource_id)
{
  //function to send message to coap client
  Serial.println("sending message to client");
}

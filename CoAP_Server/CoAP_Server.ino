#include <RF24Network.h>
#include <RF24Network_config.h>
#include <Sync.h>

#include <nRF24L01.h>
#include <RF24.h>
#include <RF24_config.h>

/**
   Implementation of CoAP Server
*/
#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <string.h>

//RADIO PART
RF24 radio(7, 8);               // nRF24L01(+) creating of RF24 object
RF24Network network(radio);      // Network uses that radio
const uint16_t this_node = 00;    // Address of our node in Octal format ( 04,031, etc)
const uint16_t other_node = 01;   // Address of the other node in Octal format



//Structure of payload sending to smart object
struct frame_t {
  uint8_t header;
  uint8_t state;
  uint32_t value;
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
  radio.begin();
  Serial.begin(115200);
  Serial.println(F("Starting CoAP Server:"));
  Serial.println(F("Radio channel:"));
  Serial.println(110, DEC);
  Serial.println(F("Radio address:"));
  Serial.println(this_node,OCT);
  SPI.begin();
  network.begin(/*channel*/ 110, /*node address*/ this_node);
 if (Ethernet.begin(mac) == 0) {
    Serial.println(F("Failed to configure Ethernet using DHCP"));
    // no point in carrying on, so do nothing forevermore:
    for(;;)
      ;
  }
  Serial.println(F("IP: "));
  Serial.println(Ethernet.localIP());
  Serial.println(F("Port: 1238"));
  Udp.begin(localPort);

}
void loop(void) {
  network.update();                  // Checking the network regularly

//MESSAGE FROM SMART OBJECT
  while (network.available() ) {     // Checking if any data is avaliable
    RF24NetworkHeader header;        // header struct, which is send with each message
    frame_t message;              // payload initialization
    network.read(header, &message, sizeof(message));
    uint8_t operation = message.header >> 6;
    uint8_t resource = message.header & 63;
    if(resource == 0){
     Serial.println(F("LIGHT"));
     if(operation == 0){
      Serial.println(F("GET"));
      Serial.println(F("Light intensity"));
      Serial.println(message.value);
     }else{
      Serial.println(F("PUT"));
      Serial.println(F("Light intensity set"));
      Serial.println(message.value);
     }
    }else if(resource == 1){
      Serial.println(F("BUTTON"));
      Serial.println(F("button state and time"));
      Serial.println(message.state);
      Serial.println(message.value);
    }
   
  }


  // SETTING LIGHT INTENSITY for debug
//  if (Serial.available()) {
//    inString = Serial.readString();
//    Serial.print("String: ");
//    Serial.println(inString);
//    RF24NetworkHeader header(/*to node*/ other_node);
//    frame_t message;
//    if(inString[0] == 'G'){
//      message.header = 0; // get i lampla
//    }else if(inString[0] == 'B'){
//    message.header = 1;
//    }
//    else{
//      message.header = 64;
//      message.value = 500;
//      Serial.print(message.value,DEC);
//    }
//    bool test = radio.testCarrier();
//    if(test){
//      Serial.println("CARRIER WAS");
//    }else{
//       Serial.println("NO CARRIER");
//    }
//    bool ok = network.write(header, &message, sizeof(message));
//    if (ok)
//      Serial.println("Sending payload OK.");
//    else
//      Serial.println("Sending payload FAILED.");
//    inString = "";
//  }

//  //ETHERNET PART
  int packetSize = Udp.parsePacket();
  if (packetSize) {
    Udp.read(packetBuffer, MAX_BUFFER);
    Serial.println(packetSize);
//
//    /*0                   1                   2                   3
//        0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//       |Ver| T |  TKL  |      Code     |          Message ID           |
//       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//       |   Token (if any, TKL bytes) ...
//       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//       |   Options (if any) ...
//       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//       |1 1 1 1 1 1 1 1|    Payload (if any) ...
//       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//    */
//
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
      Serial.println(F("Type: NON"));
      flag = true;
    }
    else
    {
      Serial.println(F("Type: other"));
      flag = true;
      diagnostic_payload = true;
    }

    //field token length 0-255
    Serial.println(packetBuffer[0], BIN);
    uint8_t TKL = packetBuffer[0] & 15;
    Serial.print(F("TKL: "));
    Serial.println(TKL, BIN);

    //field code class request(0) success response(010) client error response(100) server error response(101)
    uint8_t code_class = packetBuffer[1] >> 5;

    //field code detail empty(00000) GET(00001) POST(00010) PUT(00011) DELETE(00100) (dd) for response
    uint8_t code_detail = packetBuffer[1] & 31;

    Serial.print(F("Code: "));
    Serial.print(code_class, DEC);
    Serial.print(F("."));
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
        Serial.println(F("GET request"));
        request_type = 1;
       }
       else if(code_detail == 3)
       {
        //PUT
        Serial.println(F("PUT request"));
        request_type = 3;
       }
       else
       {
        Serial.println(F("Different request"));
        diagnostic_payload = true;
       }
       break;
      }
      case 2:
      {
        //success response
        Serial.println(F("success response"));
        flag = false;
        break;
      }
      case 4:
      {
        //client error response
        Serial.println(F("client error response"));
        flag = true;
        diagnostic_payload = true;
        break;
      }
      case 5:
      {
        //server error response
        Serial.println(F("server error response"));
        flag = true;
        diagnostic_payload = true;
        break;
      }
    }

    //field messageID
    uint16_t MessageID = ((uint16_t)packetBuffer[2] << 8) | packetBuffer[3];
    Serial.println(MessageID, DEC);

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
    //Uri-path for indicating the resource
    char * uri_path_option;
    //the relative number of the block within a sequence of blocks
    uint8_t NUM;
    //whether more blocks are following
    uint8_t M;
    //the size of the block
    uint8_t SZX;
    //Size2 for indicating the size of the representation transferred in responses
    byte * size2_option;
    int resource_id;
	  byte content_format_option;

    //number that indicates option
    uint8_t option_no = 0;

    //length of large resource
    uint8_t wellknownLength = 0;
    //length of header without options
    uint8_t h_len_wo_opt = iter;

    // .wellknown/core payload
    byte *body;
    
    //loop until flag 11111111 that ends header
    while((packetBuffer[iter] != 255) && (packetBuffer[iter] != 0))
    {
      uint8_t opt_delta = packetBuffer[iter] >> 4;
      Serial.println(opt_delta, BIN);
      uint8_t opt_length = packetBuffer[iter] & 15;
      Serial.println(opt_length, BIN);

      byte *opt_value;
      if(opt_delta == 13)
      {
        //extended delta by 1B
        ++iter;
        option_no += packetBuffer[iter] - 13;
      }
      else if (opt_delta == 14)
      {
        //extended delta by 2B
        ++iter;
        int number = packetBuffer[iter] | packetBuffer[++iter] << 8;
        option_no += number - 269;
      }
      else
      {
        option_no += opt_delta;
      }

      if(opt_length == 13)
      {
        //extended length by 1B
        ++iter;
        opt_value = (byte*) malloc (packetBuffer[iter] - 13);
      }
      else if (opt_length == 14)
      {
        //extended length by 2B
        int number = packetBuffer[iter] | packetBuffer[++iter] << 8;
        opt_value = (byte*) malloc (number - 269);
      }
      else
      {
        opt_value = (byte*) malloc (opt_length);
      }
      for(int i = 0;i < opt_length; ++i)
      {
        ++iter;
        opt_value[i] = packetBuffer[iter];
      }

      switch(option_no)
      {
        case 4:
        {
          Serial.println(F("ETag option"));
          byte etag_option[opt_length];
		      Serial.print(F("ETag: "));
          for(int i=0;i<opt_length;++i)
          {
            etag_option[i]=opt_value[i];
			      Serial.print(etag_option[i]);
			      Serial.print(F(" , "));
          }
          break;
        }
		case 11:
        {
          Serial.println(F("Uri-Path option"));
          uri_path_option = (char*) malloc (opt_length);
          for(int i=0;i<opt_length;++i)
          {
            uri_path_option[i]=opt_value[i];
          }

          if(strncmp(uri_path_option, "lamp",4) == 0)
          {
            Serial.println(F("To jest lampa"));
            resource_id = 0;
            if(request_type == 1)
            {
              sendGetToObject(resource_id);
            }
          }
          else if(strncmp(uri_path_option, "button",6) == 0)
          {
            Serial.println(F("To jest button"));
            resource_id = 1;
            sendGetToObject(resource_id);
          }
          else if(strncmp(uri_path_option, "radio",5) == 0)
          {
            Serial.println(F("To jest radio"));
            resource_id = 2;
            //TO_DO po stronie serwera
          }
          else if(strncmp(uri_path_option, ".well-known/core",16) ==0)
          {
            Serial.println(F("To jest .well-known/core"));
            body = "<button>;rt=\"button\";if=\"sensor\",<light>;rt=\"light\";/if=\"sensor\",<radio>;rt=\"radio\";if=\"sensor\"";
            wellknownLength = strlen(body);
            resource_id = 3;
          }
           break;
        }
        case 12:
        {
          Serial.println(F("Content-Format option"));
          content_format_option=opt_value[0];
          Serial.print(F("Content-Format: "));
          if(content_format_option == 0)
    		  {
            Serial.println(F("text/plain"));
    		  }
    		  else if(content_format_option == 40)
    		  {
            Serial.println(F("application/link-format"));
            Serial.print(F("Tego nie obs�ugujemy"));
    		  }
    		  else if(content_format_option == 41)
    		  {
            Serial.println(F("application/xml"));
            Serial.print(F("Tego nie obs�ugujemy"));
    		  }
    		  else if(content_format_option == 42)
    		  {
            Serial.println(F("application/octet-stream"));
            Serial.print(F("Tego nie obs�ugujemy"));
    		  }
    		  else if(content_format_option == 47)
    		  {
            Serial.println(F("application/exi"));
            Serial.print(F("Tego nie obs�ugujemy"));
    		  }
    		  else if(content_format_option == 50)
    		  {
            Serial.println(F("application/json"));
            Serial.print(F("Tego nie obs�ugujemy"));
    		  }
    		  break;
        }
        case 17:
        {
          Serial.println(F("Accept option"));
          byte accept_option;
          accept_option=opt_value[0];
          if(accept_option == content_format_option)
            Serial.println(F("Accept"));
    		  else
            Serial.println(F("Not Accept"));
          break;
        }
        case 23:
        {
          Serial.println(F("Block2 option"));
          byte byte2_option[opt_length];
          if(opt_length == 1)
          {
            NUM = byte2_option[0] >> 4;
            M = (byte2_option[0] >> 3) & 1;
            SZX = byte2_option[0] & 7;
          }
          else
          {
            //we assume that there will bo no longer payload than 15 blocks
            Serial.println(F("Block2 too long"));
            sendDiagnosticPayload();
          }
          break;
        }
        case 28:
        {
          Serial.println(F("Size2 option"));
          size2_option = (byte*) malloc (opt_length);
          for(int i=0;i<opt_length;++i)
          {
            size2_option[i]=opt_value[i];
          }
          break;
        }
      }
      ++iter;
    }
    if(packetBuffer[iter] != 0)
    {
        
    
      ++iter;
      //rest is payload
      byte payload[packetSize - iter];
      for(int j = 0; j < packetSize - iter; ++j)
      {
        payload[j] = packetBuffer[iter + j];
        Serial.println(payload[j]);
      }
      
      //Sending put to LAMP
      Serial.println(F("Method PUT"));
      int payloadSize = sizeof(payload) / sizeof(byte);
      sendPutToObject(payload, payloadSize);
      
    }
    //if flag=false do nothing, if flag=true send smth
    if(flag)
    {
      if(diagnostic_payload)
      {
        sendDiagnosticPayload();
      }
      else
      {
        if(uri_path_option == NULL)
        {
          if(resource_id < 2)
          {
            sendGetToObject(resource_id);
          }
          else
          {
            //send radio resource to client
            byte headerToSend[h_len_wo_opt+6];
            int it=0;
            headerToSend[it] = packetBuffer[0];
            //Code 2.05 OK 01000101
            headerToSend[++it] = 69;
            //MID ab
            headerToSend[++it] = 97;
            headerToSend[++it] = 98;
            //if exists copy token with TKL Bytes
            if(TKL != 0)
            {
              memcpy ( &headerToSend+4, &packetBuffer+4, TKL );
            }
            //TODO options
            //Content Format delta 12 length 1 -> 193
            headerToSend[++it + TKL] = 193;
            if(content_format_option == 50)
            {
              //content-format = 40
              headerToSend[++it + TKL] = 40;
            }
            else
            {
              //content-format = 0
              headerToSend[++it + TKL] = 40;
            }
            //Block2 delta 11 length 1 -> 177
            headerToSend[++it + TKL] = 177;
            //TODO check if M=0 or M=1 if is another packet to send
            headerToSend[++it + TKL] = NUM | M | SZX;

            //Size2 delta 5 length 1?
            headerToSend[++it + TKL] = 81;
            headerToSend[++it + TKL] = wellknownLength;

            if(resource_id == 2)
            {
              //send radio in payload
              //TODO skleic payload z radiem do wyslania
              byte payloadToSend[1];
              //TO REMOVE
              payloadToSend[0] = 97;
              sendToClient(headerToSend, payloadToSend);
            }
            if(resource_id == 3)
            {
              //send specified block of .wellknown/core
              //TODO check which fragment           
              byte payloadToSend[SZX];
              //TODO parse body
              memcpy ( &payloadToSend, &body+(NUM*SZX), SZX );
              sendToClient(headerToSend, payloadToSend);
            }
          }
        }
      }
    }
  }
}

void sendGetToObject(int resource_id)
{
  //function to send message to smart object
  Serial.println(F("sending message to object"));
  RF24NetworkHeader header(/*to node*/ other_node);
  frame_t message;
  if(resource_id == 0)
  {
    Serial.println(F("Sending GET LAMP"));
    message.header = 0;
  }
  else if(resource_id == 1)
  {
    Serial.println(F("Sending GET BUTTON"));
    message.header = 1;
  }

   bool ok = network.write(header, &message, sizeof(message));
    if (ok)
      Serial.println(F("Sending payload OK."));
    else
      Serial.println(F("Sending payload FAILED."));
}

void sendPutToObject(byte payload[], int payloadSize)
{
   //function to send message to smart object
  Serial.println(F("sending message to object, payload size:"));
  Serial.println(payloadSize);
  RF24NetworkHeader header(/*to node*/ other_node);
  frame_t message;
  message.header = 64;
  for(int i=0; i< payloadSize; i++)
  {
    message.value = (payload[i] - 48) * pow(10, payloadSize -1 -i);
  }

  if(message.value > 1000)
    message.value = 1000;
  

  Serial.println(F("Sending PUT lamp with value"));
  Serial.println(message.value, DEC);
  bool ok = network.write(header, &message, sizeof(message));
  if (ok)
    Serial.println(F("Sending payload OK."));
  else
    Serial.println(F("Sending payload FAILED."));
}

void sendToClient(byte header[], byte payload[])
{
  //function to send message to coap client
  //packet size header + flag + payload
  uint8_t flag = 255;
  byte packet[(sizeof(header)/sizeof(header[0])) + 1 + (sizeof(payload)/sizeof(payload[0]))];

  //fill packet array with function arguments
  memcpy ( &packet, &header, sizeof(header) );
  memcpy ( &packet+sizeof(header), &flag, sizeof(flag) );
  memcpy ( &packet+sizeof(header)+sizeof(flag), &payload, sizeof(payload) );
  Serial.println(F("sending message to client"));
  Udp.write(packet, sizeof(packet));
}

void sendDiagnosticPayload()
{
  //function to send to client diagnostic payload
  int buffer_size = 4;
  byte header[buffer_size];
  //Ver: 01 NON: 01 TKL: 0000
  header[0] = 01010000;
  //Code: 5.01 - not implemented
  header[1] = 10100001;
  //MID: ab
  header[2] = 97;
  header[3] = 98;
  
  //send by udp
  Serial.println(F("sending diagnostic payload to client"));
  Udp.write(header, buffer_size);
}


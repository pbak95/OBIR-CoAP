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
  Serial.println("Starting CoAP Server:");
  Serial.println("Radio channel:");
  Serial.println(110, DEC);
  Serial.println("Radio address:");
  Serial.println(this_node,OCT);
  SPI.begin();
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
  while (network.available() ) {     // Checking if any data is avaliable
    RF24NetworkHeader header;        // header struct, which is send with each message
    frame_t message;              // payload initialization
    network.read(header, &message, sizeof(message));
    uint8_t operation = message.header >> 6;
    uint8_t resource = message.header & 63;
    if(resource == 0){
     Serial.println("LIGHT"); 
     if(operation == 0){
      Serial.println("GET");
      Serial.println("Light intensity");
      Serial.println(message.value);
     }else{
      Serial.println("PUT");
      Serial.println("Light intensity set");
      Serial.println(message.value);
     }
    }else if(resource == 1){
      Serial.println("BUTTON");
      Serial.println("button state and time");
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
    char * uri_path_option;
    int resource_id;
	  byte content_format_option;

    //number that indicates option
    uint8_t option_no = 0;
    //loop until flag 11111111 that ends header
    while((packetBuffer[iter] != 255) && (packetBuffer[iter] != 0))
    {
      uint8_t opt_delta = packetBuffer[iter] >> 4;
      Serial.println(opt_delta, BIN);
      uint8_t opt_length = packetBuffer[iter] & 15;
      Serial.println(opt_length, BIN);

      byte *opt_value;
      byte *body;
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
        //Serial.println(opt_value[i], BIN);
      }

      switch(option_no)
      {
        case 4:
        {
          Serial.println("ETag option");
          byte etag_option[opt_length];
		      Serial.print("ETag: ");
          for(int i=0;i<opt_length;++i)
          {
            etag_option[i]=opt_value[i];
			      Serial.print(etag_option[i]);
			      Serial.print(" , ");
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

          if(strncmp(uri_path_option, "lamp",4) == 0)
          {
            Serial.println("To jest lampa");
            resource_id = 0;
            if(request_type == 1)
            {
              sendGetToObject(resource_id);
            }
          }
          else if(strncmp(uri_path_option, "button",6) == 0)
          {
            Serial.println("To jest button");
            resource_id = 1;
            sendGetToObject(resource_id);
          }
          else if(strncmp(uri_path_option, "radio",5) == 0)
          {
            Serial.println("To jest radio");
            resource_id = 2;
            //TO_DO po stronie serwera
          }
          else if(strncmp(uri_path_option, ".well-known/core",16) ==0)
          {
            Serial.println("To jest .well-known/core");
            body = "<button>;rt=\"button\";if=\"sensor\",<light>;rt=\"light\";/if=\"sensor\",<radio>;rt=\"radio\";if=\"sensor\"";
            byte payload[strlen(body)];
            for(int i = 0; i<strlen(body); ++i)
            {
              payload[i] = body[i];
            }
          }
           break;
        }
        case 12:
        {
          Serial.println("Content-Format option");
          content_format_option=opt_value[0];
    		  Serial.print("Content-Format: ");
    		  if(content_format_option == 0)
    		  {
    			  Serial.println("text/plain");
    		  }
    		  else if(content_format_option == 40)
    		  {
      			Serial.println("application/link-format");
      			Serial.print("Tego nie obsługujemy");
    		  }
    		  else if(content_format_option == 41)
    		  {
      			Serial.println("application/xml");
      			Serial.print("Tego nie obsługujemy");
    		  }
    		  else if(content_format_option == 42)
    		  {
      			Serial.println("application/octet-stream");
      			Serial.print("Tego nie obsługujemy");
    		  }
    		  else if(content_format_option == 47)
    		  {
      			Serial.println("application/exi");
      			Serial.print("Tego nie obsługujemy");
    		  }
    		  else if(content_format_option == 50)
    		  {
      			Serial.println("application/json");
      			Serial.print("Tego nie obsługujemy");
    		  }
    		  break;
        }
        case 17:
        {
          Serial.println("Accept option");
          byte acept_option;
          acept_option=opt_value[0];
    		  if(acept_option == content_format_option)
    			  Serial.println("Accept");
    		  else
    			  erial.println("Not Accept");
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
      Serial.println("Method PUT");
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
          if(resource_id != 2)
          {
            sendGetToObject(resource_id);
          }
          else
          {    sendToClient(resource_id);
          }
        }
      }
    }
  }
}

void sendGetToObject(int resource_id)
{
  //function to send message to smart object
  Serial.println("sending message to object");
  RF24NetworkHeader header(/*to node*/ other_node);
  frame_t message;
  if(resource_id == 0)
  {
    Serial.println("Sending GET LAMP");
    message.header = 0;
  }
  else if(resource_id == 1)
  {
    Serial.println("Sending GET BUTTON");
    message.header = 1;
  }

   bool ok = network.write(header, &message, sizeof(message));
    if (ok)
      Serial.println("Sending payload OK.");
    else
      Serial.println("Sending payload FAILED.");
}

void sendPutToObject(byte payload[], int payloadSize)
{
   //function to send message to smart object
  Serial.println("sending message to object, payload size:");
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
  

  Serial.println("Sending PUT lamp with value");
  Serial.println(message.value, DEC);
  bool ok = network.write(header, &message, sizeof(message));
  if (ok)
    Serial.println("Sending payload OK.");
  else
    Serial.println("Sending payload FAILED.");
}

void sendToClient(int resource_id)
{
  //function to send message to coap client
  Serial.println("sending message to client");
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
  Serial.println("sending diagnostic payload to client");
  Udp.write(header, buffer_size);
}


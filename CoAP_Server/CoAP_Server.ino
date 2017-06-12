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

uint32_t counterMessageOk;
uint32_t counterMessageFailed;
uint8_t id;
uint16_t MID_counter = 0;
byte* coap_header = {};
uint8_t coap_header_size = 0;
int fragmentation_size = 64;
bool payload_sended = false;

//ETHERNET PART
byte mac[] = {0x00, 0xaa, 0xbb, 0xcc, 0xde, 0xf4}; //MAC address of ehernet shield
const int MAX_BUFFER = 80;
int RADIO_PAYLOAD_SIZE = 11;
int LIGHT_PAYLOAD_SIZE = 15;
int BUTTON_PAYLOAD_SIZE = 19;
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

      int n = log10(message.value)+1;
      char string[n];
      sprintf(string, "%01d", message.value);
      for(int i=0;i<sizeof(string)/sizeof(char);++i)
      {
        Serial.println(string[i]);
      }
      
      byte toSend[12+n];
      toSend[0] = 76;//L
      toSend[1] = 73;//I
      toSend[2] = 71;//G
      toSend[3] = 72;//H
      toSend[4] = 84;//T
      toSend[5] = 32;
      toSend[6] = 83;//S
      toSend[7] = 84;//T
      toSend[8] = 65;//A
      toSend[9] = 84;//T
      toSend[10] = 69;//E
      toSend[11] = 58;//:
      for(int i=0;i<sizeof(string)/sizeof(char);++i)
      {
        toSend[12+i] = string[i];
      }

      LIGHT_PAYLOAD_SIZE = 12 + n;
      //send to client message.value
      sendToClient(coap_header,coap_header_size, toSend, sizeof(toSend));
     }
    }else if(resource == 1){
      Serial.println(F("BUTTON"));
      Serial.println(F("button state and time"));
      Serial.println(message.state);
      Serial.println(message.value);
      int n = 1;
      message.value = message.value/1000;
      if(message.value != 0)
        n = log10(message.value)+1;

      char string[n];
      sprintf(string, "%01d", message.value);
      for(int i=0;i<sizeof(string)/sizeof(char);++i)
      {
        Serial.println(string[i]);
      }
      int k = 0;
      if(message.state == 0)
      {
        k = 3;
      }
      else if(message.state == 255)
      {
        k  = 2;
      }
      
      byte toSend[12+k+n];
      toSend[0] = 83;//S
      toSend[1] = 84;//T
      toSend[2] = 65;//A
      toSend[3] = 84;//T
      toSend[4] = 69;//E
      toSend[5] = 58;//:
      if(k==3)
      {
          toSend[6] = 79;//O
          toSend[7] = 70;//F
          toSend[8] = 70;//F  
      }
      else
      {
         toSend[6] = 79;//O
         toSend[7] = 78;//N
      }
      toSend[6+k] = 32;//
      toSend[7+k] = 84;//T
      toSend[8+k] = 73;//I
      toSend[9+k] = 77;//M
      toSend[10+k] = 69;//E
      toSend[11+k] = 58;//:
      for(int i=0;i<sizeof(string)/sizeof(char);++i)
      {
        toSend[12+k+i] = string[i];
      }
      BUTTON_PAYLOAD_SIZE = 12 + n + k;
      //send to client message.value
      sendToClient(coap_header,coap_header_size, toSend ,sizeof(toSend));
    }

  }


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
    //flag to indicate diagnostic payload
    bool payload_sended = false;

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
      payload_sended = true;
      sendDiagnosticPayload();
      return;
      
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
        sendDiagnosticPayload();
        payload_sended = true;
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
        sendDiagnosticPayload();
        payload_sended = true;
        break;
      }
      case 5:
      {
        //server error response
        Serial.println(F("server error response"));
        flag = true;
        sendDiagnosticPayload();
        payload_sended = true;
        break;
      }
    }

    //field messageID
    uint16_t MessageID = ((uint16_t)packetBuffer[2] << 8) | packetBuffer[3];
    Serial.print(F("MessageID: "));
    Serial.println(MessageID, DEC);

    //if exists read token with TKL Bytes
    if(TKL != 0)
    {
      byte token[TKL];
      Serial.println(F("Token: "));
      for(int i=0; i< TKL; ++i)
      {
        token[i] = packetBuffer[i + header_length];
        Serial.println(token[i]);
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
    uint8_t SZX = 2;
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
    //wellknown/core flag
    bool wellknownflag = false;
              
    //loop until flag 11111111 that ends header
    while((packetBuffer[iter] != 255) && (packetBuffer[iter] != 0))
    {
      Serial.print("Iter = ");
      Serial.println(iter);
      uint8_t opt_delta = packetBuffer[iter] >> 4;
      uint8_t opt_length = packetBuffer[iter] & 15;

      byte *opt_value;
      if(opt_delta == 13)
      {
        Serial.println("opt_delta=13");
        //extended delta by 1B
        ++iter;
        option_no += packetBuffer[iter] - 13;
      }
      else if (opt_delta == 14)
      {
        Serial.println("opt_delta=14");
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
        Serial.println("opt_length=13");
        ++iter;
        opt_value = (byte*) malloc (packetBuffer[iter] - 13);
      }
      else if (opt_length == 14)
      {
        //extended length by 2B
        Serial.println("opt_length=14");
        int number = packetBuffer[iter] | packetBuffer[++iter] << 8;
        opt_value = (byte*) malloc (number - 269);
      }
      else
      {
        opt_value = (byte*) malloc (opt_length);
      }
      
      Serial.println(F("delta: "));
      Serial.println(opt_delta, BIN);
      Serial.println(F("length: "));
      Serial.println(opt_length, BIN);
      Serial.println(F("option_no: "));
      Serial.println(option_no, DEC);
      
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
		      Serial.println(F("ETag: "));
          for(int i=0;i<opt_length;++i)
          {
            etag_option[i]=opt_value[i];
			      Serial.println(etag_option[i]);
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
            Serial.println(uri_path_option[i]);
          }

          if(strncmp(uri_path_option, "lamp",4) == 0)
          {
            Serial.println(F("To jest  a"));
            resource_id = 0;
            if(request_type == 1)
            {
              sendGetToObject(resource_id);
            }
          }
          else if(strncmp(uri_path_option, "button",6) == 
          0)
          {
            Serial.println(F("To jest button"));
            resource_id = 1;
            //sendGetToObject(resource_id);
          }
          else if(strncmp(uri_path_option, "radio",5) == 0)
          {
            Serial.println(F("To jest radio"));
            resource_id = 2;
          }
          else if(strncmp(uri_path_option, ".well-known/core",11) == 0)
          {
            wellknownflag = true;
          }
          else if(strncmp(uri_path_option, "core",4) == 0 && wellknownflag == true)
          {
            Serial.println(F("To jest .well-known/core"));
            body = "<button>;rt=\"button\";if=\"sensor\",<light>;rt=\"light\";if=\"sensor\",<radio>;rt=\"radio\";if=\"sensor\"";
            wellknownLength = strlen(body);
            resource_id = 3;            
          }
           break;
        }
        case 12:
        {
          Serial.println(F("Content-Format option"));
          if(opt_length == 0)
          {
            content_format_option = 0;
          }
          else
          {
            content_format_option=opt_value[0]; 
          }
          Serial.print(F("Content-Format: "));
          Serial.println(content_format_option, DEC);
          if(content_format_option == 0)
    		  {
            Serial.println(F("text/plain"));
    		  }
    		  else if(content_format_option == 40)
    		  {
            Serial.println(F("application/link-format"));
    		  }
    		  else if(content_format_option == 41)
    		  {
            Serial.println(F("application/xml"));
    		  }
    		  else if(content_format_option == 42)
    		  {
            Serial.println(F("application/octet-stream"));
    		  }
    		  else if(content_format_option == 47)
    		  {
            Serial.println(F("application/exi"));
    		  }
    		  else if(content_format_option == 50)
    		  {
            Serial.println(F("application/json"));
    		  }
    		  break;
        }
        case 17:
        {
          Serial.println(F("Accept option"));
          byte accept_option;
          if(opt_length == 0)
          {
            accept_option = 0;
          }
          else
          {
            accept_option=opt_value[0]; 
          }
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
            byte2_option[0] = opt_value[0];
            NUM = byte2_option[0] >> 4;
            M = (byte2_option[0] >> 3) & 1;
            SZX = byte2_option[0] & 7;
            fragmentation_size = pow(2,SZX+4) + 1;
            Serial.println(F("Number: "));
            Serial.println(NUM);
            Serial.println(F("If-next: "));
            Serial.println(M,BIN);
            Serial.println(F("Fragmentation size: "));
            Serial.println(fragmentation_size, DEC);
          }
          else
          {
            //we assume that there will bo no longer payload than 15 blocks
            Serial.println(F("Block2 too long"));
            sendDiagnosticPayload();
            payload_sended = true;
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
    //length of header with options
    uint8_t h_len_w_opt = iter;
    
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

      if(!payload_sended)
      {
        //Sending put to LAMP
        Serial.println(F("Method PUT"));
        int payloadSize = sizeof(payload) / sizeof(byte);
        sendPutToObject(payload, payloadSize); 
      }   
    }
    //if flag=false do nothing, if flag=true send smth
    if(flag)
    {
      if(payload_sended == false)
      {
        if(uri_path_option != NULL)
        {
          byte headerToSend[h_len_wo_opt+6];
          int it=0;
          headerToSend[it] = packetBuffer[0];
          //Code 2.05 OK 01000101
          headerToSend[++it] = 69;
          //MID ab
          ++MID_counter;
          headerToSend[++it] = MID_counter >> 8;
          headerToSend[++it] = MID_counter;
          //if exists copy token with TKL Bytes
          if(TKL != 0)
          {
            //memcpy ( &headerToSend+4, &packetBuffer+4, TKL );
            for (int i=0; i<TKL; ++i)
            {
              headerToSend[i+4] = packetBuffer[i+4];
            }
            Serial.println("tkd;");
            Serial.println(headerToSend[it +TKL], HEX);
          }
          
          //Content Format delta 12 length 1 -> 193
          headerToSend[++it + TKL] = 193;

          if(wellknownflag == true)
          {
            //content-format = 40
            headerToSend[++it + TKL] = 40;
          }
          else
          {
            //content-format = 0
            headerToSend[++it + TKL] = 0;
          }
          //Block2 delta 11 length 1 -> 177
          headerToSend[++it + TKL] = 177;
          if(resource_id != 3)
          {
            headerToSend[++it + TKL] = SZX | M <<3 | NUM << 4;
          }
          else
          {
            if((NUM + 1) * fragmentation_size >= wellknownLength)
            {
              M = 0;
            }
            else
            {
              M = 1;
            }
            headerToSend[++it + TKL] = SZX | M <<3 | NUM << 4;
          }

          //Size2 delta 5 length 1?
          headerToSend[++it + TKL] = 81;
          
          if(resource_id == 3)
          { 
            headerToSend[++it + TKL] = wellknownLength;
          }
          if(resource_id == 2)
          {
            headerToSend[++it + TKL] = RADIO_PAYLOAD_SIZE;
          }
          if(resource_id == 1)
          {
            headerToSend[++it + TKL] = BUTTON_PAYLOAD_SIZE;
          }
          if(resource_id == 0)
          {
            headerToSend[++it + TKL] = LIGHT_PAYLOAD_SIZE;
          }
          
          if(resource_id < 2)
          {
            coap_header_size = sizeof(headerToSend)/sizeof(headerToSend[0]);
            coap_header =  malloc (coap_header_size);
            for(int i= 0; i<coap_header_size;++i)
            {
              coap_header[i] = headerToSend[i];
            }
            sendGetToObject(resource_id);
          }
          if(resource_id == 2)
          {
            //send radio in payload
            byte * radioState = getRadioState();
            sendToClient(headerToSend, sizeof(headerToSend), radioState, RADIO_PAYLOAD_SIZE);
          }
          if(resource_id == 3)
          {
            //send specified block of .wellknown/core
            Serial.println(F("debug")); 
            Serial.println(fragmentation_size,DEC);    
            byte payloadToSend[fragmentation_size];
            for(int i=0;i<fragmentation_size;++i)
            {
              payloadToSend[i] = body[NUM*fragmentation_size + i];
            }
            sendToClient(headerToSend, sizeof(headerToSend), payloadToSend, sizeof(payloadToSend));
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
    {
      Serial.println(F("Sending payload OK."));
      counterMessageOk++;
    }
    else
    {
      Serial.println(F("Sending payload FAILED."));
      counterMessageFailed++;
    }
}

void sendPutToObject(byte payload[], int payloadSize)
{
   //function to send message to smart object
  Serial.println(F("sending message to object, payload size:"));
  Serial.println(payloadSize);
  RF24NetworkHeader header(/*to node*/ other_node);
  frame_t message;
  message.header = 64;
  message.value = 0;
  for(int i=0; i< payloadSize; i++)
  {
    message.value += (payload[i] - 48) * pow(10, payloadSize -1 -i);
  }
  ++message.value;
  if(message.value > 1000)
    message.value = 1000;
  

  Serial.println(F("Sending PUT lamp with value"));
  Serial.println(message.value, DEC);
  bool ok = network.write(header, &message, sizeof(message));
  if (ok)
  {
    Serial.println(F("Sending payload OK."));
    counterMessageOk++;
  }
  else
  {
    Serial.println(F("Sending payload FAILED."));
    counterMessageFailed++;
  }
}

void sendToClient(byte *header,int header_size, byte *payload, int payload_size)
{
  //function to send message to coap client
  //packet size header + flag + payload
  byte packet[header_size + 1 + payload_size];

  //fill packet array with function arguments
  memcpy ( &packet, header, header_size );  
  packet[header_size]= 255;
  int it=0;
  for(int i=header_size+1;i<sizeof(packet)/sizeof(packet[0]);++i)
  {
    packet[i]= payload[it];
    ++it;
  }

  Serial.println(F("message"));
  for(int i=0;i<sizeof(packet)/sizeof(packet[0]);++i)
  {
    Serial.println(packet[i],BIN);
  }
  Serial.println(F("sending message to client"));
  Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
  int r=Udp.write(packet, sizeof(packet));
  Serial.println(r);
  Udp.endPacket();
}

void sendDiagnosticPayload()
{
  //function to send to client diagnostic payload
  int buffer_size = 4;
  byte header[buffer_size];
  //Ver: 01 NON: 01 TKL: 0000
  header[0] = 80;
  //Code: 5.01 - not implemented 10100001
  header[1] = 161;
  //MID: ab
  ++MID_counter;
  header[2] = MID_counter >> 8;
  header[3] = MID_counter;
  
  //send by udp
  for(int i=0; i<buffer_size; ++i)
  {
    Serial.println(header[i], BIN);
  }
  Serial.println(F("sending diagnostic payload to client"));
  Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
  int r=Udp.write(header, buffer_size);
  Serial.println(r);
  Udp.endPacket();
}

byte * getRadioState(){
  int n = 1;
  int m = 1;
  bool testCarrier = radio.testCarrier();
 // network.failures(_fails,_ok);
  Serial.println(F("Radio stats O(OK sended payloads), N(not OK sen...), C(bool if there was a carrier on previous listening period)"));
  Serial.println(counterMessageOk);
  Serial.println(counterMessageFailed);
  Serial.println(testCarrier);
  
  //digits in countermessageok
  if(counterMessageOk != 0)
    n = log10(counterMessageOk)+1;
  //digits in countermessagefailed
  if(counterMessageFailed != 0)
    m = log10(counterMessageFailed)+1;
  char stringN[n];
  char stringM[m];
  sprintf(stringN, "%01d", counterMessageOk);
  sprintf(stringM, "%01d", counterMessageFailed);
  
  byte radioState[9+n+m];
  int iter=0;
  radioState[iter] = 79;//o
  radioState[++iter] = 58;//:
  for(int i=0;i<sizeof(stringN)/sizeof(char);++i)
  {
    radioState[++iter+i] = stringN[i];
  }
  radioState[++iter] = 32;
  radioState[++iter] = 78;//N
  radioState[++iter] = 58;//:
  for(int i=0;i<sizeof(stringM)/sizeof(char);++i)
  {
    radioState[++iter+i] = stringM[i];
  }
  radioState[++iter] = 32;// space
  radioState[++iter] = 67;//C
  radioState[++iter] = 58;//:
  radioState[++iter] = testCarrier + 48; // 0 - false , 1 - true
  RADIO_PAYLOAD_SIZE = ++iter;
  Serial.println(F("adsfgasfadf"));
  Serial.println(iter);
  return radioState;
}





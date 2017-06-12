#include <nRF24L01.h>
#include <RF24_config.h>
#include <RF24Network_config.h>
#include <Sync.h>
#include <RF24Network.h>
#include <RF24.h>
#include <SPI.h>

RF24 radio(7, 8); // 7,8 - numery wyprowadzeń płytki Arduino, do których dołączono, odpowiednio, sygnały CE i CS układu radiowego
RF24Network network(radio); //wskazanie obiektu klasy współpracującego bezpośrednio z układem radiowym

unsigned long default_light_state = 1000; //set default light state = 1000
int temp = 0; //temporary variable
int tab[4]; // light state table
int j = 0; // potrzebna w celu usunięcia wpływu drgań pinów przycisku
unsigned long button_time = 0; // time from last button state on
unsigned long start = 0; // time from start running program
const uint16_t this_node = 01; //Smart Object node
const uint16_t other_node = 00; //CoAP Server node

struct payload_t {
  uint8_t header;
  uint8_t state;
  uint32_t value;
};

void setup() {
  // put your setup code here, to run once:
  radio.begin(); //initialize RF24 obiect
  SPI.begin(); //initialize SPI interface 
  Serial.begin(57600);
  network.begin(110, this_node); // 110 - our channel
  pinMode(2, INPUT); // button
  pinMode(3, OUTPUT); // lamp
}

void loop() {
  // put your main code here, to run repeatedly:
  network.update(); // Check the network regularly
  int i = 0;
  while (Serial.available())
  {
    tab[i] = Serial.read();
    tab[i] = tab[i] - 48;
    temp += tab[i] * pow(10, (3 - i));
    default_light_state = temp;
    if (i == 3)
    {
      ++default_light_state;
      analogWrite(3, 255 - default_light_state * 255 / 1000);
      if(default_light_state>1000)
        default_light_state = 1000;
      Serial.print(F("Change light state: "));
      Serial.println(default_light_state);
    }
    i++;
  }
  temp = 0;

  if (digitalRead(2) == LOW)
  {
    //BUTTON IS ON
    j++;
    digitalWrite(2, HIGH);
    if(j>1)
    {
      start = millis();
      button_time = 0;
    }
    delay(300);
  }
  else
  {
    //BUTTON IS OFF
    digitalWrite(2, LOW);
    delay(300);
    j = 0;
  }

  // Receive Message
  if ( network.available() ) // Is there anything ready for us?
  {
    Serial.println(F("Received packet: "));
    RF24NetworkHeader header;        // If so, grab it and print it out
    payload_t payload_r;             //received payload
    payload_t payload_s;             //payload to send
    network.read(header, &payload_r, sizeof(payload_r));

    uint8_t head = payload_r.header;              //payload header
    uint8_t operation = payload_r.header >> 6;    //received number of operation 0- GET, 1- PUT
    uint8_t resource = payload_r.header & 63;     //received resource 0 - LAMP, 1- BUTTON
    uint32_t object_value = payload_r.value;      //received object value

    Serial.print(F("Operation: "));
    //GET
    if (operation == 0)
    {
      Serial.println(F("GET"));
      Serial.print(F("Header: "));
      Serial.println(payload_r.header);
      Serial.print(F("Resource: "));
      //Lamp
      if (resource == 0) 
      {
        Serial.print(resource);
        Serial.println(F(" - LAMP"));
        object_value = default_light_state;
        Serial.print(F("Light state: "));
        Serial.println(default_light_state);

      }
      //Button
      else if (resource == 1)
      {
        Serial.println(resource);
        Serial.println(F(" - BUTTON"));
        if (digitalRead(2) == LOW)
        {
          //BUTTON IS ON
          payload_s.state = 255;
          Serial.print(F("Button state: "));
          Serial.println(payload_s.state);
          digitalWrite(2, HIGH);
        }
        else
        {
          //BUTTON IS OFF
          payload_s.state = 0;
          Serial.print(F("Button state: "));
          Serial.println(payload_s.state);
          digitalWrite(2, LOW);
        }
        button_time = millis() - start;
        Serial.print(F("Start: "));
        Serial.println(start);
        Serial.print(F("Button time: "));
        Serial.println(button_time);
        object_value = button_time;
      }
      payload_s.header = head;
      payload_s.value = object_value;
      RF24NetworkHeader header(other_node);
      bool ok = network.write(header, &payload_s, sizeof(payload_s));
      if (ok)
        Serial.println(F("Sending ok"));
      else
        Serial.println(F("Sending failed"));
      Serial.println(F("//////////////////"));
    }

    //PUT
    else if ( operation == 1)
    {
      Serial.println(F("PUT"));
      Serial.print(F("Header: "));
      Serial.println(payload_r.header);
      Serial.print(F("Resource: "));

      if (resource == 0)
      {
        Serial.print(resource);
        Serial.println(F(" - BUTTON"));
        Serial.print(F("Received light state from CoAP Server: "));
        Serial.println(object_value);
        default_light_state = object_value; // set light state which received from CoAP Server
        analogWrite(3, 255 - default_light_state * 255 / 1000);
        Serial.print(F("Set light state: "));
        Serial.println(object_value);
      }
      else
      {
        Serial.println(F("Error"));
      }
    }
  }
}

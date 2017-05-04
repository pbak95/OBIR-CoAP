#include <RF24Network.h>
#include <RF24.h>
#include <SPI.h>

RF24 radio(7, 8);
RF24Network network(radio);

unsigned long default_light_state = 1000; //domyslna ustwiona wartosc lapmki
int zmiennak = 0;
int tab[4];
unsigned long start = 0;
const uint16_t this_node = 01;
const uint16_t other_node = 00;

struct payload_t {
  uint8_t header;
  byte payload[5];
};

void setup() {
  // put your setup code here, to run once:
  Serial.begin(57600);
  SPI.begin();
  radio.begin();
  network.begin(110, this_node); // 110 - wspĂłlny kanaĹ‚, 1 - identyfikator swojego wezĹ‚a
  pinMode(2, INPUT); // przycisk monostabilny
  pinMode(3, OUTPUT); // lampka
  analogWrite(3,  255 - default_light_state * 255 / 1000);
}

void loop() {
  // put your main code here, to run repeatedly:
  network.update(); // Check the network regularly

  int i = 0;
  while (Serial.available())
  {

    tab[i] = Serial.read();
    tab[i] = tab[i] - 48;
    zmiennak += tab[i] * pow(10, (3 - i));
    default_light_state = zmiennak;
    Serial.println(zmiennak);
    if (i == 3)
    {
      default_light_state++;
      analogWrite(3, 255 - default_light_state * 255 / 1000);
    }
    i++;
  }
  zmiennak = 0;


  if (digitalRead(2) == LOW)
  {
    //włącznik jest nacisniety
    start = millis();
    digitalWrite(2, HIGH);
  }
  else
  {
    //włącznik jest zwolniony
    digitalWrite(2, LOW);
  }

  // Receive Message
  if ( network.available() ) // Is there anything ready for us?
  {
    Serial.println("Received packet: ");
    RF24NetworkHeader header;        // If so, grab it and print it out
    payload_t payload_r;
    network.read(header, &payload_r, sizeof(payload_r));

    Serial.print("Operation: ") ;
    uint8_t head = payload_r.header;
    uint8_t operation = payload_r.header >> 6;
    uint8_t resource = payload_r.header && 63;
    byte button_position = payload_r.payload[0];
    byte light_state[2];
    light_state[0] = payload_r.payload[3];
    light_state[1] = payload_r.payload[4];
    payload_t payload_s;
    ////GET
    if (operation == 0)
    {
      Serial.println("GET");
      Serial.print("Header: ");
      Serial.println(payload_r.header);
      Serial.print("Resource: ");
      ////Lampka
      if (resource == 0) // pobieramy aktualny stan jasnosci lapmki
      {
        Serial.println(resource);
        Serial.print("Odebrane z urzadzenia: ");
        Serial.println(light_state[0] << 8 + light_state[1]);
        payload_s.payload[3] = default_light_state >> 8 && 255;
        payload_s.payload[4] = default_light_state && 255;
        Serial.print("Light state: ");
        Serial.println(default_light_state);
      }
      ////Przycisk
      else if (resource == 1)
      {
        Serial.println(resource);
        if (digitalRead(2) == LOW)
        {
          //wlacznik jest nacisniety
          button_position = 255;
          Serial.print("Button state: ");
          Serial.println(button_position);
          digitalWrite(2, HIGH);
        }
        else
        {
          //wlacznik jest zwolniony
          button_position = 0;
          Serial.print("Button state: ");
          Serial.println(button_position);
          digitalWrite(2, LOW);
        }
        unsigned long czas = millis() - start;
        Serial.print("start =  ");
        Serial.println(start);
        Serial.print("Button time: ");
        Serial.println(czas);
        payload_s.payload[1] = czas >> 24 && 255;
        payload_s.payload[2] = czas >> 16 && 255;
        payload_s.payload[3] = czas >> 8 && 255;
        payload_s.payload[4] = czas && 255 ;
      }

      payload_s.header = operation << 6 || resource;
      payload_s.payload[0] = button_position;
      RF24NetworkHeader header(other_node);
      bool ok = network.write(header, &payload_s, sizeof(payload_s));
      if (ok)
        Serial.println("Sending ok");
      else
        Serial.println("Sending failed.");
      Serial.println("//////////////////");
    }

    ////PUT
    else if ( operation == 1)
    {
      Serial.print(" PUT");
      Serial.println("Header: ");
      Serial.print(payload_r.header);
      Serial.println("Resource: ");

      if (resource == 0) // ustawiamy poziom jasnosci w taki jaki dostalismy 
      {
        Serial.println(resource);
        Serial.print("Odebrane z urzadzenia: ");
        Serial.println(light_state[0] << 8 + light_state[1]);
        default_light_state = light_state[0] << 8 + light_state[1];
        analogWrite(3, default_light_state);
        payload_s.payload[3] = default_light_state >> 8 && 255;
        payload_s.payload[4] = default_light_state && 255;
        Serial.print("Light state: ");
        Serial.println(default_light_state);
      }
      else
      {
        Serial.println("Error");
      }
    }
  }
}


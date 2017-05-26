#include <nRF24L01.h>
#include <RF24_config.h>
#include <RF24Network_config.h>
#include <Sync.h>
#include <RF24Network.h>
#include <RF24.h>
#include <SPI.h>

RF24 radio(7, 8);
RF24Network network(radio);

unsigned long default_light_state = 1000; //domyslna ustwiona wartosc lapmki
int zmiennak = 0;
int tab[4];
int j = 0;
unsigned long czas = 0;
unsigned long start = 0;
const uint16_t this_node = 01;
const uint16_t other_node = 00;

struct payload_t {
  uint8_t header;
  uint8_t state;
  uint32_t value;
};

void setup() {
  // put your setup code here, to run once:
  radio.begin();
  Serial.begin(57600);
  SPI.begin();
  network.begin(110, this_node); // 110 - wspĂłlny kanaĹ‚, 1 - identyfikator swojego wezĹ‚a
  pinMode(2, INPUT); // przycisk monostabilny
  pinMode(3, OUTPUT); // lampka
  //Serial.println("jest ok ");
  //Serial.println(255 - default_light_state * 255 / 1000);
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
    //Serial.println(zmiennak);
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
    j++;
    digitalWrite(2, HIGH);
    if(j>1)
    {
    start = millis();
    czas = 0;
    }
    delay(300);
  }
  else
  {
    //włącznik jest zwolniony
    digitalWrite(2, LOW);
    delay(300);
    j = 0;
  }
//  czas = millis()-start;
//  Serial.print("Czas: ");
//  Serial.println(czas);

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
    uint8_t resource = payload_r.header & 63;
    uint32_t light_state = payload_r.value;
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
        light_state = default_light_state;
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
          payload_s.state = 255;
          Serial.print("Button state: ");
          Serial.println(payload_s.state);
          digitalWrite(2, HIGH);
        }
        else
        {
          //wlacznik jest zwolniony
          payload_s.state = 0;
          Serial.print("Button state: ");
          Serial.println(payload_s.state);
          digitalWrite(2, LOW);
        }
        czas = millis() - start;
        Serial.print("start =  ");
        Serial.println(start);
        Serial.print("Button time: ");
        Serial.println(czas);
        light_state = czas;
      }
      payload_s.header = head;
      payload_s.value = light_state;
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
      Serial.println("PUT");
      Serial.print("Header: ");
      Serial.println(payload_r.header);
      Serial.print("Resource: ");

      if (resource == 0) // ustawiamy poziom jasnosci w taki jaki dostalismy
      {
        Serial.println(resource);
        Serial.print("Odebrane z urzadzenia: ");
        Serial.println(light_state);
        default_light_state = light_state;
        analogWrite(3, 255 - default_light_state * 255 / 1000);
        Serial.print("Light state: ");
        Serial.println(light_state);
      }
      else
      {
        Serial.println("Error");
      }
    }
  }
}

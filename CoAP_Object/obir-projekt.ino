#include <RF24Network.h>
#include <RF24.h>
#include <SPI.h>

unsigned char level=0;
int zmienna = 0;
RF24 radio(7,8);        
RF24Network network(radio);   

unsigned long packets_sent=0;
unsigned long last_sent;
const unsigned long interval =2000;
unsigned short adc;

struct payload_t
 {
    unsigned long ms;
    unsigned long counter;
    unsigned short adc;
 };


void setup() {
  // put your setup code here, to run once:
  Serial.begin(57600);
  SPI.begin();
  radio.begin();
  network.begin(110,1); // 110 - wspólny kanał, 1 - identyfikator swojego wezła
  pinMode(3, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(2, OUTPUT);

  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
     pinMode(2, OUTPUT);


}

void loop() {
  // put your main code here, to run repeatedly:
    
    if(Serial.available())
    {
      zmienna = Serial.read();
      //zmienna = input.toInt();
      Serial.println("Wpisano: " );
      //Serial.println(zmienna);
      Serial.println((zmienna - 48)*255/9, DEC);
      analogWrite(2, 255- (zmienna - 48)*255/9);
      delay(1000);
    }

   
   if (digitalRead(3) == LOW)
    {
    //włącznik jest naciśniety
    digitalWrite(3,HIGH);
    digitalWrite(LED_BUILTIN,HIGH);
    digitalWrite(2,LOW);
    //Serial.println("HIGH");  
    }   
    else
    {
    digitalWrite(3,LOW);
    digitalWrite(LED_BUILTIN,LOW);
    digitalWrite(2,HIGH);
    //Serial.println("LOW");
    //włącznik jest zwolniony
    }

}

void receiveMessage()
{
  network.update();                  // Check the network regularly
  
    while ( network.available() )  // Is there anything ready for us?
    {     
      RF24NetworkHeader header;        // If so, grab it and print it out
      payload_t payload;
      network.read(header,&payload,sizeof(payload));
      Serial.println("Received packet #");
      Serial.print(payload.counter);
      Serial.print(" at ");
      Serial.print(payload.ms);
      Serial.println(" TEST BYTE: ");
      Serial.print(payload.adc, DEC);
      Serial.println("/////////");
    }
}
void sendMessage()
{
  network.update();
  unsigned long now = millis();
  if( now - last_sent >= interval)
  {
    last_sent = now;
  
  Serial.println("Sending...");
  adc = analogRead(A0);
  Serial.println(adc,DEC);
  payload_t payload = {millis(),packets_sent++,adc};
  RF24NetworkHeader header(0); // 0 - identyfikator adresata
  bool ok = network.write(header,&payload,sizeof(payload));
  if (ok)
    Serial.println("ok.");
  else
    Serial.println("failed.");

  }
}


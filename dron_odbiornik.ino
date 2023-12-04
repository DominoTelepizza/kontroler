#include <RF24.h>
#include <nRF24L01.h>
#include <Servo.h>
#include <SPI.h>

const uint64_t pipeIn = 0xE8E8F0F0E1LL; //kod po ktorym laczy sie odbiornik z nadajnikiem

RF24 radio(9,8);

struct ReceivedData {
  byte ch1;
  byte ch2;
  byte ch3;
  byte ch4;
  byte ch5;
  byte ch6;
  byte ch7;
  byte ch8;
  byte ch9;
};

ReceivedData receiveData;

Servo channel1_1;
Servo channel1_2;
Servo channel1_3;
Servo channel1_4;
Servo channel1_5;
Servo channel1_6;
Servo channel1_7;
Servo channel1_8;
Servo channel1_9;

int ch1_value = 0;
int ch2_value = 0;
int ch3_value = 0;
int ch4_value = 0;
int ch5_value = 0;
int ch6_value = 0;
int ch7_value = 0;
int ch8_value = 0;
int ch9_value = 0;

unsigned long lastTime = 0;


void setup() {
  // put your setup code here, to run once:
  channel1_1.attach(3);

  receiveData.ch1 = 127;
  receiveData.ch2 = 127;
  receiveData.ch3 = 127;
  receiveData.ch4 = 127;
  receiveData.ch5 = 0;
  receiveData.ch6 = 0;
  receiveData.ch7 = 0;
  receiveData.ch8 = 0;
  receiveData.ch9 = 0;


  radio.begin();
  radio.setAutoAck(false);
  radio.setDataRate(RF24_250KBPS);
  radio.openReadingPipe(1,pipeIn);

  //ustawiamy urzadzenie jako odbiornik
  radio.startListening();
}

void loop() {
  // put your main code here, to run repeatedly:
  receiveTheData();

  ch1_value = map(receiveData.ch1, 0, 255, 1000, 2000);
  ch2_value = map(receiveData.ch1, 0, 255, 1000, 2000);
  ch3_value = map(receiveData.ch1, 0, 255, 1000, 2000);
  ch4_value = map(receiveData.ch1, 0, 255, 1000, 2000);
  ch5_value = map(receiveData.ch1, 0, 1, 1000, 2000);
  ch6_value = map(receiveData.ch1, 0, 1, 1000, 2000);
  ch7_value = map(receiveData.ch1, 0, 1, 1000, 2000);
  ch8_value = map(receiveData.ch1, 0, 1, 1000, 2000);
  ch9_value = map(receiveData.ch1, 0, 1, 1000, 2000);

  channel1_1.write(ch1_value);
  channel1_3.write(ch3_value);
  
}

void receiveTheData(){
  while(radio.available()){
    radio.read(&receiveData, sizeof(ReceivedData));
    lastTime = millis(); //lastTime chwyta dane
  }
}

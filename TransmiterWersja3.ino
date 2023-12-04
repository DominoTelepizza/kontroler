/*Szkic używa 15926 bajtów (49%) pamięci programu. Maksimum to 32256 bajtów.
Zmienne globalne używają 860 bajtów (41%) */

#include <RF24.h>
#include <RF24_config.h>
#include <nRF24L01.h>
#include <printf.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_SH110X.h>

#define i2c_Address 0x3c //initialize with the I2C addr 0x3C for typically OLED
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1   //   QT-PY / XIAO
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
#define NUMFLAKES 10
#define XPOS 0
#define YPOS 1
#define DELTAY 2

//zmienne dla podpietych pinów buttonów
const int buttonUp = 2; //przycisk Up podpiety pod pin nr 2
const int buttonDown = 3; //przycisk Down podpiety pod pin nr 3
const int buttonSelect = 4; //przycisk Select podpiety pod pin nr 4

//zmienne do obslugi menu
int menuIndex = 0; //indeks wybranej pozycji w menu
int menuIndexMax = 1;
bool enginesEnabled = false; // stan silników
bool autoLandingEnabled = false; // stan automatycznego lądowania

/*
millis() można nazwać stoperem który nalicza czas od startu arduino
debounceDelay określa jakie ma być opóźnienie (w milisekundach)
lastDebounceTime w funkcjach zapamiętuje czas ostatniego millis()
*/
//zmienne dla funkcji millis()
unsigned long debounceDelay = 400; // Opóźnienie dla odbicia (debounce), w milisekundach
unsigned long lastDebounceTime = 0; // zapamietuje ostatni aktualny czas
unsigned long aktualnyCzas = 0; //do aktualnyCzas zostanie przypisana wartość millis() w loop
unsigned long zapamietanyCzas1 = 0;

String alert = "Brak komunikatow";

//enum - typ wyliczeniowy zawiera już listę wartości, jaką można nadać zmiennej własnego typu enum
/*
MAIN - menu glowne 
SETTINGS - ustawienia 
DIAG - diagnostyka
INFO - informacje o projekcie
DIAG_C - diagnostyka kontrolera
DIAG_C2 - diagnostyka kontrolera strona 2
DIAG_D - diagnostyka drona
*/
enum Screen { MAIN, SETTINGS, DIAG};
Screen currentScreen = MAIN;

const uint64_t myRadioPipe = 0xE8E8F0F0E1LL; //kod po ktorym laczy sie odbiornik z nadajnikiem
RF24 radio(9,8); //piny CE oraz CSN z nrf24
struct DataToBeSent{
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
//tworzymy zmienna struct ktora bedzie zawierac informacje
DataToBeSent sentData;

void setup()   {
  Serial.begin(9600);

  delay(250); // wait for the OLED to power up
  display.begin(i2c_Address, true); // Address 0x3C default
  display.display();
  delay(2000);
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 0);
  // Clear the buffer.
  display.clearDisplay();

  pinMode(buttonUp, INPUT);
  pinMode(buttonDown, INPUT);
  pinMode(buttonSelect, INPUT);

  radio.begin();
  radio.setAutoAck(false);
  radio.setDataRate(RF24_250KBPS);
  radio.openWritingPipe(myRadioPipe);

  //reset wartosci kazdego kanalu
  sentData.ch1 = 127; //srodkowa wartosc joy1X mozliwe wartosci 0-255
  sentData.ch2 = 127; //srodkowa wartosc joy1Y
  sentData.ch3 = 127; //srodkowa wartosc joy2X
  sentData.ch4 = 127; //srodkowa wartosc joy2Y
  sentData.ch5 = 0;   //przelacznik1, mozliwe wartosci 0 lub 1
  sentData.ch6 = 0;   //przelacznik2, mozliwe wartosci 0 lub 1
  sentData.ch7 = false;   //stan silnikow
  sentData.ch8 = false;   //auto lądowanie
  sentData.ch9 = false;   //cos dodatkowego
}


void loop() {

  aktualnyCzas = millis();

  //Obsługa przycisków
  if (digitalRead(buttonUp) == HIGH) {
    if (millis() - lastDebounceTime > debounceDelay) {
      menuIndex--; //przejdz nizej
      lastDebounceTime = millis();
    }
  }
  if (digitalRead(buttonDown) == HIGH) {
    if (millis() - lastDebounceTime > debounceDelay) {
      menuIndex++; //przejdz wyzej
      lastDebounceTime = millis();
    }
  }
  if (digitalRead(buttonSelect) == HIGH) {
    if (millis() - lastDebounceTime > debounceDelay) {
      executeAction(); //uruchom akcje
      lastDebounceTime = millis();
    }
  }
  
  //menuIndex = max(0, min(menuIndex, 2)); // Zapewnia, że indeks jest w zakresie 0-2
  //inna forma, powracająca:
  if(menuIndex < 0){
    menuIndex = menuIndexMax;
  }
  else if(menuIndex > menuIndexMax){
    menuIndex = 0;
  }

 //przyklad opoznienia z millis()
  if (aktualnyCzas - zapamietanyCzas1 >= debounceDelay) {
    zapamietanyCzas1 = aktualnyCzas;
  //wyswietlanie ekranu
  displayMenu();
  }
  //wyslanie danych do odbiornika
  executeSentData();

  //Jeśli różnica wynosi ponad 100milisekund (1000UL to 1 sekunda)
  //if (aktualnyCzas - zapamietanyCzas1 >= 1000UL) {}
  //Zapamietaj aktualny czas
  //zapamietanyCzas1 = aktualnyCzas;
  //}
}


//funkcja wysłania paczki danych do odbiornika
void executeSentData(){
  sentData.ch1 = map(analogRead(A0), 0, 1024, 0, 255); //zapis wartosci A0 do ch1
  sentData.ch2 = map(analogRead(A1), 0, 1024, 0, 255); //zapis wartosci A1 do ch2
  sentData.ch3 = map(analogRead(A2), 0, 1024, 0, 255); //zapis wartosci A2 do ch3
  sentData.ch4 = map(analogRead(A3), 0, 1024, 0, 255); //zapis wartosci A3 do ch4
  sentData.ch5 = digitalRead(2);                  //zapis wartosci przelacznik1 podpietego pod wejscie 2 w arduino
  sentData.ch6 = digitalRead(3);                  //zapis wartosci przelacznik2 podpietego pod wejscie 3 w arduino
  sentData.ch7 = enginesEnabled; //false lub true
  sentData.ch8 = autoLandingEnabled; //false lub true
  sentData.ch9 = autoLandingEnabled; //false lub true

  radio.write(&sentData, sizeof(DataToBeSent)); //wyslij ramke do odbiornika
}

void displayMenu() {
  display.clearDisplay();
  display.setCursor(0, 0);

  switch(currentScreen) {
    case MAIN:
      //       Tytuł,          Nazwa Opcja 1, Nazwa Opcja 2,  Nazwa Opcja 3,  menuIndex
      drawMenu("SuperDron2000", "Ustawienia", "Diagnostyka", "", menuIndex);
      break;
    case SETTINGS:
      drawMenu("Ustawienia", "Wlacz silniki", "Auto ladowanie", "Powrot", menuIndex);
      break;
    case DIAG:
      drawDiag("Diagnostyka", "Nastepna strona", "Powrot", menuIndex);
      break;
  }
  display.display();
}

//rysowanie menu
void drawMenu(const char* title, const char* line1, const char* line2, const char* line3, int index) {
  display.setCursor(0,0);
  display.println(title);
  display.println();

  // Sprawdzenie czy pierwszy element menu jest wybrany
  if (index == 0) { display.print("> "); } // Jeśli tak, dodaj symbol ">"
  display.println(line1); // Wyświetl nazwę pierwszej pozycji menu

  if (index == 1) { display.print("> "); }
  display.println(line2);

  if (index == 2) { display.print("> "); }
  display.println(line3);
  display.setCursor(0,55);
  display.print(alert);
}

//rysowanie menu diagnostycznego
void drawDiag(const char* title, const char* line1, const char* line2, int index){
  display.setCursor(0,0);
  display.println(title);
  display.println();
  
  display.print("joy1X: "); display.println(analogRead(A0));
  display.print("joy1Y: "); display.println(analogRead(A1));
  display.print("joy2X: "); display.println(analogRead(A2));
  display.print("joy2Y: "); display.println(analogRead(A3));
  
  if (index == 0) { display.print("> "); } // Jeśli tak, dodaj symbol ">"
  display.println(line1); // Wyświetl nazwę pierwszej pozycji menu

  if (index == 1) { display.print("> "); }
  display.println(line2);
}

/*
executeAction() Uruchamia akcję po wcisnieciu guzika Select
wyjasnienie działania:

case MAIN określa że currentScreen jest wlasnie w main_menu
if sprawdza w której pozycji na ekranie jesteśmy:
menuIndex = 1 to Ustawienia 
menuIndex = 2 to Diagnostyka 
menuIndex = 3 to Info

czyli jeśli aktualnie mamy wybrane na ekranie Diagnostyka,
to zmienimy wartość enuma currentScreen z MAIN na DIAG.
Po czym ustawimy menuIndex na 0, aby po wejsciu w diagnostyke domyslnie
była wybrana pierwsza opcja.
*/
void executeAction() {
  // Tu dodaj obsługę wyboru opcji w menu
  switch(currentScreen) {
    case MAIN:
      if (menuIndex == 0) {
        currentScreen = SETTINGS;
        menuIndexMax = 2;
        menuIndex = 0;
      } else if (menuIndex == 1) {
        currentScreen = DIAG;
        menuIndex = 0;
      }
      break;
    case SETTINGS:
      if (menuIndex == 0) {
        enginesEnabled = !enginesEnabled; // Zmiana stanu silników
        alert = enginesEnabled ? "Silniki wl" : "Silniki wyl";
      } else if (menuIndex == 1) {
        autoLandingEnabled = !autoLandingEnabled; // Zmiana stanu auto lądowania
        alert = enginesEnabled ? "Autol wl" : "Autol wyl";
      } else if (menuIndex == 2) {
        currentScreen = MAIN;
        menuIndexMax = 1;
        menuIndex = 0;
      }
      break;
    case DIAG:
      if (menuIndex == 0) {
        currentScreen = MAIN;
      } else if (menuIndex == 1) {
        currentScreen = MAIN;
      }
      break;
    /*
    w diagnostyce kontrolera, mamy opcje 'Nastepna strona'
    program ma dzialac tak, ze gdy klikniemy tą opcję, to
    przeniesie nas do ekranu DIAG_C2, a gdy
    tam klikniemy Nastepna strona, to przejdzie znowu do 
    DIAG_CONTROLLER
    */
  }
}

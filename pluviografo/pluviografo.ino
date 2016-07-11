#include <LiquidCrystal.h>
#include <LCDKeypad.h>
#include <SD.h>
#include <Wire.h>
#include <RtcDS3231.h>
#include <EEPROM.h>
#include "EEPROMAnything.h"


#define Reset 0
#define CambiarHora 1
#define CambiarFecha 2
#define Salir 2
unsigned int setting = 0;

struct config_t
{   byte deflt; 
    float medicion;
    int minutos_logueo;
} configuration;


RtcDS3231 Rtc;
RtcDateTime now;
float medicion=0;
String  unidades= "mm";

LCDKeypad lcd;

//LiquidCrystal lcd(8,9,4,5,6,7); 
const int chipSelect = 53;
//variables para el periodo de grabacion en la SD
long intervalo;
unsigned long previousMillis = 0;

//variables para el encoder
const int PinCLK=18;                   // Used for generating interrupts using CLK signal
const int PinDT=19;                    // Used for reading DT signal
volatile float     virtualPosition  =0;  // must be volatile to work with the isr

const float incremento=0.2;
void isr0 ()  {
  detachInterrupt(digitalPinToInterrupt(PinCLK));
  static unsigned long                lastInterruptTime = 0;
  unsigned long                       interruptTime = millis();
  // If interrupts come faster than 5ms, assume it's a bounce and ignore
  if (interruptTime - lastInterruptTime > 5) {
           Serial.println("interrupcion A valida");
          virtualPosition=virtualPosition+incremento; 
      }
    //Serial.println("interrupcion A");
//  Serial.print(valCLK);
//  Serial.print(valDT);
//  Serial.println();
//  Serial.print(virtualPosition);
//  Serial.println();  
  lastInterruptTime = interruptTime;
  attachInterrupt (digitalPinToInterrupt(PinCLK),isr0,RISING);
  
} // ISR0

void isr1 ()  {
  detachInterrupt(digitalPinToInterrupt(PinDT));
  static unsigned long                lastInterruptTime = 0;
  unsigned long                       interruptTime = millis();
  // If interrupts come faster than 5ms, assume it's a bounce and ignore
  if (interruptTime - lastInterruptTime > 5) {
          Serial.println("interrupcion B valida");
          virtualPosition=virtualPosition+incremento; 
      }
 // Serial.println("interrupcion B");
//  Serial.print(valCLK);
//  Serial.print(valDT);
//  Serial.println();
//  Serial.print(virtualPosition);
//  Serial.println();  
  lastInterruptTime = interruptTime;
  attachInterrupt (digitalPinToInterrupt(PinDT),isr1,RISING);
  
} // ISR1


void setup() {
   
   Serial.begin(9600);
   
   
   Wire.begin();
   int count=EEPROM_readAnything(0, configuration);
   Serial.println(count);
   Serial.println(configuration.deflt);
   Serial.println(configuration.medicion);
   Serial.println(configuration.minutos_logueo);
   
	  if (configuration.deflt==255) {
		// es la primera vez que se inicia el sistema
		  configuration.deflt=0;
		  configuration.medicion=0;
		  configuration.minutos_logueo=10;
		  EEPROM_writeAnything(0, configuration);
	  }
   else {
    
    medicion=configuration.medicion;
    virtualPosition=medicion;
    Serial.print("medicion en setup:");
    Serial.println(medicion);
    }
    //intervalo de grabacion en la micro sd en milisegundos
    intervalo = configuration.minutos_logueo*60*1000;

    seteoReloj();
    initSDcard();
  
	
    // Inicializamos la lectura del encoder
	attachInterrupt (digitalPinToInterrupt(PinCLK),isr0,RISING);
  attachInterrupt (digitalPinToInterrupt(PinDT),isr1,RISING);
    pinMode(PinCLK, INPUT);
    pinMode(PinDT, INPUT);

 

//pantalla
    lcd.begin(16, 2);
    lcd.clear();


}

void loop() {
	
	
     String datestring;
  // muestro la hora y temperatura
    
    
     // if they push the "Save" button, save their configuration
    if (digitalRead(13) == HIGH)
        EEPROM_writeAnything(0, configuration);
    
    
    leerHoraYTemp();
    leerMedicion();
    mostrarDatos(true);  // Escribimos el valor en pantalla
    // Listen for buttons 
    buttonListen();
    //delay(10000); // ten seconds

}

void resetValues(){
medicion=0;  
}


#define countof(a) (sizeof(a) / sizeof(a[0]))

void leerHoraYTemp(){
   if (!Rtc.IsDateTimeValid()) 
    {
        // Common Cuases:
        //    1) the battery on the device is low or even missing and the power line was disconnected
        Serial.println("RTC lost confidence in the DateTime!");
    }

    now = Rtc.GetDateTime();   
 }


void leerMedicion(){
	
//Ya no hace falta, se hace en mostrar datos, las mediciones se leen en el ISR0
 
}
  

void mostrarDatos(boolean serial)
{
	
  //muestro fecha y hora
  String datestring=formatDateTime(now);
  //if (serial)
	   // Serial.print(datestring);
	    
  //lcd.clear();
  String s = String(medicion);
  lcd.setCursor(0,0);
  lcd.print(datestring);
  lcd.setCursor(5,1);
  lcd.print(s + " " + unidades);
  
  //muestro la medicion     
   if (virtualPosition != medicion) {
        medicion = virtualPosition;
		Serial.print("Count:");
        Serial.println(medicion);

    // s = String(medicion);  // Convertimos el número en texto
    //s.toCharArray(texto, 10);  // Convertimos el texto en un formato compatible
    // if (serial)
    //   Serial.println(s + " " + unidades);
    //lcd.setCursor(5,1);
    //lcd.print(s + " " + unidades);
  
		//cambio el valor de medicion, entonces escribo en la memoria
		File dataFile = SD.open("datalog.txt", FILE_WRITE);
    
		// if the file is available, write to it:
		if (dataFile) {
      lcd.setCursor(14,1);
      lcd.print("  ");
			dataFile.println(datestring + "," + s + " " + unidades);
			dataFile.close();
        }  
      // if the file isn't open, pop up an error:
		else {
			Serial.println("error opening datalog.txt trying to init sd card");
      initSDcard();
      }

		//escribimos en la eeprom para no perder el valor si el dispositivo se apaga
		configuration.medicion=medicion;
		int c=EEPROM_writeAnything(0, configuration);
   	Serial.print("escribi medicion "); 
    Serial.print(configuration.medicion) ;
    Serial.print(" cantidad "); 
    Serial.print(c); 
    Serial.println();
  }	
  
    
  
}




String formatDateTime(const RtcDateTime& dt)
{
    char datestring[20];

    snprintf_P(datestring, 
            countof(datestring),
            PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
            dt.Day(),
            dt.Month(),
            dt.Year(),
            dt.Hour(),
            dt.Minute(),
            dt.Second() );
   
    return datestring;
}




void seteoReloj() {
    Serial.print("compiled: ");
    Serial.print(__DATE__);
    Serial.println(__TIME__);

    //--------RTC SETUP ------------
    Rtc.Begin();

    // if you are using ESP-01 then uncomment the line below to reset the pins to
    // the available pins for SDA, SCL
    // Wire.begin(0, 2); // due to limited pins, use pin 0 and 2 for SDA, SCL

    RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
    String datestring=formatDateTime(compiled);
    Serial.print(datestring);
    Serial.println();

    if (!Rtc.IsDateTimeValid()) 
    {
        // Common Cuases:
        //    1) first time you ran and the device wasn't running yet
        //    2) the battery on the device is low or even missing

        Serial.println("RTC lost confidence in the DateTime!");

        // following line sets the RTC to the date & time this sketch was compiled
        // it will also reset the valid flag internally unless the Rtc device is
        // having an issue

        Rtc.SetDateTime(compiled);
    }

    if (!Rtc.GetIsRunning())
    {
        Serial.println("RTC was not actively running, starting now");
        Rtc.SetIsRunning(true);
    }

    RtcDateTime now = Rtc.GetDateTime();
    if (now < compiled) 
    {
        Serial.println("RTC is older than compile time!  (Updating DateTime)");
        Rtc.SetDateTime(compiled);
    }
    else if (now > compiled) 
    {
        Serial.println("RTC is newer than compile time. (this is expected)");
    }
    else if (now == compiled) 
    {
        Serial.println("RTC is the same as compile time! (not expected but all is fine)");
    }

    // never assume the Rtc was last configured by you, so
    // just clear them to your needed state
    Rtc.Enable32kHzPin(false);
    Rtc.SetSquareWavePin(DS3231SquareWavePin_ModeNone); 
  }

void initSDcard(){
    //SD CARD
   Serial.print("Initializing SD card...");
   // make sure that the default chip select pin is set to
   // output, even if you don't use it:
   pinMode(chipSelect, OUTPUT);
  
  // see if the card is present and can be initialized:
   if (!SD.begin(chipSelect)) {
     Serial.println("Card failed, or not present");
     // don't do anything more:
     lcd.setCursor(14,1);
     lcd.print("SD");
   } else
   Serial.println("card initialized.");
     lcd.setCursor(14,1);
     lcd.print("  ");
  }

void buttonListen() {

  if ((lcd.button())==KEYPAD_UP) {
      lcd.setCursor(0,1);
     lcd.print("UP");}
  else {
     lcd.setCursor(0,1);
     lcd.print("  ");}
}

#include <SD.h>
#include <Wire.h>
#include <LiquidCrystal.h>
#include <RtcDS3231.h>
#include <EEPROM.h>
#include "EEPROMAnything.h"

struct config_t
{   byte deflt; 
    int medicion;
    int minutos_logueo;
} configuration;


RtcDS3231 Rtc;
RtcDateTime now;
RtcTemperature temp
float medicion=0;
String  unidades= "mm";
int pin1 = 9;
int pin2 = 8;
int anterior = 1;

LiquidCrystal lcd(8,9,4,5,6,7); 

void setup() {
  // seteo reloj
   Serial.begin(9600);
   
   
   Wire.begin();
   EEPROM_readAnything(0, configuration);
   
	  if (configuration.deflt==255) {
		// es la primera vez
		  configuration.deflt=0;
		  configuration.medicion=0;
		  configuration.minutos_logueo=10;
		  EEPROM_writeAnything(0, configuration);
	  }

    seteoReloj();
   //SD CARD
   Serial.print("Initializing SD card...");
   // make sure that the default chip select pin is set to
   // output, even if you don't use it:
   pinMode(10, OUTPUT);
  
  // see if the card is present and can be initialized:
   if (!SD.begin(chipSelect)) {
     Serial.println("Card failed, or not present");
     // don't do anything more:
     
   } else
   Serial.println("card initialized.");
  
	
    // Inicializamos la lectura del encoder
    pinMode(pin1, INPUT);
    pinMode(pin2, INPUT);

 
// Inicializamos las variables para mediciones
    anterior = 1;
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
    
    delay(10000); // ten seconds

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
  // Lectura de A, si es 0, volvemos a medir para asegurar el valor
  int actual = digitalRead(pin1);
  if(actual == 0) {
    delay(10);
    actual = digitalRead(pin1);
  }
 
  // Actuamos únicamente en el flanco descendente
  if(anterior == 1 && actual == 0)
  {
    int valor_b = digitalRead(pin2);
    if(valor_b == 1) medicion=medicion+0.2;  // Si B = 1, aumentamos el contador
    else medicion=medicion-0.2;  // Si B = 0, reducimos el contador
   }
  anterior = actual;
}
  

void mostrarDatos(boolean serial)
{
	
  //muestro fecha y hora
  datestring=formatDateTime(now);
  if (serial)
	    Serial.print(datestring);
	    
  lcd.clear();
  lcd.setCursor(1,0);
  lcd.print(datestring);
  //muestro la medicion     
  
  String s = String(medicion);  // Convertimos el número en texto
  //s.toCharArray(texto, 10);  // Convertimos el texto en un formato compatible
  if (serial)
    Serial.println(s + " " + unidades);
  lcd.setCursor(2,0);
  lcd.print(s + " " + unidades);
  
  //escribo en la SD, aca hay que controlar que sea en el periodo fijado por el usuario
  File dataFile = SD.open("datalog.txt", FILE_WRITE);

  // if the file is available, write to it:
  if (dataFile) {
    dataFile.println(datestring + "," + s + " " + unidades);
    dataFile.close();
    
  }  
  // if the file isn't open, pop up an error:
  else {
    Serial.println("error opening datalog.txt");
  } 
  
  
}




String formatDateTime(const RtcDateTime& dt)
{
    char datestring[20];

    snprintf_P(datestring, 
            countof(datestring),
            PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
            dt.Month(),
            dt.Day(),
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
    printDateTime(compiled);
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


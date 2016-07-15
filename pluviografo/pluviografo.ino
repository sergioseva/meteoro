#include <LiquidCrystal.h>
#include <LCDKeypad.h>
#include <SD.h>
#include <Wire.h>
#include <RtcDS3231.h>
#include "EEPROMAnything.h"
#include <math.h>




struct config_t
{   byte deflt; 
    float medicion;
	int minutos_logueo;
	float acumulado_mes;
	float acumulado_dia;
	int mes_actual;
	int dia_actual;
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
const int encoderPinA=18;                   // Used for generating interrupts using CLK signal
const int encoderPinB=19;                    // Used for reading DT signal
volatile float     virtualPosition  =0;  // must be volatile to work with the isr

const float incremento=0.2;
// interrupt service routine vars
boolean A_set = false;              
boolean B_set = false;
static boolean rotating=false;  

// Interrupt on A changing state
void doEncoderA(){
  // debounce
  if ( rotating ) delay (1);  // wait a little until the bouncing is done

  // Test transition, did things really RISING? 
  if( digitalRead(encoderPinA) &&  !A_set ) {  // debounce once more
    A_set = 1;
    Serial.println("A");
    Serial.println(A_set);
    // adjust counter + if A leads B
    B_set=digitalRead(encoderPinB);
    if ( A_set && !B_set ) 
      virtualPosition += incremento;

    rotating = false;  // no more debouncing until loop() hits again
  }
}

// Interrupt on B changing state, same as A above
void doEncoderB(){
  if ( rotating ) delay (1);
  if( digitalRead(encoderPinB) &&  !B_set ) {
    B_set = 1;
    Serial.println("B");
    Serial.println(B_set);
    //  adjust counter - 1 if B leads A
     A_set=digitalRead(encoderPinA);
    if( B_set && !A_set ) 
      virtualPosition += incremento;

    rotating = false;
  }
}


void setup() {
   
	Serial.begin(9600);
	Wire.begin();
   
	pinMode(encoderPinA, INPUT); 
	pinMode(encoderPinB, INPUT); 
	// turn on pullup resistors
	digitalWrite(encoderPinA, LOW);
	digitalWrite(encoderPinB, LOW);
	// encoder pin on interrupt 0 (pin 2)
	attachInterrupt(digitalPinToInterrupt(encoderPinA), doEncoderA, RISING);
	// encoder pin on interrupt 1 (pin 3)
	attachInterrupt(digitalPinToInterrupt(encoderPinB), doEncoderB, RISING);
  
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
	//pantalla
    lcd.begin(16, 2);
    lcd.clear();
    
//resetea todo, comentar
resetAcumuladoTodo();
medicion=0;
virtualPosition=0;

}

void loop() {
	
    rotating = true;  // reset the debouncer
    
    leerHoraYTemp();
    chequearAcumulados();
    mostrarDatos(true);  // Escribimos el valor en pantalla
    

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


void chequearAcumulados(){
	//Chequea que no haya cambiado el dia y el mes, si cambió, vuelvo parametros a 0
	if (configuration.mes_actual!=now.Month()){
		configuration.acumulado_mes=0;
		configuration.mes_actual=now.Month();
	}
	
	if (configuration.dia_actual!=now.Day()){
		configuration.acumulado_dia=0;
		configuration.dia_actual=now.Day();
	}
	EEPROM_writeAnything(0, configuration);
 
}

void resetAcumuladoDiario(){
configuration.acumulado_mes=0;  
EEPROM_writeAnything(0, configuration);
}

void resetAcumuladoMensual(){
configuration.acumulado_dia=0;  
EEPROM_writeAnything(0, configuration);
}

void resetAcumuladoTodo(){
resetAcumuladoDiario();
resetAcumuladoMensual();
}
  

void mostrarDatos(boolean serial)
{
	
  //muestro fecha y hora
  String datestring=formatDateTime(now);
  //if (serial)
	   // Serial.print(datestring);
	    
  //lcd.clear();
  String s;
  
  //muestro la medicion     
   if (virtualPosition != medicion) {
		float diff=virtualPosition-medicion;
     medicion = virtualPosition;
		configuration.acumulado_mes +=diff;
		configuration.acumulado_dia +=diff;
		Serial.print("Medicion:");
        Serial.println(medicion);
    Serial.print("Mes:");
        Serial.println(configuration.acumulado_mes);
    Serial.print("Dia:");
        Serial.println(configuration.acumulado_dia);
        //Grabo en la SD solo si es un nro entero
		int med_int=(int) (medicion+0.001);
    Serial.print("Medicion entera:");
        Serial.println(med_int);
     //tuve que hacer la comparacion convirtiendo a string porque no daba igual aunque sea 2.00=2
		if (String(medicion)==String(med_int)+".00") {
			//cambio el valor de medicion, entonces escribo en la memoria
			File dataFile = SD.open("datalog.txt", FILE_WRITE);
		  Serial.print("Escribo en SD");  
			// if the file is available, write to it:
			if (dataFile) {
				  lcd.setCursor(14,1);
				  lcd.print("  ");
				  s = String(medicion);
				  dataFile.println(datestring + "," + s + " " + unidades);
				  dataFile.close();
			}  
		  // if the file isn't open, pop up an error:
			else {
				Serial.println("error opening datalog.txt trying to init sd card");
				initSDcard();
			}
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
  
  lcd.setCursor(0,0);
  lcd.print(datestring);
  lcd.setCursor(0,1);
  int ad=(int) configuration.acumulado_dia+0.1;
  int am=(int) configuration.acumulado_mes+0.1;
  s = String(ad);
  lcd.print("D:"+ s + " " + unidades);
  s = String(am);
  lcd.setCursor(8,1);
  lcd.print("M:"+ s + " " + unidades);

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


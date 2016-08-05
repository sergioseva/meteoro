#include <MenuBackend.h>    //MenuBackend library - copyright by Alexander Brevig
#include <LiquidCrystal.h>
#include <LCDKeypad.h>
#include <SDM.h>
#include <Wire.h>
#include <RtcDS3231.h>
#include "EEPROMAnything.h"
#include <math.h>

#define HOURS 0
#define MINUTES 1
#define SECONDS 2
#define YEAR 2
#define MONTH 1
#define DAY 0
#define SALIR 3
#define LCD_Backlight 10 // 15. BL1 - Backlight + 
#define PANTALLA_ACTIVA 0
#define PANTALLA_STBY 1
const float minutos_stby=10;
int brillo=0;
// The time model
unsigned int year = 0;
unsigned int month = 0;
unsigned int day= 0;
unsigned int hours = 0;
unsigned int minutes = 0;
unsigned int seconds = 0;
unsigned int settingTime = 0;
unsigned int settingDate = 0;
boolean exitmenu=false;
unsigned int estado_pantalla = PANTALLA_ACTIVA;

byte enie[8] = {
    B01001,
    B10110,
    B00000,
    B10001,
    B11001,
    B10101,
    B10111,
    B00000
};

byte up[8] = {
    B00000,
    B00100,
    B01110,
    B11111,
    B00100,
    B00100,
    B00100,
    B00000
};
byte down[8] = {
    B00000,
    B00100,
    B00100,
    B00100,
    B11111,
    B01110,
    B00100,
    B00000
};

struct config_t
{   byte deflt; 
    float medicion;
    int minutos_logueo;
    bool memoria;
	byte contrast_stby;
    byte contrast_active;
} configuration;


RtcDS3231 Rtc;
RtcDateTime now;
float medicion=0;
String  unidades= "mts";
LCDKeypad lcd;
//LiquidCrystal lcd(8,9,4,5,6,7); 
const int chipSelect = 53;

//variables para el periodo de grabacion en la SD
long intervalo;
unsigned long previousMillis = 0;
unsigned long millisPantalla=0;

//variables para el encoder
const int encoderPinA=18;                   // Used for generating interrupts using CLK signal
const int encoderPinB=19;                    // Used for reading DT signal
volatile float     virtualPosition  =0;  // must be volatile to work with the isr
const float incremento=0.0003125; //  1/32
bool error_memoria=false;

// interrupt service routine vars
boolean A_set = false;              
boolean B_set = false;
static boolean rotating=false;  

// Interrupt on A changing state
void doEncoderA(){
  // debounce
  if ( rotating ) delay (1);  // wait a little until the bouncing is done

  // Test transition, did things really change? 
  if( digitalRead(encoderPinA) != A_set ) {  // debounce once more
    A_set = !A_set;

    // adjust counter + if A leads B
    if ( A_set && !B_set ) 
      virtualPosition += incremento;

    rotating = false;  // no more debouncing until loop() hits again
  }
}

// Interrupt on B changing state, same as A above
void doEncoderB(){
  if ( rotating ) delay (1);
  if( digitalRead(encoderPinB) != B_set ) {
    B_set = !B_set;
    //  adjust counter - 1 if B leads A
    if( B_set && !A_set ) 
      virtualPosition -= incremento;

    rotating = false;
  }
}

void confirmar(String texto,void (*funcion_si)(),String conf1="",String conf2=""){
int buttonPressed;
lcd.setCursor(0,0);
lcd.print(texto);
lcd.setCursor(0,1);
lcd.write((byte)1);
lcd.setCursor(1,1);
lcd.print("si");
lcd.setCursor(4,1);
lcd.write((byte)2);
lcd.setCursor(5,1);
lcd.print("no");
    do
      {
      buttonPressed=waitButton();
      } while(!(buttonPressed==KEYPAD_UP || buttonPressed==KEYPAD_DOWN) && !exitmenu);
      
    if (!exitmenu) {
      waitReleaseButton();
      if (buttonPressed==KEYPAD_UP) {
      funcion_si();
      lcd.setCursor(0,0);
      lcd.print(conf1);
      lcd.setCursor(0,1);
      lcd.print(conf2);
      delay(3000);
      }     
    }
exitmenu=false;   
}


void menuUsed(MenuUseEvent used){
  
  String smenu=String(used.item.getName());
  lcd.setCursor(0,1); 
  lcd.print("                ");
  lcd.setCursor(0,0);  
  if (smenu.indexOf("Salir ")!=-1){
    exitmenu=true; 
  } else if  (smenu.indexOf("Cambiar Hora")!=-1){
    setearHora();
    }  else if  (smenu.indexOf("Cambiar Fecha")!=-1){
    Serial.println("Cambiar fecha");
    setearFecha();
  } else if  (smenu.indexOf("Reset nivel")!=-1){
    Serial.println("Reset Nivel");
   confirmar("Conf. r. nivel?",resetNivel,"Nivel es 0      ","              ");
    exitmenu=true;
  } else if  (smenu.indexOf("Expulsar")!=-1){
    Serial.println("Expulsar");
    expulsarMemoria();
  }
  else if (smenu.indexOf("Insertar")!=-1){
    Serial.println("Insertar");
    insertarMemoria();  
  }
  else if (smenu.indexOf("Brillo")!=-1){
    Serial.println("Brillo");
    configurarBrillo();  
  }

  exitmenu=true;
  lcd.clear();
  activarPantalla();

}

 
 //Menu variables
MenuBackend menu = MenuBackend(menuUsed,menuChanged);
//initialize menuitems
    MenuItem menu1Item1 = MenuItem("Cambiar Fecha   ");
    MenuItem menu1Item2 = MenuItem("Cambiar Hora    ");      
    MenuItem menu1Item3 = MenuItem("Reset nivel       ");
	MenuItem menu1Item4 = MenuItem("Salir Brillo          ");
    MenuItem menu1Item5 = MenuItem("Memoria...        ");
    MenuItem menuItem5SubItem1 = MenuItem("Insertar        ");
    MenuItem menuItem5SubItem2 = MenuItem("Expulsar        ");
    MenuItem menuItem5SubItem3 = MenuItem("Menu anterior   ");
    MenuItem menu1Item6 = MenuItem("Salir           ");

void resetNivel(){
  medicion=0;
  configuration.medicion=0;
  virtualPosition=0;
  EEPROM_writeAnything(0, configuration);
  
}

void menuChanged(MenuChangeEvent changed){
  
  MenuItem newMenuItem=changed.to; //get the destination menu
  
  lcd.setCursor(0,1); //set the start position for lcd printing to the second row
  Serial.print("menuChanged");
  
  if(newMenuItem.getName()==menu.getRoot()){
      lcd.print("Main Menu       ");
  }else {
    lcd.print(newMenuItem.getName());
    }
  
  
}

void setup() {
   
   Serial.begin(9600);
   Wire.begin();

   pinMode(encoderPinA, INPUT); 
   pinMode(encoderPinB, INPUT); 

 // turn on pullup resistors
   digitalWrite(encoderPinA, HIGH);
   digitalWrite(encoderPinB, HIGH);


// encoder pin on interrupt 0 (pin 2)
   attachInterrupt(digitalPinToInterrupt(encoderPinA), doEncoderA, CHANGE);
// encoder pin on interrupt 1 (pin 3)
   attachInterrupt(digitalPinToInterrupt(encoderPinB), doEncoderB, CHANGE);

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
          configuration.memoria=true;
		  configuration.contrast_stby=0;
          configuration.contrast_active=128;
		  EEPROM_writeAnything(0, configuration);
	  }
   else {
    
    medicion=configuration.medicion;
    virtualPosition=medicion;
    Serial.print("medicion en setup:");
    Serial.println(medicion);
	brillo=configuration.contrast_active/12;
	//prevengo que la pantalla arranque apagada
	if (brillo==0){
		brillo=10;
		configuration.contrast_active=12*10;
		}
    }
    //intervalo de grabacion en la micro sd en milisegundos
    intervalo = configuration.minutos_logueo*60*1000;

    seteoReloj();
    initSDcard();


    //configure menu
  menu.getRoot().add(menu1Item1);
  menu1Item1.addRight(menu1Item2).addRight(menu1Item3).addRight(menu1Item4).addRight(menu1Item5).addRight(menu1Item6);
  menu1Item5.addAfter(menuItem5SubItem3);//para que al elegir subir vaya al submenu
  menu1Item5.add(menuItem5SubItem1).addRight(menuItem5SubItem2).addRight(menuItem5SubItem3);
  
  menu.toRoot();

lcd.createChar(0, enie); 
lcd.createChar(1, up); 
lcd.createChar(2, down); 

//pantalla
    lcd.begin(16, 2);
    lcd.clear();
	pinMode(LCD_Backlight, OUTPUT); 
    activarPantalla();
}

void procesarMenu(){
  int buttonPressed=lcd.button();
  if (estado_pantalla==PANTALLA_STBY && buttonPressed!=KEYPAD_NONE) {
		activarPantalla();
		waitReleaseButton();
		return;
  }
  if (buttonPressed==KEYPAD_SELECT){
    waitReleaseButton();
    lcd.setCursor(0,0);  
    lcd.print("Menu            ");
    exitmenu=false;
    menu.toRoot();
    menu.moveDown();
    buttonPressed=KEYPAD_NONE;
    while  (!exitmenu){
      
      while(!(buttonPressed==KEYPAD_SELECT || buttonPressed==KEYPAD_LEFT || buttonPressed==KEYPAD_RIGHT) && !exitmenu)
      {     
      buttonPressed=waitButton();     
      } 
      if (!exitmenu) {
      waitReleaseButton();
      navigateMenus(buttonPressed);  //in some situations I want to use the button for other purpose (eg. to change some settings)
      }
    buttonPressed=KEYPAD_NONE;  
    }
  buttonPressed=KEYPAD_NONE;  
  activarPantalla();	
}
  
}
  
void loop() {
	
	   rotating = true;  // reset the debouncer
     String datestring;
     controlarContraste();
     leerHoraYTemp();
     leerMedicion();
     mostrarDatos(true);  // Escribimos el valor en pantalla
     procesarMenu();

}

void resetValues(){
medicion=0;  
}

void waitReleaseButton()
{
  delay(50);
  while(lcd.button()!=KEYPAD_NONE)
  {
  }
  delay(50);
}

void controlarContraste(){
   unsigned long current;
   current=millis();
   if ((current-millisPantalla)>(minutos_stby*60*1000)) {
    analogWrite(LCD_Backlight, configuration.contrast_stby);  
  	estado_pantalla = PANTALLA_STBY;    
    }
}


void activarPantalla(){
    analogWrite(LCD_Backlight, configuration.contrast_active);  
  	estado_pantalla = PANTALLA_ACTIVA;   
	  millisPantalla=millis();
}


int waitButton()
{
  int buttonPressed; 
  unsigned long starting=millis();
  unsigned long current;
  while((buttonPressed=lcd.button())==KEYPAD_NONE)
  {
    current=millis();
    //controlo que no pase mas de un minuto en espera, sino salgo
    if ((current-starting)>60000) {
          exitmenu=true;
          break;
          }
  }
  delay(50);  
  lcd.noBlink();
  return buttonPressed;
}


boolean buttonProcessBrillo(int b) {
  // Read the buttons five times in a second
  

    // Read the buttons value
   
    switch (b) {

    // Up button was pushed
    case KEYPAD_UP:
        brillo++;
        if (brillo==21) {
          brillo=1;
          configuration.contrast_active=12;
          } else {
          configuration.contrast_active+= 12;
          } 
        activarPantalla();
        break;

    // Down button was pushed
    case KEYPAD_DOWN:
      brillo--;
      if (brillo==0) {
          brillo=20;
          configuration.contrast_active=12*20;
        } else {
          configuration.contrast_active-= 12;
        }      
      activarPantalla();
      break;

      case KEYPAD_SELECT:
        //seteo la hora
        EEPROM_writeAnything(0, configuration);
        return true;
        
    }
    activarPantalla();
    printBrillo();
    return false;

}



void printBrillo() {
  // Set the cursor at the begining of the second row
  lcd.setCursor(5,1);
  char cbrillo[17];
  sprintf(cbrillo, "%02u", brillo);
  lcd.print(cbrillo);
}




void navigateMenus(int b) {
  MenuItem currentMenu=menu.getCurrent();
  Serial.print("button:");
  Serial.println(b);
  switch (b){
    case KEYPAD_SELECT:
      if(!(currentMenu.moveDown() || currentMenu.getName()=="Menu anterior   ")){  //if the current menu has a child and has been pressed enter then menu navigate to item below
        menu.use();
      }else if (currentMenu.getName()=="Menu anterior   ") {
        menu.moveUp();
         Serial.print("moveUp");
        }      
      else {  //otherwise, if menu has no child and has been pressed enter the current menu is used
        menu.moveDown();
       } 
      break;
    case KEYPAD_RIGHT:
    Serial.println("moveRight");
      menu.moveRight();
      break;      
    case KEYPAD_LEFT:
    Serial.println("moveLeft");
      menu.moveLeft();
      break;      
  }
  

}

void setearHora (){
    int buttonPressed;
      boolean salir=false;
      lcd.setCursor(0,0);
      lcd.print("Conf: Hora   ");
      now = Rtc.GetDateTime();
      String s=formatDateTime(now);
      Serial.print("Ahora:");
      Serial.println(s);
        
      hours=now.Hour();
      minutes=now.Minute();
      seconds=now.Second();
      printTime();
      unsigned long starting=millis();
    
    while  (!exitmenu){
      
        do
        {
             buttonPressed=waitButton();
        } while(!(buttonPressed==KEYPAD_SELECT || buttonPressed==KEYPAD_LEFT || buttonPressed==KEYPAD_RIGHT ||
            buttonPressed==KEYPAD_UP || buttonPressed==KEYPAD_DOWN  ) && !exitmenu);
        
        if (!exitmenu) {
              exitmenu=waitReleaseButton(buttonPressed,buttonProcessTime,printTime);
        }
       //buttonPressed=lcd.button();  //I splitted button reading and navigation in two procedures because 
      }
 
  exitmenu=false;
  }
  
void configurarBrillo (){
    int buttonPressed;
      boolean salir=false;
      lcd.setCursor(0,0);
      lcd.print("Conf: Brillo   ");
       
      printBrillo();
      unsigned long starting=millis();
    
     while  (!exitmenu){
      
        do
        {
             buttonPressed=waitButton();
        } while(!(buttonPressed==KEYPAD_SELECT || buttonPressed==KEYPAD_UP || buttonPressed==KEYPAD_DOWN  ) && !exitmenu);
        
        if (!exitmenu) {
              exitmenu=waitReleaseButton(buttonPressed,buttonProcessBrillo,printBrillo);
        }
       //buttonPressed=lcd.button();  //I splitted button reading and navigation in two procedures because 
      }
 
  exitmenu=false;
  }
  


boolean waitReleaseButton(int lastButtonPressed,bool (*funcion_procesar_boton)(int),void (*funcion_print)())
{ int b=lastButtonPressed;
  boolean salir=false;
  
  do {
   
   
   // Increase the time model by one second
   //incTime();
        
          // Print the time on the LCD
   funcion_print();
   salir=funcion_procesar_boton(b);
   delay(200);
   b = lcd.button();
  } while(b!=KEYPAD_NONE && !salir);
  return salir;
  
}



boolean buttonProcessTime(int b) {
  // Read the buttons five times in a second
  

    // Read the buttons value
   
    switch (b) {

    // Right button was pushed
    case KEYPAD_RIGHT:
      settingTime++;
      break;

    // Left button was pushed
    case KEYPAD_LEFT:
      settingTime--;
      break;

    // Up button was pushed
    case KEYPAD_UP:
      switch (settingTime) {
      case HOURS:
        hours++;
        break;
      case MINUTES:
        minutes++;
        break;
      case SECONDS:
        seconds++;
      }     
      break;

    // Down button was pushed
    case KEYPAD_DOWN:
      switch (settingTime) {
      case HOURS:
        hours--;
        if (hours == -1) hours = 23;
        break;
      case MINUTES:
        minutes--;
        if (minutes == -1) minutes = 59;
        break;
      case SECONDS:
        seconds--;
        if (seconds == -1) seconds = 59;
      }
      break;

      case KEYPAD_SELECT:
        if (settingTime==SALIR) {
              Serial.println("Salir");
              return true;
              
          }
        //seteo la hora
        now = Rtc.GetDateTime();
        year=now.Year();
        month=now.Month();
        day=now.Day();
        RtcDateTime datetime=RtcDateTime(year,month,day,hours,minutes,seconds);
        Rtc.SetDateTime(datetime);
        String s=formatDateTime(datetime);
        Serial.println(s);
        lcd.setCursor(0,0);
        lcd.print("hora seteada    ");
        lcd.setCursor(0,1);
        lcd.print(s);
        delay(3000);
        settingTime=HOURS;
        return true;
        
    }

    settingTime %= 4;
    printSettingTime();

    hours %= 24;
    minutes %= 60;
    seconds %= 60;
    printTime();
    return false;

}


void setearFecha (){
      boolean salir=false;
      int buttonPressed;
      lcd.setCursor(0,0);
      lcd.print("Conf: Dia   ");
      now = Rtc.GetDateTime();
      String s=formatDateTime(now);
      Serial.print("Ahora:");
      Serial.println(s);
        
      year=now.Year();
      month=now.Month();
      day=now.Day();
      printDate();
      unsigned long starting=millis();

      while  (!exitmenu){
      
        do
        {
              Serial.println("wait button");
              buttonPressed=waitButton();
              Serial.print("button:");
              Serial.println(buttonPressed);
        } while(!(buttonPressed==KEYPAD_SELECT || buttonPressed==KEYPAD_LEFT || buttonPressed==KEYPAD_RIGHT ||
            buttonPressed==KEYPAD_UP || buttonPressed==KEYPAD_DOWN  ) && !exitmenu);
        
        if (!exitmenu) {
              exitmenu=waitReleaseButton(buttonPressed,buttonProcessDate,printDate);
        }
       //buttonPressed=lcd.button();  //I splitted button reading and navigation in two procedures because 
      }
      
  exitmenu=false;
  }



boolean buttonProcessDate(int b) {

    switch (b) {

    // Right button was pushed
    case KEYPAD_RIGHT:
      settingDate++;
      break;

    // Left button was pushed
    case KEYPAD_LEFT:
      settingDate--;
      break;

    // Up button was pushed
    case KEYPAD_UP:
      switch (settingDate) {
      case DAY:
        day++;
        break;
      case MONTH:
        month++;
        break;
      case YEAR:
        year++;
      }     
      break;

    // Down button was pushed
    case KEYPAD_DOWN:
      switch (settingDate) {
      case DAY:
        day--;
        if (day == -1) day = 31;
        break;
      case MONTH:
        month--;
        if (month == -1) month = 12;
        break;
      case YEAR:
        year--;
      }
      break;

      case KEYPAD_SELECT:
        if (settingDate==SALIR) {
              return true;
          }
        //seteo la hora
        now = Rtc.GetDateTime();
        hours=now.Hour();
        minutes=now.Minute();
        seconds=now.Second();
        RtcDateTime datetime=RtcDateTime(year,month,day,hours,minutes,seconds);
        Rtc.SetDateTime(datetime);
        String s=formatDateTime(datetime);
        Serial.println(s);
        lcd.setCursor(0,0);
        lcd.print("fecha seteada    ");
        lcd.setCursor(0,1);
        lcd.print(s);
        delay(3000);
        settingDate=DAY;
        return true;
        
    }

    settingDate %= 4;
    printSettingDate();

    day %= 32;
    if (day==0) day=1;
    month %= 13;
    if (month==0) month=1;
    printDate();
    return false;
}

// Print the current setting
void printSettingTime() {
  lcd.setCursor(6,0);

  switch (settingTime) {
  case HOURS:
    lcd.print("Hora  ");
    break;
  case MINUTES:
    lcd.print("Minuto");
    break;
  case SECONDS:
    lcd.print("Segundo");
    break;
  case SALIR:
    lcd.print("Salir   ");
  }
}


// Print the current setting
void printSettingDate() {
  lcd.setCursor(6,0);

  switch (settingDate) {
  case YEAR:
    lcd.print("A");
    lcd.setCursor(7,0);
    lcd.write((byte)0);
    lcd.setCursor(8,0);
    lcd.print("o");
    break;
  case MONTH:
    lcd.print("Mes   ");
    break;
  case DAY:
    lcd.print("Dia   ");
    break;
  case SALIR:
    lcd.print("Salir   ");
  }
}
// Increase the time model by one second
void incTime() {
  // Increase seconds
  seconds++;

  if (seconds == 60) {
    // Reset seconds
    seconds = 0;

    // Increase minutes
    minutes++;

    if (minutes == 60) {
      // Reset minutes
      minutes = 0;

      // Increase hours
      hours++;

      if (hours == 24) {
        // Reset hours
        hours = 0;

        // Increase days
    
      }
    }
  }
}

// Increase the time model by one second

// Print the time on the LCD
void printTime() {
  // Set the cursor at the begining of the second row
  lcd.setCursor(0,1);
  char time[17];
  sprintf(time, "%02i:%02i:%02i", hours, minutes, seconds);
  lcd.print(time);
}

void printDate() {
  // Set the cursor at the begining of the second row
  lcd.setCursor(0,1);
  char time[17];
  sprintf(time, "%02u/%02u/%04u", day, month, year);
  lcd.print(time);
}

void insertarMemoria() {
configuration.memoria=true;
  lcd.setCursor(0,0);  
  lcd.print("Verificando     ");
  lcd.setCursor(0,1);
  lcd.print("memoria         "); 
initSDcard();
     if (!error_memoria) {
          lcd.setCursor(0,0);  
          lcd.print("Memoria ok      ");
          lcd.setCursor(0,1);
          lcd.print("                "); 
      }  
      // if the file isn't open, pop up an error:
      else {
        lcd.setCursor(0,0);  
        lcd.print("Error en        ");
        lcd.setCursor(0,1);
        lcd.print("memoria         ");      
      }
      delay(2000);
}

void expulsarMemoria() {
configuration.memoria=false;
lcd.setCursor(0,0);  
lcd.print("Puede retirar la");
lcd.setCursor(0,1);
lcd.print("memoria"); 
delay(2000);
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

  if (configuration.memoria && !error_memoria ) {
          lcd.setCursor(15,1);
          lcd.print("M");
  } else if (configuration.memoria && error_memoria )
  {
          lcd.setCursor(15,1);
          lcd.print("E");
  } else if (!configuration.memoria){
          lcd.setCursor(15,1);
          lcd.print(" ");
  }
  
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
  
		//cambio el valor de medicion, entonces escribo en la memoria
		File dataFile = SD.open("limni.csv", FILE_WRITE);
    
		// if the file is available, write to it:
		if (dataFile) {
      lcd.setCursor(14,1);
      lcd.print("  ");
			dataFile.println(datestring + "," + s + " " + unidades);
			dataFile.close();
      Serial.println("Escribi en memoria");
        }  
      // if the file isn't open, pop up an error:
		else {
			Serial.println("error opening limni. trying to init sd card");
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
     error_memoria=true;
   } else
      {Serial.println("card initialized.");
      error_memoria=false;}     
  s}

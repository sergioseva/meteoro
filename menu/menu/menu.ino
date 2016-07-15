/*
    Copyright Giuseppe Di Cillo (www.coagula.org)
    Contact: dicillo@coagula.org
    
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
IMPORTANT: to use the menubackend library by Alexander Brevig download it at http://www.arduino.cc/playground/uploads/Profiles/MenuBackend_1-4.zip and add the next code at line 195
  void toRoot() {
    setCurrent( &getRoot() );
  }
*/
#include <MenuBackend.h>    //MenuBackend library - copyright by Alexander Brevig
#include <LiquidCrystal.h>  //this library is included in the Arduino IDE
#include <LCDKeypad.h>
#include <Wire.h>
#include <RtcDS3231.h>




#define HOURS 0
#define MINUTES 1
#define SECONDS 2
#define YEAR 2
#define MONTH 1
#define DAY 0
#define SALIR 3
// The LCD screen
LCDKeypad lcd;

// The time model
unsigned int year = 0;
unsigned int month = 0;
unsigned int day= 0;
unsigned int hours = 0;
unsigned int minutes = 0;
unsigned int seconds = 0;
unsigned int settingTime = 0;
unsigned int settingDate = 0;
RtcDS3231 Rtc;
RtcDateTime ahora;




// LiquidCrystal display with:
// rs on pin 7
// rw on ground
// enable on pin 6
// d4, d5, d6, d7 on pins 5, 4, 3, 2
 boolean exitmenu=false;
 
void menuUsed(MenuUseEvent used){
  
  String smenu=String(used.item.getName());
  lcd.setCursor(0,1); 
  lcd.print("                ");
  lcd.setCursor(0,0);  
  if (smenu.indexOf("Salir ")!=-1){
    lcd.print("Sali");  
  } else if  (smenu.indexOf("Cambiar Hora")!=-1){
    setearHora();
    }  else if  (smenu.indexOf("Cambiar Fecha")!=-1){
    Serial.println("Cambiar fecha");
    setearFecha();
  } else if  (smenu.indexOf("Reset Dia")!=-1){
    Serial.println("Reset Dia");
	confirmar("Confirma r. dia?",resetAcumuladoDiario,"Acum diario", "es 0");
//    resetAcumuladoDiario();
  }  else if  (smenu.indexOf("Reset Mes")!=-1){
    Serial.println("Reset Mes");
	confirmar("Confirma r. mes?",resetAcumuladoMensual,"Acum mensual", "es 0");
  //  resetAcumuladoMensual();
  }
  else if (smenu.indexOf("Reset Total")!=-1){
    Serial.println("Reset Todo");
	confirmar("Confirma r. t?",resetAcumuladoTodo,"Acum diario", "y mensual son 0");
  //  resetAcumuladoTodo();
  } else if  (smenu.indexOf("Expulsar")!=-1){
    Serial.println("Expulsar");
	expulsarMemoria();	
  //  resetAcumuladoMensual();
  }
  else if (smenu.indexOf("Insertar")!=-1){
    Serial.println("Insertar");
	insertarMemoria();	
  //  resetAcumuladoTodo();
  }

  lcd.setCursor(0,0);  
  lcd.print("Menu            ");
  lcd.setCursor(0,1);
  lcd.print(smenu);
  //exitmenu=true;
//  menu.toRoot();  //back to Main
}

 
 //Menu variables
MenuBackend menu = MenuBackend(menuUsed,menuChanged);
//initialize menuitems
    MenuItem menu1Item1 = MenuItem("Cambiar Fecha   ");
    MenuItem menu1Item2 = MenuItem("Cambiar Hora    ");      
    MenuItem menu1Item3 = MenuItem("Reset...        ");
    MenuItem menuItem3SubItem1 = MenuItem("Reset Dia       ");
    MenuItem menuItem3SubItem2 = MenuItem("Reset Mes       ");
    MenuItem menuItem3SubItem3 = MenuItem("Reset Total     ");
    MenuItem menuItem3SubItem4 = MenuItem("Subir           ");
	MenuItem menu1Item4 = MenuItem("Memoria...        ");
    MenuItem menuItem4SubItem1 = MenuItem("Expulsar        ");
    MenuItem menuItem4SubItem2 = MenuItem("Insertar       ");
    MenuItem menuItem4SubItem3 = MenuItem("Subir           ");
    MenuItem menu1Item5 = MenuItem("Salir           ");



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

void setup()
{

  Serial.begin(9600);
  lcd.begin(16, 2);

  seteoReloj();
    
  //configure menu
  menu.getRoot().add(menu1Item1);
  menu1Item1.addRight(menu1Item2).addRight(menu1Item3).addRight(menu1Item4).addRight(menu1Item5);
  menu1Item3.addAfter(menuItem3SubItem4);//para que al elegir subir vaya al submenu
  menu1Item3.add(menuItem3SubItem1).addRight(menuItem3SubItem2).addRight(menuItem3SubItem3).addRight(menuItem3SubItem4);
  menu1Item4.addAfter(menuItem4SubItem3);//para que al elegir subir vaya al submenu
  menu1Item4.add(menuItem4SubItem1).addRight(menuItem4SubItem2).addRight(menuItem4SubItem3);
  
  menu.toRoot();
  lcd.setCursor(0,0);  
  lcd.print("Sin menu       ");
   lcd.setCursor(0,1);  
  lcd.print("               ");

}  // setup()...


void loop()
{ int buttonPressed;
  lcd.setCursor(0,0);  
  lcd.print("Sin menu        ");
   lcd.setCursor(0,1);  
  lcd.print("                ");


  buttonPressed=lcd.button();
  if (buttonPressed==KEYPAD_SELECT){
	  waitReleaseButton();
	  lcd.setCursor(0,0);  
	  lcd.print("Menu            ");
	  exitmenu=false;
	  menu.toRoot();
	  menu.moveDown();
	  while  (!exitmenu){
		  
		  while(!(buttonPressed==KEYPAD_SELECT || buttonPressed==KEYPAD_LEFT || buttonPressed==KEYPAD_RIGHT) && !exitmenu)
		  {			
			buttonPressed=waitButton();			
		  } 
		  if (!exitmenu) {
			waitReleaseButton();
			navigateMenus(buttonPressed);  //in some situations I want to use the button for other purpose (eg. to change some settings)
		  }
	  //buttonPressed=lcd.button();  //I splitted button reading and navigation in two procedures because 
	  }
	buttonPressed=KEYPAD_NONE;  
  
}

  
  
                  
} //loop()... 


void waitReleaseButton()
{
  delay(50);
  while(lcd.button()!=KEYPAD_NONE)
  {
  }
  delay(50);
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




void navigateMenus(int b) {
  MenuItem currentMenu=menu.getCurrent();
  Serial.print("button:");
  Serial.println(b);
  switch (navigateMenus){
    case KEYPAD_SELECT:
      if(!(currentMenu.moveDown() || currentMenu.getName()=="Subir           ")){  //if the current menu has a child and has been pressed enter then menu navigate to item below
        menu.use();
      }else if (currentMenu.getName()=="Subir           ") {
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
      ahora = Rtc.GetDateTime();
      String s=formatDateTime(ahora);
      Serial.print("Ahora:");
      Serial.println(s);
        
      hours=ahora.Hour();
      minutes=ahora.Minute();
      seconds=ahora.Second();
      printTime();
      unsigned long starting=millis();
	  
	  while  (!exitmenu){
      
			  do
			  {
				     buttonPressed=waitButton();
			  } while(!(buttonPressed==KEYPAD_SELECT || buttonPressed==KEYPAD_LEFT || buttonPressed==KEYPAD_RIGHT ||
					  buttonPressed==KEYPAD_UP || buttonPressed==KEYPAD_DOWN	) && !exitmenu);
			  
			  if (!exitmenu) {
							exitmenu=waitReleaseButton(buttonPressed,buttonProcessTime,printTime);
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

void confirmar(String texto,void (*funcion_si)(),String conf1="",String conf2=""){
int buttonPressed;
lcd.setCursor(0,0);
lcd.print(texto);
lcd.setCursor(0,1);
lcd.print("up si   down no");
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
		  }		  
		}
exitmenu=false;	  
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
        ahora = Rtc.GetDateTime();
        year=ahora.Year();
        month=ahora.Month();
        day=ahora.Day();
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
      ahora = Rtc.GetDateTime();
      String s=formatDateTime(ahora);
      Serial.print("Ahora:");
      Serial.println(s);
        
      year=ahora.Year();
      month=ahora.Month();
      day=ahora.Day();
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
        ahora = Rtc.GetDateTime();
        hours=ahora.Hour();
        minutes=ahora.Minute();
        seconds=ahora.Second();
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
    lcd.print("Año   ");
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

#define countof(a) (sizeof(a) / sizeof(a[0]))

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

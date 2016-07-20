#ifndef MenuController_h
#define MenuController_h
#include <Arduino.h>


#define HOURS 0
#define MINUTES 1
#define SECONDS 2
#define YEAR 2
#define MONTH 1
#define DAY 0
#define SALIR 3



class MenuController
{
public:
	// The time model
	unsigned int year = 0;
	unsigned int month = 0;
	unsigned int day= 0;
	unsigned int hours = 0;
	unsigned int minutes = 0;
	unsigned int seconds = 0;
	unsigned int settingTime = 0;
	unsigned int settingDate = 0;
	

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

	MenuController(){
		lcd.createChar(0, enie);
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


private:

};


#endif

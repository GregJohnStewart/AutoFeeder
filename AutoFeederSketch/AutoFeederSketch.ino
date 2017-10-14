/* Sketch for Arduino to run the automatic feeder.
 *
 * Requires the Adafruit RTC library for the timing, even if not used.
 */

//includes
#include <Servo.h> //for running the servo
#include <Wire.h> //required for RTClib.h
#include "RTClib.h" //to run the real time clock functionality. https://github.com/adafruit/RTClib

//status indicator values
const int onboardLedPin = 13;

//buzzer values
const boolean buzz = true; //turns on and off the buzzer functionality
const int buzzerPin = 8;
const int buzzLength = 250;
const int buzzDefaultFreq = 1000;
const int buzzFeedStartFreq = 2000;
const int buzzFeedEndFreq = 500;
const int buzzBattWarnFreq = 2000;
const int buzzConfirmFreq = 100;

//Servo setup
Servo dispenserServo;
const int servoPin = 12;
const int feedServoSpinTime = 1000;
const int STOP = 90;
const int SPIN_CLOCKWISE = 180;
const int SPIN_COUNTER_CLOCKWISE = 0;

//clock
boolean timingEnabled = true;
RTC_DS3231 rtc;
const int numFeedTimes = 2;//be sure to update this
String feedTimes[numFeedTimes] = {//include hour, minute, and second without leading zero padding
  "7:0:0", // 7:00 AM
  "17:30:0" // 5:30 PM
};

//button
const int buttonPin = 7;
int buttonVal = 0; //the value gotten from the button when checked.
const int longPressLength = 500;

//other
const int mainWait = 250; //the number of milliseconds to wait before looping around in loop()

/*
 * Does a buzz if it is set to.
 */
void doBuzz(int frequency, boolean wait){
  if(buzz){
    tone(buzzerPin, frequency, buzzLength);
    if(wait){
      delay(buzzLength);
    }
  }
}

/*
 * Default function to not wait for buzz to finish.
 */
void doBuzz(){
  doBuzz(buzzDefaultFreq, false);
}

/*
 * Checks the state of the button, puts it into buttonVal
 */
void checkButton(){
  buttonVal = digitalRead(buttonPin);
}

/*
 * Formats a datetime in a readable format, for output.
 */
String formatDateTime(DateTime dtIn){
  return String(dtIn.month()) + "/" + String(dtIn.day()) + "/" + String(dtIn.year()) + " " + String(dtIn.hour()) + ":" + String(dtIn.minute()) + ":" + String(dtIn.second());
}
String formatDateTime(){
  return formatDateTime(rtc.now());
}

/*
 * Determines if it is time to do the feeding.
 */
boolean isFoodTime(){
  if(!timingEnabled){
    return false;
  }
  
  //build the timestamp to compare with
  DateTime curTime = rtc.now();
  Serial.println("Current Time: " + formatDateTime(curTime));
  String hr = String(curTime.hour());
  String mn = String(curTime.minute());
  String sd = String(curTime.second());
  String time = hr + ":" + mn + ":" + sd;

  //iterate through feed times to see if it is time
  for(int i = 0; i < numFeedTimes; i++){
    Serial.print("\"" + time + "\" == \"" + feedTimes[i] + "\" ?");
    
    if(feedTimes[i].equals(time)){//compare datetimes?
      Serial.println(" Yes");
      return true;
    }
    Serial.println(" No");
  }
  return false;
}

/*
 * Gets a number from a button. Use long press to exit.
 * Will beep once when incremented a number, and twice when confirmed the number, after a long press.
 * Loops around if incremented past the maxVal
 */
int getNumFromButton(int startAt, int maxVal){
  doBuzz(buzzFeedEndFreq, true);
  int output = startAt;
  bool counting = true;
  do{
    checkButton();
    if(buttonVal == HIGH){
      delay(longPressLength);
      checkButton();
      if(buttonVal == HIGH){
        counting = false;
      }else{
        output++;
        if(output > maxVal){
          output = startAt;
        }
        doBuzz(buzzConfirmFreq, true);
        delay(longPressLength);
      }
    }
  }while(counting);
  doBuzz(buzzConfirmFreq, true);
  delay(buzzLength);
  doBuzz(buzzConfirmFreq, true);
  return output;
}

/*
 * Sets the time using getNumFromButton()
 *
 * Order of setting (start value, max value):
 *   year (2000, 3000)
 *   day  (1, 31)
 *   hour (0, 23)
 *   minute (0, 59)
 */
void setTime(){
  delay(buzzLength);
  delay(buzzLength);
  doBuzz(buzzConfirmFreq, true);
  delay(buzzLength);
  doBuzz(buzzConfirmFreq, true);
  delay(buzzLength);
  doBuzz(buzzConfirmFreq, true);
  Serial.print("Getting Year... (");
  int year = getNumFromButton(2000, 3000);
  Serial.print(String(year) + ")\nGetting Month... (");
  int month = getNumFromButton(1, 12);
  Serial.print(String(month) + ")\nGetting Day... (");
  int day = getNumFromButton(1, 31);
  Serial.print(String(day) + ")\nGetting Hour... (");
  int hour = getNumFromButton(0, 23);
  Serial.print(String(hour) + ")\nGetting Minute... (");
  int minute = getNumFromButton(0, 59);
  Serial.println(String(minute));
  int second = 0;
  delay(buzzLength);
  doBuzz(buzzConfirmFreq, true);

  Serial.println("Finished getting numbers.");
  rtc.adjust(DateTime(year, month, day, hour, minute, second));
  Serial.println("Clock set to: " + formatDateTime());
}

/*
 * Does the feeding. Beeps, turns the servo, then beeps again when done.
 */
void feed(){
  Serial.println("FEEDING.");
  doBuzz(buzzFeedStartFreq, true);
  delay(buzzLength);

  pinMode(servoPin, OUTPUT);
  dispenserServo.write(SPIN_CLOCKWISE);
  delay(feedServoSpinTime);
  dispenserServo.write(STOP);
  digitalWrite(servoPin, LOW);
  pinMode(servoPin, INPUT);
  
  delay(buzzLength);
  doBuzz(buzzFeedEndFreq, false);
  Serial.println("FEEDING COMPLETE.");
  delay(1000);// ensure we are clear of the timeframe for timing
}

/*
 * Setup of things:
 *  - status led
 *  - buzzer
 *  - servo
 */
void setup(){
  Serial.begin(9600);
  Serial.println("\nBeginning setup of the Feeder.");
  // status led
  pinMode(onboardLedPin, OUTPUT);
  // buzzer
  pinMode(buzzerPin, OUTPUT);
  doBuzz();
  //servo
  dispenserServo.attach(servoPin);
  pinMode(servoPin, OUTPUT);
  //dispenserServo.write(STOP);

  if(timingEnabled){
    //clock
    boolean rtcOn = rtc.begin();
    //Serial.print(String(rtcOn) + "\n");
    if(!rtcOn){
      Serial.println("Couldn't find RTC. Timing Disabled");
      timingEnabled = false;
      doBuzz(buzzConfirmFreq, true);
      delay(buzzLength);
      doBuzz(buzzConfirmFreq, true);
      delay(buzzLength);
      doBuzz(buzzConfirmFreq, true);
      delay(buzzLength);
      doBuzz(buzzConfirmFreq, true);
    }else{
      if(rtc.lostPower()){
        Serial.println("Clock lost power.");
        setTime();
      }
    }
  }
  if(!timingEnabled){
    Serial.println("Timing disabled. Will only feed on button press.");
  }else{
    Serial.println("Current time: " + formatDateTime());
  }

  //button
  pinMode(buttonPin, INPUT);

  Serial.println("Finished setup.\n");
}

/* the loop routine runs over and over again forever
 *
 * Long press button to set the time anytime.
 */
void loop() {
  Serial.println("Begin loop()");
  checkButton();
  Serial.println("State of button: " + String(buttonVal));
  if(//decide if we need to dispense food
  buttonVal == HIGH || //if button pressed
  isFoodTime()
    ){
    if(buttonVal == HIGH){
      delay(longPressLength);
      checkButton();
      if(buttonVal == HIGH){
        setTime();
      }else{
        feed();
      }
    }else{
      feed();
    }
  }

  delay(mainWait);
  Serial.println("End loop()\n");
}


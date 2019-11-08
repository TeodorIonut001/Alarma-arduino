#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#define SS_PIN 10
#define RST_PIN 9
//~~~~~~~~~~ DATA variables - DON'T CHANGE ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.
//Non-user ouput
long accelX, accelY, accelZ;
long gyroX, gyroY, gyroZ;

//~~~~~~~~ USER VARIABLES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//pentru a seta alarma dorita, folosim "setAlarm( 1 )" sau " setAlarm ( n ) " ; n = nr. intreg, selecteaza alarma dorita 

String CARD_UID = "CB D8 BB 79";  //USER ID - ONE PER USER;
float rotX, rotY, rotZ;
float gForceX, gForceY, gForceZ;
boolean isLocked;
boolean runAlarm;
boolean calibrateOnce = true;
boolean playAlarmOnce = true;
boolean playToneOnce  = true;
int buzzer = 3; // ONLY ON DIGITAL PWM PIN
//int pot = 6; //potentiometru
int mpuRead[4]; 
int stopCTime;
int currentTime;
int numOfCalibrations = 1000; 
float error = 7;//error (deg 0->180);
long startAlarmTime;
long unsigned toneAlarmTime;
long unsigned timeX;
boolean long_alarm = false;
// ============================ SETUP & LOOP ARDUINO ==================================================================
void setup() 
{
  //Setup comunicatii generale
  setupMPU();
  Serial.begin(9600);   // Initiate a serial communication
  SPI.begin();      // Initiate  SPI bus
  mfrc522.PCD_Init();   // Initiate MFRC522
  //Potentiometru
//  pinMode(pot, INPUT);
  //BUZZER
  isLocked = true;
  pinMode(buzzer, OUTPUT);
  Serial.begin(9600);
  Wire.begin();
}

void loop() 
{
  RFID();
  lockState(isLocked); 
}
// ============================ SETUP & LOOP ARDUINO ==================================================================

//~~~~~~~~~ USER FUNCTIONS ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void lockState(boolean state){
  if(state)
  {
    //Primim datele de la giroscop
    if(calibrateOnce){
      lockCalibrateMPUGyro();
      calibrateOnce = false;
    }
    checkMovement(mpuRead);
  }
  else{
    Serial.println("Alarma dezactivata");
    runAlarm = false;
    pinMode(buzzer, LOW);
    calibrateOnce = true;
    playAlarmOnce = true;
    long_alarm = false;
  }
}

void lockCalibrateMPUGyro(){
    int counter = 0;
    currentTime = millis();
    //Citeste primul set
    Serial.println("Incepe calibrarea aparatului ...");
    setAlarm(1);
    measureMPU();
    //min
    mpuRead[0] = gForceX;
    mpuRead[1] = gForceY;
    //max
    mpuRead[2] = gForceX;
    mpuRead[3] = gForceY;

    //citeste urmatoarele 99 seturi
    while(counter < numOfCalibrations)
    {
      measureMPU();
      if(gForceX < mpuRead[0]) mpuRead[0] = gForceX;
      if(gForceY < mpuRead[1]) mpuRead[1] = gForceY;
      if(gForceX < mpuRead[2]) mpuRead[2] = gForceX;
      if(gForceY > mpuRead[3]) mpuRead[3] = gForceY;
  
      counter ++;
    }
    Serial.println("Calibrare terminata");
    stopCTime = millis();
    Serial.print("Calibrare efectuata in: ");
    Serial.print((stopCTime-currentTime)/1000);
    Serial.println(" secunde");
}

void checkMovement(int x[4]){
    measureMPU();
    runAlarm = false;
    int k;
    for(int i=0; i< 4; i++){
      //testam mpuRead curent
      if(gForceX + error < x[0]) runAlarm = true;
      if(gForceX - error > x[2]) runAlarm = true;
      
      if(gForceY + error < x[1]) runAlarm = true;
      if(gForceY - error > x[3]) runAlarm = true;
    }
    if(runAlarm){
      Serial.println("Alarma pornita");
      Serial.print("X: ");
      Serial.println(gForceX);
      Serial.print("Y: ");
      Serial.println(gForceY);
      if(!long_alarm)
      {
        setAlarm(2);  
      }
      if(long_alarm){
        setAlarm(3);
      }
      long_alarm = true;
    }   
}
void setAlarm(int i){
    
    //Introduce cantecul aici 
    if(i==1) alarm_1();
    if(i==2) alarm_2();
    if(i==3) alarm_3();
    if(i==4) alarm_4();

}

void playTone(unsigned long freq, unsigned long delayTime){
  if(playToneOnce && playAlarmOnce){
    toneAlarmTime = millis() + delayTime;
    while(millis() < toneAlarmTime)
      {
      analogWrite(buzzer, freq);
      //TODO citeste RFID
        if(RFID_STOP_ALARM()){
          playAlarmOnce = false;
          isLocked = false;
          return;
        }

      }
      analogWrite(buzzer, 0);
      playToneOnce = false;
  }
  playToneOnce = true;
}

//Return from 0 to 90 degrees 
float r3S(float gForce, float max_value){
  return gForce*90/max_value;
}

void dataToDeg(){
  //gForceX --- .52
  if(gForceZ >= 0) gForceX = r3S(gForceX, 0.52);
  if(gForceZ <  0)  gForceX = 180 - r3S(gForceX, 0.52); 
  //gForceY --- .50
  gForceY = r3S(gForceY, 0.50);
  //gForceZ -TODO not needed

}
//calibrare
void alarm_1(){
  /*  Exemplu: 
  playTone(1000, 500); //where freq = 1000Hz and tone duration =  500 miliseconds   */
  playTone(100, 1000);

}
//scurta
void alarm_2(){

 playTone(2000, 200);
 playTone(0, 200);
 playTone(100, 500);
 playTone(0, 200);
 playTone(2000, 200);
 playTone(0, 200);
 playTone(100, 500);
 playTone(0, 200);
 playTone(2000, 200);
 playTone(0, 200);
 playTone(100, 500);
 playTone(0, 200);
  
}
//alarma lunga
void alarm_3(){
 for(int i=0;i < 100; i++){
  playTone(1915, 50);
  playTone(0, 50);
 }
}

void alarm_4(){
  playTone(500, 50);
  playTone(0, 50);
  playTone(500, 50);
}

//~~~~~ DATA FUNCTIONS ~~~~ DON'T CHANGE ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

boolean RFID_STOP_ALARM(){
 boolean stop_alarm = false; 
  
 mfrc522.PICC_IsNewCardPresent();
 // mfrc522.PICC_ReadCardSerial();
 if( mfrc522.PICC_ReadCardSerial() == 0){
    //Serial.println("Unknown");
    return;
  }
  mfrc522.PICC_ReadCardSerial();
  Serial.println("UID tag :");
  String content= "";
  byte letter;
  for (byte i = 0; i < mfrc522.uid.size; i++) 
  {
     Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
     Serial.print(mfrc522.uid.uidByte[i], HEX);
     content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
     content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  Serial.println();
  content.toUpperCase();
  delay(500);
  //Daca recunoaste
  if (content.substring(1) == CARD_UID) //change here the UID of the card/cards that you want to give access
  { 
    stop_alarm = true;
    setAlarm(4);
  }else{
    stop_alarm = false;
  }
  
  return stop_alarm;
}

void RFID(){
  mfrc522.PICC_IsNewCardPresent();
 // mfrc522.PICC_ReadCardSerial();
 if( mfrc522.PICC_ReadCardSerial() == 0){
    //Serial.println("Unknown");
    return;
  }
  mfrc522.PICC_ReadCardSerial();
  Serial.println("UID tag :");
  String content= "";
  byte letter;
  for (byte i = 0; i < mfrc522.uid.size; i++) 
  {
     Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
     Serial.print(mfrc522.uid.uidByte[i], HEX);
     content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
     content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  Serial.println();
  content.toUpperCase();
  delay(500);
  //Daca recunoaste
  if (content.substring(1) == CARD_UID) //change here the UID of the card/cards that you want to give access
  { 
    if(isLocked){
      isLocked = false;
      setAlarm(4);
    }
    else
    {
      isLocked = true;  
    }
    delay(500);
  }
}

void measureMPU(){
  recordAccelRegisters();
  recordGyroRegisters();
  dataToDeg();
}

//setup registrii giroscop+accelerometru
void setupMPU(){
  Wire.beginTransmission(0b1101000); //This is the I2C address of the MPU (b1101000/b1101001 for AC0 low/high datasheet sec. 9.2)
  Wire.write(0x6B); //Accessing the register 6B - Power Management (Sec. 4.28)
  Wire.write(0b00000000); //Setting SLEEP register to 0. (Required; see Note on p. 9)
  Wire.endTransmission();  
  Wire.beginTransmission(0b1101000); //I2C address of the MPU
  Wire.write(0x1B); //Accessing the register 1B - Gyroscope Configuration (Sec. 4.4) 
  Wire.write(0x00000000); //Setting the gyro to full scale +/- 250deg./s 
  Wire.endTransmission(); 
  Wire.beginTransmission(0b1101000); //I2C address of the MPU
  Wire.write(0x1C); //Accessing the register 1C - Acccelerometer Configuration (Sec. 4.5) 
  Wire.write(0b00001000); //Setting the accel to +/- 4g
  Wire.endTransmission(); 
}


void recordAccelRegisters() {
  Wire.beginTransmission(0b1101000); //I2C address of the MPU
  Wire.write(0x3B); //Starting register for Accel Readings
  Wire.endTransmission();
  Wire.requestFrom(0b1101000,6); //Request Accel Registers (3B - 40)
  while(Wire.available() < 6);
  accelX = Wire.read()<<8|Wire.read(); //Store first two bytes into accelX
  accelY = Wire.read()<<8|Wire.read(); //Store middle two bytes into accelY
  accelZ = Wire.read()<<8|Wire.read(); //Store last two bytes into accelZ
  processAccelData();
}


void processAccelData(){
  gForceX = accelX / 16384.0;
  gForceY = accelY / 16384.0; 
  gForceZ = accelZ / 16384.0;
}

void recordGyroRegisters() {
  Wire.beginTransmission(0b1101000); //I2C address of the MPU
  Wire.write(0x43); //Starting register for Gyro Readings
  Wire.endTransmission();
  Wire.requestFrom(0b1101000,6); //Request Gyro Registers (43 - 48)
  while(Wire.available() < 6);
  gyroX = Wire.read()<<8|Wire.read(); //Store first two bytes into accelX
  gyroY = Wire.read()<<8|Wire.read(); //Store middle two bytes into accelY
  gyroZ = Wire.read()<<8|Wire.read(); //Store last two bytes into accelZ
  processGyroData();
}

void processGyroData() {
  rotX = gyroX / 131.0;
  rotY = gyroY / 131.0; 
  rotZ = gyroZ / 131.0;
}

void printData() {
  Serial.print("Gyro (deg)");
  Serial.print(" X=");
  Serial.print(rotX);
  Serial.print(" Y=");
  Serial.print(rotY);
  Serial.print(" Z=");
  Serial.print(rotZ);
  Serial.print(" Accel (g)");
  Serial.print(" X=");
  Serial.print(gForceX);
  Serial.print(" Y=");
  Serial.print(gForceY);
  Serial.print(" Z=");
  Serial.println(gForceZ);
}

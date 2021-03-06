#include <Weight.h>
#include <PostureDetector.h>

//object weight estimation
Weight scale;
LoadType load_type;

//posture detection
PostureDetector detector;
ElevationType elevation_type;
TwistType twist_type;
BackTiltType backtilt_type;

//console output
String classification = "";
String w_output = "";
String e_output = "";
String t_output = "";
String b_output = "";
String lift_dura = "";

//lift timerr
unsigned long start;
unsigned long end;
bool heavy_lift = false;  //used by checkTime to check if a lift attempt has started
const int MAX_LIFT_DURATION = 60000;

//detection delay
unsigned long d_start;
unsigned long d_end;

//buffer constants
const int BUF_SIZE = 5;
const int POLL_DELAY = 50;

// INPUT BUFFERS
int buf_0 [BUF_SIZE];    
int buf_1 [BUF_SIZE];     
int buf_2 [BUF_SIZE];     
int buf_3 [BUF_SIZE];     
int buf_4 [BUF_SIZE];     
int buf_5 [BUF_SIZE];     
int buf_6 [BUF_SIZE];     
int buf_7 [BUF_SIZE];     
int buf_8 [BUF_SIZE];     
int buf_9 [BUF_SIZE];     
int buf_10 [BUF_SIZE];    
int buf_11 [BUF_SIZE];    
int val_12 = 0;           
int val_13 = 0;           

//filtered values - can adjust the pin to sensor mapping later
int leftPressureUpper = 0;
int leftPressureLower = 0;
int leftPressureFinger = 0;
int rightPressureUpper = 0;
int rightPressureLower = 0;
int rightPressureFinger = 0;
int neckAccelX = 0;
int neckAccelY = 0;
int neckAccelZ = 0;
int wristAccelX = 0;
int wristAccelY = 0;
int wristAccelZ = 0;
int tiltUpperBack = 0;
int tiltLowerBack = 0;

//input index and avg buffers
int buf_index[12] = {0,0,0,0,0,0,0,0,0,0,0,0};
int buf_sum[12]   = {0,0,0,0,0,0,0,0,0,0,0,0};
int buf_avg[12]   = {0,0,0,0,0,0,0,0,0,0,0,0};
String consoleOutput;

void setup() {
  Serial.begin(9600);
  initIO();  
}

void initIO(){
  pinMode(0, INPUT);
  pinMode(1, INPUT);
  pinMode(2, INPUT);
  pinMode(3, INPUT);
  pinMode(4, INPUT);
  pinMode(5, INPUT);
  pinMode(6, INPUT);  
  pinMode(7, INPUT);
  pinMode(26, INPUT);
  pinMode(27, INPUT);
  pinMode(28, INPUT);
  pinMode(29, INPUT);
  pinMode(30, INPUT);
  pinMode(31, INPUT);    
  pinMode(9, OUTPUT);
}

void meanFilterInput(){
  //record analog input, 13 and 14 are binary, so no averaging is required
  buf_0[buf_index[0]]   = analogRead(0);  //left finger
  buf_1[buf_index[1]]   = analogRead(1);  //left upper palm
  buf_2[buf_index[2]]   = analogRead(2);  //right finger
  buf_3[buf_index[3]]   = analogRead(3);  //right upper palm
  buf_4[buf_index[4]]   = analogRead(4);  //right lower palm
  buf_5[buf_index[5]]   = analogRead(5);  //left lower palm
  buf_6[buf_index[6]]   = analogRead(26); //accel_1_x_neck
  buf_7[buf_index[7]]   = analogRead(27); //accel_1_y_neck
  buf_8[buf_index[8]]   = analogRead(28); //accel_1_z_neck
  buf_9[buf_index[9]]   = analogRead(29); //accel_2_x_wrist
  buf_10[buf_index[10]] = analogRead(30); //accel_2_y_wrist
  buf_11[buf_index[11]] = analogRead(31); //accel_2_z_wrist
  val_12                = analogRead(7);  //tilt_short_upper
  val_13                = analogRead(6);  //tilt_tall_lower

  //update buffer indices
  buf_index[0] = (buf_index[0] + 1) % BUF_SIZE;
  buf_index[1] = (buf_index[1] + 1) % BUF_SIZE;
  buf_index[2] = (buf_index[2] + 1) % BUF_SIZE;
  buf_index[3] = (buf_index[3] + 1) % BUF_SIZE;
  buf_index[4] = (buf_index[4] + 1) % BUF_SIZE;
  buf_index[5] = (buf_index[5] + 1) % BUF_SIZE;
  buf_index[6] = (buf_index[6] + 1) % BUF_SIZE;
  buf_index[7] = (buf_index[7] + 1) % BUF_SIZE;
  buf_index[8] = (buf_index[8] + 1) % BUF_SIZE;
  buf_index[9] = (buf_index[9] + 1) % BUF_SIZE;
  buf_index[10] = (buf_index[10] + 1) % BUF_SIZE;
  buf_index[11] = (buf_index[11] + 1) % BUF_SIZE;

  //sum up current sensor value with past values
  for (int i = 0; i < BUF_SIZE; ++i){      
    buf_sum[0] = buf_sum[0] + buf_0[i];
    buf_sum[1] = buf_sum[1] + buf_1[i];
    buf_sum[2] = buf_sum[2] + buf_2[i];
    buf_sum[3] = buf_sum[3] + buf_3[i];
    buf_sum[4] = buf_sum[4] + buf_4[i];
    buf_sum[5] = buf_sum[5] + buf_5[i];
    buf_sum[6] = buf_sum[6] + buf_6[i];
    buf_sum[7] = buf_sum[7] + buf_7[i];
    buf_sum[8] = buf_sum[8] + buf_8[i];
    buf_sum[9] = buf_sum[9] + buf_9[i];
    buf_sum[10] = buf_sum[10] + buf_10[i];
    buf_sum[11] = buf_sum[11] + buf_11[i];      
  }

  //calculate the average
  for (int i = 0; i < 12; ++i){    
    buf_avg[i] = buf_sum[i]/BUF_SIZE;      
  } 
}

void resetBuf(){
  for (int i = 0; i < 12; ++i){
    buf_sum[i] = 0;
    buf_avg[i] = 0;
  }
}

void mapSensors(){
  leftPressureFinger  = buf_avg[0];
  leftPressureUpper   = buf_avg[1];
  rightPressureFinger = buf_avg[2];
  rightPressureUpper  = buf_avg[3];
  rightPressureLower  = buf_avg[4];
  leftPressureLower   = buf_avg[5];    
  neckAccelX          = buf_avg[6];   //accel_1_x
  neckAccelY          = buf_avg[7];   //accel_1_y
  neckAccelZ          = buf_avg[8];   //accel_1_z
  wristAccelY         = buf_avg[9];   //accel_2_x
  wristAccelX         = buf_avg[10];  //accel_2_y
  wristAccelZ         = buf_avg[11];  //accel_2_z
  tiltUpperBack       = val_12;       //tilt_short
  tiltLowerBack       = val_13;       //tilt_tall
}


bool checkTime(){
  if (!heavy_lift){
    heavy_lift = true;
    start = millis();
  }
  end = millis();
  
  if ((end - start) >= MAX_LIFT_DURATION){
    //triggerHaptic();
    return true;
  }  
  return false;  
}

void checkLift(){
  load_type = scale.estimateWeight(leftPressureUpper, leftPressureLower, leftPressureFinger, rightPressureUpper, rightPressureLower, rightPressureFinger);
   
  if (load_type == LIGHT){
    elevation_type = UNKNOWN_ELEV;
    twist_type = NONE;
    backtilt_type = UNKNOWN_TILT;
    heavy_lift = false;    
    detector.clearImpulseState();    
    stopHaptic();    
  }
  else if (load_type == HEAVY){
    //check lift duration and check for incorrect posture indicators    
    elevation_type = detector.checkArmElevation(wristAccelX, wristAccelY, wristAccelZ);
    twist_type = detector.checkImpulse(wristAccelX, wristAccelY, wristAccelZ);
    backtilt_type = detector.checkBackTilt(tiltUpperBack, tiltLowerBack);

    //if any indicators of incorrect posture are found, trigger warning
    if((elevation_type == HIGH_ELEV) || (twist_type == TWIST) || (backtilt_type == FULLY_BENT) || checkTime()){
      triggerHaptic();      
    }
    else{      
      stopHaptic();
    }

  }  else if (load_type == OVERLOAD){
    elevation_type = UNKNOWN_ELEV;
    twist_type = NONE;
    backtilt_type = UNKNOWN_TILT;
    heavy_lift = false;
    detector.clearImpulseState();    
    triggerHaptic();
  }  

  printStatus(load_type, elevation_type, twist_type, backtilt_type);
}

void triggerHaptic(){
  classification = "WARNING";
  analogWrite(9, 4095);
}

void stopHaptic(){
  classification = "SAFE   ";
  analogWrite(9, 0);
}

void loop() {
  meanFilterInput(); 
  mapSensors();  

  //console
  //Serial.println(String(neckAccelX) + ", " + String(neckAccelY) + ", " + String(neckAccelZ));
  //Serial.print(String(0) + ", ");
  //Serial.print(String(1000) + ", ");
  //Serial.println(String(wristAccelX) + ", " + String(wristAccelY) + ", " + String(wristAccelZ));
  //Serial.println(String(tiltUpperBack) + ", " + String(tiltLowerBack));
  //Serial.println(String(leftPressureUpper) + ", " + String(leftPressureLower) + ", " + String(leftPressureFinger) + ", " + String(rightPressureUpper) + ", " + String(rightPressureLower) + ", " + String(rightPressureFinger));

  checkLift();     
  
  //debug
  //twist_type = detector.checkImpulse(wristAccelX, wristAccelY, wristAccelZ);
  //elevation_type = detector.checkArmElevation(wristAccelX, wristAccelY, wristAccelZ);
  //backtilt_type = detector.checkBackTilt(tiltUpperBack, tiltLowerBack);
  //load_type = scale.estimateWeight(leftPressureUpper, leftPressureLower, leftPressureFinger, rightPressureUpper, rightPressureLower, rightPressureFinger);
  //triggerHaptic();
  //printConsole();
         
  resetBuf(); 
  delay(POLL_DELAY);
}

void printStatus(LoadType weight, ElevationType elevation, TwistType twist, BackTiltType tilt){
  lift_dura = String(end - start);

  switch(weight){
    case LIGHT:
      w_output = "LIGHT   ";
      break;
    case HEAVY:
      w_output = "HEAVY   ";
      break;
    case OVERLOAD:
      w_output = "OVERLOAD";
      break;
    default:
      w_output = "UNKNOWN ";
  }

  switch(elevation){
    case LOW_ELEV:
      e_output = "LOW    ";
      break;
    case MODERATE_ELEV:
      e_output = "MOD    ";
      break;
    case HIGH_ELEV:
      e_output = "HIGH   ";
      break;
    default:
      e_output = "UNKNOWN";
  }

  switch(twist){
    case TWIST:
      t_output = "TWIST ";
      break;
    default:
      t_output = "NONE  ";
  }

  switch(tilt){
    case STRAIGHT:
      b_output = "STRAIGHT ";
      break;
    case PARTIALLY_BENT:
      b_output = "PARTIAL  ";
      break;
    case FULLY_BENT:
      b_output = "FULL     ";
      break;
    default:
      b_output = "UNKNOWN  ";
  }
  Serial.println("LOAD: " + w_output +"   ELEV: " + e_output + "   TWIST: " + t_output + "  BACK: " + b_output + "  DURATION: " + lift_dura + "   DECISION:  " + classification);  
}

void printConsole(){  
  for (int i = 0; i < 12; ++i){
    consoleOutput += String(buf_avg[i]) + ", ";
  }
  consoleOutput = consoleOutput + String(val_12) + ", " + String(val_13);
  Serial.println(consoleOutput);
  consoleOutput = "";
}


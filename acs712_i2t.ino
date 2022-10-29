/*
 * I2t AC overload protection with ACS712
 * 
 */

#define RMS_FACTOR      0.707106
#define RMS_WINDOW      50  // ms
#define AVERAGE_SAMPLES 5.0
#define SENSITIVITY     0.130 // mv/A
#define SENSOR_BIAS     -0.05 // A
#define I_NOMINAL       3.0 // A
#define I_OVER          5.0 // A
#define T_OVER          1000 // ms
#define S_I2T           ((I_OVER * I_OVER - I_NOMINAL * I_NOMINAL) * T_OVER) // A^2ms
#define TRIP_DURATION   1000 // ms
#define A_SENSE         1  // PB2
#define A_GND           2  // P4
#define A_VCC           3  // P3
#define LED             1  // PB1
//#define LED_GREEN       2
//#define LED_YELLOW      3
//#define LED_RED         4
#define RELAY           1  // PB1


float Iavg = 0;
float Irms = 0;


void updateMovingAverage(float fValue) {
  static boolean sbInitialized = false;

  if (!sbInitialized) {
    Iavg = fValue;
    sbInitialized = true;
  }
  
  Iavg = ((AVERAGE_SAMPLES - 1.0) * Iavg + fValue) / AVERAGE_SAMPLES;
}


float measure() {
  int iRefGnd = 0; //analogRead(A_GND);
  int iRefVcc = 1023; //analogRead(A_VCC);
  int iSense  = analogRead(A_SENSE);

  float fVcc = (iRefVcc + 0.5) * 5.0 / 1024.0;
  float fSense = (iSense + 0.5) * 5.0 / 1024.0;
  
  return (fSense - fVcc / 2.0) / SENSITIVITY - SENSOR_BIAS;
}


void setup() {
  pinMode(2, INPUT); // A1 = PB2
  //pinMode(3, INPUT); // A3
  //pinMode(4, INPUT); // A0
  pinMode(LED, OUTPUT);
  //pinMode(LED_GREEN, OUTPUT);
  //pinMode(LED_YELLOW, OUTPUT);
  //pinMode(LED_RED, OUTPUT);
  pinMode(RELAY, OUTPUT);
  //Serial.begin(9600);
}


void loop() {
  static float sfImax = 0;
  static float sfLastImax = 0;
  static float sfI2t = 0;
  static unsigned long sulLastMillis = millis();
  static unsigned long sulTripStart = 0;
  //static unsigned long sulPreviousMillis = 0;

  float fI, fI2t;
  boolean bTrip, bPower;
  unsigned long ulCurrentMillis = millis();
  unsigned long ulDt = ulCurrentMillis - sulLastMillis;

  fI = measure();
  fI = abs(fI);
  updateMovingAverage(fI);

  if (Iavg > sfImax) {
    sfImax = Iavg;
  }

  // debug only
  if (ulCurrentMillis % 500 == 0) {
    //digitalWrite(RELAY, random(10) > 5 ? HIGH : LOW);
    //digitalWrite(LED, random(10) > 5 ? HIGH : LOW);
    /*
    Serial.print("Irms: ");
    Serial.println(Irms, 1);
    Serial.print("SI2t: ");
    Serial.println(S_I2T, 0);
    Serial.print("sfI2t: ");
    Serial.println(sfI2t, 0);
    Serial.println();  
  */
}
  
  if (ulDt < RMS_WINDOW) return;
  
  sulLastMillis = ulCurrentMillis;
  sfLastImax = sfImax;
  sfImax = 0;
  Irms = sfLastImax * RMS_FACTOR;
    
  fI2t = (Irms * Irms - I_NOMINAL * I_NOMINAL) * (float)ulDt;
  sfI2t += fI2t;
  if (sfI2t < 0) sfI2t = 0;

  bTrip = sfI2t > S_I2T;

  if (bTrip) {
    sulTripStart = ulCurrentMillis;
    bPower = false;
  } else {
    if (ulCurrentMillis - sulTripStart > TRIP_DURATION) {
      bPower = true;
      sulTripStart = 0;
    }
  }

  digitalWrite(RELAY, bPower ? HIGH : LOW);
  //digitalWrite(LED_GREEN, Irms > 1.0 ? HIGH : LOW);
  //digitalWrite(LED_YELLOW, Irms > 2.0 ? HIGH : LOW);
  //digitalWrite(LED_RED, bTrip ? HIGH : LOW);
  //digitalWrite(LED, Irms > 2.0 ? HIGH : LOW);
}

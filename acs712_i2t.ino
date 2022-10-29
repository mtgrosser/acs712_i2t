/*
 * I2t AC overload protection with ACS712
 * 
 */

#define RMS_FACTOR      0.707106
#define RMS_WINDOW      50  // ms
#define AVERAGE_SAMPLES 5.0
#define SENSITIVITY     0.120 // mv/A
#define SENSOR_BIAS     0.25  // A
#define I_NOMINAL       3.0 // A
#define I_OVER          5.0 // A
#define T_OVER          1000 // ms
#define S_I2T           ((I_OVER * I_OVER - I_NOMINAL * I_NOMINAL) * T_OVER) // A^2ms
#define TRIP_DURATION   1000 // ms
#define LED_GREEN       2
#define LED_YELLOW      3
#define LED_RED         4
#define RELAY           6


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
  int iRefGnd = analogRead(A0);
  int iRefVcc = analogRead(A1);
  int iSense  = analogRead(A2);

  float fVcc = (iRefVcc + 0.5) * 5.0 / 1024.0;
  float fSense = (iSense + 0.5) * 5.0 / 1024.0;
  
  return (fSense - fVcc / 2.0) / SENSITIVITY - SENSOR_BIAS;
}


void setup() {
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(RELAY, OUTPUT);
  Serial.begin(9600);
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
    Serial.print("Irms: ");
    Serial.println(Irms, 1);
    Serial.print("SI2t: ");
    Serial.println(S_I2T, 0);
    Serial.print("sfI2t: ");
    Serial.println(sfI2t, 0);
    Serial.println();  
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

  digitalWrite(RELAY, bPower ? LOW : HIGH);
  digitalWrite(LED_GREEN, Irms > 1.0 ? HIGH : LOW);
  digitalWrite(LED_YELLOW, Irms > 2.0 ? HIGH : LOW);
  digitalWrite(LED_RED, bTrip ? HIGH : LOW);
}

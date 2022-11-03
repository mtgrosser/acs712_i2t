/*
 * I2t AC overload protection with ACS712
 * 
 */

#define RMS_FACTOR       0.707106
#define RMS_WINDOW       50  // ms
#define AVERAGE_SAMPLES  5.0
#define U_REF            5000.0  // mV
#define ADC_UNIT         4.88281 // mV
#define ADC_ZERO         509
#define SENSITIVITY      170.0 // mv/A
#define I_NOMINAL        3.25  // A
#define I_OVER           5.0   // A
#define T_OVER           1000  // ms
#define S_I2T            ((I_OVER * I_OVER - I_NOMINAL * I_NOMINAL) * T_OVER) // A^2ms
#define TRIP_DURATION    1000 // ms
#define RETRIP_DURATION  100  // ms
#define RETRIP_MAX_COUNT 4
#define RETRIP_RESET     5000 // ms
#define PIN_SENSE        1  // PB2
#define PIN_LED          1  // PB1
#define PIN_RELAY        0  // PB0


float Iavg = 0;
float Irms = 0;


void updateMovingAverage(float fValue) {
  static bool sbInitialized = false;

  if (!sbInitialized) {
    Iavg = fValue;
    sbInitialized = true;
  }
  
  Iavg = ((AVERAGE_SAMPLES - 1.0) * Iavg + fValue) / AVERAGE_SAMPLES;
}


float measure() {
  int iSense = analogRead(PIN_SENSE) - ADC_ZERO;
  return (ADC_UNIT * (float)iSense) / SENSITIVITY;
}


void setup() {
  pinMode(2, INPUT); // A1 = PB2
  //pinMode(3, INPUT); // A3
  //pinMode(4, INPUT); // A0
  pinMode(PIN_RELAY, OUTPUT);
  pinMode(PIN_LED, OUTPUT);
  //Serial.begin(9600);
}


void loop() {
  static float sImax = 0;
  static float sLastImax = 0;
  static float sI2t = 0;
  static unsigned long sLastMillis = millis();
  static unsigned long sTripStart = 0;
  static unsigned long sTripReset = 0;
  static unsigned int sRetripCount = 0;

  float I, Iabs, I2t;
  boolean trip, power;
  unsigned long powerDuration = 0;
  unsigned long currentMillis = millis();
  unsigned long dt = currentMillis - sLastMillis;

  I = measure();
  updateMovingAverage(I);

  Iabs = abs(Iavg);

  if (Iabs > sImax) {
    sImax = Iabs;
  }

  // debug only
  /*
  if (ulCurrentMillis % 500 == 0) {
    Serial.println(Irms, 2);
    //digitalWrite(RELAY, random(10) > 5 ? HIGH : LOW);
    //digitalWrite(LED, random(10) > 5 ? HIGH : LOW);
    Serial.print("Irms: ");
    Serial.println(Irms, 1);
    Serial.print("SI2t: ");
    Serial.println(S_I2T, 0);
    Serial.print("sfI2t: ");
    Serial.println(sfI2t, 0);
    Serial.println();  
  }
  */
  
  if (dt < RMS_WINDOW) return;
  
  sLastMillis = currentMillis;
  sLastImax = sImax;
  sImax = 0;
  Irms = sLastImax * RMS_FACTOR;
  
  I2t = (Irms * Irms - I_NOMINAL * I_NOMINAL) * (float)dt;
  sI2t += I2t;
  if (sI2t < 0) sI2t = 0;

  trip = sI2t > S_I2T;
  powerDuration = currentMillis - sTripReset;

  if (trip) {
    sTripStart = currentMillis;
    power = false;
    if (powerDuration < RETRIP_DURATION && sRetripCount < RETRIP_MAX_COUNT)
      sRetripCount++;
  } else {
    if (currentMillis - sTripStart > pow(2, sRetripCount) * TRIP_DURATION) {
      power = true;
      if (powerDuration > RETRIP_RESET) sRetripCount = 0;
      sTripReset = currentMillis;
      sTripStart = 0;
    }
  }
  
  digitalWrite(PIN_LED, power ? HIGH : LOW);
  digitalWrite(PIN_RELAY, power ? HIGH : LOW);
}

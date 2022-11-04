#define AVERAGE_SAMPLES   5.0
#define ADC_UNIT          4.88281 // mV
#define ADC_ZERO          512.2
#define SENSITIVITY       185.0 // mv/A
#define CORRECTION_FACTOR 0.9
#define I_NOMINAL         3.25  // A
#define I_OVER            5.0   // A
#define T_OVER            500  // ms
#define S_I2T             ((I_OVER * I_OVER - I_NOMINAL * I_NOMINAL) * T_OVER) // A^2ms
#define TRIP_DURATION     2000 // ms
#define RETRIP_DURATION   100  // ms
#define RETRIP_MAX_COUNT  4
#define RETRIP_RESET      5000 // ms
#define PIN_SENSE         1  // PB2
#define PIN_LED           1  // PB1
#define PIN_RELAY         0  // PB0


float Irms = 0;
float Iavg = 0;


float measure() {
  int adc = analogRead(PIN_SENSE);
  return (ADC_UNIT * ((float)adc - ADC_ZERO)) / SENSITIVITY;
}


void updateMovingAverage(float value) {
  static bool initialized = false;

  if (!initialized) {
    Iavg = value;
    initialized = true;
  }

  Iavg = ((AVERAGE_SAMPLES - 1.0) * Iavg + value) / AVERAGE_SAMPLES;
}


void setup() {
  pinMode(2, INPUT); // A1 = PB2
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_RELAY, OUTPUT);
  //Serial.begin(9600);
}


void loop() {
  static unsigned long lastMicros = micros();
  static unsigned long lastMillis = millis();
  static unsigned long lastTrip = 0;
  static unsigned long lastReset = 0;
  static float sumI2 = 0;
  static float sumI2t = 0;
  static int idx = 0;
  static int tripCount = 0;
  static bool power = true;

  unsigned long currentMicros = micros();
  unsigned long currentMillis, dt;

  float I2t;
  bool trip;
  
  if (currentMicros - lastMicros < 500) return; 
  
  // every 0.5 ms -------------------------------
  lastMicros = micros();

  float Isense = measure();
  sumI2 = sumI2 + Isense * Isense / 40.0;
  idx++;

  if (idx < 40) return; 
  
  // every 20 ms - one period at 50 Hz ----------
  currentMillis = millis();
  idx = 0;
  Irms = sqrt(sumI2) * CORRECTION_FACTOR;
  updateMovingAverage(Irms);

  sumI2 = 0;
  dt = currentMillis - lastMillis;
  lastMillis = currentMillis;

  I2t = (Iavg * Iavg - I_NOMINAL * I_NOMINAL) * (float)dt;

  sumI2t += I2t;
  if (sumI2t < 0) sumI2t = 0;

  trip = sumI2t > S_I2T;

  if (trip) {
    if (power) {
      if (currentMillis - lastReset < RETRIP_DURATION && tripCount < RETRIP_MAX_COUNT) 
        tripCount++;
      lastTrip = currentMillis;
    }
    power = false;
  } else {
    if (!power && currentMillis - lastTrip > pow(1.5, tripCount) * TRIP_DURATION) {
      power = true;
      lastReset = currentMillis;
      sumI2t = S_I2T;
    }
    if (power && currentMillis - lastReset > RETRIP_RESET) tripCount = 0;
  }

  digitalWrite(PIN_LED, I2t > 0 ? HIGH : LOW);
  digitalWrite(PIN_RELAY, power ? HIGH : LOW);
}

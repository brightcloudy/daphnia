// -----------------------------------------
//  TEMPERATURE CONTROL LOOP
// -----------------------------------------
//
// GND ---- power supply ---- arduino pin ---- relay in ---- heat sink --- DS18B20
// D2   (arduino) -->-- IN1 (relay)
// 5V   (arduino) -->-- VCC (relay in)
// D3   (arduino) -->-- data pin (DS18B20)
// 3.3V (arduino) -->-- VCC (DS18B20)
// J1-1 (relay)   -->-- VCC (heat sink)
// J1-3 (relay)   -->-- VCC  (power supply)
//
// -----------------------------------------
// Average readings from a DS18B20 1-Wire temperature sensor and actuate a peltier coil.
// Bang-bang control with hysteresis.

#include <stdio.h>

#include <OneWire.h>
#include <DallasTemperature.h>

const int SERIAL_BAUD = 9600; // Serial port speed in baud

const uint8_t ONE_WIRE_BUS = 3; // use port D3 for 1Wire data bus
const uint8_t PELTIER_OUT = 2; // peltier output to relay on port D2

const uint8_t TEMP_FORMAT_WIDTH = 5; // temperature format specifier width
const uint8_t TEMP_FORMAT_PREC = 2; // temperature format specifier precision

const uint8_t N_SAMPLES = 5; // number of temperature samples to average

// resolutions available are 9-bit to 12-bit, with varying conversion times
// 9-bit: 93.75 ms, 10-bit: 187.5 ms, 11-bit: 375 ms, 12-bit: 750 ms
const uint8_t TEMP_RESOLUTION = 10; // number of bits in temperature conversion
const float TCONV = (93.75 * (1 << (12 - TEMP_RESOLUTION))); // conversion time, ms

// Controlling TIME
const int LOOP_DELAY = 500; // ms

// We want the algae to stay at a certain temperature
// loop temperature setpoint
const float TEMP_SETPOINT = 10.0; // °C

// store temperature tolerance for implementing hysteresis
const float TEMP_HYSTERESIS = 0.5; // °C

// file struct for printf redirection
static FILE uart = {0};

// Set aside some memory for averaging samples
float sum;
float temperature;

// variable to track duration of peltier on and off-time
uint32_t peltier_duration = 0; // ms

// OneWire instance for the OneWire bus on ONE_WIRE_BUS pin
OneWire oneWire(ONE_WIRE_BUS);
// DallasTemperature module instance
DallasTemperature sensors(&oneWire);
// Device address for the main sensor
DeviceAddress mainTemp;

static int uart_putchar (char c, FILE *stream)
{
    Serial.write(c);
    return 0;
}

void setup() {
  pinMode(PELTIER_OUT, OUTPUT);
  
  Serial.begin(SERIAL_BAUD);
  
  // set up stdout to uart for printf
  fdev_setup_stream(&uart, uart_putchar, NULL, _FDEV_SETUP_WRITE);
  stdout = &uart;

  // locate temperature sensor on the bus
  sensors.begin();
  printf("-DEBUG- Found %d devices.\n", sensors.getDeviceCount());
  printf("-DEBUG- Parasitic power: %s.\n", (sensors.isParasitePowerMode() ? "ON" : "OFF"));

  // assign first (only?) temperature sensor on the bus to the mainTemp address
  oneWire.reset_search();
  if (!oneWire.search(mainTemp)) printf("-DEBUG- Unable to find sensor address!\n");

  // print 64-bit located sensor address
  printf("-DEBUG- Device address: ");
  for (int i = 0; i < 8; i++) {
    printf("%02X", mainTemp[i]);
  }
  printf("\n");

  // set sensor resolution
  // see definition of TEMP_RESOLUTION for more information
  sensors.setResolution(mainTemp, TEMP_RESOLUTION);
  printf("-DEBUG- Device resolution: %d\n", sensors.getResolution(mainTemp));
}

void loop() {
  // Average many samples
  sum = 0;
  for (int n = 0; n < N_SAMPLES; n++) {
    sensors.requestTemperaturesByAddress(mainTemp);
    sum += sensors.getTempC(mainTemp);
    peltier_duration += (uint16_t) (TCONV+0.5); // round non-integer conversion times
  }
  
  temperature = sum / N_SAMPLES;

  char temp_string[6];
  dtostrf(temperature, TEMP_FORMAT_WIDTH, TEMP_FORMAT_PREC, temp_string); // alternative function to properly format temperature strings

  printf("%s\n", temp_string);

  if (temperature > (TEMP_SETPOINT + TEMP_HYSTERESIS)) {
    if (digitalRead(PELTIER_OUT) == HIGH) {
      digitalWrite(PELTIER_OUT, LOW);
      printf("-DEBUG- Peltiers ON: off for %lu milliseconds\n", peltier_duration);
      peltier_duration = 0;
    }
  }
  else if (temperature < (TEMP_SETPOINT - TEMP_HYSTERESIS)) {
    if (digitalRead(PELTIER_OUT) == LOW) {
      digitalWrite(PELTIER_OUT, HIGH);
      printf("-DEBUG- Peltiers OFF: on for %lu milliseconds\n", peltier_duration);
      peltier_duration = 0;
    }
  }
  
  delay(LOOP_DELAY);
  peltier_duration += LOOP_DELAY;
}


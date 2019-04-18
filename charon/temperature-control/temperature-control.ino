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

#define ONE_WIRE_BUS 3 // use port D3 for 1Wire data bus
#define PELTIER_OUT 2 // peltier output to relay on port D2

#define TEMP_FORMAT_WIDTH 5 // temperature format specifier width
#define TEMP_FORMAT_PREC 2 // temperature format specifier precision

#define N_SAMPLES 5 // number of temperature samples to average

// Controlling TIME
#define LOOP_DELAY 500 // ms

// We want the algae to stay at a certain temperature
// loop temperature setpoint
#define TEMP_SETPOINT 10.0 // °C

// store temperature tolerance for implementing hysteresis
#define TEMP_HYSTERESIS 0.5 // °C

// file struct for printf redirection
static FILE uart = {0};

// Set aside some memory for averaging samples
int sum;
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
    Serial.write(c) ;
    return 0 ;
}

void setup() {
  pinMode(PELTIER_OUT, OUTPUT);
  
  Serial.begin(9600);
  
  // set up stdout to uart for printf
  fdev_setup_stream(&uart, uart_putchar, NULL, _FDEV_SETUP_WRITE);
  stdout = &uart;
}

void loop() {
  // Average many samples
  sum = 0;
  for (int n = 0; n < N_SAMPLES; n++) {
    sum += analogRead(sensorPin);
    delay(SAMPLE_FREQUENCY);
    peltier_duration += SAMPLE_FREQUENCY;
  }
  
  temperature = readingToCelsius((sum / N_SAMPLES));

  char temp_string[6];
  dtostrf(temperature, TEMP_FORMAT_WIDTH, TEMP_FORMAT_PREC, temp_string); // alternative function to properly format temperature strings

  printf("%s\n", temp_string);

  if (temperature > (setpoint + setpoint_hysteresis)) {
    if (digitalRead(PELTIER_OUT) == HIGH) {
      digitalWrite(PELTIER_OUT, LOW);
      printf("Peltiers ON: off for %d milliseconds\n", peltier_duration);
      peltier_duration = 0;
    }
  }
  else if (temperature < (setpoint - setpoint_hysteresis)) {
    if (digitalRead(PELTIER_OUT) == LOW) {
      digitalWrite(PELTIER_OUT, HIGH);
      printf("Peltiers OFF: on for %d milliseconds\n", peltier_duration);
      peltier_duration = 0;
    }
  }
  
  delay(LOOP_DELAY);
  peltier_duration += LOOP_DELAY;
}


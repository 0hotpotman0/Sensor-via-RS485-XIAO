#include <ArduinoRS485.h> // ArduinoModbus depends on the ArduinoRS485 library
#include <ArduinoModbus.h>
#include <Arduino.h>
#include <SensirionI2CScd4x.h>
#include <Wire.h>

SensirionI2CScd4x scd4x;

uint16_t co2 = 0;
float temperature = 0.0f;
float humidity = 0.0f;
bool isDataReady = false;
int RS485_tx_EN = D8;

void printUint16Hex(uint16_t value) {
  Serial.print(value < 4096 ? "0" : "");
  Serial.print(value < 256 ? "0" : "");
  Serial.print(value < 16 ? "0" : "");
  Serial.print(value, HEX);
}

void printSerialNumber(uint16_t serial0, uint16_t serial1, uint16_t serial2) {
  Serial.print("Serial: 0x");
  printUint16Hex(serial0);
  printUint16Hex(serial1);
  printUint16Hex(serial2);
  Serial.println();
}

void setup() {
  Serial.begin(9600);
  while (!Serial);

  // start the Modbus RTU server, with (slave) id 1
  if (!ModbusRTUServer.begin(1, 9600)) {
    Serial.println("Failed to start Modbus RTU Client!");
    while (1);
  }

  pinMode(RS485_tx_EN, OUTPUT);
  digitalWrite(RS485_tx_EN, LOW);  // enable RS485 comunicate

  ModbusRTUServer.configureHoldingRegisters(0x01, 1);
  ModbusRTUServer.configureHoldingRegisters(0x03, 1);
  ModbusRTUServer.configureInputRegisters(0x04, 1);
  ModbusRTUServer.configureInputRegisters(0x06, 1);

  ///////////////////////////////////////////////////////////////////////////
  Wire.begin();

  uint16_t error;
  char errorMessage[256];
  scd4x.begin(Wire);
  error = scd4x.stopPeriodicMeasurement();
  if (error) {
    Serial.print("Error trying to execute stopPeriodicMeasurement(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  }
  uint16_t serial0;
  uint16_t serial1;
  uint16_t serial2;

  error = scd4x.getSerialNumber(serial0, serial1, serial2);
  if (error) {
    Serial.print("Error trying to execute getSerialNumber(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  } else {
    printSerialNumber(serial0, serial1, serial2);
  }

  // Start Measurement
  error = scd4x.startPeriodicMeasurement();
  if (error) {
    Serial.print("Error trying to execute startPeriodicMeasurement(): ");
    errorToString(error, errorMessage, 256);
    Serial.println(errorMessage);
  }

  error = scd4x.getDataReadyFlag(isDataReady);

  Serial.println("Testing");
  ///////////////////////////////////////////////////////////////////////////

}

void loop() {
  scd4x.readMeasurement(co2, temperature, humidity);
  delay(1000);
  int flag_re = ModbusRTUServer.poll();
  if (flag_re > 0){
    digitalWrite(RS485_tx_EN, HIGH);
    delay(1);
    }
  ModbusRTUServer.reply(flag_re);

  Serial.println("*****************");
  Serial.print("Co2:");
  Serial.println(co2);
  Serial.print("Temperature:");
  Serial.println(temperature);
  Serial.print("Humidity:");
  Serial.println(humidity);

  Serial.println("*****************");

  ModbusRTUServer.holdingRegisterWrite(0x01, 0x01);
  ModbusRTUServer.holdingRegisterWrite(0x03, co2);
  ModbusRTUServer.inputRegisterWrite(0x04, temperature);
  ModbusRTUServer.inputRegisterWrite(0x06, humidity);

  digitalWrite(RS485_tx_EN, LOW);
}

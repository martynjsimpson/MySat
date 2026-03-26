#include <BQ24195.h>

float rawADC;           //unprocessed ADC value
float voltADC;          //ADC converted into voltage
float voltBatt;          //calculated voltage on battery
int battPerc;

int R1 =  330000;       // resistor between battery terminal and SAMD pin PB09
int R2 = 1000000;       // resistor between SAMD pin PB09 and ground

int max_Source_voltage; // upper source voltage for the battery

// define voltage at which battery is full/empty
float batteryFullVoltage = 4.2;   //upper voltage limit for battery
float batteryEmptyVoltage = 2.75;  //lower voltage limit for battery

float batteryCapacity = 4.0;            //set battery capacity in Ah

void setupBattery (){
  analogReference(AR_DEFAULT);      // the upper value of ADC is set to 3.3V
  analogReadResolution(12);         // this will give us 4096 (2^12) levels on the ADC

  PMIC.begin();                                               // start the PMIC I2C connection
  //PMIC.enableBoostMode();                                     // boost battery output to 5V
  PMIC.setMinimumSystemVoltage(batteryEmptyVoltage);          // set the minimum battery output
  PMIC.setChargeVoltage(batteryFullVoltage);                  // set battery voltage at full charge
  //PMIC.setChargeCurrent(batteryCapacity/2);                   // set battery current to C/2 in amps
  PMIC.setChargeCurrent(0.5);                   // set battery current to C/2 in amps

  PMIC.enableCharge();                                        // enable charging of battery

  // The formula for calculating the output of a voltage divider is Vout = (Vsource x R2)/(R1 + R2)
  // If we consider that 3.3V is the maximum that can be applied to Vout then the maximum source voltage is calculated as
  max_Source_voltage = (3.3 * (R1 + R2))/R2;
}

void updateBatteryValues(){
  rawADC = analogRead(ADC_BATTERY);                     //the value obtained directly at the PB09 input pin
  voltADC = rawADC * (3.3/4095.0);                      //convert ADC value to the voltage read at the pin
  voltBatt = voltADC * (max_Source_voltage/3.3);         //we cannot use map since it requires int inputs/outputs

  battPerc = (voltBatt - batteryEmptyVoltage) * (100) / (batteryFullVoltage - batteryEmptyVoltage);    //custom float friendly map function
}

void reportBatteryStatus() {
  updateBatteryValues();
  //Serial.println(PMIC.isBattConnected());
  sendTelemetry("BATTERY", "AVAILABLE", PMIC.isBattConnected() ? "TRUE" : "FALSE");
  sendTelemetry("BATTERY", "ON_BATTERY", PMIC.canRunOnBattery() ? "TRUE" : "FALSE");
  sendTelemetryFloat("BATTERY", "CHARGE_CURRENT_A", PMIC.getChargeCurrent(), 3);
  sendTelemetryFloat("BATTERY", "CHARGE_VOLTAGE_A", PMIC.getChargeVoltage(), 3);
  sendTelemetryFloat("BATTERY", "CHARGE_PERCENTAGE_P", battPerc, 2);

}
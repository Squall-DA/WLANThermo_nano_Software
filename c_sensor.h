 /*************************************************** 
    Copyright (C) 2016  Steffen Ochs

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    ANALOG/DIGITAL-WANDLUNG:
    - kleinster digitaler Sprung 1.06 V/1024 = 1.035 mV - eigentlich 1.0V/1024
    - Hinweis zur Abweichung: https://github.com/esp8266/Arduino/issues/2672
    -> ADC-Messspannung = Digitalwert * 1.035 mV
    - Spannungsteiler (47k / 10k) am ADC-Eingang zur 
    - Transformation von BattMin - BattMax in den Messbereich von 0 - 1.06V 
    -> Batteriespannung = ADC-Messspannung * (47+10)/10 
    -> Transformationsvariable Digitalwert-to-Batteriespannung: Battdiv = 1.035 mV * 5.7
    
    HISTORY: Please refer Github History
    
 ****************************************************/

// https://github.com/adafruit/Adafruit_HTU21DF_Library/blob/master/Adafruit_HTU21DF.cpp

#define HTU21DF_I2CADDR       0x40
#define HTU21DF_READTEMP      0xE3
#define HTU21DF_READHUM       0xE5
#define HTU21DF_WRITEREG      0xE6
#define HTU21DF_READREG       0xE7
#define HTU21DF_RESET         0xFE
#define TRIG_TEMP_RLS         0xF3 //Triggers a Temperature Measurement. Releases the SCK line (unblocks i2c bus). User must manually wait for completion before grabbing data.
#define TRIG_HUM_RLS          0xF5 //Triggers a Humidity Measurement. Releases the SCK line (unblocks i2c bus). User must manually wait for completion before grabbing data.

class HTU21DF {

  private:

    bool _exist;
    byte _state;
    float _temp, _hum;

    void wireRead(byte x) {
      Wire.beginTransmission(HTU21DF_I2CADDR);
      Wire.write(x);
      Wire.endTransmission();
      //delay(15);
    }

    uint16_t wireReq() {
      Wire.requestFrom(HTU21DF_I2CADDR, 3);
      uint16_t h = (Wire.read() << 8) | Wire.read();
      Wire.read();
      return h;
    }
  
  public:

    bool exist() {
      return _exist;
    }

    byte getState() {
      return _state;
    }

    float temp() {
      return _temp;
    }

    float hum() {
      return _hum;
    }

    boolean begin(void) {
      wireRead(HTU21DF_RESET);
      wireRead(HTU21DF_READREG);
      Wire.requestFrom(HTU21DF_I2CADDR, 1);
      _exist = (Wire.read() == 0x2); // after reset should be 0x2
      _state = 0;
      return _exist;
    }

    void trigTemperature() {
      wireRead(TRIG_TEMP_RLS);
      _state = 1;
    }

    void trigHumidity() {
      wireRead(TRIG_HUM_RLS);
      _state = 3;
    }
    
    void readTemperature(void) {
      //wireRead(HTU21DF_READTEMP); delay(50);
      float temp = wireReq();
      temp *= 175.72;
      temp /= 65536;
      temp -= 46.85;
      _state = 2;
      _temp = temp;
    }
    
    float readHumidity(void) {
      //wireRead(HTU21DF_READHUM); delay(50);
      float hum = wireReq();
      hum *= 125;
      hum /= 65536;
      hum -= 6;
      _state = 0;
      _hum = hum;
    }
};

HTU21DF htu;


/*
#define MAX17043_ADDRESS  0x36

class MAX17043 {

  // Quelle: https://github.com/lucadentella/ArduinoLib_MAX17043/blob/master/examples/VoltageSoC/VoltageSoC.ino

  private:

    void readRegister(byte address, byte &MSB, byte &LSB) {

      Wire.beginTransmission(MAX17043_ADDRESS);
      Wire.write(address);
      Wire.endTransmission();
  
      Wire.requestFrom(MAX17043_ADDRESS, 2);
      MSB = Wire.read();
      LSB = Wire.read();
    }

    void writeRegister(byte address, byte MSB, byte LSB) {
      Wire.beginTransmission(MAX17043_ADDRESS);
      Wire.write(address);
      Wire.write(MSB);
      Wire.write(LSB);
      Wire.endTransmission();
    }

    void readConfigRegister(byte &MSB, byte &LSB) {
      readRegister(0x0C, MSB, LSB);
    }

    uint16_t read16(uint8_t address)  {
      uint8_t msb, lsb;
      int16_t timeout = 1000;

      Wire.beginTransmission(MAX17043_ADDRESS);
      Wire.write(address);
      Wire.endTransmission(false);

      Wire.requestFrom(MAX17043_ADDRESS, 2);
      while ((Wire.available() < 2) && (timeout-- > 0))
        delay(1);
      msb = Wire.read();
      lsb = Wire.read();

      return ((uint16_t) msb << 8) | lsb;
    }


  public:

    float getVoltage()  {
      uint16_t vCell;
      vCell = read16(0x02);
      // vCell is a 12-bit register where each bit represents 1.25mV
      vCell = (vCell) >> 4;

      return ((float) vCell / 800.0);

    }
  
    float getVCell() {

      byte MSB = 0;
      byte LSB = 0;
  
      readRegister(0x02, MSB, LSB);
      int value = (MSB << 4) | (LSB >> 4);
      return map(value, 0x000, 0xFFF, 0, 50000) / 10000.0;
      //return value * 0.00125;

    
      //temp = ((xl|(xm << 8)) >> 4);
      //xo = 1.25* temp;
    }

    float getSoC() {
  
      byte MSB = 0;
      byte LSB = 0;
  
      readRegister(0x04, MSB, LSB);
      float decimal = LSB / 256.0;
      return MSB + decimal; 
    }

    void reset() {
  
      writeRegister(0xFE, 0x00, 0x54);
    }

    void quickStart() {
  
      writeRegister(0x06, 0x40, 0x00);
    }

    int getVersion() {

      byte MSB = 0;
      byte LSB = 0;
      readRegister(0x08, MSB, LSB);
      return (MSB << 8) | LSB;
    }

    byte getCompensateValue() {

      byte MSB = 0;
      byte LSB = 0;
  
      readRegister(0x0C, MSB, LSB);
      return MSB;
    }

    byte getAlertThreshold() {

      byte MSB = 0;
      byte LSB = 0;
  
      readRegister(0x0C, MSB, LSB); 
      return 32 - (LSB & 0x1F);
    }

    void setAlertThreshold(byte threshold) {

      byte MSB = 0;
      byte LSB = 0;
  
      readRegister(0x0C, MSB, LSB); 
      if(threshold > 32) threshold = 32;
      threshold = 32 - threshold;
  
      writeRegister(0x0C, MSB, (LSB & 0xE0) | threshold);
    }

    boolean inAlert() {

      byte MSB = 0;
      byte LSB = 0;
  
      readRegister(0x0C, MSB, LSB); 
      return LSB & 0x20;
    }

    void clearAlert() {

      byte MSB = 0;
      byte LSB = 0;
  
      readRegister(0x0C, MSB, LSB); 
    }
};

MAX17043 batteryMonitor;
*/


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Initialize Sensors
byte set_sensor() {

  // Piepser
  pinMode(MOSI, OUTPUT);
  analogWriteFreq(4000);

  
  if (sys.typk && sys.hwversion == 1) {
    // CS notwendig, da nur bei CS HIGH neue Werte im Chip gelesen werden
    pinMode(THERMOCOUPLE_CS, OUTPUT);
    digitalWrite(THERMOCOUPLE_CS, HIGH);
  }

  /*
  batteryMonitor.reset();
  batteryMonitor.quickStart();

  Serial.print("MAX17043-Version:\t\t");
  Serial.println(batteryMonitor.getVersion());
  */

  if (htu.begin()) Serial.println("Found HTU21D");
  else Serial.println("No HTU21D");
  

  // MAX1161x
  byte reg = 0xA0;    // A0 = 10100000
  // page 14
  // 1: setup mode
  // SEL2:0 = Reference (Table 6)
  // external(1)/internal(0) clock
  // unipolar(0)/bipolar(1)
  // 0: reset the configuration register to default
  // 0: dont't care
 
  Wire.beginTransmission(MAX1161x_ADDRESS);
  Wire.write(reg);
  byte error = Wire.endTransmission();
  return error;
  
}


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Reading ADC-Channel Average
int get_adc_average (byte ch) {  
  // Get the average value for the ADC channel (ch) selected. 
  // MAX11613/15 samples the channel 8 times and returns the average.  
  // Setup byte required: 0xA0 
  
  byte config = 0x21 + (ch << 1);   //00100001 + ch  // page 15 
  // 0: config mode
  // 01: SCAN = Converts the ch eight times
  // 0000: placeholder ch
  // 1: single-ended

  Wire.beginTransmission(MAX1161x_ADDRESS);
  Wire.write(config);  
  Wire.endTransmission();
  
  Wire.requestFrom(MAX1161x_ADDRESS, 2);
  word regdata = (Wire.read() << 8) | Wire.read();

  return regdata & 4095;
}


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Initialize Charge Detection
void set_batdetect(boolean stat) {

  if (!stat)  pinMode(CHARGEDETECTION, INPUT_PULLDOWN_16);
  else pinMode(CHARGEDETECTION, INPUT);
}


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Reading Battery Voltage
void get_Vbat() {
  
  // Digitalwert transformiert in Batteriespannung in mV
  int voltage = analogRead(ANALOGREADBATTPIN);

  // CHARGE DETECTION
  //                LOAD        COMPLETE        SHUTDOWN
  // MCP:           LOW           HIGH           HIGH-Z 
  // curStateNone:  LOW           HIGH           HIGH
  // curStatePull:  LOW           HIGH           LOW
  //                0             3              1
  
  // Messung bei INPUT_PULLDOWN
  set_batdetect(LOW);
  bool curStatePull = digitalRead(CHARGEDETECTION);
  // Messung bei INPUT
  set_batdetect(HIGH);
  bool curStateNone = digitalRead(CHARGEDETECTION);
  // Ladeanzeige
  battery.charge = !curStateNone;
  
  // Standby erkennen
  if (voltage < 10) {
    sys.stby = true;
    //return;
  }
  else sys.stby = false;

  // Transformation Digitalwert in Batteriespannung
  voltage = voltage * BATTDIV; 

  battery.state = curStateNone;
  battery.state |= curStatePull<<1;

  if (sys.god & (1<<1)) battery.state = 4;                   // Abschaltung der Erkennung

  switch (battery.state) {

    case 0:                                                    // LOAD
      if (battery.setreference != -1 && battery.setreference < 180) {                        // Referenz (neu) setzen
        battery.setreference = -1;
        setconfig(eSYSTEM,{});
      }
      break;

    case 1:                                                    // SHUTDOWN
      if (battery.setreference > 0 && battery.voltage > 0 && !sys.stby) {
        voltage = 4200;

        // Runterzählen
        if ((millis() - battery.correction) > CORRECTIONTIME) { 
          battery.setreference -= CORRECTIONTIME/1000;
          battery.setreference = constrain(battery.setreference, 0, 180);
          setconfig(eSYSTEM,{});                                  // SPEICHERN
          battery.correction = millis();
        }
      }
      break;

    case 3:                                                    // COMPLETE (vollständig)
      if (battery.setreference == -1) {                        // es wurde geladen
        battery.setreference = 180;                            // Referenzzeit setzen
        setconfig(eSYSTEM,{});
      }
      break;

    case 4:                                                     // NO BATTERY
      battery.voltage = 4200;
      battery.charge = false;
      break;
  }

  // Batteriespannung wird in einen Buffer geschrieben da die gemessene
  // Spannung leicht schwankt, aufgrund des aktuellen Energieverbrauchs
  // wird die Batteriespannung als Mittel aus mehreren Messungen ausgegeben
  // Während der Battery Initalisierung wird nicht in den Buffer geschrieben

  if (millis() < BATTERYSTARTUP*2) {
    vol_sum = voltage;                        // Battery Initalisierung
    vol_count = 1;
  } else {
    vol_sum += voltage;
    vol_count++;
  }
  
}


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Calculate SOC
void cal_soc() {

  // mittlere Batteriespannung aus dem Buffer lesen und in Prozent umrechnen
  int voltage;
  
  if (vol_count > 0) {

    if (vol_count == 1) {                           // Battery Initialiseurnv
      if (battery.voltage < vol_sum)
        battery.voltage = vol_sum;                  // beim Start Messung anpassen
    } else {
      voltage = vol_sum / vol_count;
      median_add(voltage);
  
      //battery.voltage = voltage;
      battery.voltage = median_average(); 
    }
    
    battery.percentage = ((battery.voltage - battery.min)*100)/(battery.max - battery.min);
  
    // Schwankungen verschiedener Batterien ausgleichen
    if (battery.percentage > 100) {
      if (battery.charge) battery.percentage = 99;
      else battery.percentage = 100;
    }

    if (millis() > BATTERYSTARTUP) {
      IPRINTF("Battery voltage: %umV,\tcharge: %u%%\r\n", battery.voltage, battery.percentage); 
    }

    // Abschaltung des Systems bei <0% Akkuleistung
    if (battery.percentage < 0) {
      display.clear();
      display.setFont(ArialMT_Plain_10);
      display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
      display.drawString(DISPLAY_WIDTH/2, DISPLAY_HEIGHT/3, "LOW BATTERY");
      display.drawString(DISPLAY_WIDTH/2, 2*DISPLAY_HEIGHT/3, "PLEASE SWITCH OFF");
      display.display();
      ESP.deepSleep(0);
      delay(100); // notwendig um Prozesse zu beenden
    }

    vol_sum = 0;
    vol_count = 0;

  }

  // HTU21D
  if (htu.exist()) {
    switch (htu.getState()) {
      case 0: htu.trigTemperature(); break;
      case 1: htu.readTemperature();
      case 2: htu.trigHumidity(); break;
      case 3: htu.readHumidity(); break;
    } 
  }
    
  /*
  float cellVoltage = batteryMonitor.getVCell();
  Serial.print("Voltage:\t\t");
  Serial.print(cellVoltage, 4);
  Serial.print(" | ");
  cellVoltage = batteryMonitor.getVoltage();
  Serial.print(cellVoltage, 4);
  Serial.println("V");

  float stateOfCharge = batteryMonitor.getSoC();
  Serial.print("State of charge:\t");
  Serial.print(stateOfCharge);
  Serial.println("%");
  */

}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Initialize Hardware Alarm
void set_piepser() {

  // Hardware-Alarm bereit machen
  pinMode(MOSI, OUTPUT);
  analogWriteFreq(4000);
  //sys.hwalarm = false;
  
}

void piepserON() {
  analogWrite(MOSI,512);
}

void piepserOFF() {
  analogWrite(MOSI,0);
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++
//Control Hardware Alarm
void controlAlarm(bool action){                // action dient zur Pulsung des Signals

  bool setalarm = false;

  for (int i=0; i < CHANNELS; i++) {
    //if (ch[i].alarm > 0) {                              // CHANNEL ALARM ENABLED
                
      // CHECK LIMITS
      if ((ch[i].temp <= ch[i].max && ch[i].temp >= ch[i].min) || ch[i].temp == INACTIVEVALUE) {
        // everything is ok
        ch[i].isalarm = false;                      // no alarm
        ch[i].showalarm = 0;                    // no OLED note
        notification.index &= ~(1<<i);              // delete channel from index
        //notification.limit &= ~(1<<i);

      } else if (ch[i].isalarm && ch[i].showalarm > 0) {  // Alarm noch nicht quittiert
        // do summer alarm
        setalarm = true;                            

        // Show Alarm on OLED
        if (ch[i].showalarm == 2 && !displayblocked) {    // falls noch nicht angezeigt
          ch[i].showalarm = 1;                            // nur noch Summer
          question.typ = HARDWAREALARM;
          question.con = i;
          drawQuestion(i);
        }
      
      } else if (!ch[i].isalarm && ch[i].temp != INACTIVEVALUE) {
        // first rising limits

        ch[i].isalarm = true;                      // alarm

        if (ch[i].alarm == 1 || ch[i].alarm == 3) {   // push or all
          notification.index &= ~(1<<i);
          notification.index |= 1<<i;                // Add channel to index   
        
          if (ch[i].temp > ch[i].max) {
            notification.limit |= 1<<i;              // add upper limit
          }
          else if (ch[i].temp < ch[i].min) { 
            notification.limit &= ~(1<<i);           // add lower limit              
          }
        } 
        
        if (ch[i].alarm > 1) {                       // only if summer
          ch[i].showalarm = 2;                    // show OLED note first time
        }
      }
    //} else {                                      // CHANNEL ALARM DISABLED
    //  ch[i].isalarm = false;
    //  ch[i].showalarm = 0;
    //  notification.index &= ~(1<<i);              // delete channel from index
      //notification.limit &= ~(1<<i);
    //}   
  }

  // Hardware-Alarm-Variable: sys.hwalarm
  if (setalarm && action) {
    piepserON();
  }
  else {
    piepserOFF();
  }  
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Reading Ampere IC
unsigned long ampere_sum = 0;
unsigned long ampere_con = 0;
float ampere = 0;
unsigned long ampere_time;

void ampere_control() {
    
    ampere_sum += ((get_adc_average(5) * 2.048 )/ 4096.0)*1000.0;
    ampere_con++;

    if (millis()-ampere_time > 10*60*1000) {
      ampere_time = millis();
      ampere = ampere_sum/ampere_con;
      ampere_con = 0;
      ampere_sum = 0;
    } else if (millis() < 120000) {
      ampere = ampere_sum/ampere_con;
    }
}


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Reading Temperature KTYPE
double get_thermocouple(bool internal) {

  long dd = 0;
  
  // Communication per I2C Pins but with CS
  digitalWrite(THERMOCOUPLE_CS, LOW);                    // START
  for (uint8_t i=32; i; i--){
    dd = dd <<1;
    if (twi_read_bit())  dd |= 0x01;
  }
  digitalWrite(THERMOCOUPLE_CS, HIGH);                   // END

  // Invalid Measurement
  if (dd & 0x7) {             // Abfrage von D0/D1/D2 (Fault)
    return INACTIVEVALUE; 
  }

  if (internal) {
    // Internal Reference
    double ii = (dd >> 4) & 0x7FF;     // Abfrage D4-D14
    ii *= 0.0625;
    if ((dd >> 4) & 0x800) ii *= -1;  // Abfrage D15 (Vorzeichen)
    return ii;
  }
  

  // Temperatur
  if (dd & 0x80000000) {    // Abfrage D31 (Vorzeichen)
    // Negative value, drop the lower 18 bits and explicitly extend sign bits.
    dd = 0xFFFFC000 | ((dd >> 18) & 0x00003FFFF);
  }
  else {
    // Positive value, just drop the lower 18 bits.
    dd >>= 18;
  }

  // Temperature in Celsius
  double vv = dd;
  vv *= 0.25;
  return vv;
}



#include "Pzem004t_V3.h"

byte getValue_para[8] = {0xf8, 0x04, 0x00, 0x00, 0x00, 0x0a, 0x64, 0x64};

byte resetEnergy_para[4] = {0xf8, 0x42, 0xc2, 0x41};

 Pzem004t_V3::Pzem004t_V3() {
  Serial.begin(9600);
  port = &Serial;
  typeSerial = HARD_SERIAL;
  setTimeout(MIN_PZEM_TIMEOUT);
}

Pzem004t_V3::Pzem004t_V3(HardwareSerial * serial) {
//  serial->begin(9600);
  port = serial;
  typeSerial = HARD_SERIAL;
  setTimeout(MIN_PZEM_TIMEOUT);
}

Pzem004t_V3::Pzem004t_V3(SoftwareSerial * serial) {
//  serial->begin(9600);
  port = serial;
  typeSerial = SOFT_SERIAL;
  setTimeout(MIN_PZEM_TIMEOUT);
}

Pzem004t_V3::Pzem004t_V3(int rxPin, int txPin) {
  SoftwareSerial * yy = new SoftwareSerial(rxPin, txPin);
//  yy->begin(9600);
  port = yy;
  typeSerial = SOFT_SERIAL;
  setTimeout(MIN_PZEM_TIMEOUT);
}

void Pzem004t_V3::begin(unsigned long _tembaud) {
  if (typeSerial == HARD_SERIAL) {
    HardwareSerial * tt = (HardwareSerial *)port;
    tt->begin(_tembaud);
  } else {
    SoftwareSerial * tt = (SoftwareSerial *) port;
    tt->begin(_tembaud);
  }
}

void Pzem004t_V3::begin() {
  this->begin(9600);
}

void Pzem004t_V3::setTimeout(unsigned int _ui_timeOut) {
  if (_ui_timeOut < MIN_PZEM_TIMEOUT) {
    ui_timeOut = MIN_PZEM_TIMEOUT;
  } else if (_ui_timeOut > MAX_PZEM_TIMEOUT) {
    ui_timeOut = MAX_PZEM_TIMEOUT;
  } else
    ui_timeOut = _ui_timeOut;
}

pzem_info Pzem004t_V3::getData(){
  pzem_info tem_pzem;
  while (port->available()) {
    port->read();
  }
  DB("port->write");
  port->write(getValue_para, sizeof(getValue_para));

  unsigned long temTime = millis();
  bool b_complete = false;
  uint8_t myBuf[RESPONSE_SIZE];

  while ((millis() - temTime) < ui_timeOut) {
    if (port->available()) {
      port->readBytes(myBuf, RESPONSE_SIZE);
      b_complete = true;
      DB_LN("port->available");
      yield();
      break;
    }
  }
 
  if (b_complete) {
//    tem_pzem.volt = PZEM_GET_VALUE(voltage,SCALE_V);
//    tem_pzem.ampe = PZEM_GET_VALUE(ampe, SCALE_A);
//    tem_pzem.power = PZEM_GET_VALUE(power, SCALE_P);
//    tem_pzem.energy = PZEM_GET_VALUE(energy, SCALE_E);
//    tem_pzem.freq = PZEM_GET_VALUE(freq, SCALE_H);
//    tem_pzem.powerFactor = PZEM_GET_VALUE(powerFactor, SCALE_PF);

    tem_pzem.volt = ((uint32_t)myBuf[3] << 8 | // Raw voltage in 0.1V
                              (uint32_t)myBuf[4])*SCALE_V;       ///10.0;

     tem_pzem.ampe = ((uint32_t)myBuf[5] << 8 | // Raw current in 0.001A
                              (uint32_t)myBuf[6] |
                              (uint32_t)myBuf[7] << 24 |
                              (uint32_t)myBuf[8] << 16)*SCALE_A;      /// 1000.0;

     tem_pzem.power =   ((uint32_t)myBuf[9] << 8 | // Raw power in 0.1W
                              (uint32_t)myBuf[10] |
                              (uint32_t)myBuf[11] << 24 |
                              (uint32_t)myBuf[12] << 16)*SCALE_P;       /// 10.0;

     tem_pzem.energy =  ((uint32_t)myBuf[13] << 8 | // Raw Energy in 1Wh
                              (uint32_t)myBuf[14] |
                              (uint32_t)myBuf[15] << 24 |
                              (uint32_t)myBuf[16] << 16)*SCALE_E;             /// 1000.0;

    tem_pzem.freq =((uint32_t)myBuf[17] << 8 | // Raw Frequency in 0.1Hz
                              (uint32_t)myBuf[18]) *SCALE_H;                        //10.0;

     tem_pzem.powerFactor   =   ((uint32_t)myBuf[19] << 8 | // Raw pf in 0.01
                              (uint32_t)myBuf[20])* SCALE_PF;                   //100.0;

    
    DB(pzemData.address, HEX);    DB(F(" - "));
    DB(pzemData.byteSuccess, HEX);    DB(F(" - "));
    DB(pzemData.numberOfByte, HEX);    DB(F(" -- "));

    DB(pzemData.voltage_int, HEX);    DB(F("V - "));
    DB(pzemData.ampe_int, HEX);    DB(F("A - "));
    DB(pzemData.power_int, HEX);    DB(F("W - "));
    DB(pzemData.energy_int, HEX);    DB(F("Wh - "));
    DB(pzemData.freq_int, HEX);    DB(F("Hz - "));
    DB_LN(pzemData.powerFactor_int, HEX);    DB(F(" - "));

  } else {
    DB_LN(F("Read fail"));
    tem_pzem.volt = 0.0;
    tem_pzem.ampe = 0.0;
    tem_pzem.power = 0.0;
    tem_pzem.energy = 0.0;
    tem_pzem.freq = 0.0;
    tem_pzem.powerFactor = 0.0;
  }
  return tem_pzem;
}

bool Pzem004t_V3::resetEnergy() {
  bool status = false;
  while (port->available()) {
    port->read();
  }
  port->write(resetEnergy_para, sizeof(resetEnergy_para));

  unsigned long temTime = millis();
  bool b_complete = false;

  byte testRespone[4];

  while ((millis() - temTime) < ui_timeOut) {
    if (port->available()) {
      port->readBytes(testRespone, sizeof(testRespone));
      b_complete = true;
      DB(F("port->available"));
      yield();
      break;
    }
  }

  if (b_complete) {
    if(testRespone[3] == resetEnergy_para[3]){
        status = true;
        DB_LN(F("resetEnergy success!"));
    }else{
        DB_LN(F("resetEnergy invalid"));
    }

  } else {
    DB_LN(F("read resetEnergy fail"));
    
  }
  return status;
}

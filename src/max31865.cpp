#include "max31865.h"

Max31865::Max31865(uint8_t cs, float refResistor, float rtdNominal) : m_cs(cs), m_configInit(0b0), m_configVbias(0b0), m_config1Shot(0b0), m_configFaultClear(0b0), m_refResistor(refResistor), m_rtdNominal(rtdNominal), m_max31865Setting(1000000, MSBFIRST, SPI_MODE1) {}

bool Max31865::config(uint8_t wire, uint8_t filterFreq) {

    SPI.begin();
    pinMode(m_cs, OUTPUT);

    if (wire == 3)
        m_configInit |= 0b00010000;
    else if (wire != 2 && wire != 4) {
        Serial.println("Wire number error");
        while (1);
    }

    if (filterFreq == 50)
        m_configInit |= 0b00000001;
    else if (filterFreq != 60) {
        Serial.println("Frequency selection error");
        while (1);
    }

    m_configVbias = m_configInit | 0b10000000;
    m_config1Shot = m_configInit | 0b10100000;
    m_configFaultClear = m_configInit | 0b00000010;

    write(REGISTER_CONFIG, &m_configInit);

    uint8_t configInitRead = 0;
    read8(REGISTER_CONFIG, &configInitRead);

    return m_configInit == configInitRead;
}

bool Max31865::enableContinuousMode(){
    m_configInit |= 0b11000000;
    m_configFaultClear = m_configInit | 0b00000010;
    write(REGISTER_CONFIG, &m_configInit);
    
    uint8_t configInitRead = 0;
    read8(REGISTER_CONFIG, &configInitRead);

    return m_configInit == configInitRead;
}

bool Max31865::disableContinuousMode(){
    m_configInit &= 0b00010001;
    m_configFaultClear = m_configInit | 0b00000010;
    write(REGISTER_CONFIG, &m_configInit);
    
    uint8_t configInitRead = 0;
    read8(REGISTER_CONFIG, &configInitRead);

    return m_configInit == configInitRead;
}

void Max31865::oneShotReadRTD(uint16_t *rtd, bool *fault) {
    uint16_t rtdRegister = 0x0;
    write(REGISTER_CONFIG, &m_configVbias);
    delay(10);
    write(REGISTER_CONFIG, &m_config1Shot);
    delay(65);
    read16(REGISTER_RTD, &rtdRegister);
    write(REGISTER_CONFIG, &m_configInit);
    *rtd = rtdRegister >> 1;
    *fault = rtdRegister & 0x1;
}

void Max31865::continusReadRTD(uint16_t *rtd, bool *fault) {
    uint16_t rtdRegister = 0x0;
    read16(REGISTER_RTD, &rtdRegister);
    *rtd = rtdRegister >> 1;
    *fault = rtdRegister & 0x1;
}

void Max31865::calculateTemperatureAdAlgo(float data, float *ratio, float *temperature, float *R) {
    float Z1, Z2, Z3, Z4;

    data /= 32768;
    *ratio = data;
    data *= m_refResistor;
    *R = data;

    Z1 = -RTD_A;
    Z2 = RTD_A * RTD_A - (4 * RTD_B);
    Z3 = (4 * RTD_B) / m_rtdNominal;
    Z4 = 2 * RTD_B;

    *temperature = Z2 + (Z3 * data);
    *temperature = (sqrt(*temperature) + Z1) / Z4;

    if (*temperature >= 0)
        return;

    data /= m_rtdNominal;
    data *= 100;

    float rpoly = data;

    *temperature = -242.02;
    *temperature += 2.2228 * rpoly;
    rpoly *= data;
    *temperature += 2.5859e-3 * rpoly;
    rpoly *= data;
    *temperature -= 4.8260e-6 * rpoly;
    rpoly *= data;
    *temperature -= 2.8183e-8 * rpoly;
    rpoly *= data;
    *temperature += 1.5243e-10 * rpoly;
}

void Max31865::calculateTemperaturePT100Algo(float data, float *ratio, float *temperature, float *R) {
    data /= 32768;
    *ratio = data;
    data *= m_refResistor;
    *R = data;

    float a1 = data - 100;
    float a2 = a1 * a1;

    if ((data - 100) > 0) {
        *temperature = KD * a1 + KC * a2;
    } else {
        float a3 = a1 * a2;
        float a4 = a1 * a3;
        *temperature = KD * a1 + KC * a2 + KB * a3 + KA * a4;
    }
}

void Max31865::readFault(uint8_t *faultData) {
    uint8_t faultValue = 0x0;
   	read8(REGISTER_FAULT, &faultValue);
    *faultData = faultValue;
}

void Max31865::clearFaultRegister() {
    write(REGISTER_CONFIG, &m_configFaultClear);
}

void Max31865::write(uint8_t address, uint8_t *data) {
    address |= 0x80;
    SPI.beginTransaction(m_max31865Setting);
    digitalWrite(m_cs, LOW);
    SPI.transfer(address);
    SPI.transfer(*data);
    digitalWrite(m_cs, HIGH);
    SPI.endTransaction();
}

void Max31865::read8(uint8_t address, uint8_t *data) {
    SPI.beginTransaction(m_max31865Setting);
    digitalWrite(m_cs, LOW);
    SPI.transfer(address);
    *data = SPI.transfer(0xFF);
    digitalWrite(m_cs, HIGH);
    SPI.endTransaction();
}

void Max31865::read16(uint8_t address, uint16_t *data) {
    uint8_t dataMSB = 0x0;
    uint8_t dataLSB = 0x0;
    SPI.beginTransaction(m_max31865Setting);
    digitalWrite(m_cs, LOW);
    SPI.transfer(address);
    dataMSB = SPI.transfer(0xFF);
    dataLSB = SPI.transfer(0xFF);
    digitalWrite(m_cs, HIGH);
    SPI.endTransaction();
    *data = dataMSB << 8 | dataLSB;
}
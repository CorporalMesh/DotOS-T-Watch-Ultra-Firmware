#include "target.h"
#include <LilyGoLib.h>
#include <esp_system.h>
WatchBoard board;
WatchRadio radio_driver(radio, board);
WatchClock rtc_clock;
SensorManager sensors;
class SecureRNG : public mesh::RNG {
 void random(uint8_t* dest, size_t size) override { esp_fill_random(dest, size); }
};
mesh::LocalIdentity radio_new_identity() {
 SecureRNG rng;
 return mesh::LocalIdentity(&rng);
}

uint16_t WatchBoard::getBattMilliVolts() { return instance.pmu.getBattVoltage(); }
float WatchBoard::getMCUTemperature() { return temperatureRead(); }
void WatchBoard::reboot() { ESP.restart(); }
void WatchBoard::powerOff() { instance.pmu.shutdown(); }
uint32_t WatchClock::getCurrentTime() {
 tm utc={}; instance.rtc.getDateTime(&utc); utc.tm_isdst=0;
 return static_cast<uint32_t>(mktime(&utc));
}
void WatchClock::setCurrentTime(uint32_t epoch) {
 time_t value=epoch; tm utc; gmtime_r(&value,&utc);
 instance.rtc.setDateTime(utc.tm_year+1900,utc.tm_mon+1,utc.tm_mday,utc.tm_hour,utc.tm_min,utc.tm_sec);
 timeval tv={static_cast<time_t>(epoch),0}; settimeofday(&tv,nullptr);
}

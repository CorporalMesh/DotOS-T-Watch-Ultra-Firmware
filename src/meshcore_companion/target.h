#pragma once
#include <sys/time.h>
#include <helpers/ArduinoHelpers.h>
#include <helpers/KeyValueStore.h>
#include <helpers/SensorManager.h>
#include <helpers/radiolib/CustomSX1262Wrapper.h>
class WatchBoard : public mesh::MainBoard {
public:
 uint16_t getBattMilliVolts() override;
 float getMCUTemperature() override;
 const char* getManufacturerName() const override { return "DotOS T-Watch Ultra"; }
 uint8_t getStartupReason() const override { return BD_STARTUP_NORMAL; }
 void reboot() override;
 void powerOff() override;
 void attachDynamicPrefs(KeyValueStore*) { }
};
// Reuse the upstream MeshCore SX1262 driver on LilyGo's single radio instance.
using WatchRadio = CustomSX1262Wrapper;
class WatchClock : public mesh::RTCClock {
public:
 uint32_t getCurrentTime() override;
 void setCurrentTime(uint32_t epoch) override;
};
extern WatchBoard board;
extern WatchRadio radio_driver;
extern WatchClock rtc_clock;
extern SensorManager sensors;
mesh::LocalIdentity radio_new_identity();

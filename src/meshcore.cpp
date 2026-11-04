#include "meshcore.h"
#include "meshcore_ui.h"
#include "meshcore_companion/MyMesh.h"
#include "psram_array.h"
#include "gps_screen.h"
#include <LilyGoLib.h>
#include <FFat.h>
#include <Preferences.h>
#include <WiFi.h>
#include <esp_bt_main.h>
#include <helpers/esp32/SerialBLEInterface.h>
#include <helpers/MultiSerialInterface.h>
#include <esp_system.h>
#include <time.h>
#include <algorithm>
#include <new>

void clock_screen_set_mesh_count(int);
void meshcore_screen_refresh();
void nodes_screen_refresh();
void clock_screen_get_local_time(struct tm*);
bool configuration_screen_get_vibrate_dm();
bool configuration_screen_get_vibrate_broadcast();
bool configuration_screen_get_broadcast_location();
uint32_t configuration_screen_get_broadcast_interval_ms();

namespace {
bool active = false, ready = false, ble_ready = false;
bool announce_on = true;
uint32_t announce_ms = 600000, last_announce = 0, last_position = 0;
int count = 0, unread = 0, outgoing_count = 0, active_channel = 0;
char short_name[MESH_MAX_SHORT_NAME] = "DOT";
char pending_name[MESH_MAX_LONG_NAME] = "";
PsramArray<MeshMessage, MESH_MAX_MESSAGES> messages;
PsramArray<MeshNode, MESH_MAX_NODES> nodes;
PsramArray<MeshOutgoing, MESH_MAX_OUTGOING> outgoing;
MeshChannel ui_channels[MESH_MAX_CHANNELS] = {};
MeshDebugInfo debug_info = {};
MeshTraceroute trace = {};
StdRNG rng;
DataStore store(FFat, rtc_clock);
SerialBLEInterface ble;
MultiSerialInterface interfaces;
MyMesh* watch_mesh = nullptr;

uint32_t node_id(const uint8_t* key) {
 return meshcoreui::display_id(key);
}
void time_label(char* buf, size_t size) {
 tm t={}; clock_screen_get_local_time(&t); strftime(buf, size, "%H:%M", &t);
}
void append_message(uint32_t from, uint32_t dest, const char* text, mesh::Packet* pkt=nullptr, int channel=-1) {
 for (int i = std::min(count, MESH_MAX_MESSAGES-1); i > 0; --i) messages[i] = messages[i-1];
 auto& msg = messages[0]; msg = {};
 msg.from_node = from; msg.dest_node = dest;
 msg.channel_idx = channel;
 strlcpy(msg.text, text, sizeof(msg.text)); time_label(msg.time_str, sizeof(msg.time_str));
 msg.rssi = radio_driver.getLastRSSI(); msg.snr = radio_driver.getLastSNR();
 msg.hop_start = pkt && pkt->isRouteFlood() ? pkt->getPathHashCount() : 0;
 count = std::min(count+1, MESH_MAX_MESSAGES);
 bool incoming = from != node_id(watch_mesh->self_id.pub_key);
 if (incoming) {
   ++unread;
   if ((dest != 0xffffffff && configuration_screen_get_vibrate_dm()) ||
       (dest == 0xffffffff && configuration_screen_get_vibrate_broadcast())) instance.vibrator();
 }
 clock_screen_set_mesh_count(unread); meshcore_screen_refresh();
}
class WatchListener : public MyMesh::Listener {
 void onMessageRecv(mesh::Packet* pkt, const ContactInfo& from, uint8_t type,
                    uint32_t timestamp, const char* text) override {
   append_message(node_id(from.id.pub_key), node_id(watch_mesh->self_id.pub_key), text, pkt);
 }
 void onChannelMessageRecv(mesh::Packet* pkt, ChannelDetails& channel, const char* text) override {
   // Channel text includes the unauthenticated display name. Never use it as a DM identity.
   append_message(0, 0xffffffff, text, pkt, watch_mesh->findChannelIdx(channel.channel));
 }
 void onQueueSizeChanged(int) override {}
 void onLocalMessageSent(const uint8_t* recipient_key, int channel, const char* text) override {
   append_message(node_id(watch_mesh->self_id.pub_key), recipient_key?node_id(recipient_key):0xffffffff, text, nullptr, channel);
 }
 void onDiscoveredContact(ContactInfo&, bool, uint8_t, const uint8_t*) override { nodes_screen_refresh(); }
 void onACKRecv(uint32_t ack) override {
   for (int i=0; i<outgoing_count; ++i)
     if (outgoing[i].packet_id == ack) outgoing[i].state = MeshOutgoing::DELIVERED;
 }
} listener;

bool start_mesh() {
 if (ready) return true;
 // Internal FAT is separate from removable SD. Keep identity even without SD.
 if (!FFat.begin(true)) { Serial.println("MeshCore: internal FAT mount failed"); return false; }
 rng.begin(esp_random());
 store.begin();
 void* memory = heap_caps_malloc(sizeof(MyMesh), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
 if (!memory) memory = malloc(sizeof(MyMesh));
 if (!memory) { Serial.println("MeshCore: out of memory"); return false; }
 watch_mesh = new(memory) MyMesh(radio_driver, rng, rtc_clock, *new SimpleMeshTables(), store);
 radio_driver.begin();
 watch_mesh->begin();
 if (!watch_mesh->getNodePrefs()->ble_pin) {
   watch_mesh->getNodePrefs()->ble_pin = 100000 + esp_random()%900000; watch_mesh->savePrefs();
 }
 watch_mesh->setBLEPin(watch_mesh->getNodePrefs()->ble_pin);
 interfaces.enable();
 watch_mesh->startInterface(interfaces);  // must exist before any RX, even with BLE off
 watch_mesh->setListener(&listener);
 ready = true;
 if (pending_name[0]) { strlcpy(watch_mesh->getNodePrefs()->node_name, pending_name, 32); watch_mesh->savePrefs(); }
 return true;
}
ContactInfo* find_contact(uint32_t id) {
 ContactInfo value;
 meshcoreui::ContactSelection selection(id);
 for (int i=MAX_ANON_CONTACTS; watch_mesh && watch_mesh->getContactByIdx(i,value); ++i)
   selection.consider(value.id.pub_key,i);
 if(selection.index()<0 || !watch_mesh->getContactByIdx(selection.index(),value)) return nullptr;
 return watch_mesh->lookupContactByPubKey(value.id.pub_key,PUB_KEY_SIZE);
}
}

void meshcore_set_active(bool on) {
 if (on == active) return;
 if (on) {
   if (!start_mesh()) return;
   radio_driver.setParams(watch_mesh->getNodePrefs()->freq, watch_mesh->getNodePrefs()->bw,
                          watch_mesh->getNodePrefs()->sf, watch_mesh->getNodePrefs()->cr);
   radio_driver.setTxPower(watch_mesh->getNodePrefs()->tx_power_dbm);
   radio_driver.setRxBoostedGainMode(watch_mesh->getNodePrefs()->rx_boosted_gain);
   radio_driver.begin(); active = true; last_announce = millis();
 } else {
   if (ble_ready) ble.disable();
   radio.clearPacketReceivedAction(); radio.standby(); active = false;
 }
}
void meshcore_bg_tick() {
 if (!active || !watch_mesh) return;
 watch_mesh->loop();
 for (int i=0; i<outgoing_count; ++i)
   if (outgoing[i].state == MeshOutgoing::PENDING && millis()-outgoing[i].sent_ms > 60000)
     outgoing[i].state = MeshOutgoing::NO_ACK;
 bool gps_broadcast = configuration_screen_get_broadcast_location();
 bool position_due = gps_broadcast && gps_screen_has_lock() &&
     millis()-last_position >= configuration_screen_get_broadcast_interval_ms();
 if (position_due || (announce_on && millis()-last_announce >= announce_ms)) {
   if (gps_broadcast) {
     sensors.node_lat = gps_screen_has_lock() ? instance.gps.location.lat() : 0;
     sensors.node_lon = gps_screen_has_lock() ? instance.gps.location.lng() : 0;
   }
   watch_mesh->advert(); last_announce = millis();
   if (position_due) last_position = millis();
 }
}
// The companion owns GAP until reboot once started. Other BLE modes are guarded.
bool meshcore_ble_owned() { return ble_ready; }
bool meshcore_phone_connected() { return ble_ready && ble.isConnected(); }
bool meshcore_phone_enabled() { return ble_ready && ble.isEnabled(); }
bool meshcore_set_phone_enabled(bool on) {
 if (!on) { if (ble_ready) ble.disable(); return true; }
 if (!active || !watch_mesh || WiFi.getMode()!=WIFI_MODE_NULL) return false;
 if (!ble_ready) {
   if (esp_bluedroid_get_status()!=ESP_BLUEDROID_STATUS_UNINITIALIZED) return false;
   uint32_t pin = watch_mesh->getNodePrefs()->ble_pin;
   if (!pin) { pin = BLE_PIN_CODE; watch_mesh->getNodePrefs()->ble_pin = pin; watch_mesh->savePrefs(); }
   watch_mesh->setBLEPin(pin);
   ble.begin("MeshCore-", watch_mesh->getNodePrefs()->node_name, pin);
   interfaces.addInterface(InterfaceType::Bluetooth, &ble);
   ble_ready = true;
 }
 ble.enable(); return true;
}
uint32_t meshcore_phone_pin() { return ready ? watch_mesh->getBLEPin() : BLE_PIN_CODE; }
float meshcore_frequency() { return ready ? watch_mesh->getNodePrefs()->freq : LORA_FREQ; }
float meshcore_bandwidth() { return ready ? watch_mesh->getNodePrefs()->bw : LORA_BW; }
int meshcore_tx_power() { return ready ? watch_mesh->getNodePrefs()->tx_power_dbm : LORA_TX_POWER; }

bool meshcore_send_text(const char* text) { return meshcore_send_text_to(text, 0xffffffff); }
bool meshcore_send_text_to(const char* text, uint32_t dest) {
 if (!active || !meshcoreui::text_fits(text)) return false;
 if (dest == 0xffffffff) {
   ChannelDetails channel;
   if (!watch_mesh->getChannel(active_channel, channel) || !channel.name[0]) return false;
   if (!watch_mesh->sendGroupMessage(rtc_clock.getCurrentTimeUnique(), channel.channel, watch_mesh->getNodeName(), text, strlen(text))) return false;
 } else {
   ContactInfo* contact = find_contact(dest);
   if (!contact) return false;
   uint32_t ack, timeout;
   int result = watch_mesh->sendMessage(*contact, rtc_clock.getCurrentTimeUnique(), 0, text, ack, timeout);
   if (result == MSG_SEND_FAILED) return false;
   for(int i=std::min(outgoing_count,MESH_MAX_OUTGOING-1);i>0;--i) outgoing[i]=outgoing[i-1];
   outgoing[0]={}; outgoing[0].packet_id=ack; outgoing[0].sent_ms=millis(); outgoing[0].dest_node=dest;
   strlcpy(outgoing[0].text,text,sizeof(outgoing[0].text)); outgoing_count=std::min(outgoing_count+1,MESH_MAX_OUTGOING);
 }
 append_message(node_id(watch_mesh->self_id.pub_key),dest,text,nullptr,dest==0xffffffff?active_channel:-1);
 return true;
}
// MeshCore companion trace/telemetry are available in the phone app; do not emit
// Meshtastic traceroute or position packets on a MeshCore network.
bool meshcore_send_traceroute(uint32_t) { return false; }
bool meshcore_traceroute_in_flight() { return false; }
const MeshTraceroute* meshcore_get_last_traceroute() { return &trace; }
bool meshcore_request_position(uint32_t) { return false; }
int meshcore_get_outgoing_count() { return outgoing_count; }
const MeshOutgoing* meshcore_get_outgoing(int i) { return i>=0&&i<outgoing_count ? &outgoing[i] : nullptr; }
const char* meshcore_routing_error_name(uint8_t) { return "Send failed"; }
const char* meshcore_role_name(uint32_t role) {
 switch(role) { case 1:return "Companion"; case 2:return "Repeater"; case 3:return "Room server"; case 4:return "Sensor"; default:return "Unknown"; }
}
const MeshChannel* meshcore_get_channel(int i) {
 if (!ready || i<0 || i>=MESH_MAX_CHANNELS) return nullptr;
 ChannelDetails channel;
 if(!watch_mesh->getChannel(i,channel)) return nullptr;
 auto& out=ui_channels[i]; out={};
 strlcpy(out.name,channel.name,sizeof(out.name)); memcpy(out.psk,channel.channel.secret,16);
 out.channel_hash=channel.channel.hash[0]; out.psk_len=channel.name[0]?16:0; out.enabled=channel.name[0]; return &out;
}
void meshcore_set_channel_enabled(int, bool) { /* phone app manages channel names/keys */ }
int meshcore_get_active_channel() { return active_channel; }
void meshcore_set_active_channel(int i) {
 if(i>=0&&i<MESH_MAX_CHANNELS) {
   ChannelDetails c;
   if (ready && watch_mesh->getChannel(i,c) && c.name[0]) active_channel=i;
 }
}
void meshcore_load_channels_from_sd() { /* legacy Meshtastic keys are deliberately not imported */ }
void meshcore_save_channels_to_sd() { /* persisted by official companion DataStore */ }
void meshcore_set_long_name(const char* value) {
 if (!value) return;
 // Once initialized, companion prefs are authoritative (including phone edits).
 if(ready) { strlcpy(watch_mesh->getNodePrefs()->node_name,value,32); watch_mesh->savePrefs(); }
 else strlcpy(pending_name,value,sizeof(pending_name));
}
void meshcore_set_short_name(const char* value) { if(value) strlcpy(short_name,value,sizeof(short_name)); }
const char* meshcore_get_long_name() { return ready?watch_mesh->getNodeName():pending_name[0]?pending_name:"DotOS"; }
const char* meshcore_get_short_name() { return short_name; }
void meshcore_set_announce(bool on,uint32_t ms) { announce_on=on; announce_ms=std::max<uint32_t>(ms,60000); }
bool meshcore_announce_enabled() { return announce_on; }
uint32_t meshcore_announce_interval_ms() { return announce_ms; }
int meshcore_get_node_count() { return ready?std::min(watch_mesh->getNumContacts(),MESH_MAX_NODES):0; }
const MeshNode* meshcore_get_node(int index) {
 if(index<0||index>=meshcore_get_node_count()) return nullptr;
 ContactInfo contact;
 if(!watch_mesh->getContactByIdx(index+MAX_ANON_CONTACTS,contact)) return nullptr;
 auto& n=nodes[index]; n={};
 n.node_id=node_id(contact.id.pub_key); memcpy(n.public_key,contact.id.pub_key,32); n.public_key_len=32;
 strlcpy(n.long_name,contact.name,sizeof(n.long_name)); strlcpy(n.short_name,contact.name,sizeof(n.short_name));
 mesh::Utils::toHex(n.id,contact.id.pub_key,6);
 n.role=contact.type; n.has_role=true;
 n.latitude_i=contact.gps_lat*10; n.longitude_i=contact.gps_lon*10;
 n.has_position=contact.gps_lat || contact.gps_lon;
 time_label(n.time_str,sizeof(n.time_str)); return &n;
}
int meshcore_get_count() { return count; }
int meshcore_get_rx_count() { return radio_driver.getPacketsRecv(); }
int meshcore_get_unread() { return unread; }
void meshcore_mark_read() { unread=0; clock_screen_set_mesh_count(0); }
const MeshMessage* meshcore_get_message(int i) { return i>=0&&i<count?&messages[i]:nullptr; }
void meshcore_clear_messages() { count=0; meshcore_mark_read(); }
bool meshcore_delete_message(int i) {
 if(i<0||i>=count)return false;
 for(int k=i;k<count-1;++k)messages[k]=messages[k+1];
 --count; return true;
}
float meshcore_get_rssi() { return radio_driver.getLastRSSI(); }
bool meshcore_get_cad() { return active && radio_driver.isChannelActive(); }
bool meshcore_is_active() { return active; }
uint32_t meshcore_get_node_id() { return ready?node_id(watch_mesh->self_id.pub_key):0; }
const MeshDebugInfo* meshcore_get_debug() {
 debug_info.rx_ok=radio_driver.getPacketsRecv(); debug_info.rx_fail=radio_driver.getPacketsRecvErrors(); return &debug_info;
}

void meshcore_clear_advert_location() {
 if (!ready) return;
 sensors.node_lat=0; sensors.node_lon=0; watch_mesh->savePrefs();
}

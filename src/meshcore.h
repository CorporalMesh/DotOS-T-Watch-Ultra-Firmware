// DotOS display API. IDs are local public-key prefixes; wire identities remain 32-byte MeshCore keys.
#pragma once
#include <stdint.h>
#include <stddef.h>

#define MESH_MAX_MESSAGES 20
#define MESH_MAX_TEXT_LEN 200

struct MeshMessage {
    int      channel_idx;  
    uint32_t from_node;
    uint32_t dest_node;     
                            
    char     text[MESH_MAX_TEXT_LEN];
    char     time_str[6];   

    float    rssi;
    float    snr;
    uint8_t  hop_start;
    uint8_t  hop_limit;
};

void meshcore_set_active(bool active);
void meshcore_bg_tick();

bool meshcore_send_text(const char *text);

bool meshcore_send_text_to(const char *text, uint32_t dest_node);

struct MeshTraceroute {
    uint32_t target_node;
    uint32_t request_id;
    uint32_t sent_ms;
    bool     has_response;
    uint32_t response_ms;
    uint8_t  hop_count;
    uint32_t hops[8];
};
bool                 meshcore_send_traceroute(uint32_t dest_node);
bool                 meshcore_traceroute_in_flight();
const MeshTraceroute *meshcore_get_last_traceroute();

#define MESH_MAX_OUTGOING 6
struct MeshOutgoing {
    uint32_t packet_id;
    char     text[MESH_MAX_TEXT_LEN];
    uint32_t sent_ms;
    uint32_t dest_node;
    enum State { PENDING = 0, DELIVERED = 1, NO_ACK = 2, NAKED = 3 } state;
    uint8_t  error_reason;
};
int                  meshcore_get_outgoing_count();
const MeshOutgoing  *meshcore_get_outgoing(int idx);     

const char          *meshcore_routing_error_name(uint8_t reason);

#define MESH_MAX_CHANNELS 8
struct MeshChannel {
    char    name[32];
    uint8_t psk[32];
    uint8_t psk_len;       
    uint8_t channel_hash;  
    bool    enabled;
};
const MeshChannel *meshcore_get_channel(int idx);
void               meshcore_set_channel_enabled(int idx, bool on);
int                meshcore_get_active_channel();
void               meshcore_set_active_channel(int idx);

void               meshcore_load_channels_from_sd();
void               meshcore_save_channels_to_sd();

bool                 meshcore_request_position(uint32_t dest_node);

#define MESH_MAX_LONG_NAME  32
#define MESH_MAX_SHORT_NAME  5   

void        meshcore_set_long_name(const char *name);
void        meshcore_set_short_name(const char *name);
const char *meshcore_get_long_name();
const char *meshcore_get_short_name();

void     meshcore_set_announce(bool on, uint32_t interval_ms);
bool     meshcore_announce_enabled();
uint32_t meshcore_announce_interval_ms();

#define MESH_MAX_NODES 100

struct MeshNeighbor {
    uint32_t node_id;
    float    snr;
};
#define MESH_MAX_NEIGHBORS  8

struct MeshNode {
    uint32_t node_id;
    char     id[16];                          
    char     long_name[MESH_MAX_LONG_NAME];   
    char     short_name[MESH_MAX_SHORT_NAME]; 
    uint8_t  macaddr[6];                      
    bool     has_macaddr;
    uint32_t hw_model;                        
    bool     has_hw_model;
    bool     is_licensed;                     
    bool     has_is_licensed;
    uint32_t role;                            
    bool     has_role;
    uint8_t  public_key[32];                  
    uint8_t  public_key_len;

    int32_t  latitude_i;
    int32_t  longitude_i;
    bool     has_position;
    int32_t  altitude;                        
    bool     has_altitude;
    char     time_str[6];                     
    char     disco_ts[16];                    

    bool     has_telemetry;
    uint32_t battery_level;                   bool has_battery_level;  
    float    voltage;                         bool has_voltage;        
    uint32_t uptime_seconds;                  bool has_uptime;

    bool     has_environment;
    float    temperature_c;                   bool has_temperature;
    float    relative_humidity;               bool has_humidity;
    float    pressure_hpa;                    bool has_pressure;
    float    lux;                             bool has_lux;
    uint32_t iaq;                             bool has_iaq;

    bool     has_power_metrics;
    float    ch1_voltage;                     bool has_ch1_voltage;
    float    ch1_current;                     bool has_ch1_current;

    uint8_t       neighbor_count;
    MeshNeighbor  neighbors[MESH_MAX_NEIGHBORS];
    bool          has_neighborinfo;
};

const char *meshcore_role_name(uint32_t role);

int             meshcore_get_node_count();
const MeshNode *meshcore_get_node(int idx); 

int  meshcore_get_count();      
int  meshcore_get_rx_count();   
int  meshcore_get_unread();
void meshcore_mark_read();

const MeshMessage *meshcore_get_message(int idx); 

void meshcore_clear_messages();

bool meshcore_delete_message(int idx);

float    meshcore_get_rssi();
bool     meshcore_get_cad();
bool     meshcore_is_active();
uint32_t meshcore_get_node_id();  

struct MeshDebugInfo {
    int      isr_count;       
    int      rx_ok;           
    int      rx_fail;         
    int16_t  rx_last_rc;      
    int      rx_last_pkt_len; 
    int      parse_ok;        
    int      nodeinfo_tx;     
    uint32_t last_portnum;    
    uint8_t  last_plain0;     
    uint8_t  last_plain1;     
};
const MeshDebugInfo *meshcore_get_debug();

bool meshcore_ble_owned();
bool meshcore_phone_connected();
bool meshcore_phone_enabled();
bool meshcore_set_phone_enabled(bool enabled);
uint32_t meshcore_phone_pin();
float meshcore_frequency();
float meshcore_bandwidth();
int meshcore_tx_power();

void meshcore_clear_advert_location();

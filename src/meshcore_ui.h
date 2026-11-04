#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
namespace meshcoreui {
constexpr size_t max_text_bytes = 130;
inline bool text_fits(const char* text) {
 if (!text || !*text) return false;
 for(size_t i=0;i<=max_text_bytes;++i) if(!text[i]) return true;
 return false;
}
inline uint32_t display_id(const uint8_t* key) {
 return uint32_t(key[0]) | uint32_t(key[1])<<8 | uint32_t(key[2])<<16 | uint32_t(key[3])<<24;
}
// UI IDs are abbreviated; an ambiguous prefix must never address a DM.
class ContactSelection {
 uint32_t id_;
 uint8_t key_[32] = {};
 int index_ = -1;
 bool ambiguous_ = false;
public:
 explicit ContactSelection(uint32_t id) : id_(id) {}
 void consider(const uint8_t* key,int index) {
  if(display_id(key)!=id_) return;
  if(index_>=0 && std::memcmp(key_,key,32)!=0) ambiguous_=true;
  else { std::memcpy(key_,key,32); index_=index; }
 }
 int index() const { return ambiguous_ ? -1 : index_; }
};
}

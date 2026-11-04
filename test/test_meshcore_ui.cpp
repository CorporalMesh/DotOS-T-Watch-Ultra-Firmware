#include "wl_test.h"
#include "meshcore_ui.h"
#include <string>
WL_TEST(meshcore_text_limit_counts_radio_bytes) {
 WL_CHECK(!meshcoreui::text_fits(nullptr));
 WL_CHECK(!meshcoreui::text_fits(""));
 WL_CHECK(meshcoreui::text_fits(std::string(130,'x').c_str()));
 WL_CHECK(!meshcoreui::text_fits(std::string(131,'x').c_str()));
 std::string utf8;
 for(int i=0;i<32;i++)utf8 += "📡";
 WL_CHECK(meshcoreui::text_fits(utf8.c_str()));
 utf8 += "📡";
 WL_CHECK(!meshcoreui::text_fits(utf8.c_str()));
}
WL_TEST(meshcore_ui_prefix_collision_refuses_dm) {
 uint8_t a[32]={0x12,0x34,0x56,0x78};
 uint8_t b[32]={0x12,0x34,0x56,0x78};b[31]=1;
 WL_CHECK_EQ(meshcoreui::display_id(a),0x78563412u);
 meshcoreui::ContactSelection choice(meshcoreui::display_id(a));
 WL_CHECK_EQ(choice.index(),-1);
 choice.consider(a,8);WL_CHECK_EQ(choice.index(),8);
 choice.consider(a,8);WL_CHECK_EQ(choice.index(),8);
 choice.consider(b,9);WL_CHECK_EQ(choice.index(),-1);
 uint8_t c[32]={0x13};
 meshcoreui::ContactSelection other(meshcoreui::display_id(a));
 other.consider(c,10);WL_CHECK_EQ(other.index(),-1);
}

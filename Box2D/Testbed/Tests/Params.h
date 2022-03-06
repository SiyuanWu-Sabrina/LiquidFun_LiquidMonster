#ifndef PARAMS_H
#define PARAMS_H
namespace LiquidMonsterParams {

static const float32 k_playfieldWidth      = 40;
static const float32 k_playfieldHeight     = 55;
static const float32 k_playfieldLeftEdge   = -k_playfieldWidth / 2;
static const float32 k_playfieldRightEdge  = k_playfieldWidth / 2;
static const float32 k_playfieldBottomEdge = -5;
static const float32 k_playfieldTopEdge =
    k_playfieldBottomEdge + k_playfieldHeight;

static const int     k_thornNumbers = 15;
static const float32 k_thornWidth =
    (k_playfieldRightEdge - k_playfieldLeftEdge) / k_thornNumbers;
static const float32 k_thornHeight = 1.5;

static const float32 k_ground_height   = 10;
static const float32 k_groundThickness = 2;
static const float32 k_wallThickness   = 2;

static const float32 k_floor_height = 10;

static const float32 k_handle_radius        = 0.8f;
static const float32 k_down_length          = 10.0f;
static const float32 k_floor_thickness      = 1.5f;
static const float32 k_floor_velocity       = 3.0f;
static const float32 k_maximum_handle_speed = 15.0f;

static const float32 range_angle   = M_PI / 20;
static const float32 life_period   = 5.0f;
static const int     t_thorn       = 1;
static const int     t_wall        = 2;
static const int     t_ball        = 3;
static const int     t_hardfloor   = 4;
static const int     t_softfloor   = 5;
static const int     t_devil       = 6;
static const int     t_monster     = 7;
static const int     t_devil_blood = 8;
static const float32 k_time_score  = 0.01;
static const float32 k_devil_score = 20;

}  // namespace LiquidMonsterParams

// do not write const params in codes, write in this namespace instead!!!
namespace DevilParams {
static const float32 k_blood                        = 5.0f;
static const float32 k_scale                        = 10.0f;
static const float32 k_size                         = 0.3f;
static const float32 k_density                      = 10.0f;
static const int     k_devil_appearance_probability = 1;
// other params
}  // namespace DevilParams

namespace FloorParams {

static const float32 k_hardfloor_width =
    LiquidMonsterParams::k_playfieldWidth * 0.8;

static const float32 k_softfloor_width =
    LiquidMonsterParams::k_playfieldWidth * 0.8;
// Speed of the floors moving upward
static const float32 k_floor_speed = 2.0f;
// Total number of layers that would appear simultaneously on the screen
static const int k_layerNum = 4;
// Distance between two layers including the thickness of the layer
static const float32 k_layerHeight =
    1.0f / k_layerNum * LiquidMonsterParams::k_playfieldHeight;

// Thickness of the layers
static const float32 floorThickness = 2.0f;

};  // namespace FloorParams

#include <vector>

struct uInfo {
    int   type;
    void *pointer;
};

class uInfoManager {
  private:
    std::vector<uInfo *> used_info;
  public:
    virtual uInfo *info(uInfo &&newinfo) {
        uInfo *userinfo = new uInfo{newinfo};
        used_info.push_back(userinfo);
        return userinfo;
    }
    virtual ~uInfoManager() {
        for(auto t : used_info) {
            delete t;
        }
    }
};
int get_type(void *usi) {
    if(usi == NULL) {
        printf("what!\n");
        return 0;
    }
    // printf("type:%d\n",((uInfo *)usi)->type);
    return ((uInfo *)usi)->type;
}
template <typename T> T *get_pointer(void *usi) {
    if(usi == NULL) {
        printf("what!\n");
        assert(0);
        while(1) {
        }
    }
    assert(((uInfo *)usi)->type);
    return (T *)(((uInfo *)usi)->pointer);
}

#endif
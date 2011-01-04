#ifndef HAT_ENGINE_JAVASCRIPT_WEAPON_INTERFACE_HPP
#define HAT_ENGINE_JAVASCRIPT_WEAPON_INTERFACE_HPP

#include <vector>

namespace hat { 

namespace Fire_modes
{
  enum e {
    SAFETY              = 1 << 1,
    BOLT_ACTION         = 1 << 2,
    SEMI_AUTOMATIC      = 1 << 3,
    TWO_ROUND_BURST     = 1 << 4,
    THREE_ROUND_BURST   = 1 << 5,
    FULL_AUTO           = 1 << 6
  };
}


enum Weapon_type
{
  PRIMARY_WEAPON,
  SECONDARY_WEAPON,
  ITEM,
  MAX_WEAPON_TYPE
};

struct Accuracy_attrs {
  float stopped, shuffle, walk, run, walk_fast, crouch;
};

struct Animation_sound_attrs {
  void cleanup() { delete[] play; }
  int at;
  char* play;
};

typedef std::vector<Animation_sound_attrs> Animation_sound_list;

struct Animation_attrs {
  Animation_attrs() : time(0.0), type(NULL) { }
  char* type;
  char* animation;
  float time;
  Animation_sound_list sounds;
  void cleanup() 
  { 
    delete[] type;
    delete[] animation;
    for (
      Animation_sound_list::iterator cit = sounds.begin();
      cit != sounds.end();
      ++cit)
      {
        cit->cleanup();
      }
  }
};

struct Weapon_times {
  Weapon_times()
  {
    idle = reload = draw = fire = firelast = -1;
  }

  int idle;
  int reload;
  int draw;
  int fire;
  int firelast;
};

typedef std::vector<Animation_attrs> Animation_list;
typedef std::vector<int> Fire_mode;

struct Weapon_attrs {
  Weapon_attrs() : fire_modes_mask(0) { }
  char* name_pretty;
  char name_internal[24];
  char* model;
  int type_id, unique_id;
  Fire_mode fire_modes;
  int fire_modes_mask;
  Weapon_times times;
  Weapon_type type;
  Accuracy_attrs accuracy;
  Animation_list animations;
  void cleanup()
  {
    /*
    delete[] name;
    delete[] model;
    for (
      Animation_list::iterator cit = animations.begin();
      cit != animations.end();
      ++cit)
      {
        cit->cleanup();
      }
      */
  }
};

}

#endif // HAT_ENGINE_JAVASCRIPT_INTERFACE_HPP
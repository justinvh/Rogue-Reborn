#ifndef HAT_ENGINE_JAVASCRIPT_WEAPON_SCRIPT_HPP
#define HAT_ENGINE_JAVASCRIPT_WEAPON_SCRIPT_HPP

#include <hat/v8/easy.hpp>

namespace hat { 

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
  Animation_attrs() : time(0.0), name(NULL) { }
  char* name;
  float time;
  Animation_sound_list sounds;
  void cleanup() 
  { 
    delete[] name;
    for (
      Animation_sound_list::iterator cit = sounds.begin();
      cit != sounds.end();
      ++cit)
      {
        cit->cleanup();
      }
  }
};

typedef std::vector<Animation_attrs> Animation_list;

struct Weapon_attrs {
  char* name;
  char* model;
  Accuracy_attrs accuracy;
  Animation_list animations;
  void cleanup()
  {
    delete[] name;
    delete[] model;
    for (
      Animation_list::iterator cit = animations.begin();
      cit != animations.end();
      ++cit)
      {
        cit->cleanup();
      }
  }
};

namespace javascript {

class Weapon {
private:
  v8::Persistent<v8::Object> self;

public:
  ~Weapon();
  Weapon_attrs weapon_attrs;
  Weapon(Weapon_attrs attrs) : weapon_attrs(attrs) { }
  JS_INTERNAL_DEF(Weapon) {
  };

  static bool build_attributes(const v8::Arguments& args, Weapon_attrs* attrs);
  static v8::Handle<v8::Value> create(const v8::Arguments& args);
};

} }

#endif // HAT_ENGINE_JAVASCRIPT_WEAPON_SCRIPT_HPP
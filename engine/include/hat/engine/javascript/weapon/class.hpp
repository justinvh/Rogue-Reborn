#ifndef HAT_ENGINE_JAVASCRIPT_WEAPON_SCRIPT_HPP
#define HAT_ENGINE_JAVASCRIPT_WEAPON_SCRIPT_HPP

#include <hat/v8/easy.hpp>

#include <hat/engine/javascript/weapon/interface.hpp>

namespace hat { namespace javascript {

class Weapon {
private:
  v8::Persistent<v8::Object> self;

public:
  ~Weapon();
  Weapon_attrs weapon_attrs;
  Weapon(Weapon_attrs attrs) : weapon_attrs(attrs) { }

  static bool build_attributes(const v8::Arguments& args, Weapon_attrs* attrs);
  static v8::Handle<v8::Value> create(const v8::Arguments& args);
};

} }

#endif // HAT_ENGINE_JAVASCRIPT_WEAPON_SCRIPT_HPP
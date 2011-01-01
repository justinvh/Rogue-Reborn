#include <hat/engine/javascript/weapon.hpp>
#include <hat/v8/easy.hpp>
#include <hat/v8/type_conversion.hpp>
#include <hat/engine/qcommon.h>
#include <hat/engine/qfiles.h>
#include <sstream>
#include <memory>
#include <map>


namespace hat {

namespace {
typedef std::map<int, javascript::Weapon_manager*> Manager_map;
Manager_map managers;
}

bool find_weapon(Weapon_descriptor* descriptor)
{
  javascript::Weapon_manager& manager = *managers[descriptor->player_id];
  if (descriptor->weapon_id != -1) return manager.find_weapon(descriptor->weapon_id, &descriptor->weapon);
  if (descriptor->weapon_name) return manager.find_weapon(descriptor->weapon_name, &descriptor->weapon);
  return false;
}

bool get_loaded_weapon(Weapon_descriptor* descriptor)
{
  javascript::Weapon_manager& manager = *managers[descriptor->player_id];
  return manager.get_loaded_weapon(descriptor->weapon_id, &descriptor->weapon);
}

bool load_weapon(Weapon_descriptor* descriptor)
{
  v8::HandleScope handle_scope;
  Com_Printf("Loading %s.js\n", descriptor->weapon_name);

  std::stringstream ss;
  ss << "javascript/weapons/" << descriptor->weapon_name << ".js";
  const std::string& weapon_script_str = ss.str();
  const char* weapon_script_src = weapon_script_str.c_str();
  Manager_map::iterator it = managers.find(descriptor->player_id);
  javascript::Weapon_manager* manager;
  if (it == managers.end()) {
    manager = managers[descriptor->player_id] = new javascript::Weapon_manager;
  } else {
    manager = it->second;
    if (manager->find_weapon(descriptor->weapon_name, &descriptor->weapon)) {
      return true;
    }
  }
  return manager->load_weapon(descriptor->weapon_name, weapon_script_src, &descriptor->weapon);
}

}

extern "C" {
int JS_LoadWeapon(const int player, const char* weapon)
{
  hat::Weapon_descriptor descriptor(player, weapon);
  return (int)hat::load_weapon(&descriptor);
}

int JS_GetWeaponUniqueID(const int player, const char* weapon)
{
  hat::Weapon_descriptor descriptor(player, weapon);
  if (hat::find_weapon(&descriptor)) {
    return descriptor.weapon->unique_id;
  }
  return 0;
}

int JS_GetWeaponAttributes(const int player, const int weapon_id, const void** attrs)
{
  hat::Weapon_descriptor descriptor(player, weapon_id);
  if (hat::find_weapon(&descriptor)) {
    hat::Weapon_attrs const ** real_attrs = (hat::Weapon_attrs const **)attrs;
    *real_attrs = descriptor.weapon;
    return 1;
  }
  return 0;
}
}
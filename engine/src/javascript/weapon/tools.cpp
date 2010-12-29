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
typedef std::map<int, javascript::Weapon_manager> Manager_map;
Manager_map managers;
}

bool get_loaded_weapon(Weapon_descriptor* descriptor)
{
  javascript::Weapon_manager& manager = managers[descriptor->player_id];
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
  javascript::Weapon_manager& manager = managers[descriptor->player_id];
  bool good = manager.load_weapon(weapon_script_src, &descriptor->weapon);
  if (good) { descriptor->weapon_id = manager.size() - 1; }
  return good;
}

}

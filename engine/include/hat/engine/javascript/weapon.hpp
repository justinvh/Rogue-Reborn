#ifndef HAT_ENGINE_JAVASCRIPT_WEAPON_HPP
#define HAT_ENGINE_JAVASCRIPT_WEAPON_HPP

#include <hat/engine/javascript/weapon/manager.hpp>
#include <hat/engine/javascript/weapon/class.hpp>

namespace hat {
struct Weapon_descriptor {
  Weapon_descriptor(const int player_id, const int weapon_id)
    : player_id(player_id), weapon_id(weapon_id) { }

  Weapon_descriptor(const int player_id, const char* weapon_name)
    : player_id(player_id), weapon_name(weapon_name), 
      weapon_id(-1), weapon(NULL) { }

  const int player_id;
  const char* weapon_name;
  int weapon_id;
  Weapon_attrs const * weapon;
};

/**
 * Loads a weapon from a JavaScript file as specified by:
 *    weapons/${weapon_name}.js
 * Attributes are cached and provided through const access.
 */
bool load_weapon(Weapon_descriptor* weapon);
bool get_loaded_weapon(Weapon_descriptor* descriptor);
}

#endif // HAT_ENGINE_JAVASCRIPT_WEAPON_HPP
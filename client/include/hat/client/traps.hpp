#ifndef HAT_CLIENT_TRAPS_HPP
#define HAT_CLIENT_TRAPS_HPP

#include <hat/engine/javascript/weapon/interface.hpp>

int trap_LoadWeapon(const char* weapon);

/**
 * Retrieves the stored attributes of a given weapon.
 * The return value corresponds to the success of the weapon
 * loading. 
 */
bool trap_GetWeaponAttributes(const int weapon_id,
  hat::Weapon_attrs const ** attrs);

#endif // HAT_CLIENT_TRAPS_HPP
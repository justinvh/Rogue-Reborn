#ifndef HAT_ENGINE_JAVASCRIPT_WEAPON_H
#define HAT_ENGINE_JAVASCRIPT_WEAPON_H

#ifdef __cplusplus
#error "This header is to be included by C files only for porting purposes."
#endif

int JS_LoadWeapon(const int player, const char* weapon);
int JS_GetWeaponUniqueID(const int player, const char* weapon);
int JS_GetWeaponAttributes(const int player, const int weapon_id, const void** attrs);
int JS_GetAndLoadWeaponAttributes(const int player, const char* weapon, const void** attrs);

#endif // HAT_ENGINE_JAVASCRIPT_WEAPON_H

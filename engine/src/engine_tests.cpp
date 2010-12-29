#include <hat/engine/engine_tests.h>
#include <hat/engine/javascript/weapon.hpp>
#include <cassert>

void run_weapon_tests()
{
  hat::Weapon_descriptor descriptor(0, "m9");
  hat::load_weapon(&descriptor);

  hat::Weapon_descriptor verify(0, descriptor.weapon_id);
  hat::get_loaded_weapon(&verify);

  assert(descriptor.weapon == verify.weapon);
}

int hat_tests()
{
  run_weapon_tests();
  return true;
}

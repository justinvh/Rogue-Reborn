#ifndef HAT_ENGINE_JAVASCRIPT_WEAPON_MANAGER_HPP
#define HAT_ENGINE_JAVASCRIPT_WEAPON_MANAGER_HPP

#include <hat/v8/easy.hpp>
#include <hat/engine/javascript/weapon/class.hpp>

namespace hat { namespace javascript {

class Weapon_manager {
  typedef std::vector<Weapon*> Weapon_list;
  Weapon_list primary_weapons;
  Weapon_list secondary_weapons;
  Weapon_list items;
  Weapon_list inventory;
  Weapon* last_weapon;
  v8::Persistent<v8::ObjectTemplate> global_scope;
  v8::Persistent<v8::FunctionTemplate> manager_tmpl;
  v8::Persistent<v8::Object> manager_obj;
  v8::Persistent<v8::Context> global_context;
  v8::Handle<v8::Object> wrap_tmpl(
    v8::Handle<v8::FunctionTemplate>* tmpl, 
    Weapon_manager* e, 
    Object_template_extension extension);
public:
  Weapon_manager();
  ~Weapon_manager();
  void add_weapon(Weapon* weapon);
  bool load_weapon(const char* name_internal, const char* filename, const Weapon_attrs** attrs);
  bool get_loaded_weapon(const int weapon_id, const Weapon_attrs** attrs);
  bool find_weapon(const char* weapon_name, const Weapon_attrs** attrs);
  bool find_weapon(const int unique_id, const Weapon_attrs** attrs);
  size_t size(Weapon_type type) 
  { 
    if (type == PRIMARY_WEAPON) return primary_weapons.size();
    else if (type == SECONDARY_WEAPON) return secondary_weapons.size();
    else if (type == ITEM) return items.size();
    return 0;
  }
  static Weapon_manager& get_active();

  JS_INTERNAL_DEF(Weapon_manager) {
    JS_GETTER(SAFETY);
    JS_GETTER(BOLT_ACTION);
    JS_GETTER(SEMI_AUTOMATIC);
    JS_GETTER(TWO_ROUND_BURST);
    JS_GETTER(THREE_ROUND_BURST);
    JS_GETTER(FULL_AUTO);
  };

};

} }

#endif // HAT_ENGINE_JAVASCRIPT_WEAPON_MANAGER_HPP

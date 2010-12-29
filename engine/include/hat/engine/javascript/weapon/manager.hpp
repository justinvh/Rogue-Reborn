#ifndef HAT_ENGINE_JAVASCRIPT_WEAPON_MANAGER_HPP
#define HAT_ENGINE_JAVASCRIPT_WEAPON_MANAGER_HPP

#include <hat/v8/easy.hpp>
#include <hat/engine/javascript/weapon/class.hpp>

namespace hat { namespace javascript {

class Weapon_manager {
  typedef std::vector<Weapon*> Weapon_list;
  Weapon_list weapons;
  v8::Persistent<v8::ObjectTemplate> global_scope;
  v8::Persistent<v8::FunctionTemplate> manager_tmpl;
  v8::Persistent<v8::Object> manager_obj;
  v8::Persistent<v8::Context> global_context;
  v8::Handle<v8::Object> Weapon_manager::wrap_tmpl(
    v8::Handle<v8::FunctionTemplate>* tmpl, 
    Weapon_manager* e, 
    Object_template_extension extension);
public:
  Weapon_manager();
  ~Weapon_manager();
  void add_weapon(Weapon* weapon);
  bool load_weapon(const char* filename, Weapon_attrs const ** attrs);
  bool get_loaded_weapon(const int weapon_id, Weapon_attrs const ** attrs);
  size_t size() { return weapons.size(); }
  static Weapon_manager& get_active();

};

} }

#endif // HAT_ENGINE_JAVASCRIPT_WEAPON_MANAGER_HPP
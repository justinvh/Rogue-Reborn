#include <algorithm>
#include <memory>

#include <hat/engine/javascript/weapon.hpp>
#include <hat/engine/qcommon.h>

namespace hat { namespace javascript {

namespace {
Weapon_manager* active;
JS_mapping accessors[] = {
  JS_MAP_GETTER(Weapon_manager, SAFETY),
  JS_MAP_GETTER(Weapon_manager, BOLT_ACTION),
  JS_MAP_GETTER(Weapon_manager, SEMI_AUTOMATIC),
  JS_MAP_GETTER(Weapon_manager, TWO_ROUND_BURST),
  JS_MAP_GETTER(Weapon_manager, THREE_ROUND_BURST),
  JS_MAP_GETTER(Weapon_manager, FULL_AUTO),
  { NULL, NULL, NULL }
};


JS_fun_mapping funs[] = {
    JS_CLASS_INVOCATION_CUSTOM(Weapon, "Weapon"),
    { NULL, NULL, NULL } // Signlas the end of the function list
};
}

JS_GETTER_CLASS(Weapon_manager, SAFETY)
{
  return v8::Int32::New(1 << Fire_modes::SAFETY);
}

JS_GETTER_CLASS(Weapon_manager, BOLT_ACTION)
{
  return v8::Int32::New(1 << Fire_modes::BOLT_ACTION);
}

JS_GETTER_CLASS(Weapon_manager, SEMI_AUTOMATIC)
{
  return v8::Int32::New(1 << Fire_modes::SEMI_AUTOMATIC);
}

JS_GETTER_CLASS(Weapon_manager, TWO_ROUND_BURST)
{
  return v8::Int32::New(1 << Fire_modes::TWO_ROUND_BURST);
}

JS_GETTER_CLASS(Weapon_manager, THREE_ROUND_BURST)
{
  return v8::Int32::New(1 << Fire_modes::THREE_ROUND_BURST);
}

JS_GETTER_CLASS(Weapon_manager, FULL_AUTO)
{
  return v8::Int32::New(1 << Fire_modes::FULL_AUTO);
}

void delete_weapon(Weapon* weapon)
{
  delete weapon;
}

Weapon_manager::~Weapon_manager()
{
  manager_tmpl.Dispose();
  manager_tmpl.Dispose();
  manager_obj.Dispose();
  global_context.Dispose();

  // Cleanup our actual weapons and items
  std::for_each(primary_weapons.begin(), primary_weapons.end(), delete_weapon);
  std::for_each(secondary_weapons.begin(), secondary_weapons.end(), delete_weapon);
  std::for_each(items.begin(), items.end(), delete_weapon);
}

Weapon_manager& Weapon_manager::get_active()
{
  assert(active != NULL);
  return *active;
}

Weapon_manager::Weapon_manager()
{
    // An execution scope is necessary for temporary references
    v8::HandleScope execution_scope;
    global_scope = v8::Persistent<v8::ObjectTemplate>::New(v8::ObjectTemplate::New());
    global_scope->SetInternalFieldCount(1);

    // Create our execution context (global context)
    global_context = v8::Context::New(NULL, global_scope);
    v8::Context::Scope context_scope(global_context);

    // Create the gui namespace template and initialize the namespace
    manager_tmpl = v8::Persistent<v8::FunctionTemplate>();
    manager_obj = v8::Persistent<v8::Object>(wrap_tmpl(&manager_tmpl, this, NULL));
    v8::Handle<v8::Object> global_proxy = global_context->Global();
    v8::Handle<v8::Object> global = global_proxy->GetPrototype().As<v8::Object>();
    global->Set(v8::String::New("weapon"), manager_obj);
}

/**
 * Some insight to this method
 * When we add a weapon, we will expect the client and server to have
 * the same setup. So, basically what happens is that if there are any
 * weapons that are out-of-order, then the client and server will
 * have a conflict and the server can settle it however it wants.
 * The size() is used as an id to the weapon attributes as a means
 * of the mask. player_weapons |= 1 << weapon_attrs.id basically.
 */
void Weapon_manager::add_weapon(Weapon* weapon)
{
  int type = weapon->weapon_attrs.type;
  Weapon_list* wl;

  if (type == PRIMARY_WEAPON) wl = &primary_weapons;
  else if (type == SECONDARY_WEAPON) wl = &secondary_weapons;
  else if (type == ITEM) wl = &items;
  else assert(false);

  last_weapon = weapon;
  wl->push_back(weapon);
  inventory.push_back(weapon);
  weapon->weapon_attrs.type_id = wl->size();
  weapon->weapon_attrs.unique_id = inventory.size();
}

bool Weapon_manager::load_weapon(const char* name_internal, const char* filename, const Weapon_attrs** attrs)
{
  v8::HandleScope handle_scope;
  active = this;

  // Open the file
  fileHandle_t handle;
  FS_FOpenFileRead(filename, &handle, qtrue);
  if (!handle) {
    Com_Printf("%s could not be opened.\n", filename);
    return false;
  }

  // Allocations / data
  const unsigned int file_size = FS_ReadFile(filename, NULL);
  std::auto_ptr<char> file_raw(new char[file_size]);
  memset(file_raw.get(), '\0', sizeof(char) * file_size);
  FS_Read(file_raw.get(), file_size, handle);
  FS_FCloseFile(handle);

  // Create our execution context (global context)
  v8::Context::Scope context_scope(global_context);
  v8::Handle<v8::String> script = v8::String::New(file_raw.get(), file_size);

  Com_Printf("Compiling...\n");
  v8::TryCatch compile_try_catch;
  v8::Handle<v8::Script> compiled_script = v8::Script::Compile(script);
  if (compile_try_catch.HasCaught()) {
      v8::Handle<v8::Message> e = compile_try_catch.Message();
      return false;
  }

  // Now run the script and check for errors.
  Com_Printf("Running...\n");
  v8::TryCatch run_try_catch;
  v8::Handle<v8::Value> compiled_result = compiled_script->Run();
  if (run_try_catch.HasCaught()) {
      v8::Handle<v8::Message> e = run_try_catch.Message();
      return false;
  }

  strcpy(last_weapon->weapon_attrs.name_internal, name_internal);
  *attrs = &last_weapon->weapon_attrs;

  Com_Printf("Success...\n");
  return true;
}

bool Weapon_manager::find_weapon(const int unique_id, const Weapon_attrs** attrs)
{
  int actual_id = unique_id - 1;
  if (actual_id >= inventory.size() || actual_id < 0) return false;
  *attrs = &inventory[actual_id]->weapon_attrs;
  return true;
}

bool Weapon_manager::find_weapon(const char* weapon_name, const Weapon_attrs** attrs)
{
  for (
    Weapon_list::const_iterator cit = inventory.begin();
    cit != inventory.end();
  ++cit)
  {
    Weapon* weapon = *cit;
    if (strcmp(weapon->weapon_attrs.name_internal, weapon_name) == 0) {
      *attrs = &weapon->weapon_attrs;
      return true;
    }
  }

  return false;
}

bool Weapon_manager::get_loaded_weapon(const int weapon_id, const Weapon_attrs** attrs)
{
  /*
  if (weapons.size() < weapon_id) {
    return false;
  }

  *attrs = &weapons[weapon_id]->weapon_attrs;
  */
  return true;
}

v8::Handle<v8::Object> Weapon_manager::wrap_tmpl(
    v8::Handle<v8::FunctionTemplate>* tmpl, 
    Weapon_manager* e, 
    Object_template_extension extension)
{
    v8::HandleScope handle_scope;

    if (tmpl->IsEmpty()) {
        (*tmpl) = v8::FunctionTemplate::New();
    }

    (*tmpl)->SetClassName(v8::String::New("Weapon"));
    // We only need to create the template once.
    generate_fun_tmpl(tmpl, accessors, funs, NULL);
    v8::Handle<v8::Function> gui_ctor = (*tmpl)->GetFunction();
    v8::Local<v8::Object> obj = gui_ctor->NewInstance();
    obj->SetInternalField(0, v8::External::New(e));
    return handle_scope.Close(obj);
}

} }

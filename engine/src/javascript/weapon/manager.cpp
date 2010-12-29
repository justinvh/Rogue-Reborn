#include <hat/engine/javascript/weapon.hpp>
#include <hat/engine/qcommon.h>

namespace hat { namespace javascript {

namespace {
Weapon_manager* active;
JS_mapping accessors[] = {
    { NULL, NULL, NULL } // Signals the end of the accessor list
};


JS_fun_mapping funs[] = {
    JS_CLASS_INVOCATION_CUSTOM(Weapon, "Weapon"),
    { NULL, NULL, NULL } // Signlas the end of the function list
};
}

Weapon_manager::~Weapon_manager()
{
  manager_tmpl.Dispose();
  manager_tmpl.Dispose();
  manager_obj.Dispose();
  global_context.Dispose();
  for (
    Weapon_list::iterator cit = weapons.begin();
    cit != weapons.end();
    ++cit)
    {
      delete (*cit);
    }
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

void Weapon_manager::add_weapon(Weapon* weapon)
{
  weapons.push_back(weapon);
}

bool Weapon_manager::load_weapon(const char* filename, Weapon_attrs const ** attrs)
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

  *attrs = &weapons.back()->weapon_attrs;

  Com_Printf("Success...\n");
  return true;
}


bool Weapon_manager::get_loaded_weapon(const int weapon_id, Weapon_attrs const ** attrs)
{
  if (weapons.size() < weapon_id) {
    return false;
  }

  *attrs = &weapons[weapon_id]->weapon_attrs;
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
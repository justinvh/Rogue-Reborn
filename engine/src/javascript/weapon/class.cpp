#include <hat/engine/javascript/weapon.hpp>
#include <hat/engine/qcommon.h>
#include <hat/v8/type_conversion.hpp>

using namespace easy;

namespace hat { namespace javascript {

namespace {

JS_mapping accessors[] = {
  { NULL, NULL, NULL }
};

JS_fun_mapping funs[] = {
  { NULL, NULL, NULL }
};

}

Weapon::~Weapon()
{
  self.Dispose();
  weapon_attrs.cleanup();
}


bool Weapon::build_attributes(const v8::Arguments& args, Weapon_attrs* attrs)
{
  if (args.Length() != 2) {
    return false;
  }

  v8::Handle<v8::String> name = args[0]->ToString();
  if (!smart_convert(name, &attrs->name)) {
    return false;
  }

  v8::Handle<v8::Object> obj = args[1]->ToObject();
  v8::Handle<v8::Value> accuracy_val = obj->Get(v8::String::New("accuracy"));
  v8::Handle<v8::Value> animations_val = obj->Get(v8::String::New("animation"));

  // We can't do anything if the accuracy or animation do not exist
  if (accuracy_val->IsUndefined() || animations_val->IsUndefined()) {
    return false;
  }

  v8::Handle<v8::Object> accuracy = accuracy_val->ToObject();
  v8::Handle<v8::Object> animations = animations_val->ToObject();

  // Load the accuracy information
  bool status = 
    smart_get(accuracy, "crouch", &attrs->accuracy.crouch) &&
    smart_get(accuracy, "run", &attrs->accuracy.run) &&
    smart_get(accuracy, "shuffle", &attrs->accuracy.shuffle) &&
    smart_get(accuracy, "stopped", &attrs->accuracy.stopped) &&
    smart_get(accuracy, "walk", &attrs->accuracy.walk) &&
    smart_get(accuracy, "walk_fast", &attrs->accuracy.walk_fast);

  // Accuracy stuff is mandatory
  if (!status) {
    return false;
  }

  // Fetch out animations
  v8::Local<v8::Array> names = animations->GetPropertyNames();
  for (int i = 0; i < names->Length(); i++) {
    Animation_attrs animation_attrs;
    v8::Local<v8::Value> key = names->Get(i);
    v8::Handle<v8::Object> animation = animations->Get(key)->ToObject();

    // Fetch out animation and time information
    status = smart_get(animation, "animation", &animation_attrs.name);
    smart_get(animation, "time", &animation_attrs.time);

    // The only thing required is really the animation itself
    if (!status) {
       return false;
    }

    // If the sound object exists, then we need to add bindings for it
    v8::Local<v8::Value> sound_val = animation->Get(v8::String::New("sound"));
    if (!sound_val->IsUndefined()) {
      v8::Handle<v8::Array> sounds = sound_val->ToObject().As<v8::Array>();
      for (int j = 0; j < sounds->Length(); j++) {
        Animation_sound_attrs sound_attrs;
        v8::Handle<v8::Object> sound = sounds->Get(j)->ToObject();

        // We need to know when to play the sound and what to play
        status = 
          smart_get(sound, "at", &sound_attrs.at) &&
          smart_get(sound, "play", &sound_attrs.play);

        // Both are required
        if (!status) {
          return false;
        }

        animation_attrs.sounds.push_back(sound_attrs);
      }
    }

    // Animation is good.
    attrs->animations.push_back(animation_attrs);
  }
  return status;
}

v8::Handle<v8::Value> Weapon::create(const v8::Arguments& args)
{
  v8::HandleScope handle_scope;

  static v8::Persistent<v8::FunctionTemplate> weapon_tmpl;
  Com_Printf("Weapon::Create() - Weapon() is being created\n");

  // Try to build the arguments. If an exceptions is caught, then we
  // need to rethrow it so the GUI can catch it.
  Weapon_attrs weapon_attrs;

  if (!build_attributes(args, &weapon_attrs)) {
    weapon_attrs.cleanup();
    return v8::Exception::Error(v8::String::New("Failed to build attributes."));
  }

  // Now wrap the rest of the image
  Weapon* weapon = new Weapon(weapon_attrs);
  Com_Printf("Weapon::Create() - Weapon has been initialized and created.\n");

  if (weapon_tmpl.IsEmpty()) {
    weapon_tmpl = v8::Persistent<v8::FunctionTemplate>::New(v8::FunctionTemplate::New());
    generate_fun_tmpl(&weapon_tmpl, accessors, funs, NULL);
    weapon_tmpl->SetClassName(v8::String::New("Weapon"));
  }

  v8::Handle<v8::Function> ctor = weapon_tmpl->GetFunction();
  v8::Local<v8::Object> obj = ctor->NewInstance();
  obj->SetInternalField(0, v8::External::New(weapon));
  weapon->self = v8::Persistent<v8::Object>::New(handle_scope.Close(obj));
  Weapon_manager::get_active().add_weapon(weapon);
  return weapon->self;
}

} }
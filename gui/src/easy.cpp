#include <hat/gui/easy.hpp>
#include <hat/gui/image.hpp>

namespace hat {

/*
Accessors provide a method getting and setting variables associated with
the class binding to an object. Functions are methods that can be called
from the JavaScript that have some sort of impact on the engine itself.

An internal method is one that will be in the *.internal.* namespace.
*/

void add_accessors_and_fun_to_fun_tmpl(
    const JS_mapping* accessors,
    const JS_fun_mapping* funs,
    v8::Handle<v8::FunctionTemplate>* tmpl,
    const bool internal_methods)
{
    v8::Handle<v8::ObjectTemplate> proto = (*tmpl)->PrototypeTemplate();
    v8::Handle<v8::ObjectTemplate> inst = (*tmpl)->InstanceTemplate();
    for (int i = 0; accessors[i].name != NULL; i++) {
        if (internal_methods && !accessors[i].is_internal)
            continue;

        inst->SetAccessor(
            v8::String::NewSymbol(accessors[i].name), 
            accessors[i].getter,
            accessors[i].setter);
    }

    for (int i = 0; funs[i].name != NULL; i++) {
        if (funs[i].is_internal && !internal_methods)
            continue;
        
        proto->Set(
            v8::String::NewSymbol(funs[i].name),
            v8::FunctionTemplate::New(funs[i].fun));
    }
}

/*
Createing a template is essentially creating a new object template that
reserves a field for the to-be-determined class that will be binded in
the internal pointer.
*/
v8::Handle<v8::FunctionTemplate> generate_fun_tmpl(
    v8::Handle<v8::FunctionTemplate>* tmpl,
    const JS_mapping* accessors, 
    const JS_fun_mapping* funs,
    const Extension_list* extension_list)
{
    v8::Handle<v8::ObjectTemplate> inst = (*tmpl)->InstanceTemplate();
    inst->SetInternalFieldCount(2);

    add_accessors_and_fun_to_fun_tmpl(accessors, funs, tmpl, false);
    add_accessors_and_fun_to_fun_tmpl(accessors, funs, tmpl, true);

    if (extension_list) {
        for (auto ecit = extension_list->begin();
            ecit != extension_list->end();
            ++ecit)
        {
            add_accessors_and_fun_to_fun_tmpl(ecit->first, ecit->second, tmpl, false);
            add_accessors_and_fun_to_fun_tmpl(ecit->first, ecit->second, tmpl, false);
        }
    }

    return *tmpl;
}

}
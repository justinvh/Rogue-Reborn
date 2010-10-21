#include <hat/gui/easy.hpp>

namespace hat {

/*
Accessors provide a method getting and setting variables associated with
the class binding to an object. Functions are methods that can be called
from the JavaScript that have some sort of impact on the engine itself.

An internal method is one that will be in the *.internal.* namespace.
*/

void add_accessors_and_fun_to_tmpl(
	const JS_mapping* accessors,
	const JS_fun_mapping* funs,
	v8::Handle<v8::ObjectTemplate>* tmpl,
	const bool internal_methods)
{
	for (int i = 0; accessors[i].name != NULL; i++) {
		if (internal_methods && !accessors[i].is_internal)
			continue;

		(*tmpl)->SetAccessor(
			v8::String::NewSymbol(accessors[i].name), 
			accessors[i].getter,
			accessors[i].setter);
	}

	for (int i = 0; funs[i].name != NULL; i++) {
		if (funs[i].is_internal && !internal_methods)
			continue;
		
		(*tmpl)->Set(
			v8::String::NewSymbol(funs[i].name),
			v8::FunctionTemplate::New(funs[i].fun));
	}
}

/*
Createing a template is essentially creating a new object template that
reserves a field for the to-be-determined class that will be binded in
the internal pointer.
*/
v8::Handle<v8::ObjectTemplate> generate_tmpl(
	const JS_mapping* accessors, 
	const JS_fun_mapping* funs)
{
	v8::Handle<v8::ObjectTemplate> element_public = v8::ObjectTemplate::New();
	v8::Handle<v8::ObjectTemplate> element_internal = v8::ObjectTemplate::New();

	element_public->SetInternalFieldCount(1);

	add_accessors_and_fun_to_tmpl(accessors, funs, &element_public, false);
	add_accessors_and_fun_to_tmpl(accessors, funs, &element_internal, true);

	element_public->Set("internal", element_internal);

	return element_public;
}

}
/*
Rogue Reborn GUI "Gui" source from src/gui.cpp 

Copyright 2010 Justin Bruce Van Horne.  All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include <memory>
#include <fstream>
#include <map>
#include <string>
#include <sstream>
#include <v8.h>
#include <hat/gui/gui.hpp>
#include <hat/gui/element.hpp>
#include <hat/engine/q_shared.h>
#include <hat/gui/ui_public.h>

#define EXCEPTION(file, line, message)													\
{																						\
	std::stringstream new_exception(std::stringstream::in | std::stringstream::out);	\
	new_exception << message;															\
    current_exception = Gui_exception(file, line, new_exception.str().c_str());			\
}

namespace hat {

namespace {
/*
This is a list of functions made available through the Gui class.
Any additional functions that are added to the class should be added here.
They will be looped over and added to the function list when a template
is constructed.

If they are marked as `internal` (denoted by true) then they will be
available in the `gui.internal` namespace.
*/
JS_mapping accessors[] = {
	{ NULL, NULL, NULL } // Signals the end of the accessor list
};

/*
This is a list of functions made availabe through the Gui class.
Any additional functions that are addeed tot he class should be added here.
They will be looped over and added to the function list when a template
is constructued.
*/
JS_fun_mapping funs[] = {
	JS_FUN_MAP_INTERNAL(Gui, setup_menus),
	{ NULL, NULL, NULL } // Signlas the end of the function list
};

/*
This is a mapping for menus and corresponding scripts.
*/
typedef std::map<int, std::string> Menu_mapping;
typedef std::map<std::string, int> Engine_mapping;

const char* UIMENUS[] = {
	NULL,
	"UIMENU_MAIN",
	"UIMENU_INGAME",
	"UIMENU_TEAM",
	"UIMENU_POSTGAME"
};

Menu_mapping available_menus;
Engine_mapping engine_menus;

}

Gui::Gui(const char* js_file)
{
	// Static menus that are a must
	if (engine_menus.empty()) {
		engine_menus["main"] = UIMENU_MAIN;
		engine_menus["ingame"] = UIMENU_INGAME;
		engine_menus["team"] = UIMENU_TEAM;
		engine_menus["postgame"] = UIMENU_POSTGAME;
	}

	// Read the file into a buffer for script conversion
	std::ifstream file(js_file, std::ios::in | std::ios::binary);
	if (!file) {
		EXCEPTION(__FILE__, __LINE__, js_file << " could not be opened/read\n");
		return;
	}
	file.seekg(0, std::ios::end);
	const unsigned int file_size = file.tellg();
	std::auto_ptr<char> file_raw(new char[file_size]);
	file.seekg(0, std::ios::beg);
	file.read(file_raw.get(), file_size);
	file.close();

	// An execution scope is necessary for temporary references
	v8::HandleScope execution_scope;
	v8::Handle<v8::String> script = v8::String::New(file_raw.get(), file_size);
	global_scope = v8::ObjectTemplate::New();

	// Create our execution context (global context)
	v8::Handle<v8::Context> execution_context = v8::Context::New(NULL, global_scope);
	global_context = v8::Persistent<v8::Context>::New(execution_context);
	v8::Context::Scope context_scope(execution_context);

	// Create the gui namespace template and initialize the namespace
	gui_tmpl = v8::Persistent<v8::ObjectTemplate>();
	wrap_tmpl(&gui_tmpl, this, NULL);
	gui_ns = gui_tmpl->NewInstance();
	global_context->Global()->SetPointerInInternalField(0, this);
	global_context->Global()->Set(v8::String::New("gui"), gui_ns);

	// Setup the error handling and compile the current script
	v8::TryCatch compile_try_catch;
	v8::Handle<v8::Script> compiled_script = v8::Script::Compile(script);
	if (compile_try_catch.HasCaught()) {
	}

	// Now run the script and check for errors.
	v8::TryCatch run_try_catch;
	v8::Handle<v8::Value> compiled_result = compiled_script->Run();
	if (run_try_catch.HasCaught()) {
		v8::Handle<v8::Message> e = run_try_catch.Message();
		const char* msg = *v8::String::Utf8Value(e->Get());
	}

	// Look for the think fun. If there isn't one, oh well.
	v8::Handle<v8::String> think_name = v8::String::New("think");
	v8::Handle<v8::Value> think_val = global_context->Global()->Get(think_name);
	if (think_val->IsFunction())  {
		v8::Handle<v8::Function> think_fun = v8::Handle<v8::Function>::Cast(think_val);
		gui_think_fun = v8::Persistent<v8::Function>::New(think_fun);
	}
}

void Gui::think(const Gui_kbm& kbm_state)
{
}

void Gui::shutdown()
{
}

bool Gui::in_exception_state()
{
	return current_exception.exception_line != -1;
}

const Gui_exception& Gui::exception()
{
	return current_exception;
}


const char* Gui::engine_menu_exists(const int menu)
{
	auto menu_name = available_menus.find(menu);
	if (menu_name == available_menus.end()) {
		return NULL;
	}
	return menu_name->second.c_str();
}

const char* Gui::engine_menu_name(const int menu)
{
	return UIMENUS[menu];
}

JS_FUN_CLASS(Gui, setup_menus)
{
	v8::Local<v8::Object> menu_obj = args[0]->ToObject();
	v8::Local<v8::Array> menu_to_be_set = menu_obj->GetPropertyNames();

	for (int i = 0; i < menu_to_be_set->Length(); i++) {
		const v8::Local<v8::String> real_key = menu_to_be_set->Get(i)->ToString();
		const std::string key = *v8::String::Utf8Value(real_key);
		const std::string val = *v8::String::Utf8Value(menu_obj->Get(real_key));

		auto cit = engine_menus.find(key);
		if (cit == engine_menus.end()) {
			// Do some warning
			continue;
		}

		available_menus[cit->second] = val;
	}

	return v8::Undefined();
}

/*
The only way to expose classes to JavaScript natively is to wrap the
existing object. To properly due this, we have to ensure that the object
remains in memory during the execution of script. This responsibility
is dictated by the container, which in this case is the active Gui.
*/
v8::Handle<v8::Object> Gui::wrap_tmpl(
	v8::Handle<v8::ObjectTemplate>* tmpl, 
	Gui* e, 
	Object_template_extension extension)
{
	// We only need to create an "image" of the template once.
	if (tmpl->IsEmpty()) {
		v8::Handle<v8::ObjectTemplate> result = generate_tmpl(accessors, funs);
		result->SetInternalFieldCount(1);
		if (extension != NULL) extension(&result);
		*tmpl = v8::Persistent<v8::ObjectTemplate>::New(result);
	}

	// The active Gui is all we care about

	v8::Handle<v8::External> class_ptr = v8::External::New(e);
	v8::Handle<v8::Object> result = (*tmpl)->NewInstance();
	result->SetInternalField(0, class_ptr);
	return result;
}

}
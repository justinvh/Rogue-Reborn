/*
Rogue Reborn GUI "Gui" definition from hat/gui/gui.hpp

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

#ifndef HAT_GUI_GUI_HPP
#define HAT_GUI_GUI_HPP

#include <map>
#include <vector>
#include <hat/gui/easy.hpp>

namespace hat {

class Element;

/*
A Gui can have a state, defined as a mix of buttons, mouse positions, and
button states, that affect how different elements interact with the user.
*/
struct Gui_kbm
{
	int mx, my, button;
	bool down, held;
};

/*
A Gui exception describing what happened and how.
*/
struct Gui_exception
{
	Gui_exception() 
		: exception_file(NULL), exception_message(NULL), exception_line(-1) { }
	Gui_exception(const char* file, int line, const char* message)
		: exception_file(file), exception_message(message), exception_line(line) { }

	const char* exception_file;
	const char* exception_message;
	int exception_line;
};



/*
The Gui class is a manager class of scripts. There can be multiple Gui
instances running, but event processing is sequential in that there is a
list of sequential Guis running, not concurrent Guis. The reason is that
we can have multiple threads of V8 running, but only one thread can be
active at a time.
*/
class Gui
{
public:
	/*
	A Gui can not exist without being initialized without a JavaScript
	file being intialized. This constructor will throw exceptions if
	anything goes wrong. 
	
	Exceptions thrown:
		o hat::Gui_compile_error
			If the intial compilations goes bad. Unrecoverable.
		o hat::Gui_execution_error
			If the initial run of the script goes bad. Unrecoverable.
		o std::ifstream::failure
			Loading the file goes bad. Unrecoverable.
	*/
	Gui(const char* js_file);

	JS_INTERNAL_DEF(Gui)
	{
		/*
		These are internal methods and are available via gui.internal.*
		*/
		JS_FUN(setup_menus);
	};

	/*
	Tells the Gui instance that it is the master.
	*/
	void add(Element* element);

	/*
	This is a method that is called 1 time per frame.
	It will run the think method for all the elements in the object
	array given that they are active.
	*/
	void think(const Gui_kbm& kbm_state);

	/*
	This method will cause the gui to unload all the elements that the
	Gui is responsible for. It will also render the object useless.
	*/
	void shutdown();

	/*
	If the Gui went into an exception state, then this will be true.
	*/
	bool in_exception_state();

	/*
	Return the exception that occured.
	*/
	const Gui_exception& exception();

	/*
	Returns the name of the menu if the menu exists.
	*/
	static const char* engine_menu_exists(const int menu);
	static const char* engine_menu_name(const int menu);
private:
	/*
	It doesn't make sense to declare a Gui without a JavaScript file.
	Nor does it make sense to allow accessible methods in the case that
	the JavaScript file does not properly compile. However, we do need
	container libraries to be able to have Gui objects. So, we allow those
	on a per-container basis.
	*/
	Gui() { }
	friend struct Gui_state;

	typedef std::vector<Element*> Element_list;
	Element_list pending_elements, available_elements;

	/*
	Wraps an instance of Element and transforms it into an object that
	is usable by JavaScript. The object can then be unwrapped at any time.
	*/
	static v8::Handle<v8::Object> wrap_tmpl(v8::Handle<v8::ObjectTemplate>* tmpl,
		Gui* e, Object_template_extension extension);

	/*
	In general we need the following:
	- A global scope
	- An object template for the `gui` namespace.
	- The `gui` namespace itself.
	- A sort-of "entry point" to the JavaScript file
	*/
	v8::Handle<v8::ObjectTemplate> global_scope;
	v8::Persistent<v8::ObjectTemplate> gui_tmpl;
	v8::Handle<v8::Object> gui_ns;
	v8::Persistent<v8::Function> gui_think_fun;
	Gui_exception current_exception;
public:
	v8::Persistent<v8::Context> global_context;
};

}

#endif // HAT_GUI_GUI_HPP
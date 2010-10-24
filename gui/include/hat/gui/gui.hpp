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
#include <string>
#include <hat/gui/easy.hpp>

namespace hat {

class Element;

/*
A Gui can have a state, defined as a mix of buttons, mouse positions, and
button states, that affect how different elements interact with the user.
*/
struct Gui_kbm
{
    int mx, my, key;
    bool down, held;

    void reset_keys()
    {
        key = 0;
        down = held = false;
    }

    void reset_all()
    {
        mx = my = key = 0;
        down = held = false;
    }
};

/*
A Gui exception describing what happened and how.
*/
struct Gui_exception
{
    Gui_exception() : line(-1) { }

    Gui_exception(const std::string& file, int line, const std::string& message)
        : file(file), line(line), message(message) { }

    std::string file;
    std::string message;
    int line;
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
    ~Gui();

    JS_INTERNAL_DEF(Gui)
    {
        /*
        These are internal methods and are available via gui.internal.*
        */
        JS_FUN(setup_menus);

        /*
        These are public methods and are available via gui.*
        */
        JS_FUN(log);
        JS_FUN(toString);
        JS_FUN(think);
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
    Adds an element to the current GUI.
    */
    void add_element(Element* element)
    {
        pending_elements.push_back(element);
    }

    /*
    Returns the name of the menu if the menu exists.
    */
    static void engine_menu_clear();
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

    Think_list gui_think_funs;
    Gui_exception current_exception;
    Gui_kbm last_kbm_state;
private:
    void think_fun();
public:
    v8::Persistent<v8::Context> global_context;
    const char* js_filename;
};

}

#endif // HAT_GUI_GUI_HPP
/*
Rogue Reborn GUI "Element" source from src/element.cpp

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

#include <hat/gui/element.hpp>
#include <hat/gui/gui.hpp>

// We want to avoid the double to float issues between V8 and C++
#pragma warning(disable : 4244)

namespace hat {

namespace {
/*
    This is a list of accessors made available through the Element class.
    Any additional elements that are added to the class should be added here.
    They will be looped over and added to the accessor list when a template
    is constructed.
    */
    JS_mapping accessors[] = {
        JS_MAP(Element, id),
        JS_MAP(Element, x),
        JS_MAP(Element, y),
        JS_MAP(Element, active),
        JS_MAP(Element, width),
        JS_MAP(Element, height),
        JS_MAP(Element, parent),
        JS_MAP(Element, border),
        JS_MAP(Element, border_left),
        JS_MAP(Element, border_right),
        JS_MAP(Element, border_top),
        JS_MAP(Element, border_bottom),
        { NULL, NULL, NULL } // Signals the end of the accessor list
    };

    /*
    This is a list of functions made availabe through the Element class.
    Any additional functions that are addeed tot he class should be added here.
    They will be looped over and added to the function list when a template
    is constructued.
    */
    JS_fun_mapping funs[] = {
        JS_FUN_MAP(Element, think),
        { NULL, NULL, NULL } // Signlas the end of the function list
    };
}

/*
Ids are unique to the Element. They are hashed and provide quick access
to an element through a lookup. If a value is a string, then it is
converted to a hash. If it is an integer, then it is used as the hash.
*/
JS_SETTER_CLASS(Element, id)
{
    Element* e = unwrap<Element>(info.Holder());
    e->element_attrs.id = value->Int32Value();
    e->element_attrs.modified = true;
}

/*
The position of the element in a arbitrary precision number between
the gui resolution (1024x768). The value from JavaScript will be
double precision, but it will be converted to a float.
*/
JS_SETTER_CLASS(Element, x)
{
    Element* e = unwrap<Element>(info.Holder());
    e->element_attrs.x = (float)value->NumberValue();
    e->element_attrs.modified = true;
}

/*
The position of the element in a arbitrary precision number between
the gui resolution (1024x768). The value from JavaScript will be
double precision, but it will be converted to a float.
*/
JS_SETTER_CLASS(Element, y)
{
    Element* e = unwrap<Element>(info.Holder());
    e->element_attrs.y = (float)value->NumberValue();
    e->element_attrs.modified = true;
}

/*
An element that is active is an element that will be called once per
frame. If an element is inactive then it will now be called at all.
In this case, another condition will have to trigger the active-ness of
the element to get it to "think" again.
*/
JS_SETTER_CLASS(Element, active)
{
    Element* e = unwrap<Element>(info.Holder());
    e->element_attrs.active = value->BooleanValue();
    e->element_attrs.modified = true;
}

JS_SETTER_CLASS(Element, height)
{
    Element* e = unwrap<Element>(info.Holder());
    e->element_attrs.height = value->NumberValue();
    e->element_attrs.modified = true;
}

JS_SETTER_CLASS(Element, width)
{
    Element* e = unwrap<Element>(info.Holder());
    e->element_attrs.width = value->NumberValue();
    e->element_attrs.modified = true;
}

JS_SETTER_CLASS(Element, parent)
{
    Element* e = unwrap<Element>(info.Holder());
    Element* p = unwrap<Element>(value->ToObject());
    if (!p) {
        v8::ThrowException(v8::Exception::TypeError(v8::String::New("Expected an instance of a derived `Element` class.")));
        return;
    }

    e->element_attrs.modified = true;
    e->element_attrs.parent = p;
}

JS_SETTER_CLASS(Element, border)
{
    Element* e = unwrap<Element>(info.Holder());
    e->element_attrs.modified = true;
}

JS_SETTER_CLASS(Element, border_left)
{
    Element* e = unwrap<Element>(info.Holder());
    e->element_attrs.modified = true;
}

JS_SETTER_CLASS(Element, border_right)
{
    Element* e = unwrap<Element>(info.Holder());
    e->element_attrs.modified = true;
}

JS_SETTER_CLASS(Element, border_top)
{
    Element* e = unwrap<Element>(info.Holder());
    e->element_attrs.modified = true;
}

JS_SETTER_CLASS(Element, border_bottom)
{
    Element* e = unwrap<Element>(info.Holder());
    e->element_attrs.modified = true;
}

/*
Ids are unique to the Element. They are hashed and provide quick access
to an element through a lookup. When an id is requested via a getter, then
the hash value will be returned.
*/
JS_GETTER_CLASS(Element, id)
{
    Element* e = unwrap<Element>(info.Holder());
    return v8::String::New(e->element_attrs.id.c_str());
}

/*
The position of the element in a arbitrary precision number between
the gui resolution (1024x768). Since the value going into the class
does not hold a double precision, it will be returned back with not
all of its precision. This means that you can lose precision in between
getting and setting.
*/
JS_GETTER_CLASS(Element, x)
{
    Element* e = unwrap<Element>(info.Holder());
    return v8::Number::New(e->element_attrs.x);
}

/*
The position of the element in a arbitrary precision number between
the gui resolution (1024x768). Since the value going into the class
does not hold a double precision, it will be returned back with not
all of its precision. This means that you can lose precision in between
getting and setting.
*/
JS_GETTER_CLASS(Element, y)
{
    Element* e = unwrap<Element>(info.Holder());
    return v8::Number::New(e->element_attrs.y);
}

/*
An element that is active is an element that will be called once per
frame. If an element is inactive then it will now be called at all.
In this case, another condition will have to trigger the active-ness of
the element to get it to "think" again.
*/
JS_GETTER_CLASS(Element, active)
{
    Element* e = unwrap<Element>(info.Holder());
    return v8::Boolean::New(e->element_attrs.active);
}

/*
*/
JS_GETTER_CLASS(Element, parent)
{
    Element* e = unwrap<Element>(info.Holder());
    if (e->element_attrs.parent) {
        return e->element_attrs.parent->self;
    }
    return v8::Undefined();
}

/*
*/
JS_GETTER_CLASS(Element, height)
{
    Element* e = unwrap<Element>(info.Holder());
    return v8::Number::New(e->element_attrs.height);
}

/*
*/
JS_GETTER_CLASS(Element, width)
{
    Element* e = unwrap<Element>(info.Holder());
    return v8::Number::New(e->element_attrs.width);
}

/*
*/
JS_GETTER_CLASS(Element, border)
{
    Element* e = unwrap<Element>(info.Holder());
    return v8::Undefined();
}

/*
*/
JS_GETTER_CLASS(Element, border_left)
{
    Element* e = unwrap<Element>(info.Holder());
    return v8::Undefined();
}

JS_GETTER_CLASS(Element, border_right)
{
    Element* e = unwrap<Element>(info.Holder());
    return v8::Undefined();
}

JS_GETTER_CLASS(Element, border_top)
{
    Element* e = unwrap<Element>(info.Holder());
    return v8::Undefined();
}

JS_GETTER_CLASS(Element, border_bottom)
{
    Element* e = unwrap<Element>(info.Holder());
    return v8::Undefined();
}

/*
A new think() call has been made. This means that we are either calling
think without arguments (implied to call the methods themselves) or we
are adding a new function to the list.
*/
JS_FUN_CLASS(Element, think)
{
    v8::Local<v8::Object> menu_obj = args[0]->ToObject();
    Element* e = unwrap<Element>(args.Holder());

    if (!e) {
        return v8::Exception::Error(v8::String::New("Element has become detached?"));
    }

    // We have an actual argument to the function, we are going to need
    // to check if the argument is a function and if it is, we append
    // the function to the function list
    if (args.Length() > 0) {
        v8::Handle<v8::Value> think_val = args[0];
        if (think_val->IsFunction()) {
            e->element_attrs.modified = true;
            v8::Handle<v8::Function> think_fun = v8::Handle<v8::Function>::Cast(think_val);
            e->element_attrs.think_funs.push_back(v8::Persistent<v8::Function>::New(think_fun));
            return args.Holder();
        } else {
            return v8::Exception::TypeError(v8::String::New("Expected a function, but got something else."));
        }
    }

    e->think_fun(0);
    return args.Holder();
}

bool Element::check_bounds(int mx, int my)
{
    float top = element_attrs.y;
    float bottom = element_attrs.y + element_attrs.height;
    float left = element_attrs.x;
    float right = element_attrs.x + element_attrs.width;

    if (element_attrs.parent) {
        const Element_attributes& p = element_attrs.parent->element_attrs;
        left += p.x;
        right += p.x;
        top += p.y;
        bottom += p.y;
    }

    if (mx < left || mx > right || my < top || my > bottom) {
        return false;
    }

    return true;
}


/*
*/
void Element::think_fun(int ms)
{
    if (!element_attrs.think_funs.size()) return;

    // Important, we always need to know where we are at in terms of the
    // execution scope and context scope.
    v8::HandleScope execution_scope;

    // These will change; they are the KBM state.
    v8::Handle<v8::Object> ms_argv = v8::Object::New();
    ms_argv->Set(v8::Int32::New(0), v8::Int32::New(ms));

    // The two arguments to the GUI think() are the timer and kbm
    v8::Handle<v8::Value> argvs[1] = { ms_argv };

    // We're in the case that the args don't exist, so now we are calling 
    // the various methods of the think()
    v8::TryCatch run_try_catch;
    for (Function_list::const_iterator tci = element_attrs.think_funs.begin();
        tci != element_attrs.think_funs.end();
        ++tci)
    {
        (*tci)->Call(self, 1, argvs);
    }
}

bool Element::build_attributes(const v8::Arguments& args, Element_attributes* ea)
{
    if (args.Length() > 1 || !args[0]->IsObject()) {
        return true;
    }

    v8::Local<v8::Object> arg_obj = args[0]->ToObject();

    // Get the various values for this structure
    v8::Local<v8::Value> id     = arg_obj->Get(v8::String::New("id"));
    v8::Local<v8::Value> active	= arg_obj->Get(v8::String::New("active"));
    v8::Local<v8::Value> x      = arg_obj->Get(v8::String::New("x"));
    v8::Local<v8::Value> y      = arg_obj->Get(v8::String::New("y"));
    v8::Local<v8::Value> width  = arg_obj->Get(v8::String::New("width"));
    v8::Local<v8::Value> height = arg_obj->Get(v8::String::New("height"));
    v8::Local<v8::Value> bg     = arg_obj->Get(v8::String::New("background-color"));
    v8::Local<v8::Value> parent = arg_obj->Get(v8::String::New("parent"));

    // Borders have two variations
    v8::Local<v8::Value> border  = arg_obj->Get(v8::String::New("border"));
    v8::Local<v8::Value> borderl = arg_obj->Get(v8::String::New("border-left"));
    v8::Local<v8::Value> borderr = arg_obj->Get(v8::String::New("border-right"));
    v8::Local<v8::Value> bordert = arg_obj->Get(v8::String::New("border-top"));
    v8::Local<v8::Value> borderb = arg_obj->Get(v8::String::New("border-bottom"));

    // A lot of values have to be interpreted
    std::string bg_str;

    // Assign values; the constructor of the Element_attributes is
    // responsible for assigning initial values, so we don't do it here.
    JS_BA_FLOAT_REQUIRED(ea->x, x, "Expected a number for `x`.");
    JS_BA_FLOAT_REQUIRED(ea->y, y, "Expected a number for `y`.");
    JS_BA_FLOAT_REQUIRED(ea->width, width, "Expected a number for `width`.");
    JS_BA_FLOAT_REQUIRED(ea->height, height, "Expected a number for `height`.");

    // These are values that we can live without
    JS_BA_STR(ea->id, id, "Expected a string for `id`.");
    JS_BA_INT(bg_str, bg, "Expected a string for `background-color`.");

    ea->modified = true;

    return true;
}


/*
The only way to expose classes to JavaScript natively is to wrap the
existing object. To properly due this, we have to ensure that the object
remains in memory during the execution of script. This responsibility
is dictated by the container, which in this case is the active Gui.
*/
v8::Handle<v8::Object> Element::wrap_tmpl(
    v8::Handle<v8::FunctionTemplate>* tmpl, 
    Element* e, 
    const Extension_list& extension_list)
{
    v8::HandleScope handle_scope;

    if (tmpl->IsEmpty()) {
        (*tmpl) = v8::FunctionTemplate::New();
    }

    (*tmpl)->SetClassName(v8::String::New(e->name));

    // We only need to create the template once.
    generate_fun_tmpl(tmpl, accessors, funs, &extension_list);

    // The active Gui is all we care about
    v8::Handle<v8::Function> gui_ctor = (*tmpl)->GetFunction();
    v8::Local<v8::Object> obj = gui_ctor->NewInstance();
    obj->SetInternalField(0, v8::External::New(e));
    e->self = v8::Persistent<v8::Object>::New(handle_scope.Close(obj));
    Gui* gui = unwrap_global_pointer<Gui>(0);
    gui->add_element(e);
    return e->self;
    
}

}

#pragma warning(default : 4244)
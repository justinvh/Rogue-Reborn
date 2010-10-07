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
	{ NULL, NULL, NULL } // Signals the end of the accessor list
};

/*
This is a list of functions made availabe through the Element class.
Any additional functions that are addeed tot he class should be added here.
They will be looped over and added to the function list when a template
is constructued.
*/
JS_fun_mapping funs[] = {
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
}

/*
Ids are unique to the Element. They are hashed and provide quick access
to an element through a lookup. When an id is requested via a getter, then
the hash value will be returned.
*/
JS_GETTER_CLASS(Element, id)
{
	Element* e = unwrap<Element>(info.Holder());
	return v8::Int32::New(e->element_attrs.id);
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
The only way to expose classes to JavaScript natively is to wrap the
existing object. To properly due this, we have to ensure that the object
remains in memory during the execution of script. This responsibility
is dictated by the container, which in this case is the active Gui.
*/
v8::Handle<v8::Object> Element::wrap_tmpl(
	v8::Handle<v8::ObjectTemplate>* tmpl, 
	Element* e, 
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
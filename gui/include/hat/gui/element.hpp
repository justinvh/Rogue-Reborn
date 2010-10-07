/*
Rogue Reborn GUI "Element" definition from hat/gui/element.hpp

Copyright 20010 Justin Bruce Van Horne.  All Rights Reserved.

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

#ifndef HAT_GUI_ELEMENT_HPP
#define HAT_GUI_ELEMENT_HPP

#include <hat/gui/easy.hpp>

namespace hat {

class Element;

/*
Defines attributes of a single border in terms of width and height.
This means that you can have 4 different borders (or more) depending
on the method of its use.
*/
struct Border_attributes
{
	float width;
	float color[4];

	enum Border_side {
		TOP,
		RIGHT,
		DOWN,
		LEFT,
	};
};

/*
The basic element has at the mininum these attributes. No element can
exist on a canvas without having access to these attributes. You are
guarenteed to have x, y, width, and height set. These values will be
initialized to 0.
*/
struct Element_attributes
{
	Element_attributes() :
		x(0), y(0), width(0), height(0), id(0) { }
	
	int id;
	bool active;
	float x, y, width, height;
	float background_color[3];
	Border_attributes borders[4];
	Element* parent;

};

/*
The basic element class. All other objects that are rendered directly
on the cavas are derived of this class. A list of this base class will be
stored in the Gui manager and dynamically casted against other classes
to represent any sort of per-class markup.
*/
class Element
{
public:
	virtual ~Element() { };	

	/*
	A think method is a method that is called 1 per frame.
	*/
	virtual void think() = 0;

	/*
	This is an internal structure that is represented by a few macros
	to make creating setters and getters easier. There is really no
	reason to access these methods directly. They are set as accessors
	and made available through the interface between V8 and JavaScript.
	*/
	JS_INTERNAL_DEF(Element)
	{
		/*
		Getters and setters for the Element_attributes.
		*/
		JS_GETTER_AND_SETTER(id);
		JS_GETTER_AND_SETTER(x);
		JS_GETTER_AND_SETTER(y);
		JS_GETTER_AND_SETTER(width);
		JS_GETTER_AND_SETTER(height);
		JS_GETTER_AND_SETTER(background_color);
		JS_GETTER_AND_SETTER(border);
		JS_GETTER_AND_SETTER(parent);
		JS_GETTER_AND_SETTER(active);
	};

	/*
	Wraps an instance of Element and transforms it into an object that
	is usable by JavaScript. The object can then be unwrapped at any time.
	*/
	static v8::Handle<v8::Object> wrap_tmpl(v8::Handle<v8::ObjectTemplate>* tmpl,
		Element* e, Object_template_extension extension);
protected:
	Element_attributes element_attrs;
};

}

#endif // HAT_GUI_ELEMENT_HPP
/*
Rogue Reborn GUI "Image" definition from hat/gui/image.hpp

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

#ifndef HAT_GUI_IMAGE_HPP
#define HAT_GUI_IMAGE_HPP

#include <hat/gui/element.hpp>
#include <hat/gui/eventful.hpp>

// Yeah, yeah, diamond inheritance, I know.
#pragma warning(disable : 4250)

namespace hat {

struct Image_attributes
{
    Image_attributes() 
        : src_handle(-1), s1(0), t1(0), s2(1), t2(1) { }
    std::string src;
    int src_handle;
    float scaled_x, scaled_y, scaled_h, scaled_w;
    float s1, s2, t1, t2;
};

class Image :
    public virtual Element
{
public:
    Image(const Element_attributes& element_attributes,
        const Image_attributes& image_attributes);

    virtual ~Image() { }

    /*
    The think() routine is called 60 time per frame.
    */
    virtual void think(int ms);

    /*
    This is an internal structure that is represented by a few macros
    to make creating setters and getters easier. There is really no
    reason to access these methods directly. They are set as accessors
    and made available through the interface between V8 and JavaScript.
    */
    JS_INTERNAL_DEF(Image)
    {
        /*
        Getters and setters for the Image_attributes.
        */
        JS_GETTER_AND_SETTER(src);
    };

    /*
    Create a new instance of Image whenever a matching JavaScript object
    is instanitiated. This method will only be called internally.
    */
    static v8::Handle<v8::Value> create(const v8::Arguments& args);

protected:
    Image_attributes image_attrs;
    static bool build_attributes(const v8::Arguments& args, Element_attributes* ea, Image_attributes* ia);

private:
    static void wrap_extension_list(Extension_list* list);
};

class Eventful_image
    : public Image, public Eventful
{
public:
    Eventful_image(const Element_attributes& element_attributes,
        const Image_attributes& image_attributes);
    /*
    Create a new instance of Eventful_image whenever a matching JavaScript object
    is instanitiated. This method will only be called internally.
    */
    static v8::Handle<v8::Value> create(const v8::Arguments& args);
    Eventful_attributes eventful_attrs;

};

}

#endif // HAT_GUI_IMAGE_HPP
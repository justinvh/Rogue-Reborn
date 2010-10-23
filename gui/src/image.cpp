/*
Rogue Reborn GUI "Image" source from src/image.cpp

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

#include <hat/gui/image.hpp>

namespace hat {
   
namespace {
/*
This is a list of accessors made available through the Image class.
Any additional elements that are added to the class should be added here.
They will be looped over and added to the accessor list when a template
is constructed.
*/
JS_mapping accessors[] = {
    JS_MAP(Image, src),
    { NULL, NULL, NULL } // Signals the end of the accessor list
};

/*
This is a list of functions made availabe through the Image class.
Any additional functions that are addeed tot he class should be added here.
They will be looped over and added to the function list when a template
is constructued.
*/
JS_fun_mapping funs[] = {
    { NULL, NULL, NULL } // Signlas the end of the function list
};
}

/*
*/
JS_SETTER_CLASS(Image, src)
{
    Image* i = unwrap<Image>(info.Holder());
    //i->image_attributes.src = value->BooleanValue();
}

/*
*/
JS_GETTER_CLASS(Image, src)
{
    Image* i = unwrap<Image>(info.Holder());
    return v8::String::New(i->image_attributes.src);
}

/*
*/
Image::Image(const Element_attributes& element_attributes,
    const Image_attributes& image_attributes)
{
}

/*
*/
void Image::think()
{
}


/*
*/
bool Image::build_attributes(const v8::Arguments& args, Image_attributes* ia)
{
    return true;
}

/*
*/
void Image::wrap_extension_list(Extension_list* list)
{
    list->push_back(std::make_pair(accessors, funs));
}

/*
The only way to expose classes to JavaScript natively is to wrap the
existing object. To properly due this, we have to ensure that the object
remains in memory during the execution of script. This responsibility
is dictated by the container, which in this case is the active Gui.
*/

v8::Handle<v8::Value> Image::create(const v8::Arguments& args)
{
    static v8::Persistent<v8::ObjectTemplate> image_tmpl;
    static Extension_list extension_list;

    // We only need to build our extension list once
    if (!extension_list.size()) extension_list.push_back(std::make_pair(accessors, funs));

    // Create a new instance of the image, build the attributes,
    // and make sure there are no problems during the build.
    Element_attributes element_attributes;
    Image_attributes image_attributes;

    // Try to build the arguments. If an exceptions is caught, then we
    // need to rethrow it so the GUI can catch it.
    if (!Element::build_attributes(args, &element_attributes) ||
        !Image::build_attributes(args, &image_attributes))
    {
        return v8::Undefined();
    }

    Image* image = new Image(element_attributes, image_attributes);

    // Now wrap the rest of the image
    Element::wrap_tmpl(&image_tmpl, image, extension_list);
    return image->self();
}

}
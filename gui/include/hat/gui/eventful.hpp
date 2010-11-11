/*
Rogue Reborn GUI "Eventful" definition from hat/gui/eventful.hpp

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

#ifndef HAT_GUI_EVENTFUL_HPP
#define HAT_GUI_EVENTFUL_HPP

#include <hat/v8/easy.hpp>
#include <hat/gui/element.hpp>

namespace hat {

enum Eventful_events
{
    EV_MOUSE_DOWN   = BIT(0),
    EV_MOUSE_UP     = BIT(1),
    EV_MOUSE_DRAG   = BIT(2),
    EV_MOUSE_OVER   = BIT(3),
    EV_MOUSE_OUT    = BIT(4),
    EV_SCROLL_DOWN  = BIT(5),
    EV_SCROLL_UP    = BIT(6),
    EV_KEY_DOWN     = BIT(7),
    EV_KEY_UP       = BIT(8),
    EV_KEY_PRESS    = BIT(9)
};

struct Eventful_attributes
{
    Eventful_attributes() : event_state(0), previous_event_state(0), last_state(0), held_timer(0) { }
    Function_list mouse_down, mouse_up, mouse_drag, mouse_over, mouse_out;
    Function_list scroll_down, scroll_up;
    Function_list key_down, key_up, key_press;

    bool is_mouse_down, is_mouse_up, is_mouse_drag, is_mouse_over, is_mouse_out,
         is_scroll_down, is_scroll_up, is_key_down, is_key_up, is_key_press;

    int event_state, previous_event_state, last_state, held_timer;
};

class Eventful
    : public virtual Base 
{
public:
    void mouse_down(int mx, int my, int button);
    void mouse_up(int mx, int my, int button);
    void mouse_drag(int rx, int ry, int button);
    bool is_draggable(int time);
    void update_held_timer(int dt);
    void set_held_timer(int timer);
    void mouse_over(int mx, int my);
    void mouse_out(int mx, int my);
    void scroll_down();
    void scroll_up();
    void key_down(int key);
    void key_up(int key);
    void key_press(char character);
    void force_state(int state);
    void finish_state();

    /*
    This is an internal structure that is represented by a few macros
    to make creating setters and getters easier. There is really no
    reason to access these methods directly. They are set as accessors
    and made available through the interface between V8 and JavaScript.
    */
    JS_INTERNAL_DEF(Eventful)
    {
        /*
        Generalized functions.
        */
        JS_FUN(mousedown);
        JS_FUN(mouseup);
        JS_FUN(mousedrag);
        JS_FUN(scrolldown);
        JS_FUN(scrollup);
        JS_FUN(mouseover);
        JS_FUN(mouseout);
        JS_FUN(keydown);
        JS_FUN(keyup);
        JS_FUN(keypress);
    };

    Eventful_attributes eventful_attrs;
protected:
    static void wrap_extension_list(Extension_list* list);
};

}

#endif // HAT_GUI_EVENTFUL_HPP
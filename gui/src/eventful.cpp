/*
Rogue Reborn GUI "Eventful" source from src/eventful.cpp

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

#include <hat/gui/eventful.hpp>
#include <hat/gui/image.hpp>
#include <hat/gui/gui.hpp>
#include <hat/gui/ui_local.h>

namespace hat {

namespace {
/*
    This is a list of accessors made available through the Eventful class.
    Any additional elements that are added to the class should be added here.
    They will be looped over and added to the accessor list when a template
    is constructed.
    */
    JS_mapping accessors[] = {
        { NULL, NULL, NULL } // Signals the end of the accessor list
    };

    /*
    This is a list of functions made availabe through the Element class.
    Any additional functions that are addeed tot he class should be added here.
    They will be looped over and added to the function list when a template
    is constructued.
    */
    JS_fun_mapping funs[] = {
        JS_FUN_MAP(Eventful, mousedown),
        JS_FUN_MAP(Eventful, mouseup),
        JS_FUN_MAP(Eventful, mousedrag),
        JS_FUN_MAP(Eventful, mouseover),
        JS_FUN_MAP(Eventful, mouseout),
        JS_FUN_MAP(Eventful, scrolldown),
        JS_FUN_MAP(Eventful, scrollup),
        JS_FUN_MAP(Eventful, keydown),
        JS_FUN_MAP(Eventful, keyup),
        JS_FUN_MAP(Eventful, keypress),
        { NULL, NULL, NULL } // Signlas the end of the function list
    };
}

/*
*/
JS_FUN_CLASS(Eventful, mousedown)
{
    JS_BA_FUNCTION(Eventful, eventful_attrs.mouse_down, {
        Gui* g = unwrap_global_pointer<Gui>(0);
        if (!g) { return v8::Exception::Error(v8::String::New("Gui became detached.")); }
        e->mouse_down(g->last_kbm_state.mx, g->last_kbm_state.my, g->last_kbm_state.button);
    });
}

/*
*/
JS_FUN_CLASS(Eventful, mouseup)
{
    JS_BA_FUNCTION(Eventful, eventful_attrs.mouse_up, {
        Gui* g = unwrap_global_pointer<Gui>(0);
        if (!g) { return v8::Exception::Error(v8::String::New("Gui became detached.")); }
        e->mouse_up(g->last_kbm_state.mx, g->last_kbm_state.my, g->last_kbm_state.button);
    });
}

/*
*/
JS_FUN_CLASS(Eventful, mouseover)
{
    JS_BA_FUNCTION(Eventful, eventful_attrs.mouse_over, {
        Gui* g = unwrap_global_pointer<Gui>(0);
        if (!g) { return v8::Exception::Error(v8::String::New("Gui became detached.")); }
        e->mouse_over(g->last_kbm_state.mx, g->last_kbm_state.my);
    });
}

/*
*/
JS_FUN_CLASS(Eventful, mouseout)
{
    JS_BA_FUNCTION(Eventful, eventful_attrs.mouse_out, {
        Gui* g = unwrap_global_pointer<Gui>(0);
        if (!g) { return v8::Exception::Error(v8::String::New("Gui became detached.")); }
        e->mouse_out(g->last_kbm_state.mx, g->last_kbm_state.my);
    });
}

/*
*/
JS_FUN_CLASS(Eventful, mousedrag)
{
    JS_BA_FUNCTION(Eventful, eventful_attrs.mouse_drag, {
        Gui* g = unwrap_global_pointer<Gui>(0);
        if (!g) { return v8::Exception::Error(v8::String::New("Gui became detached.")); }
        const Gui_kbm& kbm = g->last_kbm_state;
        e->mouse_drag(kbm.mx, kbm.my, kbm.button);
    });
}

/*
*/
JS_FUN_CLASS(Eventful, scrolldown)
{
    JS_BA_FUNCTION(Eventful, eventful_attrs.scroll_down, {
        Gui* g = unwrap_global_pointer<Gui>(0);
        if (!g) { return v8::Exception::Error(v8::String::New("Gui became detached.")); }
        e->scroll_down();
    });
}

/*
*/
JS_FUN_CLASS(Eventful, scrollup)
{
    JS_BA_FUNCTION(Eventful, eventful_attrs.scroll_up, {
        Gui* g = unwrap_global_pointer<Gui>(0);
        if (!g) { return v8::Exception::Error(v8::String::New("Gui became detached.")); }
        e->scroll_up();
    });
}

/*
*/
JS_FUN_CLASS(Eventful, keydown)
{
    JS_BA_FUNCTION(Eventful, eventful_attrs.key_down, {
        Gui* g = unwrap_global_pointer<Gui>(0);
        if (!g) { return v8::Exception::Error(v8::String::New("Gui became detached.")); }
        e->key_down(g->last_kbm_state.key);
    });
}

/*
*/
JS_FUN_CLASS(Eventful, keyup)
{
    JS_BA_FUNCTION(Eventful, eventful_attrs.key_up, {
        Gui* g = unwrap_global_pointer<Gui>(0);
        if (!g) { return v8::Exception::Error(v8::String::New("Gui became detached.")); }
        e->key_up(g->last_kbm_state.key);
    });
}

/*
*/
JS_FUN_CLASS(Eventful, keypress)
{
    JS_BA_FUNCTION(Eventful, eventful_attrs.key_press, {
        Gui* g = unwrap_global_pointer<Gui>(0);
        if (!g) { return v8::Exception::Error(v8::String::New("Gui became detached.")); }
        e->key_press(g->last_kbm_state.character);
    });
}

void Eventful::mouse_down(int mx, int my, int button)
{
    eventful_attrs.event_state |= EV_MOUSE_DOWN;

    if (!eventful_attrs.mouse_down.size()) return;

    v8::HandleScope execution_scope;
    v8::Handle<v8::Value> argvs[3] = {
        v8::Int32::New(mx), v8::Int32::New(my), v8::Int32::New(button)
    };

    v8::TryCatch run_try_catch;
    for (Function_list::const_iterator tci = eventful_attrs.mouse_down.begin();
        tci != eventful_attrs.mouse_down.end();
        ++tci)
    {
        (*tci)->Call(self, 3, argvs);
        if (run_try_catch.HasCaught()) {
            run_try_catch.ReThrow();
            return;
        }
    }
 }

void Eventful::mouse_up(int mx, int my, int button)
{
    eventful_attrs.event_state |= EV_MOUSE_UP;
    set_held_timer(0);
    if (!eventful_attrs.mouse_up.size()) return;

    v8::HandleScope execution_scope;
    v8::Handle<v8::Value> argvs[3] = {
        v8::Int32::New(mx), v8::Int32::New(my), v8::Int32::New(button)
    };

    v8::TryCatch run_try_catch;
    for (Function_list::const_iterator tci = eventful_attrs.mouse_up.begin();
        tci != eventful_attrs.mouse_up.end();
        ++tci)
    {
        (*tci)->Call(self, 3, argvs);
        if (run_try_catch.HasCaught()) {
            run_try_catch.ReThrow();
            return;
        }
    }
 }


bool Eventful::is_draggable(int time)
{
    return eventful_attrs.is_mouse_down && eventful_attrs.held_timer > time;
}

void Eventful::update_held_timer(int dt)
{
    eventful_attrs.held_timer += dt;
}

void Eventful::set_held_timer(int timer)
{
    eventful_attrs.held_timer = timer;
}

void Eventful::mouse_drag(int rx, int ry, int button)
{
    eventful_attrs.event_state |= EV_MOUSE_DRAG;
    if (!eventful_attrs.mouse_drag.size()) return;

    v8::HandleScope execution_scope;
    v8::Handle<v8::Value> argvs[3] = {
        v8::Int32::New(rx), v8::Int32::New(ry), v8::Int32::New(button)
    };

    v8::TryCatch run_try_catch;
    for (Function_list::const_iterator tci = eventful_attrs.mouse_drag.begin();
        tci != eventful_attrs.mouse_drag.end();
        ++tci)
    {
        (*tci)->Call(self, 3, argvs);
        if (run_try_catch.HasCaught()) {
            run_try_catch.ReThrow();
            return;
        }
    }
 }

void Eventful::mouse_over(int mx, int my)
{
    eventful_attrs.event_state |= EV_MOUSE_OVER;
    if (!eventful_attrs.mouse_over.size()) return;

    v8::HandleScope execution_scope;
    v8::Handle<v8::Value> argvs[2] = {
        v8::Int32::New(mx), v8::Int32::New(my)
    };

    v8::TryCatch run_try_catch;
    for (Function_list::const_iterator tci = eventful_attrs.mouse_over.begin();
        tci != eventful_attrs.mouse_over.end();
        ++tci)
    {
        (*tci)->Call(self, 2, argvs);
        if (run_try_catch.HasCaught()) {
            run_try_catch.ReThrow();
            return;
        }
    }
 }

 void Eventful::mouse_out(int mx, int my)
 {
    eventful_attrs.event_state |= EV_MOUSE_OUT;
    if (!eventful_attrs.mouse_out.size()) return;

    v8::HandleScope execution_scope;
    v8::Handle<v8::Value> argvs[2] = {
        v8::Int32::New(mx), v8::Int32::New(my)
    };

    v8::TryCatch run_try_catch;
    for (Function_list::const_iterator tci = eventful_attrs.mouse_out.begin();
        tci != eventful_attrs.mouse_out.end();
        ++tci)
    {
        (*tci)->Call(self, 2, argvs);
        if (run_try_catch.HasCaught()) {
            run_try_catch.ReThrow();
            return;
        }
    }
 }

 void Eventful::key_down(int key)
 {
    eventful_attrs.event_state |= EV_KEY_DOWN;
    if (!eventful_attrs.key_down.size()) return;

    v8::HandleScope execution_scope;
    v8::Handle<v8::Value> argvs[1] = {
        v8::Int32::New(key)
    };

    v8::TryCatch run_try_catch;
    for (Function_list::const_iterator tci = eventful_attrs.key_down.begin();
        tci != eventful_attrs.key_down.end();
        ++tci)
    {
        (*tci)->Call(self, 1, argvs);
        if (run_try_catch.HasCaught()) {
            run_try_catch.ReThrow();
            return;
        }
    }
 }

 void Eventful::key_up(int key)
 {
   eventful_attrs.event_state |= EV_KEY_UP;
   if (!eventful_attrs.key_up.size()) return;

    v8::HandleScope execution_scope;
    v8::Handle<v8::Value> argvs[1] = {
        v8::Int32::New(key)
    };

    v8::TryCatch run_try_catch;
    for (Function_list::const_iterator tci = eventful_attrs.key_up.begin();
        tci != eventful_attrs.key_up.end();
        ++tci)
    {
        (*tci)->Call(self, 1, argvs);
        if (run_try_catch.HasCaught()) {
            run_try_catch.ReThrow();
            return;
        }
    }
 }

 void Eventful::key_press(char character)
 {
    eventful_attrs.event_state |= EV_KEY_PRESS;
    if (!eventful_attrs.key_press.size()) return;

    v8::HandleScope execution_scope;
    v8::Handle<v8::Value> argvs[1] = {
        v8::String::New("" + character)
    };

    v8::TryCatch run_try_catch;
    for (Function_list::const_iterator tci = eventful_attrs.key_press.begin();
        tci != eventful_attrs.key_press.end();
        ++tci)
    {
        (*tci)->Call(self, 1, argvs);
        if (run_try_catch.HasCaught()) {
            run_try_catch.ReThrow();
            return;
        }
    }
 }

void Eventful::scroll_down()
{
    eventful_attrs.event_state |= EV_SCROLL_DOWN;
    if (!eventful_attrs.scroll_down.size()) return;

    v8::HandleScope execution_scope;
    v8::TryCatch run_try_catch;
    for (Function_list::const_iterator tci = eventful_attrs.scroll_down.begin();
        tci != eventful_attrs.scroll_down.end();
        ++tci)
    {
        (*tci)->Call(self, 0, NULL);
        if (run_try_catch.HasCaught()) {
            run_try_catch.ReThrow();
            return;
        }
    }
}

void Eventful::scroll_up()
{
    eventful_attrs.event_state |= EV_SCROLL_UP;
    if (!eventful_attrs.scroll_up.size()) return;

    v8::HandleScope execution_scope;
    v8::TryCatch run_try_catch;
    for (Function_list::const_iterator tci = eventful_attrs.scroll_up.begin();
        tci != eventful_attrs.scroll_up.end();
        ++tci)
    {
        (*tci)->Call(self, 0, NULL);
        if (run_try_catch.HasCaught()) {
            run_try_catch.ReThrow();
            return;
        }
    }
}

void Eventful::force_state(int state)
{
    eventful_attrs.event_state |= state;
}

 void Eventful::finish_state()
 {
     int s = eventful_attrs.event_state;
     int p = eventful_attrs.previous_event_state;

     eventful_attrs.is_mouse_down = false;
     if (s & EV_MOUSE_DOWN) {
         eventful_attrs.is_mouse_down = true;
     } else if (p & EV_MOUSE_DOWN && !(s & EV_MOUSE_UP)) {
         eventful_attrs.is_mouse_down = true;
         s |= EV_MOUSE_DOWN;
     }

    eventful_attrs.is_mouse_up = false;
     if (s & EV_MOUSE_UP) {
         eventful_attrs.is_mouse_up = true;
         eventful_attrs.is_mouse_down = false;
     }

     eventful_attrs.is_mouse_drag = false;
     if (s & EV_MOUSE_DRAG) {
         eventful_attrs.is_mouse_drag = true;
     }

     eventful_attrs.is_mouse_over = false;
     if (s & EV_MOUSE_OVER) {
         eventful_attrs.is_mouse_over = true;
     }

    eventful_attrs.is_mouse_out = false;
     if (s & EV_MOUSE_OUT) {
         eventful_attrs.is_mouse_out = true;
     }

     eventful_attrs.is_key_down = false;
     if (s & EV_KEY_DOWN) {
         eventful_attrs.is_key_down = true;
     }

     eventful_attrs.is_key_up = false;
     if (s & EV_KEY_UP) {
         eventful_attrs.is_key_up = true;
     }

     eventful_attrs.is_key_press = false;
     if (s & EV_KEY_PRESS) {
         eventful_attrs.is_key_press = true;
     }

     eventful_attrs.is_scroll_up = false;
     if (s & EV_SCROLL_UP) {
         eventful_attrs.is_scroll_up = true;
     }

     eventful_attrs.is_scroll_down = false;
     if (s & EV_SCROLL_DOWN) {
         eventful_attrs.is_scroll_down = true;
     }

     eventful_attrs.previous_event_state = s;
     eventful_attrs.event_state = 0;
 }

/*
*/
void Eventful::wrap_extension_list(Extension_list* list)
{
    list->push_back(std::make_pair(accessors, funs));
}

}

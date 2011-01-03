#include <hat/v8/type_conversion.hpp>
#include <cstring>

namespace easy {
bool convert(const v8::Handle<v8::Value>& from, bool* to)
{
  if (from->IsBoolean()) {
    (*to) = from->BooleanValue();
    return true;
  }
  return false;
}

bool convert(const bool& from, v8::Handle<v8::Value>* to)
{
  (*to) = v8::Boolean::New(from);
  return true;
}

bool convert(const v8::Handle<v8::Value>& from, int32_t* to)
{
  if (from->IsInt32()) {
    (*to) = from->Int32Value();
    return true;
  }
  return false;
}

bool convert(const int32_t& from, v8::Handle<v8::Value>* to)
{
  (*to) = v8::Int32::New(from);
  return true;
}

bool convert(const v8::Handle<v8::Value>& from, int64_t* to)
{
  if (from->IsNumber()) {
    (*to) = from->IntegerValue();
    return true;
  }
  return false;
}

bool convert(const int64_t& from, v8::Handle<v8::Value>* to)
{
  (*to) = v8::Integer::New(from);
  return true;
}

bool convert(const v8::Handle<v8::Value>& from, double* to)
{
  if (from->IsNumber()) {
    (*to) = from->NumberValue();
    return true;
  }
  return false;
}

bool convert(const double& from, v8::Handle<v8::Value>* to)
{
  (*to) = v8::Number::New(from);
  return true;
}

bool convert(const v8::Handle<v8::Value>& from, float* to)
{
  if (from->IsNumber()) {
    (*to) = (float)from->NumberValue();
    return true;
  }
  return false;
}

bool convert(const float& from, v8::Handle<v8::Value>* to)
{
  (*to) = v8::Number::New(from);
  return true;
}

bool convert(const v8::Handle<v8::Value>& from, char** to)
{
  if (from->IsString()) {
    v8::Local<v8::String> string_val = from->ToString();
    char* s = new char[string_val->Length()];
    strcpy(s, *v8::String::Utf8Value(string_val));
    *to = s;
    return true;
  }
  return false;
}

bool convert(const v8::Handle<v8::Value>& from, std::string* to)
{
  if (from->IsString()) {
    v8::Local<v8::String> string_val = from->ToString();
    (*to) = *v8::String::Utf8Value(string_val);
    return true;
  }
  return false;
}

bool convert(const std::string& from, v8::Handle<v8::Value>* to)
{
  (*to) = v8::String::New(from.c_str(), from.size());
  return true;
}

/*
template <class Insert_container>
bool convert(const v8::Handle<v8::Value>& from, Insert_container* to)
{
  if (from->IsArray()) {
    v8::Handle<v8::Array> array_val(v8::Array::Cast(*from));
    std::insert_iterator<Insert_container> insert_iter(*to, to->begin());
    for (uint32_t i = 0; i < array_val->Length(); i++) {
      Insert_container::value_type local;
      if (!smart_convert(array_val->Get(i), &local)) {
        return false;
      }
      insert_iter = local;
    }
    return true;
  }
  return false;
}

template <class Iterable_container>
bool convert(const Iterable_container& from, v8::Handle<v8::Value>* to)
{
  v8::Handle<v8::Array> array_list = v8::Array::New();
  int ith = 0;
  for (Iterable_container::const_iterator cit = from.begin();
    cit != from.end();
    ++cit, ith++)
  {
    v8::Handle<v8::Value> val;
    if (!smart_convert(*cit, val)) {
      return false;
    }
    array_list->Set(ith, val);
  }

  return true;
}

*/

}

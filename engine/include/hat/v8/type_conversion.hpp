#ifndef EASYV8_TYPE_CONVERSION_HPP
#define EASYV8_TYPE_CONVERSION_HPP

#include <v8.h>
#include <string>
#include <iterator>
#include <iostream>

namespace easy {
bool convert(const v8::Handle<v8::Value>& from, bool* to);
bool convert(const bool& from, v8::Handle<v8::Value>* to);
bool convert(const v8::Handle<v8::Value>& from, int32_t* to);
bool convert(const int32_t& from, v8::Handle<v8::Value>* to);
bool convert(const v8::Handle<v8::Value>& from, int64_t* to);
bool convert(const int64_t& from, v8::Handle<v8::Value>* to);
bool convert(const v8::Handle<v8::Value>& from, double* to);
bool convert(const double& from, v8::Handle<v8::Value>* to);
bool convert(const v8::Handle<v8::Value>& from, float* to);
bool convert(const float& from, v8::Handle<v8::Value>* to);
bool convert(const v8::Handle<v8::Value>& from, char** to);
bool convert(const v8::Handle<v8::Value>& from, std::string* to);
bool convert(const std::string& from, v8::Handle<v8::Value>* to);

template <class Type>
bool smart_convert(const v8::Handle<v8::Value>& from, Type* to)
{
  if (to == NULL) return false;
  return convert(from, to);
}

template <class Type>
bool smart_convert(const Type& from, v8::Handle<v8::Value>* to)
{
  if (to == NULL) return false;
  return convert(from, to);
}

template <class Type>
bool smart_get(const v8::Handle<v8::Object>& from, const char* key, Type* to)
{
  v8::Handle<v8::Value> from_val = from->Get(v8::String::New(key));
  if (from_val->IsUndefined()) {
    return false;
  } else {
    return convert(from_val, to);
  }
}

};

#endif // EASYV8_TYPE_CONVERSION_HPP
#include <stdio.h>

#include "memory.h"
#include "object.h"
#include "value.h"
#include "vm.h"

static Obj* allocateObject(size_t size, ObjType type) {
  Obj* object = (Obj*)reallocate(NULL, 0, size);
  object->type = type;

  object->next = vm.objects;
  vm.objects = object;
  return object;
}

ObjString* createString(bool ownsChars, const char* chars, int length) {
  ObjString* string = (ObjString*)allocateObject(
      sizeof(ObjString), OBJ_STRING);
  string->ownsChars = ownsChars;
  string->length = length;
  string->chars = chars;
  return string;
}
void printObject(Value value) {
  switch (OBJ_TYPE(value)) {
    case OBJ_STRING:
      printf("%.*s", AS_STRING(value)->length, AS_CSTRING(value));
      break;
  }
}

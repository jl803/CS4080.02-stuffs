#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "memory.h"
#include "object.h"
#include "vm.h"

VM vm; // [one]
static bool absNative(int argCount, Value* args) {
  if (!IS_NUMBER(args[0])) {
    args[-1] = OBJ_VAL(copyString("abs() expects a number.", 23));
    return false;
  }

  double value = AS_NUMBER(args[0]);
  args[-1] = NUMBER_VAL(value < 0 ? -value : value);
  return true;
}
static bool clockNative(int argCount, Value* args) {
  args[-1] = NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
  return true;
}
static bool failNative(int argCount, Value* args) {
  args[-1] = OBJ_VAL(copyString("Native function failed.", 23));
  return false;
}
static bool typeNative(int argCount, Value* args) {
  const char* name;
  if (IS_BOOL(args[0])) {
    name = "boolean";
  } else if (IS_NIL(args[0])) {
    name = "nil";
  } else if (IS_NUMBER(args[0])) {
    name = "number";
  } else if (IS_CLOSURE(args[0]) || IS_FUNCTION(args[0])) {
    name = "function";
  } else if (IS_NATIVE(args[0])) {
    name = "native";
  } else if (IS_STRING(args[0])) {
    name = "string";
  } else {
    name = "unknown";
  }

  args[-1] = OBJ_VAL(copyString(name, (int)strlen(name)));
  return true;
}
static void resetStack() {
  vm.stackTop = vm.stack;
  vm.frameCount = 0;
  vm.openUpvalues = NULL;
}
static inline ObjFunction* frameFunction(CallFrame* frame) {
  if (frame->callable->type == OBJ_FUNCTION) {
    return (ObjFunction*)frame->callable;
  }
  return ((ObjClosure*)frame->callable)->function;
}
static void runtimeError(const char* format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputs("\n", stderr);

  for (int i = vm.frameCount - 1; i >= 0; i--) {
    CallFrame* frame = &vm.frames[i];
    ObjFunction* function = frameFunction(frame);
    size_t instruction = frame->ip - function->chunk.code - 1;
    fprintf(stderr, "[line %d] in ",
            getLine(&function->chunk, (int)instruction));
    if (function->name == NULL) {
      fprintf(stderr, "script\n");
    } else {
      fprintf(stderr, "%s()\n", function->name->chars);
    }
  }

  resetStack();
}
static void defineNative(const char* name, int arity,
                         NativeFn function) {
  ObjString* string = copyString(name, (int)strlen(name));
  int index = vm.globalValues.count;
  writeValueArray(&vm.globalValues,
                  OBJ_VAL(newNative(arity, function)));
  tableSet(&vm.globalNames, OBJ_VAL(string), NUMBER_VAL(index));
}

void initVM() {
  vm.stackCapacity = GROW_CAPACITY(0);
  vm.stack = GROW_ARRAY(Value, NULL, 0, vm.stackCapacity);
  resetStack();
  vm.objects = NULL;
  initTable(&vm.globalNames);
  initTable(&vm.immutableGlobals);
  initValueArray(&vm.globalValues);
  initTable(&vm.strings);

  defineNative("abs", 1, absNative);
  defineNative("clock", 0, clockNative);
  defineNative("fail", 0, failNative);
  defineNative("type", 1, typeNative);
}

void freeVM() {
  FREE_ARRAY(Value, vm.stack, vm.stackCapacity);
  vm.stack = NULL;
  vm.stackTop = NULL;
  vm.stackCapacity = 0;
  freeTable(&vm.globalNames);
  freeTable(&vm.immutableGlobals);
  freeValueArray(&vm.globalValues);
  freeTable(&vm.strings);
  freeObjects();
}
void push(Value value) {
  int count = (int)(vm.stackTop - vm.stack);
  if (count == vm.stackCapacity) {
    int oldCapacity = vm.stackCapacity;
    int frameOffsets[FRAMES_MAX];
    for (int i = 0; i < vm.frameCount; i++) {
      frameOffsets[i] = (int)(vm.frames[i].slots - vm.stack);
    }

    int openUpvalueCount = 0;
    for (ObjUpvalue* upvalue = vm.openUpvalues;
         upvalue != NULL; upvalue = upvalue->next) {
      openUpvalueCount++;
    }
    int* upvalueOffsets = ALLOCATE(int, openUpvalueCount);
    int upvalueIndex = 0;
    for (ObjUpvalue* upvalue = vm.openUpvalues;
         upvalue != NULL; upvalue = upvalue->next) {
      upvalueOffsets[upvalueIndex++] =
          (int)(upvalue->location - vm.stack);
    }

    vm.stackCapacity = GROW_CAPACITY(oldCapacity);
    vm.stack = GROW_ARRAY(Value, vm.stack,
        oldCapacity, vm.stackCapacity);
    vm.stackTop = vm.stack + count;
    for (int i = 0; i < vm.frameCount; i++) {
      vm.frames[i].slots = vm.stack + frameOffsets[i];
    }
    upvalueIndex = 0;
    for (ObjUpvalue* upvalue = vm.openUpvalues;
         upvalue != NULL; upvalue = upvalue->next) {
      upvalue->location = vm.stack + upvalueOffsets[upvalueIndex++];
    }
    FREE_ARRAY(int, upvalueOffsets, openUpvalueCount);
  }

  *vm.stackTop = value;
  vm.stackTop++;
}
Value pop() {
  vm.stackTop--;
  return *vm.stackTop;
}
static Value peek(int distance) {
  return vm.stackTop[-1 - distance];
}
static bool callObject(Obj* callable, ObjFunction* function,
                       int argCount) {
  if (argCount != function->arity) {
    runtimeError("Expected %d arguments but got %d.",
        function->arity, argCount);
    return false;
  }

  if (vm.frameCount == FRAMES_MAX) {
    runtimeError("Stack overflow.");
    return false;
  }

  CallFrame* frame = &vm.frames[vm.frameCount++];
  frame->callable = callable;
  frame->ip = function->chunk.code;
  frame->slots = vm.stackTop - argCount - 1;
  return true;
}
static bool callClosure(ObjClosure* closure, int argCount) {
  return callObject((Obj*)closure, closure->function, argCount);
}
static bool callFunction(ObjFunction* function, int argCount) {
  return callObject((Obj*)function, function, argCount);
}
static bool callValue(Value callee, int argCount) {
  if (IS_OBJ(callee)) {
    switch (OBJ_TYPE(callee)) {
      case OBJ_CLOSURE:
        return callClosure(AS_CLOSURE(callee), argCount);
      case OBJ_FUNCTION:
        return callFunction(AS_FUNCTION(callee), argCount);
      case OBJ_NATIVE: {
        ObjNative* native = AS_NATIVE(callee);
        if (argCount != native->arity) {
          runtimeError("Expected %d arguments but got %d.",
                       native->arity, argCount);
          return false;
        }
        if (native->function(argCount,
                             vm.stackTop - argCount)) {
          vm.stackTop -= argCount;
          return true;
        }
        runtimeError("%s",
            AS_CSTRING(vm.stackTop[-argCount - 1]));
        return false;
      }
      default:
        break; // Non-callable object type.
    }
  }
  runtimeError("Can only call functions and classes.");
  return false;
}
static ObjUpvalue* captureUpvalue(Value* local) {
  ObjUpvalue* previous = NULL;
  ObjUpvalue* upvalue = vm.openUpvalues;
  while (upvalue != NULL && upvalue->location > local) {
    previous = upvalue;
    upvalue = upvalue->next;
  }

  if (upvalue != NULL && upvalue->location == local) {
    return upvalue;
  }

  ObjUpvalue* created = newUpvalue(local);
  created->next = upvalue;
  if (previous == NULL) {
    vm.openUpvalues = created;
  } else {
    previous->next = created;
  }
  return created;
}
static void closeUpvalues(Value* last) {
  while (vm.openUpvalues != NULL &&
         vm.openUpvalues->location >= last) {
    ObjUpvalue* upvalue = vm.openUpvalues;
    upvalue->closed = *upvalue->location;
    upvalue->location = &upvalue->closed;
    vm.openUpvalues = upvalue->next;
  }
}
static bool isFalsey(Value value) {
  return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}
static void concatenate() {
  ObjString* b = AS_STRING(pop());
  ObjString* a = AS_STRING(pop());

  int length = a->length + b->length;
  char* chars = ALLOCATE(char, length + 1);
  memcpy(chars, a->chars, a->length);
  memcpy(chars + a->length, b->chars, b->length);
  chars[length] = '\0';

  ObjString* result = takeString(chars, length);
  push(OBJ_VAL(result));
}
static InterpretResult run() {
  CallFrame* frame = &vm.frames[vm.frameCount - 1];
  ObjFunction* function = frameFunction(frame);
  register uint8_t* ip = frame->ip;

#define READ_BYTE() (*ip++)

#define READ_SHORT() \
    (ip += 2, (uint16_t)((ip[-2] << 8) | ip[-1]))

#define READ_CONSTANT() \
    (function->chunk.constants.values[READ_BYTE()])

#define READ_STRING() AS_STRING(READ_CONSTANT())
#define BINARY_OP(valueType, op) \
    do { \
      if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
        frame->ip = ip; \
        runtimeError("Operands must be numbers."); \
        return INTERPRET_RUNTIME_ERROR; \
      } \
      double b = AS_NUMBER(pop()); \
      double a = AS_NUMBER(pop()); \
      push(valueType(a op b)); \
    } while (false)

  for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
    printf("          ");
    for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
      printf("[ ");
      printValue(*slot);
      printf(" ]");
    }
    printf("\n");
    disassembleInstruction(&function->chunk,
        (int)(ip - function->chunk.code));
#endif

    uint8_t instruction;
    switch (instruction = READ_BYTE()) {
      case OP_CONSTANT: {
        Value constant = READ_CONSTANT();
        push(constant);
        break;
      }
      case OP_CONSTANT_LONG: {
        uint32_t constant = (uint32_t)READ_BYTE() << 16;
        constant |= (uint32_t)READ_BYTE() << 8;
        constant |= READ_BYTE();
        push(function->chunk.constants.values[constant]);
        break;
      }
      case OP_NIL: push(NIL_VAL); break;
      case OP_TRUE: push(BOOL_VAL(true)); break;
      case OP_FALSE: push(BOOL_VAL(false)); break;
      case OP_POP: pop(); break;
      case OP_DUP: push(peek(0)); break;
      case OP_GET_LOCAL: {
        uint8_t slot = READ_BYTE();
        push(frame->slots[slot]);
        break;
      }
      case OP_SET_LOCAL: {
        uint8_t slot = READ_BYTE();
        frame->slots[slot] = peek(0);
        break;
      }
      case OP_GET_LOCAL_LONG: {
        uint16_t slot = READ_SHORT();
        push(frame->slots[slot]);
        break;
      }
      case OP_SET_LOCAL_LONG: {
        uint16_t slot = READ_SHORT();
        frame->slots[slot] = peek(0);
        break;
      }
      case OP_GET_UPVALUE: {
        uint8_t slot = READ_BYTE();
        ObjClosure* closure = (ObjClosure*)frame->callable;
        push(*closure->upvalues[slot]->location);
        break;
      }
      case OP_SET_UPVALUE: {
        uint8_t slot = READ_BYTE();
        ObjClosure* closure = (ObjClosure*)frame->callable;
        *closure->upvalues[slot]->location = peek(0);
        break;
      }
      case OP_GET_GLOBAL: {
        Value value = vm.globalValues.values[READ_BYTE()];
        if (IS_UNDEFINED(value)) {
          frame->ip = ip;
          runtimeError("Undefined variable.");
          return INTERPRET_RUNTIME_ERROR;
        }
        push(value);
        break;
      }
      case OP_DEFINE_GLOBAL: {
        vm.globalValues.values[READ_BYTE()] = pop();
        break;
      }
      case OP_SET_GLOBAL: {
        uint8_t index = READ_BYTE();
        if (IS_UNDEFINED(vm.globalValues.values[index])) {
          frame->ip = ip;
          runtimeError("Undefined variable.");
          return INTERPRET_RUNTIME_ERROR;
        }
        vm.globalValues.values[index] = peek(0);
        break;
      }
      case OP_EQUAL: {
        Value b = pop();
        Value a = pop();
        push(BOOL_VAL(valuesEqual(a, b)));
        break;
      }
      case OP_GREATER:  BINARY_OP(BOOL_VAL, >); break;
      case OP_LESS:     BINARY_OP(BOOL_VAL, <); break;
      case OP_ADD: {
        if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
          concatenate();
        } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
          double b = AS_NUMBER(pop());
          double a = AS_NUMBER(pop());
          push(NUMBER_VAL(a + b));
        } else {
          frame->ip = ip;
          runtimeError(
              "Operands must be two numbers or two strings.");
          return INTERPRET_RUNTIME_ERROR;
        }
        break;
      }
      case OP_SUBTRACT: BINARY_OP(NUMBER_VAL, -); break;
      case OP_MULTIPLY: BINARY_OP(NUMBER_VAL, *); break;
      case OP_DIVIDE:   BINARY_OP(NUMBER_VAL, /); break;
      case OP_NOT:
        push(BOOL_VAL(isFalsey(pop())));
        break;
      case OP_NEGATE:
        if (!IS_NUMBER(peek(0))) {
          frame->ip = ip;
          runtimeError("Operand must be a number.");
          return INTERPRET_RUNTIME_ERROR;
        }
        vm.stackTop[-1] = NUMBER_VAL(-AS_NUMBER(vm.stackTop[-1]));
        break;
      case OP_PRINT: {
        printValue(pop());
        printf("\n");
        break;
      }
      case OP_JUMP: {
        uint16_t offset = READ_SHORT();
        ip += offset;
        break;
      }
      case OP_JUMP_IF_FALSE: {
        uint16_t offset = READ_SHORT();
        if (isFalsey(peek(0))) ip += offset;
        break;
      }
      case OP_LOOP: {
        uint16_t offset = READ_SHORT();
        ip -= offset;
        break;
      }
      case OP_CALL: {
        int argCount = READ_BYTE();
        frame->ip = ip;
        if (!callValue(peek(argCount), argCount)) {
          return INTERPRET_RUNTIME_ERROR;
        }
        frame = &vm.frames[vm.frameCount - 1];
        function = frameFunction(frame);
        ip = frame->ip;
        break;
      }
      case OP_CLOSURE: {
        ObjFunction* closureFunction = AS_FUNCTION(READ_CONSTANT());
        ObjClosure* closure = newClosure(closureFunction);
        push(OBJ_VAL(closure));
        for (int i = 0; i < closure->upvalueCount; i++) {
          uint8_t isLocal = READ_BYTE();
          uint8_t index = READ_BYTE();
          if (isLocal) {
            closure->upvalues[i] =
                captureUpvalue(frame->slots + index);
          } else {
            ObjClosure* enclosing = (ObjClosure*)frame->callable;
            closure->upvalues[i] = enclosing->upvalues[index];
          }
        }
        break;
      }
      case OP_CLOSE_UPVALUE:
        closeUpvalues(vm.stackTop - 1);
        pop();
        break;
      case OP_RETURN: {
        Value result = pop();
        closeUpvalues(frame->slots);
        vm.frameCount--;
        if (vm.frameCount == 0) {
          pop();
          return INTERPRET_OK;
        }

        vm.stackTop = frame->slots;
        push(result);
        frame = &vm.frames[vm.frameCount - 1];
        function = frameFunction(frame);
        ip = frame->ip;
        break;
      }
    }
  }

#undef READ_BYTE
#undef READ_SHORT
#undef READ_CONSTANT
#undef BINARY_OP
}
void hack(bool b) {
  // Keep run() reachable while Chapter 16 only prints scanned tokens.
  run();
  if (b) hack(false);
}
InterpretResult interpret(const char* source) {
  ObjFunction* function = compile(source);
  if (function == NULL) return INTERPRET_COMPILE_ERROR;

  push(OBJ_VAL(function));
  callFunction(function, 0);

  return run();
}

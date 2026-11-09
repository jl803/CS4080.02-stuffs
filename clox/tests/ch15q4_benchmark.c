#include <stdio.h>
#include <time.h>

#include "../value.h"

#define ITERATIONS 50000000

static volatile Value result;
static volatile Value stack[1];
static volatile Value* stackTop;

__attribute__((noinline)) static void pushValue(Value value) {
  *stackTop = value;
  stackTop++;
}

__attribute__((noinline)) static Value popValue(void) {
  stackTop--;
  return *stackTop;
}

static double benchmarkBefore(void) {
  stack[0] = 1;
  stackTop = stack + 1;
  clock_t start = clock();

  for (int i = 0; i < ITERATIONS; i++) {
    pushValue(-popValue());
  }

  result = stack[0];
  return (double)(clock() - start) / CLOCKS_PER_SEC;
}

static double benchmarkAfter(void) {
  stack[0] = 1;
  clock_t start = clock();

  for (int i = 0; i < ITERATIONS; i++) {
    stack[0] = -stack[0];
  }

  result = stack[0];
  return (double)(clock() - start) / CLOCKS_PER_SEC;
}

int main(void) {
  double before = benchmarkBefore();
  double after = benchmarkAfter();

  printf("%d OP_NEGATE instructions (-O2)\n", ITERATIONS);
  printf("Before: %.3f seconds\n", before);
  printf("After:  %.3f seconds\n", after);
  return result != 1;
}

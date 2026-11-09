#include <stdbool.h>
#include <stdio.h>

#include "../vm.h"

int main(void) {
  initVM();

  for (int i = 0; i < 1000; i++) {
    push(i);
  }

  bool passed = true;
  for (int i = 999; i >= 0; i--) {
    if (pop() != i) passed = false;
  }

  printf("Dynamic stack test: %s (1000 values)\n",
         passed ? "passed" : "failed");

  freeVM();
  return passed ? 0 : 1;
}

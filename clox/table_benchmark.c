#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "object.h"
#include "table.h"
#include "vm.h"

#define NUMBER_KEY_COUNT 200000
#define HOT_KEY_COUNT 1024
#define HOT_UPDATE_COUNT 2000000
#define STRING_KEY_COUNT 50000

static volatile double checksum = 0;

static double elapsedSeconds(clock_t start) {
  return (double)(clock() - start) / CLOCKS_PER_SEC;
}

static void printResult(const char* name, double seconds,
                        long operationCount) {
  double millionsPerSecond = operationCount / seconds / 1000000.0;
  printf("%-25s %.4f seconds (%5.1f million operations/second)\n",
         name, seconds, millionsPerSecond);
}

static double benchmarkNumberKeys() {
  Table table;
  initTable(&table);
  clock_t start = clock();

  for (int i = 0; i < NUMBER_KEY_COUNT; i++) {
    tableSet(&table, NUMBER_VAL(i), NUMBER_VAL(i * 2));
  }

  Value value;
  for (int i = 0; i < NUMBER_KEY_COUNT; i++) {
    if (tableGet(&table, NUMBER_VAL(i), &value)) {
      checksum += AS_NUMBER(value);
    }
  }

  double elapsed = elapsedSeconds(start);
  freeTable(&table);
  return elapsed;
}

static double benchmarkHotUpdates() {
  Table table;
  initTable(&table);

  for (int i = 0; i < HOT_KEY_COUNT; i++) {
    tableSet(&table, NUMBER_VAL(i), NUMBER_VAL(0));
  }

  clock_t start = clock();
  for (int i = 0; i < HOT_UPDATE_COUNT; i++) {
    int key = i % HOT_KEY_COUNT;
    tableSet(&table, NUMBER_VAL(key), NUMBER_VAL(i));
  }

  Value value;
  for (int i = 0; i < HOT_KEY_COUNT; i++) {
    if (tableGet(&table, NUMBER_VAL(i), &value)) {
      checksum += AS_NUMBER(value);
    }
  }

  double elapsed = elapsedSeconds(start);
  freeTable(&table);
  return elapsed;
}

static double benchmarkDeletionChurn() {
  Table table;
  initTable(&table);

  for (int i = 0; i < NUMBER_KEY_COUNT; i++) {
    tableSet(&table, NUMBER_VAL(i), NUMBER_VAL(i));
  }

  clock_t start = clock();
  for (int i = 0; i < NUMBER_KEY_COUNT; i += 2) {
    tableDelete(&table, NUMBER_VAL(i));
  }
  for (int i = 0; i < NUMBER_KEY_COUNT; i += 2) {
    tableSet(&table, NUMBER_VAL(i), NUMBER_VAL(i + 1));
  }

  Value value;
  for (int i = 0; i < NUMBER_KEY_COUNT; i++) {
    if (tableGet(&table, NUMBER_VAL(i), &value)) {
      checksum += AS_NUMBER(value);
    }
  }

  double elapsed = elapsedSeconds(start);
  freeTable(&table);
  return elapsed;
}

static double benchmarkStringKeys() {
  Table table;
  initTable(&table);
  ObjString** keys = (ObjString**)malloc(
      sizeof(ObjString*) * STRING_KEY_COUNT);
  if (keys == NULL) exit(1);

  clock_t start = clock();
  for (int i = 0; i < STRING_KEY_COUNT; i++) {
    char text[32];
    int length = snprintf(text, sizeof(text), "key-%d", i);
    keys[i] = copyString(text, length);
    tableSet(&table, OBJ_VAL(keys[i]), NUMBER_VAL(i));
  }

  Value value;
  for (int i = 0; i < STRING_KEY_COUNT; i++) {
    if (tableGet(&table, OBJ_VAL(keys[i]), &value)) {
      checksum += AS_NUMBER(value);
    }
  }

  double elapsed = elapsedSeconds(start);
  free(keys);
  freeTable(&table);
  return elapsed;
}

int main(void) {
  initVM();

  printResult("Number insert and lookup", benchmarkNumberKeys(),
              NUMBER_KEY_COUNT * 2L);
  printResult("Repeated updates", benchmarkHotUpdates(),
              HOT_UPDATE_COUNT + HOT_KEY_COUNT);
  printResult("Delete and reinsert", benchmarkDeletionChurn(),
              NUMBER_KEY_COUNT * 2L);
  printResult("String insert and lookup", benchmarkStringKeys(),
              STRING_KEY_COUNT * 2L);

  freeVM();
  return checksum < 0;
}


#include <stdio.h>

#include "../object.h"
#include "../table.h"
#include "../vm.h"

static bool hasNumber(Table* table, Value key, double expected) {
  Value value;
  return tableGet(table, key, &value) &&
      IS_NUMBER(value) && AS_NUMBER(value) == expected;
}

static void printResult(const char* name, bool passed) {
  printf("%s: %s\n", name, passed ? "passed" : "failed");
}

int main(void) {
  initVM();

  Table table;
  initTable(&table);
  bool allPassed = true;

  tableSet(&table, BOOL_VAL(true), NUMBER_VAL(1));
  tableSet(&table, BOOL_VAL(false), NUMBER_VAL(2));
  bool booleanPassed =
      hasNumber(&table, BOOL_VAL(true), 1) &&
      hasNumber(&table, BOOL_VAL(false), 2);
  printResult("Boolean keys", booleanPassed);
  allPassed = allPassed && booleanPassed;

  tableSet(&table, NIL_VAL, NUMBER_VAL(3));
  bool nilPassed = hasNumber(&table, NIL_VAL, 3);
  printResult("nil key", nilPassed);
  allPassed = allPassed && nilPassed;

  tableSet(&table, NUMBER_VAL(42.5), NUMBER_VAL(4));
  bool numberPassed = hasNumber(&table, NUMBER_VAL(42.5), 4);
  printResult("Number keys", numberPassed);
  allPassed = allPassed && numberPassed;

  ObjString* stringKey = copyString("key", 3);
  tableSet(&table, OBJ_VAL(stringKey), NUMBER_VAL(5));
  bool stringPassed =
      hasNumber(&table, OBJ_VAL(copyString("key", 3)), 5);
  printResult("String key", stringPassed);
  allPassed = allPassed && stringPassed;

  for (int i = 0; i < 1000; i++) {
    tableSet(&table, NUMBER_VAL(i), NUMBER_VAL(i));
  }
  for (int i = 0; i < 1000; i += 2) {
    tableDelete(&table, NUMBER_VAL(i));
    tableSet(&table, NUMBER_VAL(i), NUMBER_VAL(i + 1));
  }

  bool growthPassed = true;
  for (int i = 0; i < 1000; i++) {
    double expected = i % 2 == 0 ? i + 1 : i;
    if (!hasNumber(&table, NUMBER_VAL(i), expected)) {
      growthPassed = false;
      break;
    }
  }
  printResult("Growth and deletion", growthPassed);
  allPassed = allPassed && growthPassed;

  freeTable(&table);
  freeVM();
  return allPassed ? 0 : 1;
}

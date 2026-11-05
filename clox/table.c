#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"

#define TABLE_MAX_LOAD 0.75

void initTable(Table* table) {
  table->count = 0;
  table->capacity = 0;
  table->entries = NULL;
}

void freeTable(Table* table) {
  FREE_ARRAY(Entry, table->entries, table->capacity);
  initTable(table);
}

static uint32_t hashNumber(double number) {
  // Lox considers -0 and 0 equal, so they need the same hash.
  if (number == 0) number = 0;

  uint64_t bits;
  memcpy(&bits, &number, sizeof(double));
  bits ^= bits >> 33;
  bits *= UINT64_C(0xff51afd7ed558ccd);
  bits ^= bits >> 33;
  return (uint32_t)(bits ^ (bits >> 32));
}

static uint32_t hashValue(Value key) {
  switch (key.type) {
    case VAL_BOOL:
      return AS_BOOL(key) ? 0x9e3779b9u : 0x85ebca6bu;
    case VAL_NIL:
      return 0x27d4eb2du;
    case VAL_NUMBER:
      return hashNumber(AS_NUMBER(key));
    case VAL_OBJ:
      return AS_STRING(key)->hash;
    default:
      return 0; // Unreachable.
  }
}

static Entry* findEntry(Entry* entries, int capacity, Value key) {
  uint32_t index = hashValue(key) % capacity;
  Entry* tombstone = NULL;

  for (;;) {
    Entry* entry = &entries[index];
    if (!entry->occupied) {
      if (!entry->tombstone) {
        return tombstone != NULL ? tombstone : entry;
      }
      if (tombstone == NULL) tombstone = entry;
    } else if (valuesEqual(entry->key, key)) {
      return entry;
    }

    index = (index + 1) % capacity;
  }
}

bool tableGet(Table* table, Value key, Value* value) {
  if (table->count == 0) return false;

  Entry* entry = findEntry(table->entries, table->capacity, key);
  if (!entry->occupied) return false;

  *value = entry->value;
  return true;
}

static void adjustCapacity(Table* table, int capacity) {
  Entry* entries = ALLOCATE(Entry, capacity);
  for (int i = 0; i < capacity; i++) {
    entries[i].key = NIL_VAL;
    entries[i].value = NIL_VAL;
    entries[i].occupied = false;
    entries[i].tombstone = false;
  }

  table->count = 0;
  for (int i = 0; i < table->capacity; i++) {
    Entry* entry = &table->entries[i];
    if (!entry->occupied) continue;

    Entry* dest = findEntry(entries, capacity, entry->key);
    dest->key = entry->key;
    dest->value = entry->value;
    dest->occupied = true;
    dest->tombstone = false;
    table->count++;
  }

  FREE_ARRAY(Entry, table->entries, table->capacity);
  table->entries = entries;
  table->capacity = capacity;
}

bool tableSet(Table* table, Value key, Value value) {
  if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
    int capacity = GROW_CAPACITY(table->capacity);
    adjustCapacity(table, capacity);
  }

  Entry* entry = findEntry(table->entries, table->capacity, key);
  bool isNewKey = !entry->occupied;
  if (isNewKey && !entry->tombstone) table->count++;

  entry->key = key;
  entry->value = value;
  entry->occupied = true;
  entry->tombstone = false;
  return isNewKey;
}

bool tableDelete(Table* table, Value key) {
  if (table->count == 0) return false;

  Entry* entry = findEntry(table->entries, table->capacity, key);
  if (!entry->occupied) return false;

  entry->key = NIL_VAL;
  entry->value = NIL_VAL;
  entry->occupied = false;
  entry->tombstone = true;
  return true;
}

void tableAddAll(Table* from, Table* to) {
  for (int i = 0; i < from->capacity; i++) {
    Entry* entry = &from->entries[i];
    if (entry->occupied) {
      tableSet(to, entry->key, entry->value);
    }
  }
}

ObjString* tableFindString(Table* table, const char* chars,
                           int length, uint32_t hash) {
  if (table->count == 0) return NULL;

  uint32_t index = hash % table->capacity;
  for (;;) {
    Entry* entry = &table->entries[index];
    if (!entry->occupied) {
      if (!entry->tombstone) return NULL;
    } else if (IS_STRING(entry->key)) {
      ObjString* string = AS_STRING(entry->key);
      if (string->length == length && string->hash == hash &&
          memcmp(string->chars, chars, length) == 0) {
        return string;
      }
    }

    index = (index + 1) % table->capacity;
  }
}

#include <stdlib.h>

#include "chunk.h"
#include "memory.h"

void initChunk(Chunk* chunk) {
  chunk->count = 0;
  chunk->capacity = 0;
  chunk->code = NULL;
  chunk->lineCount = 0;
  chunk->lineCapacity = 0;
  chunk->lines = NULL;
  initValueArray(&chunk->constants);
}
void freeChunk(Chunk* chunk) {
  FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
  FREE_ARRAY(LineRun, chunk->lines, chunk->lineCapacity);
  freeValueArray(&chunk->constants);
  initChunk(chunk);
}
void writeChunk(Chunk* chunk, uint8_t byte, int line) {
  if (chunk->capacity < chunk->count + 1) {
    int oldCapacity = chunk->capacity;
    chunk->capacity = GROW_CAPACITY(oldCapacity);
    chunk->code = GROW_ARRAY(uint8_t, chunk->code,
        oldCapacity, chunk->capacity);
  }

  chunk->code[chunk->count] = byte;
  chunk->count++;

  if (chunk->lineCount > 0 &&
      chunk->lines[chunk->lineCount - 1].line == line) {
    chunk->lines[chunk->lineCount - 1].byteCount++;
    return;
  }

  if (chunk->lineCapacity < chunk->lineCount + 1) {
    int oldCapacity = chunk->lineCapacity;
    chunk->lineCapacity = GROW_CAPACITY(oldCapacity);
    chunk->lines = GROW_ARRAY(LineRun, chunk->lines,
        oldCapacity, chunk->lineCapacity);
  }

  LineRun* run = &chunk->lines[chunk->lineCount++];
  run->line = line;
  run->byteCount = 1;
}
int getLine(Chunk* chunk, int instruction) {
  int bytesSeen = 0;

  for (int i = 0; i < chunk->lineCount; i++) {
    bytesSeen += chunk->lines[i].byteCount;
    if (instruction < bytesSeen) {
      return chunk->lines[i].line;
    }
  }

  return -1;
}
int addConstant(Chunk* chunk, Value value) {
  writeValueArray(&chunk->constants, value);
  return chunk->constants.count - 1;
}
void writeConstant(Chunk* chunk, Value value, int line) {
  int index = addConstant(chunk, value);

  if (index <= UINT8_MAX) {
    writeChunk(chunk, OP_CONSTANT, line);
    writeChunk(chunk, (uint8_t)index, line);
    return;
  }

  writeChunk(chunk, OP_CONSTANT_LONG, line);
  writeChunk(chunk, (uint8_t)((index >> 16) & 0xff), line);
  writeChunk(chunk, (uint8_t)((index >> 8) & 0xff), line);
  writeChunk(chunk, (uint8_t)(index & 0xff), line);
}

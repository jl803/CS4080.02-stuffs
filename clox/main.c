#include "common.h"
#include "chunk.h"
#include "debug.h"

int main(int argc, const char* argv[]) {
  Chunk chunk;
  initChunk(&chunk);

  writeConstant(&chunk, 1.2, 123);

  while (chunk.constants.count < 256) {
    addConstant(&chunk, chunk.constants.count);
  }
  writeConstant(&chunk, 999.0, 124);

  writeChunk(&chunk, OP_RETURN, 125);

  disassembleChunk(&chunk, "test chunk");
  freeChunk(&chunk);
  return 0;
}

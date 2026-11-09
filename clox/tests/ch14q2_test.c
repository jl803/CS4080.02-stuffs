#include "../chunk.h"
#include "../debug.h"

int main(void) {
  Chunk chunk;
  initChunk(&chunk);

  writeConstant(&chunk, 1.2, 123);
  for (int i = 1; i < 256; i++) {
    addConstant(&chunk, i);
  }
  writeConstant(&chunk, 999, 124);
  writeChunk(&chunk, OP_RETURN, 125);

  disassembleChunk(&chunk, "test chunk");

  bool passed =
      chunk.code[0] == OP_CONSTANT &&
      chunk.code[2] == OP_CONSTANT_LONG &&
      chunk.constants.count == 257;

  freeChunk(&chunk);
  return passed ? 0 : 1;
}

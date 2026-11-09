#include <stdbool.h>

#include "../chunk.h"
#include "../debug.h"

int main(void) {
  Chunk chunk;
  initChunk(&chunk);

  writeConstant(&chunk, 1.2, 123);
  writeChunk(&chunk, OP_RETURN, 124);

  bool passed =
      chunk.lineCount == 2 &&
      getLine(&chunk, 0) == 123 &&
      getLine(&chunk, 1) == 123 &&
      getLine(&chunk, 2) == 124;

  disassembleChunk(&chunk, "test chunk");
  freeChunk(&chunk);
  return passed ? 0 : 1;
}

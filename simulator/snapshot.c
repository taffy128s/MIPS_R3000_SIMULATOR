#include "load.h"
#include "snapshot.h"

void dumpSnap() {
    fprintf(snap, "cycle %u\n", cycle++);
    unsigned i;
    for (i = 0; i < 32; ++i) {
        fprintf(snap, "$%02u: 0x", i);
        fprintf(snap, "%08X\n", reg[i]);
    }
    fprintf(snap, "PC: 0x");
    fprintf(snap, "%08X\n\n\n", PC + insPos);
}

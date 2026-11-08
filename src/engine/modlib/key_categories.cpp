// 0x4b000: key-code -> category
#include "slinky/engine.h"
extern "C" int key_category(int k) {
    switch (k) {
    case 0x1: case 0x2: case 0x3: case 0x14: case 0x18: case 0x2d: case 0x57: case 0x9f: return 2;
    case 0x4: case 0x5: case 0x11: case 0x79: case 0xa2: return 3;
    case 0x23: return 1;
    case 0x31: return 4;
    default: return -1;
    }
}

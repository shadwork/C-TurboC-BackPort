#include "bios.h"
#include "../pccore/pccore.h"

int bioskey(int cmd) {
    int current_key;
    switch (cmd) {
        case 0:
            current_key = pccore.key;
            pccore.key = 0; 
            return current_key;
        case 1:
            return pccore.key;
        case 2:
            return pccore.memory[0x417];
        default:
            return 0;
    }
}
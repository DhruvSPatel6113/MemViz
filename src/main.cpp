#include <iostream>
#include "MemoryManager.h"

int main() {
    MemoryManager engine;

    engine.allocateVariable("x", "int", "10");

    engine.allocateVariable("ptr", "int*", "0x1000", true, "0x1000"); 

    auto history = engine.getHistory();
    std::cout << "--- Memory Snapshot at Step 2 ---" << std::endl;

    for (const auto& var : history[2].stack[0].variables) {
        std::cout << var.address << " | Type: " << var.type 
                  << " | Name: " << var.name << " | Value: " << var.value;
        if (var.isPointer) {
            std::cout << " (Points to: " << var.pointsTo << ")";
        }
        std::cout << std::endl;
    }

    return 0;
}
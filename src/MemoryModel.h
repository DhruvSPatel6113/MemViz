#pragma once
#include <string>
#include <vector>

struct MemoryVariable {
    std::string name;
    std::string type;
    std::string value;
    std::string address;
    bool isPointer = false;
    std::string pointsTo = "";
    bool isReference = false;
};

struct StackFrame {
    std::string functionName;
    std::vector<MemoryVariable> variables;
};

struct MemorySnapshot {
    int stepNumber;
    std::vector<StackFrame> stack;
    std::vector<MemoryVariable> heap;
    std::vector<std::string> consoleOut;
    int currentLine; // NEW: The pointer for the UI
};
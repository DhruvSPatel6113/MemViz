#include <emscripten/bind.h>
#include "MemoryManager.h"

using namespace emscripten;

EMSCRIPTEN_BINDINGS(memory_module) {
    value_object<MemoryVariable>("MemoryVariable")
        .field("name", &MemoryVariable::name)
        .field("type", &MemoryVariable::type)
        .field("value", &MemoryVariable::value)
        .field("address", &MemoryVariable::address)
        .field("isPointer", &MemoryVariable::isPointer)
        .field("pointsTo", &MemoryVariable::pointsTo)
        .field("isReference", &MemoryVariable::isReference);

    register_vector<MemoryVariable>("VectorMemoryVariable");

    value_object<StackFrame>("StackFrame")
        .field("functionName", &StackFrame::functionName)
        .field("variables", &StackFrame::variables);

    register_vector<StackFrame>("VectorStackFrame");
    register_vector<std::string>("VectorString");

    value_object<MemorySnapshot>("MemorySnapshot")
        .field("stepNumber", &MemorySnapshot::stepNumber)
        .field("stack", &MemorySnapshot::stack)
        .field("heap", &MemorySnapshot::heap)
        .field("consoleOut", &MemorySnapshot::consoleOut)
        .field("currentLine", &MemorySnapshot::currentLine); 

    register_vector<MemorySnapshot>("VectorMemorySnapshot");

    class_<MemoryManager>("MemoryManager")
        .constructor<>()
        .function("getHistory", &MemoryManager::getHistory)
        .function("parseAndSimulate", &MemoryManager::parseAndSimulate)
        .function("clearHistory", &MemoryManager::clearHistory);
}
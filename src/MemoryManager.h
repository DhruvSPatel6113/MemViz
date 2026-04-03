#pragma once
#include "MemoryModel.h"
#include <iostream>
#include <sstream>
#include <regex>
#include <map>
#include <vector>
#include <algorithm>

struct ClassProperty {
    std::string type;
    std::string name;
    std::string sourceClass;
    std::string defaultValue;
};

class MemoryManager {
private:
    std::vector<MemorySnapshot> history;
    int currentStep, nextStackAddress, nextHeapAddress, activeLine;
    std::map<std::string, std::string> functionDefinitions;
    std::map<std::string, std::vector<ClassProperty>> classBlueprints;
    
    std::string generateStackAddress();
    std::string generateHeapAddress();
    std::string findAddressOf(const std::string& targetName);
    std::string findPointsToOf(const std::string& varName);
    std::string getValueOf(const std::string& name, const std::string& ctx);
    std::string resolveValue(const std::string& raw, const std::string& ctx);
    std::string resolveCompoundExpr(const std::string& expr);
    void executeBlock(const std::string& code, const std::string& ctx = "");
    void processCout(const std::string& line, const std::string& ctx);
    bool isKnownType(const std::string& type);

public:
    MemoryManager();
    void pushStackFrame(const std::string& name);
    void popStackFrame();
    void allocateVariable(const std::string& name, const std::string& type, const std::string& value, bool isPtr=false, const std::string& pTo="", bool isRef=false);
    std::string allocateHeapVariable(const std::string& type, const std::string& value);
    void freeHeapMemory(const std::string& addr);
    
    void updateVariableValue(const std::string& name, const std::string& val, const std::string& ctx="", const std::string& pTo="");
    void updateVariableByAddress(const std::string& addr, const std::string& val);
    
    void printToConsole(const std::string& text);
    void saveSnapshot();
    std::vector<MemorySnapshot> getHistory();
    void parseAndSimulate(const std::string& sourceCode);
    void clearHistory();
};
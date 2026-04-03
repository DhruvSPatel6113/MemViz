#include "MemoryManager.h"

int safeStoi(const std::string& str) {
    int res = 0; std::stringstream ss(str); ss >> res; return res;
}

MemoryManager::MemoryManager() { clearHistory(); }

void MemoryManager::clearHistory() {
    history.clear(); functionDefinitions.clear(); classBlueprints.clear();
    currentStep = 0; nextStackAddress = 0x1000; nextHeapAddress = 0x8000; activeLine = 0;
    MemorySnapshot initialState; initialState.stepNumber = 0; initialState.currentLine = 0;
    history.push_back(initialState);
}

std::string MemoryManager::generateStackAddress() {
    std::stringstream ss; ss << "0x" << std::hex << std::uppercase << nextStackAddress;
    nextStackAddress += 4; return ss.str();
}

std::string MemoryManager::generateHeapAddress() {
    std::stringstream ss; ss << "0x" << std::hex << std::uppercase << nextHeapAddress;
    nextHeapAddress += 4; return ss.str();
}

void MemoryManager::pushStackFrame(const std::string& name) {
    StackFrame f; f.functionName = name;
    history.back().stack.push_back(f); saveSnapshot();
}

void MemoryManager::popStackFrame() {
    if (!history.back().stack.empty()) { 
        for (const auto& var : history.back().stack.back().variables) {
            if (classBlueprints.count(var.type) && var.name.find('.') == std::string::npos && var.value == "Object") {
                printToConsole("~ Destructor called: " + var.name);
            }
        }
        history.back().stack.pop_back(); saveSnapshot(); 
    }
}

void MemoryManager::allocateVariable(const std::string& n, const std::string& t, const std::string& v, bool p, const std::string& pt, bool r) {
    if (history.back().stack.empty()) return;
    MemoryVariable var = {n, t, v, generateStackAddress(), p, pt, r};
    history.back().stack.back().variables.push_back(var); saveSnapshot();
}

std::string MemoryManager::allocateHeapVariable(const std::string& t, const std::string& v) {
    std::string addr = generateHeapAddress();
    MemoryVariable hv = {"(Dynamic)", t, v, addr, false, "", false};
    history.back().heap.push_back(hv);
    if (classBlueprints.count(t)) {
        for (auto& p : classBlueprints[t]) {
            std::string propAddr = generateHeapAddress();
            std::string displayType = p.type;
            if (p.sourceClass != t) displayType += " [Inherited]";
            std::string initVal = p.defaultValue.empty() ? "Uninitialized" : p.defaultValue;
            MemoryVariable propVar = {addr + "->" + p.name, displayType, initVal, propAddr, false, "", false};
            history.back().heap.push_back(propVar);
        }
    }
    saveSnapshot(); return addr;
}

void MemoryManager::freeHeapMemory(const std::string& addr) {
    auto& h = history.back().heap;
    h.erase(std::remove_if(h.begin(), h.end(), [&](const MemoryVariable& v){ return v.address == addr || v.name.find(addr + "->") == 0; }), h.end());
    saveSnapshot();
}

void MemoryManager::printToConsole(const std::string& txt) {
    history.back().consoleOut.push_back(txt); saveSnapshot();
}

void MemoryManager::saveSnapshot() {
    currentStep++; MemorySnapshot ns = history.back();
    ns.stepNumber = currentStep; ns.currentLine = activeLine;
    history.push_back(ns);
}

std::string MemoryManager::getValueOf(const std::string& n, const std::string& ctx) {
    std::string s = ctx.empty() ? n : (ctx.find("0x8") == 0 ? ctx + "->" + n : ctx + "." + n);
    for (int i = history.back().stack.size() - 1; i >= 0; i--) {
        for (auto& v : history.back().stack[i].variables) if (v.name == s || v.name == n) return v.value;
    }
    for (auto& v : history.back().heap) if (v.name == s || v.name == n) return v.value;
    return "";
}

std::string MemoryManager::resolveCompoundExpr(const std::string& expr) {
    std::regex arrowScopeRe(R"(([a-zA-Z_]\w*)->([a-zA-Z_]\w*)::([a-zA-Z_]\w*))");
    std::smatch m;
    if (std::regex_match(expr, m, arrowScopeRe)) {
        std::string ptrName = m[1].str();
        std::string heapAddr = getValueOf(ptrName, "");
        return getValueOf(heapAddr + "->" + m[2].str() + "::" + m[3].str(), "");
    }
    std::regex arrowRe(R"(([a-zA-Z_]\w*)->([a-zA-Z_]\w*))");
    if (std::regex_match(expr, m, arrowRe)) {
        std::string heapAddr = getValueOf(m[1].str(), "");
        return getValueOf(heapAddr + "->" + m[2].str(), "");
    }
    std::regex dotScopeRe(R"(([a-zA-Z_]\w*)\.([a-zA-Z_]\w*)::([a-zA-Z_]\w*))");
    if (std::regex_match(expr, m, dotScopeRe)) {
        return getValueOf(m[1].str() + "." + m[2].str() + "::" + m[3].str(), "");
    }
    std::regex dotRe(R"(([a-zA-Z_]\w*)\.([a-zA-Z_]\w*))");
    if (std::regex_match(expr, m, dotRe)) {
        return getValueOf(m[1].str() + "." + m[2].str(), "");
    }
    std::regex arrAccessRe(R"(([a-zA-Z_]\w*)\[([^\]]+)\])");
    if (std::regex_match(expr, m, arrAccessRe)) {
        std::string idxStr = std::regex_replace(m[2].str(), std::regex("^\\s+|\\s+$"), "");
        std::string resolvedIdx = resolveValue(idxStr, "");
        std::string heapAddr = getValueOf(m[1].str(), "");
        if (!heapAddr.empty())
            return getValueOf(heapAddr + "->[" + resolvedIdx + "]", "");
        return "";
    }
    std::regex derefRe(R"(\*([a-zA-Z_]\w*))");
    if (std::regex_match(expr, m, derefRe)) {
        std::string addr = getValueOf(m[1].str(), "");
        if (!addr.empty()) {
            for (auto& f : history.back().stack)
                for (auto& v : f.variables) if (v.address == addr) return v.value;
            for (auto& v : history.back().heap) if (v.address == addr) return v.value;
        }
        return "";
    }
    std::regex dderefRe(R"(\*\*([a-zA-Z_]\w*))");
    if (std::regex_match(expr, m, dderefRe)) {
        std::string addr1 = getValueOf(m[1].str(), "");
        if (!addr1.empty()) {
            std::string addr2 = "";
            for (auto& f : history.back().stack)
                for (auto& v : f.variables) if (v.address == addr1) { addr2 = v.value; break; }
            if (!addr2.empty()) {
                for (auto& f : history.back().stack)
                    for (auto& v : f.variables) if (v.address == addr2) return v.value;
                for (auto& v : history.back().heap) if (v.address == addr2) return v.value;
            }
        }
        return "";
    }
    return "";
}

std::string MemoryManager::resolveValue(const std::string& r, const std::string& c) {
    if (r.empty() || r == "Uninitialized" || std::isdigit(static_cast<unsigned char>(r[0])) || r[0] == '-' || r[0] == '"') return r;
    if (r.find("->") != std::string::npos || r.find('.') != std::string::npos || r.find('[') != std::string::npos || r.find('*') != std::string::npos) {
        std::string cv = resolveCompoundExpr(r);
        if (!cv.empty()) return cv;
    }
    std::string v = getValueOf(r, c); return v.empty() ? r : v;
}

void MemoryManager::updateVariableValue(const std::string& n, const std::string& v, const std::string& c, const std::string& pTo) {
    std::string s = (c.empty() || n.find('.') != std::string::npos || n.find("->") != std::string::npos) ? n : (c.find("0x8") == 0 ? c + "->" + n : c + "." + n);
    for (auto& f : history.back().stack) {
        for (auto& var : f.variables) {
            if (var.name == s) { 
                var.value = v; 
                if (!pTo.empty()) { var.pointsTo = pTo; var.isPointer = true; }
                saveSnapshot(); return; 
            }
        }
    }
    for (auto& var : history.back().heap) {
        if (var.name == s) { 
            var.value = v; 
            if (!pTo.empty()) { var.pointsTo = pTo; var.isPointer = true; }
            saveSnapshot(); return; 
        }
    }
}

void MemoryManager::updateVariableByAddress(const std::string& addr, const std::string& val) {
    for (auto& f : history.back().stack) {
        for (auto& var : f.variables) {
            if (var.address == addr) { var.value = val; saveSnapshot(); return; }
        }
    }
    for (auto& var : history.back().heap) {
        if (var.address == addr) { var.value = val; saveSnapshot(); return; }
    }
}

std::vector<MemorySnapshot> MemoryManager::getHistory() { return history; }
static size_t findArithOp(const std::string& s) {
    for (size_t i = 1; i < s.length(); i++) {
        char ch = s[i];
        if (ch == '+' || ch == '-' || ch == '*' || ch == '/') {
            return i;
        }
    }
    return std::string::npos;
}

void MemoryManager::processCout(const std::string& l, const std::string& c) {
    std::string out = ""; std::regex seg(R"(<<\s*([^<;]+))");
    for (std::sregex_iterator i(l.begin(), l.end(), seg), end; i != end; ++i) {
        std::string s = std::regex_replace((*i)[1].str(), std::regex("^\\s+|\\s+$"), "");
        if (s == "endl") out += "\n";
        else if (!s.empty() && s.front() == '"') out += s.substr(1, s.length() - 2);
        else {
            size_t opPos = findArithOp(s);
            if (opPos != std::string::npos) {
                char op = s[opPos];
                std::string lStr = s.substr(0, opPos), rStr = s.substr(opPos + 1);
                lStr = std::regex_replace(lStr, std::regex("^\\s+|\\s+$"), "");
                rStr = std::regex_replace(rStr, std::regex("^\\s+|\\s+$"), "");
                int lVal = safeStoi(resolveValue(lStr, c));
                int rVal = safeStoi(resolveValue(rStr, c));
                int result = 0;
                switch (op) {
                    case '+': result = lVal + rVal; break;
                    case '-': result = lVal - rVal; break;
                    case '*': result = lVal * rVal; break;
                    case '/': result = (rVal != 0) ? lVal / rVal : 0; break;
                }
                out += std::to_string(result);
            } else {
                std::string resolved = resolveValue(s, c);
                if (resolved.size() >= 2 && resolved.front() == '"' && resolved.back() == '"')
                    resolved = resolved.substr(1, resolved.size() - 2);
                out += resolved;
            }
        }
    }
    printToConsole("> " + out);
}

void MemoryManager::parseAndSimulate(const std::string& sourceCode) {
    clearHistory();
    std::string cbName = "", cbBody = "", cClassName = "";
    int bLevel = 0; bool inF = false, inC = false; int absoluteLine = 0;

    std::istringstream stream(sourceCode); std::string line;
    std::regex cStart(R"(class\s+([a-zA-Z_]\w*))");
    std::regex fStart(R"((?:[a-zA-Z_]\w*\s+)?([a-zA-Z_]\w*)\s*\(([^)]*)\))");
    std::regex vRegex(R"((int|float|double|char|string)\s*([\*\&]?)\s+([a-zA-Z_]\w*)\s*(?:=\s*([^;]+))?;)");

    while (std::getline(stream, line)) {
        absoluteLine++;
        if (line.find("using") != std::string::npos || line.find("public:") != std::string::npos || line.find("private:") != std::string::npos || line.find("protected:") != std::string::npos || line.find("friend ") != std::string::npos || line.find("operator") != std::string::npos) continue;

        std::smatch m;
        bool isFuncStartLine = false;

        if (!inF && !inC && std::regex_search(line, m, cStart)) {
            cClassName = m[1].str(); inC = true; bLevel = (line.find('{') != std::string::npos);
            classBlueprints[cClassName] = {}; 
        }
        else if (!inF && std::regex_search(line, m, fStart) && line.find(";") == std::string::npos) {
            std::string pName = m[1].str();
            if (pName != "if" && pName != "while" && pName != "for" && pName != "switch") {
                std::string params = m[2].str();
                int paramCount = params.empty() ? 0 : std::count(params.begin(), params.end(), ',') + 1;
                if (inC && pName == cClassName) {
                    cbName = cClassName + "::" + cClassName + "_" + std::to_string(paramCount);
                    
                    std::regex paramSplit(R"([a-zA-Z_]\w*\s+([a-zA-Z_]\w*))");
                    std::sregex_iterator pNext(params.begin(), params.end(), paramSplit), pEnd;
                    std::string paramMapCode = ""; int pIdx = 0;
                    while (pNext != pEnd) {
                        paramMapCode += "@@@" + std::to_string(absoluteLine) + "@@@" + "auto " + (*pNext)[1].str() + " = __arg" + std::to_string(pIdx++) + ";\n"; pNext++;
                    }
                    cbBody += paramMapCode;
                } else if (inC) {
                    cbName = cClassName + "::" + pName;
                } else {
                    cbName = pName + "_" + std::to_string(paramCount);

                    std::regex paramSplit(R"([a-zA-Z_]\w*\s+([a-zA-Z_]\w*))");
                    std::sregex_iterator pNext(params.begin(), params.end(), paramSplit), pEnd;
                    std::string paramMapCode = ""; int pIdx = 0;
                    while (pNext != pEnd) {
                        paramMapCode += "@@@" + std::to_string(absoluteLine) + "@@@" + "auto " + (*pNext)[1].str() + " = __arg" + std::to_string(pIdx++) + ";\n"; pNext++;
                    }
                    cbBody += paramMapCode;
                }
                inF = true;
                isFuncStartLine = true;
            }
        }

        int classBaseLevel = inC ? 1 : 0;
        int openBracesOnLine = std::count(line.begin(), line.end(), '{');
        int closeBracesOnLine = std::count(line.begin(), line.end(), '}');
        bLevel += openBracesOnLine;

        if (inF) {
            std::string marker = "@@@" + std::to_string(absoluteLine) + "@@@";
            if (isFuncStartLine) {
                size_t pos = line.find('{');
                if (pos != std::string::npos && pos + 1 < line.length()) {
                    std::string rest = line.substr(pos + 1);
                    size_t endPos = rest.find('}');
                    if (endPos != std::string::npos) cbBody += marker + rest.substr(0, endPos) + "\n";
                    else cbBody += marker + rest + "\n";
                }
            } else {
                size_t endPos = line.rfind('}');
                if (bLevel - closeBracesOnLine == classBaseLevel && endPos != std::string::npos) {
                    cbBody += marker + line.substr(0, endPos) + "\n";
                } else {
                    cbBody += marker + line + "\n";
                }
            }
            bLevel -= closeBracesOnLine;
            if (bLevel == classBaseLevel && closeBracesOnLine > 0) {
                functionDefinitions[cbName] = cbBody;
                inF = false; cbBody = "";
            }
        } else if (inC) {
            bLevel -= closeBracesOnLine;
            if (bLevel == 0 && closeBracesOnLine > 0) inC = false;
            else if (std::regex_search(line, m, vRegex) && line.find('(') == std::string::npos) {
                std::string defVal = m[4].matched ? m[4].str() : "";
                defVal = std::regex_replace(defVal, std::regex("^\\s+|\\s+$"), "");
                classBlueprints[cClassName].push_back({m[1].str()+m[2].str(), m[3].str(), cClassName, defVal});
            }
        } else {
            bLevel -= closeBracesOnLine;
        }
    }

    bool inheritanceChanged = true;
    while (inheritanceChanged) {
        inheritanceChanged = false;
        for (auto& pair : classBlueprints) {
            std::regex bRegex("class\\s+" + pair.first + "\\s*:\\s*([^{]+)");
            std::smatch m2;
            if (std::regex_search(sourceCode, m2, bRegex)) {
                std::stringstream ss(m2[1].str()); std::string b;
                while(std::getline(ss, b, ',')) {
                    b = std::regex_replace(b, std::regex("(public|protected|private|virtual|\\s)"), "");
                    if (classBlueprints.count(b)) {
                        for(auto& prop : classBlueprints[b]) {
                            bool childHasSameName = false, alreadyAdded = false;
                            std::string qualifiedName = prop.sourceClass + "::" + prop.name;
                            for(auto& e : pair.second) {
                                if (e.name == prop.name && e.sourceClass == pair.first) childHasSameName = true;
                                if ((e.name == prop.name && e.sourceClass == prop.sourceClass) || e.name == qualifiedName) alreadyAdded = true;
                            }
                            if (!alreadyAdded) {
                                if (childHasSameName) {
                                    ClassProperty qualified = prop;
                                    qualified.name = qualifiedName;
                                    pair.second.push_back(qualified);
                                } else {
                                    pair.second.push_back(prop);
                                }
                                inheritanceChanged = true;
                            }
                        }
                    }
                }
            }
        }
    }
    if (functionDefinitions.count("main_0")) { pushStackFrame("main()"); executeBlock(functionDefinitions["main_0"]); popStackFrame(); }
}

void MemoryManager::executeBlock(const std::string& code, const std::string& ctx) {
    std::istringstream stream(code); std::string line;
    std::regex dRegex(R"(^\s*([a-zA-Z_]\w*)\s*([\*\&]?)\s*([a-zA-Z_]\w*)(?:\s*\(([^)]*)\))?\s*(?:=\s*([^;]+))?;)");
    std::regex aRegex(R"(^\s*([\*]?)\s*([a-zA-Z_]\w*)(?:\s*(\.|->)\s*([a-zA-Z_]\w*))?\s*=\s*([^;]+);)");
    std::regex arrRegex(R"(new\s+([a-zA-Z_]\w*)\s*\[\s*([^\]]+)\s*\])");
    std::regex objMethodRegex(R"(^\s*([a-zA-Z_]\w*)\.([a-zA-Z_]\w*)\(\s*\)\s*;)");
    std::regex funcCallRegex(R"(^\s*([a-zA-Z_]\w*)\s*\(([^)]*)\)\s*;)");
    std::regex delRegex(R"(delete\s*(?:\[\])?\s*([a-zA-Z_]\w*))");
    std::regex scopeAssignRegex(R"(^\s*([a-zA-Z_]\w*)(?:\.|->)([a-zA-Z_]\w*)::([a-zA-Z_]\w*)\s*=\s*([^;]+);)");
    std::regex markerRegex(R"(^@@@(\d+)@@@)");

    while (std::getline(stream, line)) {
        std::smatch markerMatch;
        if (std::regex_search(line, markerMatch, markerRegex)) {
            activeLine = safeStoi(markerMatch[1].str());
            line = std::regex_replace(line, markerRegex, "");
        }

        if (line.find("cout") != std::string::npos) { processCout(line, ctx); continue; }
        if (line.find("vector") != std::string::npos || line.find("return") != std::string::npos) continue;
        {
            std::regex forHeaderRegex(R"(\bfor\s*\(\s*(.*?)\s*;\s*(.*?)\s*;\s*(.*?)\s*\))");
            std::smatch forMatch;
            if (std::regex_search(line, forMatch, forHeaderRegex)) {
                std::string init = std::regex_replace(forMatch[1].str(), std::regex("^\\s+|\\s+$"), "");
                std::string cond = std::regex_replace(forMatch[2].str(), std::regex("^\\s+|\\s+$"), "");
                std::string incr = std::regex_replace(forMatch[3].str(), std::regex("^\\s+|\\s+$"), "");

                int bl = std::count(line.begin(), line.end(), '{') - std::count(line.begin(), line.end(), '}');
                std::string forBody = "";

                size_t bracePos = line.find('{');
                if (bracePos != std::string::npos) {
                    std::string afterBrace = line.substr(bracePos + 1);
                    if (bl == 0) {
                        size_t closePos = afterBrace.rfind('}');
                        if (closePos != std::string::npos) forBody = afterBrace.substr(0, closePos);
                    } else {
                        forBody += afterBrace + "\n";
                    }
                }

                std::string bodyLine;
                while (bl > 0 && std::getline(stream, bodyLine)) {
                    bl += std::count(bodyLine.begin(), bodyLine.end(), '{');
                    bl -= std::count(bodyLine.begin(), bodyLine.end(), '}');
                    if (bl <= 0) {
                        size_t cp = bodyLine.rfind('}');
                        if (cp != std::string::npos && cp > 0) forBody += bodyLine.substr(0, cp) + "\n";
                        break;
                    }
                    forBody += bodyLine + "\n";
                }

                std::regex initDeclRe(R"(([a-zA-Z_]\w*)\s+([a-zA-Z_]\w*)\s*=\s*(.+))");
                std::smatch initM;
                std::string loopVar = "";
                if (std::regex_match(init, initM, initDeclRe)) {
                    loopVar = initM[2].str();
                    allocateVariable(loopVar, initM[1].str(), resolveValue(initM[3].str(), ctx));
                }

                int maxIter = 1000;
                while (maxIter-- > 0) {
                    std::regex condRe(R"(([a-zA-Z_]\w*)\s*(<=|>=|<|>|!=|==)\s*(.+))");
                    std::smatch condM;
                    if (!std::regex_match(cond, condM, condRe)) break;
                    int lhs = safeStoi(resolveValue(condM[1].str(), ctx));
                    int rhs = safeStoi(resolveValue(std::regex_replace(condM[3].str(), std::regex("^\\s+|\\s+$"), ""), ctx));
                    std::string op = condM[2].str();
                    bool ok = false;
                    if (op=="<") ok=lhs<rhs; else if (op=="<=") ok=lhs<=rhs;
                    else if (op==">") ok=lhs>rhs; else if (op==">=") ok=lhs>=rhs;
                    else if (op=="!=") ok=lhs!=rhs; else if (op=="==") ok=lhs==rhs;
                    if (!ok) break;

                    executeBlock(forBody, ctx);

                    if (incr.find("+=") != std::string::npos) {
                        std::regex iaRe(R"(([a-zA-Z_]\w*)\s*\+=\s*(.+))"); std::smatch im;
                        if (std::regex_match(incr, im, iaRe)) {
                            int c2=safeStoi(getValueOf(im[1].str(),ctx))+safeStoi(resolveValue(im[2].str(),ctx));
                            updateVariableValue(im[1].str(),std::to_string(c2),ctx);
                        }
                    } else if (incr.find("-=") != std::string::npos) {
                        std::regex isRe(R"(([a-zA-Z_]\w*)\s*-=\s*(.+))"); std::smatch im;
                        if (std::regex_match(incr, im, isRe)) {
                            int c2=safeStoi(getValueOf(im[1].str(),ctx))-safeStoi(resolveValue(im[2].str(),ctx));
                            updateVariableValue(im[1].str(),std::to_string(c2),ctx);
                        }
                    } else if (incr.find("++") != std::string::npos) {
                        std::string var = std::regex_replace(incr, std::regex("[+\\s]"), "");
                        updateVariableValue(var, std::to_string(safeStoi(getValueOf(var,ctx))+1), ctx);
                    } else if (incr.find("--") != std::string::npos) {
                        std::string var = std::regex_replace(incr, std::regex("[-\\s]"), "");
                        updateVariableValue(var, std::to_string(safeStoi(getValueOf(var,ctx))-1), ctx);
                    }
                }
                continue;
            }
        }
        
        std::smatch m;
        if (std::regex_search(line, m, delRegex)) {
            std::string tAddr = findPointsToOf(m[1].str());
            if (!tAddr.empty()) freeHeapMemory(tAddr);
            continue;
        }

        if (std::regex_search(line, m, funcCallRegex) && m[1].str() != "delete" && m[1].str() != "cout") {
            std::string fName = m[1].str(); std::string args = m[2].str();
            int argCount = args.empty() ? 0 : std::count(args.begin(), args.end(), ',') + 1;
            std::string overloadKey = fName + "_" + std::to_string(argCount);
            if (functionDefinitions.count(overloadKey)) {
                pushStackFrame(fName + "()");
                if (!args.empty()) {
                    std::stringstream ss(args); std::string arg; int pIdx = 0;
                    while(std::getline(ss, arg, ',')) {
                        arg = std::regex_replace(arg, std::regex("^\\s+|\\s+$"), "");
                        allocateVariable("__arg" + std::to_string(pIdx++), "auto", resolveValue(arg, ctx), false, "");
                    }
                }
                executeBlock(functionDefinitions[overloadKey], "");
                popStackFrame();
                continue;
            }
        }

        if (std::regex_search(line, m, objMethodRegex)) {
            std::string objName = m[1].str(); std::string methodName = m[2].str();
            std::string objType = "";
            for (int i = history.back().stack.size() - 1; i >= 0; i--) {
                for (const auto& var : history.back().stack[i].variables) {
                    if (var.name == objName) { objType = var.type; break; }
                }
                if (!objType.empty()) break;
            }
            std::string fullMethodName = objType + "::" + methodName;
            if (functionDefinitions.count(fullMethodName)) {
                pushStackFrame(objName + "." + methodName + "()");
                executeBlock(functionDefinitions[fullMethodName], objName);
                popStackFrame();
            }
            continue;
        }

        {
            std::regex arrAssignRe(R"(^\s*([a-zA-Z_]\w*)\[([^\]]+)\]\s*=\s*([^;]+);)");
            std::smatch am2;
            if (std::regex_search(line, am2, arrAssignRe)) {
                std::string arrName = am2[1].str();
                std::string idxExpr = std::regex_replace(am2[2].str(), std::regex("^\\s+|\\s+$"), "");
                std::string valExpr = std::regex_replace(am2[3].str(), std::regex("^\\s+|\\s+$"), "");
                std::string idxVal = resolveValue(idxExpr, ctx);
                std::string finalVal;
                size_t opPos = findArithOp(valExpr);
                if (opPos != std::string::npos) {
                    char aop = valExpr[opPos];
                    std::string ls = std::regex_replace(valExpr.substr(0,opPos), std::regex("^\\s+|\\s+$"), "");
                    std::string rs = std::regex_replace(valExpr.substr(opPos+1), std::regex("^\\s+|\\s+$"), "");
                    int lv = safeStoi(resolveValue(ls,ctx)), rv = safeStoi(resolveValue(rs,ctx)), res = 0;
                    switch(aop){case '+':res=lv+rv;break;case '-':res=lv-rv;break;case '*':res=lv*rv;break;case '/':res=rv?lv/rv:0;break;}
                    finalVal = std::to_string(res);
                } else { finalVal = resolveValue(valExpr, ctx); }
                std::string heapAddr = getValueOf(arrName, ctx);
                if (!heapAddr.empty() && heapAddr.find("0x") == 0) {
                    updateVariableValue(heapAddr + "->[" + idxVal + "]", finalVal, ctx);
                }
                continue;
            }
        }

        if (std::regex_search(line, m, scopeAssignRegex)) {
            std::string objName = m[1].str(), scopeClass = m[2].str(), member = m[3].str();
            std::string rawVal = std::regex_replace(m[4].str(), std::regex("^\\s+|\\s+$"), "");
            std::string ptrVal = getValueOf(objName, ctx);
            std::string target;
            if (!ptrVal.empty() && ptrVal.find("0x8") == 0) {
                target = ptrVal + "->" + scopeClass + "::" + member;
            } else {
                target = objName + "." + scopeClass + "::" + member;
            }
            updateVariableValue(target, resolveValue(rawVal, ctx), ctx);
        } else if (std::regex_search(line, m, dRegex) && isKnownType(m[1].str())) {
            std::string type = m[1].str(), ptrRef = m[2].str(), name = m[3].str(), args = m[4].str(), raw = m[5].str();
            std::string finalVal = raw.empty() ? "Uninitialized" : raw; 

            if (classBlueprints.count(type) && ptrRef != "*") {
                for (auto& p : classBlueprints[type]) {
                    std::string initVal = p.defaultValue.empty() ? "Uninitialized" : p.defaultValue;
                    std::string displayType = p.type;
                    if (p.sourceClass != type) displayType += " [Inherited]";
                    allocateVariable(name+"."+p.name, displayType, initVal);
                }
                allocateVariable(name, type, "Object");
                
                int argCount = args.empty() ? 0 : std::count(args.begin(), args.end(), ',') + 1;
                std::string constrName = type + "::" + type + "_" + std::to_string(argCount);
                if (functionDefinitions.count(constrName)) {
                    pushStackFrame(type + " Constructor");
                    if (argCount > 0) {
                        std::stringstream ss(args); std::string arg; int pIdx = 0;
                        while(std::getline(ss, arg, ',')) {
                            arg = std::regex_replace(arg, std::regex("^\\s+|\\s+$"), "");
                            allocateVariable("__arg" + std::to_string(pIdx++), "int", resolveValue(arg, ctx), false, "");
                        }
                    }
                    executeBlock(functionDefinitions[constrName], name);
                    popStackFrame();
                }
            } else {
                std::string pTo = "";
                if (finalVal.find("new") != std::string::npos) {
                    std::smatch am;
                    if (std::regex_search(finalVal, am, arrRegex)) {
                        std::string elemType = am[1].str();
                        int sz = safeStoi(resolveValue(am[2].str(), ctx));
                        std::string baseAddr = generateHeapAddress();
                        pTo = baseAddr;
                        finalVal = pTo;
                        MemoryVariable arrSentinel = {"(Dynamic)", elemType + "[]", std::to_string(sz) + " elements", baseAddr, false, "", false};
                        history.back().heap.push_back(arrSentinel);
                        for(int i=0; i<sz; i++) {
                            std::string elemAddr = generateHeapAddress();
                            MemoryVariable av = {baseAddr + "->" + "[" + std::to_string(i) + "]", elemType, "Uninitialized", elemAddr, false, "", false};
                            history.back().heap.push_back(av);
                        }
                        saveSnapshot();
                    } else { 
                        pTo = allocateHeapVariable(type, "Object"); 
                        finalVal = pTo;
                    }
                } else if (ptrRef == "&") {
                    pTo = findAddressOf(finalVal);
                    finalVal = pTo; 
                } else if (ptrRef == "*" && !raw.empty() && raw[0] == '&') {
                    pTo = findAddressOf(raw.substr(1));
                    finalVal = pTo;
                } else {
                    finalVal = resolveValue(finalVal, ctx);
                }
                allocateVariable(name, type + ptrRef, finalVal, ptrRef=="*", pTo, ptrRef=="&");
            }
        } else if (std::regex_search(line, m, aRegex)) {
            bool isDereference = (m[1].str() == "*");
            std::string t = m[2].str();
            
            if (!m[3].str().empty()) {
                t = (m[3].str() == "->") ? findPointsToOf(t) + "->" + m[4].str() : t + "." + m[4].str();
            }
            
            std::string rawVal = m[5].str();
            rawVal = std::regex_replace(rawVal, std::regex("^\\s+|\\s+$"), "");

            if (isDereference) {
                std::string ptrAddr = getValueOf(t, ctx); 
                updateVariableByAddress(ptrAddr, resolveValue(rawVal, ctx));
            } else {
                std::string finalVal = rawVal;
                std::string pointsTo = "";
                if (rawVal[0] == '&') {
                    pointsTo = findAddressOf(rawVal.substr(1));
                    finalVal = pointsTo;
                } else {
                    finalVal = resolveValue(rawVal, ctx);
                }
                updateVariableValue(t, finalVal, ctx, pointsTo);
            }
        }
    }
}

bool MemoryManager::isKnownType(const std::string& t) {
    return t=="int"||t=="float"||t=="double"||t=="char"||t=="string"||t=="bool"||t=="auto"||classBlueprints.count(t);
}

std::string MemoryManager::findAddressOf(const std::string& n) {
    for (int i = history.back().stack.size() - 1; i >= 0; i--) {
        for (auto& v : history.back().stack[i].variables) if(v.name == n) return v.address;
    }
    return "";
}

std::string MemoryManager::findPointsToOf(const std::string& n) {
    for (int i = history.back().stack.size() - 1; i >= 0; i--) {
        for (auto& v : history.back().stack[i].variables) if(v.name == n) return v.pointsTo;
    }
    for (auto& f : history.back().heap) if(f.name == n) return f.pointsTo;
    return "";
}
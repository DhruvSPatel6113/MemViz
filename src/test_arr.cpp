#include <iostream>
#include "MemoryManager.h"

int main() {
    MemoryManager engine;
    std::string code = R"(
#include <iostream>
using namespace std;
int main() {
    int* arr = new int[5];
    for(int i = 0; i < 5; i++) {
        arr[i] = i * 10;
    }
    cout << arr[0] << endl;
    cout << arr[4] << endl;
    return 0;
}
)";
    engine.parseAndSimulate(code);
    auto history = engine.getHistory();
    int last = history.size() - 1;
    auto& state = history[last];
    std::cout << "Console:" << std::endl;
    for (auto& l : state.consoleOut) std::cout << "  " << l << std::endl;
    std::cout << "\nHeap:" << std::endl;
    for (auto& v : state.heap) std::cout << "  " << v.name << " = " << v.value << std::endl;
    return 0;
}

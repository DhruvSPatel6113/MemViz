let engine;
let historyStates = [];
let currentStepIndex = 0;

createMemoryEngine().then(function(Module) {
    engine = new Module.MemoryManager();
    const codeInput = document.getElementById("code-input");
    
    document.getElementById("btn-run").addEventListener("click", () => {
        historyStates = [];
        currentStepIndex = 0;
        document.getElementById("step-counter").innerText = "Running...";
        
        engine.parseAndSimulate(codeInput.value);
        const historyVector = engine.getHistory();
        for (let i = 0; i < historyVector.size(); i++) {
            historyStates.push(historyVector.get(i));
        }
        updateUI();
    });

    document.getElementById("btn-next").addEventListener("click", () => {
        if (currentStepIndex < historyStates.length - 1) {
            currentStepIndex++;
            updateUI();
        }
    });

    document.getElementById("btn-prev").addEventListener("click", () => {
        if (currentStepIndex > 0) {
            currentStepIndex--;
            updateUI();
        }
    });

    // Bug 10 fix: Create console div once on init
    let consoleDiv = document.getElementById("console-container");
    if (!consoleDiv) {
        consoleDiv = document.createElement("div");
        consoleDiv.id = "console-container";
        consoleDiv.style.cssText = "background: #1e1e1e; color: #00ff00; font-family: monospace; padding: 15px; margin-top: 20px; border-radius: 5px; min-height: 120px; border: 1px solid #444; overflow-y: auto;";
        document.querySelector('.right-panel').appendChild(consoleDiv);
    }

    // Bug 9 fix: Show initial step counter state
    document.getElementById("step-counter").innerText = "Ready";
});



function updateUI() {
    if(historyStates.length === 0) return;
    const state = historyStates[currentStepIndex];
    
    document.getElementById("step-counter").innerText = `Step ${currentStepIndex} / ${historyStates.length - 1}`;
    document.getElementById("btn-prev").disabled = (currentStepIndex === 0);
    document.getElementById("btn-next").disabled = (currentStepIndex === historyStates.length - 1);
    
    const stackContainer = document.getElementById("stack-container");
    stackContainer.innerHTML = ""; 

    for (let i = 0; i < state.stack.size(); i++) {
        const frame = state.stack.get(i);
        let frameHTML = `<div class="stack-frame"><h4>${frame.functionName}</h4>`;
        let standardVars = [];
        let objectVars = {}; 

        for (let j = 0; j < frame.variables.size(); j++) {
            const v = frame.variables.get(j);
            if (v.name.includes('.')) {
                let objName = v.name.split('.')[0];
                if (!objectVars[objName]) objectVars[objName] = [];
                objectVars[objName].push(v);
            } else {
                standardVars.push(v);
            }
        }

        standardVars.forEach(v => {
            if (!v.name.includes("__arg")) {
                if (v.value === "Object") {
                    // Bug 8 fix: hidden sentinel for arrow targeting
                    frameHTML += `<div class="memory-var" id="var-${v.address}" style="display:none;"></div>`;
                    return;
                }
                frameHTML += `<div class="memory-var" id="var-${v.address}">
                    <span class="var-addr">[${v.address}]</span>
                    <span class="var-type">${v.type}</span>
                    <span class="var-name">${v.name}</span> = 
                    <span class="var-val" style="color: ${v.value==='Uninitialized' ? '#ff6b6b' : '#9cdcfe'}">${v.value}</span>
                </div>`;
            }
        });

        for (const [objName, props] of Object.entries(objectVars)) {
            frameHTML += `<div class="object-box"><div class="object-header">Object: ${objName}</div>`;
            props.forEach(v => {
                let dType = v.type;
                let badge = dType.includes("[Inherited]") ? `<span style="font-size:0.7em;background:#5d3f6a;padding:2px;border-radius:3px;margin-left:5px;color:#e6cceb;">Inherited</span>` : "";
                dType = dType.replace("[Inherited]", "").trim();
                
                frameHTML += `<div class="memory-var" id="var-${v.address}">
                    <span class="var-addr">[${v.address}]</span>
                    <span class="var-type">${dType}</span>
                    <span class="var-name">${v.name.split('.')[1]} ${badge}</span> = 
                    <span class="var-val" style="color: ${v.value==='Uninitialized' ? '#ff6b6b' : '#9cdcfe'}">${v.value}</span>
                </div>`;
            });
            frameHTML += `</div>`;
        }
        stackContainer.innerHTML += frameHTML + `</div>`;
    }

    const heapContainer = document.getElementById("heap-container");
    heapContainer.innerHTML = "";
    
    let standardHeapVars = [];
    let objectHeapVars = {}; 

    for (let i = 0; i < state.heap.size(); i++) {
        const v = state.heap.get(i);
        if (v.name.includes('->')) {
            let objAddr = v.name.split('->')[0];
            if (!objectHeapVars[objAddr]) objectHeapVars[objAddr] = [];
            objectHeapVars[objAddr].push(v);
        } else {
            standardHeapVars.push(v);
        }
    }

    standardHeapVars.forEach(v => {
        heapContainer.innerHTML += `<div class="memory-var" id="var-${v.address}" style="border-color: #d16969;">
            <span class="var-addr" style="color: #d16969;">[${v.address}]</span>
            <span class="var-type">${v.type}</span>
            <span class="var-name">${v.name}</span> = 
            <span class="var-val" style="color: ${v.value==='Uninitialized' ? '#ff6b6b' : '#9cdcfe'}">${v.value}</span>
        </div>`;
    });
    
    for (const [objAddr, props] of Object.entries(objectHeapVars)) {
        let boxHTML = `<div class="object-box" style="border-color:#d16969; background:rgba(209,105,105,0.05);"><div class="object-header" style="color:#d16969;">Heap Object: [${objAddr}]</div>`;
        props.forEach(v => {
            boxHTML += `<div class="memory-var" id="var-${v.address}">
                <span class="var-addr" style="color:#d16969;">[${v.address}]</span>
                <span class="var-type">${v.type}</span>
                <span class="var-name">${v.name.split('->')[1]}</span> = 
                <span class="var-val" style="color: ${v.value==='Uninitialized' ? '#ff6b6b' : '#9cdcfe'}">${v.value}</span>
            </div>`;
        });
        heapContainer.innerHTML += boxHTML + `</div>`;
    }

    // Bug 10 fix: Use the statically created console div (created once on init)
    const activeConsole = document.getElementById("console-container");
    if (activeConsole) {
        activeConsole.innerHTML = `<div style="color: #888; margin-bottom: 8px;">> Console Output</div>`;
        for (let i = 0; i < state.consoleOut.size(); i++) {
            const lines = state.consoleOut.get(i).split('\n');
            lines.forEach(l => {
                if(l.trim() !== "") activeConsole.innerHTML += `<div>${l}</div>`;
            });
        }
    }
    
    setTimeout(() => drawArrows(state), 50);
}

function drawArrows(state) {
    const svg = document.getElementById("arrow-canvas");
    svg.innerHTML = `
        <defs>
            <marker id="arrowhead" markerWidth="10" markerHeight="7" refX="9" refY="3.5" orient="auto">
                <polygon points="0 0, 10 3.5, 0 7" fill="#ffcc00" />
            </marker>
            <marker id="refhead" markerWidth="10" markerHeight="7" refX="9" refY="3.5" orient="auto">
                <polygon points="0 0, 10 3.5, 0 7" fill="#4CAF50" />
            </marker>
        </defs>`;

    let allVars = [];
    for (let i = 0; i < state.stack.size(); i++) {
        let frame = state.stack.get(i);
        for(let j=0; j < frame.variables.size(); j++) allVars.push(frame.variables.get(j));
    }
    for (let i = 0; i < state.heap.size(); i++) allVars.push(state.heap.get(i));
    
    allVars.forEach(v => {
        if ((v.isPointer || v.isReference) && v.pointsTo !== "") {
            const sourceEl = document.getElementById(`var-${v.address}`);
            const targetEl = document.getElementById(`var-${v.pointsTo}`);
            if (sourceEl && targetEl) {
                const panel = document.querySelector('.right-panel').getBoundingClientRect();
                const srcRect = sourceEl.getBoundingClientRect();
                const tgtRect = targetEl.getBoundingClientRect();
                const startX = srcRect.right - panel.left;
                const startY = srcRect.top + (srcRect.height / 2) - panel.top;
                const endX = tgtRect.left - panel.left;
                const endY = tgtRect.top + (tgtRect.height / 2) - panel.top;
                
                const color = v.isReference ? "#4CAF50" : "#ffcc00";
                const dash = v.isReference ? "2" : "5";
                const marker = v.isReference ? "url(#refhead)" : "url(#arrowhead)";
                
                svg.innerHTML += `<path d="M ${startX} ${startY} C ${startX + 50} ${startY}, ${endX - 50} ${endY}, ${endX} ${endY}" style="fill:none; stroke: ${color}; stroke-width: 2; stroke-dasharray: ${dash}; marker-end: ${marker};" />`;
            }
        }
    });
}
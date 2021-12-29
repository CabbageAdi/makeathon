import React from 'react';
import './App.css';

import { buildHex } from "./compile";
import { AVRRunner } from "./execute";
import { formatTime } from "./format-time";
import AceEditor from "react-ace";

let CODE = `
# define lf 0 //left forward
# define lb 1 //left backward
# define rf 2 //right forward
# define rb 3 //right backward

# define fs 4 //forward sensor
# define ls 5 //left sensor
# define rs 6 //right sensor

void setup() {
  Serial.begin(115200);
}

void loop() {
  
}
`.trim();

const outPins : number[] = [ 0, //stop or start
  1, //left
  2, //right
];
const inPins : number[] = [ 4, //forward sensor
    5, //left sensor
    6 //right sensor
];

// Set up toolbar
let runner: AVRRunner | null;

let runButton : Element;
let stopButton : Element;
let compilerOutputText : Element;

window.onload = async function (){
  runButton = document.querySelector("#run-button") as Element;
  runButton.addEventListener("click", compileAndRun);
  stopButton = document.querySelector("#stop-button") as Element;
  stopButton.addEventListener("click", stopCode);
  compilerOutputText = document.querySelector("#compiler-output-text") as Element;

  outPins.forEach((pin) =>{
    var element = document.createElement('div');
    element.hidden = true;
    element.id = pin.toString();
    document.body.appendChild(element);
  });
  inPins.forEach((pin) =>{
    var element = document.createElement('div');
    element.hidden = true;
    element.id = pin.toString();
    document.body.appendChild(element);
  });
}

function executeProgram(hex: string) {
  runner = new AVRRunner(hex);
  const statusLabel = document.querySelector("#status-label") as Element;
  const MHZ = 16000000;

  runner.portD.addListener(value => {
    outPins.forEach((pin) => {
      (document.getElementById(pin.toString()) as Element).textContent = runner?.portD.pinState(pin).toString() ?? null;
    })
  });
  runner.usart.onByteTransmit = (value: number) => {
    SerialLog(String.fromCharCode(value));
  };

  runner.execute(cpu => {
    const time = formatTime(cpu.cycles / MHZ);
    statusLabel.textContent = "Simulation time: " + time;
  });
}

async function compileAndRun() {
  compilerOutputText.textContent = "Compiling..."

  runButton.setAttribute("disabled", "1");
  try {
    const result = await buildHex(CODE);
    compilerOutputText.textContent = result.stderr || result.stdout;
    if (result.hex) {
      compilerOutputText.textContent += "\nProgram running.\n\nSerial Output:\n";
      stopButton.removeAttribute("disabled");
      executeProgram(result.hex);
    } else {
      runButton.removeAttribute("disabled");
    }
  } catch (err) {
    runButton.removeAttribute("disabled");
    alert("Failed: " + err);
  }
}

function SerialLog(text: any){
  compilerOutputText.textContent += text.toString();
}

function stopCode() {
  stopButton.setAttribute("disabled", "1");
  runButton.removeAttribute("disabled");
  if (runner) {
    runner.stop();
    runner = null;
  }
  compilerOutputText.textContent = null;
}

//add scripts
let script2 = document.createElement('script');
script2.src = document.documentURI + 'sim';
script2.async = true;
document.body.appendChild(script2);
let script = document.createElement('script');
script.src = document.documentURI + 'script.js';
script.async = true;
document.body.appendChild(script);

function App() {
  return (
      <div>
        <div id="spinner"/>
        <div id="status">Downloading...</div>
        <progress hidden id="progress"/>
        <canvas id="canvas"/>

        <div className="app-container">
          <p>
            Controls: Hold W for movement <br />
            Nothing else: Forward <br />
            A: Rotate left <br />
            D: Rotate right <br />
            A and D: Backwards
          </p>
          <div className={"code-editor"}>
            <div className="toolbar">
              <button id="run-button" >Run</button>
              <button id="stop-button" disabled>Stop</button>
              <div className="spacer"/>
              <div id="status-label"/>
            </div>
            <AceEditor value={CODE} onChange={code => CODE = code } width={"auto"} fontSize={"15px"}/>
          </div>
        </div>

        <div className="compiler-output">
          <div className={"toolbar"}>
            <div>Serial Monitor</div>
          </div>
          <p id="compiler-output-text"/>
        </div>

        <script>
          editorLoaded();
        </script>
      </div>
  );
}

export default App;
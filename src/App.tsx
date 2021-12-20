import React from 'react';
import './App.css';

import { buildHex } from "./compile";
import { AVRRunner } from "./execute";
import { formatTime } from "./format-time";
import AceEditor from "react-ace";

let CODE = `
# define forward 0
# define back 1
# define down 2
# define up 3
# define in 4

void setup() {
  Serial.begin(115200);
  pinMode(forward, OUTPUT);
  pinMode(back, OUTPUT);
  pinMode(down, OUTPUT);
  pinMode(up, OUTPUT);
  pinMode(in, INPUT);
}

void loop() {
  digitalWrite(back, LOW);
  digitalWrite(down, HIGH);
  delay(300);
  Serial.print(digitalRead(in));
  digitalWrite(down, LOW);
  digitalWrite(forward, HIGH);
  delay(300);
  Serial.print(digitalRead(in));
  digitalWrite(forward, LOW);
  digitalWrite(up, HIGH);
  delay(300);
  Serial.print(digitalRead(in));
  digitalWrite(up, LOW);
  digitalWrite(back, HIGH);
  delay(300);
  Serial.print(digitalRead(in));
}`.trim();

const outPins : number[] = [0, 1, 2, 3];
const inPins : number[] = [4];
let values: { [id: number] : boolean; } = {};

// Set up toolbar
let runner: AVRRunner | null;

let runButton : Element;
let stopButton : Element;
let compilerOutputText : Element;

window.onload = function (){
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
    compilerOutputText.textContent += String.fromCharCode(value);
  };

  runner.execute(cpu => {
    const time = formatTime(cpu.cycles / MHZ);
    statusLabel.textContent = "Simulation time: " + time;
    inPins.forEach((pin) => {
      const val = document.getElementById(pin.toString())?.textContent == '1' ? true : false;
      runner?.portD.setPin(pin, val);
      console.log(val);
    });
  });
}

async function compileAndRun() {
  compilerOutputText.textContent = "Compiling..."

  runButton.setAttribute("disabled", "1");
  try {
    const result = await buildHex(CODE);
    compilerOutputText.textContent = result.stderr || result.stdout;
    if (result.hex) {
      compilerOutputText.textContent += "\nProgram running...";
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
script2.src = document.documentURI + 'makeathonsim.js';
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
          <div className={"code-editor"}>
            <div className="toolbar">
              <button id="run-button" >Run</button>
              <button id="stop-button" disabled>Stop</button>
              <div className="spacer"/>
              <div id="status-label"/>
            </div>
            <AceEditor value={CODE} onChange={code => CODE = code } width={"auto"} fontSize={"15px"}/>
          </div>

          <div className="compiler-output">
            <p id="compiler-output-text"/>
          </div>
        </div>
        <script>
          editorLoaded();
        </script>
      </div>
  );
}

export default App;
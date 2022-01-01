import React from 'react';
import './App.css';

import { buildHex } from "./compile";
import { AVRRunner } from "./execute";
import { formatTime } from "./format-time";
import AceEditor from "react-ace";
import { ADCMuxInput, ADCMuxInputType, PinState } from 'avr8js';
//@ts-ignore
import ScrollToBottom from 'react-scroll-to-bottom';
import "ace-builds/src-noconflict/mode-java";

let CODE = `
# define stop 0 //LOW is stop, HIGH is move
# define left 1 //left 
# define right 2 //right

# define fs A0 //forward sensor
# define rs A1 //right sensor
# define ls A2 //left sensor

# define mapped 4 //set to high if first run is complete

void setup() {
  Serial.begin(115200);
  pinMode(stop, OUTPUT);
  pinMode(left, OUTPUT);
  pinMode(right, OUTPUT);
  pinMode(fs, INPUT);
  pinMode(rs, INPUT);
  pinMode(ls, INPUT);
  pinMode(mapped, INPUT);
  for (int i = 3; i < 11; i++)
    pinMode(i, OUTPUT);
  set_speed(255);
}

void loop() {
  
}

bool is_mapped(){
  return digitalRead(mapped) == HIGH ? true : false;
}

void brake(){
  digitalWrite(stop, LOW);
}

float forward_dist(){
  return analogRead(fs) / 8.0;
}

float left_dist(){
  return analogRead(ls) / 8.0;
}

float right_dist(){
  return analogRead(rs) / 8.0;
}

void forward(){
  digitalWrite(stop, HIGH);
  digitalWrite(left, LOW);
  digitalWrite(right, LOW);
}

void back(){
  digitalWrite(stop, HIGH);
  digitalWrite(left, HIGH);
  digitalWrite(right, HIGH);
}

void left_turn(){
  digitalWrite(stop, HIGH);
  digitalWrite(left, HIGH);
  digitalWrite(right, LOW);
}

void right_turn(){
  digitalWrite(stop, HIGH);
  digitalWrite(left, LOW);
  digitalWrite(right, HIGH);
}

void set_speed(byte speed){
  int binaryNum[8];
 
  int i = 0;
  while (speed > 0) {
    digitalWrite(i + 3, speed % 2 == 1 ? HIGH : LOW);
    speed = speed / 2;
    i++;
  }
}
`.trim();

const outPins : number[] = [ 0, //stop or start
    1, //left
    2, //right

    3,
    4,
    5,
    6,
    7,
    8,
    9,
    10
];
const inPins : number[] = [ 0, //forward sensor
  1, //right sensor
  2, //left sensor
];

const statePins : number[] = [
    4, //finished first run
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
    element.id = pin.toString() + 'out';
    document.body.appendChild(element);
  });
  inPins.forEach((pin) =>{
    var element = document.createElement('div');
    element.hidden = true;
    element.id = pin.toString();
    document.body.appendChild(element);
  });
  statePins.forEach((pin) =>{
    var element = document.createElement('div');
    element.hidden = true;
    element.id = pin.toString();
    document.body.appendChild(element);
  });
  //reset on stop
  var element = document.createElement('div');
  element.hidden = true;
  element.id = "12out";
  document.body.appendChild(element);
}

let speedData: boolean[] = [];

function executeProgram(hex: string) {
  runner = new AVRRunner(hex);
  const statusLabel = document.querySelector("#status-label") as Element;
  let startTime = new Date().getTime();
  let mapped = false;

  runner.portD.addListener(value => {
    outPins.forEach((pin) => {
      if (pin < 8) (document.getElementById(pin.toString() + 'out') as Element).textContent = runner?.portD.pinState(pin).toString() ?? null;
    });
  });
  runner.portB.addListener(value => {
    outPins.forEach((pin) => {
      if (pin > 7) (document.getElementById(pin.toString() + 'out') as Element).textContent = runner?.portB.pinState(pin - 8).toString() ?? null;
    });
  });
  runner.usart.onByteTransmit = (value: number) => {
    SerialLog(String.fromCharCode(value));
  };

  runner.execute(cpu => {
    const time = formatTime((new Date().getTime() - startTime) / 1000);
    statusLabel.textContent = "Simulation time: " + time;
    inPins.forEach((pin) => {
      const val = parseFloat(document.getElementById(pin.toString())?.textContent as string);
      (runner as AVRRunner).adc.channelValues[pin] = val;
    });
    statePins.forEach(pin => {
      const val = parseInt(document.getElementById(pin.toString())?.textContent as string) === 1 ? true : false;
      (runner as AVRRunner).portD.setPin(pin, val);
      if (pin === 4 && val && !mapped){
        mapped = true;
        startTime = new Date().getTime();
      }
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

let lastTime = 0;
function SerialLog(text: any){
  compilerOutputText.textContent += text.toString();
  const diff = new Date().getTime() - lastTime;
  if (diff > 3){
    console.log(diff);
  }
  lastTime = new Date().getTime();
}

function stopCode() {
  stopButton.setAttribute("disabled", "1");
  runButton.removeAttribute("disabled");
  if (runner) {
    runner.stop();
    runner = null;
  }
  compilerOutputText.textContent = null;

  (document.getElementById("12out") as Element).textContent = "1";
  setTimeout(function (){
    (document.getElementById("12out") as Element).textContent = "0";
  }, 200);

  outPins.forEach((pin) =>{
    (document.getElementById(pin.toString() + 'out') as Element).textContent = null;
  });
  inPins.forEach((pin) =>{
    (document.getElementById(pin.toString()) as Element).textContent = null;
  });
  statePins.forEach((pin) =>{
    (document.getElementById(pin.toString()) as Element).textContent = null;
  });
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
          <div className={"code-editor"}>
            <div className="toolbar">
              <button id="run-button" >Run</button>
              <button id="stop-button" disabled>Stop</button>
              <div className="spacer"/>
              <div id="status-label"/>
            </div>
            <AceEditor value={CODE} onChange={code => CODE = code } width={"auto"} fontSize={"15px"} mode={'java'}/>
          </div>
        </div>

        <div className="compiler-output">
          <div className={"toolbar"}>
            <div>Serial Monitor</div>
          </div>
          <ScrollToBottom className="scroll">
            <p id="compiler-output-text"/>
          </ScrollToBottom>
        </div>

        <script>
          editorLoaded();
        </script>
      </div>
  );
}

export default App;
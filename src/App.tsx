import React from 'react';
import './App.css';

import {buildHex} from "./compile";
import {AVRRunner} from "./execute";
import {formatTime} from "./format-time";
import AceEditor from "react-ace";
//@ts-ignore
import ScrollToBottom from 'react-scroll-to-bottom';
import "ace-builds/src-noconflict/mode-java";

let CODE = `
# define stop_pin 0 //LOW is stop, HIGH is move
# define left_pin 1 //left 
# define right_pin 2 //right

# define fs_pin A0 //forward sensor
# define rs_pin A1 //right sensor
# define ls_pin A2 //left sensor

# define mapped_pin 4 //set to high if first run is complete

void setup() {
  Serial.begin(115200);
  pinMode(stop_pin, OUTPUT);
  pinMode(left_pin, OUTPUT);
  pinMode(right_pin, OUTPUT);
  pinMode(fs_pin, INPUT);
  pinMode(rs_pin, INPUT);
  pinMode(ls_pin, INPUT);
  pinMode(mapped_pin, INPUT);
  for (int i = 3; i < 11; i++)
    pinMode(i, OUTPUT);
  set_speed(255);
}

void loop() {
  
}

bool is_mapped(){
  return digitalRead(mapped_pin) == HIGH ? true : false;
}

void brake(){
  digitalWrite(stop_pin, LOW);
}

float forward_dist(){
  return analogRead(fs_pin) / 8.0;
}

float left_dist(){
  return analogRead(ls_pin) / 8.0;
}

float right_dist(){
  return analogRead(rs_pin) / 8.0;
}

void forward(){
  digitalWrite(stop_pin, HIGH);
  digitalWrite(left_pin, LOW);
  digitalWrite(right_pin, LOW);
}

void back(){
  digitalWrite(stop_pin, HIGH);
  digitalWrite(left_pin, HIGH);
  digitalWrite(right_pin, HIGH);
}

void left_turn(){
  digitalWrite(stop_pin, HIGH);
  digitalWrite(left_pin, HIGH);
  digitalWrite(right_pin, LOW);
}

void right_turn(){
  digitalWrite(stop_pin, HIGH);
  digitalWrite(left_pin, LOW);
  digitalWrite(right_pin, HIGH);
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

const outPins: number[] = [0, //stop or start
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
const inPins: number[] = [0, //forward sensor
    1, //right sensor
    2, //left sensor
];

const statePins: number[] = [
    11, //finished first run
    13 //finished entire thing
];

// Set up toolbar
let runner: AVRRunner | null;

let runButton: Element;
let stopButton: Element;
let compilerOutputText: Element;
let submitButton: HTMLButtonElement;

let finishTime: number;

window.onload = async function () {
    runButton = document.querySelector("#run-button") as Element;
    runButton.addEventListener("click", compileAndRun);
    stopButton = document.querySelector("#stop-button") as Element;
    stopButton.addEventListener("click", stopCode);
    compilerOutputText = document.querySelector("#compiler-output-text") as Element;
    submitButton = document.getElementById("submit") as HTMLButtonElement;
    submitButton.addEventListener("click", submit);

    outPins.forEach((pin) => {
        var element = document.createElement('div');
        element.hidden = true;
        element.id = pin.toString() + 'out';
        document.body.appendChild(element);
    });
    inPins.forEach((pin) => {
        var element = document.createElement('div');
        element.hidden = true;
        element.id = pin.toString();
        document.body.appendChild(element);
    });
    statePins.forEach((pin) => {
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
        const time = (new Date().getTime() - startTime) / 1000
        const formattedTime = formatTime(time);
        statusLabel.textContent = "Simulation time: " + formattedTime;
        inPins.forEach((pin) => {
            const val = parseFloat(document.getElementById(pin.toString())?.textContent as string);
            (runner as AVRRunner).adc.channelValues[pin] = val;
        });
        statePins.forEach(pin => {
            const val = parseInt(document.getElementById(pin.toString())?.textContent as string) === 1 ? true : false;
            (runner as AVRRunner).portB.setPin(pin - 8, val);
            if (pin === 11 && val && !mapped) {
                mapped = true;
                startTime = new Date().getTime();
            }
            if (pin === 13 && val){
                finishTime = time;
                submitButton.hidden = false;
                stopCode();
            }
        });
    });
}

async function compileAndRun() {
    compilerOutputText.textContent = "Compiling..."
    submitButton.hidden = true;
    finishTime = 0;
    statePins.forEach(pin => {
        (document.getElementById(pin.toString()) as Element).textContent = "0";
    };

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

function SerialLog(text: any) {
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

    (document.getElementById("12out") as Element).textContent = "1";
    setTimeout(function () {
        (document.getElementById("12out") as Element).textContent = "0";
    }, 200);

    outPins.forEach((pin) => {
        (document.getElementById(pin.toString() + 'out') as Element).textContent = null;
    });
    inPins.forEach((pin) => {
        (document.getElementById(pin.toString()) as Element).textContent = null;
    });
    statePins.forEach((pin) => {
        (document.getElementById(pin.toString()) as Element).textContent = null;
    });
}

async function submit(){
    await fetch('url', {
        method: "POST",
        body: JSON.stringify({
            code: CODE,
            time: finishTime
        }),
        headers: {
            "content-type": "application/json"
        }
    });
    submitButton.hidden = true;
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
            <button className={"button"} id={"submit"} hidden>Submit</button>
            <div id="status">Downloading...</div>
            <canvas id="canvas"/>
            <br/>
            <div className="app-container">
                <div className="code-toolbar">
                    <button id="run-button" className={"button"}>Run</button>
                    <button id="stop-button" className={"button"} disabled>Stop</button>
                    <div id="status-label"/>
                </div>
                <AceEditor value={CODE} onChange={code => CODE = code} width={"auto"} height={"1250px"}
                           fontSize={"15px"} mode={'java'}/>
            </div>
            <div className="compiler-output">
                <div className={"serial-toolbar"}>
                    <div>Serial Monitor</div>
                </div>
                <ScrollToBottom className={"scroll"}>
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
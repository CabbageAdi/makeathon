import React from "react";

import { buildHex } from "./compile";
import { AVRRunner } from "./execute";
import { formatTime } from "./format-time";
import AceEditor from "react-ace";
//@ts-ignore
import ScrollToBottom from "react-scroll-to-bottom";
import "ace-builds/src-noconflict/mode-java";
import "ace-builds/src-noconflict/theme-monokai";
import "ace-builds/src-noconflict/ext-language_tools";
import "ace-builds/src-noconflict/ext-beautify";

let CODE = `
# define stop_pin 0 //brake or move
# define left_pin 1 //move left
# define right_pin 2 //move right

# define fs_pin A0 //forward sensor
# define rs_pin A1 //right sensor
# define ls_pin A2 //left sensor
# define rotation_pin A3 //rotation

# define mapped_pin 11 //mapped state

void setup() {
  Serial.begin(115200);
  pinMode(stop_pin, OUTPUT);
  pinMode(left_pin, OUTPUT);
  pinMode(right_pin, OUTPUT);
  pinMode(fs_pin, INPUT);
  pinMode(rs_pin, INPUT);
  pinMode(ls_pin, INPUT);
  pinMode(mapped_pin, INPUT);
  pinMode(rotation_pin, INPUT);
  for (int i = 3; i < 11; i++)
    pinMode(i, OUTPUT);
  set_speed(255);
  delay(1000);
}

void loop() {
  
}

void set_speed(byte speed){
  int i = 3;
  while (speed > 0) {
    digitalWrite(i, speed % 2 == 1 ? HIGH : LOW);
    speed = speed / 2;
    i++;
  }
  for (;i < 11; i++) digitalWrite(i, LOW);
}
`.trim();

const outPins: number[] = [
  0, //stop or start
  1, //left
  2, //right

  3,
  4,
  5,
  6,
  7,
  8,
  9,
  10, //speed pins
];
const inPins: number[] = [
  0, //forward sensor
  1, //right sensor
  2, //left sensor
  3, //rotation
];

const statePins: number[] = [
  11, //finished first run
  13, //finished entire thing
];

// Set up toolbar
let runner: AVRRunner | null;

let runButton: Element;
let stopButton: Element;
let compilerOutputText: Element;

let finishTime: number;
let serialText: string;

//add scripts
let script = document.createElement("script");
script.src = document.documentURI + "script.js";
script.async = true;
document.body.appendChild(script);
let script2 = document.createElement("script");
script2.src = document.documentURI + "sim";
script2.async = true;
document.body.appendChild(script2);

function App() {
  window.onload = async function () {
    runButton = document.querySelector("#run-button") as Element;
    stopButton = document.querySelector("#stop-button") as Element;
    stopButton.addEventListener("click", stopCode);
    compilerOutputText = document.getElementById("compiler-output-text") as Element;

    outPins.forEach((pin) => {
      var element = document.createElement("div");
      element.hidden = true;
      element.id = pin.toString() + "out";
      document.body.appendChild(element);
    });
    inPins.forEach((pin) => {
      var element = document.createElement("div");
      element.hidden = true;
      element.id = pin.toString();
      document.body.appendChild(element);
    });
    statePins.forEach((pin) => {
      var element = document.createElement("div");
      element.hidden = true;
      element.id = pin.toString();
      document.body.appendChild(element);
    });
    //reset on stop
    var element = document.createElement("div");
    element.hidden = true;
    element.id = "12out";
    document.body.appendChild(element);
  };

  function executeProgram(hex: string) {
    runner = new AVRRunner(hex);
    const statusLabel = document.querySelector("#status-label") as Element;
    let startTime = new Date().getTime();
    let mapped = false;

    runner.portD.addListener((value) => {
      outPins.forEach((pin) => {
        if (pin < 8)
          (
              document.getElementById(pin.toString() + "out") as Element
          ).textContent = runner?.portD.pinState(pin).toString() ?? null;
      });
    });
    runner.portB.addListener((value) => {
      outPins.forEach((pin) => {
        if (pin > 7)
          (
              document.getElementById(pin.toString() + "out") as Element
          ).textContent = runner?.portB.pinState(pin - 8).toString() ?? null;
      });
    });
    runner.usart.onByteTransmit = (value: number) => {
      SerialLog(String.fromCharCode(value));
    };

    runner.execute((cpu) => {
      const time = (new Date().getTime() - startTime) / 1000;
      finishTime = time;
      const formattedTime = formatTime(time);
      statusLabel.textContent = "Simulation time: " + formattedTime;
      inPins.forEach((pin) => {
        const val = parseFloat(
            document.getElementById(pin.toString())?.textContent as string
        );
        (runner as AVRRunner).adc.channelValues[pin] = val;
      });
      statePins.forEach((pin) => {
        const val =
            parseInt(
                document.getElementById(pin.toString())?.textContent as string
            ) === 1
                ? true
                : false;
        (runner as AVRRunner).portB.setPin(pin - 8, val);
        if (pin === 11 && val && !mapped) {
          mapped = true;
          startTime = new Date().getTime();
        }
        if (pin === 13 && val) {
          finishTime = time;
          submit();
          stopCode();
        }
      });
    });
  }

  async function compileAndRun() {
    serialText = "";
    SerialLog("Compiling...");
    finishTime = 0;
    statePins.forEach((pin) => {
      (document.getElementById(pin.toString()) as Element).textContent = "0";
    });

    runButton.setAttribute("disabled", "1");
    try {
      const result = await buildHex(CODE);
      SerialLog(result.stderr || result.stdout);
      if (result.hex) {
        SerialLog("Program running.\n\nSerial Output:\n");
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
    serialText += text.toString();
    if (compilerOutputText !== null) compilerOutputText.textContent = serialText;
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
      (document.getElementById(pin.toString() + "out") as Element).textContent =
          null;
    });
    inPins.forEach((pin) => {
      (document.getElementById(pin.toString()) as Element).textContent = null;
    });
    statePins.forEach((pin) => {
      (document.getElementById(pin.toString()) as Element).textContent = null;
    });
  }

  async function submit() {
    const statusLabel = document.querySelector("#status-label") as Element;
    statusLabel.textContent = "Submitted: " + formatTime(finishTime);;
  }

  const [serial, setSerial] = React.useState(true);
  async function serialSet(val: boolean){
    if (val === true){
      setSerial(true);
      while (document.getElementById("compiler-output-text") === null) {await new Promise(resolve => setTimeout(resolve, 0));};
      compilerOutputText = document.getElementById("compiler-output-text") as Element;
      compilerOutputText.textContent = serialText;
    }
    else {
      setSerial(false);
    }
  }
  return (
    <div>
      <div id="status">Downloading...</div>
      <canvas id="canvas" />
      <br />
      <div id="status-label" className={"status-label"} />
      <div className="app-container">
        <div className="code-toolbar">
          <button
            onClick={async () => await serialSet(true)}
            className={"button toggle-btn"}
          >
            <b>Serial Monitor</b>
          </button>
          <button
            onClick={async () => await serialSet(false)}
            className={"button toggle-btn"}
          >
            <b>Editor</b>
          </button>
          <button id="run-button" className={"button success"} onClick={async () => { await serialSet(true); compileAndRun(); }}>
            <b>Run</b>
          </button>
          <button id="stop-button" className={"button danger"} disabled>
            <b>Stop</b>
          </button>
        </div>
        {serial ? (
          <div className="compiler-output">
            <div className={"serial-toolbar"}>
              <div>Serial Monitor</div>
            </div>
            <ScrollToBottom className={"scroll"}>
              <p id="compiler-output-text"></p>
            </ScrollToBottom>
          </div>
        ) : (
          <AceEditor
            value={CODE}
            onChange={(code) => (CODE = code)}
            width={"auto"}
            mode="java"
            theme="monokai"
            fontSize={14}
            className="editor"
            showPrintMargin={true}
            showGutter={true}
            highlightActiveLine={true}
            setOptions={{
              enableBasicAutocompletion: false,
              enableLiveAutocompletion: false,
              enableSnippets: false,
              showLineNumbers: true,
              tabSize: 2,
            }}
          />
        )}
      </div>
      <script>editorLoaded();</script>
    </div>
  );
}

export default App;

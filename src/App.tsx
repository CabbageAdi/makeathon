import React from 'react';
import './App.css';

import "@wokwi/elements";
import { buildHex } from "./compile";
import { AVRRunner } from "./execute";
import { LEDElement } from "@wokwi/elements";
import "./index.css";
import {formatTime} from "./format-time";

const BLINK_CODE = `
// LEDs connected to pins 8..13

byte leds[] = {13, 12, 11, 10, 9, 8};
void setup() {
  for (byte i = 0; i < sizeof(leds); i++) {
    pinMode(leds[i], OUTPUT);
  }
}

int i = 0;
void loop() {
  digitalWrite(leds[i], HIGH);
  delay(250);
  digitalWrite(leds[i], LOW);
  i = (i + 1) % sizeof(leds);
}`.trim();

// Set up LEDs
const LEDs = document.querySelectorAll<LEDElement & HTMLElement>("wokwi-led");

// Set up toolbar
let runner: AVRRunner | null;

const runButton = document.querySelector("#run-button") as Element;
runButton?.addEventListener("click", compileAndRun);
const stopButton = document.querySelector("#stop-button") as Element;
stopButton?.addEventListener("click", stopCode);
const statusLabel = document.querySelector("#status-label") as Element;
const compilerOutputText = document.querySelector("#compiler-output-text") as Element;

function updateLEDs(value: number, startPin: number) {
  for (const led of Array.from(LEDs)) {
    const pin = parseInt(led.getAttribute("pin") as string, 10);
    if (pin >= startPin && pin <= startPin + 8) {
      led.value = !!(value & (1 << (pin - startPin)));
    }
  }
}

function executeProgram(hex: string) {
  runner = new AVRRunner(hex);
  const MHZ = 16000000;

  console.log('executing');

  // Hook to PORTB register
  runner.portD.addListener(value => {
    console.log(`updating led with value ${value}`);
    updateLEDs(value, 0);
  });
  runner.portB.addListener(value => {
    console.log(`updating led with value ${value}`);
    updateLEDs(value, 8);
  });
  runner.execute(cpu => {
    const time = formatTime(cpu.cycles / MHZ);
    statusLabel.textContent = "Simulation time: " + time;
  });
}

async function compileAndRun() {
  console.log('running code');
  for (const led of Array.from(LEDs)) {
    led.value = false;
  }

  runButton?.setAttribute("disabled", "1");
  try {

    const result = await buildHex(BLINK_CODE);
    //compilerOutputText.textContent = result.stderr || result.stdout;
    if (result.hex) {
      //compilerOutputText.textContent += "\nProgram running...";
      //stopButton.removeAttribute("disabled");
      executeProgram(result.hex);
    } else {
      //runButton.removeAttribute("disabled");
    }
  } catch (err) {
    //runButton.removeAttribute("disabled");
    alert("Failed: " + err);
  } finally {

  }
}

function stopCode() {
  stopButton.setAttribute("disabled", "1");
  runButton.removeAttribute("disabled");
  if (runner) {
    runner.stop();
    runner = null;
  }
}

//add scripts
let script = document.createElement('script');
script.src = document.documentURI + 'script.js';
script.async = true;
document.body.appendChild(script);
let script2 = document.createElement('script');
script2.src = document.documentURI + 'makeathonsim.js';
script2.async = true;
document.body.appendChild(script2);

function App() {
  return (
      <div>
        <div className="app-container">
          <div className={"leds"}>
            <wokwi-led />
            <wokwi-led color="red" label="13"></wokwi-led>
            <wokwi-led color="green" label="12"></wokwi-led>
            <wokwi-led color="blue" label="11"></wokwi-led>
            <wokwi-led color="yellow" label="10"></wokwi-led>
            <wokwi-led color="orange" label="9"></wokwi-led>
            <wokwi-led color="white" label="8"></wokwi-led>
          </div>
          <div className="toolbar">
            <button id="run-button" onClick={compileAndRun}>Run</button>
            <button id="stop-button" disabled>Stop</button>
            <div className="spacer"></div>
            <div id="status-label"></div>
          </div>
          <div className="code-editor"></div>
          <div className="compiler-output">
            <pre id="compiler-output-text"></pre>
          </div>

          <div id="spinner"></div>
          <div id="status">Downloading...</div>
          <progress hidden id="progress"></progress>
          <canvas id="canvas"></canvas>
        </div>
        <script>
          editorLoaded();
        </script>
      </div>
  );
}

export default App;
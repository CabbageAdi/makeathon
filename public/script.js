var statusElement = document.querySelector('#status');
var progressElement = document.querySelector('#progress');
var spinnerElement = document.querySelector('#spinner');
var Module = {
    preRun: [],
    postRun: [],
    print: (function() {
        var element = document.querySelector('#output');

        if (element) element.value = '';    // Clear browser cache

        return function(text) {
            if (arguments.length > 1) text = Array.prototype.slice.call(arguments).join(' ');
            console.log(text);
            if (element) {
                element.value += text + "\n";
                element.scrollTop = element.scrollHeight; // focus on bottom
            }
        };
    })(),
    printErr: function(text) {
        if (arguments.length > 1) text = Array.prototype.slice.call(arguments).join(' ');
        console.error(text);
    },
    canvas: (function() {
        var canvas = document.querySelector('#canvas');
        canvas.addEventListener("webglcontextlost", function(e) { alert('WebGL context lost. You will need to reload the page.'); e.preventDefault(); }, false);
        return canvas;
    })(),
    setStatus: function(text) {
        if (!Module.setStatus.last) Module.setStatus.last = { time: Date.now(), text: '' };
        if (text === Module.setStatus.last.text) return;

        var m = text.match(/([^(]+)\((\d+(\.\d+)?)\/(\d+)\)/);
        var now = Date.now();

        if (m && now - Module.setStatus.last.time < 30) return; // If this is a progress update, skip it if too soon

        Module.setStatus.last.time = now;
        Module.setStatus.last.text = text;

        if (m) {
            text = m[1];
            progressElement.value = parseInt(m[2])*100;
            progressElement.max = parseInt(m[4])*100;
            progressElement.hidden = true;
            spinnerElement.hidden = false;
        } else {
            progressElement.value = null;
            progressElement.max = null;
            progressElement.hidden = true;
            if (!text) spinnerElement.style.display = 'none';
        }

        statusElement.innerHTML = text;
    },
    totalDependencies: 0,
    monitorRunDependencies: function(left) {
        this.totalDependencies = Math.max(this.totalDependencies, left);
        Module.setStatus(left ? 'Preparing... (' + (this.totalDependencies-left) + '/' + this.totalDependencies + ')' : 'All downloads complete.');
    },
    //noInitialRun: true
};

Module.setStatus('Downloading...');

function pinVal(pin){
    return parseInt(document.getElementById(pin.toString() + 'out').textContent);
}

function setPin(pin, val){
    document.getElementById(pin.toString()).textContent = val.toString();
}
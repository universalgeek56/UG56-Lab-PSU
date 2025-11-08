#include <ESPAsyncWebServer.h>
#include "WebInterface.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include "Globals.h"
#include "Config.h"
#include <map>
#include <functional>

const char style_css[] PROGMEM = R"rawliteral(
@import url("https://fonts.googleapis.com/css2?family=Roboto+Mono&display=swap");
:root {
  --f: "Roboto Mono", monospace;
  --h: 180; /* Dynamic hue from themeColorHSV.hue via WebSocket */
  --ah: 60;
  --eh: 356;
  --c1: hsl(var(--h), 50%, 50%);
  --c2: hsl(var(--h), 50%, 70%);
  --c3: hsl(var(--h), 50%, 35%);
  --c4: hsl(var(--h), 30%, 80%);
  --s1: 0 0 10px hsl(var(--h), 50%, 50%);
  --b1: hsl(var(--h), 50%, 80%);
  --b2: hsl(var(--h), 50%, 45%);
  --b3: hsl(var(--h), 50%, 60%);
  --a1: hsl(var(--ah), 80%, 50%);
  --s3: 0 0 15px hsl(var(--ah), 80%, 50%);
  --e1: hsl(var(--eh), 80%, 36%);
  --s2: 0 0 15px hsl(var(--eh), 80%, 36%);
  --n1: hsl(0, 0%, 90%);
  --n2: hsl(0, 0%, 50%);
  --n3: hsl(0, 0%, 15%);
  --b4: hsl(0, 0%, 7%);
  --b5: hsl(0, 0%, 12%);
  --b6: hsl(0, 0%, 10%);
  --b7: hsl(0, 0%, 80%);
  --b8: hsl(0, 0%, 85%);
  --g1: 0.5em;
  --g2: 0.5em;
}
@keyframes text-blink { 0%, 50%, 100% { color: var(--color); text-shadow: 0 0 15px var(--color); filter: brightness(1.5); } 25%, 75% { color: var(--color); text-shadow: 0 0 5px var(--color); filter: brightness(0.8); } }
@keyframes shadow-blink { 0%, 50%, 100% { box-shadow: 0 0 15px var(--color); } 25%, 75% { box-shadow: 0 0 5px var(--color); } }
@keyframes drawFade { 0% { stroke-dashoffset: 100; opacity: 0; } 20% { opacity: 1; } 70% { stroke-dashoffset: 0; opacity: 1; } 100% { opacity: 0; } }
body { font-family: var(--f); background: var(--b4); color: var(--c1); margin: 0; padding: 1em; box-sizing: border-box; min-height: 100vh; }
h1 { font-weight: 700; font-size: 1.1em; margin: 0.2em 0; color: var(--c2); user-select: none; }
h2 { font-size: 1.1em; margin: 0.2em 0; color: var(--c1); user-select: none; }
.section { background-color: var(--b5); border-radius: 0.375em; padding: 1em 1.5em; box-shadow: var(--s1); flex: 1 1 0; min-width: 12.5em; color: var(--c4); margin: 0.5em 0; }
.section hr { border: none; border-bottom: 1px solid hsl(0, 0%, 20%); margin: 1em 0; }
input[type="text"], input[type="password"], input[type="number"] { text-align: center; font-size: 1.4em; height: 2.4em; line-height: 2.4em; border-radius: 0.375em; background-color: var(--b6); color: var(--c4); border: none; box-shadow: var(--s1); }
input[type="number"]::-webkit-inner-spin-button, input[type="number"]::-webkit-outer-spin-button { -webkit-appearance: none; margin: 0; }
input[type="number"] { -moz-appearance: textfield; }
input[type="number"]:focus, input[type="text"]:focus, input[type="password"]:focus { outline: 0; background-color: var(--b1); color: var(--b4); }
button.nav-btn { background-color: var(--b6); border: none; color: var(--c1); padding: 0; border-radius: 0.375em; cursor: pointer; transition: background-color 0.3s, color: 0.3s; flex: 1; display: flex; justify-content: center; align-items: center; gap: var(--g2); box-shadow: var(--s1); user-select: none; }
button.nav-btn:hover { background-color: var(--b1); color: var(--b4); }
button.nav-btn.active, button.nav-btn.active-page { background-color: var(--b3); color: var(--b4); box-shadow: 0 0 15px var(--c1); }
.field-group { display: grid; grid-template-columns: 10em 1fr 8em; gap: 0.5em 0.8em; align-items: center; margin: 0.6em 0; }
.field-group.password-group { grid-template-columns: 7em 1fr 14em; gap: 0.8em 0.5em; }
.field { display: contents; }
.field label { text-align: right; color: var(--c2); font-size: 1.1em; user-select: none; }
.field .global { text-align: left; font-size: 1em; color: var(--c4); font-variant-numeric: tabular-nums; }
.field input[type="text"], .field input[type="number"] { width: 4em; height: 1.8em; font-size: 1em; text-align: center; }
.field input.input-network { width: 12em; height: 2em; font-size: 1.1em; padding: 0 0.2em; }
.field input[type="checkbox"] { width: 1.2em; height: 1.2em; accent-color: var(--c1); margin: 0 auto; }
#onlineMode { justify-content: flex-start; }
.password-field { display: flex; justify-content: flex-end; align-items: center; }
.password-field input { flex: 1 1 auto; width: 100%; }
.hidden { display: none; }
#statusList { max-height: 150px; overflow-y: auto; font-weight: bold; font-size: 1em; }
#statusList.status-error { color: var(--e1); }
#statusList.status-alert { color: var(--e1); }
#statusList.status-normal { color: var(--c1); }
#statusSection { display: none; }
#statusSection.visible { display: block; }
.status-error { --color: var(--e1); --shadow: var(--s2); --hue: var(--eh); animation: text-blink 1.2s infinite; }
.status-alert { --color: var(--e1); --shadow: var(--s2); --hue: var(--eh); animation: text-blink 1.2s infinite; }
.status-normal { --color: var(--c1); --shadow: var(--s1); }
#logo { width: 1.6em; height: 1.6em; pointer-events: auto; cursor: pointer; color: hsl(104, 95%, 70%); vertical-align: bottom; }
.line, .circle { stroke: currentColor; stroke-width: 2; fill: none; stroke-dasharray: 100; stroke-dashoffset: 100; opacity: 0; animation-timing-function: ease-out; animation-fill-mode: forwards; }
.animate .circle { animation: drawFade 4.8s 0s backwards; }
.animate .line { animation: drawFade 4s 0.4s backwards; }
::-webkit-scrollbar { width: 0.4em; }
::-webkit-scrollbar-track { background: var(--b4); }
::-webkit-scrollbar-thumb { background: var(--b7); border-radius: 0.5em; }
::-webkit-scrollbar-thumb:hover { background: var(--b8); }
)rawliteral";

// Wi-Fi setup page HTML
#pragma once
const char wifi_setup_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Lab PSU Wi-Fi Setup</title>
<style>
:root{--c1:hsl(180,50%,50%);--b4:hsl(0,0%,7%);--s1:0 0 10px hsl(180,50%,50%);}
body{font-family:monospace;background:var(--b4);color:var(--c1);margin:0;padding:1em;text-align:center;}
h1{font-size:1.5em;margin-bottom:0.5em;text-shadow:var(--s1);}
.section{background:hsl(0,0%,12%);padding:1em;border-radius:0.375em;margin:0.5em auto;max-width:25em;box-shadow:var(--s1);}
.row{display:flex;align-items:center;margin:0.5em 0;}
label{flex:1;text-align:left;font-size:1.5em;}
input{flex:2;padding:0.5em;border:none;border-radius:0.375em;background:hsl(0,0%,10%);color:var(--c1);font-family:monospace;}
button{padding:0.5em 1em;background:hsl(180,50%,45%);color:var(--b4);border:none;border-radius:0.375em;cursor:pointer;box-shadow:var(--s1);margin:1em auto;display:block;}
button:hover{background:hsl(180,50%,80%);}
.spinner{display:none;width:2em;height:2em;border:3px solid var(--c1);border-top-color:transparent;border-radius:50%;animation:spin 1s linear infinite;margin:1em auto;}
@keyframes spin{to{transform:rotate(360deg);}}
#countdown{margin-top:1em;font-size:1em;}
</style>
</head>
<body>
<h1>Lab PSU Wi-Fi Setup</h1>
<div class="section">
<div class="row">
<label>SSID:</label>
<input type="text" id="ssid" placeholder="Wi-Fi SSID">
</div>
<div class="row">
<label>Password:</label>
<input type="text" id="pass" placeholder="Wi-Fi Password">
</div>
</div>
<button onclick="save()">Apply</button>
<div id="countdown"></div>
<div class="spinner" id="spinner"></div>
<script>
function save(){
const ssid=document.getElementById('ssid').value,pass=document.getElementById('pass').value;
if(!ssid){alert('SSID cannot be empty');return}
fetch('/save-wifi',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({WiFiSSID:ssid,WiFiPass:pass})})
.then(res=>{if(!res.ok)throw new Error('HTTP error '+res.status);return res.json()})
.then(data=>{
if(data.status==='ok'){
const countdown=document.getElementById('countdown'),spinner=document.getElementById('spinner');
spinner.style.display='block';
let time=5;
countdown.innerText=`Switching in ${time}...`;
const interval=setInterval(()=>{
time--;
countdown.innerText=`Switching in ${time}...`;
if(time<=0){
clearInterval(interval);
spinner.style.display='none';
countdown.innerHTML = `
  Connect to <b>${data.ssid}</b> with password <b>${data.pass}</b>.<br>
  Find device IP via local display<br>
  Long press encoder to enter Config<br>
  Turn encoder to page WiFi: ON/OFF<br>
  Enter IP from page in your browser to open web interface
`;
}
},1000)
}else{alert('Error: '+data.error)}
})
.catch(err=>alert('Request failed: '+err))
}
</script>
</body>
</html>
)rawliteral";

const char index_html[] PROGMEM = R"rawliteral(
<!doctype html><html lang="ru"><head><meta charset="UTF-8"><title>Lab PSU</title>
<link href="/style.css" rel="stylesheet"><meta content="width=device-width,initial-scale=1,user-scalable=no" name="viewport">
<style>body{max-width:40em;min-width:40em}header{display:flex;justify-content:space-between;align-items:center;margin-bottom:0.3em;gap:0.6em}
header>div{display:flex;gap:1em;align-items:center}
#ws-status{font-weight:700;user-select:none;transition:color 0.3s;color:var(--n2)}
#ws-status.active{color: #00b300}
#wifi{font-size:0.9em;color:var(--c4);display:flex;align-items:center;gap:0.3em;user-select:none}
#error-status{font-weight:700;font-size:0.9em;user-select:none;color:var(--n2);transition:all 0.3s}
#error-status.status-normal{display:none!important}
#error-status.status-error{color:var(--e1);--h:var(--eh);animation:text-blink 1.2s infinite}
#error-status.status-alert{color:var(--a1);--h:var(--ah);animation:text-blink 1.2s infinite}
main{display:flex;flex-direction:column;gap:var(--g1);max-width:43.75em;margin:0 auto}
main>*{margin:0}form,section{background-color:var(--b5);border-radius:0.375em;padding:1em 1.5em}
form,section{box-shadow:var(--s1);flex:1 1 0;min-width:12.5em;color:var(--c4)}
#readings p{font-size:1.8em;display:flex;justify-content:space-between;align-items:baseline;margin:0.15em 0;color:var(--c2)}
#readings p .label{font-size:1.2em;flex-shrink:0;user-select:none}
#readings p .number{font-size:1.2em;font-family:var(--f);font-variant-numeric:tabular-nums;width:6ch;text-align:right}
#readings p .unit{font-size:1em;margin-left:0.1em;white-space:nowrap}
#temperature{background-color:var(--b5);border-radius:0.375em;padding:0.5em 1.5em;box-shadow:var(--s1)}
#temperature{color:var(--c2);font-size:1.1em;max-width:43.75em;margin:0 auto;display:flex;justify-content:center}
#temperature{width:100%;box-sizing:border-box}
#temperature span{padding:0.1em 0.3em;border-radius:0.375em;color:var(--c4);user-select:none}
#temperature span{min-width:7ch;text-align:right;transition:background-color 0.5s;white-space:nowrap}
#temperature span{font-variant-numeric:tabular-nums}
#temperature span.status-error{--color:var(--e1);--shadow:var(--s2);--h:var(--eh);color:var(--e1)}
#temperature span.status-error{animation:text-blink 1.2s infinite}
#temperature span.status-alert{--color:var(--e1);--shadow:var(--s2);--h:var(--eh);color:var(--e1)}
#temperature span.status-alert{animation:text-blink 1.2s infinite}
#temperature span.status-normal{--color:var(--c1);--shadow:var(--s1);color:var(--c1)}
.param-row{display:flex;align-items:center;margin:0.5em 0;gap:0.8em}
.param-row label{min-width:6.4em;text-align:left;user-select:none}
.controls{display:flex;align-items:center;gap:0.5em;position:relative}
.controls input[type=number]{width:5.5em;height:2.2em;text-align:center;font-size:1.4em}
.controls input[type=number]{border-radius:0.375em;background-color:var(--b6);color:var(--c4);border:none}
.controls input[type=number]{box-shadow:var(--s1)}
.controls button{width:2.2em;height:2.2em;border-radius:0.375em;display:flex;justify-content:center}
.controls button{align-items:center;font-size:1.4em;font-weight:700;color:var(--c1)}
.controls button{background-color:var(--b6);border:none;cursor:pointer;box-shadow:var(--s1)}
.controls button{transition:background-color 0.3s,color 0.3s}
.controls button:hover{background-color:var(--b1);color:var(--b4)}
.controls button:active{background-color:var(--b2);color:var(--n1)}
fieldset{border:none;padding:0!important;margin:0!important}
fieldset.buttons{display:flex;gap:var(--g2)}
button.nav-btn{font-size:1.2em;cursor:pointer;height:3em}
button.nav-btn.alert{animation:shadow-blink 1.2s infinite;--shadow:var(--s2);--color:var(--e1)}
.preset-btn,button.btn-step{font-size:1.4em;font-weight:700;line-height:1}
form label{color:var(--c2);font-size:1.4em;flex-shrink:0}
svg.icon { width: 1.1em; height: 1.1em; stroke: currentColor; fill: none; stroke-width: 2; stroke-linecap: round; stroke-linejoin: round; user-select: none; pointer-events: none; vertical-align: middle; }
.dropdown-menu{position:absolute;top:50%;left:100%;transform:translate(0.5em,0);min-width:5em}
.dropdown-menu{width:max-content;max-height:10em;overflow-y:auto;padding:0.1em 0.1em}
.dropdown-menu{border-radius:0.375em;background:var(--b4);color:var(--n1);box-shadow:var(--s1);z-index:1000}
.dropdown-menu ul{list-style:none;margin:0;padding:0}
.dropdown-menu li{margin:0;padding:0}
.preset-btn{width:100%;padding:0.2em 0.1em;text-align:center;background-color:var(--b6)}
.preset-btn{border:none;color:var(--c4);cursor:pointer;font-size:1.2em}
.preset-btn{transition:background-color 0.3s,color 0.3s}
.preset-btn:hover{background-color:var(--b1);color:var(--b4)}
.btn-preset.active{background-color:var(--b3);color:var(--b4)}</style></head>
<body>
<header><h1><svg id="logo" viewBox="0 0 24 24" aria-label="Logo" role="img"><defs><filter id="glow" x="-50%" y="-50%" width="200%" height="200%"><feGaussianBlur stdDeviation="1.5" result="blur"/>
<feMerge><feMergeNode in="blur"/><feMergeNode in="SourceGraphic"/></feMerge></filter></defs>
<g filter="url(#glow)"><line x1="12" y1="4" x2="12" y2="10.5" class="line"/>
<path d="M16.24 7a7 7 0 1 1-8.48 0" class="circle"/></g></svg> Lab PSU</h1>
<div><span id="error-status" aria-label="Error Status" aria-live="assertive" role="status">ERROR</span>
<span id="ws-status" aria-label="WebSocket Status" aria-live="polite" role="status">WS</span>
<span id="wifi" title="Wi-Fi Status">Wi-Fi: <span class="ssid">...</span> <span class="rssi">(… dBm)</span></span></div></header>
<main><section aria-label="Power Supply Readings" id="readings">
<p id="v"><span class="label">Voltage</span> <span class="value-unit"><span class="number value">--.--</span> <span class="unit">V</span></span></p>
<p id="i"><span class="label">Current</span> <span class="value-unit"><span class="number value">--.--</span> <span class="unit">A</span></span></p>
<p id="q"><span class="label">Power </span> <span class="value-unit"><span class="number value">--.--</span> <span class="unit">W</span></span></p></section>
<section aria-label="Temperature" id="temperature"><span id="ntcTemp">NTC Temp: --.- °C</span></section>
<form aria-label="Power Supply Settings"><fieldset>
<div class="param-row"><label for="inputV">Vset (V)</label><div class="controls">
<button class="btn-step" type="button" data-target="inputV" data-step="-0.01">−</button>
<input id="inputV" step="0.01" type="number">
<button class="btn-step" type="button" data-target="inputV" data-step="0.01">+</button>
<button class="btn-preset" type="button" data-target="inputV" aria-label="Presets">⋮</button></div></div>
<div class="param-row"><label for="inputIL">Iset (A)</label><div class="controls">
<button class="btn-step" type="button" data-target="inputIL" data-step="-0.01">−</button>
<input id="inputIL" step="0.01" type="number">
<button class="btn-step" type="button" data-target="inputIL" data-step="0.01">+</button>
<button class="btn-preset" type="button" data-target="inputIL" aria-label="Presets">⋮</button></div></div>
<div class="param-row"><label for="inputIF">Icut (A)</label><div class="controls">
<button class="btn-step" type="button" data-target="inputIF" data-step="-0.01">−</button>
<input id="inputIF" step="0.01" type="number">
<button class="btn-step" type="button" data-target="inputIF" data-step="0.01">+</button>
<button class="btn-preset" type="button" data-target="inputIF" aria-label="Presets">⋮</button></div></div></fieldset></form>
<fieldset aria-label="Control" class="buttons">
<button class="nav-btn" type="button" id="btnCharts" title="Charts"><svg class="icon"><use href="#icon-chart"></use></svg>Charts</button>
<button class="nav-btn" type="button" id="btnSettings" title="Settings"><svg class="icon"><use href="#icon-settings"></use></svg>Config</button>
<button class="nav-btn" type="button" id="btnSystem" title="Info"><svg class="icon"><use href="#icon-info"></use></svg>Info</button>
<button class="nav-btn" type="button" id="modeBtn" title="Digital Control" aria-pressed="false"><svg class="icon"><use href="#icon-mode"></use></svg>Auto</button></fieldset>
<fieldset aria-label="Control" class="buttons">
<button class="nav-btn" type="button" id="outputBtn" title="Output" aria-pressed="false"><svg class="icon"><use href="#icon-power"></use></svg>Output</button></fieldset>
<div class="dropdown-menu" aria-label="Presets" hidden id="presetMenu"><ul id="presetList"></ul></div></main>
<script>let globals={};const e={wsStatus:document.getElementById("ws-status"),errorStatus:document.getElementById("error-status"),
wifiSSID:document.querySelector("#wifi .ssid"),wifiRSSI:document.querySelector("#wifi .rssi"),
vNumber:document.querySelector("#v .number"),iNumber:document.querySelector("#i .number"),
qNumber:document.querySelector("#q .number"),inputV:document.getElementById("inputV"),
inputIL:document.getElementById("inputIL"),inputIF:document.getElementById("inputIF"),
ntcTemp:document.getElementById("ntcTemp"),modeBtn:document.getElementById("modeBtn"),
outputBtn:document.getElementById("outputBtn"),btnCharts:document.getElementById("btnCharts"),
btnSettings:document.getElementById("btnSettings"),btnSystem:document.getElementById("btnSystem"),
presetMenu:document.getElementById("presetMenu"),presetList:document.getElementById("presetList")};
const presets={inputV:[{name:"1.5 V",value:1.5},{name:"3 V",value:3},{name:"3.3 V",value:3.3},
{name:"5 V",value:5},{name:"6 V",value:6},{name:"9 V",value:9},{name:"12 V",value:12},
{name:"24 V",value:24},{name:"36 V",value:36}],
inputIL_IF:[{name:"0.02 A",value:.02},{name:"0.05 A",value:.05},{name:"0.1 A",value:.1},
{name:"0.25 A",value:.25},{name:"0.5 A",value:.5},{name:"0.75 A",value:.75},{name:"1.0 A",value:1},
{name:"1.5 A",value:1.5},{name:"2.0 A",value:2}]};
function initApp(){if(!validateElements(e)){setTimeout(initApp,100);return}setupApplication(e);
const logo=document.getElementById('logo');if(logo){logo.classList.add('animate');
logo.addEventListener('mouseenter', () => {logo.classList.remove('animate');void logo.getBoundingClientRect();setTimeout(() => logo.classList.add('animate'), 20);});}}
function validateElements(e){for(let[k,v]of Object.entries(e))if(!v&&k!=='presetMenu'&&k!=='presetList'&&k!=='errorStatus')return false;return true}
function setupApplication(e){let ws=null,reconnectTimeout=null,reconnectAttempts=0,
systemLimits={VoutMin:null,VoutMax:null,IlimitMax:null},settingsRequested=false;
function applySystemLimits(){if(systemLimits.VoutMin!==null&&systemLimits.VoutMax!==null){
e.inputV.min=systemLimits.VoutMin.toFixed(2);e.inputV.max=systemLimits.VoutMax.toFixed(2);
if(e.inputV.value){let v=parseFloat(e.inputV.value);if(v<systemLimits.VoutMin)v=systemLimits.VoutMin;
if(v>systemLimits.VoutMax)v=systemLimits.VoutMax;e.inputV.value=v.toFixed(2)}}
if(systemLimits.IlimitMax!==null){e.inputIL.max=systemLimits.IlimitMax.toFixed(2);
e.inputIF.max=systemLimits.IlimitMax.toFixed(2);if(e.inputIL.value){
let v=parseFloat(e.inputIL.value);if(v>systemLimits.IlimitMax)v=systemLimits.IlimitMax;
e.inputIL.value=v.toFixed(2)}if(e.inputIF.value){let v=parseFloat(e.inputIF.value);
if(v>systemLimits.IlimitMax)v=systemLimits.IlimitMax;e.inputIF.value=v.toFixed(2)}}}
function requestSettingsPage(){if(ws&&ws.readyState===WebSocket.OPEN&&!settingsRequested){
ws.send(JSON.stringify({action:"OPEN",page:"settings"}));settingsRequested=true}}
function createPresetButton(p,t){const li=document.createElement("li"),btn=document.createElement("button");
btn.className="preset-btn";btn.textContent=`${p.value.toFixed(2)} ${t==="inputV"?"V":"A"}`;
btn.type="button";btn.addEventListener("click",()=>{const i=document.getElementById(t);if(i){
let v=p.value;if(t==="inputV"&&systemLimits.VoutMin!==null&&systemLimits.VoutMax!==null)
v=Math.max(systemLimits.VoutMin,Math.min(systemLimits.VoutMax,v));
else if((t==="inputIL"||t==="inputIF")&&systemLimits.IlimitMax!==null)v=Math.min(systemLimits.IlimitMax,v);
i.value=v.toFixed(2);sendSetting(t.replace(/^input/i,"").toUpperCase(),i.value)}closePresetMenus()});
li.appendChild(btn);return li}
function closePresetMenus(){if(e.presetMenu){e.presetMenu.setAttribute("hidden","");
e.presetMenu.style.display="none"}document.querySelectorAll(".btn-preset.active").forEach(b=>b.classList.remove("active"))}
function positionPresetMenu(b){if(!e.presetMenu||!e.presetList)return;const r=b.getBoundingClientRect(),
gap=6,itemHeight=32,presetCount=e.presetList.children.length,maxVisible=5,
menuHeight=Math.min(presetCount,maxVisible)*itemHeight,menuWidth=Math.max(e.presetMenu.offsetWidth,12*.6*16);
e.presetMenu.style.width=`${menuWidth}px`;e.presetMenu.style.height=`${menuHeight}px`;
e.presetMenu.style.overflowY=presetCount>maxVisible?"auto":"hidden";
let top=r.top+window.scrollY+r.height/2-menuHeight/2;top=Math.max(top,10);
top=Math.min(top,window.innerHeight-menuHeight-10);e.presetMenu.style.top=`${top}px`;
e.presetMenu.style.left=`${r.right+gap+window.scrollX}px`;e.presetMenu.style.display="block";
e.presetMenu.removeAttribute("hidden")}
document.querySelectorAll("button.btn-preset").forEach(b=>{b.addEventListener("click",ev=>{
ev.stopPropagation();const t=b.getAttribute("data-target"),a=b.classList.contains("active");
closePresetMenus();if(a)return;b.classList.add("active");if(e.presetList){e.presetList.innerHTML="";
let pa=t==="inputV"?presets.inputV.filter(p=>!systemLimits.VoutMin||p.value>=systemLimits.VoutMin&&
(!systemLimits.VoutMax||p.value<=systemLimits.VoutMax)):presets.inputIL_IF.filter(p=>
!systemLimits.IlimitMax||p.value<=systemLimits.IlimitMax);pa.forEach(p=>e.presetList.appendChild(createPresetButton(p,t)));
positionPresetMenu(b)}});document.addEventListener("click",closePresetMenus)});
function sendSetting(p,v){if(ws&&ws.readyState===WebSocket.OPEN)ws.send(JSON.stringify({[p]:parseFloat(v)}))}
function formatNumber(v,d=3,w=6){const s=v<0?'-':'\u2007',f=Math.abs(v).toFixed(d);return (s+f).padStart(w,'\u2007')}
function handleMessage(d){try{const o=JSON.parse(d);if(!o)return;
if("HUE" in o){globals['HUE']=parseFloat(o.HUE)||0;document.documentElement.style.setProperty('--h',globals['HUE'])}
if("ERR" in o&&e.errorStatus){const ec=parseInt(o.ERR)||0;e.errorStatus.classList.remove("status-error","status-alert","status-normal");
e.btnSettings.classList.remove("alert");if(ec!==0){e.errorStatus.classList.add("status-error");
e.btnSettings.classList.add("alert")}else{e.errorStatus.classList.add("status-normal")}
if(e.ntcTemp){e.ntcTemp.classList.remove("status-error","status-alert","status-normal");
e.ntcTemp.classList.add(ec&1?"status-alert":"status-normal")}}
if("V" in o&&e.vNumber)e.vNumber.textContent=formatNumber(parseFloat(o.V)||0,3,6);
if("I" in o&&e.iNumber)e.iNumber.textContent=formatNumber(parseFloat(o.I)||0,3,6);
if("Q" in o&&e.qNumber)e.qNumber.textContent=formatNumber(parseFloat(o.Q)||0,3,6);
if("VSET" in o&&e.inputV&&document.activeElement!==e.inputV){
let v=parseFloat(o.VSET)||0;if(systemLimits.VoutMin!==null&&v<systemLimits.VoutMin)v=systemLimits.VoutMin;
if(systemLimits.VoutMax!==null&&v>systemLimits.VoutMax)v=systemLimits.VoutMax;e.inputV.value=v.toFixed(2)}
if("IL" in o&&e.inputIL&&document.activeElement!==e.inputIL){
let v=parseFloat(o.IL)||0;if(systemLimits.IlimitMax!==null&&v>systemLimits.IlimitMax)v=systemLimits.IlimitMax;
e.inputIL.value=v.toFixed(2)}
if("IF" in o&&e.inputIF&&document.activeElement!==e.inputIF){
let v=parseFloat(o.IF)||0;if(systemLimits.IlimitMax!==null&&v>systemLimits.IlimitMax)v=systemLimits.IlimitMax;
e.inputIF.value=v.toFixed(2)}
if("TEMP" in o&&e.ntcTemp)e.ntcTemp.textContent=`NTC Temp: ${(parseFloat(o.TEMP)||0).toFixed(1)} °C`;
if("MODE" in o&&e.modeBtn){const m=o.MODE==="auto";e.modeBtn.classList.toggle("active",m);
e.modeBtn.setAttribute("aria-pressed",m);e.modeBtn.title=`Mode: ${m?"Auto":"Manual"}`}
if("OUT" in o&&e.outputBtn){const on=o.OUT==="1";e.outputBtn.classList.toggle("active",on);
e.outputBtn.setAttribute("aria-pressed",on);e.outputBtn.title=on?"Output ON":"Output OFF"}
if("PAGE_CHARTS" in o&&e.btnCharts){const ca=o.PAGE_CHARTS===1;e.btnCharts.classList.toggle("active-page",ca);
e.btnCharts.title=ca?"Charts (открыта)":"Charts"}
if("PAGE_SETTINGS" in o&&e.btnSettings){const sa=o.PAGE_SETTINGS===1;e.btnSettings.classList.toggle("active-page",sa);
e.btnSettings.title=sa?"Settings (открыта)":"Settings"}
if("PAGE_SYSTEM" in o&&e.btnSystem){const sa=o.PAGE_SYSTEM===1;e.btnSystem.classList.toggle("active-page",sa);
e.btnSystem.title=sa?"System (открыта)":"System"}
if("WIFI_SSID" in o&&e.wifiSSID)e.wifiSSID.textContent=o.WIFI_SSID;
if("WIFI_RSSI" in o&&e.wifiRSSI)e.wifiRSSI.textContent=`(${o.WIFI_RSSI} dBm)`;
if("VoutMin" in o)systemLimits.VoutMin=parseFloat(o.VoutMin)||0;
if("VoutMax" in o)systemLimits.VoutMax=parseFloat(o.VoutMax)||0;
if("IlimitMax" in o)systemLimits.IlimitMax=parseFloat(o.IlimitMax)||0;applySystemLimits()}catch(e){}}
function connectWS(){if(reconnectTimeout){clearTimeout(reconnectTimeout);reconnectTimeout=null}
try{const p=window.location.protocol==='https:'?'wss://':'ws://';ws=new WebSocket(p+window.location.hostname+"/ws");
ws.onopen=()=>{if(e.wsStatus)e.wsStatus.classList.add("active");
ws.send(JSON.stringify({action:"OPEN",page:"main"}));
if(systemLimits.VoutMin===null||systemLimits.VoutMax===null||systemLimits.IlimitMax===null)requestSettingsPage();
reconnectAttempts=0};
ws.onclose=()=>{if(e.wsStatus)e.wsStatus.classList.remove("active");
reconnectTimeout=setTimeout(connectWS,Math.min(2000*(2**reconnectAttempts),10000));reconnectAttempts++};
ws.onmessage=e=>handleMessage(e.data);ws.onerror=()=>{ws.close()}}catch(e){reconnectTimeout=setTimeout(connectWS,2000)}}
document.querySelectorAll(".btn-step").forEach(b=>{let interval=null,timeout=null;
const t=b.getAttribute("data-target"),s=parseFloat(b.getAttribute("data-step")),i=document.getElementById(t);
if(!i)return;const u=()=>{let v=parseFloat(i.value)||0;v+=s;
if(t==="inputV"&&systemLimits.VoutMin!==null&&systemLimits.VoutMax!==null)
v=Math.max(systemLimits.VoutMin,Math.min(systemLimits.VoutMax,v));
else if((t==="inputIL"||t==="inputIF")&&systemLimits.IlimitMax!==null)v=Math.min(systemLimits.IlimitMax,v);
i.value=v.toFixed(2);sendSetting(t.replace(/^input/i,"").toUpperCase(),i.value)};
b.addEventListener("mousedown",()=>{u();timeout=setTimeout(()=>{interval=setInterval(u,100)},500)});
const c=()=>{clearInterval(interval);clearTimeout(timeout)};
b.addEventListener("mouseup",c);b.addEventListener("mouseleave",c);b.addEventListener("blur",c)});
[e.inputV,e.inputIL,e.inputIF].forEach(i=>{if(!i)return;i.addEventListener("change",()=>{
let v=parseFloat(i.value);if(isNaN(v))return;
if(i.id==="inputV"&&systemLimits.VoutMin!==null&&systemLimits.VoutMax!==null)
v=Math.max(systemLimits.VoutMin,Math.min(systemLimits.VoutMax,v));
else if((i.id==="inputIL"||i.id==="inputIF")&&systemLimits.IlimitMax!==null)v=Math.min(systemLimits.IlimitMax,v);
i.value=v.toFixed(2);sendSetting(i.id.replace(/^input/i,"").toUpperCase(),i.value)})});
if(e.modeBtn)e.modeBtn.addEventListener("click",()=>{if(ws&&ws.readyState===WebSocket.OPEN)
ws.send(JSON.stringify({MODE:e.modeBtn.classList.contains("active")?"manual":"auto"}))});
if(e.outputBtn)e.outputBtn.addEventListener("click",()=>{if(ws&&ws.readyState===WebSocket.OPEN)
ws.send(JSON.stringify({OUT:e.outputBtn.classList.contains("active")?"0":"1"}))});
if(e.btnCharts)e.btnCharts.addEventListener("click",()=>{window.open("/charts","_blank");
e.btnCharts.classList.add("active-page");e.btnCharts.title="Charts (открыта)"});
if(e.btnSettings)e.btnSettings.addEventListener("click",()=>{window.open("/settings","_blank");
e.btnSettings.classList.add("active-page");e.btnSettings.title="Settings (открыта)"});
if(e.btnSystem)e.btnSystem.addEventListener("click",()=>{window.open("/system","_blank");
e.btnSystem.classList.add("active-page");e.btnSystem.title="System (открыта)"});connectWS()}
if(document.readyState==='loading')document.addEventListener('DOMContentLoaded',initApp);else initApp();
</script>
<svg style=display:none xmlns=http://www.w3.org/2000/svg>
<symbol id=icon-settings viewBox="0 0 24 24"><circle cx=12 cy=12 r=3 />
<path d="M19.4 15a1.65 1.65 0 0 0 .33 1.82l.06.06a2 2 0 1 1-2.83 2.83l-.06-.06a1.65 1.65 0 0 0-1.82-.33 1.65 1.65 0 0 
0-1 1.51V21a2 2 0 1 1-4 0v-.09a1.65 1.65 0 0 0-1-1.51 1.65 1.65 0 0 0-1.82.33l-.06.06a2 2 0 1 1-2.83-2.83l.06-.06a1.65 
1.65 0 0 0 .33-1.82 1.65 1.65 0 0 0-1.51-1H3a2 2 0 1 1 0-4h.09a1.65 1.65 0 0 0 1.51-1 1.65 1.65 0 0 0-.33-1.82l-.06-.06a2 
2 0 1 1 2.83-2.83l.06.06a1.65 1.65 0 0 0 1.82.33h.09a1.65 1.65 0 0 0 1-1.51V3a2 2 0 1 1 4 0v.09a1.65 1.65 0 0 0 1 1.51h.09a1.65 
1.65 0 0 0 1.82-.33l.06-.06a2 2 0 1 1 2.83 2.83l-.06.06a1.65 1.65 0 0 0-.33 1.82v.09a1.65 1.65 0 0 0 1.51 1H21a2 2 0 1 1 0 4h-.09a1.65 1.65 0 0 0-1.51 1z" /></symbol>
<symbol id=icon-chart viewBox="0 0 24 24"><rect height=20 rx=3 ry=3 width=20 x=2 y=2 /><polyline points="2 17 9 10 14 16 22 8" /></symbol>
<symbol id=icon-info viewBox="0 0 24 24"><circle cx=12 cy=12 r=10 /><line x1=12 x2=12 y1=16 y2=12 /><line x1=12 x2=12 y1=8 y2=8 /></symbol>
<symbol id=icon-mode viewBox="0 0 24 24"><path d="M8 2 L23 11 L8 20 Z" /></symbol>
<symbol id=icon-power viewBox="0 0 24 24"><path d="M16 4 A8 8 0 1 1 8 4" /><line x1=12 x2=12 y1=1 y2=8 /></symbol>
</svg></body></html>
)rawliteral";

const char system_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Lab PSU Info</title>
<link rel="stylesheet" href="/style.css">
<style>
:root { transition: all 0.3s ease; }
a { color: var(--c2, hsl(var(--h, 0), 80%, 50%)); text-decoration: none; }
a:hover { text-decoration: underline; }
ul { margin: 0.5em 0 0 1em; padding: 0; }
li { margin: 0.3em 0; }
.section { background: var(--b5); border-radius: .375em; padding: .8em 1em; box-shadow: var(--s1); margin: .5em 0; }
</style>
</head>
<body>
<h1>Lab PSU Info</h1>
<div class="section">
<h2>About</h2>
<p>This is a UG56 Lab Power Supply Unit project.</p>
<p>Version: 1.0.0</p>
<hr>
<h2>License</h2>
<p>MIT License</p>
<hr>
<h2>Links</h2>
<ul>
  <li><a href="https://github.com/UG56/lab-psu" target="_blank">GitHub Repository</a></li>
  <li><a href="https://www.youtube.com/channel/UC_YourChannelID" target="_blank">YouTube Channel</a></li>
  <li><a href="https://ko-fi.com/universalgeek56" target="_blank">Support on Ko-fi</a></li>
</ul>
<hr>
<h2>Instructions</h2>
<p>Use the main interface to control the Lab PSU and monitor voltage, current, power, and temperature.</p>
<p>Expert mode unlocks advanced settings (PWM, PID, system limits).</p>
</div>
<script>
let ws = new WebSocket("ws://" + location.hostname + "/ws");
const pageName = "system";
let globals = {};
ws.onopen = () => {console.log("WS connected");sendOpen();setInterval(sendOpen, 5000);};
function sendOpen() {if (ws.readyState === WebSocket.OPEN) {ws.send(JSON.stringify({ page: pageName, action: "OPEN" }));}}
ws.onmessage = (e) => {try {const obj = JSON.parse(e.data);console.log("Received from server:", obj);if ('HUE' in obj) {globals['HUE'] = parseFloat(obj['HUE']);
  document.documentElement.style.setProperty('--h', globals['HUE']);}} catch (err) {console.warn("WS parse error:", err);}};
ws.onclose = () => {console.log("WS closed");};
ws.onerror = () => {console.log("WS error");};
</script>
</body>
</html>
)rawliteral";



// === Charts ==
const char charts_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en"><head><meta charset="UTF-8">
<title>Lab PSU - Charts</title>
<link rel="stylesheet" href="/style.css">
<style>
html, body { width: 100%; height: 100%; }
body { display: flex; flex-direction: column; padding: 1em; gap: 0; }
.chart-container { flex: 1 1 auto; background: var(--b5); border-radius: 0.5em; padding: 1em 1.5em; box-shadow: var(--s1); box-sizing: border-box; display: flex; align-items: stretch; justify-content: stretch; min-height: 0; }
canvas { width: 100% !important; height: 100% !important; display: block; border-radius: 0.25em; background: var(--b5); }
h1 { display: flex; align-items: center; justify-content: space-between; }
#debugToggle { margin-left: 1em; }
</style>
</head>
<body>
<h1>Lab PSU - Charts <label><input type="checkbox" id="debugToggle"> Debug</label></h1>
<div class="chart-container"><canvas id="chartCombined"></canvas></div>
<script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
<script>
const maxPoints = 100;const dataArray = [];const startTime = Date.now();const pageName = "charts";let globals = {};let reconnectAttempts = 0;const maxReconnectDelay = 30000;
function wsOnMessage(event) {try {
const data = JSON.parse(event.data);
if ('HUE' in data) {globals['HUE'] = parseFloat(data['HUE']);document.documentElement.style.setProperty('--h', globals['HUE']);}
const now = Date.now() - startTime;
const jitter = (arr, maxDiff) => {if (!arr.length) return 0;let min = Infinity, max = -Infinity;for (const v of arr)if (v != null && !isNaN(v) && isFinite(v)) {if (v < min) min = v;if (v > max) max = v;}if (max === -Infinity || min === Infinity) return 0;
if (max - min < maxDiff * 0.05)return arr[arr.length - 1] + (Math.random() - 0.5) * maxDiff * 0.02;return arr[arr.length - 1];};
const newData = {time: now,V: +data.V || 0,I: +data.I || 0,Q: +data.Q || 0,TEMP: +data.TEMP || 0,VSET: +data.VSET || 0,IL: +data.IL || 0,IF: +data.IF || 0,
D0: +data.debug?.Dbg0 || 0,D1: +data.debug?.Dbg1 || 0,D2: +data.debug?.Dbg2 || 0,D3: +data.debug?.Dbg3 || 0,D4: +data.debug?.Dbg4 || 0,D5: +data.debug?.Dbg5 || 0};
dataArray.push(newData);if (dataArray.length > maxPoints) dataArray.shift();
combinedChart.data.labels = dataArray.map(d=>d.time);
combinedChart.data.datasets[0].data = dataArray.map(d=>d.V);
combinedChart.data.datasets[1].data = dataArray.map(d=>d.I);
combinedChart.data.datasets[2].data = dataArray.map(d=>d.Q);
combinedChart.data.datasets[3].data = dataArray.map(d=>d.TEMP);
combinedChart.data.datasets[4].data = dataArray.map(d=>d.VSET);
combinedChart.data.datasets[5].data = dataArray.map(d=>d.IL);
combinedChart.data.datasets[6].data = dataArray.map(d=>d.IF);
combinedChart.data.datasets[7].data = dataArray.map(d=>d.D0);
combinedChart.data.datasets[8].data = dataArray.map(d=>d.D1);
combinedChart.data.datasets[9].data = dataArray.map(d=>d.D2);
combinedChart.data.datasets[10].data = dataArray.map(d=>d.D3);
combinedChart.data.datasets[11].data = dataArray.map(d=>d.D4);
combinedChart.data.datasets[12].data = dataArray.map(d=>d.D5);
if (data.debug && data.debug.enabled) {
  document.getElementById("debugToggle").checked = true;for (let i=7;i<=12;i++) combinedChart.options.scales[`yDbg${i-7}`].display = true;} else {document.getElementById("debugToggle").checked = false;for (let i=7;i<=12;i++) combinedChart.options.scales[`yDbg${i-7}`].display = false;}
combinedChart.resize();
updateScales();
combinedChart.update('none');
} catch(e) {}}
function setupWS() {const wsNew = new WebSocket("ws://" + location.hostname + "/ws");wsNew.onopen = () => {reconnectAttempts = 0;if (wsNew.readyState === WebSocket.OPEN) {wsNew.send(JSON.stringify({ page: pageName, action: "OPEN" }));}
  setInterval(() => {if (wsNew.readyState === WebSocket.OPEN) {wsNew.send(JSON.stringify({ page: pageName, action: "OPEN" }));}}, 5000);};
  wsNew.onmessage = wsOnMessage;wsNew.onclose = () => {reconnectAttempts++;const delay = Math.min(1000 * Math.pow(2, reconnectAttempts), maxReconnectDelay);setTimeout(setupWS, delay);};wsNew.onerror = e => wsNew.close();return wsNew;}
const ws = setupWS();
function calcSmartRange(arr, step, minMax) {let max = -Infinity, min = Infinity;for (const v of arr) {if (v != null && !isNaN(v) && isFinite(v)) {if (v < min) min = v;if (v > max) max = v;}}
  if (max === -Infinity || min === Infinity) return [0, minMax];
  let range = max - min;
  let maxWithPadding = max + range * 0.15;
  let minWithPadding = min - range * 0.15;
  if (maxWithPadding < minMax) maxWithPadding = minMax;
  maxWithPadding = Math.ceil(maxWithPadding / step) * step;
  minWithPadding = Math.floor(minWithPadding / step) * step;
  return [minWithPadding, maxWithPadding];}
const unitsPlugin = {id: "unitsPlugin",afterDraw(chart) {const ctx = chart.ctx;ctx.save();ctx.font = "12px 'Roboto Mono'";ctx.textAlign = "center";ctx.textBaseline = "top";const scales = [
  { id: "yVoltage", unit: "V", color: "#4CAF50" },
  { id: "yCurrent", unit: "A", color: "#FFC107" },
  { id: "yPower", unit: "W", color: "#03A9F4" },
  { id: "yTemp", unit: "°C", color: "#FF69B4" }];
  scales.forEach(scale => {const yScale = chart.scales[scale.id];if (yScale && yScale.options.display) {const x = yScale.left + yScale.width / 2;const y = yScale.bottom + 10;ctx.fillStyle = scale.color;ctx.fillText(scale.unit, x, y);}});ctx.restore();}};
Chart.register(unitsPlugin);
const ctx = document.getElementById("chartCombined").getContext("2d");const combinedChart = new Chart(ctx, {type: "line",data: {labels: [],datasets: [
  { label: "Vmeas", data: [], borderColor: "#4CAF50", yAxisID: "yVoltage", fill: false, tension: 0.2, pointRadius: 0, borderWidth: 2, order: 1, hidden: false },
  { label: "Imeas", data: [], borderColor: "#FFC107", yAxisID: "yCurrent", fill: false, tension: 0.2, pointRadius: 0, borderWidth: 2, order: 1, hidden: false },
  { label: "Pmeas", data: [], borderColor: "#03A9F4", yAxisID: "yPower", fill: false, tension: 0.2, pointRadius: 0, borderWidth: 2, order: 1, hidden: true },
  { label: "NTC temp", data: [], borderColor: "#FF69B4", yAxisID: "yTemp", fill: false, tension: 0.2, pointRadius: 0, borderWidth: 2, order: 1, hidden: true },
  { label: "Vset", data: [], borderColor: "#8BC34A", yAxisID: "yVoltage", fill: false, tension: 0.2, pointRadius: 0, borderWidth: 1, order: 2, hidden: true },
  { label: "Iset", data: [], borderColor: "#FF9800", yAxisID: "yCurrent", fill: false, tension: 0.2, pointRadius: 0, borderWidth: 1, order: 2, hidden: true },
  { label: "Icut", data: [], borderColor: "#FF0000", yAxisID: "yCurrent", fill: false, tension: 0.2, pointRadius: 0, borderWidth: 1, order: 2, hidden: true },
  { label: "Dbg0", data: [], borderColor: "#9C27B0", yAxisID: "yDbg0", fill: false, tension: 0.2, pointRadius: 0, borderWidth: 2, order: 1, hidden: true },
  { label: "Dbg1", data: [], borderColor: "#3F51B5", yAxisID: "yDbg1", fill: false, tension: 0.2, pointRadius: 0, borderWidth: 2, order: 1, hidden: true },
  { label: "Dbg2", data: [], borderColor: "#E91E63", yAxisID: "yDbg2", fill: false, tension: 0.2, pointRadius: 0, borderWidth: 2, order: 1, hidden: true },
  { label: "Dbg3", data: [], borderColor: "#FF9800", yAxisID: "yDbg3", fill: false, tension: 0.2, pointRadius: 0, borderWidth: 2, order: 1, hidden: true },
  { label: "Dbg4", data: [], borderColor: "#00BCD4", yAxisID: "yDbg4", fill: false, tension: 0.2, pointRadius: 0, borderWidth: 2, order: 1, hidden: true },
  { label: "Dbg5", data: [], borderColor: "#F06292", yAxisID: "yDbg5", fill: false, tension: 0.2, pointRadius: 0, borderWidth: 2, order: 1, hidden: true }]},
  options: {responsive: true,maintainAspectRatio: false,animation: false,interaction: { mode: "nearest", intersect: false },layout: {padding: {bottom: 40}},plugins: {legend: {
  display: true,position: "top",labels: {
    usePointStyle: true,
    pointStyle: 'circle',
    boxWidth: 8,
    boxHeight: 8,
    font: { size: 12, family: "'Roboto Mono', monospace" },
    filter: item => document.getElementById('debugToggle').checked || item.datasetIndex < 7,
    generateLabels(chart) {
      return chart.data.datasets.map((dataset, i) => ({
        text: dataset.label,
        fillStyle: chart.isDatasetVisible(i) ? dataset.borderColor : 'transparent',
        strokeStyle: dataset.borderColor,
        pointStyle: 'circle',
        lineWidth: 2,
        boxWidth: 8,
        boxHeight: 8,
        datasetIndex: i,
        fontColor: chart.isDatasetVisible(i) ? dataset.borderColor : '#888'
      }));
    },
    onClick(e, item, legend) {const i = item.datasetIndex;legend.chart.setDatasetVisibility(i, !legend.chart.isDatasetVisible(i));legend.chart.update();}}},
  tooltip: {callbacks: {title: () => '',label: context => `${context.dataset.label}: ${context.raw}`}}},
  scales: {
  x: { ticks: { display: false, maxTicksLimit: 10 }, grid: { color: "#444", lineWidth: 1, drawTicks: false } },
  yVoltage: { position: "left", beginAtZero: true, ticks: { color: "#4CAF50" }, grid: { color: "#4CAF5040" } },
  yCurrent: { position: "left", min: 0, beginAtZero: true, ticks: { color: "#FFC107" }, grid: { drawOnChartArea: false } },
  yPower: { position: "right", beginAtZero: true, ticks: { color: "#03A9F4" }, grid: { drawOnChartArea: false } },
  yTemp: { position: "right", beginAtZero: true, ticks: { color: "#FF69B4" }, grid: { drawOnChartArea: false } },
  yDbg0: { position: "right", display: false, beginAtZero: true, ticks: { color: "#9C27B0" } },
  yDbg1: { position: "right", display: false, beginAtZero: true, ticks: { color: "#3F51B5" } },
  yDbg2: { position: "right", display: false, beginAtZero: true, ticks: { color: "#E91E63" } },
  yDbg3: { position: "right", display: false, beginAtZero: true, ticks: { color: "#FF9800" } },
  yDbg4: { position: "right", display: false, beginAtZero: true, ticks: { color: "#00BCD4" } },
  yDbg5: { position: "right", display: false, beginAtZero: true, ticks: { color: "#F06292" } }}}});
function updateScales() {
  const V = combinedChart.data.datasets[0].data;
  const I = combinedChart.data.datasets[1].data;
  const P = combinedChart.data.datasets[2].data;
  const T = combinedChart.data.datasets[3].data;
  for (let i = 0; i <= 5; i++) {combinedChart.options.scales[`yDbg${i}`].max = calcSmartRange(combinedChart.data.datasets[7 + i].data, 0, 0)[1];}
  combinedChart.options.scales.yVoltage.max = calcSmartRange(V, 1, 1)[1];
  combinedChart.options.scales.yCurrent.max = calcSmartRange(I, 0.05, 0.05)[1];
  combinedChart.options.scales.yPower.max = calcSmartRange(P, 0.05, 0.05)[1];
  combinedChart.options.scales.yTemp.max = calcSmartRange(T, 5, 10)[1];}
document.getElementById("debugToggle").addEventListener("change", e => {const show = e.target.checked;for (let i = 7; i <= 12; i++) {combinedChart.setDatasetVisibility(i, show);combinedChart.options.scales[`yDbg${i - 7}`].display = show;}
  ws.send(JSON.stringify({ action: show ? "DEBUG_ON" : "DEBUG_OFF" }));combinedChart.update();});
window.addEventListener("resize", () => combinedChart.resize());
</script>
</body>
</html>
)rawliteral";  


// === Settings =======
const char settings_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Lab PSU Settings</title>
<link rel="stylesheet" href="/style.css">
<style>
:root { transition: all 0.3s ease; }
body { max-width: 40em; min-width: 40em}
.section.button-section { box-shadow: none; display: flex; gap: var(--g2); padding: 0;}
.nav-btn { background: var(--b6); border: none; color: var(--c1); padding: .5em; border-radius: .375em; font-size: 1.1em; height: 2.2em; cursor: pointer; transition: background-color .3s ease, color .3s ease; flex: 1; min-width: 0; user-select: none; display: flex; justify-content: center; align-items: center; box-shadow: var(--s1); }
.nav-btn:hover { background: var(--b1); color: var(--b4); }
.nav-btn:active { background: var(--b2); color: var(--n1); }
.nav-btn.active { background: var(--b3); color: var(--b4); box-shadow: 0 0 15px var(--c1); }
#statusSection.status-error { --color: var(--e1); --shadow: var(--s2); animation: shadow-blink 1.2s infinite; }
#statusSection.status-normal { --color: var(--c1); --shadow: var(--s1); }
#statusList.status-error { color: var(--e1); animation: none; }
#statusList.status-normal { color: var(--c1); }
#statusList div.error-active { color: var(--e1); }
#statusList div.error-inactive { color: var(--c4); }
#statusList div.no-status { color: var(--c4); }
.field-group { display: grid; grid-template-columns: 10em 1fr 8em; gap: .5em .8em; align-items: center; margin: .6em 0; }
.field-group.password-group { grid-template-columns: 7em 1fr 14em; gap: .8em .5em; }
.field { display: contents; }
.field label { text-align: right; color: var(--c2); font-size: 1.1em; user-select: none; }
.field .global { text-align: left; font-size: 1em; color: var(--c4); font-variant-numeric: tabular-nums; }
.field input[type="text"], .field input[type="number"] { justify-self: center; width: 4em; height: 1.8em; font-size: 1em; text-align: center; border-radius: .375em; background: var(--b6); color: var(--c4); border: none; box-shadow: var(--s1); }
.field input[type="number"]::-webkit-outer-spin-button, .field input[type="number"]::-webkit-inner-spin-button { -webkit-appearance: none; margin: 0; }
.field input[type="number"] { -moz-appearance: textfield; }
.field input:focus { outline: none; background: var(--b1); color: var(--b4); }
.field input.input-network { width: 12em; height: 2em; font-size: 1.1em; padding: 0 .2em; }
.field input[type="checkbox"] { justify-self: center; width: 1.2em; height: 1.2em; accent-color: var(--c1); margin: 0; align-self: center; }
#onlineMode { justify-self: start; }
.password-field { display: flex; justify-content: flex-end; align-items: center; }
.password-field input { flex: 1 1 auto; width: 100%; }
.hidden { display: none; }
#statusList { max-height: 150px; overflow-y: auto; font-weight: bold; font-size: 1em; }
#statusSection { display: none; }
#statusSection.visible { display: block; }
#hueSlider { width: 8em; height: 0.8em; border-radius: .375em; background: linear-gradient(to right, 
  hsl(0, 80%, 50%), hsl(30, 80%, 50%), hsl(60, 80%, 50%), hsl(90, 80%, 50%), hsl(120, 80%, 50%), hsl(150, 80%, 50%), hsl(180, 80%, 50%), hsl(210, 80%, 50%), 
  hsl(240, 80%, 50%), hsl(270, 80%, 50%), hsl(300, 80%, 50%), hsl(330, 80%, 50%), hsl(360, 80%, 50%)); justify-self: center; -webkit-appearance: none; appearance: none;}
#hueSlider::-webkit-slider-thumb { -webkit-appearance: none; width: 1.4em; height: 1.4em; border-radius: 50%; background: hsl(var(--h, 85), 80%, 50%); border: 2px solid var(--b4); 
  box-shadow: 0 0 8px rgba(0, 0, 0, 0.3); cursor: pointer; transition: transform 0.2s ease, box-shadow 0.2s ease; }
#hueSlider::-webkit-slider-thumb:hover { transform: scale(1.2); box-shadow: 0 0 12px rgba(0, 0, 0, 0.4); }
#hueSlider::-moz-range-thumb { width: 1.4em; height: 1.4em; border-radius: 50%; background: hsl(var(--h, 85), 80%, 50%); border: 2px solid var(--b4); 
  box-shadow: 0 0 8px rgba(0, 0, 0, 0.3); cursor: pointer; transition: transform 0.2s ease, box-shadow 0.2s ease; }
#hueSlider::-moz-range-thumb:hover { transform: scale(1.2); box-shadow: 0 0 12px rgba(0, 0, 0, 0.4); }
.collapsible-header { cursor: pointer; display: flex; align-items: center; justify-content: space-between; margin-right: 1em;}
.collapsible-header .arrow { display: inline-block; font-size: 0.8em; transition: transform 0.3s ease; margin-left: 0.3em; }
.collapsible-header.collapsed .arrow { transform: rotate(-90deg); }
.collapsible-section { transition: max-height 0.3s ease, opacity 0.3s ease; overflow: hidden; }
.collapsible-section.collapsed { max-height: 0; opacity: 0; }
</style>
</head>
<body>
<h1>Lab PSU Config</h1>
<div class="section" id="userSettings"><h2>User Settings</h2>
<div class="field-group password-group">
<div class="field"><label>SSID:</label><span class="global" id="global_WiFiSSID"></span><input type="text" id="draft_WiFiSSID" class="input-network"></div>
<div class="field"><label>Password:</label><span class="global" id="global_WiFiPass"></span><input type="text" id="draft_WiFiPass" class="input-network"></div>
<div class="field"><label>Theme Hue:</label><span class="global" id="global_HUE"></span><input type="range" id="hueSlider" min="0" max="360" step="1"><input type="number" id="draft_HUE" class="hidden" step="1"></div>
</div>
</div>
<div class="section button-section">
<button id="toggleExpert" class="nav-btn">Show Expert</button>
<button id="btnApply" class="nav-btn">Apply</button>
<button id="toggleErrors" class="nav-btn">Show Errors</button>
</div>
<div class="section status-error hidden" id="statusSection"><h1>Status</h1>
<div id="statusList"></div>
</div>
<div id="expertSettings" class="section hidden">
<h2>Expert Settings</h2><hr>
<label><input type="checkbox" id="onlineMode"> Online Mode (Auto Apply)</label><hr>
<h1 class="collapsible-header" data-section="wifi">WiFi Settings <span class="arrow">▼</span></h1>
<div class="collapsible-section" id="wifi">
<div class="field-group">
<div class="field"><label>WiFi Enabled:</label><span class="global" id="global_WiFiEnabled"></span><input type="checkbox" id="draft_WiFiEnabled"></div>
<div class="field"><label>OTA Enabled:</label><span class="global" id="global_OTAEnabled"></span><input type="checkbox" id="draft_OTAEnabled"></div>
</div>
</div><hr>
<h1 class="collapsible-header" data-section="voltage-pid">Voltage PID <span class="arrow">▼</span></h1>
<div class="collapsible-section" id="voltage-pid">
<div class="field-group">
<div class="field"><label>Kp:</label><span class="global" id="global_Kp"></span><input type="number" id="draft_Kp" step="1"></div>
<div class="field"><label>Ki:</label><span class="global" id="global_Ki"></span><input type="number" id="draft_Ki" step="0.01"></div>
<div class="field"><label>Kd:</label><span class="global" id="global_Kd"></span><input type="number" id="draft_Kd" step="0.01"></div>
<div class="field"><label>Int. Limit:</label><span class="global" id="global_IntegralLimit"></span><input type="number" id="draft_IntegralLimit" step="1"></div>
</div>
</div><hr>
<h1 class="collapsible-header" data-section="current-pid">Current PID <span class="arrow">▼</span></h1>
<div class="collapsible-section" id="current-pid">
<div class="field-group">
<div class="field"><label>Kp_I:</label><span class="global" id="global_Kp_I"></span><input type="number" id="draft_Kp_I" step="1"></div>
<div class="field"><label>Ki_I:</label><span class="global" id="global_Ki_I"></span><input type="number" id="draft_Ki_I" step="0.01"></div>
<div class="field"><label>Kd_I:</label><span class="global" id="global_Kd_I"></span><input type="number" id="draft_Kd_I" step="0.01"></div>
<div class="field"><label>Int. Limit I:</label><span class="global" id="global_IntegralLimit_I"></span><input type="number" id="draft_IntegralLimit_I" step="1"></div>
</div>
</div><hr>
<h1 class="collapsible-header" data-section="other-pid">Other PID Settings <span class="arrow">▼</span></h1>
<div class="collapsible-section" id="other-pid">
<div class="field-group">
<div class="field"><label>Duty Min:</label><span class="global" id="global_DutyMin"></span><input type="number" id="draft_DutyMin" step="1"></div>
<div class="field"><label>Duty Max:</label><span class="global" id="global_DutyMax"></span><input type="number" id="draft_DutyMax" step="1"></div>
<div class="field"><label>Invert PWM:</label><span class="global" id="global_InvertPWM"></span><input type="checkbox" id="draft_InvertPWM"></div>
<div class="field"><label>Debug Mode:</label><span class="global" id="global_DBG"></span><input type="number" id="draft_DBG" step="1" min="0" max="9"></div>
</div>
</div><hr>
<h1 class="collapsible-header" data-section="limits">Limits <span class="arrow">▼</span></h1>
<div class="collapsible-section" id="limits">
<div class="field-group">
<div class="field"><label>Vout Min:</label><span class="global" id="global_VoutMin"></span><input type="number" id="draft_VoutMin" step="0.1"></div>
<div class="field"><label>Vout Max:</label><span class="global" id="global_VoutMax"></span><input type="number" id="draft_VoutMax" step="0.1"></div>
<div class="field"><label>Ilimit Max:</label><span class="global" id="global_IlimitMax"></span><input type="number" id="draft_IlimitMax" step="0.1"></div>
<div class="field"><label>Power Max:</label><span class="global" id="global_PowerMax"></span><input type="number" id="draft_PowerMax" step="0.1"></div>
<div class="field"><label>Temp Max:</label><span class="global" id="global_TempMax"></span><input type="number" id="draft_TempMax" step="1"></div>
<div class="field"><label>Temp. Hyst.:</label><span class="global" id="global_TempDiff"></span><input type="number" id="draft_TempDiff" step="0.1" min="0.1" max="10"></div>
<div class="field"><label>Voltage Dev. V:</label><span class="global" id="global_VdevLimit"></span><input type="number" id="draft_VdevLimit" step="0.1" min="0" max="10"></div>
<div class="field"><label>Current Dev. %:</label><span class="global" id="global_IdevLimit"></span><input type="number" id="draft_IdevLimit" step="1" min="0" max="100"></div>
</div>
</div>
</div>
<script>
let ws;
let reconnectInterval = 1000;
const maxReconnect = 30000;
const pageName = "settings";
let initialized = false;
const fieldPrecision = {Kp: 2, Ki: 3, Kd: 3, IntegralLimit: 1, Kp_I: 2, Ki_I: 3, Kd_I: 3, IntegralLimit_I: 1, DutyMin: 1, DutyMax: 1, VoutMin: 1, VoutMax: 1, IlimitMax: 1, PowerMax: 1, TempMax: 1, TempDiff: 1, HUE: 0, VdevLimit: 1,IdevLimit: 1, DBG: 0};
const fields = ['WiFiSSID','WiFiPass','HUE','WiFiEnabled','OTAEnabled','InvertPWM','Kp','Ki','Kd','IntegralLimit','DutyMin','Kp_I','Ki_I','Kd_I','IntegralLimit_I','DutyMax','VoutMin','VoutMax','IlimitMax','PowerMax','TempMax','TempDiff','VdevLimit','IdevLimit','DBG'];
const expertFields = ['WiFiEnabled','OTAEnabled','InvertPWM','Kp','Ki','Kd','IntegralLimit','Kp_I','Ki_I','Kd_I','IntegralLimit_I','DutyMin','DutyMax','VoutMin','VoutMax','IlimitMax','PowerMax','TempMax','TempDiff','DBG'];
const errorMap = ["Overheat","Overcurrent","Fuse Blown","Sensor Fail","INA226 Init Fail","WiFi Init Fail","SSD1306 Init Fail","PWM Init Fail","Vout Over Limit","Over Power","Voltage Deviation",
  "Current Deviation","Power Over Limit","LEDC Init Fail","PID Divergence","Low Memory","High CPU Temp","Current PID Div"];
let globals = {HUE: 85, TempDiff: 5.0};
let hueTimeout;
let errorLog = [];
function connectWS() {ws = new WebSocket("ws://" + location.hostname + "/ws");ws.onopen = () => {reconnectInterval = 1000;sendOpen();errorLog = []};
  ws.onclose = () => {setTimeout(connectWS, reconnectInterval);reconnectInterval = Math.min(reconnectInterval * 2, maxReconnect);};
  ws.onerror = () => {ws.close();};
  ws.onmessage = e => {const obj = JSON.parse(e.data);updateGlobals(obj);if ('ERR' in obj) updateErrorLog(obj.ERR);if (!initialized) {setDraftsFromGlobals();initialized = true;}updateApplyButton();};
  setInterval(() => {if (ws && ws.readyState === WebSocket.OPEN) {sendOpen();}}, 5000);}
function sendOpen() {if (ws && ws.readyState === WebSocket.OPEN) {ws.send(JSON.stringify({ page: pageName, action: "OPEN" }));}}
function validateDraftValue(field, value) {
  if(['Kp','Ki','Kd','IntegralLimit','Kp_I','Ki_I','Kd_I','IntegralLimit_I','DutyMin','DutyMax','VoutMin','VoutMax','IlimitMax','PowerMax','TempMax','TempDiff','HUE','VdevLimit','IdevLimit'].includes(field)) {
  const num = parseFloat(value);if (isNaN(num)) return false;if (field === 'TempDiff') return num >= 0.1 && num <= 10;return true;}
  if (field === 'DBG') {const num = parseInt(value);return !isNaN(num) && num >= 0 && num <= 9;}return true;}
function updateGlobals(obj) {
  fields.forEach(field => {
  if (!(field in obj)) return;
  globals[field] = obj[field];
  const isBool = ['WiFiEnabled','OTAEnabled','InvertPWM'].includes(field);
  const globalSpan = document.getElementById(`global_${field}`);
  if (globalSpan) {
  globalSpan.innerText = isBool ? (globals[field] ? 'Yes' : 'No') : (field in fieldPrecision ? Number(globals[field]).toFixed(fieldPrecision[field]) : globals[field]);}
  if (field === 'HUE') {document.getElementById("draft_HUE").value = globals[field];if (!initialized) {document.getElementById("hueSlider").value = globals[field];
  document.documentElement.style.setProperty('--h', globals[field]);}}});updateApplyButton();}
function setDraftsFromGlobals() {
  fields.forEach(field => {
  const input = document.getElementById(`draft_${field}`);
  if (!input) return;
  if (['WiFiEnabled','OTAEnabled','InvertPWM'].includes(field)) input.checked = !!globals[field];
  else if (['WiFiSSID','WiFiPass'].includes(field)) input.value = globals[field] || '';
  else input.value = field in fieldPrecision ? Number(globals[field]).toFixed(fieldPrecision[field]) : globals[field] || '';});updateApplyButton();}
function getDraftValue(field) {
  const input = document.getElementById(`draft_${field}`);
  if (['WiFiEnabled','OTAEnabled','InvertPWM'].includes(field)) return input.checked ? 1 : 0;if (field === 'DBG') return parseInt(input.value);
  if (['Kp','Ki','Kd','IntegralLimit','Kp_I','Ki_I','Kd_I','IntegralLimit_I','DutyMin','DutyMax','VoutMin','VoutMax','IlimitMax','PowerMax','TempMax','TempDiff','HUE','VdevLimit','IdevLimit'].includes(field)) return parseFloat(input.value);
  return input.value;}
function updateGlobalDisplay(field) {const globalSpan = document.getElementById(`global_${field}`);
  if (globalSpan) globalSpan.innerText = ['WiFiEnabled','OTAEnabled','InvertPWM'].includes(field) ? (globals[field] ? 'Yes' : 'No') : (field in fieldPrecision ? Number(globals[field]).toFixed(fieldPrecision[field]) : globals[field]);}
function updateApplyButton() {
  let hasChanges = false;
  for (const field of fields) {
  const dval = getDraftValue(field), gval = globals[field];
  if (['WiFiEnabled','OTAEnabled','InvertPWM'].includes(field)) {
  if (validateDraftValue(field, dval) && dval !== (gval ? 1 : 0)) { hasChanges = true; break; }} else if (field in fieldPrecision) {
  if (validateDraftValue(field, dval) && Number(dval).toFixed(fieldPrecision[field]) !== Number(gval).toFixed(fieldPrecision[field])) { hasChanges = true; break; }
  } else if (validateDraftValue(field, dval) && dval !== gval) { hasChanges = true; break; }}document.getElementById("btnApply").classList.toggle("active", hasChanges);}
document.getElementById("btnApply").addEventListener("click", () => {
  const changes = {};
  fields.forEach(field => {
  const dval = getDraftValue(field), gval = globals[field];
  if (['WiFiEnabled','OTAEnabled','InvertPWM'].includes(field)) {
  if (validateDraftValue(field, dval) && dval !== (gval ? 1 : 0)) changes[field] = dval;
  } else if (field in fieldPrecision) {
  if (validateDraftValue(field, dval) && Number(dval).toFixed(fieldPrecision[field]) !== Number(gval).toFixed(fieldPrecision[field])) changes[field] = Number(dval);
  } else if (validateDraftValue(field, dval) && dval !== gval) changes[field] = dval;});
  if (Object.keys(changes).length) {ws.send(JSON.stringify(changes));
  Object.keys(changes).forEach(field => {globals[field] = changes[field];updateGlobalDisplay(field);});}updateApplyButton();});
expertFields.forEach(field => {const input = document.getElementById(`draft_${field}`);if (input && field !== 'HUE') {input.addEventListener("change", () => {
  if (document.getElementById("onlineMode").checked) {const value = getDraftValue(field);if (validateDraftValue(field, value)) {
  ws.send(JSON.stringify({ [field]: value }));globals[field] = value;updateGlobalDisplay(field);}}updateApplyButton();});}});
document.getElementById("toggleExpert").addEventListener("click", () => {const expertDiv = document.getElementById("expertSettings");
  const btn = document.getElementById("toggleExpert");expertDiv.classList.toggle("hidden");btn.innerText = expertDiv.classList.contains("hidden") ? "Show Expert" : "Hide Expert";btn.classList.toggle("active", !expertDiv.classList.contains("hidden"));});
document.getElementById("toggleErrors").addEventListener("click", () => {
  const statusDiv = document.getElementById("statusSection");const btn = document.getElementById("toggleErrors");
  if (statusDiv.classList.contains("visible")) {errorLog = [];statusDiv.classList.remove("visible");btn.innerText = "Show Errors";
  btn.classList.remove("active");} else {statusDiv.classList.add("visible");btn.innerText = "Hide Errors";btn.classList.add("active");}updateErrorLog(0);});
document.getElementById("hueSlider").addEventListener("input", () => {
  const value = document.getElementById("hueSlider").value;
  document.getElementById("draft_HUE").value = value;
  document.documentElement.style.setProperty('--h', value);
  clearTimeout(hueTimeout);
  hueTimeout = setTimeout(() => {
    const parsedValue = parseFloat(value);
    if (validateDraftValue('HUE', parsedValue)) {
      ws.send(JSON.stringify({ HUE: parsedValue }));globals['HUE'] = parsedValue;
      document.getElementById("global_HUE").innerText = parsedValue;}updateApplyButton();}, 300);});
document.querySelectorAll(".collapsible-header").forEach(header => {
  header.addEventListener("click", () => {
    const sectionId = header.getAttribute("data-section");
    const section = document.getElementById(sectionId);
    section.classList.toggle("collapsed");
    header.classList.toggle("collapsed");});});
function updateErrorLog(code) {for (let i = 0; i < errorMap.length; i++) {const isActive = (code >> i) & 1;
    const existing = errorLog.find(entry => entry.bit === i);if (isActive && !existing) {errorLog.push({ bit: i, active: true });
    } else if (existing && !isActive && existing.active) {existing.active = false;}}updateStatusDisplay();}
function updateStatusDisplay() {const listDiv = document.getElementById("statusList");const statusDiv = document.getElementById("statusSection");
  const toggleBtn = document.getElementById("toggleErrors");listDiv.innerHTML = "";const hasLoggedErrors = errorLog.length > 0;
  const hasActiveErrors = errorLog.some(entry => entry.active);const statusClass = hasActiveErrors ? "status-error" : "status-normal";
  if (!hasLoggedErrors) {listDiv.innerHTML = "<div class='no-status'>No errors</div>";} else {for (let i = 0; i < errorMap.length; i++) {const entry = errorLog.find(e => e.bit === i);
  if (entry) {const className = entry.active ? "error-active" : "error-inactive";listDiv.innerHTML += `<div class="${className}">${errorMap[i]}</div>`;}}}listDiv.scrollTop = listDiv.scrollHeight;
  statusDiv.classList.remove("status-error", "status-normal");listDiv.classList.remove("status-error", "status-normal");statusDiv.classList.add(statusClass);listDiv.classList.add(statusClass);
  if (hasLoggedErrors && !statusDiv.classList.contains("visible")) {statusDiv.classList.add("visible");toggleBtn.innerText = "Hide Errors";toggleBtn.classList.add("active");}}
connectWS();
document.querySelectorAll('.collapsible-section, .collapsible-header').forEach(el => el.classList.add('collapsed'));
</script>
</body>
</html>
)rawliteral";



static AsyncWebServer server(80);
static AsyncWebSocket ws("/ws");

namespace WebInterface {

struct PageState {
    bool active = false;
    unsigned long lastOpen = 0;
};
std::map<String, PageState> pages;
const unsigned long PAGE_TIMEOUT = 10000;

void handlePageOpen(const String &pageName) {
    pages[pageName].active = true;
    pages[pageName].lastOpen = millis();
}

void begin() {
    // Wi-Fi setup route (always available)
    server.on("/wifi-setup", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", wifi_setup_html);
    });

    // Handle Wi-Fi settings save
    server.on("/save-wifi", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        StaticJsonDocument<256> doc;
        DeserializationError error = deserializeJson(doc, data, len);

        if (!error && doc.containsKey("WiFiSSID") && doc["WiFiSSID"].as<String>().length() > 0) {
            strncpy(wifiSSID, doc["WiFiSSID"], sizeof(wifiSSID) - 1);
            wifiSSID[sizeof(wifiSSID) - 1] = '\0';
            strncpy(wifiPass, doc["WiFiPass"], sizeof(wifiPass) - 1);
            wifiPass[sizeof(wifiPass) - 1] = '\0';
            apMode = false;

            Serial.printf("WiFi settings updated: SSID=%s, Pass=%s\n", wifiSSID, wifiPass);

            String response = "{\"status\":\"ok\",\"ssid\":\"" + String(wifiSSID) +
                              "\",\"pass\":\"" + String(wifiPass) + "\"}";
            request->send(200, "application/json", response);
        } else {
            request->send(400, "application/json",
                          "{\"status\":\"error\",\"error\":\"Invalid or missing WiFiSSID\"}");
        }
    });


    // Routes for STA mode
    if (!apMode) {
        server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
            if (index_html) {
                request->send_P(200, "text/html", index_html);
            } else {
                request->send(404, "text/plain", "File not found");
            }
        });
        server.on("/wifi", HTTP_GET, [](AsyncWebServerRequest *request) {
            String info = "Wi-Fi: " + WiFi.SSID() + " (" + String(WiFi.RSSI()) + " dBm)";
            request->send(200, "text/plain", info);
        });
        server.on("/charts", HTTP_GET, [](AsyncWebServerRequest *request) {
            if (charts_html) {
                request->send_P(200, "text/html", charts_html);
            } else {
                request->send(404, "text/plain", "File not found");
            }
        });
        server.on("/settings", HTTP_GET, [](AsyncWebServerRequest *request) {
            if (settings_html) {
                request->send_P(200, "text/html", settings_html);
            } else {
                request->send(404, "text/plain", "File not found");
            }
        });
        server.on("/system", HTTP_GET, [](AsyncWebServerRequest *request) {
            if (system_html) {
                request->send_P(200, "text/html", system_html);
            } else {
                request->send(404, "text/plain", "File not found");
            }
        });
        server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
            if (style_css) {
                request->send_P(200, "text/css", style_css);
            } else {
                request->send(404, "text/plain", "File not found");
            }
        });

        // WebSocket event
        ws.onEvent([&](AsyncWebSocket *server, AsyncWebSocketClient *client,
                       AwsEventType type, void *arg, uint8_t *data, size_t len) {
            if (type != WS_EVT_DATA) return;
            AwsFrameInfo *info = (AwsFrameInfo *)arg;
            if (!info->final || info->index != 0 || info->len != len || info->opcode != WS_TEXT) return;

            String msg = String((char *)data).substring(0, len);
            msg.trim();
            StaticJsonDocument<128> doc;
            if (deserializeJson(doc, msg) != DeserializationError::Ok) return;

            // Page open
            if (doc.containsKey("action") && doc["action"] == "OPEN" && doc.containsKey("page")) {
                String pageName = doc["page"].as<String>();
                pages[pageName].active = true;
                pages[pageName].lastOpen = millis();
                return;
            }

            // Debug control
            if (doc.containsKey("action")) {
                if (doc["action"] == "DEBUG_ON") debugEnabled = true;
                else if (doc["action"] == "DEBUG_OFF") debugEnabled = false;
            }

            // Live setpoints
            if (doc.containsKey("V")) labV_set = doc["V"];
            if (doc.containsKey("IL")) labI_set = doc["IL"];
            if (doc.containsKey("IF")) labI_cut = doc["IF"];
            if (doc.containsKey("MODE")) modeAuto = (String(doc["MODE"].as<const char *>()) == "auto");
            if (doc.containsKey("OUT")) manualOutputEnable = (String(doc["OUT"].as<const char *>()) == "1");
            if (doc.containsKey("HUE")) themeHue = doc["HUE"];

            // Settings
            if (doc.containsKey("Kp")) ::Kp = doc["Kp"];
            if (doc.containsKey("Ki")) ::Ki = doc["Ki"];
            if (doc.containsKey("Kd")) ::Kd = doc["Kd"];
            if (doc.containsKey("IntegralLimit")) ::integralLimit = doc["IntegralLimit"];
            if (doc.containsKey("Kp_I")) ::Kp_I = doc["Kp_I"];
            if (doc.containsKey("Ki_I")) ::Ki_I = doc["Ki_I"];
            if (doc.containsKey("Kd_I")) ::Kd_I = doc["Kd_I"];
            if (doc.containsKey("IntegralLimit_I")) ::integralLimit_I = doc["IntegralLimit_I"];
            if (doc.containsKey("DutyMin")) ::dutyMin = doc["DutyMin"];
            if (doc.containsKey("DutyMax")) ::dutyMax = doc["DutyMax"];
            if (doc.containsKey("InvertPWM")) ::invertPwmSignal = !!doc["InvertPWM"];
            if (doc.containsKey("WiFiEnabled")) ::wifiEnabled = !!doc["WiFiEnabled"];
            if (doc.containsKey("WiFiSSID")) strncpy(::wifiSSID, doc["WiFiSSID"], sizeof(::wifiSSID) - 1);
            if (doc.containsKey("WiFiPass")) strncpy(::wifiPass, doc["WiFiPass"], sizeof(::wifiPass) - 1);
            if (doc.containsKey("OTAEnabled")) ::otaEnabled = !!doc["OTAEnabled"];
            if (doc.containsKey("VoutMin")) ::systemVoutMin = doc["VoutMin"];
            if (doc.containsKey("VoutMax")) ::systemVoutMax = doc["VoutMax"];
            if (doc.containsKey("IlimitMax")) ::systemIlimitMax = doc["IlimitMax"];
            if (doc.containsKey("PowerMax")) ::systemPowerMax = doc["PowerMax"];
            if (doc.containsKey("TempMax")) ::tempLimitC = doc["TempMax"];
            if (doc.containsKey("TempDiff")) {
                float tempDiff = doc["TempDiff"];
                ::tempDiffC = constrain(tempDiff, 0.1f, 10.0f);
            }
            if (doc.containsKey("VdevLimit")) ::VdevLimit = doc["VdevLimit"];
            if (doc.containsKey("IdevLimit")) ::IdevLimit = doc["IdevLimit"].as<float>() / 100.0f;
            if (doc.containsKey("DBG")) {
                uint8_t mode = constrain(doc["DBG"], 0, 9);
                ::dbgMode = mode;
            }

            // Reboot
            if (doc.containsKey("Reboot") && doc["Reboot"]) ESP.restart();
        });

        server.addHandler(&ws);
    } else {
        // Redirect all routes to /wifi-setup in AP mode
        server.onNotFound([](AsyncWebServerRequest *request) {
            request->redirect("/wifi-setup");
        });
    }

    server.begin();
}

void update() {
    static unsigned long lastSend = 0;
    if (apMode || (millis() - lastSend) < WEBSOCKET_SEND_INTERVAL) return;
    lastSend = millis();

    // Page timeout
    for (auto &kv : pages) {
        if ((millis() - kv.second.lastOpen) > PAGE_TIMEOUT) kv.second.active = false;
    }

    StaticJsonDocument<2048> doc;
    // Live data
    doc["V"] = labV_meas;
    doc["I"] = labI_meas;
    doc["Q"] = labQ_meas;
    doc["VSET"] = labV_set;
    doc["IL"] = labI_set;
    doc["IF"] = labI_cut;
    doc["MODE"] = modeAuto ? "auto" : "manual";
    doc["OUT"] = manualOutputEnable ? "1" : "0";
    doc["TEMP"] = labTemp_ntc;
    doc["HUE"] = themeHue;
    doc["WIFI_SSID"] = WiFi.SSID();
    doc["WIFI_RSSI"] = WiFi.RSSI();

    // Page flags
    doc["PAGE_CHARTS"] = pages["charts"].active ? 1 : 0;
    doc["PAGE_SETTINGS"] = pages["settings"].active ? 1 : 0;
    doc["PAGE_SYSTEM"] = pages["system"].active ? 1 : 0;

    // Error code
    doc["ERR"] = errorCode;

    // Debug data
    if (debugEnabled) {
        JsonObject debug = doc.createNestedObject("debug");
        debug["Dbg0"] = debugVars[0];
        debug["Dbg1"] = debugVars[1];
        debug["Dbg2"] = debugVars[2];
        debug["Dbg3"] = debugVars[3];
        debug["Dbg4"] = debugVars[4];
        debug["Dbg5"] = debugVars[5];
        debug["enabled"] = debugEnabled;
    }

    // Active settings page
    if (pages["settings"].active) {
        doc["Kp"] = ::Kp;
        doc["Ki"] = ::Ki;
        doc["Kd"] = ::Kd;
        doc["IntegralLimit"] = ::integralLimit;
        doc["Kp_I"] = ::Kp_I;
        doc["Ki_I"] = ::Ki_I;
        doc["Kd_I"] = ::Kd_I;
        doc["IntegralLimit_I"] = ::integralLimit_I;
        doc["DutyMin"] = ::dutyMin;
        doc["DutyMax"] = ::dutyMax;
        doc["InvertPWM"] = ::invertPwmSignal;
        doc["WiFiEnabled"] = ::wifiEnabled;
        doc["WiFiSSID"] = ::wifiSSID;
        doc["WiFiPass"] = ::wifiPass;
        doc["OTAEnabled"] = ::otaEnabled;
        doc["VoutMin"] = ::systemVoutMin;
        doc["VoutMax"] = ::systemVoutMax;
        doc["IlimitMax"] = ::systemIlimitMax;
        doc["PowerMax"] = ::systemPowerMax;
        doc["TempMax"] = ::tempLimitC;
        doc["TempDiff"] = ::tempDiffC;
        doc["VdevLimit"] = ::VdevLimit;
        doc["IdevLimit"] = ::IdevLimit * 100;
        doc["DBG"] = ::dbgMode;
    }

    // Send to clients
    String output;
    serializeJson(doc, output);
    ws.textAll(output);
}

} // namespace WebInterface
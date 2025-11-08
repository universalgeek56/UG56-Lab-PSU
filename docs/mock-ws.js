// === Mock WebSocket for GitHub Pages demo ===
// Emulates PSU behavior, ramping, CC, thermal and fuse cut
console.log("UG56 Lab PSU — Demo mode active (mock-ws.js)");

class MockWebSocket {
  constructor(url) {
    this.url = url;
    this.readyState = 1;
    this.onmessage = null;
    this.onopen = null;
    this.onerror = null;
    this._interval = null;
    this._state = this._defaultState();

    setTimeout(() => this.onopen && this.onopen({ type: "open" }), 200);
    this._interval = setInterval(() => this._tick(), 500);
  }

  send(msg) {
    try {
      const data = JSON.parse(msg);
      this._handleCommand(data);
    } catch (e) {
      console.warn("MockWS invalid JSON:", msg);
    }
  }

  close() {
    clearInterval(this._interval);
    this.readyState = 3;
  }

  _defaultState() {
    return {
      V: 0,
      I: 0,
      Q: 0,
      TEMP: 25,
      VSET: 5,
      IL: 2,
      IF: 2,
      MODE: "auto",
      OUT: "0",
      ERR: 0,
      PAGE_CHARTS: 1,
      PAGE_SETTINGS: 0,
      PAGE_SYSTEM: 0,
      WIFI_SSID: "DemoNet",
      WIFI_RSSI: -52,
      HUE: 180,
      fuseBlown: false,
      rampedVset: 5,
      rampedIset: 2,
    };
  }

  _handleCommand(cmd) {
    const s = this._state;
    if (cmd.V !== undefined) s.VSET = cmd.V;
    if (cmd.IL !== undefined) s.IL = cmd.IL;
    if (cmd.IF !== undefined) s.IF = cmd.IF;
    if (cmd.MODE !== undefined) s.MODE = cmd.MODE;
    if (cmd.OUT !== undefined) {
      if (cmd.OUT === "1") {
        if (s.fuseBlown) {
          s.fuseBlown = false;
          s.ERR = 0;
        }
        s.OUT = "1";
      } else {
        s.OUT = "0";
      }
    }
    if (cmd.HUE !== undefined) s.HUE = cmd.HUE;
  }

  _tick() {
    const s = this._state;
    const loadR = 10.0;
    const noise = (x) => x * (1 + (Math.random() - 0.5) * 0.02);

    if (s.OUT === "1" && !s.fuseBlown) {
      // плавное применение уставок
      s.rampedVset += (s.VSET - s.rampedVset) * 0.1;
      s.rampedIset += (s.IL - s.rampedIset) * 0.1;

      let V = s.rampedVset;
      let I = V / loadR;

      // ограничение по току
      if (I > s.rampedIset) {
        I = s.rampedIset;
        V = I * loadR;
      }

      // “предохранитель”
      if (I > s.IF) {
        s.fuseBlown = true;
        s.OUT = "0";
        s.ERR = 4; // произвольный код fuse blown
      }

      s.V = noise(V);
      s.I = noise(I);
      s.Q = s.V * s.I;
      s.TEMP = noise(25 + s.Q * 0.5 + Math.random() * 2);
    } else {
      // выход выключен
      s.V = 0;
      s.I = 0;
      s.Q = 0;
      s.TEMP = Math.max(25, s.TEMP - 0.5);
    }

    const msg = JSON.stringify({
      V: +s.V.toFixed(2),
      I: +s.I.toFixed(3),
      Q: +s.Q.toFixed(2),
      TEMP: +s.TEMP.toFixed(1),
      VSET: +s.VSET.toFixed(2),
      IL: +s.IL.toFixed(2),
      IF: +s.IF.toFixed(2),
      MODE: s.MODE,
      OUT: s.OUT,
      ERR: s.ERR,
      HUE: s.HUE,
      WIFI_SSID: s.WIFI_SSID,
      WIFI_RSSI: s.WIFI_RSSI,
      PAGE_CHARTS: s.PAGE_CHARTS,
      PAGE_SETTINGS: s.PAGE_SETTINGS,
      PAGE_SYSTEM: s.PAGE_SYSTEM,
    });

    this.onmessage && this.onmessage({ data: msg });
  }
}

window.WebSocket = MockWebSocket;
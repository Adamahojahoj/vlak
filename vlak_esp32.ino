#include <WiFi.h>
#include <WebServer.h>

// ==========================================
// Piny pro L298N H-Můstek (Motory)
// ==========================================
// Motor A
const int ENA = 14; 
const int IN1 = 27;
const int IN2 = 26;

// Motor B
const int ENB = 32;
const int IN3 = 33;
const int IN4 = 25;

// ==========================================
// Piny pro LED (světla vlaku)
// ==========================================
const int LED1_PIN = 12;
const int LED2_PIN = 13;

WebServer server(80);

// ==========================================
// Nastavení WiFi AP
// ==========================================
const char* jmenoWifi = "Vlak_Ovladani";
const char* hesloWifi = "12345678"; // Musí mít alespoň 8 znaků!

// Nastavení rychlosti a zpomalování (Ramping)
int targetSpeed = 0;      // Cílová rychlost z ovládání (-255 až 255)
float currentSpeed = 0;   // Aktuální PWM rychlost zapsaná do motorů
unsigned long lastRampTime = 0;

// Webová stránka (HTML + CSS + JS) zapsaná přímo do paměti ESP32
const char html_page[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="cs">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
    <title>Ovládání Vlaku</title>
    <style>
        :root {
            --bg-color: #0f172a;
            --panel-bg: #1e293b;
            --text-color: #f8fafc;
            --accent: #3b82f6;
            --danger: #ef4444;
            --warning: #f59e0b;
        }

        body {
            margin: 0;
            padding: 0;
            background-color: var(--bg-color);
            color: var(--text-color);
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
            min-height: 100vh;
            touch-action: pan-y;
        }

        .container {
            background-color: var(--panel-bg);
            padding: 2rem;
            border-radius: 1.5rem;
            box-shadow: 0 10px 25px rgba(0,0,0,0.5);
            width: 90%;
            max-width: 400px;
            text-align: center;
        }

        h1 {
            margin-top: 0;
            font-size: 1.8rem;
            margin-bottom: 1.5rem;
            font-weight: 700;
            text-transform: uppercase;
        }

        .slider-container {
            margin: 2.5rem 0;
            position: relative;
        }

        input[type=range] {
            -webkit-appearance: none;
            width: 100%;
            background: transparent;
        }

        input[type=range]::-webkit-slider-thumb {
            -webkit-appearance: none;
            height: 60px;
            width: 60px;
            border-radius: 50%;
            background: var(--accent);
            cursor: pointer;
            margin-top: -24px;
            box-shadow: 0 0 15px rgba(59, 130, 246, 0.6);
            border: 4px solid #fff;
            transition: transform 0.1s;
        }
        
        input[type=range]::-webkit-slider-thumb:active {
            transform: scale(1.1);
        }

        input[type=range]::-webkit-slider-runnable-track {
            width: 100%;
            height: 12px;
            background: linear-gradient(90deg, #ef4444 0%, #475569 50%, #10b981 100%);
            border-radius: 6px;
        }

        .speed-display {
            font-size: 3.5rem;
            font-weight: 800;
            margin-bottom: 0.2rem;
            font-variant-numeric: tabular-nums;
            text-shadow: 0 2px 10px rgba(0,0,0,0.3);
        }

        .speed-label {
            font-size: 1.2rem;
            color: #94a3b8;
            margin-bottom: 1rem;
        }

        .btn-stop {
            background-color: var(--danger);
            color: white;
            border: none;
            padding: 1rem;
            border-radius: 1rem;
            font-size: 1.3rem;
            font-weight: 700;
            cursor: pointer;
            width: 100%;
            margin-bottom: 1rem;
            box-shadow: 0 4px 6px rgba(239, 68, 68, 0.3);
            text-transform: uppercase;
        }

        .btn-stop:active {
            background-color: #dc2626;
            transform: scale(0.98);
        }

        .btn-lights {
            background-color: #475569;
            color: white;
            border: none;
            padding: 1rem;
            border-radius: 1rem;
            font-size: 1.2rem;
            font-weight: 600;
            cursor: pointer;
            width: 100%;
            display: flex;
            align-items: center;
            justify-content: center;
            gap: 10px;
            transition: background-color 0.3s;
        }
        
        .btn-lights.active {
            background-color: var(--warning);
            color: #000;
            box-shadow: 0 0 15px rgba(245, 158, 11, 0.6);
        }
        
        .btn-lights:active {
            transform: scale(0.98);
        }

        .status {
            margin-top: 1.5rem;
            font-size: 0.9rem;
            color: #64748b;
        }
    </style>
</head>
<body>

    <div class="container">
        <h1>🚂 Vlak Ovládání</h1>
        
        <div class="speed-display" id="speedText">0 %</div>
        <div class="speed-label" id="directionText">Stojí</div>

        <div class="slider-container">
            <input type="range" id="speedSlider" min="-100" max="100" value="0">
        </div>

        <button class="btn-stop" onclick="stopTrain()">🛑 Zastavit</button>
        <button class="btn-lights" id="lightBtn" onclick="toggleLights()">💡 Světla (Zapnout)</button>
        
        <div class="status" id="status">Připojeno k vlaku</div>
    </div>

    <script>
        const slider = document.getElementById('speedSlider');
        const speedText = document.getElementById('speedText');
        const directionText = document.getElementById('directionText');
        const lightBtn = document.getElementById('lightBtn');
        const statusEl = document.getElementById('status');

        let lightsOn = false;
        let lastSendTime = 0;

        function updateSpeedDisplay(val) {
            speedText.innerText = Math.abs(val) + " %";
            if (val > 0) {
                directionText.innerText = "Jízda Vpřed ⏫";
                directionText.style.color = "#10b981";
            } else if (val < 0) {
                directionText.innerText = "Jízda Vzad ⏬";
                directionText.style.color = "#ef4444";
            } else {
                directionText.innerText = "Stojí ⏸";
                directionText.style.color = "#94a3b8";
            }
        }

        function sendSpeed(val) {
            fetch('/speed?v=' + val)
            .then(res => {
                if(res.ok) statusEl.innerText = "Signál OK";
            }).catch(e => {
                statusEl.innerText = "Chyba signálu!";
                statusEl.style.color = "#ef4444";
            });
        }

        slider.addEventListener('input', (e) => {
            updateSpeedDisplay(e.target.value);
            const now = Date.now();
            if(now - lastSendTime > 150) { // Omezíme posílání na max každých 150ms aby nepadal server
                sendSpeed(e.target.value);
                lastSendTime = now;
            }
        });

        slider.addEventListener('change', (e) => {
            updateSpeedDisplay(e.target.value);
            sendSpeed(e.target.value);
        });

        function stopTrain() {
            slider.value = 0;
            updateSpeedDisplay(0);
            sendSpeed(0);
        }

        function toggleLights() {
            lightsOn = !lightsOn;
            if (lightsOn) {
                lightBtn.classList.add('active');
                lightBtn.innerHTML = '💡 Světla (Vypnout)';
            } else {
                lightBtn.classList.remove('active');
                lightBtn.innerHTML = '💡 Světla (Zapnout)';
            }
            fetch('/led?state=' + (lightsOn ? 1 : 0));
        }
    </script>
</body>
</html>
)rawliteral";


//===========================================
// Obsluha Serveru
//===========================================
void handleRoot() {
  server.send(200, "text/html", html_page);
}

void handleSpeed() {
  if (server.hasArg("v")) {
    int percentage = server.arg("v").toInt();
    
    // Omezení rozsahu -100 až 100
    if (percentage > 100) percentage = 100;
    if (percentage < -100) percentage = -100;
    
    // Konverze procent na PWM (-255 až +255)
    targetSpeed = map(percentage, -100, 100, -255, 255);
    
    server.send(200, "text/plain", "OK");
  } else {
    server.send(400, "text/plain", "Chyba pozadavku");
  }
}

void handleLed() {
  if(server.hasArg("state")) {
    int state = server.arg("state").toInt();
    digitalWrite(LED1_PIN, state ? HIGH : LOW);
    digitalWrite(LED2_PIN, state ? HIGH : LOW);
    server.send(200, "text/plain", "OK");
  } else {
    server.send(400, "text/plain", "Chyba pozadavku");
  }
}

void setup() {
  Serial.begin(115200);

  // Nastavení pinů pro motory
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  
  // Přidáme i ENA a ENB na OUTPUT (nutné kvůli analogWrite u některých modelů)
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);
  
  // Nastavení pinů pro LED světla
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);

  // Vytvoření přístupového bodu (WiFi bez internetu jen pro vlak)
  Serial.println("\nVytvarim WiFi sít...");
  WiFi.softAP(jmenoWifi, hesloWifi);
  
  IPAddress IP = WiFi.softAPIP();
  Serial.print("Vlak je pripraven! Pripojte se na WiFi: ");
  Serial.println(jmenoWifi);
  Serial.print("A otevrete v prohlizeci adresu: http://");
  Serial.println(IP);

  // Namapování odkazů na funkce
  server.on("/", handleRoot);
  server.on("/speed", handleSpeed);
  server.on("/led", handleLed);

  server.begin();
  Serial.println("Webovy server bezi.");
}

void loop() {
  // Obslouží čekající webové dotazy
  server.handleClient();
  
  // ===================================
  // RAMPING: Plynulé zpomalení/zrychlení
  // ===================================
  // Každých 5ms zkusíme pohnout rychlostí o kousíček směrem k cíli (targetSpeed).
  // Díky tomu se motory nikdy nezaseknou z prudké změny 100->0.
  if (millis() - lastRampTime > 5) {
    if (currentSpeed < targetSpeed) {
      currentSpeed += 1.5; // Zrychlení (Změnou hodnoty 1.5 upravíte jak prudce brzdí)
      if (currentSpeed > targetSpeed) currentSpeed = targetSpeed;
    } else if (currentSpeed > targetSpeed) {
      currentSpeed -= 1.5; // Brždění
      if (currentSpeed < targetSpeed) currentSpeed = targetSpeed;
    }
    
    lastRampTime = millis();
    
    // Aplikujeme aktuální vyhlazenou PWM
    applyMotorSpeed((int)currentSpeed);
  }
}

// Funkce co reálně zapisuje hodnoty do pinů
void applyMotorSpeed(int speedPwm) {
  if (speedPwm > 0) {
    // Jízda Vpřed
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
    
    analogWrite(ENA, speedPwm);
    analogWrite(ENB, speedPwm);
    
  } else if (speedPwm < 0) {
    // Jízda Vzad
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
    
    analogWrite(ENA, -speedPwm); // Absolutní hodnota (vždy kladné PWM)
    analogWrite(ENB, -speedPwm);
    
  } else {
    // Stojí
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
    
    analogWrite(ENA, 0);
    analogWrite(ENB, 0);
  }
}

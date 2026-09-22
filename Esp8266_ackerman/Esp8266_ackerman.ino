// ============================================================
// ROBOT ACKERMAN — ESP8266 (FINAL v5)
// Mode: Access Point
// SSID    : RobotAckerman | Password: robot1234
// IP      : 192.168.4.1
// Steering: Center=75, Kiri=60, Kanan=110
// Benih   : Tutup=34°, Buka=95°
// ============================================================

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

const char* ap_ssid     = "RobotAckerman";
const char* ap_password = "robot1234";

ESP8266WebServer server(80);

// --- DATA DARI MEGA ---
long enc1 = 0, enc2 = 0;
int  rpm1 = 0, rpm2 = 0;
int  servoAngle = 75;
int  stepperPos = 0;
int  benihAngle = 45;
bool seqRunning = false;

// --- PARSE DATA ---
// Format: DATA,enc1,enc2,rpm1,rpm2,servo,stepper,benih,seq
void parseData(String line) {
  if (!line.startsWith("DATA,")) return;
  line = line.substring(5);
  int p1 = line.indexOf(',');
  int p2 = line.indexOf(',', p1+1);
  int p3 = line.indexOf(',', p2+1);
  int p4 = line.indexOf(',', p3+1);
  int p5 = line.indexOf(',', p4+1);
  int p6 = line.indexOf(',', p5+1);
  int p7 = line.indexOf(',', p6+1);
  if (p1<0||p2<0||p3<0||p4<0||p5<0||p6<0||p7<0) return;
  enc1       = line.substring(0,   p1).toInt();
  enc2       = line.substring(p1+1,p2).toInt();
  rpm1       = line.substring(p2+1,p3).toInt();
  rpm2       = line.substring(p3+1,p4).toInt();
  servoAngle = line.substring(p4+1,p5).toInt();
  stepperPos = line.substring(p5+1,p6).toInt();
  benihAngle = line.substring(p6+1,p7).toInt();
  seqRunning = line.substring(p7+1).toInt();
}

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="id">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Robot Ackerman</title>
<style>
  :root{
    --bg:#0f1117;--card:#1a1d27;--border:#2a2d3e;
    --accent:#4f8ef7;--accent2:#38d9a9;--warn:#ffd43b;
    --text:#e2e8f0;--muted:#64748b;--red:#f87171;--green:#4ade80;
    --purple:#c084fc;--orange:#fb923c;
  }
  *{box-sizing:border-box;margin:0;padding:0}
  body{background:var(--bg);color:var(--text);font-family:'Segoe UI',monospace;min-height:100vh}
  header{padding:16px 24px;border-bottom:1px solid var(--border);display:flex;align-items:center;gap:12px;background:var(--card)}
  .dot{width:10px;height:10px;border-radius:50%;background:var(--green);animation:pulse 2s infinite}
  .dot.busy{background:var(--orange)}
  @keyframes pulse{0%,100%{opacity:1}50%{opacity:.4}}
  h1{font-size:18px;font-weight:600}
  .tag{font-size:11px;color:var(--muted);margin-left:auto}
  .seq-badge{font-size:11px;padding:4px 10px;border-radius:20px;background:var(--orange);color:#000;font-weight:700;display:none}
  .seq-badge.show{display:inline-block}
  .grid{display:grid;grid-template-columns:1fr 1fr;gap:16px;padding:20px;max-width:960px;margin:0 auto}
  @media(max-width:600px){.grid{grid-template-columns:1fr}}
  .card{background:var(--card);border:1px solid var(--border);border-radius:12px;padding:18px}
  .card.full{grid-column:span 2}
  @media(max-width:600px){.card.full{grid-column:span 1}}
  .card-title{font-size:11px;color:var(--muted);text-transform:uppercase;letter-spacing:1px;margin-bottom:12px}
  .enc-row{display:flex;justify-content:space-between;margin-bottom:8px}
  .enc-label{font-size:12px;color:var(--muted)}
  .enc-val{font-size:22px;font-weight:700;font-family:monospace;color:var(--accent)}
  .enc-rpm{font-size:12px;color:var(--accent2);margin-top:2px}
  .btn-grid{display:grid;grid-template-columns:1fr 1fr 1fr;gap:8px;margin-bottom:12px}
  .btn{padding:10px;border:1px solid var(--border);border-radius:8px;background:var(--bg);color:var(--text);cursor:pointer;font-size:13px;transition:background .15s;touch-action:manipulation}
  .btn:hover,.btn:active{background:var(--accent);border-color:var(--accent);color:#fff}
  .btn:disabled{opacity:.4;cursor:not-allowed}
  .btn.stop{border-color:var(--red);color:var(--red);grid-column:span 3}
  .btn.stop:hover:not(:disabled){background:var(--red);color:#fff}
  .pwm-row{display:flex;align-items:center;gap:10px;margin-top:8px}
  .pwm-row label{font-size:12px;color:var(--muted);min-width:36px}
  input[type=range]{flex:1;accent-color:var(--accent)}
  input[type=range]:disabled{opacity:.4}
  .pwm-val{font-size:13px;font-weight:600;min-width:32px;text-align:right}
  .servo-wrap{text-align:center}
  .servo-angle-label{font-size:28px;font-weight:700;color:var(--accent2);margin-bottom:12px;display:block}
  input[type=range].servo-slider{width:100%;accent-color:var(--accent2)}
  .benih-angle-label{font-size:24px;font-weight:700;color:var(--purple);margin-bottom:8px;display:block;text-align:center}
  input[type=range].benih-slider{width:100%;accent-color:var(--purple)}
  .stepper-val{font-size:28px;font-weight:700;color:var(--orange);text-align:center;margin:6px 0 10px}
  input[type=range].stepper-slider{width:100%;accent-color:var(--orange)}
  .stepper-labels{display:flex;justify-content:space-between;font-size:11px;color:var(--muted);margin-top:4px}
  .row-btns{display:flex;gap:8px;margin-top:10px}
  .row-btns .btn{flex:1;font-size:12px}
  .btn.orange{border-color:var(--orange);color:var(--orange)}
  .btn.orange:hover:not(:disabled){background:var(--orange);color:#000}
  .btn.purple{border-color:var(--purple);color:var(--purple)}
  .btn.purple:hover:not(:disabled){background:var(--purple);color:#000}
  .tanam-btn{
    width:100%;padding:16px;border-radius:12px;
    background:linear-gradient(135deg,#16a34a,#15803d);
    border:none;color:#fff;font-size:16px;font-weight:700;
    cursor:pointer;letter-spacing:1px;transition:opacity .2s,transform .1s;
    touch-action:manipulation;
  }
  .tanam-btn:hover:not(:disabled){opacity:.85;transform:scale(1.01)}
  .tanam-btn:disabled{opacity:.4;cursor:not-allowed;background:#333}
  .tanam-status{text-align:center;font-size:12px;color:var(--orange);margin-top:8px;min-height:18px}
  .note{font-size:11px;color:var(--muted);text-align:center;margin-top:8px}
  .status{text-align:center;font-size:11px;color:var(--muted);padding:10px}
  #last-update{color:var(--accent2)}
</style>
</head>
<body>
<header>
  <div class="dot" id="status-dot"></div>
  <h1>Robot Ackerman</h1>
  <span class="seq-badge" id="seq-badge">🌱 MENANAM...</span>
  <span class="tag">192.168.4.1</span>
</header>

<div class="grid">

  <!-- ENCODER -->
  <div class="card">
    <div class="card-title">Encoder Monitor</div>
    <div class="enc-row">
      <div>
        <div class="enc-label">Motor 1</div>
        <div class="enc-val" id="e1">0</div>
        <div class="enc-rpm" id="r1">0 RPM</div>
      </div>
      <div style="text-align:right">
        <div class="enc-label">Motor 2</div>
        <div class="enc-val" id="e2">0</div>
        <div class="enc-rpm" id="r2">0 RPM</div>
      </div>
    </div>
    <button class="btn" id="btn-reset-enc" style="width:100%;margin-top:8px"
      onclick="sendCmd('STOP');fetch('/reset_enc')">Reset Counter</button>
  </div>

  <!-- MOTOR -->
  <div class="card">
    <div class="card-title">Motor Drive</div>
    <div class="btn-grid">
      <div></div>
      <button class="btn" id="btn-maju" onclick="toggleDir('fwd')">▲ Maju</button>
      <div></div>
      <button class="btn" id="btn-kiri" onclick="toggleDir('lft')">◄ Kiri</button>
      <button class="btn stop" id="btn-stop" onclick="resetDir();go('stp')">STOP</button>
      <button class="btn" id="btn-kanan" onclick="toggleDir('rgt')">Kanan ►</button>
      <div></div>
      <button class="btn" id="btn-mundur" onclick="toggleDir('rev')">▼ Mundur</button>
      <div></div>
    </div>
    <div class="pwm-row">
      <label>PWM</label>
      <input type="range" id="pwm-slider" min="0" max="255" value="150"
        oninput="document.getElementById('pwm-disp').textContent=this.value">
      <span class="pwm-val" id="pwm-disp">150</span>
    </div>
  </div>

  <!-- SERVO STEERING -->
  <div class="card">
    <div class="card-title">Servo Steering — pin 6</div>
    <div class="servo-wrap">
      <svg width="140" height="70" viewBox="0 0 140 70" style="margin-bottom:4px">
        <path d="M10,70 A60,60 0 0,1 130,70" fill="none" stroke="#2a2d3e" stroke-width="8" stroke-linecap="round"/>
        <path d="M10,70 A60,60 0 0,1 130,70" fill="none" stroke="#4f8ef7" stroke-width="4" stroke-linecap="round" opacity=".5"/>
        <line id="servo-needle" x1="70" y1="70" x2="70" y2="15" stroke="#38d9a9" stroke-width="3" stroke-linecap="round"/>
        <circle cx="70" cy="70" r="5" fill="#38d9a9"/>
      </svg>
      <span class="servo-angle-label" id="servo-disp">75°</span>
      <input type="range" class="servo-slider" id="servo-slider" min="60" max="110" value="75"
        oninput="setServo(this.value)">
      <div style="display:flex;justify-content:space-between;font-size:11px;color:var(--muted);margin-top:4px">
        <span>60° Kiri</span><span>75° Center</span><span>110° Kanan</span>
      </div>
      <div class="row-btns">
        <button class="btn" onclick="snapServo(60)">◄ Kiri Max</button>
        <button class="btn" onclick="snapServo(75)">Center</button>
        <button class="btn" onclick="snapServo(110)">Kanan Max ►</button>
      </div>
    </div>
  </div>

  <!-- SERVO BENIH -->
  <div class="card">
    <div class="card-title">Servo Benih SG90 — pin 7</div>
    <span class="benih-angle-label" id="benih-disp">45°</span>
    <input type="range" class="benih-slider" id="benih-slider" min="0" max="180" value="34"
      oninput="setBenih(this.value)">
    <div style="display:flex;justify-content:space-between;font-size:11px;color:var(--muted);margin-top:4px">
      <span>0°</span><span>34° Tutup</span><span>90°</span><span>95° Buka</span><span>180°</span>
    </div>
    <div class="row-btns">
      <button class="btn" id="btn-b45" onclick="snapBenih(34)">34° Tutup</button>
      <button class="btn purple" id="btn-b135" onclick="snapBenih(95)">95° Buka</button>
    </div>
    <div class="note">Nonaktif saat sequence tanam berjalan</div>
  </div>

  <!-- STEPPER -->
  <div class="card full">
    <div class="card-title">Stepper Benih — Manual & Kalibrasi</div>
    <div class="stepper-val" id="stepper-disp">0 step</div>
    <input type="range" class="stepper-slider" id="stepper-slider" min="0" max="1380" value="0"
      oninput="setStepper(this.value)">
    <div class="stepper-labels">
      <span>0 (Home)</span><span>345</span><span>690</span><span>1035</span><span>1380 (Max/Ngebor)</span>
    </div>
    <div class="row-btns">
      <button class="btn orange" id="btn-s10"  onclick="snapStepper(0)">Home</button>
      <button class="btn orange" id="btn-s200" onclick="snapStepper(345)">345</button>
      <button class="btn orange" id="btn-s400" onclick="snapStepper(690)">690</button>
      <button class="btn orange" id="btn-s600" onclick="snapStepper(1035)">1035</button>
      <button class="btn orange" id="btn-s800" onclick="snapStepper(1380)">1380 Max</button>
    </div>
    <div class="note">Posisi aktual: <span id="stepper-actual" style="color:var(--orange);font-weight:700">0</span> step</div>
  </div>

  <!-- TOMBOL TANAM -->
  <div class="card full">
    <div class="card-title">Sequence Tanam Otomatis</div>
    <button class="tanam-btn" id="btn-tanam" onclick="jalankanTanam()">
      🌱 TANAM SEKARANG
    </button>
    <div class="tanam-status" id="tanam-status"></div>
    <div class="note">
      Stop → Bor turun (1380 step) → Buka 95° → Dorong+Tutup 34° → Bor naik
    </div>
  </div>

</div>
<div class="status">Last update: <span id="last-update">-</span></div>

<script>
const SERVO_CENTER = 75;
const SERVO_LEFT   = 60;
const SERVO_RIGHT  = 110;

function getPwm(){return parseInt(document.getElementById('pwm-slider').value)}

function setAllBtns(disabled){
  ['btn-maju','btn-mundur','btn-kiri','btn-kanan','btn-stop',
   'btn-b45','btn-b135','btn-s10','btn-s200','btn-s400','btn-s600','btn-s800',
   'btn-tanam','btn-reset-enc'].forEach(id=>{
    const el=document.getElementById(id);
    if(el) el.disabled=disabled;
  });
  document.getElementById('stepper-slider').disabled=disabled;
  document.getElementById('benih-slider').disabled=disabled;
}

function go(dir){
  const p=getPwm();
  const cmds={
    'fwd':'MOTOR,'+p+',1,'+p+',1',
    'rev':'MOTOR,'+p+',0,'+p+',0',
    'lft':'MOTOR,'+(p/3|0)+',1,'+p+',1',
    'rgt':'MOTOR,'+p+',1,'+(p/3|0)+',1',
    'stp':'STOP'
  };
  sendCmd(cmds[dir]||'STOP');
}

// Toggle 1 grup untuk 4 arah — cuma 1 yang bisa aktif (1 command MOTOR cuma 1 arah)
const dirBtns={fwd:{id:'btn-maju',label:'▲ Maju',stopLabel:'■ STOP MAJU'},
               rev:{id:'btn-mundur',label:'▼ Mundur',stopLabel:'■ STOP MUNDUR'},
               lft:{id:'btn-kiri',label:'◄ Kiri',stopLabel:'■ STOP KIRI'},
               rgt:{id:'btn-kanan',label:'Kanan ►',stopLabel:'■ STOP KANAN'}};
let activeDir=null;

function paintDirBtn(dir,active){
  const b=document.getElementById(dirBtns[dir].id);
  if(active){
    b.textContent=dirBtns[dir].stopLabel;
    b.style.background='var(--red)'; b.style.borderColor='var(--red)'; b.style.color='#fff';
  } else {
    b.textContent=dirBtns[dir].label;
    b.style.background=''; b.style.borderColor=''; b.style.color='';
  }
}

function toggleDir(dir){
  if(activeDir===dir){
    paintDirBtn(dir,false);
    activeDir=null;
    go('stp');
    return;
  }
  if(activeDir) paintDirBtn(activeDir,false);
  activeDir=dir;
  paintDirBtn(dir,true);
  go(dir);
}

function resetDir(){
  if(activeDir) paintDirBtn(activeDir,false);
  activeDir=null;
}

function setServo(val){
  val=Math.min(Math.max(parseInt(val),SERVO_LEFT),SERVO_RIGHT);
  document.getElementById('servo-disp').textContent=val+'°';
  document.getElementById('servo-slider').value=val;
  const deg=((val-SERVO_CENTER)/(SERVO_RIGHT-SERVO_CENTER))*90;
  const rad=deg*Math.PI/180;
  document.getElementById('servo-needle').setAttribute('x2',(70+55*Math.sin(rad)).toFixed(1));
  document.getElementById('servo-needle').setAttribute('y2',(70-55*Math.cos(rad)).toFixed(1));
  sendCmd('SERVO,'+val);
}
function snapServo(val){setServo(val)}

function setBenih(val){
  val=parseInt(val);
  document.getElementById('benih-disp').textContent=val+'°';
  document.getElementById('benih-slider').value=val;
  sendCmd('BENIH,'+val);
}
function snapBenih(val){setBenih(val)}

function setStepper(val){
  val=parseInt(val);
  document.getElementById('stepper-disp').textContent=val+' step';
  document.getElementById('stepper-slider').value=val;
  sendCmd('STEPPER,'+val);
}
function snapStepper(val){setStepper(val)}

function jalankanTanam(){
  sendCmd('TANAM');
  document.getElementById('tanam-status').textContent='Sequence dimulai...';
  setAllBtns(true);
}

function sendCmd(cmd){fetch('/cmd?c='+encodeURIComponent(cmd))}

setInterval(()=>{
  fetch('/data').then(r=>r.json()).then(d=>{
    document.getElementById('e1').textContent=d.enc1;
    document.getElementById('e2').textContent=d.enc2;
    document.getElementById('r1').textContent=d.rpm1+' RPM';
    document.getElementById('r2').textContent=d.rpm2+' RPM';

    const sv=document.getElementById('servo-slider');
    if(Math.abs(d.servo-parseInt(sv.value))>2) setServo(d.servo);

    if(!d.seq){
      const bv=document.getElementById('benih-slider');
      if(Math.abs(d.benih-parseInt(bv.value))>2) setBenih(d.benih);
      const stv=document.getElementById('stepper-slider');
      if(Math.abs(d.stepper-parseInt(stv.value))>5){
        stv.value=d.stepper;
        document.getElementById('stepper-disp').textContent=d.stepper+' step';
      }
    }

    document.getElementById('stepper-actual').textContent=d.stepper;

    const badge=document.getElementById('seq-badge');
    const dot=document.getElementById('status-dot');
    const status=document.getElementById('tanam-status');
    if(d.seq){
      badge.classList.add('show');
      dot.classList.add('busy');
      status.textContent='🌱 Menanam... bor: '+d.stepper+' step';
      setAllBtns(true);
    } else {
      badge.classList.remove('show');
      dot.classList.remove('busy');
      if(status.textContent.includes('Menanam')){
        status.textContent='✅ Selesai!';
        setTimeout(()=>{status.textContent=''},2000);
      }
      setAllBtns(false);
    }

    document.getElementById('last-update').textContent=new Date().toLocaleTimeString();
  }).catch(()=>{});
},300);
</script>
</body>
</html>
)rawliteral";

void handleRoot()    { server.send_P(200,"text/html",INDEX_HTML); }
void handleData() {
  String j="{\"enc1\":"+String(enc1)+",\"enc2\":"+String(enc2)+
    ",\"rpm1\":"+String(rpm1)+",\"rpm2\":"+String(rpm2)+
    ",\"servo\":"+String(servoAngle)+",\"stepper\":"+String(stepperPos)+
    ",\"benih\":"+String(benihAngle)+
    ",\"seq\":"+(seqRunning?"true":"false")+"}";
  server.send(200,"application/json",j);
}
void handleCmd() {
  if(server.hasArg("c")) Serial.println(server.arg("c"));
  server.send(200,"text/plain","OK");
}
void handleResetEnc() { enc1=enc2=0; Serial.println("RESETENC"); server.send(200,"text/plain","OK"); }

void setup() {
  Serial.begin(115200);
  WiFi.softAP(ap_ssid, ap_password);
  server.on("/",          handleRoot);
  server.on("/cmd",       handleCmd);
  server.on("/data",      handleData);
  server.on("/reset_enc", handleResetEnc);
  server.begin();
}

String megaBuffer="";
void loop() {
  server.handleClient();
  yield(); // beri waktu ESP proses WiFi stack
  while(Serial.available()){
    char c=Serial.read();
    if(c=='\n'){parseData(megaBuffer);megaBuffer="";}
    else megaBuffer+=c;
  }
  delay(1); // 1ms yield tambahan agar tidak block
}
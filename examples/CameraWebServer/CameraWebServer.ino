/*
  CameraWebServer: Web-based Camera Control Panel

  A full-featured web interface for the Arducam Qwiic CAM.
  Left sidebar with all camera controls, right pane with
  live image preview. Supports single capture and polling
  live view modes.

  Hardware Connections:
    QWIIC --> QWIIC

  SSID:     Arducam_Qwiic_CAM
  Password: 123456789
  IP:       192.168.4.1

  Controls:
    Resolution: QVGA / VGA / HD / UXGA / FHD / WQXGA2 / ...
    Quality:    High / Default / Low
    Brightness: -4 ~ +4
    Contrast:   -3 ~ +3
    Saturation: -3 ~ +3
    WB Mode:    Auto / Sunny / Office / Cloudy / Home
    Color FX:   None / Blueish / Redish / B&W / Sepia / ...

  License: MIT License (https://en.wikipedia.org/wiki/MIT_License)
  Web: http://www.ArduCAM.com
*/
#include "Arducam_Qwiic_CAM.h"
#include <WiFiS3.h>

Arducam_Qwiic_CAM myCAM;

#define IMAGE_BUF_SIZE 2000

uint32_t imageLength = 0;
uint8_t imageBuf[IMAGE_BUF_SIZE];

const char* ssid = "Arducam_Qwiic_CAM";
const char* pass = "123456789";
WiFiServer server(80);

CAM_IMAGE_MODE currentMode             = CAM_IMAGE_MODE_QVGA;
CAM_IMAGE_PIX_FMT currentPixelFormat   = CAM_IMAGE_PIX_FMT_JPG;   
IMAGE_QUALITY currentQuality           = DEFAULT_QUALITY;
CAM_BRIGHTNESS_LEVEL currentBrightness = CAM_BRIGHTNESS_LEVEL_DEFAULT;
CAM_CONTRAST_LEVEL currentContrast     = CAM_CONTRAST_LEVEL_DEFAULT;
CAM_STAURATION_LEVEL currentSaturation = CAM_STAURATION_LEVEL_DEFAULT;
CAM_WHITE_BALANCE currentWB            = CAM_WHITE_BALANCE_MODE_DEFAULT;
CAM_COLOR_FX currentColorFx            = CAM_COLOR_FX_NONE;

void handleCapture(WiFiClient& client, const String& path);
void handleSet(WiFiClient& client, const String& path);
void handleStream(WiFiClient& client);
void applyCurrentSettings(void);

bool isSmallRawMode(CAM_IMAGE_MODE mode);
void getModeSize(CAM_IMAGE_MODE mode, uint16_t& width, uint16_t& height);
uint8_t rawBytesPerPixel(CAM_IMAGE_PIX_FMT fmt);

CAM_IMAGE_MODE imageModeFromValue(int val);
int imageModeToValue(CAM_IMAGE_MODE mode);
CAM_IMAGE_PIX_FMT pixelFormatFromValue(int val);
int pixelFormatToValue(CAM_IMAGE_PIX_FMT fmt);
const char* imageModeName(CAM_IMAGE_MODE mode);
const char* pixelFormatName(CAM_IMAGE_PIX_FMT fmt);
const char* pixelFormatContentType(CAM_IMAGE_PIX_FMT fmt);
bool isRawPixelFormat(CAM_IMAGE_PIX_FMT fmt);
bool isSmallRawMode(CAM_IMAGE_MODE mode);
void getModeSize(CAM_IMAGE_MODE mode, uint16_t& width, uint16_t& height);
uint8_t rawBytesPerPixel(CAM_IMAGE_PIX_FMT fmt);
CAM_IMAGE_MODE fixModeForPixelFormat(CAM_IMAGE_MODE mode, CAM_IMAGE_PIX_FMT fmt);


CAM_IMAGE_MODE imageModeFromValue(int val) {
  switch (val) {
    case 1: return CAM_IMAGE_MODE_QVGA;
    case 2: return CAM_IMAGE_MODE_VGA;
    case 3: return CAM_IMAGE_MODE_HD;
    case 4: return CAM_IMAGE_MODE_UXGA;
    case 5: return CAM_IMAGE_MODE_FHD;
    case 6: return CAM_IMAGE_MODE_WQXGA2;
    case 7: return CAM_IMAGE_MODE_96X96;
    case 8: return CAM_IMAGE_MODE_128X128;
    case 9: return CAM_IMAGE_MODE_320X320;
    default: return currentMode;
  }
}

int imageModeToValue(CAM_IMAGE_MODE mode) {
  if (mode == CAM_IMAGE_MODE_QVGA) return 1;
  if (mode == CAM_IMAGE_MODE_VGA) return 2;
  if (mode == CAM_IMAGE_MODE_HD) return 3;
  if (mode == CAM_IMAGE_MODE_UXGA) return 4;
  if (mode == CAM_IMAGE_MODE_FHD) return 5;
  if (mode == CAM_IMAGE_MODE_WQXGA2) return 6;
  if (mode == CAM_IMAGE_MODE_96X96) return 7;
  if (mode == CAM_IMAGE_MODE_128X128) return 8;
  if (mode == CAM_IMAGE_MODE_320X320) return 9;
  return 1;
}

const char* imageModeName(CAM_IMAGE_MODE mode) {
  if (mode == CAM_IMAGE_MODE_QVGA) return "QVGA 320x240";
  if (mode == CAM_IMAGE_MODE_VGA) return "VGA 640x480";
  if (mode == CAM_IMAGE_MODE_HD) return "HD 1280x720";
  if (mode == CAM_IMAGE_MODE_UXGA) return "UXGA 1600x1200";
  if (mode == CAM_IMAGE_MODE_FHD) return "FHD 1920x1080";
  if (mode == CAM_IMAGE_MODE_WQXGA2) return "WQXGA2 2592x1944";
  if (mode == CAM_IMAGE_MODE_96X96) return "96x96";
  if (mode == CAM_IMAGE_MODE_128X128) return "128x128";
  if (mode == CAM_IMAGE_MODE_320X320) return "320x320";
  return "UNKNOWN";
}

CAM_IMAGE_PIX_FMT pixelFormatFromValue(int val) {
  switch (val) {
    case 0:
      return CAM_IMAGE_PIX_FMT_JPG;

    case 1:
      return CAM_IMAGE_PIX_FMT_RGB565;

    case 2:
      return CAM_IMAGE_PIX_FMT_Y8;

    default:
      return CAM_IMAGE_PIX_FMT_JPG;
  }
}


int pixelFormatToValue(CAM_IMAGE_PIX_FMT fmt) {
  if (fmt == CAM_IMAGE_PIX_FMT_JPG) return 0;
  if (fmt == CAM_IMAGE_PIX_FMT_RGB565) return 1;
  if (fmt == CAM_IMAGE_PIX_FMT_Y8) return 2;
  return 0;
}

bool isRawPixelFormat(CAM_IMAGE_PIX_FMT fmt) {
  return (fmt == CAM_IMAGE_PIX_FMT_RGB565 || fmt == CAM_IMAGE_PIX_FMT_Y8);
}

bool isSmallRawMode(CAM_IMAGE_MODE mode) {
  return (mode == CAM_IMAGE_MODE_96X96 ||
          mode == CAM_IMAGE_MODE_128X128 ||
          mode == CAM_IMAGE_MODE_QVGA);  
}

void getModeSize(CAM_IMAGE_MODE mode, uint16_t& width, uint16_t& height) {
  switch (mode) {
    case CAM_IMAGE_MODE_QVGA:    width = 320;  height = 240;  break;
    case CAM_IMAGE_MODE_VGA:     width = 640;  height = 480;  break;
    case CAM_IMAGE_MODE_HD:      width = 1280; height = 720;  break;
    case CAM_IMAGE_MODE_UXGA:    width = 1600; height = 1200; break;
    case CAM_IMAGE_MODE_FHD:     width = 1920; height = 1080; break;
    case CAM_IMAGE_MODE_WQXGA2:  width = 2592; height = 1944; break;
    case CAM_IMAGE_MODE_96X96:   width = 96;   height = 96;   break;
    case CAM_IMAGE_MODE_128X128: width = 128;  height = 128;  break;
    case CAM_IMAGE_MODE_320X320: width = 320;  height = 320;  break;
    default:                     width = 0;    height = 0;    break;
  }
}

uint8_t rawBytesPerPixel(CAM_IMAGE_PIX_FMT fmt) {
  if (fmt == CAM_IMAGE_PIX_FMT_RGB565) return 2;
  if (fmt == CAM_IMAGE_PIX_FMT_Y8) return 1;
  return 0;
}

CAM_IMAGE_MODE fixModeForPixelFormat(CAM_IMAGE_MODE mode, CAM_IMAGE_PIX_FMT fmt) {
  if (isRawPixelFormat(fmt) && !isSmallRawMode(mode)) {
    return CAM_IMAGE_MODE_QVGA;   
  }
  return mode;
}

const char* pixelFormatName(CAM_IMAGE_PIX_FMT fmt) {
  if (fmt == CAM_IMAGE_PIX_FMT_JPG) {
    return "JPEG";
  }

  if (fmt == CAM_IMAGE_PIX_FMT_RGB565) {
    return "RGB565";
  }

  if (fmt == CAM_IMAGE_PIX_FMT_Y8) {
    return "Y8";
  }

  return "JPEG";
}

const char* pixelFormatContentType(CAM_IMAGE_PIX_FMT fmt) {
  if (fmt == CAM_IMAGE_PIX_FMT_JPG) {
    return "image/jpeg";
  }

  return "application/octet-stream";
}

static void serveIndex(WiFiClient& client) {
  client.print(F("HTTP/1.1 200 OK\r\n"
                 "Content-Type: text/html; charset=UTF-8\r\n"
                 "Connection: close\r\n\r\n"));

  client.print(F("<!DOCTYPE html><html><head>"
                 "<meta charset=\"UTF-8\">"
                 "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1,maximum-scale=1\">"
                 "<title>Arducam Qwiic CAM</title>"
                 "<style>"
                 "*{margin:0;padding:0;box-sizing:border-box}"
                 "body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;background:#111;color:#ddd;display:flex;height:100vh;overflow:hidden}"
                 ".sd{width:310px;min-width:310px;background:#1a1a2e;padding:16px 18px;overflow-y:auto;border-right:1px solid #2a2a4a}"
                 ".main{flex:1;display:flex;flex-direction:column;align-items:center;justify-content:center;padding:16px;position:relative}"
                 ".logo{font-size:16px;font-weight:700;color:#e94560;text-align:center;padding-bottom:10px;border-bottom:1px solid #2a2a4a;margin-bottom:8px}"
                 ".grp{margin-bottom:2px}"
                 ".grp-t{font-size:13px;font-weight:600;color:#666;text-transform:uppercase;letter-spacing:1px;padding:10px 0 6px}"
                 ".r{display:flex;align-items:center;margin-bottom:14px;gap:8px}"
                 ".r lb{font-size:14px;color:#aaa;width:85px;min-width:85px}"
                 ".r select{flex:1;padding:5px 6px;border:1px solid #333;border-radius:3px;background:#0d0d1a;color:#ddd;font-size:14px}"
                 ".r select:focus{outline:0;border-color:#e94560}"
                 ".bb{padding:8px 14px;border:0;border-radius:4px;font-size:14px;font-weight:600;cursor:pointer;width:100%;margin-top:6px}"
                 ".bp{background:#e94560;color:#fff}.bp:hover{background:#d63850}.bp.act{background:#2ecc71}"
                 ".bs{background:#333;color:#ddd}.bs:hover{background:#444}"
                 ".rw{display:flex;gap:5px;margin-top:6px}"
                 ".rw .bb{width:auto;flex:1}"
                 ".sg{display:flex;align-items:center;flex:1;gap:6px}"
                 ".sg input[type=range]{flex:1;-webkit-appearance:none;appearance:none;height:4px;background:#444;border-radius:2px;outline:0;margin:0;padding:0}"
                 ".sg input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;appearance:none;width:18px;height:18px;border-radius:50%;background:#e94560;cursor:pointer;border:0}"
                 ".sg input[type=range]::-moz-range-thumb{width:18px;height:18px;border-radius:50%;background:#e94560;cursor:pointer;border:0}"
                 ".sv{font-size:13px;color:#e94560;min-width:36px;text-align:center;font-weight:600}"
                 "#pv,#cv{max-width:100%;max-height:85vh;border-radius:6px;box-shadow:0 2px 20px rgba(0,0,0,.6);display:none;image-rendering:pixelated}"
                 "#ph{color:#555;font-size:15px;text-align:center;padding:20px}"
                 "#fps{position:absolute;bottom:24px;right:24px;font-size:11px;color:#666;background:rgba(0,0,0,.75);padding:3px 10px;border-radius:4px;display:none}"
                 ".st{font-size:13px;color:#555;margin-top:6px;text-align:center}"
                 "@media(max-width:750px){.sd{width:100%;min-width:unset;max-height:45vh;border-right:0;border-bottom:1px solid #2a2a4a}body{flex-direction:column}.main{padding:10px}}"
                 "</style></head><body>"));

  client.print(F("<div class=\"sd\">"
                 "<div class=\"logo\">Arducam Qwiic CAM</div>"
                 "<div class=\"grp\">"
                 "<div class=\"grp-t\">Capture</div>"
                 "<div class=\"r\"><lb>Resolution</lb><select id=\"md\" onchange=\"onMode(this)\">"
                 "<option value=\"1\">QVGA 320x240</option>"
                 "<option value=\"2\">VGA 640x480</option>"
                 "<option value=\"3\">HD 1280x720</option>"
                 "<option value=\"4\">UXGA 1600x1200</option>"
                 "<option value=\"5\">FHD 1920x1080</option>"
                 "<option value=\"6\">WQXGA2 2592x1944</option>"
                 "<option value=\"7\">96x96</option>"
                 "<option value=\"8\">128x128</option>"
                 "<option value=\"9\">320x320</option>"
                 "</select></div>"
                 "<div class=\"r\"><lb>Quality</lb><select id=\"qlt\" onchange=\"sp('quality',this.value)\">"
                 "<option value=\"0\">High</option>"
                 "<option value=\"1\" selected>Default</option>"
                 "<option value=\"2\">Low</option>"
                 "</select></div>"
                 "<div class=\"r\"><lb>Format</lb><select id=\"fmt\" onchange=\"onFmt(this)\">"
                 "<option value=\"0\" selected>JPEG Preview</option>"
                 "<option value=\"1\">RGB565 Preview</option>"
                 "<option value=\"2\">Y8 Preview</option>"
                 "</select></div></div>"));

  client.print(F("<div class=\"rw\">"
                 "<button class=\"bb bp\" id=\"cpBtn\" onclick=\"capt()\">Capture</button>"
                 "<button class=\"bb bs\" id=\"lvBtn\" onclick=\"toggleLV()\">Stream</button>"
                 "</div>"
                 "<div class=\"st\" id=\"st\">Ready</div>"));

  client.print(F("<div class=\"grp\"><div class=\"grp-t\">Camera Settings</div>"
                 "<div class=\"r\"><lb>Brightness</lb><div class=\"sg\"><input type=\"range\" id=\"brt\" min=\"0\" max=\"8\" value=\"4\" oninput=\"onBrt(this)\"><span class=\"sv\" id=\"brtV\">0</span></div></div>"
                 "<div class=\"r\"><lb>Contrast</lb><div class=\"sg\"><input type=\"range\" id=\"cst\" min=\"0\" max=\"6\" value=\"3\" oninput=\"onCst(this)\"><span class=\"sv\" id=\"cstV\">0</span></div></div>"
                 "<div class=\"r\"><lb>Saturation</lb><div class=\"sg\"><input type=\"range\" id=\"sat\" min=\"0\" max=\"6\" value=\"3\" oninput=\"onSat(this)\"><span class=\"sv\" id=\"satV\">0</span></div></div>"
                 "<div class=\"r\"><lb>WB Mode</lb><select id=\"wb\" onchange=\"sp('wb',this.value)\">"
                 "<option value=\"0\">Auto</option><option value=\"1\">Sunny</option><option value=\"2\">Office</option>"
                 "<option value=\"3\">Cloudy</option><option value=\"4\">Home</option>"
                 "</select></div>"
                 "<div class=\"r\"><lb>Color FX</lb><select id=\"cfx\" onchange=\"sp('colorfx',this.value)\">"
                 "<option value=\"0\">None</option><option value=\"1\">Blueish</option><option value=\"2\">Redish</option>"
                 "<option value=\"3\">B&amp;W</option><option value=\"4\">Sepia</option><option value=\"5\">Negative</option>"
                 "<option value=\"6\">Grass Grn</option><option value=\"7\">Over Exp</option><option value=\"8\">Solarize</option>"
                 "</select></div>"
                 "</div>"
                 "</div>"));

  client.print(F("<div class=\"main\">"
                 "<div id=\"ph\">Adjust camera settings in the sidebar,<br>then click Capture or toggle Stream.</div>"
                 "<img id=\"pv\" src=\"\" alt=\"preview\"/>"
                 "<canvas id=\"cv\"></canvas>"
                 "<div id=\"fps\"></div>"
                 "</div>"));

  client.print(F("<script>"
                 "var lv=0,fc=0,ft=0;"
                 "var brtM=[8,6,4,2,0,1,3,5,7];var csM=[6,4,2,0,1,3,5];"
                 "function rawOK(v){return v=='1'||v=='7'||v=='8'}"
                 "function wh(m){if(m=='7')return[96,96];if(m=='8')return[128,128];if(m=='9')return[320,320];if(m=='1')return[320,240];if(m=='2')return[640,480];if(m=='3')return[1280,720];if(m=='4')return[1600,1200];if(m=='5')return[1920,1080];if(m=='6')return[2592,1944];return[0,0]}"
                 "function onBrt(e){var v=brtM[e.value];document.getElementById('brtV').textContent=e.value-4;sp('brightness',v)}"
                 "function onCst(e){var v=csM[e.value];document.getElementById('cstV').textContent=e.value-3;sp('contrast',v)}"
                 "function onSat(e){var v=csM[e.value];document.getElementById('satV').textContent=e.value-3;sp('saturation',v)}"
                 "function sp(n,v){var x=new XMLHttpRequest();x.open('GET','/set?'+n+'='+v,1);x.send();}"
                 "function onMode(e){var f=document.getElementById('fmt').value;if(f!='0'&&!rawOK(e.value)){e.value='1';document.getElementById('st').textContent='RAW preview uses 96x96, 128x128, 320x240';}sp('mode',e.value)}"
                 "function onFmt(e){sp('fmt',e.value);if(e.value!='0'){if(lv)stopLV();var m=document.getElementById('md');if(!rawOK(m.value)){m.value='1';sp('mode','1');}document.getElementById('st').textContent='RAW preview mode: 96x96 / 128x128 / 320x240';}else{document.getElementById('st').textContent='JPEG preview mode';}}"
                 "function showJPEG(m){var i=document.getElementById('pv');var c=document.getElementById('cv');var h=document.getElementById('ph');c.style.display='none';i.onload=function(){document.getElementById('st').textContent='JPEG captured'};i.onerror=function(){document.getElementById('st').textContent='JPEG capture failed'};i.src='/capture?mode='+m+'&fmt=0&t='+Date.now();i.style.display='block';h.style.display='none'}"
                 "function putPixel(d,p,r,g,b){var k=p*4;d[k]=r;d[k+1]=g;d[k+2]=b;d[k+3]=255}"
                 "function rgb565At(u,j){return (u[j]<<8)|u[j+1]}"
                 "function rgb565ToCanvas(u,w,h,d){var total=w*h;for(var p=0,j=0;p<total&&j+1<u.length;p++,j+=2){var v=rgb565At(u,j);var r=((v>>11)&31)<<3;var g=((v>>5)&63)<<2;var b=(v&31)<<3;putPixel(d,p,r|(r>>5),g|(g>>6),b|(b>>5));}}"
                 "function y8ToCanvas(u,w,h,d){var total=w*h;for(var p=0;p<total&&p<u.length;p++){var y=u[p];putPixel(d,p,y,y,y);}}"
                 "function scoreBytes(u,off){var n=Math.min(4096,Math.floor((u.length-off)/2));if(n<=2)return 0;var min=255,max=0,sum=0,sum2=0;for(var i=0;i<n;i++){var v=u[i*2+off];if(v<min)min=v;if(v>max)max=v;sum+=v;sum2+=v*v;}var mean=sum/n;var varr=sum2/n-mean*mean;return (max-min)+Math.sqrt(Math.max(0,varr));}"
                 "function y16ToCanvas(u,w,h,d){var total=w*h;var n=Math.min(total,Math.floor(u.length/2));var s0=scoreBytes(u,0),s1=scoreBytes(u,1);var byteMode=(Math.max(s0,s1)>18);var min=65535,max=0;var vals=new Array(n);if(byteMode){var off=s1>=s0?1:0;for(var p=0;p<n;p++){var v=u[p*2+off];vals[p]=v;if(v<min)min=v;if(v>max)max=v;}}else{for(var p=0;p<n;p++){var j=p*2;var v=u[j]|(u[j+1]<<8);vals[p]=v;if(v<min)min=v;if(v>max)max=v;}}var span=max-min;if(span<1)span=1;for(var p=0;p<n;p++){var y=Math.round((vals[p]-min)*255/span);putPixel(d,p,y,y,y);}return byteMode?'Y16 byte-aligned auto':'Y16 little-endian autoscale'}"
                 "function drawRaw(buf,fmt,w,h){var u=new Uint8Array(buf);var c=document.getElementById('cv');var i=document.getElementById('pv');var ph=document.getElementById('ph');if(!w||!h){document.getElementById('st').textContent='RAW capture failed: missing width/height';return;}c.width=w;c.height=h;var ctx=c.getContext('2d');var im=ctx.createImageData(w,h);var d=im.data;var label='';var expect=w*h;if(fmt==1){rgb565ToCanvas(u,w,h,d);label='RGB565';if(u.length<expect*2)label+=' partial '+Math.floor(u.length/2)+'/'+expect+' pixels';}else{if(u.length>=expect*2){label=y16ToCanvas(u,w,h,d);}else{y8ToCanvas(u,w,h,d);label='Y8';if(u.length<expect)label+=' partial '+u.length+'/'+expect+' pixels';}}ctx.putImageData(im,0,0);i.style.display='none';c.style.display='block';ph.style.display='none';document.getElementById('st').textContent=label+' captured: '+w+'x'+h+', '+u.length+' bytes'}"
                 "function showRaw(m,f){var st=document.getElementById('st');st.textContent='Capturing RAW preview...';fetch('/capture?mode='+m+'&fmt='+f+'&t='+Date.now()).then(function(r){if(!r.ok){return r.text().then(function(t){throw new Error(t||('HTTP '+r.status));});}var w=parseInt(r.headers.get('X-Width')||'0');var h=parseInt(r.headers.get('X-Height')||'0');return r.arrayBuffer().then(function(b){drawRaw(b,parseInt(f),w,h);});}).catch(function(e){st.textContent='RAW capture failed: '+e.message;});}"
                 "function capt(){var m=document.getElementById('md').value;var f=document.getElementById('fmt').value;if(f!='0'){if(!rawOK(m)){m='1';document.getElementById('md').value='1';sp('mode','1');}showRaw(m,f);return;}showJPEG(m)}"
                 "function toggleLV(){if(lv)stopLV();else startLV()}"
                 "function nextFrame(){if(!lv)return;var i=document.getElementById('pv');var m=document.getElementById('md').value;i.src='/capture?mode='+m+'&fmt=0&t='+Date.now()}"
                 "function startLV(){if(document.getElementById('fmt').value!='0'){alert('Stream preview supports JPEG only. Use Capture for RGB565/Y8/Y16 Canvas preview.');return;}lv=1;fc=0;ft=Date.now();var i=document.getElementById('pv');var c=document.getElementById('cv');var h=document.getElementById('ph');c.style.display='none';i.style.display='block';h.style.display='none';i.onload=function(){if(!lv)return;fc++;var n=Date.now();if(n-ft>=2000){document.getElementById('fps').textContent=Math.round(fc*1000/(n-ft))+' FPS';fc=0;ft=n;}setTimeout(nextFrame,80)};document.getElementById('lvBtn').textContent='Stop';document.getElementById('lvBtn').className='bb bp act';document.getElementById('fps').style.display='block';document.getElementById('st').textContent='Stream active, settings can be changed';nextFrame()}"
                 "function stopLV(){lv=0;document.getElementById('pv').onload=null;document.getElementById('pv').src='';document.getElementById('lvBtn').textContent='Stream';document.getElementById('lvBtn').className='bb bs';document.getElementById('fps').style.display='none';document.getElementById('st').textContent='Ready'}"
                 "</script></body></html>"));

  Serial.println(F("Index served"));
}

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println(F("CameraWebServer - Arducam Qwiic CAM"));
  Serial.println(F("Connect WiFi -> SSID: Arducam_Qwiic_CAM / 123456789"));
  Serial.println(F("Open http://192.168.4.1 in your browser"));
  Serial.println(F("Default pixel format: JPEG"));

  if (myCAM.begin()) {
    Serial.println(F("camera init failed!"));
    while (true);
  }

  while (1) {
    myCAM.writeReg(0x00, 0x55);
    uint8_t data = myCAM.readReg(0x00);

    if (data == 0x55) {
      Serial.println(F("camera init success!"));
      break;
    } else {
      Serial.println("camera not detect");
    }

    delay(10);
  }

  applyCurrentSettings();

  WiFi.config(IPAddress(192, 168, 4, 1));

  if (WiFi.beginAP(ssid, pass) != WL_AP_LISTENING) {
    Serial.println(F("AP creation failed!"));
  } else {
    server.begin();
    Serial.println(F("Web server started on http://192.168.4.1"));
  }
}

void loop() {
  WiFiClient client = server.available();
  if (!client) return;

  String reqLine = client.readStringUntil('\n');
  reqLine.trim();

  String path = "/";
  if (reqLine.startsWith("GET ")) {
    int sp1 = reqLine.indexOf(' ');
    int sp2 = reqLine.indexOf(' ', sp1 + 1);

    if (sp1 >= 0 && sp2 > sp1) {
      path = reqLine.substring(sp1 + 1, sp2);
    }
  }

  while (client.available() && !client.find("\r\n\r\n"));

  if (path == "/" || path.startsWith("/index")) {
    serveIndex(client);
  } else if (path.startsWith("/capture")) {
    handleCapture(client, path);
  } else if (path.startsWith("/stream")) {
    handleStream(client);
  } else if (path.startsWith("/set")) {
    handleSet(client, path);
  } else {
    client.print(F("HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n"
                    "<html><body><h2>404 Not Found</h2></body></html>"));
  }

  client.stop();

  while (client.available()) {
    client.read();
  }

}

void handleCapture(WiFiClient& client, const String& path) {
  CAM_IMAGE_MODE mode = currentMode;
  CAM_IMAGE_PIX_FMT pixFmt = currentPixelFormat;

  int qpos = path.indexOf('?');
  if (qpos >= 0) {
    String qs = path.substring(qpos + 1);

    int mpos = qs.indexOf("mode=");
    if (mpos >= 0) {
      int mend = qs.indexOf('&', mpos);
      if (mend < 0) mend = qs.length();

      int val = qs.substring(mpos + 5, mend).toInt();
      mode = imageModeFromValue(val);
    }

    int fpos = qs.indexOf("fmt=");
    if (fpos >= 0) {
      int fend = qs.indexOf('&', fpos);
      if (fend < 0) fend = qs.length();

      int val = qs.substring(fpos + 4, fend).toInt();
      pixFmt = pixelFormatFromValue(val);
    }
  }

  bool rawFormat = isRawPixelFormat(pixFmt);

  CAM_IMAGE_MODE fixedMode = fixModeForPixelFormat(mode, pixFmt);
  if (fixedMode != mode) {
    Serial.println(F("RGB565/Y8 preview uses 96x96, 128x128 or QVGA 320x240; force mode to 320x240."));
    mode = fixedMode;
  }

  Serial.print(F("Capture pixel format: "));
  Serial.println(pixelFormatName(pixFmt));

  CamStatus ret = myCAM.takePicture(mode, pixFmt);
  if (ret != CAM_ERR_NONE) {
    client.print(F("HTTP/1.1 500 ERROR\r\nConnection: close\r\n\r\n"));
    Serial.println(F("Capture failed!"));
    return;
  }

  imageLength = myCAM.getTotalLength();

  if (!rawFormat) {
    client.print(F("HTTP/1.1 200 OK\r\n"
                   "Content-Type: image/jpeg\r\n"
                   "Content-Length: "));
    client.print(imageLength);
    client.print(F("\r\n"
                   "Cache-Control: no-cache, no-store, must-revalidate\r\n"
                   "Pragma: no-cache\r\n"
                   "Connection: close\r\n\r\n"));

    uint32_t totalRead = 0;
    while (totalRead < imageLength) {
      uint32_t toRead = IMAGE_BUF_SIZE;
      if (toRead > imageLength - totalRead) toRead = imageLength - totalRead;

      uint32_t actual = myCAM.readImageBuf(imageBuf, toRead);
      if (actual == 0) break;

      client.write(imageBuf, actual);
      totalRead += actual;
    }

    Serial.print(F("Image bytes sent: "));
    Serial.println(totalRead);
    return;
  }

  uint16_t width = 0;
  uint16_t height = 0;
  getModeSize(mode, width, height);

  client.print(F("HTTP/1.1 200 OK\r\n"
                 "Content-Type: application/octet-stream\r\n"
                 "Content-Length: "));
  client.print(imageLength);
  client.print(F("\r\nX-Pixel-Format: "));
  client.print(pixelFormatName(pixFmt));
  client.print(F("\r\nX-Width: "));
  client.print(width);
  client.print(F("\r\nX-Height: "));
  client.print(height);
  client.print(F("\r\nX-Raw-Length: "));
  client.print(imageLength);
  client.print(F("\r\nCache-Control: no-cache, no-store, must-revalidate\r\n"
                 "Pragma: no-cache\r\n"
                 "Connection: close\r\n\r\n"));

  uint32_t totalRead = 0;
  while (totalRead < imageLength) {
    uint32_t toRead = IMAGE_BUF_SIZE;
    if (toRead > imageLength - totalRead) toRead = imageLength - totalRead;

    uint32_t actual = myCAM.readImageBuf(imageBuf, toRead);
    if (actual == 0) break;

    client.write(imageBuf, actual);
    totalRead += actual;
  }

  Serial.print(F("RAW bytes sent to browser: "));
  Serial.println(totalRead);
}

void handleStream(WiFiClient& client) {
  client.print(F("HTTP/1.1 200 OK\r\n"
                  "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n"
                  "Cache-Control: no-cache, no-store, must-revalidate\r\n"
                  "Pragma: no-cache\r\n"
                  "Connection: keep-alive\r\n\r\n"));

  while (client.connected()) {
    CamStatus ret = myCAM.takePicture(currentMode, CAM_IMAGE_PIX_FMT_JPG);

    if (ret != CAM_ERR_NONE) {
      Serial.println(F("Stream capture failed!"));
      break;
    }

    uint32_t len = myCAM.getTotalLength();

    client.print(F("--frame\r\nContent-Type: "));
    client.print(F("image/jpeg"));
    client.print(F("\r\nContent-Length: "));
    client.print(len);
    client.print(F("\r\nX-Pixel-Format: "));
    client.print(F("JPEG"));
    client.print(F("\r\n\r\n"));

    uint32_t totalRead = 0;

    while (totalRead < len) {
      uint32_t toRead = IMAGE_BUF_SIZE;

      if (toRead > len - totalRead) {
        toRead = len - totalRead;
      }

      uint32_t actual = myCAM.readImageBuf(imageBuf, toRead);

      if (actual == 0) {
        break;
      }

      client.write(imageBuf, actual);
      totalRead += actual;
    }

    client.print(F("\r\n"));
  }

  Serial.println(F("Stream ended"));
}

void handleSet(WiFiClient& client, const String& path) {
  int qpos = path.indexOf('?');

  if (qpos < 0) {
    client.print(F("HTTP/1.1 200 OK\r\nConnection: close\r\n\r\nOK"));
    return;
  }

  String qs = path.substring(qpos + 1);
  int pos = 0;

  while (pos < (int)qs.length()) {
    int amp = qs.indexOf('&', pos);
    if (amp < 0) amp = qs.length();

    String pair = qs.substring(pos, amp);
    pos = amp + 1;

    int eq = pair.indexOf('=');
    if (eq < 0) continue;

    String key = pair.substring(0, eq);
    int val = pair.substring(eq + 1).toInt();

    if (key == "mode") {
      switch (val) {
        case 1: currentMode = CAM_IMAGE_MODE_QVGA;    break;
        case 2: currentMode = CAM_IMAGE_MODE_VGA;     break;
        case 3: currentMode = CAM_IMAGE_MODE_HD;      break;
        case 4: currentMode = CAM_IMAGE_MODE_UXGA;    break;
        case 5: currentMode = CAM_IMAGE_MODE_FHD;     break;
        case 6: currentMode = CAM_IMAGE_MODE_WQXGA2;  break;
        case 7: currentMode = CAM_IMAGE_MODE_96X96;   break;
        case 8: currentMode = CAM_IMAGE_MODE_128X128; break;
        case 9: currentMode = CAM_IMAGE_MODE_320X320; break;
      }
      currentMode = fixModeForPixelFormat(currentMode, currentPixelFormat);
    } else if (key == "format" || key == "fmt") {
      currentPixelFormat = pixelFormatFromValue(val);
      currentMode = fixModeForPixelFormat(currentMode, currentPixelFormat);

      Serial.print(F("Set pixel format: "));
      Serial.println(pixelFormatName(currentPixelFormat));

    } else if (key == "quality") {
      currentQuality = (IMAGE_QUALITY)val;
      myCAM.setImageQuality(currentQuality);

    } else if (key == "brightness") {
      currentBrightness = (CAM_BRIGHTNESS_LEVEL)val;
      myCAM.setBrightness(currentBrightness);

    } else if (key == "contrast") {
      currentContrast = (CAM_CONTRAST_LEVEL)val;
      myCAM.setContrast(currentContrast);

    } else if (key == "saturation") {
      currentSaturation = (CAM_STAURATION_LEVEL)val;
      myCAM.setSaturation(currentSaturation);

    } else if (key == "wb") {
      currentWB = (CAM_WHITE_BALANCE)val;
      myCAM.setAutoWhiteBalanceMode(currentWB);

    } else if (key == "colorfx") {
      currentColorFx = (CAM_COLOR_FX)val;
      myCAM.setColorEffect(currentColorFx);
    }
  }

  client.print(F("HTTP/1.1 200 OK\r\nConnection: close\r\n\r\nOK"));
}

void applyCurrentSettings(void) {
  myCAM.setImageQuality(currentQuality);
  myCAM.setBrightness(currentBrightness);
  myCAM.setContrast(currentContrast);
  myCAM.setSaturation(currentSaturation);
  myCAM.setAutoWhiteBalanceMode(currentWB);
  myCAM.setColorEffect(currentColorFx);
}

#include "web.h"

#include "app/digimon_cfg.h"
#include "clock.h"
#include "ui.h"

#include "bsp/esp-bsp.h"
#include "cJSON.h"
#include "esp_event.h"
#include "esp_heap_caps.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stdlib.h>
#include <string.h>

namespace {

const char *TAG = "web";
const char *SSID = "Digimon-175C";
const char *PASS = "digimon123";

const char *HTML = R"HTML(<!doctype html>
<html lang="zh-CN"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>数码宝贝 网页管理</title>
<style>
:root{color-scheme:dark}body{margin:0;background:#080c12;color:#e8f1fa;font:15px/1.5 system-ui,-apple-system,"Segoe UI",sans-serif}
main{max-width:760px;margin:0 auto;padding:18px 16px 40px}h1{font-size:22px;margin:8px 0 4px}h2{font-size:17px;margin:24px 0 8px;color:#4fc3f7}
.card{background:#141d29;border:1px solid #1e2a3a;border-radius:14px;padding:14px;margin:10px 0}
label{display:block;color:#7e90a6;font-size:13px;margin-top:8px}input,select,button{font:inherit;border-radius:9px;border:1px solid #2a3a52;background:#0d141d;color:#e8f1fa;padding:8px 10px}
input[type=number]{width:100%;box-sizing:border-box}select{width:100%}button{background:#1e2a3a;cursor:pointer;margin:8px 6px 0 0}
button.primary{background:#1565c0}button.danger{background:#7f1d1d}button:active{transform:translateY(1px)}
.row{display:grid;grid-template-columns:1fr 1fr;gap:8px}@media(max-width:480px){.row{grid-template-columns:1fr}}
.mono{font-family:ui-monospace,Consolas,monospace;font-size:12px;color:#7e90a6;word-break:break-all}
#msg{color:#81c784}#log{white-space:pre-wrap;color:#7e90a6;font-size:12px;min-height:18px}
progress{width:100%;height:10px;margin-top:8px}
</style></head><body><main>
<h1>数码宝贝 网页管理</h1>
<div class="mono">AP: Digimon-175C / digimon123 / http://192.168.4.1</div>
<div id="msg"></div>

<h2>数值</h2>
<div class="card">
<label>形态</label><select id="form"></select>
<div class="row">
<label>年龄（分钟）<input id="age" type="number" min="0"></label>
<label>经验<input id="exp" type="number" min="0"></label>
</div>
<div class="row">
<label>体重附加（0.1kg）<input id="wb" type="number" min="0"></label>
<label>胜场<input id="wins" type="number" min="0"></label>
</div>
<div class="row">
<label>HP<input id="hp" type="number" min="0"></label>
<label>SP<input id="sp" type="number" min="0"></label>
</div>
<label>时间<input id="time" type="datetime-local"></label>
<label>训练值 [HP, SP, ATK, DEF, INT, SPI, SPD]</label>
<div class="row">
<input id="t0" type="number"><input id="t1" type="number">
<input id="t2" type="number"><input id="t3" type="number">
<input id="t4" type="number"><input id="t5" type="number">
<input id="t6" type="number">
</div>
<button class="primary" onclick="save()">保存数值</button>
<button onclick="load()">重新读取</button>
</div>

<h2>存档</h2>
<div class="card">
<button onclick="downloadSave()">导出 JSON</button>
<input type="file" id="file" accept=".json,application/json">
<button onclick="importSave()">导入并保存</button>
<div class="mono">导入文件会覆盖当前数值，请先导出备份。</div>
</div>

<h2>OTA 固件</h2>
<div class="card">
<input type="file" id="bin" accept=".bin">
<button class="primary" onclick="ota()">上传并重启</button>
<progress id="prog" max="100" value="0"></progress>
<div id="log"></div>
<div class="mono">选择 build/digimon_pet.bin（app 固件，不是合并镜像）。上传完成后设备自动重启。</div>
</div>
</main>
<script>
let species = [];
function $(id){return document.getElementById(id)}
function msg(t){$('msg').textContent=t||''}
function pad(n){return String(n).padStart(2,'0')}
function isoLocal(sec){const d=new Date(sec*1000);return d.getFullYear()+'-'+pad(d.getMonth()+1)+'-'+pad(d.getDate())+'T'+pad(d.getHours())+':'+pad(d.getMinutes())}
async function load(){
  species = await (await fetch('/api/species')).json();
  const sel = $('form'); sel.innerHTML='';
  species.forEach((n,i)=>{const o=document.createElement('option');o.value=i;o.textContent=i+' '+n;sel.appendChild(o)});
  const s = await (await fetch('/api/save')).json();
  sel.value = s.form;
  $('age').value=s.age; $('exp').value=s.exp; $('wb').value=s.weightBonus10;
  $('wins').value=s.wins; $('hp').value=s.hp; $('sp').value=s.sp;
  for(let i=0;i<7;i++) $('t'+i).value=s.train[i];
  $('time').value = s.epoch ? isoLocal(s.epoch) : '';
  msg('已读取：'+s.name+' Lv.'+s.level+' / 体重 '+(s.weight10/10).toFixed(1)+'kg');
}
async function save(){
  const sec = $('time').value ? Math.floor(new Date($('time').value).getTime()/1000) : 0;
  const data={form:+$('form').value,age:+$('age').value,exp:+$('exp').value,
    weightBonus10:+$('wb').value,wins:+$('wins').value,hp:+$('hp').value,sp:+$('sp').value,
    epoch:sec,train:[0,1,2,3,4,5,6].map(i=>+$('t'+i).value)};
  const r=await fetch('/api/save',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(data)});
  if(r.ok){msg('已保存');await load()}else{msg('保存失败 '+r.status)}
}
function downloadSave(){
  fetch('/api/save').then(r=>r.text()).then(t=>{
    const b=new Blob([t],{type:'application/json'});const a=document.createElement('a');
    a.href=URL.createObjectURL(b);a.download='digimon-save.json';a.click();setTimeout(()=>URL.revokeObjectURL(a.href),1000);
  });
}
function importSave(){
  const f=$('file').files[0]; if(!f){msg('请选择 JSON 文件');return}
  const rd=new FileReader(); rd.onload=async()=>{
    const r=await fetch('/api/save',{method:'POST',headers:{'Content-Type':'application/json'},body:rd.result});
    if(r.ok){msg('已导入');await load()}else{msg('导入失败 '+r.status)}
  }; rd.readAsText(f);
}
function ota(){
  const f=$('bin').files[0]; if(!f){msg('请选择 .bin 固件');return}
  const x=new XMLHttpRequest(); x.open('POST','/api/ota');
  x.setRequestHeader('Content-Type','application/octet-stream');
  x.upload.onprogress=e=>{if(e.lengthComputable){const p=Math.round(e.loaded*100/e.total);$('prog').value=p;$('log').textContent='上传 '+p+'%'}};
  x.onload=()=>{$('log').textContent='上传完成，设备重启中…';msg('固件已写入')};
  x.onerror=()=>{$('log').textContent='上传失败';msg('OTA 失败')};
  x.send(f);
}
load();
</script></body></html>)HTML";

httpd_handle_t s_server = nullptr;
bool s_wifiInit = false;
bool s_active = false;
bool s_ota = false;
uint32_t s_last = 0;
Digimon *s_pet = nullptr;
uint8_t *s_otaBuf = nullptr;

uint32_t nowMs() { return (uint32_t)(esp_timer_get_time() / 1000); }
void touch() { s_last = nowMs(); }

int32_t jint(cJSON *o, const char *k) {
  cJSON *v = cJSON_GetObjectItem(o, k);
  return v ? (int32_t)cJSON_GetNumberValue(v) : 0;
}

void sendJson(httpd_req_t *req, cJSON *o, const char *status = "200 OK") {
  char *txt = cJSON_PrintUnformatted(o);
  httpd_resp_set_status(req, status);
  httpd_resp_set_type(req, "application/json");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  httpd_resp_sendstr(req, txt ? txt : "{}");
  if (txt) cJSON_free(txt);
}

int readBody(httpd_req_t *req, char *buf, size_t cap) {
  size_t total = req->content_len;
  if (total == 0 || total >= cap) return -1;
  size_t got = 0;
  while (got < total) {
    int r = httpd_req_recv(req, buf + got, total - got);
    if (r == HTTPD_SOCK_ERR_TIMEOUT) continue;
    if (r <= 0) return -1;
    got += (size_t)r;
    touch();
  }
  buf[got] = 0;
  return (int)got;
}

cJSON *buildSave() {
  cJSON *o = cJSON_CreateObject();
  if (!o || !s_pet) return o;
  cJSON_AddNumberToObject(o, "form", s_pet->form);
  cJSON_AddStringToObject(o, "name", s_pet->name());
  cJSON_AddStringToObject(o, "type", dgTypeName(s_pet->type()));
  cJSON_AddNumberToObject(o, "level", s_pet->level());
  cJSON_AddNumberToObject(o, "age", s_pet->ageMin);
  cJSON_AddNumberToObject(o, "exp", s_pet->exp);
  cJSON_AddNumberToObject(o, "weightBonus10", s_pet->weightBonus10);
  cJSON_AddNumberToObject(o, "weight10", s_pet->weight10());
  cJSON_AddNumberToObject(o, "hp", s_pet->hp);
  cJSON_AddNumberToObject(o, "maxHp", s_pet->maxHp());
  cJSON_AddNumberToObject(o, "sp", s_pet->sp);
  cJSON_AddNumberToObject(o, "maxSp", s_pet->maxSp());
  cJSON_AddNumberToObject(o, "wins", s_pet->wins);
  cJSON_AddNumberToObject(o, "power", s_pet->power());
  cJSON_AddNumberToObject(o, "epoch", clockGetEpoch());
  cJSON *tr = cJSON_CreateArray();
  for (int i = 0; i < DG_STAT_N; i++) cJSON_AddItemToArray(tr, cJSON_CreateNumber(s_pet->train[i]));
  cJSON_AddItemToObject(o, "train", tr);
  return o;
}

esp_err_t rootGet(httpd_req_t *req) {
  touch();
  httpd_resp_set_type(req, "text/html; charset=utf-8");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  return httpd_resp_send(req, HTML, HTTPD_RESP_USE_STRLEN);
}

esp_err_t speciesGet(httpd_req_t *req) {
  touch();
  cJSON *a = cJSON_CreateArray();
  for (int i = 0; i < g_spN; i++) cJSON_AddItemToArray(a, cJSON_CreateString(g_sp[i].name));
  sendJson(req, a);
  cJSON_Delete(a);
  return ESP_OK;
}

esp_err_t saveGet(httpd_req_t *req) {
  touch();
  if (!s_pet) {
    cJSON *e = cJSON_CreateObject();
    cJSON_AddBoolToObject(e, "ok", false);
    sendJson(req, e, "503 Service Unavailable");
    cJSON_Delete(e);
    return ESP_OK;
  }
  bsp_display_lock(-1);
  cJSON *o = buildSave();
  bsp_display_unlock();
  sendJson(req, o);
  cJSON_Delete(o);
  return ESP_OK;
}

esp_err_t savePost(httpd_req_t *req) {
  touch();
  if (!s_pet) {
    httpd_resp_sendstr(req, "{\"ok\":false}");
    return ESP_OK;
  }
  char *body = (char *)heap_caps_malloc(4096, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!body) return httpd_resp_send_500(req);
  int bodyLen = readBody(req, body, 4096);
  if (bodyLen <= 0) {
    heap_caps_free(body);
    return httpd_resp_send_500(req);
  }
  cJSON *o = cJSON_Parse(body);
  heap_caps_free(body);
  if (!o) {
    httpd_resp_set_status(req, "400 Bad Request");
    httpd_resp_sendstr(req, "{\"ok\":false}");
    return ESP_OK;
  }

  bsp_display_lock(-1);
  s_pet->form = (uint8_t)jint(o, "form");
  s_pet->ageMin = (uint32_t)jint(o, "age");
  s_pet->exp = (uint32_t)jint(o, "exp");
  s_pet->weightBonus10 = (uint16_t)jint(o, "weightBonus10");
  s_pet->wins = (uint16_t)jint(o, "wins");
  s_pet->hp = (uint16_t)jint(o, "hp");
  s_pet->sp = (uint16_t)jint(o, "sp");
  cJSON *tr = cJSON_GetObjectItem(o, "train");
  if (tr && cJSON_IsArray(tr)) {
    for (int i = 0; i < DG_STAT_N; i++) {
      cJSON *v = cJSON_GetArrayItem(tr, i);
      if (v) s_pet->train[i] = (uint16_t)cJSON_GetNumberValue(v);
    }
  }
  s_pet->sanitize();
  s_pet->save();
  bsp_display_unlock();

  int32_t epoch = jint(o, "epoch");
  if (epoch > 0) clockSetEpoch((uint32_t)epoch);
  cJSON_Delete(o);

  uiRefreshFromWeb();
  httpd_resp_set_type(req, "application/json");
  httpd_resp_sendstr(req, "{\"ok\":true}");
  return ESP_OK;
}

esp_err_t otaPost(httpd_req_t *req) {
  touch();
  if (s_ota) {
    httpd_resp_set_status(req, "409 Conflict");
    httpd_resp_sendstr(req, "{\"ok\":false,\"error\":\"ota busy\"}");
    return ESP_OK;
  }
  if (req->content_len == 0) {
    httpd_resp_set_status(req, "400 Bad Request");
    httpd_resp_sendstr(req, "{\"ok\":false}");
    return ESP_OK;
  }

  const esp_partition_t *part = esp_ota_get_next_update_partition(NULL);
  if (!part || req->content_len > part->size) {
    httpd_resp_set_status(req, "400 Bad Request");
    httpd_resp_sendstr(req, "{\"ok\":false,\"error\":\"partition\"}");
    return ESP_OK;
  }

  s_ota = true;
  s_otaBuf = (uint8_t *)heap_caps_malloc(4096, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!s_otaBuf) { s_ota = false; httpd_resp_send_500(req); return ESP_OK; }
  esp_ota_handle_t h = 0;
  esp_err_t err = esp_ota_begin(part, req->content_len, &h);
  size_t left = req->content_len;
  while (err == ESP_OK && left > 0) {
    size_t want = left > 4096 ? 4096 : left;
    int r = httpd_req_recv(req, (char *)s_otaBuf, want);
    if (r == HTTPD_SOCK_ERR_TIMEOUT) continue;
    if (r <= 0) { err = ESP_FAIL; break; }
    err = esp_ota_write(h, s_otaBuf, (size_t)r);
    left -= (size_t)r;
    touch();
  }
  if (s_otaBuf) { heap_caps_free(s_otaBuf); s_otaBuf = nullptr; }
  if (err == ESP_OK) err = esp_ota_end(h);
  if (err == ESP_OK) err = esp_ota_set_boot_partition(part);

  if (err != ESP_OK) {
    if (h) esp_ota_abort(h);
    s_ota = false;
    ESP_LOGE(TAG, "OTA fallo: %s", esp_err_to_name(err));
    httpd_resp_set_status(req, "500 Internal Server Error");
    httpd_resp_sendstr(req, "{\"ok\":false}");
    return ESP_OK;
  }

  ESP_LOGI(TAG, "OTA OK -> %s, reiniciando", part->label);
  httpd_resp_set_type(req, "application/json");
  httpd_resp_sendstr(req, "{\"ok\":true,\"reboot\":true}");
  vTaskDelay(pdMS_TO_TICKS(1200));
  esp_restart();
  return ESP_OK;
}

const httpd_uri_t URI_ROOT = { "/", HTTP_GET, rootGet, nullptr };
const httpd_uri_t URI_SPECIES = { "/api/species", HTTP_GET, speciesGet, nullptr };
const httpd_uri_t URI_SAVE_GET = { "/api/save", HTTP_GET, saveGet, nullptr };
const httpd_uri_t URI_SAVE_POST = { "/api/save", HTTP_POST, savePost, nullptr };
const httpd_uri_t URI_OTA = { "/api/ota", HTTP_POST, otaPost, nullptr };

}  // namespace

const char *webSsid(void) { return SSID; }
const char *webPassword(void) { return PASS; }

bool webStart(Digimon *pet) {
  if (s_active) return true;
  if (!pet) return false;
  s_pet = pet;

  if (!s_wifiInit) {
    esp_err_t e = esp_netif_init();
    if (e != ESP_OK && e != ESP_ERR_INVALID_STATE) {
      ESP_LOGE(TAG, "netif: %s", esp_err_to_name(e));
      return false;
    }
    e = esp_event_loop_create_default();
    if (e != ESP_OK && e != ESP_ERR_INVALID_STATE) {
      ESP_LOGE(TAG, "event: %s", esp_err_to_name(e));
      return false;
    }
    if (!esp_netif_create_default_wifi_ap()) {
      ESP_LOGE(TAG, "no se pudo crear AP netif");
      return false;
    }
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    e = esp_wifi_init(&cfg);
    if (e != ESP_OK) {
      ESP_LOGE(TAG, "wifi init: %s", esp_err_to_name(e));
      return false;
    }
    s_wifiInit = true;
  }

  wifi_config_t wc = {};
  strncpy((char *)wc.ap.ssid, SSID, sizeof(wc.ap.ssid));
  wc.ap.ssid_len = (uint8_t)strlen(SSID);
  strncpy((char *)wc.ap.password, PASS, sizeof(wc.ap.password));
  wc.ap.channel = 1;
  wc.ap.max_connection = 4;
  wc.ap.authmode = WIFI_AUTH_WPA2_PSK;

  esp_err_t e = esp_wifi_set_mode(WIFI_MODE_AP);
  if (e == ESP_OK) e = esp_wifi_set_config(WIFI_IF_AP, &wc);
  if (e == ESP_OK) e = esp_wifi_start();
  if (e != ESP_OK) {
    ESP_LOGE(TAG, "wifi AP: %s", esp_err_to_name(e));
    esp_wifi_stop();
    return false;
  }

  httpd_config_t hc = HTTPD_DEFAULT_CONFIG();
  hc.max_uri_handlers = 5;
  hc.stack_size = 4096;
  hc.max_open_sockets = 2;
  hc.backlog_conn = 1;
  hc.lru_purge_enable = true;
  hc.recv_wait_timeout = 30;
  hc.send_wait_timeout = 30;
  hc.uri_match_fn = httpd_uri_match_wildcard;
  if (httpd_start(&s_server, &hc) != ESP_OK) {
    ESP_LOGE(TAG, "httpd start fallo");
    esp_wifi_stop();
    s_server = nullptr;
    return false;
  }
  httpd_register_uri_handler(s_server, &URI_ROOT);
  httpd_register_uri_handler(s_server, &URI_SPECIES);
  httpd_register_uri_handler(s_server, &URI_SAVE_GET);
  httpd_register_uri_handler(s_server, &URI_SAVE_POST);
  httpd_register_uri_handler(s_server, &URI_OTA);

  s_active = true;
  s_last = nowMs();
  ESP_LOGI(TAG, "AP '%s' / '%s' http://192.168.4.1", SSID, PASS);
  return true;
}

void webStop(void) {
  if (!s_active && !s_server) return;
  if (s_server) {
    httpd_stop(s_server);
    s_server = nullptr;
  }
  esp_wifi_stop();
  s_active = false;
  ESP_LOGI(TAG, "web detenido");
}

bool webIsActive(void) { return s_active; }
uint32_t webLastActivity(void) { return s_last; }

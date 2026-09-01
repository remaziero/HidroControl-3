#include "provisioning.h"

#include "esp_http_server.h"
#include "esp_log.h"
#include "wificreds.h"
#include "netwifi.h"
#include "deviceid.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <string.h>
#include <stdlib.h>

static const char *TAG = "PROVISION";

static void reboot_task(void *arg)
{
    ESP_LOGI(TAG, "Reinicio programado em 3 segundos...");
    vTaskDelay(pdMS_TO_TICKS(3000));

    ESP_LOGI(TAG, "Reiniciando equipamento...");
    esp_restart();

    vTaskDelete(nullptr);
}

static void url_decode(char *s)
{
    char *src = s;
    char *dst = s;

    while (*src) {
        if (*src == '%' &&
            src[1] && src[2]) {

            char hex[3] = {
                src[1],
                src[2],
                '\0'
            };

            *dst++ = (char)strtol(hex, nullptr, 16);
            src += 3;
        }
        else if (*src == '+') {
            *dst++ = ' ';
            src++;
        }
        else {
            *dst++ = *src++;
        }
    }

    *dst = '\0';
}

static httpd_handle_t s_server = nullptr;

static esp_err_t root_handler(httpd_req_t *req)
{
    static const char html_before[] =
        "<!DOCTYPE html>"
        "<html>"
        "<head>"
        "<meta charset=\"UTF-8\">"
        "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
        "<title>HIDROCONTROL</title>"
        "<style>"
        "body{font-family:Arial,sans-serif;background:#f4f4f4;margin:0;padding:24px;}"
        ".box{max-width:420px;margin:40px auto;background:white;padding:24px;"
        "border-radius:12px;box-shadow:0 2px 10px rgba(0,0,0,.15);}"
        "h1{margin-top:0;text-align:center;}"
        ".status{padding:12px;border-radius:8px;margin:14px 0;line-height:1.45;}"
        ".aguardando{background:#fff8e1;color:#795548;border:1px solid #ffe082;}"
        ".sucesso{background:#e8f5e9;color:#1b5e20;border:1px solid #a5d6a7;}"
        ".erro{background:#ffebee;color:#b71c1c;border:1px solid #ef9a9a;}"
        ".progress{width:100%;height:18px;background:#e0e0e0;border-radius:9px;"
        "overflow:hidden;margin-top:12px;}"
        ".progress-bar{height:100%;width:0%;background:#1565c0;"
        "transition:width .4s ease;}"
        ".progress-text{text-align:center;margin-top:6px;font-size:13px;color:#607d8b;}"
        "label{display:block;margin-top:16px;margin-bottom:6px;font-weight:bold;}"
        "input{width:100%;padding:12px;box-sizing:border-box;border:1px solid #bbb;"
        "border-radius:8px;font-size:16px;}"
        "button{width:100%;margin-top:22px;padding:12px;border:0;border-radius:8px;"
        "font-size:16px;cursor:pointer;}"
        "button:disabled{opacity:.6;cursor:not-allowed;}"
        "</style>"
        "</head>"
        "<body>"
        "<div class=\"box\">"
        "<h1>HIDROCONTROL</h1>"
        "<p>Configuracao Wi-Fi</p>"
        "<div id=\"status\">";

    static const char wifi_error[] =
        "<div class=\"status erro\">"
        "<strong>Nao foi possivel conectar a rede Wi-Fi.</strong><br>"
        "Verifique o nome da rede (SSID) e a senha."
        "</div>";

    static const char html_after[] =
        "</div>"
        "<form id=\"wifiForm\" method=\"POST\" action=\"/save\">"
        "<label for=\"ssid\">Rede Wi-Fi</label>"
        "<input id=\"ssid\" name=\"ssid\" type=\"text\" placeholder=\"Nome da rede\" required>"
        "<button id=\"scanBtn\" type=\"button\" style=\"margin-top:10px;\">Procurar redes Wi-Fi</button>"
        "<div id=\"networkList\" style=\"margin-top:10px;\"></div>"
        "<label for=\"password\">Senha</label>"
        "<input id=\"password\" name=\"password\" type=\"password\" placeholder=\"Senha da rede\">"
        "<label style=\"display:flex;align-items:center;gap:8px;margin:10px 0 18px;\">"
        "<input type=\"checkbox\" "
        "onclick=\"document.getElementById('password').type=this.checked?'text':'password'\">"
        "<span>Visualizar senha</span>"
        "</label>"
        "<button id=\"saveBtn\" type=\"submit\">Salvar configuracao</button>"
        "</form>"

        "<script>"
        "const form=document.getElementById('wifiForm');"
        "const statusBox=document.getElementById('status');"
        "const saveBtn=document.getElementById('saveBtn');"
        "const scanBtn=document.getElementById('scanBtn');"
        "const networkList=document.getElementById('networkList');"
        "const ssidInput=document.getElementById('ssid');"

        "function sinal(rssi){"
        "if(rssi>=-55)return 'Excelente';"
        "if(rssi>=-67)return 'Bom';"
        "if(rssi>=-75)return 'Regular';"
        "return 'Fraco';"
        "}"

        "scanBtn.addEventListener('click',async function(){"
        "scanBtn.disabled=true;"
        "networkList.innerHTML='<div style=\"padding:10px 0;\">Procurando redes...</div>';"
        "try{"
        "const r=await fetch('/scan?ts='+Date.now(),{cache:'no-store'});"
        "if(!r.ok)throw new Error();"
        "const d=await r.json();"
        "networkList.innerHTML='';"
        "if(!d.networks||d.networks.length===0){"
        "networkList.innerHTML='<div style=\"padding:10px 0;\">Nenhuma rede encontrada.</div>';"
        "}else{"
        "d.networks.sort((a,b)=>b.rssi-a.rssi);"
        "d.networks.forEach(n=>{"
        "const b=document.createElement('button');"
        "b.type='button';"
        "b.style.marginTop='6px';"
        "b.textContent=n.ssid+' ('+sinal(n.rssi)+')';"
        "b.onclick=function(){"
"ssidInput.value=n.ssid;"
"networkList.innerHTML='';"
"scanBtn.textContent='Rede selecionada: '+n.ssid;"
"document.getElementById('password').focus();"
"};"
        "networkList.appendChild(b);"
        "});"
        "}"
        "}catch(e){"
        "networkList.innerHTML='<div style=\"padding:10px 0;\">Falha ao procurar redes.</div>';"
        "}"
        "scanBtn.disabled=false;"
        "});"

        "function aguardando(){"
        "statusBox.innerHTML="
        "'<div class=\"status aguardando\">"
        "<strong>Aguardando conexao a rede Wi-Fi...</strong><br>"
        "O dispositivo sera reiniciado e tentara conectar a nova rede."
        "<div class=\"progress\"><div id=\"progressBar\" class=\"progress-bar\"></div></div>"
        "<div id=\"progressText\" class=\"progress-text\">Preparando conexao...</div>"
        "</div>';"
        "}"

        "function sucesso(){"
        "statusBox.innerHTML="
        "'<div class=\"status sucesso\">"
        "<strong>Sucesso! Conectado a rede Wi-Fi.</strong><br>"
        "Voce ja pode acessar <b>http://hidrocontrol</b> pela rede Wi-Fi."
        "</div>';"
        "saveBtn.disabled=false;"
        "}"

        "function falha(){"
        "statusBox.innerHTML="
        "'<div class=\"status erro\">"
        "<strong>Nao foi possivel conectar a rede Wi-Fi.</strong><br>"
        "Verifique o nome da rede (SSID) e a senha."
        "</div>';"
        "saveBtn.disabled=false;"
        "}"

        "async function verificarStatus(){"
        "try{"
        "const r=await fetch('/wifi-status?ts='+Date.now(),{cache:'no-store'});"
        "if(!r.ok)throw new Error();"
        "const d=await r.json();"
        "const bar=document.getElementById('progressBar');"
        "const txt=document.getElementById('progressText');"
        "if(bar&&txt&&d.max_attempts>0){"
        "const tentativa=Math.max(0,d.attempt);"
        "const pct=Math.min(100,(tentativa/d.max_attempts)*100);"
        "bar.style.width=pct+'%';"
        "txt.textContent='Tentativa '+tentativa+' de '+d.max_attempts;"
        "}"
        "if(d.connected){sucesso();return;}"
        "if(d.failed){falha();return;}"
        "}catch(e){}"
        "setTimeout(verificarStatus,1500);"
        "}"

        "form.addEventListener('submit',async function(e){"
        "e.preventDefault();"
        "saveBtn.disabled=true;"
        "aguardando();"
        "const dados=new URLSearchParams(new FormData(form));"
        "try{"
        "const r=await fetch('/save',{"
        "method:'POST',"
        "headers:{'Content-Type':'application/x-www-form-urlencoded'},"
        "body:dados.toString()"
        "});"
        "if(!r.ok)throw new Error();"
        "setTimeout(verificarStatus,7000);"
        "}catch(e){"
        "statusBox.innerHTML="
        "'<div class=\"status erro\">"
        "<strong>Falha ao enviar a configuracao.</strong><br>"
        "Tente novamente."
        "</div>';"
        "saveBtn.disabled=false;"
        "}"
        "});"
        "</script>"

        "</div>"
        "</body>"
        "</html>";

    httpd_resp_set_type(req, "text/html; charset=utf-8");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");

    esp_err_t err = httpd_resp_send_chunk(
        req,
        html_before,
        HTTPD_RESP_USE_STRLEN
    );

    if (err != ESP_OK) {
        return err;
    }

    if (netwifi_connection_failed()) {
        err = httpd_resp_send_chunk(
            req,
            wifi_error,
            HTTPD_RESP_USE_STRLEN
        );

        if (err != ESP_OK) {
            return err;
        }
    }

    err = httpd_resp_send_chunk(
        req,
        html_after,
        HTTPD_RESP_USE_STRLEN
    );

    if (err != ESP_OK) {
        return err;
    }

    return httpd_resp_send_chunk(req, nullptr, 0);
}

static bool maintenance_authorized(httpd_req_t *req)
{
    size_t len = httpd_req_get_hdr_value_len(req, "Cookie");

    if (len == 0 || len >= 128) {
        return false;
    }

    char cookie[128] = {0};

    if (httpd_req_get_hdr_value_str(
            req,
            "Cookie",
            cookie,
            sizeof(cookie)
        ) != ESP_OK) {
        return false;
    }

    return strstr(cookie, "HCSESSION=autorizado") != nullptr;
}

static esp_err_t login_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "GET /login recebido");

    static const char html[] =
        "<!DOCTYPE html>"
        "<html>"
        "<head>"
        "<meta charset=\"UTF-8\">"
        "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
        "<title>HIDROCONTROL</title>"
        "<style>"
        "*{box-sizing:border-box;}"
        "body{font-family:Arial,sans-serif;background:#eef2f5;margin:0;padding:20px;color:#263238;}"
        ".box{max-width:430px;margin:40px auto;background:#fff;padding:28px;"
        "border-radius:16px;box-shadow:0 4px 18px rgba(0,0,0,.12);}"
        "h1{text-align:center;margin:0;color:#1565c0;font-size:28px;}"
        ".subtitle{text-align:center;color:#607d8b;margin:6px 0 26px;font-size:18px;}"
        "label{display:block;margin-bottom:6px;color:#546e7a;font-size:14px;}"
        "input[type=text],input[type=password]{width:100%;padding:13px 12px;"
        "border:1px solid #cfd8dc;border-radius:8px;font-size:16px;outline:none;}"
        "input[type=text]:focus,input[type=password]:focus{border-color:#1565c0;}"
        ".field{margin-bottom:18px;}"
        ".showpass{display:flex;align-items:center;gap:8px;margin:4px 0 22px;"
        "color:#546e7a;font-size:14px;}"
        ".showpass input{width:18px;height:18px;}"
        "button{width:100%;padding:14px;border:0;border-radius:9px;"
        "background:#1565c0;color:#fff;font-size:16px;font-weight:bold;cursor:pointer;}"
        ".footer{text-align:left;margin-top:30px;padding-top:18px;"
        "border-top:1px solid #e0e0e0;color:#78909c;font-size:12px;}"
        "</style>"
        "</head>"
        "<body>"
        "<div class=\"box\">"
        "<h1>HIDROCONTROL</h1>"
        "<div class=\"subtitle\">Acesso a manutencao</div>"
        "<form method=\"POST\" action=\"/login\">"
        "<div class=\"field\">"
        "<label for=\"usuario\">Usuario</label>"
        "<input id=\"usuario\" name=\"username\" type=\"text\" autocomplete=\"username\" required>"
        "</div>"
        "<div class=\"field\">"
        "<label for=\"senha\">Senha</label>"
        "<input id=\"senha\" name=\"password\" type=\"password\" autocomplete=\"current-password\" required>"
        "</div>"
        "<label class=\"showpass\">"
        "<input type=\"checkbox\" "
        "onclick=\"document.getElementById('senha').type=this.checked?'text':'password'\">"
        "<span>Visualizar senha</span>"
        "</label>"
        "<button type=\"submit\">ENTRAR</button>"
        "</form>"
        "<div class=\"footer\">Desenvolvido por Eng. Renato M. Pedrosa</div>"
        "</div>"
        "</body>"
        "</html>";

    httpd_resp_set_type(req, "text/html; charset=utf-8");
    return httpd_resp_send(req, html, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t login_post_handler(httpd_req_t *req)
{
    if (req->content_len <= 0 || req->content_len >= 128) {
        httpd_resp_set_status(req, "400 Bad Request");
        return httpd_resp_send(
            req,
            "Requisicao invalida",
            HTTPD_RESP_USE_STRLEN
        );
    }

    char body[128] = {0};
    int received = 0;

    while (received < req->content_len) {
        int ret = httpd_req_recv(
            req,
            body + received,
            req->content_len - received
        );

        if (ret <= 0) {
            httpd_resp_set_status(req, "400 Bad Request");
            return httpd_resp_send(
                req,
                "Falha ao receber dados",
                HTTPD_RESP_USE_STRLEN
            );
        }

        received += ret;
    }

    body[received] = '\0';

    if (strcmp(body, "username=Admin&password=Admin") != 0) {
        httpd_resp_set_status(req, "401 Unauthorized");
        httpd_resp_set_type(req, "text/html; charset=utf-8");

        return httpd_resp_send(
            req,
            "<!DOCTYPE html>"
            "<html><head>"
            "<meta charset=\"UTF-8\">"
            "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
            "<title>Acesso negado</title>"
            "</head><body style=\"font-family:Arial;text-align:center;padding:40px\">"
            "<h2>Usuario ou senha incorretos</h2>"
            "<p><a href=\"/login\">Tentar novamente</a></p>"
            "</body></html>",
            HTTPD_RESP_USE_STRLEN
        );
    }

    httpd_resp_set_hdr(
        req,
        "Set-Cookie",
        "HCSESSION=autorizado; Path=/; HttpOnly; SameSite=Strict"
    );

    httpd_resp_set_status(req, "303 See Other");
    httpd_resp_set_hdr(req, "Location", "/manutencao");

    return httpd_resp_send(req, nullptr, 0);
}

static esp_err_t maintenance_handler(httpd_req_t *req)
{
    if (!maintenance_authorized(req)) {
        httpd_resp_set_status(req, "303 See Other");
        httpd_resp_set_hdr(req, "Location", "/login");
        return httpd_resp_send(req, nullptr, 0);
    }

    const bool connected = netwifi_is_connected();
    const char *sta_ip = netwifi_ip();
    const char *device_id = deviceid_get();

    static char html[3000];

    snprintf(
        html,
        sizeof(html),
        "<!DOCTYPE html>"
        "<html>"
        "<head>"
        "<meta charset=\"UTF-8\">"
        "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
        "<title>HIDROCONTROL - Manutencao</title>"
        "<style>"
        "*{box-sizing:border-box;}"
        "body{font-family:Arial,sans-serif;background:#eef2f5;margin:0;padding:20px;color:#263238;}"
        ".box{max-width:430px;margin:30px auto;background:#fff;padding:28px;"
        "border-radius:16px;box-shadow:0 4px 18px rgba(0,0,0,.12);}"
        "h1{text-align:center;margin:0;color:#1565c0;font-size:28px;}"
        ".subtitle{text-align:center;color:#607d8b;margin:6px 0 26px;font-size:18px;}"
        ".card{background:#f7f9fa;border:1px solid #e0e6ea;border-radius:10px;"
        "padding:15px;margin-bottom:14px;}"
        ".label{font-size:13px;color:#78909c;margin-bottom:6px;}"
        ".value{font-size:17px;font-weight:bold;word-break:break-word;}"
        ".status{font-size:17px;font-weight:bold;}"
        ".dot{display:inline-block;width:10px;height:10px;border-radius:50%%;"
        "background:%s;margin-right:7px;}"
        ".button{display:block;text-align:center;margin-top:22px;padding:14px;"
        "border-radius:9px;background:#1565c0;color:white;text-decoration:none;"
        "font-size:16px;font-weight:bold;}"
        ".footer{text-align:left;margin-top:32px;padding-top:18px;"
        "border-top:1px solid #e0e0e0;color:#78909c;font-size:12px;line-height:1.7;}"
        "</style>"
        "</head>"
        "<body>"
        "<div class=\"box\">"
        "<h1>HIDROCONTROL</h1>"
        "<div class=\"subtitle\">Manutencao local</div>"

        "<div class=\"card\">"
        "<div class=\"label\">Dispositivo</div>"
        "<div class=\"value\">%s</div>"
        "</div>"

        "<div class=\"card\">"
        "<div class=\"label\">Conexao Wi-Fi</div>"
        "<div class=\"status\"><span class=\"dot\"></span>STA %s</div>"
        "</div>"

        "<div class=\"card\">"
        "<div class=\"label\">IP da rede STA</div>"
        "<div class=\"value\">%s</div>"
        "</div>"

        "<a class=\"button\" href=\"/\">Configurar rede Wi-Fi</a>"

        "<div class=\"footer\">"
        "<div>Desenvolvido por Eng. Renato M. Pedrosa</div>"
        "</div>"

        "</div>"
        "</body>"
        "</html>",
        connected ? "#2e7d32" : "#c62828",
        device_id,
        connected ? "Conectado" : "Desconectado",
        sta_ip
    );

    httpd_resp_set_type(
        req,
        "text/html; charset=utf-8"
    );

    return httpd_resp_send(
        req,
        html,
        HTTPD_RESP_USE_STRLEN
    );
}


static esp_err_t wifi_scan_handler(httpd_req_t *req)
{
    wifi_scan_config_t scan_config = {};
    scan_config.show_hidden = false;

    ESP_LOGI(TAG, "Iniciando scan de redes Wi-Fi...");

    esp_err_t err = esp_wifi_scan_start(&scan_config, true);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Falha no scan Wi-Fi: %s", esp_err_to_name(err));

        httpd_resp_set_type(req, "application/json");
        httpd_resp_set_status(req, "500 Internal Server Error");
        return httpd_resp_sendstr(req, "{\"error\":\"scan_failed\"}");
    }

    uint16_t count = 0;
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&count));

    if (count > 20) {
        count = 20;
    }

    static wifi_ap_record_t records[20] = {};

    if (count > 0) {
        ESP_ERROR_CHECK(
            esp_wifi_scan_get_ap_records(&count, records)
        );
    }

    static char json[2048];
    size_t used = 0;

    used += snprintf(
        json + used,
        sizeof(json) - used,
        "{\"networks\":["
    );

    for (uint16_t i = 0; i < count; i++) {
        if (records[i].ssid[0] == '\0') {
            continue;
        }

        bool duplicate = false;

        for (uint16_t j = 0; j < i; j++) {
            if (strcmp(
                    (const char *)records[i].ssid,
                    (const char *)records[j].ssid
                ) == 0) {
                duplicate = true;
                break;
            }
        }

        if (duplicate) {
            continue;
        }

        if (used > 20 && json[used - 1] != '[') {
            used += snprintf(
                json + used,
                sizeof(json) - used,
                ","
            );
        }

        used += snprintf(
            json + used,
            sizeof(json) - used,
            "{\"ssid\":\"%s\",\"rssi\":%d}",
            records[i].ssid,
            records[i].rssi
        );

        if (used >= sizeof(json) - 100) {
            break;
        }
    }

    snprintf(
        json + used,
        sizeof(json) - used,
        "]}"
    );

    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");

    return httpd_resp_sendstr(req, json);
}

static esp_err_t wifi_status_handler(httpd_req_t *req)
{
    static char json[128];

    snprintf(
        json,
        sizeof(json),
        "{\"connected\":%s,\"failed\":%s,\"attempt\":%d,\"max_attempts\":%d,\"ip\":\"%s\"}",
        netwifi_is_connected() ? "true" : "false",
        netwifi_connection_failed() ? "true" : "false",
        netwifi_attempt(),
        netwifi_max_attempts(),
        netwifi_ip()
    );

    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");

    return httpd_resp_send(
        req,
        json,
        HTTPD_RESP_USE_STRLEN
    );
}


static esp_err_t save_handler(httpd_req_t *req)
{
    char body[160] = {0};

    if (req->content_len <= 0 ||
        req->content_len >= (int)sizeof(body)) {
        httpd_resp_send_err(
            req,
            HTTPD_400_BAD_REQUEST,
            "Dados invalidos"
        );
        return ESP_FAIL;
    }

    int received = httpd_req_recv(
        req,
        body,
        req->content_len
    );

    if (received <= 0) {
        httpd_resp_send_err(
            req,
            HTTPD_400_BAD_REQUEST,
            "Falha ao receber dados"
        );
        return ESP_FAIL;
    }

    body[received] = '\0';

    char ssid[33] = {0};
    char password[65] = {0};

    esp_err_t err = httpd_query_key_value(
        body,
        "ssid",
        ssid,
        sizeof(ssid)
    );

    if (err != ESP_OK || ssid[0] == '\0') {
        httpd_resp_send_err(
            req,
            HTTPD_400_BAD_REQUEST,
            "SSID invalido"
        );
        return ESP_FAIL;
    }

    httpd_query_key_value(
        body,
        "password",
        password,
        sizeof(password)
    );

    url_decode(ssid);
    url_decode(password);

    if (!wificreds_save(ssid, password)) {
        httpd_resp_send_err(
            req,
            HTTPD_500_INTERNAL_SERVER_ERROR,
            "Falha ao salvar configuracao"
        );
        return ESP_FAIL;
    }

    ESP_LOGI(
        TAG,
        "Configuracao WiFi recebida e salva para SSID: %s",
        ssid
    );

    httpd_resp_set_type(
        req,
        "application/json"
    );

    httpd_resp_set_hdr(
        req,
        "Cache-Control",
        "no-store"
    );

    esp_err_t send_result = httpd_resp_send(
        req,
        "{\"saved\":true}",
        HTTPD_RESP_USE_STRLEN
    );

    if (send_result == ESP_OK) {
        BaseType_t task_result = xTaskCreate(
            reboot_task,
            "reboot_task",
            2048,
            nullptr,
            5,
            nullptr
        );

        if (task_result != pdPASS) {
            ESP_LOGE(TAG, "Falha ao criar tarefa de reinicio");
        }
    }

    return send_result;
}

void provisioning_start()
{
    if (s_server != nullptr) {
        return;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    esp_err_t err = httpd_start(&s_server, &config);

    if (err != ESP_OK) {
        ESP_LOGE(
            TAG,
            "Falha ao iniciar servidor HTTP: %s",
            esp_err_to_name(err)
        );
        s_server = nullptr;
        return;
    }

    httpd_uri_t root_uri = {};
    root_uri.uri = "/";
    root_uri.method = HTTP_GET;
    root_uri.handler = root_handler;

    ESP_ERROR_CHECK(
        httpd_register_uri_handler(
            s_server,
            &root_uri
        )
    );

    httpd_uri_t login_uri = {};
    login_uri.uri = "/login";
    login_uri.method = HTTP_GET;
    login_uri.handler = login_handler;

    ESP_ERROR_CHECK(
        httpd_register_uri_handler(
            s_server,
            &login_uri
        )
    );

    httpd_uri_t login_post_uri = {};
    login_post_uri.uri = "/login";
    login_post_uri.method = HTTP_POST;
    login_post_uri.handler = login_post_handler;

    ESP_ERROR_CHECK(
        httpd_register_uri_handler(
            s_server,
            &login_post_uri
        )
    );

    httpd_uri_t maintenance_uri = {};
    maintenance_uri.uri = "/manutencao";
    maintenance_uri.method = HTTP_GET;
    maintenance_uri.handler = maintenance_handler;

    ESP_ERROR_CHECK(
        httpd_register_uri_handler(
            s_server,
            &maintenance_uri
        )
    );

    httpd_uri_t scan_uri = {};
    scan_uri.uri = "/scan";
    scan_uri.method = HTTP_GET;
    scan_uri.handler = wifi_scan_handler;

    ESP_ERROR_CHECK(
        httpd_register_uri_handler(
            s_server,
            &scan_uri
        )
    );

    httpd_uri_t wifi_status_uri = {};
    wifi_status_uri.uri = "/wifi-status";
    wifi_status_uri.method = HTTP_GET;
    wifi_status_uri.handler = wifi_status_handler;

    ESP_ERROR_CHECK(
        httpd_register_uri_handler(
            s_server,
            &wifi_status_uri
        )
    );

    httpd_uri_t save_uri = {};
    save_uri.uri = "/save";
    save_uri.method = HTTP_POST;
    save_uri.handler = save_handler;

    ESP_ERROR_CHECK(
        httpd_register_uri_handler(
            s_server,
            &save_uri
        )
    );

    ESP_LOGI(
        TAG,
        "Servidor HTTP de provisionamento iniciado"
    );
}

void provisioning_stop()
{
    if (s_server == nullptr) {
        return;
    }

    httpd_stop(s_server);
    s_server = nullptr;

    ESP_LOGI(
        TAG,
        "Servidor HTTP de provisionamento encerrado"
    );
}

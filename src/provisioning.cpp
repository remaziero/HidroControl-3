#include "provisioning.h"

#include "esp_http_server.h"
#include "esp_log.h"
#include "wificreds.h"
#include "netwifi.h"

#include <string.h>
#include <stdlib.h>

static const char *TAG = "PROVISION";

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
    const char *html =
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
        "label{display:block;margin-top:16px;margin-bottom:6px;font-weight:bold;}"
        "input{width:100%;padding:12px;box-sizing:border-box;border:1px solid #bbb;"
        "border-radius:8px;font-size:16px;}"
        "button{width:100%;margin-top:22px;padding:12px;border:0;border-radius:8px;"
        "font-size:16px;cursor:pointer;}"
        "</style>"
        "</head>"
        "<body>"
        "<div class=\"box\">"
        "<h1>HIDROCONTROL</h1>"
        "<p>Configuracao Wi-Fi</p>"
        "<form method=\"POST\" action=\"/save\">"
        "<label for=\"ssid\">Rede Wi-Fi</label>"
        "<input id=\"ssid\" name=\"ssid\" type=\"text\" placeholder=\"Nome da rede\" required>"
        "<label for=\"password\">Senha</label>"
        "<input id=\"password\" name=\"password\" type=\"password\" placeholder=\"Senha da rede\">"
        "<button type=\"submit\">Salvar configuracao</button>"
        "</form>"
        "</div>"
        "</body>"
        "</html>";

    httpd_resp_set_type(req, "text/html; charset=utf-8");
    return httpd_resp_send(req, html, HTTPD_RESP_USE_STRLEN);
}


static esp_err_t maintenance_handler(httpd_req_t *req)
{
    const bool connected = netwifi_is_connected();
    const char *sta_ip = netwifi_ip();

    char html[1800];

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
        "body{font-family:Arial,sans-serif;background:#f4f4f4;margin:0;padding:24px;}"
        ".box{max-width:420px;margin:40px auto;background:white;padding:24px;"
        "border-radius:12px;box-shadow:0 2px 10px rgba(0,0,0,.15);}"
        "h1{text-align:center;margin-top:0;}"
        ".status{padding:12px;background:#f0f0f0;border-radius:8px;margin:12px 0;}"
        "a{display:block;text-align:center;margin-top:22px;padding:12px;"
        "border-radius:8px;background:#ddd;text-decoration:none;color:#000;}"
        "</style>"
        "</head>"
        "<body>"
        "<div class=\"box\">"
        "<h1>HIDROCONTROL</h1>"
        "<h2>Manutencao local</h2>"
        "<div class=\"status\"><b>STA:</b> %s</div>"
        "<div class=\"status\"><b>IP STA:</b> %s</div>"
        "<a href=\"/\">Configurar rede Wi-Fi</a>"
        "</div>"
        "</body>"
        "</html>",
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

    const char *html =
        "<!DOCTYPE html>"
        "<html>"
        "<head>"
        "<meta charset=\"UTF-8\">"
        "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
        "<title>HIDROCONTROL</title>"
        "</head>"
        "<body>"
        "<h1>HIDROCONTROL</h1>"
        "<p>Configuracao Wi-Fi salva com sucesso.</p>"
        "<p>O dispositivo ainda nao sera reiniciado nesta etapa.</p>"
        "</body>"
        "</html>";

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

    httpd_uri_t maintenance_uri = {};
    maintenance_uri.uri = "/maintenance";
    maintenance_uri.method = HTTP_GET;
    maintenance_uri.handler = maintenance_handler;

    ESP_ERROR_CHECK(
        httpd_register_uri_handler(
            s_server,
            &maintenance_uri
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

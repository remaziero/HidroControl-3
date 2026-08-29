#include "wificreds.h"

#include <string.h>

#include "esp_log.h"
#include "nvs.h"

static const char *TAG = "WIFICREDS";

static constexpr const char *NVS_NAMESPACE = "wifi_cfg";
static constexpr const char *KEY_SSID      = "ssid";
static constexpr const char *KEY_PASSWORD  = "password";

bool wificreds_exists()
{
    nvs_handle_t handle;

    esp_err_t err = nvs_open(
        NVS_NAMESPACE,
        NVS_READONLY,
        &handle
    );

    if (err != ESP_OK) {
        return false;
    }

    size_t required_size = 0;

    err = nvs_get_str(
        handle,
        KEY_SSID,
        nullptr,
        &required_size
    );

    nvs_close(handle);

    return err == ESP_OK && required_size > 1;
}

bool wificreds_load(
    char *ssid,
    size_t ssid_len,
    char *password,
    size_t password_len)
{
    if (ssid == nullptr || ssid_len == 0 ||
        password == nullptr || password_len == 0) {
        return false;
    }

    nvs_handle_t handle;

    esp_err_t err = nvs_open(
        NVS_NAMESPACE,
        NVS_READONLY,
        &handle
    );

    if (err != ESP_OK) {
        return false;
    }

    size_t ssid_size = ssid_len;
    size_t password_size = password_len;

    err = nvs_get_str(
        handle,
        KEY_SSID,
        ssid,
        &ssid_size
    );

    if (err == ESP_OK) {
        err = nvs_get_str(
            handle,
            KEY_PASSWORD,
            password,
            &password_size
        );
    }

    nvs_close(handle);

    if (err != ESP_OK) {
        ssid[0] = '\0';
        password[0] = '\0';
        return false;
    }

    return ssid[0] != '\0';
}

bool wificreds_save(
    const char *ssid,
    const char *password)
{
    if (ssid == nullptr || ssid[0] == '\0' ||
        password == nullptr) {
        return false;
    }

    nvs_handle_t handle;

    esp_err_t err = nvs_open(
        NVS_NAMESPACE,
        NVS_READWRITE,
        &handle
    );

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao abrir NVS: %s",
                 esp_err_to_name(err));
        return false;
    }

    err = nvs_set_str(handle, KEY_SSID, ssid);

    if (err == ESP_OK) {
        err = nvs_set_str(
            handle,
            KEY_PASSWORD,
            password
        );
    }

    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }

    nvs_close(handle);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao salvar credenciais: %s",
                 esp_err_to_name(err));
        return false;
    }

    ESP_LOGI(TAG, "Credenciais WiFi salvas no NVS");
    return true;
}

bool wificreds_clear()
{
    nvs_handle_t handle;

    esp_err_t err = nvs_open(
        NVS_NAMESPACE,
        NVS_READWRITE,
        &handle
    );

    if (err != ESP_OK) {
        return false;
    }

    err = nvs_erase_all(handle);

    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }

    nvs_close(handle);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao apagar credenciais: %s",
                 esp_err_to_name(err));
        return false;
    }

    ESP_LOGI(TAG, "Credenciais WiFi removidas do NVS");
    return true;
}

#include "oled.h"
#include "hardware.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/i2c_master.h"
#include "esp_log.h"

static const char *TAG = "OLED";

static i2c_master_bus_handle_t s_i2c_bus = nullptr;
static i2c_master_dev_handle_t s_oled_dev = nullptr;

static uint8_t s_buffer[OLED_WIDTH * OLED_HEIGHT / 8];
static bool s_ok = false;

static esp_err_t oled_write_cmd(uint8_t cmd) {
    uint8_t data[2] = {0x00, cmd};
    return i2c_master_transmit(s_oled_dev, data, sizeof(data), 100);
}

static esp_err_t oled_write_data(const uint8_t* data, size_t len) {
    uint8_t tx[17];
    size_t offset = 0;

    while (offset < len) {
        size_t chunk = len - offset;
        if (chunk > 16) chunk = 16;

        tx[0] = 0x40;
        memcpy(&tx[1], &data[offset], chunk);

        esp_err_t err = i2c_master_transmit(s_oled_dev, tx, chunk + 1, 100);
        if (err != ESP_OK) return err;

        offset += chunk;
    }

    return ESP_OK;
}

static void oled_clear_buffer() {
    memset(s_buffer, 0x00, sizeof(s_buffer));
}

static void oled_pixel(int x, int y, bool on = true) {
    if (x < 0 || x >= OLED_WIDTH || y < 0 || y >= OLED_HEIGHT) return;

    uint16_t index = x + (y / 8) * OLED_WIDTH;
    uint8_t bit = 1 << (y & 7);

    if (on) s_buffer[index] |= bit;
    else    s_buffer[index] &= ~bit;
}

static void oled_line(int x0, int y0, int x1, int y1) {
    int dx = abs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    while (true) {
        oled_pixel(x0, y0);

        if (x0 == x1 && y0 == y1) break;

        int e2 = 2 * err;

        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }

        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

static const uint8_t* font5x7(char c) {
    static const uint8_t sp[5] = {0,0,0,0,0};
    static const uint8_t eq[5] = {0x14,0x14,0x14,0x14,0x14};
    static const uint8_t dot[5] = {0x00,0x60,0x60,0x00,0x00};
    static const uint8_t colon[5] = {0x00,0x36,0x36,0x00,0x00};
    static const uint8_t slash[5] = {0x20,0x10,0x08,0x04,0x02};

    static const uint8_t n0[5] = {0x3E,0x51,0x49,0x45,0x3E};
    static const uint8_t n1[5] = {0x00,0x42,0x7F,0x40,0x00};
    static const uint8_t n2[5] = {0x42,0x61,0x51,0x49,0x46};
    static const uint8_t n3[5] = {0x21,0x41,0x45,0x4B,0x31};
    static const uint8_t n4[5] = {0x18,0x14,0x12,0x7F,0x10};
    static const uint8_t n5[5] = {0x27,0x45,0x45,0x45,0x39};
    static const uint8_t n6[5] = {0x3C,0x4A,0x49,0x49,0x30};
    static const uint8_t n7[5] = {0x01,0x71,0x09,0x05,0x03};
    static const uint8_t n8[5] = {0x36,0x49,0x49,0x49,0x36};
    static const uint8_t n9[5] = {0x06,0x49,0x49,0x29,0x1E};

    static const uint8_t A[5] = {0x7E,0x11,0x11,0x11,0x7E};
    static const uint8_t D[5] = {0x7F,0x41,0x41,0x22,0x1C};
    static const uint8_t E[5] = {0x7F,0x49,0x49,0x49,0x41};
    static const uint8_t F[5] = {0x7F,0x09,0x09,0x09,0x01};
    static const uint8_t I[5] = {0x00,0x41,0x7F,0x41,0x00};
    static const uint8_t L[5] = {0x7F,0x40,0x40,0x40,0x40};
    static const uint8_t M[5] = {0x7F,0x02,0x0C,0x02,0x7F};
    static const uint8_t N[5] = {0x7F,0x04,0x08,0x10,0x7F};
    static const uint8_t O[5] = {0x3E,0x41,0x41,0x41,0x3E};
    static const uint8_t Q[5] = {0x3E,0x41,0x51,0x21,0x5E};
    static const uint8_t R[5] = {0x7F,0x09,0x19,0x29,0x46};
    static const uint8_t T[5] = {0x01,0x01,0x7F,0x01,0x01};
    static const uint8_t U[5] = {0x3F,0x40,0x40,0x40,0x3F};
    static const uint8_t W[5] = {0x7F,0x20,0x18,0x20,0x7F};
    static const uint8_t X[5] = {0x63,0x14,0x08,0x14,0x63};

    switch (c) {
        case ' ': return sp;
        case '=': return eq;
        case '.': return dot;
        case ':': return colon;
        case '/': return slash;

        case '0': return n0;
        case '1': return n1;
        case '2': return n2;
        case '3': return n3;
        case '4': return n4;
        case '5': return n5;
        case '6': return n6;
        case '7': return n7;
        case '8': return n8;
        case '9': return n9;

        case 'A': return A;
        case 'D': return D;
        case 'E': return E;
        case 'F': return F;
        case 'I': return I;
        case 'L': return L;
        case 'M': return M;
        case 'N': return N;
        case 'O': return O;
        case 'Q': return Q;
        case 'R': return R;
        case 'T': return T;
        case 'U': return U;
        case 'W': return W;
        case 'X': return X;

        default: return sp;
    }
}

static void oled_char(int x, int y, char c) {
    const uint8_t* f = font5x7(c);

    for (int col = 0; col < 5; col++) {
        uint8_t line = f[col];

        for (int row = 0; row < 7; row++) {
            if (line & (1 << row)) {
                oled_pixel(x + col, y + row);
            }
        }
    }
}

static void oled_text(int x, int y, const char* txt) {
    while (*txt) {
        oled_char(x, y, *txt);
        x += 6;
        txt++;
    }
}

static void oled_icon_wifi(int x, int y, bool ok) {
    // Antena/base
    oled_pixel(x + 6, y + 7);

    // Ondas simples
    oled_line(x + 3, y + 5, x + 6, y + 2);
    oled_line(x + 6, y + 2, x + 9, y + 5);

    oled_line(x + 1, y + 7, x + 6, y + 1);
    oled_line(x + 6, y + 1, x + 11, y + 7);

    if (!ok) {
        oled_line(x, y, x + 11, y + 7);
        oled_line(x + 11, y, x, y + 7);
    }
}

static void oled_icon_mqtt(int x, int y, bool ok) {
    // Três nós conectados
    oled_pixel(x + 1, y + 6);
    oled_pixel(x + 6, y + 1);
    oled_pixel(x + 11, y + 6);

    oled_line(x + 1, y + 6, x + 6, y + 1);
    oled_line(x + 6, y + 1, x + 11, y + 6);
    oled_line(x + 1, y + 6, x + 11, y + 6);

    if (!ok) {
        oled_line(x, y, x + 11, y + 7);
        oled_line(x + 11, y, x, y + 7);
    }
}

static void oled_flush() {
    for (uint8_t page = 0; page < 8; page++) {
        oled_write_cmd(0xB0 + page);
        oled_write_cmd(0x00);
        oled_write_cmd(0x10);
        oled_write_data(&s_buffer[OLED_WIDTH * page], OLED_WIDTH);
    }
}

void oled_init() {
    i2c_master_bus_config_t bus_config = {};
    bus_config.i2c_port = OLED_I2C_PORT;
    bus_config.sda_io_num = PIN_OLED_SDA;
    bus_config.scl_io_num = PIN_OLED_SCL;
    bus_config.clk_source = I2C_CLK_SRC_DEFAULT;
    bus_config.glitch_ignore_cnt = 7;
    bus_config.flags.enable_internal_pullup = true;

    esp_err_t err = i2c_new_master_bus(&bus_config, &s_i2c_bus);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao criar barramento I2C: %s", esp_err_to_name(err));
        return;
    }

    i2c_device_config_t dev_config = {};
    dev_config.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    dev_config.device_address = OLED_I2C_ADDR;
    dev_config.scl_speed_hz = OLED_I2C_FREQ_HZ;

    err = i2c_master_bus_add_device(s_i2c_bus, &dev_config, &s_oled_dev);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao adicionar OLED I2C: %s", esp_err_to_name(err));
        return;
    }

    vTaskDelay(pdMS_TO_TICKS(100));

    const uint8_t init_cmds[] = {
        0xAE, 0x20, 0x00, 0xB0, 0xC8, 0x00, 0x10,
        0x40, 0x81, 0x7F, 0xA1, 0xA6, 0xA8, 0x3F,
        0xA4, 0xD3, 0x00, 0xD5, 0x80, 0xD9, 0xF1,
        0xDA, 0x12, 0xDB, 0x40, 0x8D, 0x14, 0xAF
    };

    for (uint8_t cmd : init_cmds) {
        oled_write_cmd(cmd);
    }

    oled_clear_buffer();
    oled_flush();

    s_ok = true;

    ESP_LOGI(TAG, "OLED SSD1306 inicializado");
}

void oled_show_status(bool wifi_connected,
                      bool mqtt_connected,
                      const char* modo,
                      float fluxo_frio,
                      float fluxo_quente) {
    if (!s_ok) return;

    oled_clear_buffer();

    char linha[32];

    // Linha 0: ícones + modo
    oled_icon_wifi(0, 0, wifi_connected);
    oled_text(14, 0, "WIFI");

    oled_icon_mqtt(48, 0, mqtt_connected);
    oled_text(62, 0, "MQTT");

    snprintf(linha, sizeof(linha), "M:%s", modo);
    oled_text(0, 18, linha);

    snprintf(linha, sizeof(linha), "F=%.2f L/MIN", fluxo_frio);
    oled_text(0, 34, linha);

    snprintf(linha, sizeof(linha), "Q=%.2f L/MIN", fluxo_quente);
    oled_text(0, 50, linha);

    oled_flush();
}
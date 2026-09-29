// main/main.c —— 塔罗牌占卜固件入口(78 张牌 · 单张/三牌阵 · 正逆位,衍生应用,独立 UI)。
//
// 启动序列:I2C → 显示/LVGL → 背光 → 电量计 → 塔罗应用任务。
// 本固件不复用 baseline 硬件测试 demo 的菜单外壳;按键语义由塔罗应用自定义:
//   上/下 短按  页面内选择/滚动
//   确定  短按  确认/抽牌/翻牌
//   确定  长按  返回封面
#include "bsp_battery.h"
#include "bsp_display.h"
#include "bsp_i2c.h"
#include "bsp_pins.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "tarot_app.h"

static const char *TAG = "main";

#define TAROT_BACKLIGHT_PERCENT 100

void app_main(void) {
    ESP_LOGI(TAG, "塔罗占卜固件启动(ESP32-C3 AI Passport)");
    esp_sleep_wakeup_cause_t wakeup = esp_sleep_get_wakeup_cause();
    if (wakeup != ESP_SLEEP_WAKEUP_UNDEFINED) {
        ESP_LOGI(TAG, "休眠唤醒原因: %d", wakeup);
    }

    // I2C 供电量计(CW2017)使用:失败只告警,电量显示退回 "--",不阻塞启动。
    esp_err_t i2c_err = bsp_i2c_init();
    if (i2c_err != ESP_OK) {
        ESP_LOGW(TAG, "I2C 初始化失败(0x%x):电量计不可用", i2c_err);
    }

    // 显示是本应用的载体,失败无 UI 可言:打日志后退出。
    if (bsp_display_init() != ESP_OK || !bsp_lvgl_init()) {
        ESP_LOGE(TAG, "显示/LVGL 初始化失败。"
                      "检查 SPI 接线(MOSI=%d SCLK=%d CS=%d DC=%d BL=%d)",
                 BSP_LCD_MOSI, BSP_LCD_SCLK, BSP_LCD_CS, BSP_LCD_DC, BSP_LCD_BL);
        return;
    }
    bsp_display_backlight(TAROT_BACKLIGHT_PERCENT);

    // 电量计失败不阻塞:界面上电量显示为 "--"。
    if (bsp_battery_init() != ESP_OK) {
        ESP_LOGW(TAG, "电量计初始化失败(CW2017 未应答)");
    }

    tarot_app_start();
    ESP_LOGI(TAG, "启动完成");
}

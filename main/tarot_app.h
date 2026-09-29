// main/tarot_app.h —— 塔罗应用启动入口(由 main.c 在显示初始化后调用)。
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// 创建输入队列与应用任务,注册按键回调并进入封面页。
// 显示/LVGL 必须已就绪;失败打日志并保持黑屏(不重启)。
void tarot_app_start(void);

#ifdef __cplusplus
}
#endif

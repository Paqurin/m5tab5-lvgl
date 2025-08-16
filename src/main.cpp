#include <Arduino.h>
#include <lvgl.h>

#if LV_COLOR_DEPTH != 16
#error "LV_COLOR_DEPTH should be 16 for M5Stack displays"
#endif

static lv_disp_draw_buf_t draw_buf;
static lv_color_t *buf;

void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  lv_disp_flush_ready(disp);
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting M5Stack Tab5 LVGL Demo");
  
  lv_init();
  
  uint16_t screen_width = 1280;
  uint16_t screen_height = 720;
  uint32_t buf_size = screen_width * 10;
  
  buf = (lv_color_t*)malloc(buf_size * sizeof(lv_color_t));
  if (!buf) {
    Serial.println("Failed to allocate display buffer");
    return;
  }
  
  lv_disp_draw_buf_init(&draw_buf, buf, NULL, buf_size);
  
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = screen_width;
  disp_drv.ver_res = screen_height;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  
  lv_disp_t *disp = lv_disp_drv_register(&disp_drv);
  if (!disp) {
    Serial.println("Failed to register display driver");
    return;
  }
  
  lv_obj_t *label = lv_label_create(lv_scr_act());
  lv_label_set_text(label, "M5Stack Tab5\nLVGL Demo\n\n5-inch 1280x720\nMIPI-DSI Display\nGT911 Touch\n\nESP32-P4 Arduino\nBuild Successful!");
  lv_obj_center(label);
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
  
  Serial.println("LVGL UI initialized");
}

void loop() {
  lv_timer_handler();
  delay(5);
}
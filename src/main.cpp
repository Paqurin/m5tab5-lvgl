#include <Arduino.h>
#include <M5Unified.h>
#include <lvgl.h>

#if LV_COLOR_DEPTH != 16
#error "LV_COLOR_DEPTH should be 16 for M5Stack displays"
#endif

static lv_disp_draw_buf_t draw_buf;
static lv_color_t *buf;

void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {
  auto touch = M5.Touch.getDetail();
  if (touch.isPressed()) {
    data->state = LV_INDEV_STATE_PR;
    data->point.x = touch.x;
    data->point.y = touch.y;
  } else {
    data->state = LV_INDEV_STATE_REL;
  }
}

void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);
  
  M5.Display.startWrite();
  M5.Display.setAddrWindow(area->x1, area->y1, w, h);
  M5.Display.writePixels((lgfx::rgb565_t*)&color_p->full, w * h);
  M5.Display.endWrite();
  
  lv_disp_flush_ready(disp);
}

void setup() {
  auto cfg = M5.config();
  cfg.serial_baudrate = 0; // Disable serial
  M5.begin(cfg);
  
  lv_init();
  
  uint16_t screen_width = M5.Display.width();
  uint16_t screen_height = M5.Display.height();
  uint32_t buf_size = screen_width * 10;
  
  buf = (lv_color_t*)malloc(buf_size * sizeof(lv_color_t));
  if (!buf) {
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
    return;
  }
  
  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_touchpad_read;
  lv_indev_drv_register(&indev_drv);
  
  lv_obj_t *label = lv_label_create(lv_scr_act());
  lv_label_set_text(label, "M5Stack Tab5\nLVGL Demo\n\nTouch screen\nto interact!");
  lv_obj_center(label);
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
}

void loop() {
  M5.update();
  lv_timer_handler();
  delay(5);
}
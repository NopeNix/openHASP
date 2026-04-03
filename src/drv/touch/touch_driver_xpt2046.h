/* MIT License - Copyright (c) 2019-2024 Francis Van Roie
   For full license information read the LICENSE file in the project folder */

#ifndef HASP_XPT2046_TOUCH_DRIVER_H
#define HASP_XPT2046_TOUCH_DRIVER_H

#ifdef ARDUINO
#include "lvgl.h"
#include "touch_driver.h"

namespace dev {

class TouchXpt2046 : public BaseTouch {
  public:
    IRAM_ATTR bool read(lv_indev_drv_t* indev_driver, lv_indev_data_t* data);
    void init(int w, int h);
    void set_rotation(uint8_t rotation);

  private:
    uint8_t _rotation;
    int16_t _min_x;
    int16_t _max_x;
    int16_t _min_y;
    int16_t _max_y;
};

} // namespace dev

using dev::TouchXpt2046;
extern dev::TouchXpt2046 haspTouch;

#endif // ARDUINO

#endif // HASP_XPT2046_TOUCH_DRIVER_H

# HƯỚNG DẪN ĐỌC SOURCE CODE ĐỂ VẼ BIỂU ĐỒ VÀ HIỂU SÂU KIẾN TRÚC HỆ THỐNG
**Dự án:** Hệ thống giám sát và điều khiển phòng thông minh (STM32F103C8T6)  
**Kiến trúc:** 3-Tier Layered Embedded Architecture (Tuân thủ `stm32f103_embedded_rules.md` & `Requirement_Spec.pdf`)

---

# PHẦN 1: CÁCH ĐỌC CODE ĐỂ VẼ SƠ ĐỒ KHỐI, HLSM VÀ FLOWCHART

Để vẽ được các mô hình chuẩn kỹ thuật phục vụ báo cáo / đồ án, bạn không cần đọc dàn trải mà cần bóc tách đúng các thực thể: **Khối chức năng (Blocks)**, **Trạng thái (States)**, **Sự kiện (Events)** và **Luồng điều khiển (Decisions/Flows)**.

---

## 1.1. Cách đọc để vẽ Sơ đồ khối hệ thống (System Block Diagram)

Sơ đồ khối thể hiện phân tầng kiến trúc và luồng giao tiếp giữa phần cứng với các module phần mềm.

### Bước bóc tách từ code:
1. **Khối Phần cứng (Hardware Blocks):**
   - Mở [STM32.kicad_sch](file:///C:/Users/LENOVO/OneDrive/Desktop/TKHTVT2/Hardware/01_Design/STM32.kicad_sch) hoặc file khởi tạo [Core/Src/main.c](file:///D:/STM32/WorkSpace/BTL_TKHTN2/Core/Src/main.c) để liệt kê các ngoại vi:
     - **TFT LCD ILI9341 ($320 \times 240$):** Kết nối qua SPI1 (`PA5`-SCK, `PA7`-MOSI) và 3 GPIO (`PA4`-CS, `PB0`-DC, `PB1`-RST).
     - **Touch XPT2046:** Kết nối qua SPI2 (`PB13`-SCK, `PB14`-MISO, `PB15`-MOSI) và 2 GPIO (`PB12`-CS, `PB11`-IRQ).
     - **4 Nút nhấn vật lý:** Kết nối GPIO Input Pull-up (`PA3`, `PB2`, `PB3`, `PB4`).
     - **4 Kênh Relay (Tải):** Kết nối GPIO Output Active-LOW (`PB5`-Fan, `PB6`-Light1, `PB7`-Light2, `PB8`-Dehum).
     - **3 Cảm biến:** `PA1` (DHT22 1-Wire), `PA0` (LDR ADC1_IN0), `PA2` (PIR Input).

2. **Khối Tầng Driver Hệ thống (System Layer):**
   - Đọc thư mục `System/`:
     - [button.c / button.h](file:///D:/STM32/WorkSpace/BTL_TKHTN2/System/button.h): Module đọc và lọc rung nút bấm vật lý.
     - [ili9341.c / ili9341.h](file:///D:/STM32/WorkSpace/BTL_TKHTN2/System/ili9341.h): Driver điều khiển màn hình đồ họa.
     - [font5x7.c / font5x7.h](file:///D:/STM32/WorkSpace/BTL_TKHTN2/System/font5x7.h): Thư viện font ký tự điểm ảnh.
     - [xpt2046.c / xpt2046.h](file:///D:/STM32/WorkSpace/BTL_TKHTN2/System/xpt2046.h): Driver đọc và hiệu chuẩn tọa độ cảm ứng.
     - [sys_types.h](file:///D:/STM32/WorkSpace/BTL_TKHTN2/System/sys_types.h): Định nghĩa mã lỗi chuẩn (`SYS_OK`, `SYS_ERROR`).

3. **Khối Tầng Ứng dụng (Application Layer):**
   - Đọc thư mục `Application/`:
     - [display_manager.c / display_manager.h](file:///D:/STM32/WorkSpace/BTL_TKHTN2/Application/display_manager.h): Quản lý bố cục giao diện, vẽ card cảm biến, vẽ nút và tính toán vùng chạm (Hit-testing).
     - [mode_control.c / mode_control.h](file:///D:/STM32/WorkSpace/BTL_TKHTN2/Application/mode_control.h): Quản lý trạng thái chuyển đổi giữa Manual và Auto.
     - [app_main.c / app_main.h](file:///D:/STM32/WorkSpace/BTL_TKHTN2/Application/app_main.h): Trung tâm điều phối, phân chia thời gian (Cooperative Scheduler) cho toàn bộ hệ thống.

---

## 1.2. Cách đọc để vẽ HLSM (High-Level State Machine)

Khi vẽ HLSM, bạn cần xác định: **Trạng thái (State)**, **Sự kiện kích hoạt (Event)**, **Điều kiện rẽ nhánh (Guard `[...]`)**, và **Hành động (Action `/{...}`)**.

Trong mã nguồn hiện tại có **3 máy trạng thái (FSM)** cần mô hình hóa:

### 1. FSM Cấp cao nhất: Chế độ hoạt động (System Mode FSM)
- **Nơi đọc:** [Application/mode_control.c](file:///D:/STM32/WorkSpace/BTL_TKHTN2/Application/mode_control.c) và [Application/mode_control.h](file:///D:/STM32/WorkSpace/BTL_TKHTN2/Application/mode_control.h).
- **Các trạng thái:**
  - `MODE_MANUAL`: Chế độ điều khiển thủ công bởi người dùng.
  - `MODE_AUTO`: Chế độ tự động chạy theo thuật toán cảm biến.
- **Các chuyển trạng thái (Transitions):**
  - `MODE_MANUAL` $\to$ `MODE_AUTO`: Xảy ra khi có sự kiện `Chạm nút MODE trên màn hình TFT` (Hàm `mode_control_toggle()`).
  - `MODE_AUTO` $\to$ `MODE_MANUAL`: Xảy ra qua 2 nhánh:
    - *Nhánh 1:* Người dùng chạm nút MODE trên màn hình (`mode_control_toggle()`).
    - *Nhánh 2:* Người dùng bấm bất kỳ nút vật lý nào hoặc chạm vào bất kỳ ô thiết bị nào trên màn hình (`mode_control_handle_user_interaction()`).
- **Hành động (Action):** Khi trạng thái thay đổi $\to$ Gọi `display_manager_update_mode()` để đổi màu và chữ của nút Mode trên UI.

### 2. FSM Con 1: Lọc rung nút bấm vật lý (Button Debounce FSM)
- **Nơi đọc:** [System/button.c dòng 44–65](file:///D:/STM32/WorkSpace/BTL_TKHTN2/System/button.c#L44-L65).
- **Các trạng thái của mỗi nút:**
  - `STATE_IDLE` (Nút đang nhả, GPIO = 1).
  - `STATE_DEBOUNCING` (Phát hiện GPIO = 0 lần đầu, bộ đếm `s_samples = 1`).
  - `STATE_PRESSED` (Mẫu tiếp theo vẫn là 0 $\to$ `s_samples >= 2` $\to$ Xác nhận nhấn hợp lệ, sinh sự kiện `s_event = 1`).
  - `STATE_WAIT_RELEASE` (Chờ người dùng nhả tay, GPIO quay về 1 đủ 2 chu kỳ liên tiếp mới quay về `STATE_IDLE`).

### 3. FSM Con 2: Chống dội cảm ứng điện trở (Touch Debounce & Cooldown FSM)
- **Nơi đọc:** [Application/app_main.c dòng 85–133](file:///D:/STM32/WorkSpace/BTL_TKHTN2/Application/app_main.c#L85-L133).
- **Các trạng thái:**
  - `TOUCH_IDLE`: Màn hình không có chạm (`xpt2046_read() == SYS_ERROR`).
  - `TOUCH_FILTERING`: Có chạm lần 1 (`touch_consecutive_press = 1`), chưa kích hoạt lệnh.
  - `TOUCH_TRIGGERED`: Chạm giữ ổn định 2 chu kỳ 30ms liên tiếp (`touch_consecutive_press >= 2`) VÀ đã hết thời gian hồi chiêu (`touch_cooldown_until`). Lúc này kích hoạt lệnh 1 lần duy nhất, đặt `touch_was_down = 1` và nạp thời gian hồi chiêu 350ms.
  - `TOUCH_HOLD`: Tiếp tục giữ tay nhưng không lặp lại lệnh vì `touch_was_down == 1`.

---

## 1.3. Cách đọc để vẽ Flowchart (Lưu đồ thuật toán)

Một lưu đồ hoàn chỉnh gồm 1 **Lưu đồ vòng lặp chính (Main Loop)** và các **Lưu đồ nhánh chức năng (Sub-routines)**.

### 1. Main Flowchart (`app_loop`)
- **Nơi đọc:** [Application/app_main.c dòng 36–140](file:///D:/STM32/WorkSpace/BTL_TKHTN2/Application/app_main.c#L36-L140).
- **Các khối logic:**
  1. *Khối Bắt đầu:* Nhận `current_tick_ms` từ `HAL_GetTick()`.
  2. *Khối Quyết định 1 (10ms Slot):* `(current_tick_ms - last_btn_tick) >= 10`?
     - Nếu ĐÚNG $\to$ Chạy tiến trình quét nút vật lý (Xem Sub-flowchart 1).
  3. *Khối Quyết định 2 (30ms Slot):* `(current_tick_ms - last_touch_tick) >= 30`?
     - Nếu ĐÚNG $\to$ Chạy tiến trình quét cảm ứng (Xem Sub-flowchart 2).
  4. *Khối Quyết định 3 (500ms Slot):* `(current_tick_ms - last_sensor_tick) >= 500`?
     - Nếu ĐÚNG $\to$ Gọi `display_manager_update_sensors()` cập nhật số đo lên UI.
  5. *Khối Kết thúc:* Trở về vòng lặp `while(1)` trong `main.c`.

### 2. Sub-flowchart 1: Quét Nút vật lý & Quyền ưu tiên (10ms Task)
- **Nơi đọc:** [Application/app_main.c dòng 47–82](file:///D:/STM32/WorkSpace/BTL_TKHTN2/Application/app_main.c#L47-L82).
- **Luồng xử lý:**
  1. Gọi `button_scan_10ms()`.
  2. Vòng lặp `for (i = 0; i < 4; i++)`:
     - Kiểm tra `button_is_down(i)` $\to$ Nếu có nút đang bị giữ, bật cờ `any_physical_down = 1`.
     - Kiểm tra `button_has_event(i)` $\to$ Nếu có sự kiện nhấn mới:
       - Ép chế độ về `MANUAL` (gọi `mode_control_handle_user_interaction()`).
       - Đảo trạng thái thiết bị: `s_device_states[i] ^= 1`.
       - Cập nhật giao diện nút và hiển thị chữ thông báo nút vật lý vừa can thiệp.
       - Hủy lệnh cảm ứng đang dở: `touch_was_down = 0`.
       - **Khóa cảm ứng 500ms:** `touch_lockout_until = current_tick_ms + 500`.
  3. Nếu `any_physical_down == 1` $\to$ Gia hạn giữ khóa cảm ứng liên tục.

### 3. Sub-flowchart 2: Quét Cảm ứng & Phân tích tọa độ chạm (30ms Task)
- **Nơi đọc:** [Application/app_main.c dòng 85–133](file:///D:/STM32/WorkSpace/BTL_TKHTN2/Application/app_main.c#L85-L133).
- **Luồng xử lý:**
  1. Kiểm tra thời gian khóa cảm ứng: `current_tick_ms >= touch_lockout_until`? Nếu CHƯA $\to$ Thoát ngay (Ưu tiên nút vật lý).
  2. Đọc cảm ứng `xpt2046_read(&tx, &ty)`:
     - Nếu không có chạm $\to$ Reset cờ `touch_consecutive_press = 0`, `touch_was_down = 0` $\to$ Thoát.
     - Nếu có chạm $\to$ Tăng `touch_consecutive_press++`.
  3. Phân tích vùng chạm: Gọi `display_manager_hit_test(tx, ty)` để xác định `target`.
  4. Hiển thị tọa độ chạm lên màn hình: `display_manager_update_touch_pos(tx, ty, target)`.
  5. Kiểm tra điều kiện kích hoạt: `!touch_was_down && touch_consecutive_press >= 2 && current_tick_ms >= touch_cooldown_until`?
     - Nếu ĐÚNG:
       - Đặt `touch_was_down = 1` và nạp hồi chiêu `touch_cooldown_until = current_tick_ms + 350`.
       - Rẽ nhánh theo `target`:
         - Nếu chạm nút MODE $\to$ Gọi `mode_control_toggle()`.
         - Nếu chạm ô FAN / LIGHT 1 / LIGHT 2 / DEHUM $\to$ Ép về `MANUAL` $\to$ Đảo bit `s_device_states[dev_id] ^= 1` $\to$ Cập nhật UI.

---

# PHẦN 2: CÁCH ĐỌC ĐỂ HIỂU RÕ TOÀN BỘ CHƯƠNG TRÌNH, TỪNG DÒNG CODE

Để hiểu từng dòng code mà không bị "ngợp", bạn phải đọc theo nguyên tắc **Truy vết luồng thực thi từ khởi động đến vòng lặp vô tận (Execution Trace)**:

---

## 2.1. Đọc tầng Khởi động (Startup & Hardware Initialization)

Mở file [Core/Src/main.c](file:///D:/STM32/WorkSpace/BTL_TKHTN2/Core/Src/main.c):
1. **Dòng 81: `SystemClock_Config()`:** Thiết lập xung nhịp vi điều khiển chạy tối đa **72 MHz** từ thạch anh ngoài 8 MHz (HSE) qua bộ nhân tần PLL, với độ trễ bộ nhớ Flash `FLASH_LATENCY_2`.
2. **Dòng 88: `MX_GPIO_Init()`:** Cấp xung nhịp cho `GPIOA`, `GPIOB`, `GPIOC` qua thanh ghi RCC và cấu hình chân LED kiểm tra.
3. **Dòng 90: `app_init()`:** Bước nhảy chuyển giao quyền điều khiển từ tầng sinh tự động của STM32Cube sang tầng Ứng dụng nghiệp vụ.
4. **Dòng 97: `app_loop(HAL_GetTick())`:** Nằm trong vòng lặp `while (1)`, liên tục truyền biến đếm mili-giây của hệ thống (`SysTick`) vào hàm quản trị.

---

## 2.2. Đọc tầng Driver Phần cứng (System Layer)

### 1. Driver Nút bấm: [System/button.c](file:///D:/STM32/WorkSpace/BTL_TKHTN2/System/button.c)
- **Dòng 14: `button_init()`:**
  - Cấu hình 4 chân GPIO `PA3`, `PB2`, `PB3`, `PB4` là `GPIO_MODE_INPUT`, điện trở nội kéo lên `GPIO_PULLUP`. Khi không bấm, điện áp chân là 3.3V (Logic 1); khi bấm nút, chân bị nối xuống đất GND (Logic 0).
- **Dòng 44: `button_scan_10ms()`:**
  - `HAL_GPIO_ReadPin()` đọc mức logic chân. Mức 0 tức là đang ấn (`is_pressed = 1`).
  - Mảng `s_samples[i]` đóng vai trò là bộ đếm tích lũy. Nếu người dùng bấm và giữ nút, mỗi 10ms biến này tăng 1. Khi đạt $\ge 2$ ($20$ms), nút được xem là đã qua giai đoạn nảy tiếp điểm cơ khí (Contact Bouncing) và chuyển sang trạng thái hợp lệ.
- **Dòng 69: `button_has_event()`:**
  - Cơ chế "Đọc một lần rồi tự xóa" (Self-clearing flag): Trả về giá trị của `s_event[id]` đồng thời gán `s_event[id] = 0`. Điều này giải thích tại sao bấm 1 lần chỉ kích hoạt 1 lần, không bị kích hoạt liên tục khi giữ tay.

### 2. Driver Màn hình: [System/ili9341.c](file:///D:/STM32/WorkSpace/BTL_TKHTN2/System/ili9341.c)
- **Dòng 35–45: `ili9341_write_cmd()` và `ili9341_write_data()`:**
  - Giao tiếp qua chân `DC` (Data/Command): Kéo `DC = 0` trước khi gửi SPI nếu muốn gửi mã lệnh; kéo `DC = 1` nếu muốn gửi dữ liệu pixel.
- **Dòng 106: `ili9341_set_window(x0, y0, x1, y1)`:**
  - Gửi lệnh `0x2A` (Column Address Set) và `0x2B` (Page Address Set) để định vị khung chữ nhật cần vẽ trên màn hình. Sau đó gửi lệnh `0x2C` (Memory Write) để chuẩn bị nhận dữ liệu màu.
- **Dòng 130: `ili9341_fill_rect()`:**
  - Bơm liên tục các từ mã màu 16-bit RGB565 vào vùng đã định vị để lấp đầy hình chữ nhật.
- **Dòng 182: `ili9341_draw_string(x, y, str, color, bg, scale)`:**
  - Dò từng ký tự trong chuỗi $\to$ lấy ma trận điểm ảnh từ `font5x7.c` $\to$ nhân kích thước ma trận theo tham số `scale` $\to$ vẽ chữ nét to rõ mà không tốn dung lượng bộ nhớ RAM (toàn bộ font nằm trong Flash ROM).

### 3. Driver Cảm ứng: [System/xpt2046.c](file:///D:/STM32/WorkSpace/BTL_TKHTN2/System/xpt2046.c)
- **Dòng 125: `xpt2046_read(uint16_t *x, uint16_t *y)`:**
  - Kiểm tra chân `T_IRQ` (PB11): Nếu chân này ở mức HIGH, chứng tỏ không có ngón tay đè lên tấm kính cảm ứng $\to$ trả về `SYS_ERROR` ngay lập tức để tiết kiệm CPU.
  - Nếu `T_IRQ == 0` (có lực đè): Chip gửi xung SPI bitbang để kích hoạt kênh đo điện áp trục Y (mã `0x90`) và trục X (mã `0xD0`) từ bộ ADC 12-bit nội của XPT2046.
- **Dòng 70: `xpt2046_calibrate_point()`:**
  - Chuyển đổi giá trị thô ADC ($200 \dots 3800$) thành tọa độ pixel $(0 \dots 320, 0 \dots 240)$ qua công thức nội suy tuyến tính.
  - Macro `#define TOUCH_INVERT_Y 1` tại dòng 31 đảo chiều trục Y (`pixel_y = 240 - pixel_y`) để khớp 100% với chiều xoay của màn hình ILI9341, tránh lỗi chạm nút trên mà nhận nút dưới.

---

## 2.3. Đọc tầng Ứng dụng & Quản trị Giao diện (Application Layer)

### 1. Quản lý Chế độ: [Application/mode_control.c](file:///D:/STM32/WorkSpace/BTL_TKHTN2/Application/mode_control.c)
- Biến `static SystemMode_t s_mode` được ẩn kín bên trong file (nguyên tắc đóng gói dữ liệu - Encapsulation).
- Hàm `mode_control_handle_user_interaction()`:
  ```c
  if (s_mode == MODE_AUTO) {
      s_mode = MODE_MANUAL;
      return 1; /* Báo cho caller biết có sự thay đổi để vẽ lại UI */
  }
  return 0;
  ```
  Đây chính là luật nghiệp vụ: Bất kỳ khi nào người dùng tác động vào nút bấm hoặc chạm màn hình, hệ thống lập tức nhường quyền kiểm soát cho con người bằng cách chuyển về `MANUAL`.

### 2. Thiết kế Giao diện: [Application/display_manager.c](file:///D:/STM32/WorkSpace/BTL_TKHTN2/Application/display_manager.c)
- **Hệ tọa độ giao diện ($320 \times 240$):**
  - $y = 0 \dots 40$: Header (Tiêu đề "SMART ROOM" và Nút chọn MODE).
  - $y = 48 \dots 146$: Vùng 4 Card hiển thị thông số môi trường (TEMP, HUMIDITY, LIGHT ADC, MOTION PIR).
  - $y = 154 \dots 216$: Vùng 4 Nút điều khiển thiết bị (FAN, LIGHT 1, LIGHT 2, DEHUM).
  - $y = 222 \dots 238$: Footer hiển thị tọa độ cảm ứng thực tế đang chạm và trạng thái phản hồi nút vật lý.
- **Dòng 120: Hàm `is_inside()`:**
  ```c
  static inline uint8_t is_inside(int32_t px, int32_t py, int32_t bx, int32_t by, int32_t bw, int32_t bh)
  ```
  Sử dụng số nguyên có dấu `int32_t` với biên đệm `BOX_PAD = 6` pixel. Điều này giúp các nút bấm trên màn hình điện trở có "vùng nhận diện" rộng hơn kích thước vẽ thực tế 6 pixel, giúp người dùng bấm ngón tay rất nhạy và không bị trượt.
- **Kỹ thuật Differential Redraw (Vẽ cục bộ):**
  - Không bao giờ gọi hàm xóa toàn màn hình trong vòng lặp!
  - [`display_manager_update_device()`](file:///D:/STM32/WorkSpace/BTL_TKHTN2/Application/display_manager.c#L330): Khi quạt bật, hàm chỉ tính toán đúng tọa độ của ô chữ nhật FAN và tô lại màu xanh sáng. Các vùng khác trên màn hình giữ nguyên 100%, triệt tiêu hoàn toàn hiện tượng nhấp nháy màn hình.

---

## 2.4. Đọc "Trái tim điều phối": [Application/app_main.c](file:///D:/STM32/WorkSpace/BTL_TKHTN2/Application/app_main.c)

Tất cả tinh hoa điều khiển tập trung tại hàm `app_loop()`:

```c
void app_loop(uint32_t current_tick_ms)
```

1. **Hiểu cách tính thời gian không khóa (Non-blocking Cooperative Scheduling):**
   ```c
   if ((uint32_t)(current_tick_ms - last_btn_tick) >= 10)
   ```
   Phép trừ không dấu `(uint32_t)(current - last)` là chuẩn lập trình nhúng an toàn tuyệt đối, chạy chính xác ngay cả khi biến đếm `current_tick_ms` bị tràn số (sau ~49.7 ngày hoạt động liên tục).

2. **Cơ chế Quyền ưu tiên Nút vật lý (Physical Button Precedence):**
   - Vòng quét nút vật lý chạy ở slot **10ms**, nhanh gấp 3 lần vòng quét cảm ứng (30ms).
   - Khi phát hiện có nút vật lý được bấm:
     ```c
     touch_lockout_until = current_tick_ms + 500;
     ```
     Dòng lệnh này dựng lên một "bức tường bảo vệ" kéo dài 500ms. Trong suốt thời gian này, mọi tín hiệu đọc được từ màn hình cảm ứng đều bị hủy bỏ. Điều này ngăn chặn tình huống người dùng đang bấm nút vật lý mà tay vô tình quẹt vào màn hình làm loạn trạng thái.

3. **Cơ chế Lọc rung cảm ứng điện trở 2 lớp (Two-stage Touch Debounce):**
   ```c
   if (!touch_was_down && touch_consecutive_press >= 2) {
       if ((uint32_t)(current_tick_ms - touch_cooldown_until) < 0x80000000U) {
           touch_was_down = 1;
           touch_cooldown_until = current_tick_ms + 350;
           ...
       }
   }
   ```
   - **Lớp 1 (`touch_consecutive_press >= 2`):** Màn hình cảm ứng điện trở rất dễ phát sinh xung nhiễu giả ngắn hạn ($\sim 5-10$ms). Đoạn code yêu cầu phải lấy được mẫu tọa độ hợp lệ trong 2 chu kỳ quét 30ms liên tiếp (tổng cộng 60ms giữ tay) mới công nhận là một cú chạm có chủ đích.
   - **Lớp 2 (`touch_cooldown_until` 350ms):** Khi ngón tay chạm vào màn hình rồi nhấc lên, bề mặt cảm ứng sẽ có hiện tượng rung tiếp điểm cơ học (bật/tắt liên tục trong vài chục mili-giây). Cửa sổ thời gian 350ms khóa chặt mọi lần kích hoạt tiếp theo, triệt tiêu hoàn toàn hiện tượng thiết bị bị bật rồi tắt ngay lập tức trong một lần chạm.

4. **Hiển thị thông số định kỳ 500ms:**
   - Dữ liệu cảm biến không cần cập nhật quá nhanh gây quá tải bus SPI. Chu kỳ 500ms vừa đủ để mắt người quan sát mượt mà, vừa tối ưu hóa băng thông CPU để dành thời gian phục vụ tương tác người dùng.

---

### TỔNG KẾT BẢNG TRA CỨU FILE & TRÁCH NHIỆM

| Tên File | Tầng kiến trúc | Trách nhiệm chính |
| :--- | :--- | :--- |
| [main.c](file:///D:/STM32/WorkSpace/BTL_TKHTN2/Core/Src/main.c) | Hardware/Core | Cấu hình RCC 72MHz, gọi khởi động và duy trì vòng lặp `while(1)`. |
| [button.c](file:///D:/STM32/WorkSpace/BTL_TKHTN2/System/button.c) | System Layer | Đọc GPIO 4 nút bấm, đếm mẫu 20ms lọc rung tiếp điểm cơ học. |
| [ili9341.c](file:///D:/STM32/WorkSpace/BTL_TKHTN2/System/ili9341.c) | System Layer | Giao tiếp SPI1 điều khiển LCD, mở window và bơm pixel RGB565. |
| [xpt2046.c](file:///D:/STM32/WorkSpace/BTL_TKHTN2/System/xpt2046.c) | System Layer | Đọc ngắt IRQ, đọc ADC SPI2, hiệu chuẩn tọa độ tuyến tính và đảo trục Y. |
| [display_manager.c](file:///D:/STM32/WorkSpace/BTL_TKHTN2/Application) | Application Layer | Định nghĩa bố cục UI $320 \times 240$, vẽ viền card, tính hit-test vùng chạm $\pm 6$px. |
| [mode_control.c](file:///D:/STM32/WorkSpace/BTL_TKHTN2/Application) | Application Layer | Quản lý máy trạng thái Manual/Auto, tự động chuyển Manual khi có can thiệp. |
| [app_main.c](file:///D:/STM32/WorkSpace/BTL_TKHTN2/Application) | Application Layer | Điều phối thời gian non-blocking (10ms, 30ms, 500ms), xử lý ưu tiên và chống dội cảm ứng. |

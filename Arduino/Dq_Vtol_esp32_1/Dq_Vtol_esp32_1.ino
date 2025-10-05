#include <Arduino.h>
#include <mavlink.h> // <--- 关键修改在这里！

// --- 配置参数 ---

// 用于和飞控通信的串口
// ESP32-S3 有多个硬件串口。Serial1 默认使用 GPIO1(TX) 和 GPIO2(RX)
#define MAVLINK_SERIAL Serial1
const long MAVLINK_BAUD = 115200; // 必须和飞控 SERIAL2_BAUD 参数一致

// 定义要生成的 PWM 通道数量和起始通道号
// ArduPilot 的 SERVO_OUTPUT_RAW 消息中，通道从1开始计数
const int START_CHANNEL = 14; // 我们从通道14开始拓展
const int NUM_CHANNELS = 7;   // 我们需要拓展7个通道 (14, 15, 16, 17, 18, 19, 20)

// 将 PWM 通道映射到 ESP32-S3 的 GPIO 引脚
// 请根据你的接线修改
const int pwmPins[NUM_CHANNELS] = {
    4,  // Channel 14 -> GPIO4
    5,  // Channel 15 -> GPIO5
    6,  // Channel 16 -> GPIO6
    7,  // Channel 17 -> GPIO7
    15, // Channel 18 -> GPIO15
    16, // Channel 19 -> GPIO16
    17  // Channel 20 -> GPIO17
};

// PWM 属性
const int PWM_FREQUENCY = 50;      // 舵机和电调的标准频率是 50Hz
const int PWM_RESOLUTION = 16;     // 16位分辨率 (0-65535)，可以提供非常平滑的控制
const int PWM_TIMER_WIDTH = 20000; // 50Hz 对应的周期是 20000 微秒 (µs)

// --- 全局变量 ---
mavlink_message_t msg;
mavlink_status_t status;


/**
 * @brief 将 PWM 脉宽值 (µs) 转换为 ESP32 LEDC 的占空比值
 * @param us PWM 脉宽，单位微秒 (例如 1000-2000)
 * @return 适用于 ledcWrite() 的占空比值
 */
uint32_t pwmUsToDuty(uint16_t us) {
    // 限制输入范围，防止舵机超出行程
    us = constrain(us, 500, 2500); 
    // 计算占空比
    // (us / 总周期us) * 最大分辨率值
    return (uint32_t)((us / (float)PWM_TIMER_WIDTH) * (float)((1 << PWM_RESOLUTION) - 1));
}


/**
 * @brief 处理 SERVO_OUTPUT_RAW 消息
 * @param msg MAVLink 消息
 */
void handle_servo_output(const mavlink_message_t* msg) {
    mavlink_servo_output_raw_t servo_out;
    mavlink_msg_servo_output_raw_decode(msg, &servo_out);

    uint16_t servo_raw_values[8] = {
        servo_out.servo1_raw, servo_out.servo2_raw, servo_out.servo3_raw, servo_out.servo4_raw,
        servo_out.servo5_raw, servo_out.servo6_raw, servo_out.servo7_raw, servo_out.servo8_raw
    };
    
    // 消息中的 `port` 字段指示这是哪个通道块 (0=通道1-8, 1=通道9-16, etc.)
    int channel_offset = servo_out.port * 8;

    for (int i = 0; i < 8; ++i) {
        int current_channel = channel_offset + i + 1; // MAVLink 通道从 1 开始
        
        // 检查这个通道是不是我们需要的
        for (int j = 0; j < NUM_CHANNELS; ++j) {
            if (current_channel == START_CHANNEL + j) {
                uint16_t pwm_us = servo_raw_values[i];
                
                // 如果 PWM 值为 0 或 UINT16_MAX，表示该通道无效或未配置
                if (pwm_us > 0 && pwm_us != UINT16_MAX) {
                    uint32_t duty = pwmUsToDuty(pwm_us);
                    ledcWrite(j, duty); // 使用 LEDC 通道索引 j (0 到 NUM_CHANNELS-1)
                }
            }
        }
    }
}


void setup() {
    // 启动用于调试的串口
    Serial.begin(115200);
    Serial.println("MAVLink2 to PWM Converter for ESP32-S3");

    // 启动与飞控通信的串口
    // RX on GPIO2, TX on GPIO1 for Serial1 on S3
    MAVLINK_SERIAL.begin(MAVLINK_BAUD, SERIAL_8N1, 2, 1);

    // 配置 PWM 输出通道
    Serial.println("Configuring PWM channels...");
    for (int i = 0; i < NUM_CHANNELS; i++) {
        // 设置 LEDC 通道
        ledcSetup(i, PWM_FREQUENCY, PWM_RESOLUTION);
        // 将 GPIO 引脚附加到 LEDC 通道
        ledcAttachPin(pwmPins[i], i);
        Serial.printf("Channel %d (Output %d) on GPIO %d initialized.\n", i + START_CHANNEL, i, pwmPins[i]);
    }

    Serial.println("Setup complete. Waiting for MAVLink messages...");
}


void loop() {
    // 持续从飞控读取数据
    while (MAVLINK_SERIAL.available() > 0) {
        // 解析 MAVLink 字节流
        if (mavlink_parse_char(MAVLINK_COMM_0, MAVLINK_SERIAL.read(), &msg, &status)) {
            // 当成功解析出一个完整的消息时，进行处理
            switch (msg.msgid) {
                case MAVLINK_MSG_ID_SERVO_OUTPUT_RAW:
                    handle_servo_output(&msg);
                    break;
                case MAVLINK_MSG_ID_HEARTBEAT:
                    // 可以通过心跳包来确认连接是否正常
                    // Serial.println("Heartbeat received.");
                    break;
                default:
                    // 其他我们不关心的消息
                    break;
            }
        }
    }
}
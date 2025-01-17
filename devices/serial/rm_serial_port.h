#pragma once
#include <errno.h>  //ERROR数字定义
#include <fcntl.h>  //文件控制定义
#include <string.h>
#include <sys/select.h>
#include <termios.h>  //POSIX终端控制定义
#include <unistd.h>   //UNIX标准定义

#include <iostream>
#include <opencv2/opencv.hpp>
namespace serial_port {
/**
 * @brief 串口各个数组的长度设置
 */
enum BuffLength {
  // 接收
  REC_INFO_LENGTH = 8,  // 接受后解码得到的数组长度 TODO: 还未使用
  // 发送
  CRC_BUFF_LENGTH = 11,    // 写入 CRC 校验的 BUFF 长度
  WRITE_BUFF_LENGTH = 13,  // 写入串口的 BUFF 长度

};
enum color {
  BLUE,
  RED,
  /**
      @brief: 描述己方颜色信息
      @param: RED           己方为红色
      @param: BLUE          己方为蓝色
    */
};
enum run_mode {
  DEFAULT_MODE,
  SUP_SHOOT,
  ENERGY_AGENCY,
  SENTRY_MODE,
  BASE_MODE,
  /**
      @brief: 描述运行模式信息
      @param: SUP_SHOOT         自瞄模式
      @param: ENERGY_AGENCY     神符模式
      @param: SENTRY_MODE       SENTRY模式
      @param: BASE_MODE         BASE模式
    */
};
enum Robot_ID {
  HERO = 1,
  ENGINEERING,
  INFANTRY,
  UAV = 6,
  SENTRY,
  /**
      @brief: 描述当前机器人ID信息
      @param: HERO          英雄
      @param: ENGINEERING   工程
      @param: INFANTRY      步兵
      @param: UAV           无人机
      @param: SENTRY        哨兵
    */
};
typedef struct Serial_Config {
  int set_bandrate = 1;
  int show_serial_information = 1;

} Serial_Cfg;
/**
 * @brief   串口信息接收结构体
 * @param:  my_color              描述己方颜色信息
 * @param:  now_run_mode          描述运行模式信息
 * @param:  my_robot_id           描述当前机器人ID信息
 * @param:  bullet_volacity       描述当前机器人子弹速度
 * @param:  acceleration          描述陀螺仪角加速度 .00
 * @param:  yaw_angle             描述陀螺仪的yaw轴角度（有符号）
 * @param:  pitch_angle           描述陀螺仪的pitch轴角度（有符号）
 */
typedef struct Receive_Data {
  int my_color;
  int now_run_mode;
  int my_robot_id;
  int bullet_volacity;
  float acceleration;

  union Receive_Yaw_Angle_Information {
    float yaw_angle;
    uint8_t arr_yaw_angle[4] = {0};
  } Receive_Yaw_Angle_Info;

  union Receive_Pitch_Angle_Information {
    float pitch_angle;
    uint8_t arr_pitch_angle[4] = {0};
  } Receive_Pitch_Angle_Info;

  Receive_Data() {
    my_color = RED;
    now_run_mode = SUP_SHOOT;
    my_robot_id = INFANTRY;
    Receive_Yaw_Angle_Info.yaw_angle = 0.f;
    Receive_Pitch_Angle_Info.pitch_angle = 0.f;
    acceleration = 0.f;
    bullet_volacity = 30;
  }
};
/**
 * @brief   串口信息发送结构体
 * @param:  angle_yaw         yaw 轴角度
 * @param:  angle_pitch       pitch 轴角度
 * @param:  symbol_yaw        yaw轴符号（右正左负）
 * @param:  symbol_pitch      pitch轴符号（下正上负）
 * @param:  is_shooting       机器人的开火指令：0->不开火 1->单发开火一枪
 * 2->跟随 3->不跟随
 * @param:  data_type         是否发现目标：1->发现 0->未发现
 * @param:  depth             深度信息
 */
typedef struct Write_Data {
  int symbol_yaw;
  int symbol_pitch;
  int depth;
  int is_shooting;
  int data_type;
  float yaw_angle;
  float pitch_angle;
};

class SerialPort {
 private:
  Serial_Cfg serial_config_;
  Receive_Data receive_data_;
  Receive_Data last_receive_data_;

  //串口标志量
  int fd;
  int transform_arr[4];
  unsigned char write_buff_[WRITE_BUFF_LENGTH];
  unsigned char crc_buff_[CRC_BUFF_LENGTH];
  unsigned char receive_buff_[REC_INFO_LENGTH];
  unsigned char receive_buff_temp_[REC_INFO_LENGTH * 2];
  // 高低八位的数据还原
  // 发送
  int16_t _yaw_reduction;  // TODO:是否可以改为 uint16_t
  int16_t _pitch_reduction;
  int16_t _depth_reduction;

  // 接收
  int16_t _angle_reduction;  // TODO:是否可以改为 uint16_t
  int16_t _acceleration_reduction;

  //交接返回值
  unsigned char exchangebyte;
  int16_t exchangebit;

  /** ---------- 函数声明 ---------- **/
  // CRC校验函数
  uint8_t Checksum_CRC8(unsigned char* buf, uint16_t len);
  // 数据分解函数
  void getDataForCRC(const int& data_type, const int& is_shooting,
                     const int& _yaw, const int16_t& yaw, const int& _pitch,
                     const int16_t& pitch, const int16_t& depth);
  void getDataForSend(const int& data_type, const int& is_shooting,
                      const int& _yaw, const int16_t& yaw, const int& _pitch,
                      const int16_t& pitch, const int16_t& depth,
                      const uint8_t& CRC);

 public:
  /** ---------- 函数声明 ---------- **/
  SerialPort(std::string _serial_config);
  ~SerialPort();

  inline float returnReceiveAcceleration() {
    return receive_data_.acceleration;
  }
  inline int returnReceiveBulletVolacity() {
    return receive_data_.bullet_volacity;
  }
  inline int returnReceiveRobotId() { return receive_data_.my_robot_id; }
  inline int returnReceiceColor() { return receive_data_.my_color; }
  inline int returnReceiveMode() { return receive_data_.now_run_mode; }
  inline float returnReceivePitch() {
    return receive_data_.Receive_Pitch_Angle_Info.pitch_angle;
  }
  inline float receiveYaw() {
    return receive_data_.Receive_Yaw_Angle_Info.yaw_angle;
  }

  // 拆开成位 bit
  inline unsigned char returnHighBit(const int& Byte) {
    exchangebyte = (Byte >> 8) & 0xff;
    return exchangebyte;
  }
  inline unsigned char returnLowBit(const int& Byte) {
    exchangebyte = Byte & 0xff;
    return exchangebyte;
  }

  // 合成为字节 Byte
  inline int16_t mergeIntoBytes(const unsigned char& highbit,
                                const unsigned char& lowbit) {
    exchangebit = (highbit << 8) | (lowbit);
    return exchangebit;
  }

  //自定义串口发送
  void RMserialWrite(const int& _yaw, const int16_t& yaw, const int& _pitch,
                     const int16_t& pitch, const int16_t& depth,
                     const int& data_type = 0, const int& is_shooting = 0);
  void RMserialWrite(Write_Data _write_data);
  //接收并处理串口数据
  void RMreceiveData();
  //判断是否接受到数据
  bool isEmpty();
  //更新串口接受数据
  void updateReceiveInformation();
};
}  // namespace serial_port
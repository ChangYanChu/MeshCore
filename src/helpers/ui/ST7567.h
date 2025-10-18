// 轻量 ST7567 图形液晶驱动头文件：声明类接口。
// 使用方式：
//   ST7567Display lcd(cs,rst,a0,sck,sda[,xOffset]);
//   if (lcd.begin()) { lcd.startFrame(); lcd.print("Hi"); lcd.endFrame(); }
// 保留与其它 DisplayDriver 一致的 API。

#pragma once

#include <Arduino.h>
#include <SPI.h>
#include "DisplayDriver.h"

class ST7567Display : public DisplayDriver {
public:
  ST7567Display(int cs, int rst, int a0, int sck, int sda, int xOffset = 0);
  ~ST7567Display();
  bool begin();

  bool isOn() override { return _isOn; }
  void turnOn() override;
  void turnOff() override;
  void clear() override;
  void startFrame(Color bkg = DARK) override;
  void setTextSize(int sz) override;
  void setColor(Color c) override;
  void setCursor(int x, int y) override;
  void print(const char* str) override;
  void fillRect(int x, int y, int w, int h) override;
  void drawRect(int x, int y, int w, int h) override;
  void drawXbm(int x, int y, const uint8_t* bits, int w, int h) override;
  uint16_t getTextWidth(const char* str) override;
  void endFrame() override;

  void initBacklight(int pin, uint8_t channel = 0, uint32_t freq = 5000, uint8_t resolution = 8, bool invert = false);
  void setBacklight(uint8_t value);
  uint8_t getBacklight();
  void setContrast(uint8_t resistor_ratio, uint8_t electronic_volume);
  void setInverse(bool inverse);
  // 运行时配置：段(列)翻转、行(COM)翻转、垂直位顺序 (LSB 顶部 or LSB 底部)
  void configureOrientation(bool segRemap, bool comReverse, bool lsbAtTop);
  void setSpiFrequency(uint32_t hz);
  void setOffset(int xOffset); // 运行期调整列起始偏移，便于硬件接线差异或与其它设备共享
  void setBitmapMSBLeft(bool v) { _bitmapMSBLeft = v; } // 设置位图解码使用 MSB 在左（常见某些工具导出的位序）
  void setBitmapFlipHorizontal(bool v) { _bitmapFlipH = v; }
  void setBitmapFlipVertical(bool v) { _bitmapFlipV = v; }

private:
  static constexpr int WIDTH = 128;
  static constexpr int HEIGHT = 64;
  static constexpr int INTERNAL_WIDTH = 132;
  static constexpr int PAGE_SIZE = 8;

  int _cs, _rst, _a0, _sck, _sda, _xOffset;
  bool _isOn;
  uint8_t _textSize;
  uint8_t _color;
  int _cursorX, _cursorY;

  int _blPin = -1;
  uint8_t _blChannel = 0;
  uint8_t _blResolution = 8;
  uint32_t _blFreq = 5000;
  bool _blInited = false;
  bool _blInvert = false;

  uint8_t* _buffer;

  inline void writeByte(uint8_t v, bool cmd);
  inline void writeCommand(uint8_t v) { writeByte(v, true); }
  inline void writeData(uint8_t v) { writeByte(v, false); }

  void initPins();
  void hwInitSequence();
  void flush();
  void setPixel(int x, int y, bool color);
  void drawChar(int x, int y, char c);
  void testPattern();

  bool _segRemap = false;     // false=A0(正常) true=A1(段重映射)
  bool _comReverse = true;    // true=C8(行反向) false=C0(正常)
  bool _lsbAtTop = true;      // true: 字节 bit0 对应顶部像素
  uint32_t _spiFreq = 800000; // SPI 频率，可调低排查乱码
  bool _bitmapMSBLeft = true;  // 默认采用 MSB 左，匹配导出工具
  bool _bitmapFlipH = true;    // 默认启用水平翻转，纠正发现的镜像
  bool _bitmapFlipV = false;   // 位图垂直翻转
};

extern const uint8_t ST7567_FONT6x8[][8];

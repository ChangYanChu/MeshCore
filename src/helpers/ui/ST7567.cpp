#include <Arduino.h>
#include "ST7567.h"
#include "esp32-hal.h"
#include <string.h>

#ifndef MSBFIRST
#define MSBFIRST 1
#endif

// 控制命令宏 (与头文件中私有使用一致)
static constexpr uint8_t CMD_PAGE_BASE = 0xB0;          // page address base
static constexpr uint8_t CMD_COL_HIGH_BASE = 0x10;      // column addr high nibble base
static constexpr uint8_t CMD_COL_LOW_BASE  = 0x00;      // column addr low nibble base
static constexpr uint8_t CMD_BRIGHTNESS    = 0x81;      // electronic volume set

// 5x8 列式字体 (ASCII 0x20-0x7E)，每字符 5 列，bit0 顶部。最后一列空白由绘制逻辑补。
static const uint8_t FONT5x8[] PROGMEM = {
  0x00,0x00,0x00,0x00,0x00, // space
  0x00,0x00,0x5F,0x00,0x00, // !
  0x00,0x07,0x00,0x07,0x00, // "
  0x14,0x7F,0x14,0x7F,0x14, // #
  0x24,0x2A,0x7F,0x2A,0x12, // $
  0x23,0x13,0x08,0x64,0x62, // %
  0x36,0x49,0x55,0x22,0x50, // &
  0x00,0x05,0x03,0x00,0x00, // '
  0x00,0x1C,0x22,0x41,0x00, // (
  0x00,0x41,0x22,0x1C,0x00, // )
  0x14,0x08,0x3E,0x08,0x14, // *
  0x08,0x08,0x3E,0x08,0x08, // +
  0x00,0x50,0x30,0x00,0x00, // ,
  0x08,0x08,0x08,0x08,0x08, // -
  0x00,0x60,0x60,0x00,0x00, // .
  0x20,0x10,0x08,0x04,0x02, // /
  0x3E,0x51,0x49,0x45,0x3E, // 0
  0x00,0x42,0x7F,0x40,0x00, // 1
  0x42,0x61,0x51,0x49,0x46, // 2
  0x21,0x41,0x45,0x4B,0x31, // 3
  0x18,0x14,0x12,0x7F,0x10, // 4
  0x27,0x45,0x45,0x45,0x39, // 5
  0x3C,0x4A,0x49,0x49,0x30, // 6
  0x01,0x71,0x09,0x05,0x03, // 7
  0x36,0x49,0x49,0x49,0x36, // 8
  0x06,0x49,0x49,0x29,0x1E, // 9
  0x00,0x36,0x36,0x00,0x00, // :
  0x00,0x56,0x36,0x00,0x00, // ;
  0x08,0x14,0x22,0x41,0x00, // <
  0x14,0x14,0x14,0x14,0x14, // =
  0x00,0x41,0x22,0x14,0x08, // >
  0x02,0x01,0x51,0x09,0x06, // ?
  0x32,0x49,0x79,0x41,0x3E, // @
  0x7E,0x11,0x11,0x11,0x7E, // A
  0x7F,0x49,0x49,0x49,0x36, // B
  0x3E,0x41,0x41,0x41,0x22, // C
  0x7F,0x41,0x41,0x22,0x1C, // D
  0x7F,0x49,0x49,0x49,0x41, // E
  0x7F,0x09,0x09,0x09,0x01, // F
  0x3E,0x41,0x49,0x49,0x7A, // G
  0x7F,0x08,0x08,0x08,0x7F, // H
  0x00,0x41,0x7F,0x41,0x00, // I
  0x20,0x40,0x41,0x3F,0x01, // J
  0x7F,0x08,0x14,0x22,0x41, // K
  0x7F,0x40,0x40,0x40,0x40, // L
  0x7F,0x02,0x04,0x02,0x7F, // M
  0x7F,0x04,0x08,0x10,0x7F, // N
  0x3E,0x41,0x41,0x41,0x3E, // O
  0x7F,0x09,0x09,0x09,0x06, // P
  0x3E,0x41,0x51,0x21,0x5E, // Q
  0x7F,0x09,0x19,0x29,0x46, // R
  0x46,0x49,0x49,0x49,0x31, // S
  0x01,0x01,0x7F,0x01,0x01, // T
  0x3F,0x40,0x40,0x40,0x3F, // U
  0x1F,0x20,0x40,0x20,0x1F, // V
  0x7F,0x20,0x18,0x20,0x7F, // W
  0x63,0x14,0x08,0x14,0x63, // X
  0x03,0x04,0x78,0x04,0x03, // Y
  0x61,0x51,0x49,0x45,0x43, // Z
  0x00,0x7F,0x41,0x41,0x00, // [
  0x02,0x04,0x08,0x10,0x20, // backslash
  0x00,0x41,0x41,0x7F,0x00, // ]
  0x04,0x02,0x01,0x02,0x04, // ^
  0x40,0x40,0x40,0x40,0x40, // _
  0x00,0x01,0x02,0x00,0x00, // '
  0x20,0x54,0x54,0x54,0x78, // a
  0x7F,0x48,0x44,0x44,0x38, // b
  0x38,0x44,0x44,0x44,0x20, // c
  0x38,0x44,0x44,0x48,0x7F, // d
  0x38,0x54,0x54,0x54,0x18, // e
  0x08,0x7E,0x09,0x01,0x02, // f
  0x08,0x54,0x54,0x54,0x3C, // g
  0x7F,0x08,0x04,0x04,0x78, // h
  0x00,0x44,0x7D,0x40,0x00, // i
  0x20,0x40,0x44,0x3D,0x00, // j
  0x7F,0x10,0x28,0x44,0x00, // k
  0x00,0x41,0x7F,0x40,0x00, // l
  0x7C,0x04,0x18,0x04,0x78, // m
  0x7C,0x08,0x04,0x04,0x78, // n
  0x38,0x44,0x44,0x44,0x38, // o
  0x7C,0x14,0x14,0x14,0x08, // p
  0x08,0x14,0x14,0x18,0x7C, // q
  0x7C,0x08,0x04,0x04,0x08, // r
  0x48,0x54,0x54,0x54,0x20, // s
  0x04,0x3F,0x44,0x40,0x20, // t
  0x3C,0x40,0x40,0x20,0x7C, // u
  0x1C,0x20,0x40,0x20,0x1C, // v
  0x3C,0x40,0x30,0x40,0x3C, // w
  0x44,0x28,0x10,0x28,0x44, // x
  0x0C,0x50,0x50,0x50,0x3C, // y
  0x44,0x64,0x54,0x4C,0x44, // z
  0x00,0x08,0x36,0x41,0x00, // {
  0x00,0x00,0x7F,0x00,0x00, // |
  0x00,0x41,0x36,0x08,0x00, // }
  0x10,0x08,0x08,0x10,0x08  // ~
};

ST7567Display::ST7567Display(int cs, int rst, int a0, int sck, int sda, int xOffset)
	: DisplayDriver(WIDTH, HEIGHT), _cs(cs), _rst(rst), _a0(a0), _sck(sck), _sda(sda), _xOffset(xOffset) {
	if (_xOffset < 0) _xOffset = 0;
	if (_xOffset > (INTERNAL_WIDTH - WIDTH)) _xOffset = INTERNAL_WIDTH - WIDTH;
	_isOn = false;
	_textSize = 1;
	_color = 1; // 默认白色像素
	_cursorX = 0; _cursorY = 0;
	_buffer = (uint8_t*)malloc(INTERNAL_WIDTH * HEIGHT / PAGE_SIZE);
	if (_buffer) memset(_buffer, 0, INTERNAL_WIDTH * HEIGHT / PAGE_SIZE);
}

ST7567Display::~ST7567Display() {
	if (_buffer) {
		free(_buffer);
		_buffer = nullptr;
	}
}

bool ST7567Display::begin() {
	if (!_buffer) return false;
	initPins();
	// ESP32C3 需要提供一个有效的 MISO 引脚，虽然此屏只用到 MOSI/SCK。
	// 若已有 LoRa 总线引脚 (P_LORA_MISO) 则共享；否则使用 -1（部分平台可能报错）。
#ifdef P_LORA_MISO
	SPI.begin(_sck, P_LORA_MISO, _sda, _cs);
#else
	SPI.begin(_sck, -1, _sda, _cs);
#endif
	// 硬件复位脉冲
	digitalWrite(_rst, LOW);
	delay(2);
	digitalWrite(_rst, HIGH);
	delay(10);
	hwInitSequence();
	_isOn = true;
	return true;
}

void ST7567Display::initPins() {
	pinMode(_cs, OUTPUT);
	pinMode(_rst, OUTPUT);
	pinMode(_a0, OUTPUT);
	pinMode(_sck, OUTPUT);
	pinMode(_sda, OUTPUT);
	digitalWrite(_rst, HIGH);
	digitalWrite(_cs, HIGH);
}

inline void ST7567Display::writeByte(uint8_t v, bool cmd) {
	digitalWrite(_a0, cmd ? LOW : HIGH);
	digitalWrite(_cs, LOW);
	SPI.beginTransaction(SPISettings(_spiFreq, MSBFIRST, SPI_MODE0));
	SPI.transfer(v);
	SPI.endTransaction();
	digitalWrite(_cs, HIGH);
}

void ST7567Display::hwInitSequence() {
	writeCommand(0xE2); // 软件复位
	delay(10);
	writeCommand(_segRemap ? 0xA1 : 0xA0); // 段重映射可选
	writeCommand(_comReverse ? 0xC8 : 0xC0); // 行方向
	writeCommand(0xA2); // bias 1/9
	writeCommand(0x2F); // power control
	writeCommand(0x25); // resistor ratio (0x20|0x05)
	writeCommand(CMD_BRIGHTNESS);
	writeCommand(0x19); // electronic volume 微调
	writeCommand(0x40); // start line 0
	writeCommand(0xAF); // display on
}

void ST7567Display::turnOn() { writeCommand(0xAF); _isOn = true; }
void ST7567Display::turnOff() { writeCommand(0xAE); _isOn = false; }

void ST7567Display::clear() {
	if (_buffer) memset(_buffer, 0, INTERNAL_WIDTH * HEIGHT / PAGE_SIZE);
	flush();
}

void ST7567Display::startFrame(Color bkg) {
	if (_buffer) memset(_buffer, (bkg!=0)?0xFF:0x00, INTERNAL_WIDTH * HEIGHT / PAGE_SIZE);
	_cursorX = 0; _cursorY = 0;
	_color = 1;
}

void ST7567Display::setTextSize(int sz) { _textSize = (sz<=1)?1:1; }
void ST7567Display::setColor(Color c) { _color = (c!=0)?1:0; }
void ST7567Display::setCursor(int x, int y) { _cursorX = x; _cursorY = y; }

uint16_t ST7567Display::getTextWidth(const char* str) {
	if (!str) return 0;
	return (uint16_t)(strlen(str) * 6 * _textSize); // 5 列数据 + 1 列空白
}

void ST7567Display::drawChar(int x, int y, char c) {
	if (c < ' ' || c > '~') c = '?';
	uint8_t idx = (uint8_t)c - ' ';
	const uint8_t* glyph = &FONT5x8[idx * 5];
	for (int col=0; col<5; col++) {
		uint8_t pattern = pgm_read_byte(glyph + col);
		for (int row=0; row<8; row++) {
			bool pixelOn = _lsbAtTop ? (pattern & (1 << row)) : (pattern & (1 << (7 - row)));
			setPixel(x+col, y+row, pixelOn && (_color==1));
		}
	}
	for (int row=0; row<8; row++) setPixel(x+5, y+row, false); // 间隔列
}

void ST7567Display::print(const char* str) {
	if (!str) return;
	while (*str) {
		if (_cursorX + 6 > WIDTH) { _cursorX = 0; _cursorY += 8; }
		if (_cursorY + 8 > HEIGHT) break;
		drawChar(_cursorX, _cursorY, *str++);
		_cursorX += 6;
	}
}

void ST7567Display::setPixel(int x, int y, bool color) {
	if (x<0 || y<0 || x>=WIDTH || y>=HEIGHT) return;
	int phys_col = x + _xOffset;
	if (phys_col >= INTERNAL_WIDTH) return;
	size_t index = (y / PAGE_SIZE) * INTERNAL_WIDTH + phys_col;
	uint8_t mask = (1 << (y % PAGE_SIZE));
	if (color) _buffer[index] |= mask; else _buffer[index] &= ~mask;
}

void ST7567Display::fillRect(int x, int y, int w, int h) {
	if (w<=0 || h<=0) return;
	for (int yy=y; yy<y+h; yy++) {
		for (int xx=x; xx<x+w; xx++) {
			setPixel(xx, yy, _color==1);
		}
	}
}

void ST7567Display::drawRect(int x, int y, int w, int h) {
	if (w<=0 || h<=0) return;
	for (int xx=x; xx<x+w; xx++) { setPixel(xx, y, _color==1); setPixel(xx, y+h-1, _color==1); }
	for (int yy=y; yy<y+h; yy++) { setPixel(x, yy, _color==1); setPixel(x+w-1, yy, _color==1); }
}

void ST7567Display::drawXbm(int x, int y, const uint8_t* bits, int w, int h) {
	if (!bits) return;
	int bytes_per_row = (w + 7)/8;
	// 大尺寸横幅 (例如 128x13 logo) 不需要水平翻转，避免文字反向；保留小图标翻转。
	bool effectiveFlipH = _bitmapFlipH;
	if (w >= 100) effectiveFlipH = false; // 阈值经验：logo 宽度>>小图标
	for (int yy=0; yy<h; yy++) {
		for (int byteIndex=0; byteIndex<bytes_per_row; byteIndex++) {
			uint8_t b = bits[yy*bytes_per_row + byteIndex];
			if (_bitmapMSBLeft) {
				for (int bit=0; bit<8; bit++) {
					int rawX = byteIndex*8 + bit;
					int xx = effectiveFlipH ? (x + (w - 1 - rawX)) : (x + rawX);
					int yyDraw = _bitmapFlipV ? (y + (h - 1 - yy)) : (y + yy);
					if (xx >= x + w) break;
					bool on = (b & (0x80 >> bit)) != 0; // MSB 在左
					setPixel(xx, yyDraw, on && (_color==1));
				}
			} else {
				for (int bit=0; bit<8; bit++) {
					int rawX = byteIndex*8 + bit;
					int xx = effectiveFlipH ? (x + (w - 1 - rawX)) : (x + rawX);
					int yyDraw = _bitmapFlipV ? (y + (h - 1 - yy)) : (y + yy);
					if (xx >= x + w) break;
					bool on = (b & (1 << bit)) != 0; // LSB 在左
					setPixel(xx, yyDraw, on && (_color==1));
				}
			}
		}
	}
}

void ST7567Display::flush() {
	if (!_buffer) return;
	// 与 LoRa 共享 SPI 时：一次 page 保持 CS 低，减少频繁拉高拉低导致其它设备误触发。
	for (uint8_t page=0; page < (HEIGHT / PAGE_SIZE); page++) {
		// 设置页与列起始（命令阶段）
		writeCommand(CMD_PAGE_BASE + page);
		uint8_t high = (_xOffset >> 4) & 0x0F;
		uint8_t low  = _xOffset & 0x0F;
		writeCommand(CMD_COL_HIGH_BASE | high);
		writeCommand(CMD_COL_LOW_BASE  | low);
		// 数据阶段：A0=HIGH, 单一事务发送整页
		digitalWrite(_a0, HIGH);
		digitalWrite(_cs, LOW);
		SPI.beginTransaction(SPISettings(_spiFreq, MSBFIRST, SPI_MODE0));
		size_t base = page * INTERNAL_WIDTH + _xOffset;
		for (int col=0; col < WIDTH; col++) {
			SPI.transfer(_buffer[base + col]);
		}
		SPI.endTransaction();
		digitalWrite(_cs, HIGH);
	}
}

void ST7567Display::endFrame() { flush(); }

void ST7567Display::setContrast(uint8_t resistor_ratio, uint8_t electronic_volume) {
	writeCommand(0x20 | (resistor_ratio & 0x07));
	writeCommand(CMD_BRIGHTNESS);
	writeCommand(electronic_volume & 0x3F);
}

void ST7567Display::setInverse(bool inv) { writeCommand(inv ? 0xA7 : 0xA6); }

void ST7567Display::configureOrientation(bool segRemap, bool comReverse, bool lsbAtTop) {
	_segRemap = segRemap;
	_comReverse = comReverse;
	_lsbAtTop = lsbAtTop;
	// 重新应用相关指令（不做全复位，必要可调用 begin 前设置）
	writeCommand(_segRemap ? 0xA1 : 0xA0);
	writeCommand(_comReverse ? 0xC8 : 0xC0);
}

void ST7567Display::setSpiFrequency(uint32_t hz) {
	if (hz < 100000) hz = 100000;
	if (hz > 8000000) hz = 8000000; // 安全上限
	_spiFreq = hz;
}

void ST7567Display::setOffset(int xOffset) {
	if (xOffset < 0) xOffset = 0;
	if (xOffset > (INTERNAL_WIDTH - WIDTH)) xOffset = INTERNAL_WIDTH - WIDTH;
	_xOffset = xOffset;
	flush(); // 立即重绘
}

void ST7567Display::initBacklight(int pin, uint8_t channel, uint32_t freq, uint8_t resolution, bool invert) {
	_blPin = pin; _blChannel = channel; _blFreq = freq; _blResolution = (resolution>14)?14:resolution; _blInvert = invert;
	ledcSetup(_blChannel, _blFreq, _blResolution);
	ledcAttachPin(_blPin, _blChannel);
	_blInited = true;
	uint32_t maxDuty = (1u << _blResolution) - 1u;
	ledcWrite(_blChannel, _blInvert ? 0 : maxDuty);
}

void ST7567Display::setBacklight(uint8_t value) {
	if (!_blInited) return;
	uint32_t maxDuty = (1u << _blResolution) - 1u;
	uint32_t duty = (uint32_t)((value/255.0f) * maxDuty + 0.5f);
	if (_blInvert) duty = maxDuty - duty;
	ledcWrite(_blChannel, duty);
}

uint8_t ST7567Display::getBacklight() {
	if (!_blInited) return 0;
	uint32_t maxDuty = (1u << _blResolution) - 1u;
	uint32_t duty = ledcRead(_blChannel);
	if (_blInvert) duty = maxDuty - duty;
	return (uint8_t)((duty * 255 + maxDuty/2) / maxDuty);
}

// 其它高级绘图（线/圆等）需要时再添加。

void ST7567Display::testPattern() {
	startFrame();
	for (int x=0; x<WIDTH; x++) {
		if (x % 8 < 4) {
			for (int y=0; y<HEIGHT; y++) setPixel(x,y,true);
		}
	}
	for (int gx=0; gx<WIDTH; gx+=8) {
		for (int gy=0; gy<HEIGHT; gy+=8) {
			for (int xx=gx; xx<gx+8 && xx<WIDTH; xx++) {
				setPixel(xx, gy, true);
				setPixel(xx, gy+7 < HEIGHT ? gy+7 : gy, true);
			}
			for (int yy=gy; yy<gy+8 && yy<HEIGHT; yy++) {
				setPixel(gx, yy, true);
				setPixel(gx+7 < WIDTH ? gx+7 : gx, yy, true);
			}
		}
	}
	endFrame();
}

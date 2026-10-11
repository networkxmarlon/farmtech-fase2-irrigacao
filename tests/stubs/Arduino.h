#pragma once
#include <cstdint>
#include <cstddef>
#include <string>
#include <cstdarg>
#include <cstdio>
#define HIGH 1
#define LOW 0
#define INPUT_PULLUP 2
#define OUTPUT 3
#define ADC_11db 3
inline uint32_t tempoTeste = 0;
inline int gpioTeste[40] = {};
inline void pinMode(int,int) {}
inline void digitalWrite(int pin,int valor) { gpioTeste[pin] = valor; }
inline int digitalRead(int pin) { return gpioTeste[pin]; }
inline int analogRead(int) { return 1755; }
inline void analogReadResolution(int) {}
inline void analogSetPinAttenuation(int,int) {}
inline uint32_t millis() { return tempoTeste; }
inline void delay(int) {}
struct SerialStub {
  std::string saida;
  void begin(int) {}
  int available() { return 0; }
  int read() { return -1; }
  void println(const char* s) { saida += s; saida += '\n'; }
  void printf(const char* fmt,...) {
    char buf[1024]; va_list a; va_start(a,fmt);
    vsnprintf(buf,sizeof(buf),fmt,a); va_end(a); saida += buf;
  }
};
inline SerialStub Serial;

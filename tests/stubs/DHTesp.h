#pragma once
struct TempAndHumidity { float temperature; float humidity; };
class DHTesp {
public:
  enum { DHT22 = 22 };
  void setup(int,int) {}
  TempAndHumidity getTempAndHumidity() { return {24,50}; }
  int getStatus() { return 0; }
};

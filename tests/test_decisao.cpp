// Testa o proprio sketch com entradas de hardware substituidas, nao um modelo duplicado.
#include <cassert>
#include <iostream>
#include "../esp32/sketch.ino"
int main() {
  assert(!decidir(50,true,false,false,false,0,0,-1).ligar);
  assert(decidir(39,true,false,false,false,0,0,-1).ligar);
  assert(!decidir(40,true,false,false,false,0,0,-1).ligar);
  assert(decidir(40,true,false,true,false,0,0,-1).ligar);
  assert(decidir(59.9,true,false,true,false,0,0,-1).ligar);
  assert(!decidir(60,true,false,true,false,0,0,-1).ligar);
  assert(!decidir(35,true,false,true,true,3,60,-1).ligar);
  assert(decidir(25,true,false,false,true,8,95,0).ligar);
  assert(decidir(35,true,false,false,true,2.99,80,-1).ligar);
  assert(decidir(35,true,false,false,true,10,59.9,-1).ligar);
  assert(!decidir(NAN,true,false,true,false,0,0,-1).ligar);
  assert(!decidir(35,false,false,true,false,0,0,-1).ligar);
  assert(!decidir(-1,true,false,true,false,0,0,-1).ligar);
  assert(!decidir(101,true,false,true,false,0,0,-1).ligar);
  assert(!decidir(20,true,true,true,false,0,0,1).ligar);
  assert(decidir(50,true,false,false,false,0,0,1).ligar);
  assert(!decidir(35,true,false,true,false,0,0,0).ligar);
  assert(!decidir(50,true,false,true,true,4,80,1).ligar);
  assert(!decidir(65,true,false,true,false,0,0,1).ligar);
  assert(vigente(true,99,0,100));
  assert(!vigente(true,100,0,100));
  assert(!vigente(false,10,0,100));
  assert(vigente(true,10,UINT32_MAX-20,100)); // wrap de millis.
  processarComando("CHUVA 5 80", 100);
  assert(climaRecebido && chuvaMm == 5 && probChuva == 80);
  processarComando("CHUVA 1 20 lixo", 200);
  assert(chuvaMm == 5 && probChuva == 80 && climaEm == 100);
  processarComando("CHUVA nan 80", 200);
  assert(climaEm == 100);
  processarComando("CHUVA 1 101", 200);
  assert(climaEm == 100);
  processarComando("CLIMA OFF", 200);
  assert(!climaRecebido);
  processarComando("R LIGAR", 200);
  assert(rRecomendacao == 1);
  processarComando("R AUTO", 200);
  assert(rRecomendacao == -1);
  umidade = 35; dhtValido = true; bomba = false;
  controlar(1000);
  assert(bomba && gpioTeste[PIN_RELE] == RELE_LIGADO);
  controlar(121000);
  assert(!bomba && bloqueado && gpioTeste[PIN_RELE] == RELE_DESLIGADO);
  controlar(122000); assert(!bomba);
  processarComando("REARME", 123000); controlar(123000); assert(bomba);
  dhtValido = false; controlar(124000); assert(!bomba);
  Botao b{PIN_N}; gpioTeste[PIN_N] = LOW;
  assert(!b.atualizar(0)); assert(!b.atualizar(39));
  assert(b.atualizar(40) && b.pressionado);
  gpioTeste[PIN_N] = HIGH;
  assert(!b.atualizar(50)); assert(b.atualizar(90) && !b.pressionado);
  std::cout << "OK: histerese, limites, clima, R, falha, timeout, parser e debounce.\n";
}

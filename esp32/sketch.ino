#include <Arduino.h>
#include <DHTesp.h>
#include <cmath>
#include <cstdio>
#include <cstring>

// FarmTech Solutions, Fase 2: cafe. Sensores agricolas substituidos didaticamente.
constexpr int PIN_N = 18, PIN_P = 19, PIN_K = 21;
constexpr int PIN_DHT = 15, PIN_LDR = 34, PIN_RELE = 23;
// diagram.json usa transistor=pnp (active-low): LOW fecha COM-NO.
// A polaridade foi verificada pelo LED ligado aos contatos no simulador.
constexpr int RELE_LIGADO = LOW, RELE_DESLIGADO = HIGH;
constexpr uint32_t LEITURA_MS = 2000, DEBOUNCE_MS = 40;
constexpr uint32_t CLIMA_TTL_MS = 6UL * 60UL * 60UL * 1000UL;
constexpr uint32_t R_TTL_MS = 120000, MAX_BOMBA_MS = 120000;
constexpr float UMIDADE_CRITICA = 25, CHUVA_MIN_MM = 3, PROB_MIN = 60;
constexpr float PH_MIN = 5.5, PH_MAX = 6.5;
float umidadeLigar = 40, umidadeDesligar = 60;

struct Decisao { bool ligar; const char* motivo; };

// Funcao pura, testada em tests/test_decisao.cpp. NPK/pH geram alertas separados.
Decisao decidir(float umidade, bool sensorValido, bool bloqueado,
               bool bombaAtual, bool climaValido, float chuva, float prob,
               int recomendacaoR, float minimo = 40, float maximo = 60) {
  if (bloqueado) return {false, "TEMPO_MAXIMO_REARME_NECESSARIO"};
  if (!sensorValido || !std::isfinite(umidade) || umidade < 0 || umidade > 100)
    return {false, "FALHA_DHT22"};
  if (umidade >= maximo) return {false, "UMIDADE_SUFICIENTE"};
  if (umidade <= UMIDADE_CRITICA) return {true, "SOLO_CRITICAMENTE_SECO"};
  if (climaValido && chuva >= CHUVA_MIN_MM && prob >= PROB_MIN)
    return {false, "CHUVA_PREVISTA"};
  if (recomendacaoR == 0) return {false, "RECOMENDACAO_R_DESLIGAR"};
  if (recomendacaoR == 1) return {true, "RECOMENDACAO_R_LIGAR"};
  if (umidade < minimo) return {true, "UMIDADE_BAIXA"};
  return {bombaAtual, bombaAtual ? "HISTERESE_MANTEM_LIGADA" : "HISTERESE_MANTEM_DESLIGADA"};
}

bool vigente(bool recebido, uint32_t agora, uint32_t desde, uint32_t ttl) {
  return recebido && uint32_t(agora - desde) < ttl;
}

struct Botao {
  int pin;
  bool ultimoBruto = false, pressionado = false;
  uint32_t mudouEm = 0;
  void iniciar() { pinMode(pin, INPUT_PULLUP); }
  bool atualizar(uint32_t agora) {
    bool bruto = digitalRead(pin) == LOW;
    if (bruto != ultimoBruto) { ultimoBruto = bruto; mudouEm = agora; }
    if (uint32_t(agora - mudouEm) >= DEBOUNCE_MS && pressionado != bruto) {
      pressionado = bruto;
      return true;
    }
    return false;
  }
};

DHTesp dht;
Botao botoes[3] = {{PIN_N}, {PIN_P}, {PIN_K}};
float umidade = NAN, temperatura = NAN, ph = NAN;
int adc = 0;
bool dhtValido = false, bomba = false, bloqueado = false;
bool climaRecebido = false;
float chuvaMm = 0, probChuva = 0;
uint32_t climaEm = 0, rEm = 0, bombaEm = 0, ultimaLeitura = 0;
int rRecomendacao = -1;  // -1=automatico local; 0=desligar; 1=ligar.
char linha[96];
size_t posLinha = 0;
bool descartarLinha = false;
const char* motivo = "AGUARDANDO_DHT22";

void cabecalho() {
  Serial.println("ms,umidade,temperatura,adc,ph,n,p,k,bomba,clima_valido,chuva_mm,prob_chuva,r_valido,motivo");
}

void ajudar() {
  Serial.println("# FarmTech Cafe | DHT22 e LDR sao substituicoes didaticas.");
  Serial.println("# CHUVA <mm_proximas_6h> <maior_prob_horaria_%> (ex.: CHUVA 5 80)");
  Serial.println("# CLIMA OFF | R LIGAR | R DESLIGAR | R AUTO | REARME | STATUS | AJUDA");
  Serial.println("# N/P/K pressionados=adequado; soltos=insuficiente. Ajuste o LDR ao mudar NPK.");
  cabecalho();
}

void imprimirStatus(uint32_t agora) {
  bool cv = vigente(climaRecebido, agora, climaEm, CLIMA_TTL_MS);
  bool rv = vigente(rRecomendacao != -1, agora, rEm, R_TTL_MS);
  Serial.printf("%lu,%.1f,%.1f,%d,%.2f,%d,%d,%d,%d,%d,%.2f,%.1f,%d,%s\n",
    (unsigned long)agora, umidade, temperatura, adc, ph,
    botoes[0].pressionado, botoes[1].pressionado, botoes[2].pressionado,
    bomba, cv, cv ? chuvaMm : NAN, cv ? probChuva : NAN, rv, motivo);
}

void processarComando(const char* comando, uint32_t agora) {
  float mm = 0, prob = 0;
  int fim = 0;
  if (std::sscanf(comando, "CHUVA %f %f %n", &mm, &prob, &fim) == 2 &&
      fim > 0 && comando[fim] == '\0' && std::isfinite(mm) && mm >= 0 &&
      std::isfinite(prob) && prob >= 0 && prob <= 100) {
    chuvaMm = mm; probChuva = prob; climaEm = agora; climaRecebido = true;
    Serial.println("# Previsao recebida, validade de 6 horas de tempo simulado.");
  } else if (strcmp(comando, "CLIMA OFF") == 0) {
    climaRecebido = false; Serial.println("# Previsao removida; controle local.");
  } else if (strcmp(comando, "R LIGAR") == 0 || strcmp(comando, "R DESLIGAR") == 0) {
    rRecomendacao = strcmp(comando, "R LIGAR") == 0 ? 1 : 0; rEm = agora;
    Serial.println("# Recomendacao R recebida, validade 120 segundos de tempo simulado.");
  } else if (strcmp(comando, "R AUTO") == 0) {
    rRecomendacao = -1; Serial.println("# Controle por histerese local restaurado.");
  } else if (strcmp(comando, "REARME") == 0) {
    bloqueado = false; Serial.println("# Bloqueio de tempo rearmado.");
  } else if (strcmp(comando, "STATUS") == 0) {
    imprimirStatus(agora);
  } else if (strcmp(comando, "AJUDA") == 0) {
    ajudar();
  } else {
    Serial.println("# Comando invalido. Digite AJUDA. Use ponto decimal e Enter.");
  }
}

void lerSerial(uint32_t agora) {
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\r') continue;
    if (c == '\n') {
      if (descartarLinha) Serial.println("# Linha muito longa descartada.");
      else if (posLinha) { linha[posLinha] = '\0'; processarComando(linha, agora); }
      posLinha = 0; descartarLinha = false;
    } else if (!descartarLinha) {
      if (posLinha < sizeof(linha) - 1) linha[posLinha++] = c;
      else descartarLinha = true;
    }
  }
}

void controlar(uint32_t agora) {
  if (bomba && uint32_t(agora - bombaEm) >= MAX_BOMBA_MS) bloqueado = true;
  bool cv = vigente(climaRecebido, agora, climaEm, CLIMA_TTL_MS);
  int rr = vigente(rRecomendacao != -1, agora, rEm, R_TTL_MS) ? rRecomendacao : -1;
  Decisao d = decidir(umidade, dhtValido, bloqueado, bomba, cv, chuvaMm, probChuva,
                      rr, umidadeLigar, umidadeDesligar);
  if (d.ligar != bomba) {
    bomba = d.ligar;
    if (bomba) bombaEm = agora;
    digitalWrite(PIN_RELE, bomba ? RELE_LIGADO : RELE_DESLIGADO);
    Serial.printf("# BOMBA %s: %s\n", bomba ? "LIGADA" : "DESLIGADA", d.motivo);
  }
  motivo = d.motivo;
}

void setup() {
  digitalWrite(PIN_RELE, RELE_DESLIGADO);
  pinMode(PIN_RELE, OUTPUT);
  Serial.begin(115200);
  for (auto& b : botoes) b.iniciar();
  analogReadResolution(12);
  analogSetPinAttenuation(PIN_LDR, ADC_11db);
  dht.setup(PIN_DHT, DHTesp::DHT22);
  ultimaLeitura = millis();
  ajudar();
}

void loop() {
  uint32_t agora = millis();
  lerSerial(agora);
  bool npkMudou = false;
  for (auto& b : botoes) if (b.atualizar(agora)) npkMudou = true;
  if (npkMudou) Serial.println("# NPK alterado: ajuste manualmente a luz do LDR para representar novo pH.");

  if (uint32_t(agora - ultimaLeitura) >= LEITURA_MS) {
    ultimaLeitura = agora;
    TempAndHumidity leitura = dht.getTempAndHumidity();
    umidade = leitura.humidity; temperatura = leitura.temperature;
    dhtValido = dht.getStatus() == 0 && std::isfinite(umidade) && umidade >= 0 && umidade <= 100;
    long soma = 0;
    for (int i = 0; i < 16; ++i) soma += analogRead(PIN_LDR);
    adc = int(soma / 16);
    ph = adc * 14.0f / 4095.0f; // Conversao didatica, nao calibracao de pH real.
    controlar(agora);
    imprimirStatus(agora);
    if (!(botoes[0].pressionado && botoes[1].pressionado && botoes[2].pressionado))
      Serial.println("# ALERTA_NPK: ao menos um nutriente insuficiente; avaliar adubacao por analise de solo.");
    if (ph < PH_MIN || ph > PH_MAX)
      Serial.println("# ALERTA_PH: fora da faixa 5.5-6.5; avaliar correcao do solo.");
  }
  controlar(agora); // Comandos e timeout independem da proxima leitura do DHT.
  delay(10);
}

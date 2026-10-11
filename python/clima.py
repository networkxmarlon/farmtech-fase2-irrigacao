"""Consulta publica Open-Meteo e prepara um comando para o Monitor Serial.

Somente biblioteca padrao, Python 3.10+. Sem chaves, sem conexao USB ficticia.
"""
import argparse
from datetime import datetime, timedelta, timezone
import json
import math
from pathlib import Path
import sys
from urllib.parse import urlencode
from urllib.request import urlopen


def resumir_previsao(payload, agora, horas=6):
    hourly = payload["hourly"]
    tempos = hourly["time"]
    chuva = hourly["precipitation"]
    prob = hourly["precipitation_probability"]
    if not (len(tempos) == len(chuva) == len(prob)):
        raise ValueError("Vetores meteorologicos com comprimentos diferentes.")
    if payload.get("utc_offset_seconds", 0) != 0:
        raise ValueError("A previsao deve estar em UTC.")
    unidades = payload.get("hourly_units", {})
    if unidades.get("precipitation") != "mm" or unidades.get("precipitation_probability") != "%":
        raise ValueError("Unidades inesperadas na resposta da API.")
    # Cada valor horario resume a hora que termina em time. Inclui a proxima
    # hora inteira; a janela cobre seis periodos horarios futuros sobrepostos.
    fim = agora + timedelta(hours=horas)
    selecionados = []
    for t, mm, p in zip(tempos, chuva, prob):
        instante = datetime.fromisoformat(t).replace(tzinfo=timezone.utc)
        if agora < instante <= fim:
            if mm is None or p is None:
                raise ValueError("A API nao forneceu todos os dados da janela.")
            mm, p = float(mm), float(p)
            if not math.isfinite(mm) or mm < 0 or not math.isfinite(p) or not 0 <= p <= 100:
                raise ValueError("Valores meteorologicos invalidos.")
            selecionados.append((instante, mm, p))
    if len(selecionados) != horas:
        raise ValueError(f"Esperadas {horas} horas futuras, recebidas {len(selecionados)}.")
    if any((b[0] - a[0]).total_seconds() != 3600 for a, b in zip(selecionados, selecionados[1:])):
        raise ValueError("Horas da previsao duplicadas ou nao consecutivas.")
    acumulada = round(sum(item[1] for item in selecionados), 2)
    maior_prob = max(item[2] for item in selecionados)
    return {
        "consultado_em_utc": agora.isoformat(),
        "periodos_horarios_utc": [item[0].isoformat() for item in selecionados],
        "chuva_acumulada_mm": acumulada,
        "maior_probabilidade_horaria_percentual": maior_prob,
        "comando_serial": f"CHUVA {acumulada:.2f} {maior_prob:.1f}",
        "origem": "Open-Meteo Forecast API",
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    # Ponto aproximado demonstrativo do municipio de Varginha, nao uma fazenda cadastrada.
    parser.add_argument("--latitude", type=float, default=-21.55)
    parser.add_argument("--longitude", type=float, default=-45.43)
    parser.add_argument("--saida", type=Path, default=Path("resultados/clima"))
    args = parser.parse_args()
    if not math.isfinite(args.latitude) or not -90 <= args.latitude <= 90:
        parser.error("Latitude deve estar entre -90 e 90.")
    if not math.isfinite(args.longitude) or not -180 <= args.longitude <= 180:
        parser.error("Longitude deve estar entre -180 e 180.")
    query = urlencode({"latitude": args.latitude, "longitude": args.longitude,
        "hourly": "precipitation,precipitation_probability", "forecast_days": 2,
        "timezone": "UTC"})
    url = "https://api.open-meteo.com/v1/forecast?" + query
    try:
        with urlopen(url, timeout=20) as response:
            payload = json.load(response)
        resumo = resumir_previsao(payload, datetime.now(timezone.utc))
        resumo["coordenadas_solicitadas"] = {"latitude": args.latitude, "longitude": args.longitude}
        resumo["url_consultada"] = url
        args.saida.mkdir(parents=True, exist_ok=True)
        (args.saida / "resposta_api.json").write_text(json.dumps(payload, indent=2), encoding="utf-8")
        (args.saida / "resumo.json").write_text(json.dumps(resumo, indent=2), encoding="utf-8")
        (args.saida / "comando_serial.txt").write_text(resumo["comando_serial"] + "\n", encoding="utf-8")
    except (OSError, ValueError, KeyError, TypeError) as error:
        print(f"Consulta falhou: {error}. Nao foi criado comando novo; nao reutilize previsao antiga.", file=sys.stderr)
        return 1
    print("Ponto de demonstracao: Varginha/MG; altere as coordenadas para seu estudo.")
    print(f"Chuva acumulada em seis periodos horarios futuros: {resumo['chuva_acumulada_mm']:.2f} mm")
    print(f"Maior probabilidade HORARIA: {resumo['maior_probabilidade_horaria_percentual']:.1f}%")
    print("Este percentual nao e a probabilidade conjunta da janela de seis horas.")
    print("Cole AGORA no Monitor Serial do Wokwi e pressione Enter:")
    print(resumo["comando_serial"])
    print("A transferencia para o ESP32 e manual, conforme alternativa prevista no enunciado.")
    print("Atribuicao meteorologica: Open-Meteo, https://open-meteo.com/")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

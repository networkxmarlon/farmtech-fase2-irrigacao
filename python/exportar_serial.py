"""Extrai CSV do texto copiado do Monitor Serial, preservando os registros validos."""
import argparse
import csv
import io
from pathlib import Path

CAMPOS = "ms,umidade,temperatura,adc,ph,n,p,k,bomba,clima_valido,chuva_mm,prob_chuva,r_valido,motivo".split(",")


def extrair(texto):
    rows = []
    for linha in texto.splitlines():
        linha = linha.strip()
        if not linha or linha.startswith("#") or linha.startswith("ms,"):
            continue
        row = next(csv.reader(io.StringIO(linha)))
        if len(row) != len(CAMPOS):
            continue
        try:
            int(row[0])
            for i in range(1, 13):
                float(row[i])
        except ValueError:
            continue
        rows.append(row)
    return rows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("entrada", type=Path)
    parser.add_argument("--saida", type=Path, default=Path("resultados/sensores.csv"))
    args = parser.parse_args()
    rows = extrair(args.entrada.read_text(encoding="utf-8-sig"))
    if not rows:
        parser.error("Nenhum registro CSV encontrado no texto. Copie as linhas do Monitor Serial.")
    args.saida.parent.mkdir(parents=True, exist_ok=True)
    with args.saida.open("w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(CAMPOS)
        w.writerows(rows)
    print(f"{len(rows)} registros exportados para {args.saida}.")


if __name__ == "__main__":
    main()

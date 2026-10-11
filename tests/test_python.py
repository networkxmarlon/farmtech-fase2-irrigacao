import sys
import unittest
from datetime import datetime, timezone
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "python"))
from clima import resumir_previsao
from exportar_serial import extrair

class ClimaTest(unittest.TestCase):
    def setUp(self):
        self.agora = datetime(2026, 10, 10, 10, 30, tzinfo=timezone.utc)
        self.p = {"utc_offset_seconds": 0,
            "hourly_units": {"precipitation": "mm", "precipitation_probability": "%"},
            "hourly": {"time": [f"2026-10-10T{h:02}:00" for h in range(9,18)],
                       "precipitation": [99,99,1,0,2,0,0,0,99],
                       "precipitation_probability": [99,99,20,80,40,10,5,0,99]}}
    def test_futuro_seis_horas(self):
        r = resumir_previsao(self.p,self.agora)
        self.assertEqual(r["comando_serial"], "CHUVA 3.00 80.0")
        self.assertEqual(len(r["periodos_horarios_utc"]),6)
    def test_falha_nao_vira_sol(self):
        self.p["hourly"]["precipitation"][2] = None
        with self.assertRaises(ValueError): resumir_previsao(self.p,self.agora)
    def test_prob_invalida(self):
        self.p["hourly"]["precipitation_probability"][2] = 101
        with self.assertRaises(ValueError): resumir_previsao(self.p,self.agora)
    def test_janela_incompleta(self):
        for k in self.p["hourly"]: self.p["hourly"][k] = self.p["hourly"][k][:4]
        with self.assertRaises(ValueError): resumir_previsao(self.p,self.agora)
    def test_horas_duplicadas(self):
        self.p["hourly"]["time"][3] = self.p["hourly"]["time"][2]
        with self.assertRaises(ValueError): resumir_previsao(self.p,self.agora)
    def test_exporta_serial(self):
        text = "# BOMBA LIGADA\nms,umidade\n2000,35.0,24.0,1755,6.0,1,0,0,1,0,nan,nan,0,UMIDADE_BAIXA\nrom boot log\n"
        rows = extrair(text)
        self.assertEqual(len(rows),1)
        self.assertEqual(rows[0][1],"35.0")

if __name__ == "__main__": unittest.main()

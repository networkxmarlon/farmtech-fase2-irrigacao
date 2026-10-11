# FarmTech Fase 2: estatistica descritiva e tendencia local, sem pacotes externos.
# Uso: Rscript r/analise_irrigacao.R resultados/sensores.csv
args <- commandArgs(trailingOnly = TRUE)
entrada <- if (length(args) > 0) args[1] else "dados/exemplo_sintetico.csv"
saida <- if (length(args) > 1) args[2] else "resultados/analise_r"
dados <- read.csv(entrada, stringsAsFactors = FALSE)
colunas <- c("ms", "umidade", "ph", "n", "p", "k", "bomba")
if (!all(colunas %in% names(dados))) stop("CSV sem as colunas exigidas.")
dados[colunas] <- lapply(dados[colunas], function(x) {
  if (is.logical(x) && all(is.na(x))) as.numeric(x) else x
})
if (!all(vapply(dados[colunas], is.numeric, logical(1)))) stop("Colunas devem ser numericas.")
if (nrow(dados) < 1) stop("CSV vazio.")
# Use apenas a ultima sessao. Um reinicio do ESP32 faz ms voltar ao inicio.
reinicios <- which(diff(dados$ms) < 0)
if (length(reinicios)) dados <- dados[(tail(reinicios, 1) + 1):nrow(dados), ]
if (any(!is.finite(dados$ms))) stop("Tempos invalidos.")
cat("Origem:", entrada, "\n")
if (grepl("sintetico", basename(entrada))) cat("ATENCAO: dados SINTETICOS, sem medicao de fazenda.\n")
validos <- is.finite(dados$umidade) & dados$umidade >= 0 & dados$umidade <= 100
cat("Registros:", nrow(dados), "| Umidades validas:", sum(validos), "\n")
if (sum(validos)) {
  u <- dados$umidade[validos]
  cat(sprintf("Umidade: media %.2f%% | mediana %.2f%% | desvio %.2f | min %.1f | max %.1f\n",
              mean(u), median(u), if (length(u) > 1) sd(u) else NA_real_, min(u), max(u)))
}
ph_ok <- is.finite(dados$ph) & dados$ph >= 5.5 & dados$ph <= 6.5
cat("Registros com pH simulado na faixa 5.5-6.5:", sum(ph_ok), "\n")
for (nome in c("n", "p", "k")) cat("Registros com", toupper(nome), "insuficiente:", sum(dados[[nome]] == 0, na.rm=TRUE), "\n")
janela <- tail(dados, 5)
comando <- "R AUTO"
justificativa <- "Sem cinco leituras recentes validas; usar controle local."
media <- previsao <- inclinacao <- NA_real_
if (nrow(janela) == 5 && all(is.finite(janela$umidade)) &&
    all(janela$umidade >= 0 & janela$umidade <= 100) &&
    all(diff(janela$ms) > 0) && all(diff(janela$ms) <= 5000)) {
  janela$segundos <- (janela$ms - min(janela$ms)) / 1000
  modelo <- lm(umidade ~ segundos, data=janela)
  media <- mean(janela$umidade)
  inclinacao <- unname(coef(modelo)[2])
  passo <- median(diff(janela$segundos))
  previsao <- unname(predict(modelo, data.frame(segundos=max(janela$segundos) + passo)))
  previsao <- max(0, min(100, previsao))
  cat(sprintf("Ultimas 5: media %.2f%% | tendencia %.3f pontos/segundo | proxima leitura %.2f%%\n",
              media, inclinacao, previsao))
  if (media < 40 && previsao < 40) {
    comando <- "R LIGAR"
    justificativa <- "Media e extrapolacao de curto prazo indicam baixa umidade."
  } else if (media >= 60 && previsao >= 60) {
    comando <- "R DESLIGAR"
    justificativa <- "Media e extrapolacao de curto prazo indicam umidade suficiente."
  } else {
    justificativa <- "Zona intermediaria ou tendencia divergente; manter histerese local."
  }
}
cat(justificativa, "\nComando para o Monitor Serial:", comando, "\n")
cat("Cole logo apos a coleta. Validade no ESP32: 120 s de tempo simulado.\n")
cat("Clima, umidade critica, umidade >=60, falha DHT e limite de bomba prevalecem.\n")
cat("A regressao descreve cinco leituras: nao valida previsao agronomica nem causalidade.\n")
dir.create(saida, recursive=TRUE, showWarnings=FALSE)
writeLines(comando, file.path(saida, "comando_serial.txt"))
write.csv(data.frame(registros=nrow(dados), media_ultimas_5=media,
                     tendencia_pontos_por_segundo=inclinacao, previsao_proxima=previsao,
                     comando=comando, justificativa=justificativa),
          file.path(saida, "resumo.csv"), row.names=FALSE)
png(file.path(saida, "umidade.png"), width=1200, height=650)
plot(dados$ms / 1000, dados$umidade, type=if (any(validos)) "o" else "n", pch=16, col="#156e5c",
     xlab="Tempo simulado (s)", ylab="Umidade simulada (%)", ylim=c(0,100),
     main=paste("FarmTech - serie de umidade |", basename(entrada)))
abline(h=c(25,40,60), col=c("#bd3434", "#d49c2e", "#2470aa"), lty=c(3,2,2))
legend("topright", c("Umidade", "Critica (25)", "Liga abaixo de 40", "Desliga em 60"),
       col=c("#156e5c", "#bd3434", "#d49c2e", "#2470aa"), lty=c(1,3,2,2), bty="n")
dev.off()

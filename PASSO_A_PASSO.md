# Do pacote à entrega

## Primeiro: testar o circuito

1. Extraia o ZIP. Entre na pasta `FarmTech_Fase2`.
2. Abra https://wokwi.com/projects/new/esp32.
3. Abra `esp32/sketch.ino` em um editor de texto, copie tudo e substitua o código no Wokwi. Ctrl+A deve selecionar todo o conteúdo do editor antes de colar.
4. Faça o mesmo com `esp32/diagram.json` na aba `diagram.json`.
5. Em Library Manager, instale `DHT sensor library for ESPx`.
6. Inicie a simulação e espere a primeira linha CSV. A bomba começa desligada, com umidade 50.
7. No DHT22, teste umidade 35 → 50 → 65 → 50. Compare com a tabela do README.
8. Teste os três botões. Ctrl+clique mantém pressionado; o próximo clique libera. Após mudar NPK, mova o LDR e observe o pH no Monitor Serial.
9. Com umidade 35, envie `CHUVA 5 80` e Enter. Deve suspender a irrigação. Esse comando é um cenário controlado, não uma consulta meteorológica.
10. Se a bomba atingir 120 s ligada continuamente, envie `REARME` após avaliar o motivo.
11. Salve o projeto na sua conta Wokwi e coloque o endereço no README.

## Segundo: executar os opcionais

Na pasta do projeto, abra o terminal. No Windows, pode usar a barra de endereço do Explorador de Arquivos, digitar `cmd` e Enter.

```bash
python python/clima.py
```

Se `python` não funcionar e o Python estiver instalado, tente `py`. Copie o comando retornado para o Monitor Serial do Wokwi.

Para R, use sua instalação de R/Rscript. No RStudio, também pode abrir `r/analise_irrigacao.R` e executar o script a partir da raiz do projeto; sem argumentos, ele usa o CSV sintético.

```bash
Rscript r/analise_irrigacao.R dados/exemplo_sintetico.csv
```

Para usar leituras próprias, copie o texto do Monitor Serial para `serial.txt`, salve em UTF-8 e execute:

```bash
python python/exportar_serial.py serial.txt
Rscript r/analise_irrigacao.R resultados/sensores.csv
```

Confira a média, a tendência e a recomendação. Cole o comando no Wokwi. A integração Python/R → ESP32 deste projeto é manual; não precisa conectar uma placa física.

## Terceiro: GitHub

1. Revise o README. Confira seu nome e RM e preencha curso, turma e link Wokwi.
2. Na sua conta GitHub, crie um repositório para `farmtech-fase2-irrigacao` ou outro nome que você escolher.
3. Se o repositório estiver vazio, use o botão de upload indicado na página. Se já houver arquivos, use **Add file → Upload files**.
4. Arraste os arquivos e pastas da raiz `FarmTech_Fase2`, preservando `esp32`, `docs`, `python`, `r`, `dados`, `tests` e `video`. O `README.md` deve ficar na raiz.
5. Não envie somente o ZIP; o professor deve conseguir ver os códigos e imagens no GitHub.
6. Confirme as alterações com uma mensagem que descreva o trabalho realizado.
7. Abra o README publicado e confira as imagens. Confira também `esp32/sketch.ino` e os opcionais.

## Quarto: vídeo e conferência final

1. Siga `video/ROTEIRO_VIDEO.md`, adaptando as falas ao que você entende e aos testes que executar.
2. Grave a tela e sua explicação; mantenha até 5 minutos.
3. Envie ao YouTube e escolha **não listado**.
4. Copie o endereço real para o início do README e para `video/link_video.txt`.
5. Confira os dois links, GitHub e YouTube, em uma janela sem login ou com outra conta.
6. Entregue no portal exatamente os links/arquivos solicitados pelo professor.

## O que precisa ser completado fora deste pacote

- Conferência de seu nome e RM e preenchimento de curso/turma.
- Projeto salvo na conta Wokwi e respectivo endereço.
- Repositório GitHub publicado e revisado por você.
- Gravação, publicação e endereço real do vídeo.

As capturas, o código e os testes disponíveis no pacote apoiam a execução. Eles não substituem sua gravação de demonstração nem a revisão autoral.

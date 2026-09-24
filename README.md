# Serra_Tunnel
Sistema di automazione e monitoraggio per serra tunnel, basato su ESP32 e sviluppato con PlatformIO. Il progetto permette di controllare i parametri ambientali della serra e di visualizzarli tramite una dashboard web dedicata. Include inoltre una sezione dedicata all'ufficio (Smart Green Office) per il monitoraggio ambientale degli spazi di lavoro.

✨ Features
🌱 Monitoraggio dei parametri ambientali della serra

🎛️ Controllo automatizzato dei dispositivi (irrigazione, ventilazione, ecc.)

🏢 Sezione Smart Green Office per il monitoraggio dell'ambiente ufficio

📊 Dashboard web (Dashboard Serra.html) per visualizzazione e comando

📡 Comunicazione Bluetooth tra ESP32 e dispositivi di controllo

⚙️ Build tramite PlatformIO

📖 Manuale completo di progetto in Smart_GreenOffice_Manuale.pdf

🏗️ Architettura
text
   ┌──────────────┐        Bluetooth         ┌──────────────────┐
   │   ESP32      │ ◄──────────────────────► │  Dashboard Serra │
   │ (serra)      │                          │      .html       │
   └──────┬───────┘                          └──────────────────┘
          │
   ┌──────▼───────────────────────────────┐
   │ Sensori & attuatori                  │
   │ (temperatura, umidità, irrigazione…) │
   └──────────────────────────────────────┘

   ┌──────────────┐        Bluetooth         ┌──────────────────┐
   │   ESP32      │ ◄──────────────────────► │  Smart Green     │
   │ (ufficio)    │                          │  Office          │
   └──────────────┘                          └──────────────────┘
Il sistema si compone di due ambiti:

Serra tunnel: monitoraggio e automazione dei parametri della serra

Smart Green Office: monitoraggio ambientale dell'ufficio

Entrambi comunicano via Bluetooth e sono visualizzabili tramite la dashboard web.

📂 Struttura del repository
File / Cartella	Descrizione
SerraTunnel/	Codice sorgente del progetto (firmware ESP32, PlatformIO)
Dashboard Serra.html	Dashboard web di monitoraggio/controllo
Smart_GreenOffice_Manuale.pdf	Manuale di progetto
README.md	Questo file
.gitattributes	Normalizzazione dei file nel repo
🧰 Hardware
Board ESP32

Sensori ambientali (temperatura, umidità, luce…)

Attuatori (pompa irrigazione, ventole, relè…)

Modulo Bluetooth (integrato nell'ESP32)

Alimentazione adeguata

🚀 Installazione e avvio
Clona il repository:

bash
git clone https://github.com/gabbro95/Serra_Tunnel.git
cd Serra_Tunnel
Apri il progetto SerraTunnel/ con VS Code + PlatformIO.

Configura i parametri (pin, soglie, parametri Bluetooth) nel file di configurazione.

Compila e carica sull'ESP32:

bash
pio run -t upload
(Opzionale) Monitora il seriale:

bash
pio device monitor
Apri Dashboard Serra.html nel browser per visualizzare i dati della serra e dell'ufficio.

📖 Manuale
Per dettagli su installazione, configurazione e utilizzo consulta:
📄 Smart_GreenOffice_Manuale.pdf

🖼️ Screenshot


🤝 Contributi
Contributi e segnalazioni sono benvenuti: apri una issue o una pull request.

📄 Licenza
Rilasciato sotto licenza MIT. Vedi il file LICENSE per i dettagli.
 

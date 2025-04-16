# BNI Assignment 1 – Task Ventilator (POSIX Message Queues)

## Protokoll 

## Pierre Erhardt - 209910

### Themen

Im Rahmen dieser Übung wurden folgende Themen behandelt:

- Erzeugung paralleler Prozesse mittels `fork()`
- Kommunikation über POSIX-konforme Message Queues (`mqueue.h`)
- Verwendung blockierender Queues zur kontrollierten Datenübertragung
- Einsatz von Kommandozeilenparametern mit `getopt()`
- Strukturierte Übergabe und Rückmeldung von Tasks und Ergebnissen
- Saubere Ressourcenverwaltung nach POSIX-Standard

### Erkenntnisse

Durch die Umsetzung dieser Aufgabe konnten zentrale Konzepte der interprozessualen Kommunikation vertieft werden.  
Insbesondere wurde deutlich, wie Blocking Queues das Synchronisationsverhalten vereinfachen und wie man mehrere Prozesse gezielt steuert.

Es zeigte sich, dass die Trennung von Aufgaben- und Ergebnisverarbeitung (mittels zwei separater Queues) eine klare und skalierbare Architektur ermöglicht.  
Zudem wurde die Bedeutung einer sauberen Ressourcennutzung (`mq_close`, `mq_unlink`, `wait()`) für wiederholbares und stabiles Verhalten des Systems deutlich.

### Umsetzung

Ein Hauptprozess (Ventilator) erzeugt mithilfe von `fork()` mehrere Worker-Prozesse.  
Der Ventilator erstellt zwei POSIX-Message-Queues:

1. **task_queue**: Empfängt Aufgaben mit einem zufälligen "Effort"-Wert (1–10).
2. **result_queue**: Erhält abschließend ein Ergebnisobjekt (`result_t`) von jedem Worker.

Jeder Worker verarbeitet Aufgaben sequentiell, simuliert den Aufwand mit `sleep(effort)` und zählt dabei die Anzahl und Summe der Verarbeitungszeit.  
Ein spezieller Task mit dem Wert `0` signalisiert die Terminierung.  
Nach dem letzten Task sendet der Worker ein Ergebnis an den Ventilator, der alle Resultate ausgibt und anschließend die Prozesse wartet und die Ressourcen bereinigt.

### Zusätzliche Recherchen & Lösungen

- Die Funktion `getopt()` führte in Verbindung mit `-std=c17` zu Kompilierungsproblemen, da sie nicht Teil des ISO-C-Standards ist. Durch die Verwendung des Flags `-std=gnu17` im Makefile konnte dieses Problem gelöst werden.
- Die Initialisierung und korrekte Verwendung der Struktur `mq_attr` war erforderlich, um die Queue-Größen (Anzahl und Nachrichtengröße) anzupassen.
- Die Verwendung von Blocking-Queues wurde analysiert, um unnötige Schleifen oder Busy Waiting zu vermeiden.

### Besondere Aspekte

- Das Blocking-Verhalten der Message Queues ermöglicht eine elegante Steuerung des Programmflusses.
- Die Architektur lässt sich leicht erweitern, z. B. durch zusätzliche Tasktypen oder Rückmeldestrukturen.
- Die Kommunikation erfolgt ausschließlich über POSIX-konforme Queues, wodurch das System portabel und standardkonform bleibt.
- Der zeitlich formatierte Log-Output erlaubt eine gute Nachvollziehbarkeit der Prozessabfolge.

### Herausforderungen

- Die Nutzung von `getopt()` unter strikten Compiler-Einstellungen führte zu Problemen, die mit entsprechenden Flags gelöst werden mussten.
- Fehlerhafte Terminierung einzelner Prozesse führte in der Anfangsphase zu nicht sauber beendeten Workern.
- Die Synchronisation zwischen Ventilator und Workern musste sorgfältig getestet werden, um Datenverluste oder doppelte Task-Verarbeitung zu verhindern.
- Logging und Formatierung erforderten mehrere Überarbeitungen, um eine gut lesbare Ausgabe zu ermöglichen.

### Fazit

Die Anforderungen der Aufgabe wurden vollständig umgesetzt.  
Das entwickelte System arbeitet stabil, verarbeitet alle Tasks deterministisch und ermöglicht durch die getrennte Aufgaben- und Ergebnisverarbeitung eine klare und nachvollziehbare Architektur.  
Blocking-Mechanismen, Prozesssteuerung und Ressourcenkontrolle wurden erfolgreich miteinander kombiniert und erfüllen die vorgegebenen Kriterien.
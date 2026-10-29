# GPS-Tab

Der CamperUI-GPS-Tab liest optionale Navigationsdaten aus dem bestehenden
`/position_sensor/?request=true`-Payload. Unterstützt werden:

- `latitude` / `longitude` oder kurz `lat` / `lon`
- `accuracy` oder `accuracy_m`
- `speed_kmh` oder `speed`
- `heading`

Die Wasserwaagenwerte `x_angle` und `y_angle` bleiben unverändert kompatibel.
Auf dem ESP32 wird zunächst eine leichte native Kartenfläche mit Positionsmarker
verwendet. Eine echte OpenStreetMap-Hintergrundkarte sollte später vom VanPi/Pi
als gecachtes Bild geliefert werden; dadurch bleiben Speicherverbrauch und
Netzwerkverkehr auf dem Display niedrig.

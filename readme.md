
# Human-Tracker

Jeff's R&D into using mmWave radar to track x,y locations of people in buildings.

# Parts

 - https://a.co/d/0i4YfnZb
 - https://a.co/d/0cmmnirq
 - breadboard etc

# Dependencies

```bash
yay -S arduino-cli

# Data lives in ~/.arduino15/
arduino-cli config init
arduino-cli core update-index
arduino-cli core install arduino:avr

```

# Building & Running

```bash
arduino-cli compile --fqbn arduino:avr:uno

arduino-cli upload -p /dev/ttyACM0 --fqbn arduino:avr:uno

```


# Listing boards

```bash
arduino-cli board list

```

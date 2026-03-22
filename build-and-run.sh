#!/bin/bash

set -e

run_cmd() {
  echo "> " "$@"
  "$@"
}

run_cmd arduino-cli compile --fqbn arduino:avr:uno

deploy_program() {
  run_cmd arduino-cli upload -p /dev/ttyACM0 --fqbn arduino:avr:uno
  sleep 1
  run_cmd arduino-cli monitor -p /dev/ttyACM0 -b arduino:avr:uno
}

while true; do
    read -p "Do you wish to deploy this program? " yn
    case $yn in
        [Yy]* ) deploy_program ; break;;
        [Nn]* ) exit;;
        * ) echo "Please answer yes or no.";;
    esac
done

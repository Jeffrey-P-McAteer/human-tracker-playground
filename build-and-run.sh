#!/bin/bash

set -ex

arduino-cli compile --fqbn arduino:avr:uno

deploy_program() {
  arduino-cli upload -p /dev/ttyACM0 --fqbn arduino:avr:uno
  sleep 1
  arduino-cli monitor -p /dev/ttyACM0 -b arduino:avr:uno
}

while true; do
    read -p "Do you wish to deploy this program? " yn
    case $yn in
        [Yy]* ) deploy_program ; break;;
        [Nn]* ) exit;;
        * ) echo "Please answer yes or no.";;
    esac
done

#!/bin/bash

for i in {1..1000000}; do printf "\e[48;5;$((RANDOM%256))m%s\e[0m" " "; done

#!/bin/bash

# activate mouse tracking
printf "\e[?1000h\e[?1006h"

# wait for 3 mouse button events
printf "Click now 3x:\n"
for ((i=0; i < 3; i++)); do
	# wait for 'm' as end character, which signals button released
	read -d m
	printf "\n"
done
# deactivate mouse tracking
printf "\e[?1006l\e[?1000l\n"
printf "Done!\n"

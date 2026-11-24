clear
printf "\e[3;24r"

printf "\e7\e[1;1fThis is line # 1....\e8"
printf "\e[31m\e7\e[2;1f------ This is line # 2 (above scroll region) ....\e8\e[0m"
printf "\e7\e[3;0fThis is line # 3....\e8"
printf "\e[3;0fThis is line # 3....\n"


printf "\e7\e[24;0fThis is line # 24....\e8"
printf "\e[31m\e7\e[25;0f----- This is line # 25 (below scroll region)....\e8\e[0m"

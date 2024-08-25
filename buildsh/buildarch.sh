sudo echo Starting Build
cd ../
sudo pacman -S gcc
./include/shell/all-commands.sh
sudo make && sudo make iso
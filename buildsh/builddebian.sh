sudo echo Starting Build
cd ../
sudo apt install build-essential gcc g++
./include/shell/all-commands.sh
sudo make && sudo make iso
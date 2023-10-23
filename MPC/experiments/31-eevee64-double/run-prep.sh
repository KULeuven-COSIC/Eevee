cd MP-SPDZ
# clear directories in Player-Data
mkdir -p Player-Data
cd Player-Data
find . -maxdepth 1 -mindepth 1 -type d -exec rm -rf '{}' \;
cd ..
# time offline phase
/usr/bin/time -f 'Player: Kernel %Ss User %Us' ./mascot-offline.x -p $(cat playerid) -N $(cat nplayers) -ip HOSTS $(cat target)
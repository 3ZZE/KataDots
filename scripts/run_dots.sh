mkdir data/
mkdir data/selfplay/
mkdir data/models/

#export LD_LIBRARY_PATH=LD_LIBRARY_PATH:"/root/lib/"  #linux

. ../.venv/bin/activate

while true
do 
    chmod +x ./engine/katago  #linux
    ./engine/katago selfplay -models-dir  data/models -config selfplay.cfg -output-dir data/selfplay  -max-games-total 8000 
    cd train
    bash shuffle.sh ../data ./ktmp 16 128
    bash train.sh ../data b10c256n b10c256nbt-fson-rvgl-bnh 128 main -lr-scale 1  -samples-per-epoch 800000
    bash export.sh test ../data 0
    python view_loss.py
    cd ..
done

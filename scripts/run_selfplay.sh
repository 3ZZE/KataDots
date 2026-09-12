rm ./balancedsgfs/*
./engine/katago genbalancedopenings -models-dir ./data/models/ -sgf-dir ./balancedsgfs/ -xsize 20 -ysize 20 -num 64 -config selfplay.cfg
./engine/katago genbalancedopenings -models-dir ./data/models/ -sgf-dir ./balancedsgfs/ -xsize 18 -ysize 18 -num 32 -config selfplay.cfg
./engine/katago genbalancedopenings -models-dir ./data/models/ -sgf-dir ./balancedsgfs/ -xsize 16 -ysize 16 -num 20 -config selfplay.cfg
./engine/katago genbalancedopenings -models-dir ./data/models/ -sgf-dir ./balancedsgfs/ -xsize 14 -ysize 14 -num 10 -config selfplay.cfg

./engine/katago selfplay -models-dir  data/models -config selfplay.cfg -output-dir data/selfplay  -max-games-total 10000 

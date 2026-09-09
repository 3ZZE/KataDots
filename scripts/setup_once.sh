cd ../cpp
cmake . -DUSE_BACKEND=OPENCL -DBUILD_DISTRIBUTED=0
make -j 4
cd ../scripts/
python -m venv .venv
. .venv/bin/activate
pip install numpy==1.26.4 matplotlib==3.11.0 psutil==7.2.2 requests-toolbelt==1.0.0 scipy==1.17.1 sgfmill==1.1.1  torch==2.4.1



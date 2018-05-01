set -x
wget -nv https://github.com/misson20000/libtransistor/releases/download/b3/libtransistor_b3.tar.gz
mkdir libtransistor
tar xzf libtransistor_b3.tar.gz -C libtransistor
pip3 install --user --upgrade pip setuptools wheel
pip3 install lz4 pyelftools --user

wget -nv https://xenotoad.net/switch/build_pfs0

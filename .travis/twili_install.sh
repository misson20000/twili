set -x
wget -nv https://github.com/reswitched/libtransistor/releases/download/$LIBTRANSISTOR_VERSION/libtransistor_$LIBTRANSISTOR_VERSION.tar.gz
mkdir libtransistor
tar xzf libtransistor_$LIBTRANSISTOR_VERSION.tar.gz -C libtransistor
pip3 install --user --upgrade pip setuptools wheel
pip3 install lz4 pyelftools --user

wget -nv https://downloads.devkitpro.org/packages/linux/switch-tools-1.4.1-3-any.pkg.tar.xz
mkdir fr
tar xf switch-tools-1.4.1-3-any.pkg.tar.xz -C fr

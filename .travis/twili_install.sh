set -x
wget -nv https://github.com/$LIBTRANSISTOR_REPO/releases/download/$LIBTRANSISTOR_VERSION/libtransistor_$LIBTRANSISTOR_VERSION.tar.gz
mkdir libtransistor
tar xzf libtransistor_$LIBTRANSISTOR_VERSION.tar.gz -C libtransistor
pip3 install --user --upgrade pip setuptools wheel
pip3 install lz4 pyelftools --user

#wget http://xenotoad.net/switch_tools_elf2kip_fix.tar.gz
#mkdir fr
#tar xf switch_tools_elf2kip_fix.tar.gz -C fr

# blocked from devkitpro repos?
wget -nv http://xenotoad.net/switch/switch-tools-1.7.0-1-any.pkg.tar.xz
mkdir fr
tar xf switch-tools-1.7.0-1-any.pkg.tar.xz -C fr

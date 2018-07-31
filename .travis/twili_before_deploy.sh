cd build
zip -r twili.zip atmosphere
cd ..
mkdir -p deploy/
cp build/twili_launcher.{kip,nso} build/twili.zip deploy/

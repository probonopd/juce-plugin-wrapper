ROOT=$(cd "$(dirname "$0")/.."; pwd)

"$ROOT/build/bin/JUCE/Projucer" --resave "$ROOT/PluginWrapper.jucer"

cd "$ROOT/Builds/LinuxMakefile"
make CONFIG=Release

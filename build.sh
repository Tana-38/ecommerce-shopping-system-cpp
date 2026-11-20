#!/usr/bin/env sh
# ShopSphere build script for Linux / macOS / Git Bash.
#
#   ./build.sh          build bin/shopsphere and the test programs
#   ./build.sh test     build, then run all tests
#   ./build.sh demo     build, then run the scripted demo
#   ./build.sh run      build, then start the app
#   ./build.sh clean    delete build/ and bin/
set -e
cd "$(dirname "$0")"

CXX="${CXX:-g++}"
STD="${STD:-c++14}"
CXXFLAGS="-std=$STD -Wall -Wextra -O2 -Iinclude"
EXE=""
case "$(uname -s 2>/dev/null)" in
    MINGW*|MSYS*|CYGWIN*) EXE=".exe" ;;
esac

if [ "$1" = "clean" ]; then
    rm -r -f -- ./build ./bin
    echo " Cleaned."
    exit 0
fi

mkdir -p build bin
OBJECTS=""
echo
echo " ShopSphere - compiling modules"
echo " ------------------------------"
for module in util core cart payment services ui; do
    for src in src/$module/*.cpp; do
        name=$(basename "$src" .cpp)
        obj="build/${module}_${name}.o"
        echo "   $module/$name.cpp"
        $CXX $CXXFLAGS -c "$src" -o "$obj"
        OBJECTS="$OBJECTS $obj"
    done
done

echo "   linking bin/shopsphere$EXE"
$CXX $CXXFLAGS src/main.cpp $OBJECTS -o "bin/shopsphere$EXE"
for t in test_all test_partA test_partB; do
    echo "   linking bin/$t$EXE"
    $CXX $CXXFLAGS "tests/$t.cpp" $OBJECTS -o "bin/$t$EXE"
done
echo " Build OK."
echo

case "$1" in
    test)
        "bin/test_all$EXE"
        "bin/test_partA$EXE" > /dev/null
        "bin/test_partB$EXE" > /dev/null
        echo " Part A and Part B drivers ran successfully."
        ;;
    demo)
        shift
        "bin/shopsphere$EXE" --demo "$@"
        ;;
    run)
        shift
        "bin/shopsphere$EXE" "$@"
        ;;
esac

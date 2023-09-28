call vcvarsall.bat
mkdir release
pushd release
call cmake -S .. --preset release
call cmake --build .
call cmake --install . --prefix ..
popd

In one terminal run, 
```
cd validation
npm i
npm start
```

In another terminal run,
```
rm -rf build
mkdir -p build
cd build
cmake ..
cmake --build .
./getColor
```

Run xiao.ino in the microcontroller.


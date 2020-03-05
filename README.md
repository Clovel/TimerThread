# TimerThread

This class impelements a threaded timer/clock system. For now, it is has a microsecond resolution. Performance and precision depends on the target system.

## Building
To build this project, follow these steps : 
```bash
mkdir build
cd build
cmake .. && make
```

To build in Release configuration, follow these steps :
```bash
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release .. && make
```

A `make install` command is available, but you must specify your own destination. Otherwise, it will install to `<project/root/dir>/dest/`.

## Contributing
Contributions are welcome !
Please refer to the [CONTRIBUTING.md](https://github.com/Clovel/TimerThread/blob/master/CONTRIBUTING.md) for more information.

## Code of Conduct
A [code of conduct](https://github.com/Clovel/TimerThread/blob/master/CODE_OF_CONDUCT.md) has been established. Please do your best to comply to it.
By following the rules, we ensure that our interractions will be as civil and enjoyable as possible.


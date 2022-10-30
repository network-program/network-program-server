# network-program-server

# Build project
```
cmake -B build
cmake --build build
```

# Run
```
./build/network_server
```
or
`gcc server.c -D_REENTRANT -o server -lpthread`

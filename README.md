# network-program-server

# Build project
```
cmake -B build
cmake --build build
```

# Run
```
./build/chat_server [port] [webserver_port] [ip_address]
```

# Run test
```
cd build
ctest . --extra_verbose --output_on_failure
```

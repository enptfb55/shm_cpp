
C++ POSIX Shared Memory Containers

only tested on linux 4.4


current benchmarks:

(gcc v5.4)
    $ ./build/spsc_queue_example 
    producer: 331 ns
    consumer: 225 ns

    $ ./build/mpsc_queue_example 
    producer: 57 ns
    producer: 53 ns
    producer: 60 ns
    producer: 114 ns
    consumer: 782 ns

(clang++ 3.8)
    $ ./build/spsc_queue_example
    producer: 230 ns
    consumer: 137 ns

    $ ./build/mpsc_queue_example 
    producer: 18 ns
    producer: 16 ns
    producer: 15 ns
    producer: 15 ns
    consumer: 157 ns



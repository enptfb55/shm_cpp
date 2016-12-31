
#include <iostream>
#include <thread>

#include <errno.h>

#include <shm_cpp/segment.h>

void producer()
{
    shm_cpp::segment segment("test", 1080);
    if (!segment.create()) {
        std::cerr << "Unable to create segment : " << strerror(errno) << std::endl;
    }

    std::cout << "Successfully created shm_cpp::segment" << std::endl;
}

void consumer()
{
    shm_cpp::segment segment("test");
    if (!segment.open()) {
        std::cerr << "Unable to create segment : " << strerror(errno) << std::endl;
    }

    std::cout << "Successfully opened shm_cpp::segment" << std::endl;
}


int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;


    ::shm_unlink("test");


    std::thread producer_thr(producer);
    std::thread consumer_thr(consumer);

    producer_thr.join();
    consumer_thr.join();

    return 0;
}

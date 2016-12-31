
#include <iostream>
#include <thread>
#include <sys/time.h>

#include <errno.h>

#include <shm_cpp/spsc_queue.h>

const uint32_t MAX_MSG_SIZE = 100;


void producer()
{
    shm_cpp::spsc_queue q("spsc_test", 1080);
    if (!q.create(MAX_MSG_SIZE)) {
        std::cerr << "Unable to create queue : " << strerror(errno) << std::endl;
        return;
    }

    std::cout << "Successfully created shm_cpp::spsc_queue" << std::endl;

    // wait for consumer thread
    sleep(1);

    char buffer[MAX_MSG_SIZE] = {"hello"};
    ssize_t blen = strlen(buffer);
    struct timeval start, end;
    gettimeofday(&start, nullptr);
    for (int i = 0; i < 1000; i++) {
        while (!q.push(buffer, blen));
        //printf("push cnt %d\n", i);
    }
    gettimeofday(&end, nullptr);

    long diff = (end.tv_sec - start.tv_sec)*1000000 + (end.tv_usec - start.tv_usec);
    long lat = (diff * 1000) / 1000;
    printf("producer: %li ns\n", lat);

    sleep(1);
    q.destroy();
}

void consumer()
{
    shm_cpp::spsc_queue q("spsc_test");
    if (!q.open()) {
        std::cerr << "Unable to open queue : " << strerror(errno) << std::endl;
        return;
    }

    std::cout << "Successfully opened shm_cpp::spsc_queue" << std::endl;

    char buffer[MAX_MSG_SIZE];
    ssize_t buf_size = 0;
    // wait for the first push before starting the clock
    while (!q.pop(buffer, &buf_size));


    struct timeval start, end;
    gettimeofday(&start, nullptr);
    for (int i = 0; i < 999 ;) {
        if (q.pop(buffer, &buf_size)) {
            ++i;
            //printf("pop cnt %d\n", i);
        }
    }
    gettimeofday(&end, nullptr);
    long diff = (end.tv_sec - start.tv_sec)*1000000 + (end.tv_usec - start.tv_usec);
    long lat = (diff * 1000) / 1000;
    printf("consumer: %li ns\n", lat);
}


int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    std::thread producer_thr(producer);
    sleep(0.1);
    std::thread consumer_thr(consumer);

    producer_thr.join();
    consumer_thr.join();

    return 0;
}

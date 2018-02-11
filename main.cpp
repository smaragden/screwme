
// CPU affinity code from:
// Eli Bendersky [http://eli.thegreenplace.net]
// This code is in the public domain.
#include <algorithm>
#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>
#include <sys/sysinfo.h>
#include <unistd.h>
#include <cstring>


// Stress the cpu for a fixed anount of time.
// And at a specified percent of load.
void stress_cpu(int percent, double timeout = 1.0){
    auto start = std::chrono::high_resolution_clock::now();
    auto last_checkpoint = start;
    volatile int found_primes = 0; // Don't optimize away this.
    while(1) {

        // Break if we reached timeout
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed_seconds = end - start;
        if (elapsed_seconds.count() >= timeout) break;

        // Throttle the cpu down by adding caclulated sleeps.
        if(percent < 100){
            std::chrono::duration<double> pause = end-last_checkpoint;
            std::this_thread::sleep_for(pause*(1.0 - (double) percent / 100.0));
            last_checkpoint = end;
        }

        // Calculate the first primes in range 0-10000
        for (int i = 0; i < 10000; i++) {
            bool prime = true;
            for (int j = 2; j * j <= i; j++) {
                if (i % j == 0) {
                    prime = false;
                    break;
                }
            }
            if(prime){
                found_primes++;
            }
        }
    }
}

struct Settings{
    int threads = 1;
    int percent = 100;
    double timeout = 5.0;
    size_t memory = 0;
    unsigned long totalram = 0;
    int available_threads = std::thread::hardware_concurrency();
};

Settings parse_args(int argc, char** argv){
    Settings settings{};
    // Process arguments
    struct sysinfo si;
    sysinfo (&si);
    settings.totalram = si.totalram;

    int c;
    while ((c = getopt (argc, argv, "n:t:p:m:h")) != -1)
        switch (c)
        {
            case 'n':
                settings.threads = atoi(optarg);
                break;
            case 'p':
                settings.percent = atoi(optarg);
                break;
            case 't':
                settings.timeout = atof(optarg);
                break;
            case 'm':
                settings.memory = (size_t) atoi(optarg);
                break;
            case 'h':
                fprintf(stderr, "Usage: screwme [OPTION]...\n\n");
                fprintf(stderr, "OPTIONS:\n");
                fprintf(stderr, "  -n\t\tNumber of threads to screw. [0 - %d]\n", settings.available_threads);
                fprintf(stderr, "  -p\t\tPercent to screw the cpus [0 - 100]. (default: 100%%)\n");
                fprintf(stderr, "  -t\t\tFor how long do you want to be screwed? (seconds)\n");
                fprintf(stderr, "  -m\t\tHow much memory do you want me to screw? [0 - %lu] (Mb)\n", settings.totalram >> 20);
                fprintf(stderr, "  -h\t\tShow this help.\n");
                exit(0);
            case '?':
                if (optopt == 'n')
                    fprintf (stderr, "Option -%c requires an argument.\n", optopt);
                if (optopt == 't')
                    fprintf (stderr, "Option -%c requires an argument.\n", optopt);
                else if (isprint (optopt))
                    fprintf (stderr, "Unknown option `-%c'.\n", optopt);
                else
                    fprintf (stderr,
                             "Unknown option character `\\x%x'.\n",
                             optopt);
                exit(1);
            default:
                abort();
        }
    return settings;
}

int main(int argc, char** argv) {
    Settings settings = parse_args(argc, argv);

    std::cout << "You are getting screwed!" << std::endl;
    std::cout << "I'm going to screw " << settings.threads << " of your "<< settings.available_threads << " available threads for " << settings.timeout << " second(s) at "<< settings.percent<< "% load. \n";
    if(settings.memory>0){
        std::cout << "I'm going to screw " << settings.memory << "Mb of your "<< (settings.totalram >> 20) << "Mb total memory for " << settings.timeout << " seconds. \n";
    }

    // Screw memory
    unsigned char *result;
    auto s = (size_t) (settings.memory<<20);
    result = (unsigned char*) calloc(s, sizeof(char));

    // Screw cpu's
    int num_threads = std::min(settings.threads, settings.available_threads);
    // A mutex ensures orderly access to std::cout from multiple threads.
    std::vector<std::thread> m_threads(num_threads);
    for (unsigned i = 0; i < num_threads; ++i) {
        m_threads[i] = std::thread(stress_cpu, settings.percent, settings.timeout);

        // Pin this thread to a single CPU.
        cpu_set_t cpuset{};
        CPU_ZERO(&cpuset);
        CPU_SET(i, &cpuset);
        int rc = pthread_setaffinity_np(m_threads[i].native_handle(), sizeof(cpu_set_t), &cpuset);
        if (rc != 0) {
            std::cerr << "Error calling pthread_setaffinity_np: " << rc << "\n";
        }
    }

    for (auto& t : m_threads) {
        t.join();
    }
    return 0;
}
#include "conSet.h"
#include <unistd.h>
#include <thread> 

cuckooHash<int, std::string> testHash;
void doWork();
int n_operations;
int max_key;
struct config_t {
	int n_threads;
	int max_key;
	int n_operations;
	int init_size;

	config_t(): n_threads(2), max_key(10000), n_operations(10000), init_size(100) { } 
};

void usage(){
	std::cout << "Options: " << std::endl;
	std::cout << "-t <int> : number of threads (default: 2)" << std::endl;
	std::cout << "-k <int> : maximum value for key (default: 10000)" << std::endl;
	std::cout << "-n <int> : total number of insert + remove operations (default: 10000)" << std::endl;
	std::cout << "-z <int> : intital size for each of the tables (default: 100)" << std::endl;
	std::cout << "-h : displays command line options" << std::endl;
	exit(0);
}

void parseargs(int argc, char** argv, config_t& cfg){
	int opt;
	while ((opt = getopt(argc, argv, "t:k:n:z:h")) != -1){

		switch (opt){
			case 't': cfg.n_threads = atoi(optarg); break;
			case 'k': cfg.max_key = atoi(optarg); break;
			case 'n': cfg.n_operations = atoi(optarg); break;
			case 'z': cfg.init_size = atoi(optarg); break;
			case 'h': usage(); break;

		}
	}

}


int main(int argc, char **argv){
    config_t cfg;
    parseargs(argc, argv, cfg);
    
	int nThreads = cfg.n_threads;
    n_operations = cfg.n_operations;
    max_key = cfg.max_key;
    std::thread myThreads[nThreads];
    
	testHash.populate(cfg.init_size, max_key);
    
	auto start_time = std::chrono::high_resolution_clock::now();
     
    //spawn threads
    for (int i=0; i<nThreads; i++){
		myThreads[i] = std::thread(doWork);
	
    }

	//wait for threads to join
    for (int i=0; i<nThreads; i++){
		myThreads[i].join();
    }
    

    auto end_time = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();
    std::cout << "Main app duration: " << duration << std::endl;
    std::cout << "Size of the Cuckoo Hash is : " << testHash.size() << std::endl;
    std::cout << "number of resizes: " << testHash.numResizes << std::endl;
    testHash.getTablesSize();
    return 0;
}


void doWork(){
	auto start_time = std::chrono::high_resolution_clock::now();
	int noOperations = n_operations;
	for (int i =0; i< noOperations; i++){
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<int> dis(0, 11);
		std::uniform_real_distribution<> valDis(0, max_key);
		int prob = dis(gen);
		if (prob <= 6){
			//insert operation
			int k = valDis(gen);
			std::string v = testHash.generateRandomString(prob);
			testHash.add(k,v);
		}else {
			//remove operation
			int k = valDis(gen);
			testHash.remove(k);
		}
	}

	
	auto end_time = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();
	std::thread::id this_id = std::this_thread::get_id();
							
	std::cout<<"Thread ID: " << this_id  << std::endl;
	std::cout << "Duration: " << duration << std::endl;
	std::cout << " ============ " << std::endl;
}


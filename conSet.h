#ifndef CONSET_H
#define CONSET_H

#include <vector>
#include <mutex>
#include <atomic>
#include <shared_mutex>
#include <cmath>
#include <iostream>
#include <string>
#include <random>


template <typename Tkey, typename Tval>
class cuckooHash {
    public:
        
        cuckooHash(){
        
        }
	    void getTablesSize(){
		    std::cout << "Table 1 size is : " << table1.size() << std::endl;
		    std::cout << "Table 2 size is : " << table2.size() << std::endl;
	    }

        bool add(Tkey key, Tval value){
            //If key already exists, return false;
            std::pair<bool, int8_t> entryExists = contains(key);
            if (entryExists.first == true){
                return false;
            }

            //Otherwise, construct a new entry 
            entry toAdd;
            toAdd.key = key;
            toAdd.value = value;
            toAdd.isPresent = true;
            
            //while (resizing) {}; //Keep spinning until current resize (if any) finishes
            int LIMIT = table1.size() + table2.size();
            int hash1, hash2;
            
            for (int i = 0; i < LIMIT ; i++){
                fullTableLock.lock_shared();
                hash1 = hash(toAdd.key, 1);
                hash2 = hash(toAdd.key, 2);
                bool locksAcquired = false;
                std::mutex& m1 = locks1[hash1];
                std::mutex& m2 = locks2[hash2];
                int firstLockTrials = 0;
                int secondLockTrials = 0; 
            
                bool firstLock, secondLock;
                while (true){
		            if (m1.try_lock()){
			        break;
		            }
	            }
	        //First lock is acquired now, try to acquire the second 
	    
	        while (true){
		        if (m2.try_lock()){
			        break;
		        }
	        }
	        //Now both locks are acquired 
	        locksAcquired = true;
            if (locksAcquired){

                entry entry1 = table1.at(hash1);

                //Insert the new entry at this location
                table1.at(hash1).key = toAdd.key;
                table1.at(hash1).value = toAdd.value;
                table1.at(hash1).isPresent = true;

                //If the old entry was not NULL, then we need to move it
                //to table 2
                if (entry1.isPresent == true){
                    //hash key using table2 hash function
                    //hash2 = hash(entry1.key, 2);

                    //Keep a copy of the entry that exists at this location
                    entry entry2 = table2.at(hash2);

                    //Insert the old table1 entry into table2
                    table2.at(hash2).key = entry1.key;
                    table2.at(hash2).value = entry1.value;
                    table2.at(hash2).isPresent = true;

                    //If the old table2 was not NULL, we need to move it
                    //to table 1
                    if (entry2.isPresent == true){
                        //consider it a new entry to add and keep looping
                        toAdd = entry2;
                m2.unlock();
                m1.unlock();
                //fullTableLock.unlock_shared();		
                    }else {
                        
                        m2.unlock();
                        m1.unlock();
                        fullTableLock.unlock_shared();
                        return true;
                    }
                }else{ 
                    m2.unlock();
                    m1.unlock();
                    fullTableLock.unlock_shared();
                    return true;
                }
                }
            fullTableLock.unlock_shared();
            }

            
            //If we have reached LIMIT iterations and still did not return
            //We need to resize the whole map and re-hash all elements
            //Then try to add the last element that hasn't been added yet
            resize();

            add(toAdd.key, toAdd.value);
            
            return true;
        }
        
        bool remove(Tkey key){
            std::pair<bool, int8_t> keyExists = contains(key); 
            fullTableLock.lock_shared();   
            if (keyExists.first == true){
                if (keyExists.second == 1){
                    int hash1  = hash(key, 1);
                    std::mutex& m1 = locks1[hash1];
                    
                    while (true){
                        
                            if (m1.try_lock()){
                                break;
                            }
                        
                    }
                    entry myEntry = table1.at(hash1);
                    if (myEntry.isPresent == true){
                        table1.at(hash1).isPresent = false;
                        m1.unlock();
                        fullTableLock.unlock_shared();
                        return true;
                    }
                    m1.unlock();
                    
                }else {
                    int hash2 = hash(key, 2);
                    std::mutex& m2 = locks2[hash2];
                    while (true){
                        
                            if (m2.try_lock()){
                                break;
                            }
                        
                    }
                    entry myEntry = table2.at(hash2);
                    if (myEntry.isPresent == true){
                        table2.at(hash2).isPresent = false;
                        m2.unlock();
                        fullTableLock.unlock_shared();
                        return true;
                    }
                    m2.unlock();
                    
                }
            }
            
            fullTableLock.unlock_shared();
            return false;

        }

        std::pair<bool,int8_t> contains(Tkey key){
            int hash1 = hash(key, 1);
            entry entry1 = table1.at(hash1);
            if (entry1.key == key){
                return std::make_pair(true, 1);
            }
            int hash2 = hash(key, 2);
            entry entry2 = table2.at(hash2);
            if (entry2.key == key){
                return std::make_pair(true,2);
            }
            return std::make_pair(false,-1);
        }

        int size(){
            int totalSize = 0;
            //Iterate over both tables and see which entries 
            //are not actually null
            for (int i=0; i<table1.size(); i++){
                entry currentEntry = table1.at(i);
                if (currentEntry.isPresent == true){
                    totalSize++;
                }
            }

            for (int i=0; i<table2.size(); i++){
                entry currentEntry = table2.at(i);
                if (currentEntry.isPresent == true){
                    totalSize++;
                }
            }
            return totalSize;
        }

        void populate(int init_size, int max_key){
            table1.resize(init_size);
                table2.resize(init_size);
                locks1 = new std::mutex[table1.size()];
                locks2  = new std::mutex[table2.size()];

            int successfulInserts = 0;
                while (successfulInserts < init_size*2){
                    entry newEntry;
                    newEntry.key = generateRandomInt(0,max_key);
                    newEntry.value = generateRandomString(generateRandomInt(2,12));
                    if (add(newEntry.key, newEntry.value)){ successfulInserts++; }
                }
        }

        std::atomic<int> numResizes; 
        

        static int generateRandomInt(int min, int max){
            std::random_device dev;
            std::mt19937 rng(dev());
            std::uniform_int_distribution<std::mt19937::result_type> dist(min, max); 

            return dist(rng);
        }


        static char generateRandomChar(){
            static const char allChars[] = "0123456789"
                "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                "abcdefghijklmnopqrstuvwxyz";
            char ch;
            
            std::random_device dev;
            std::mt19937 rng(dev());
            std::uniform_int_distribution<std::mt19937::result_type> dist(0, 62);
            ch = allChars[dist(rng)];
            
            return ch;
        }

        static std::string generateRandomString(int len){
            std::string s; 
            for (int i = 0; i < len; i++){
                s += generateRandomChar();
            }
            return s;
        }
 

    private:


        struct entry {
            Tkey key;
            Tval value;
            bool isPresent;
        };
        std::vector<entry> table1;
        std::vector<entry> table2;
        
        std::mutex *locks1;
        std::mutex *locks2;
        std::shared_mutex fullTableLock;
        std::mutex fullTableLock2;
        std::atomic<bool> resizing;
        std::atomic<bool> anyFineGrainLock; 
        

        int hash(Tkey key, int funcID){
            size_t cusHash = std::hash<Tkey>()(key);
             int hash =  funcID == 1 ? cusHash % table1.size() : floor(table2.size() * ((cusHash * 0.6180339887) - floor(cusHash * 0.6180339887)));
            return hash;
        }

        bool resize(){

            fullTableLock.lock();
	        numResizes++;    
            
            int oldt1_size = table1.size();
            int oldt2_size = table2.size();
            int new_size = 2 * std::max(oldt1_size, oldt2_size);


            //Keep all entries in both tables in this vector so that
            //they're re-hashed and stored in the new positions
            std::vector<entry> oldEntries;
	    
            //Resize the first table    
            table1.resize(new_size);
            for (int i = 0; i < oldt1_size; i++){
                entry t1_entry = table1.at(i);
                //if we have a valid entry at this position, mark it as removed
                //and add the entry to the oldEntries vector
                if (t1_entry.isPresent){
                    table2.at(i).isPresent = false;
                    oldEntries.push_back(t1_entry);
                }
            }

            //Resize the second table
            table2.resize(new_size);
            for (int i = 0; i < oldt2_size; i++){
                entry t2_entry = table2.at(i);
                //if we have a valid entry at this position, mark it as removed
                //and add the entry to the oldEntries vector
                if (t2_entry.isPresent){
                    table2.at(i).isPresent = false;
                    oldEntries.push_back(t2_entry);
                }      
            }

            locks1 = new std::mutex[table1.size()];
            locks2  = new std::mutex[table2.size()];

            fullTableLock.unlock();
 
            //Now, compute the new hashes for all old elements and add them again
            for (int i = 0; i < oldEntries.size() ; i++){
                add(oldEntries.at(i).key, oldEntries.at(i).value);
            }

            return true;
        }

       
};


#endif

#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <list>
#include <map>

using namespace std;

//single cache line
struct CacheLine {
    bool available;
    unsigned long tag;
    unsigned long Counter;
    long int arrival;
};

class Cache {
    int associativity;
    int num_sets;
    int num_lines;
    int set_field;
    int tagfield_width;
    string replacement_policy; //Replacement policy
    int hits;
    int count;
    vector<vector<CacheLine>> cache;
public:
    Cache(int cache_size, int block_size, int associativity, string replacement_policy) {
        //fully associative
        if (associativity == 0) {
            this->num_lines = cache_size/block_size;
            this->associativity = num_lines;
            num_sets = 1;
            this->replacement_policy = replacement_policy;
            this->tagfield_width = 32 - 6;
        }
        //Set associative/ Direct mapped
        else {
            this->num_lines = cache_size/block_size;
            this->num_sets = floor(num_lines/associativity); // how many total sets in cache
            this->associativity = associativity; // 0,1,2, or 4
            this->replacement_policy = replacement_policy; //FIFO or LRU
            this->set_field = log2(num_sets); //how many bytes needed for set
            this->tagfield_width = 32 - 6 - set_field; //addy size - offset - set field size
        }

        hits = 0;
        count = 0;



        vector<CacheLine> temp1;

        CacheLine temp;
        temp.available = true;
        temp.Counter= 0;
        temp.tag = 0;
        temp.arrival = 0;

        for (int i = 0; i < num_sets; i++) {
            for (int j = 0; j < this->associativity; j++) {
                temp1.push_back(temp);
            }
            cache.push_back(temp1);
            temp1.clear();
        }
    }

    void access(string address) {
        bool hit = false, inserted = false;
        unsigned long Set, Tag;
        count++;

        //Direct Mapped
        if(associativity == 1){
            string binary_set = address.substr(tagfield_width, set_field);
            address = address.substr(0,tagfield_width);
            Tag = binary_to_decimal(address);
            Set = binary_to_decimal(binary_set);
        }
        //initialize Set associative cache
        else if(associativity == 2 || associativity == 4){
            string binary_set = address.substr(tagfield_width, set_field);
            address = address.substr(0,tagfield_width);
            Tag = binary_to_decimal(address);
            Set = binary_to_decimal(binary_set);
        }
        //initialize Fully associative cache
        else{
            address = address.substr(0,tagfield_width);
            Tag = binary_to_decimal(address);
            Set = 0;
        }


        for (int i = 0; i < associativity; i++) {
            if (cache[Set][i].available) {
                cache[Set][i].tag = Tag;
                cache[Set][i].arrival = count;
                cache[Set][i].available = false;
                cache[Set][i].Counter = count;
                inserted = true;
                break;
            }
            else if (cache[Set][i].tag == Tag) {
                hit = true;
                cache[Set][i].Counter = count;
                break;
            }
        }

        if(hit){
            hits++;
        }

        if(!hit && !inserted){
            if(replacement_policy == "LRU"){
                LRU(Tag, Set);
            }
            if(replacement_policy == "FIFO"){
                FIFO(Tag, Set);
            }
        }

    }

    unsigned long binary_to_decimal(string s){
        {
            string num = s;
            unsigned long dec_value = 0;
            // Initializing base value to 1, i.e 2^0
            int base = 1;

            int len = num.length();
            for (int i = len - 1; i >= 0; i--) {
                if (num[i] == '1')
                    dec_value += base;
                base = base * 2;
            }

            return dec_value;
        }
    }

    float get_hit_rate() {
        return ((float)hits / (float)count);
    }

    void FIFO(long int tag, int setNum){
        int minCount = INT_MAX;
        int replaceIdx = 0;

        for (int i = 0; i < cache[setNum].size(); i++) {
            if (cache[setNum][i].arrival < minCount) {  // Found oldest cache line
                minCount = cache[setNum][i].arrival;
                replaceIdx = i;
            }
        }

        // Replace oldest cache line
        cache[setNum][replaceIdx].tag = tag;
        cache[setNum][replaceIdx].available = false;
        cache[setNum][replaceIdx].arrival = count;
        cache[setNum][replaceIdx].Counter = count;

    }

    void LRU(long int Tag, int setNum) {
        int minCount = cache[setNum][0].Counter;
        int replaceIdx;
        for (int i = 1; i < cache[setNum].size(); i++) {
            if (cache[setNum][i].Counter < minCount) {  // Found least recently used cache line
                minCount = cache[setNum][i].Counter;
                replaceIdx = i;
            }
        }

        // Replace least recently used cache line
        cache[setNum][replaceIdx].tag = Tag;
        cache[setNum][replaceIdx].available = false;
        cache[setNum][replaceIdx].Counter = count;
        cache[setNum][replaceIdx].arrival = count;

    }

};

// retrieve hexadecimal and convert to binary\
// sourced from to GeeksforGeeks how to convert hexadecimal -> binary
string hexToBinary(string input)
{
    unsigned int x =  stoul(input, nullptr, 16) ;

    string result = bitset<32>(x).to_string();

    return result;
}

int main() {
    //---------parameters--------------------------------
    int cache_size, block_size;
    string replacement;
    cout<<"caches size: "; //do 512 for tests easy to debug
    cin>>cache_size;
    cout<<"block size: "; //can stick to one such as 64 for now
    cin>>block_size;
    cout << "replacement strategy (FIFO or LRU): ";
    cin>> replacement;

    // Create cache object
    Cache Fullcache(cache_size, block_size, 0, replacement);
    Cache DirectCache(cache_size, block_size, 1, replacement);
    Cache Cache_2(cache_size, block_size, 2, replacement);
    Cache Cache_4(cache_size, block_size, 4, replacement);

    // Read memory access pattern from file and simulate cache accesses
    unsigned long address;
    ifstream accessfile("Data1.txt");

    //make sure file is open
    if (!accessfile.is_open()){
        cout<< "file not open"<<endl;
    }
    string hexValue, BinaryValue;

    //read through file/convert to decimal/ access using cache
    while(getline(accessfile, hexValue)){
        hexValue = hexValue.substr(4, 8);
        BinaryValue = hexToBinary(hexValue);
        Fullcache.access(BinaryValue);
        DirectCache.access(BinaryValue);
        Cache_2.access(BinaryValue);
        Cache_4.access(BinaryValue);

    }
    accessfile.close();

    cout<<endl;

// Print cache hit rate
    cout<< "---------"<< replacement <<" replacement strategy -----------"<<endl;
    cout <<setw(30) << " Direct-Mapped Cache hit rate: " << DirectCache.get_hit_rate() << endl;
    cout <<setw(30)<< " 2 set Associative Cache hit rate: " << Cache_2.get_hit_rate() << endl;
    cout <<setw(30)<< " 4 set Associative Cache hit rate: " << Cache_4.get_hit_rate() << endl;
    cout <<setw(30)<< " Fully associative Cache hit rate: " << Fullcache.get_hit_rate() << endl;

    cout<<setw(30)<<"----------------------------------------------";

    return 0;
}

#include <omp.h>
#include <vector>
#include <functional>
#include <iostream>

template<typename K, typename V>
using key_value = std::pair<K, V>;

template<class T>
using vector = std::vector<T>;

template <typename K, typename V>
using bucket = vector<key_value<K, V>>;

template <typename T, typename A> struct map_argument {
    vector<T> things;
    vector<A>& results;
    std::function<A(T)> func;
    int from;
    int to;
    map_argument(vector<T>& t, vector<A>& r, std::function<A(T)> f, int start, int end)
    : things(t), results(r), func(f), from(start), to(end) {};
};

/*
Consider an example of word count, T is a file as a string, K is a word string and V is the count
The implementation of this depends on the problem we wish to solve. Maybe not templated TBH.
*/
template <typename T, typename K, typename V> vector<key_value<K, V>> do_map(const T thing){
    // for(int i = arg.from; i < arg.to; i++){
        // arg.results[i] = arg.func(arg.things[i]);
    // }
    
    return NULL;
}

/*
A local function for a worker to reduce the elements in the input vector with duplicate keys into a single Key and reduced Value,
takes the specific reduce function as a parameter
*/
template <typename K, typename V>
std::unordered_map<K, V> do_reduce(const vector<bucket<K, V>> &buckets,
                                   const V identity,
                                   std::function<key_value<K,V>(const key_value<K, V>, const key_value<K, V>)> reduce_func)
{
    std::unordered_map<K, V> reduce_map = {};
    for(const auto bucket : buckets){
        for (const auto [key, value] : bucket){
            if (reduce_map.find(key) == reduce_map.end()){
                reduce_map[key] = identity;
            }
            key_value<K, V> existing = {key, reduce_map.at(key)};
            key_value<K, V> created = {key, value};
            key_value<K, V> pair  = reduce_func(existing, created);
            reduce_map.at(key) = pair.second;
        }
    }
    return reduce_map;
}

/*
Perform the Map stage, take the source objects, and destructure it into Key Value pairs,
 where the Value has an associative Reduce operation
 and an Identity for the Reduce
 To support shuffling the Keys into the same bucket, we hash the keys and modulo over the thread size to map into a bucket
*/
template <typename T, typename K, typename V>
void concurrent_map(const vector<T>& things,
                   vector<vector<bucket<K, V>>>& map_pools,
                   std::function<uint(K)> key_hash,
                   std::function<void(const T, vector<bucket<K, V>>& , std::function<uint(K)>)> map_func,
                   const uint nthreads)
{
    // 1 vector for each thread, with 1 bucket to map into for each thread, and each bucket has many Key-Value pairs, with each Key being in exactly 1 bucket. trust me
    // We define buckets for each map operation to group keys for the reduce operation
    // Technically we should preallocate most of this but whatever??
    // Map input globs T, into vectors of Key Value pairs
    // #pragma omp parallel for num_threads(nthreads)
    for(int j = 0 ; j < things.size(); j++){
        int buckets_index = j % map_pools.size();
        // Hash to place all occurences of a key within the same bucket index
        std::cout << "input " <<  things.at(j) << std::endl;
        map_func(things.at(j), map_pools.at(buckets_index), key_hash);
    }    
    for(auto bucket : map_pools.at(0)) {

        std::cout << "Bucket of size " << bucket.size() << std::endl;
    }
}

template <typename K, typename V>
vector<std::unordered_map<K, V>> concurrent_reduce(vector<vector<bucket<K, V>>> pile_of_kv,
                                                   std::function<key_value<K,V>(const key_value<K, V>, const key_value<K, V>)> reduce_func,
                                                   const V identity,
                                                   const uint nthreads)
{
    // Each thread produces an unordered_map of unique keys with the values being the result of reducing all values with that key
    vector<std::unordered_map<K, V>> reduce_pools(pile_of_kv.size());
    // #pragma omp parallel for num_threads(nthreads)
    for(auto i = 0; i < pile_of_kv.size(); i++){
        reduce_pools.at(i) = do_reduce(pile_of_kv.at(i), identity, reduce_func);
    }
    return reduce_pools;
}

template <typename T, typename K, typename V>
std::unordered_map<K, V> map_reduce(const vector<T> &things,
                                    std::function<uint(K)> key_hash,
                                    std::function<void(T, vector<bucket<K, V>>& , std::function<uint(K)>)> map_func,
                                    std::function<key_value<K,V>(const key_value<K, V>, const key_value<K, V>)> reduce_func,
                                    const V identity,
                                    const uint nthreads)
{
    vector<vector<bucket<K, V>>> map_result(nthreads,
                                            vector<bucket<K, V>>(nthreads,
                                                                 bucket<K, V>()));
    concurrent_map(things, map_result, key_hash, map_func, nthreads);
    // Shuffle and reorder
    // Flatten the buckets created at the Map stage into a vector for each thread to reduce
    vector<vector<bucket<K, V>>> shuffled_result(nthreads,
                                                 vector<bucket<K, V>>());
    // Flatten map_result by taking all K, V in the [i]th bucket and putting it into an [i]th vector of shuffled result;
    // Due to consistent hashing at the Map stage, the [i]th bucket of every worker will have the same hash space. This groups all copies of the same key.
    // map_result contains, per_thread, per_bucket, keys
    for (auto i = 0; i < map_result.size(); i++){
        for (auto j = 0; j < shuffled_result.size(); j++){
            // For the jth thread's vec, place the ith bucket into the ith shuffled_results to group keys
            shuffled_result.at(i).push_back(map_result.at(j).at(i));
        }
    }

    // Do reduce
    vector<std::unordered_map<K, V>> reduce_result = concurrent_reduce(shuffled_result, reduce_func, identity, nthreads);
    // We expect each unordered_map to have unique keys from the other ones, this should be due to our hash function being a pure function;
    // Finalise and sort?
    std::unordered_map<K, V> results = {};
    for (const auto& item : reduce_result){
        results.insert(item.begin(), item.end());
    }

    return results;
}

void map_func_add(const int x, vector<bucket<int, int>>& buckets, std::function<uint(int)> key_hash){
    std::cout << "x is " << x << std::endl;
    std::pair<int, int> kv = {x, 1};
    int index = key_hash(kv.first % buckets.size());
    std::cout << "bucket size is " << buckets.at(index).size() << std::endl;
    buckets.at(index).push_back(kv);
    std::cout << "bucket size is now " << buckets.at(index).size() << std::endl;
}

std::pair<int, int> reduce_func_add(const std::pair<int, int>  x, const std::pair<int, int> y){
    std::pair<int, int> out = {x.first, x.second + y.second};

    return out;
}                        
                  
int main(int argv, char* argc[]){
    omp_set_dynamic(0);
    uint nthreads = 2;
    std::function<uint(int)> hash_func = [](int key){
        return (unsigned)std::hash<int>{}(key);
        };
    vector<int> things = {1 , 2, 31 , 2, 31 , 2, 31 , 2, 31 , 2, 3};
    std::function<void(int, vector<bucket<int, int>>& , std::function<uint(int)>)>  map_func = map_func_add;
    std::function<key_value<int, int>(const key_value<int, int>, const key_value<int, int>)> reduce_func = reduce_func_add;
    std::unordered_map<int, int> results = map_reduce(things, hash_func, map_func, reduce_func, 0, nthreads);
    std::cout << results.size() << std::endl;
    for(const auto [key, val] : results){
        std::cout << key << " " << val << std::endl;
    }
    return 0;
}
#include <vector>
#include <functional>


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
                                   std::function<key_value<K,V>(key_value<K, V>, key_value<K, V>)> reduce_func)
{
    std::unordered_map<K, V> reduce_map = {};
    for(const auto bucket : buckets){
        for (const auto [key, value] : bucket){
            if (!reduce_map.contains(key)){
                reduce_map.at(key) = identity;
            }
            reduce_map.at(key) = func(reduce_map.at(key), value);
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
vector<vector<vector<key_value<K, V>>>> concurrent_map(const vector<T>& things,
                                                       std::function<uint(K)> key_hash,
                                                       std::function<vector<key_value<K, V>>(const T, std::function<uint(K)>)> map_func,
                                                       const uint nthreads)
{
    // 1 vector for each thread, with 1 bucket to map into for each thread, and each bucket has many Key-Value pairs, with each Key being in exactly 1 bucket. trust me
    // We define buckets for each map operation to group keys for the reduce operation
    // Technically we should preallocate most of this but whatever??
    vector<vector<bucket<K, V>>> mapped_pools(nthreads, vector<vector<key_value<K, V>>>(nthreads));
    // Map input globs T, into vectors of Key Value pairs
    #pragma omp parallel for num_threads(nthreads)
    for(int j = 0 ; j < things.size(); j++){
        vector<bucket<K, V>> thread_buckets = mapped_pools.at(j);
        // Hash to place all occurences of a key within the same bucket index
        vector<key_value<K, V>> results = map_func(things.at(j));
        for(const auto kv : results){
            uint index = key_hash(kv.first()) % nthreads;
            thread_buckets.at(index).push_back(kv);
        }
    }    

    return mapped_pools;
}

template <typename K, typename V>
vector<std::unordered_map<K, V>> concurrent_reduce(vector<vector<bucket<K, V>>> pile_of_kv,
                                                   std::function<key_value<K,V>(const key_value<K, V>, const key_value<K, V>)> reduce_func,
                                                   const V identity,
                                                   const uint nthreads)
{
    // Each thread produces an unordered_map of unique keys with the values being the result of reducing all values with that key
    vector<std::unordered_map<K, V>> reduce_pools(pile_of_kv.size());
    #pragma omp parallel for num_threads(nthreads)
    for(auto i = 0; i < pile_of_kv.size(); i++){
        reduce_pools.at(i) = do_reduce(&pile_of_kv.at(i), identity, reduce_func);
    }
    return reduce_pools;
}

template <typename T, typename K, typename V>
std::unordered_map<K, V> map_reduce(const vector<T> &things,
                                    std::function<uint(K)> key_hash,
                                    std::function<vector<vector<key_value<K, V>>>(const T, std::function<uint(K)>)> map_func,
                                    std::function<key_value<K,V>(const key_value<K, V>, const key_value<K, V>)> reduce_func,
                                    const V identity,
                                    const uint nthreads)
{
    vector<vector<bucket<K, V>>> map_result(nthreads,
                                            vector<bucket<K, V>>(nthreads,
                                                                 bucket<K, V>()));
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
            vector<bucket<K, V>> target = shuffled_result.at(i);
            bucket<K, V> bucket_i = map_result.at(j).at(i);
            target.push_back(bucket_i);
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

int main(int argv, char* argc[]){
    vector<int> things = {1 , 2, 1};
    auto map_func = [](int x){return std::pair(x, x);};
    auto reduce_func = [](int x, int y){return x + y;};
    std::unordered_map<int, int> results = map_reduce(things, std::hash<int>, , , , )
    return 0;
}

#include "../wyt/hash.h"
#include <vector>
#include <ctime>
#include <iostream>
#include <memory>

struct Test{
    Test(int k, int v){
        key = k;
        value = v;
        is_valid = true;
    }
    int key;
    int value;
    bool is_valid;
};

const int testCount = 50000;
static std::vector<Test> s_array;
static std::unique_ptr<wyt::Hash<int, int>> s_hash(wyt::Hash<int,int>::create(32));

static void generateTestCases(){
    int i;
    srand((unsigned int)time(0));
    for(i = 0; i < testCount; ++i){
        Test tmp(rand(), rand());
        if(s_hash->insert(tmp.key, tmp.value)){
            s_array.push_back(tmp);
        }
    }
    std::cout << s_array.size() << " test cases generated" << std::endl;
}

static void testModify(){
    int number;
    auto size = s_array.size();
    for(size_t i = 0; i < size; ++i){
        number = rand() % 100;
        if(number < 30){
            s_array[i].value = rand();
            s_hash->modify(s_array[i].key, s_array[i].value);
        }
    }
}

static void testRemove(){
    int number;
    auto size = s_array.size();
    for(size_t i = 0; i < size; ++i){
        number = rand() % 100;
        if(number < 30){
            s_array[i].is_valid = false;
            s_hash->remove(s_array[i].key);
        }
    }
}

static void check(){
    bool result;
    int value;
    auto size = s_array.size();
    for(size_t i = 0; i < size; ++i){
        result = s_hash->search(s_array[i].key, value);
        assert(s_array[i].is_valid == result);
        if(result == true){
            assert(s_array[i].value == value);
        }
    }
}

int main(int argc, char* argv[]){

    generateTestCases();
    check();
    std::cout << "insert test passed" << std::endl;

    testModify();
    check();
    std::cout << "modify test passed" << std::endl;

    testRemove();
    check();
    std::cout << "remove test passed" << std::endl;
}
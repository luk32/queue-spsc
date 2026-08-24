/**
Copyright Łukasz Kucharski 2026
Single producer - single consumer FIFO queue usage example 
*/

#include <iostream>

#include "./naive.hpp"

int main() {
    NaiveQueue<int> q(10);
    q.tryPush(5);
    q.tryPush(42);
    q.tryPush(1337);


    std::cout << " ---- Example start ---- \n";

    std::cout << q.peek() << '\n';
    int e;
    q.tryPop(e);
    std::cout << e << '\n';
    q.tryPop(e);
    std::cout << e << '\n';

    std::cout << " ---- Example finish ---- \n";    
}
/**
Copyright Łukasz Kucharski 2026
Single producer - single consumer FIFO queue usage example 
*/

#include <iostream>
#include <utility>

#include "./naive.hpp"
#include "./circular_buffer.hpp"

struct  C {
    int c;
    C() {
        std::cout << "default\n";
    }
    C(const C& other) {
        std::cout << "copy\n";
    }
    C(C&& other) {
        *this = std::move(other);        
    }
    C& operator=(C&& other) {
        std::cout << "move\n";
        return *this;
    }
    C& operator=(const C& other) {
        std::cout << "copy\n";
        return *this;
    }
};

int main() {
    std::cout << " ---- Example start ---- \n";

    {
        // Basic usage example
        NaiveQueue<int> q(10);
        q.tryPush(5);
        q.tryPush(42);
        q.tryPush(1337);
        std::cout << q.peek() << '\n';
        int e;
        q.tryPop(e);
        std::cout << e << '\n';
        q.tryPop(e);
        std::cout << e << '\n';
    }

    {
        // Object life time examples
        C c;
        CirularBufferQueue<C> c_q(5);

        std::cout << " ---- copy ---- \n";
        c_q.tryPush(c);             // copies
        std::cout << " ---- def ---- \n";
        c_q.tryPush(C{});           // default + move
        std::cout << " ---- move ---- \n";
        c_q.tryPush(std::move(c));  // moves
    }

    std::cout << " ---- Example finish ---- \n";    
}
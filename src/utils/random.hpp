#pragma once

#include <iostream>
#include <random>
#include <stdexcept>


namespace balancer::utils{
  inline size_t my_random(size_t a, size_t b) {
    if (b < a){
      return my_random(b,a);
    }
    if (b!=a){
      std::random_device rd;
      std::mt19937_64 mersenne(rd());
      std::uniform_int_distribution<size_t> uid(a, b);
      size_t random = uid(mersenne);
      return random;
    }
    else return b;
  }
}


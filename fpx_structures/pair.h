#ifndef FPX_PAIR_H
#define FPX_PAIR_H

//
//  "pair.h"
//  Part of fpxlibc (https://git.goodgirl.dev/foorpyxof/fpxlibc)
//  Author: Erynn 'foorpyxof' Scholtes
//

namespace fpx {

template <typename T1, typename T2>
struct Pair {
  public:
    Pair() : Key(T1()), Value(T2()) { };
    Pair(T1 k, T2 v) : Key(k), Value(v) { };

  public:
    T1 Key;
    T2 Value;
};

}  // namespace fpx

#endif /* FPX_PAIR_H */

# dist_wrap
Convenience wrapper functions on std random library
<br><br>**Usage:**
```c++
UniDist<0, 100>(); //a bit faster non-type parameter version for integers

UniDist(0, 100);     //using std::uniform_int_distribution, return type is int
UniDist(0.f, 100.f); //using std::uniform_real_distribution, return type is float

UniDist(100); //range is -100, 100

BerDist(); //faster version of BerDist(0.5)
```

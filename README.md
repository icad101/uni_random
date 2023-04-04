# uni_random

**Convenience functions offering a seamless interface for generating random numbers at runtime and compile-time.**
<br><br>
## Usage
<br>

* *constexpr context: compile-time using template non-type parameters*
```c++
//integral:
constexpr int low_i = 0, high_i = 10;
constexpr int cx_i = uni_dist<low_i, high_i>();

//floating-point: result depends on argument
constexpr float cx_f = uni_dist<-1.f, 1.f>(); //range is [-1.f, 0.f, 1.f]
constexpr float cx_f2 = uni_dist<0.f, 1.0000001f>(); //range of 10'000'001 values
```
* *runtime context*
```c++
int rt_i = uni_dist(low_i, high_i) /*<low_i, high_i>()*/; //using std::uniform_int_distribution
float rt_f = uni_dist(static_cast<float>(low_i), static_cast<float>(high_i)); //using std::uniform_real_distribution
int cx_i = uni_dist<low_i, high_i, force_cx_t::yes>(); //force use of compile-time random (like in constexpr context)
```
* *template type parameters*
```c++
/*constexpr*/ int cx_rt_t_i = uni_dist<int>(); //full range: [<int>::lowest(), <int>::max()]
/*constexpr*/ float cx_rt_t_f = uni_dist<float>(); //range limited to int32_t
```
* *also supports following types*
```c++
std::chrono::milliseconds rt_ms = uni_dist(0ms, 10ms);
/*constexpr*/ std::chrono::milliseconds cx_rt_ms = uni_dist<std::chrono::milliseconds>();
/*constexpr*/ bool cx_rt_b = uni_dist<false, true>() /*(false, true)*/;
/*constexpr*/ char cx_rt_c = uni_dist<'a', 'z'>() /*('a', 'z')*/;
//as well as char8_t, char16_t, char32_t, wchar_t
```
* *container overloads*
```c++
using std::array;
//use containers (of size 2) as bounds
constexpr int tpl_cx_cont = uni_dist<array{low_i, high_i}>(); 
int rt_cont = uni_dist(array{low_i, high_i}); //also supports vector/pair/tuple

/*constexpr*/ int cx_ao = uni_dist_any_of(array{2, 4, 6, 8}); //get random element of container
```

<br>**Compiles on visual studio 17.6 + clang-cl (/std:c++latest) and GCC 11 (-std=c++2b).**

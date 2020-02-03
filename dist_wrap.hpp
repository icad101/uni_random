#include <random>
#include "pcg_random.hpp" //for pcg32

///----------------
template <typename T>
inline auto DistImpl( T&& dist ) noexcept {
    static pcg32 rng{ pcg_extras::seed_seq_from<std::random_device>{} };
    return std::forward<T>(dist)(rng);
}

//----------------
template <auto bound0, decltype(bound0) bound1 = std::numeric_limits<decltype(bound0)>::max()>
    requires bound0 < bound1
inline auto UniDist() noexcept {
    static std::uniform_int_distribution uid{bound0, bound1};
    return DistImpl(uid);
}
template <typename T>
inline auto UniDist( T const bound0, T const bound1 ) noexcept {
    return DistImpl( std::conditional_t<
        std::is_integral_v<T>, std::uniform_int_distribution<T>, std::uniform_real_distribution<T> >{
            std::min(bound0, bound1), std::max(bound0, bound1)} );
}
template <typename T> requires std::is_signed_v<T>
inline auto UniDist( T const bound ) noexcept { return UniDist(-bound, bound); }

//----------------
template <typename T> requires std::is_floating_point_v<T>
inline bool BerDist( T const prm ) noexcept {
    return DistImpl( std::bernoulli_distribution{ std::clamp(prm, 0.0, 1.0) } );
}
inline bool BerDist() noexcept { return UniDist<std::uint8_t{0}, std::uint8_t{1}>(); }

//----------------
template <typename T> requires std::is_floating_point_v<T>
inline auto ExpDist( T const prm ) noexcept { return DistImpl( std::exponential_distribution{prm} ); }

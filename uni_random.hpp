#pragma once

#include <random>
#include <array>
#include <pcg_random.hpp>


using counter_id_t = std::uint64_t;
inline constexpr counter_id_t const counter_id_base = static_cast<counter_id_t>(0);

template <counter_id_t id = counter_id_base>
struct counter final
{
    struct save_id final { friend consteval bool is_defined(counter) noexcept { return true; } };
    friend consteval bool is_defined(counter) noexcept;

    template <typename tag = counter, bool = is_defined(tag{})>
    [[nodiscard]] static consteval bool exists(counter_id_t) noexcept { return true; }

    [[nodiscard]] static consteval bool exists(...) noexcept { return save_id{}, false; }
};

template <counter_id_t id = counter_id_base, std::regular_invocable auto lda = [](){}>
[[nodiscard]] consteval counter_id_t unique_id() noexcept
{
    if constexpr (counter<id>::exists(id))
        return unique_id<id + static_cast<counter_id_t>(1), lda>();
    else
        return id;
}

struct cx_rand_t final
{
    using value_type = counter_id_t;

    static constexpr value_type const seed = [] () consteval noexcept
	{
        value_type shifted = 0;
        for (char const chr : __DATE__ __TIME__) {
            shifted <<= 8;
            shifted |= static_cast<value_type>(chr);
        }
        return shifted;
    }();


    value_type state = static_cast<value_type>(0);

    [[nodiscard]] constexpr value_type operator()(value_type const inc = static_cast<value_type>(1)) noexcept
    {
        constexpr std::array<value_type, 3> const bit_noises{1'759'714'724, 3'039'394'381, 458'671'337};

        state += inc;
        value_type mangled_bits = state;
        mangled_bits *= bit_noises[0];
        mangled_bits += seed;
        mangled_bits ^= (mangled_bits >> 8);
        mangled_bits += bit_noises[1];
        mangled_bits ^= (mangled_bits << 8);
        mangled_bits *= bit_noises[2];
        mangled_bits ^= (mangled_bits >> 8);
        return mangled_bits;
    }
};


template <typename T, typename... Ts>
concept same_as_any = (std::same_as<T, Ts> or...);

template <typename T> concept integral_floating_point = std::integral<T> or std::floating_point<T>;

namespace chrono {
    template <typename T> concept duration_type = requires {
        typename T::rep; typename T::period;
        static_cast<T>(static_cast<typename T::rep>(0)).count();
    };
}
template <typename T> concept integral_floating_point_duration = integral_floating_point<T> or chrono::duration_type<T>;


//--------------
template <typename dist_t>
	requires requires { std::declval<dist_t>()(std::declval<pcg32>); }
[[nodiscard]] inline auto dist_impl( dist_t&& dist ) noexcept {
    static pcg32 rng{ pcg_extras::seed_seq_from<std::random_device>{} };
    return std::forward<dist_t>(dist)(rng);
}

template <integral_floating_point_duration bounds_t>
[[nodiscard]] inline bounds_t uni_dist( bounds_t const bound0, bounds_t const bound1 ) noexcept
{
    if constexpr (std::integral<bounds_t>)
	{
		using to_uid_t = std::conditional_t<
            same_as_any<bounds_t, short, int, long, long long, unsigned short, unsigned int, unsigned long, unsigned long long>,
            bounds_t,
            std::conditional_t<std::unsigned_integral<bounds_t>, unsigned, int>>;

        auto const [bound_low, bound_high] = std::minmax(bound0, bound1);
        return static_cast<bounds_t>(dist_impl(std::uniform_int_distribution{
            static_cast<to_uid_t>(bound_low), static_cast<to_uid_t>(bound_high)}));
	}
	else if constexpr (std::floating_point<bounds_t>) {
        auto const [bound_low, bound_high] = std::minmax(bound0, bound1);
		return static_cast<bounds_t>(dist_impl(std::uniform_real_distribution{bound_low, bound_high}));
	}
    else if constexpr (chrono::duration_type<bounds_t>) {
        return static_cast<bounds_t>(uni_dist(bound0.count(), bound1.count()));
    }
}

template <integral_floating_point auto bound_low,
    std::same_as<std::remove_const_t<decltype(bound_low)>> auto bound_high,
	counter_id_t id = unique_id<counter_id_base, [](){}>(),
	typename bounds_t = std::remove_const_t<decltype(bound_low)>>
    requires (bound_low < bound_high)
[[nodiscard]] consteval auto uni_dist() noexcept
{
    using rand_val_t = cx_rand_t::value_type;
    static_assert(std::unsigned_integral<rand_val_t>);

    if constexpr (std::integral<bounds_t>)
    {
        constexpr rand_val_t const bound_low_rvt  = static_cast<rand_val_t>(bound_low);
        constexpr rand_val_t const bound_high_rvt = static_cast<rand_val_t>(bound_high);

        if constexpr (std::unsigned_integral<bounds_t>)
        {
            constexpr rand_val_t const rand_val = cx_rand_t{}(id);

            if constexpr (rand_val >= bound_low_rvt
                      and rand_val <= bound_high_rvt)
                return static_cast<bounds_t>(rand_val);
            else
                return static_cast<bounds_t>(bound_low_rvt + rand_val % (bound_high_rvt - bound_low_rvt + static_cast<rand_val_t>(1)));
        }
        else if constexpr (std::signed_integral<bounds_t>)
        {
            if constexpr (bound_low >= static_cast<bounds_t>(0)) {
                return static_cast<bounds_t>(uni_dist<bound_low_rvt, bound_high_rvt, id>());
            }
            else
            {
                constexpr rand_val_t const new_bound_low_rvt = [] () consteval noexcept {
                    if constexpr (bound_low == std::numeric_limits<bounds_t>::lowest())
                        return static_cast<rand_val_t>(std::numeric_limits<bounds_t>::max()) + static_cast<rand_val_t>(1);
                    else
                        return static_cast<rand_val_t>(-bound_low);
                }();

				if constexpr (bound_high >= static_cast<bounds_t>(0))
					return static_cast<bounds_t>(uni_dist<static_cast<rand_val_t>(0), new_bound_low_rvt + bound_high_rvt, id>());
				else
					return static_cast<bounds_t>(uni_dist<static_cast<rand_val_t>(0), new_bound_low_rvt - static_cast<rand_val_t>(-bound_high), id>() - new_bound_low_rvt);
            }
        }
    }
    else if constexpr (std::floating_point<bounds_t>)
    {
        using fp_ul_t = long double;
        using fp_ud_t = std::int64_t;

        static_assert( bound_low >= static_cast<bounds_t>(std::numeric_limits<fp_ud_t>::lowest()) );
        static_assert( bound_high <= static_cast<bounds_t>(std::numeric_limits<fp_ud_t>::max()) );

        constexpr auto const via = [] () consteval noexcept
        {
            struct bound_i_t final {
                fp_ul_t bound;
                size_t i = 0;
            };
            std::array<bound_i_t, 2> ret{{{static_cast<fp_ul_t>(bound_low)}, {static_cast<fp_ul_t>(bound_high)}}};

            constexpr auto const cx_fabs = [] (fp_ul_t const prm) consteval noexcept {
                return prm >= static_cast<fp_ul_t>(0) ? prm : -prm;
            };
            for (auto& [bound, i] : ret) {
                while (cx_fabs(bound) > cx_fabs(static_cast<fp_ul_t>(static_cast<fp_ud_t>(bound)))) {
                    bound *= static_cast<fp_ul_t>(10);
                    if (bound < static_cast<fp_ul_t>(std::numeric_limits<fp_ud_t>::lowest()) / static_cast<fp_ul_t>(10)
                     or bound > static_cast<fp_ul_t>(std::numeric_limits<fp_ud_t>::max()) / static_cast<fp_ul_t>(10))
                        break;
                    ++i;
                }
            }

            for (size_t i_01 = 0; i_01 < std::size(ret); ++i_01) {
                auto& arr_i = (ret)[i_01];
                while (arr_i.i < (ret)[(i_01 + 1) % std::size(ret)].i) {
                    arr_i.bound *= static_cast<fp_ul_t>(10);
                    ++arr_i.i;
                }
            }
            return ret;
		}();

		constexpr auto const cx_pow_10 = [] (size_t const exp) consteval noexcept {
			size_t ret = 1;
			for (size_t _ = 0; _ < exp; ++_)
				ret *= 10;
			return ret;
		};
        return static_cast<bounds_t>(uni_dist<static_cast<fp_ud_t>(via.front().bound),
                                              static_cast<fp_ud_t>(via.back().bound), id>())
             / static_cast<bounds_t>(cx_pow_10(static_cast<size_t>(via.front().i)));
    }
}

template <integral_floating_point_duration range_t>
[[nodiscard]] constexpr std::remove_const_t<range_t> uni_dist() noexcept
{
    static_assert( not std::is_reference_v<range_t> );
    static_assert( not std::is_pointer_v<range_t> );

    using nl_t = std::numeric_limits<range_t>;

	if constexpr (std::integral<range_t>)
	{
		if (std::is_constant_evaluated())
            return uni_dist<nl_t::lowest(), nl_t::max()>();
        else
			return uni_dist(nl_t::lowest(), nl_t::max());
    }
    else if constexpr (std::floating_point<range_t>)
	{
		if (std::is_constant_evaluated()) {
            using nl_alt_t = std::numeric_limits<int32_t>;
			return uni_dist<static_cast<range_t>(nl_alt_t::lowest()), static_cast<range_t>(nl_alt_t::max())>();
		} else
			return uni_dist(nl_t::lowest(), nl_t::max());
    }
    else if constexpr (chrono::duration_type<range_t>) {
        return static_cast<range_t>(uni_dist<typename range_t::rep>());
    }
}

template <std::array bounds>
    requires (std::size(bounds) == 2)
[[nodiscard]] consteval typename std::remove_const_t<decltype(bounds)>::value_type uni_dist() noexcept {
    return uni_dist<bounds.front(), bounds.back()>();
}

template <typename cont_t>
concept tuple_like = requires {
	std::get<0>(std::declval<cont_t>());
	std::tuple_size_v<cont_t>;
	typename std::tuple_element<0, cont_t>;
};
template <typename cont_t>
concept vector_like = requires {
	std::declval<cont_t>()[0];
	std::declval<cont_t>().front();
	std::declval<cont_t>().back();
	std::size(std::declval<cont_t>());
};

template <typename cont_t> concept tuple_vector_like = tuple_like<cont_t> or vector_like<cont_t>;

template <tuple_vector_like bounds_t>
[[nodiscard]] inline typename bounds_t::value_type uni_dist( bounds_t const bounds ) noexcept
{
    if constexpr (tuple_like<bounds_t>) {
        static_assert(std::tuple_size_v<bounds_t> == 2);
        return uni_dist(std::get<0>(bounds), std::get<1>(bounds));
    }
    else if constexpr (vector_like<bounds_t>) {
        assert(std::size(bounds) == 2);
        return uni_dist(bounds.front(), bounds.back());
    }
}

template <tuple_vector_like cont_t>
[[nodiscard]] constexpr decltype(auto) uni_dist_any_of( cont_t& cont ) noexcept
{
    if constexpr (tuple_like<cont_t>)
    {
        if (std::is_constant_evaluated()) {
			static_assert(std::tuple_size_v<cont_t> > 1);
            return std::get<uni_dist<static_cast<size_t>(0), std::tuple_size_v<cont_t> - 1>()>(cont);
        } else {
			assert(std::size(cont) > 1);
			return cont[uni_dist(static_cast<size_t>(0), std::size(cont) - 1)];
        }
    }
    else if constexpr (vector_like<cont_t>) {
        assert(std::size(cont) > 1);
        return cont[uni_dist(static_cast<size_t>(0), std::size(cont) - 1)];
    }
}

//--------------
template <std::floating_point prob_t>
[[nodiscard]] inline bool ber_dist( prob_t const prob ) noexcept {
    assert( prob > static_cast<prob_t>(0) and prob < static_cast<prob_t>(1) );
    return dist_impl(std::bernoulli_distribution{static_cast<double>(prob)});
}

template <std::floating_point auto prob,
    counter_id_t id = unique_id<counter_id_base, [](){}>(),
    typename prob_t = std::remove_const_t<decltype(prob)>,
    prob_t prob_1 = static_cast<prob_t>(1)>
	requires (prob > static_cast<prob_t>(0)
		  and prob < prob_1)
[[nodiscard]] constexpr bool ber_dist() noexcept
{
    if (std::is_constant_evaluated())
    {
        constexpr prob_t const prob_05 = static_cast<prob_t>(0.5);
        constexpr auto const nnot = [] (std::uint64_t const res_prob) consteval noexcept {
            return not ((prob >= prob_05) xor static_cast<bool>(res_prob));
        };
        return nnot(uni_dist<static_cast<std::uint64_t>(0), static_cast<std::uint64_t>(prob_1 / (prob >= prob_05 ? prob_1 - prob : prob)) - 1>());
    }
    else {
		return ber_dist(prob);
	}
}

//--------------
template <std::floating_point prob_t>
[[nodiscard]] inline int rand_sign( prob_t const prob ) noexcept {
    return static_cast<int>(ber_dist(prob)) * 2 - 1;
}
template <std::floating_point auto prob,
    counter_id_t id = unique_id<counter_id_base, [](){}>()>
[[nodiscard]] constexpr int rand_sign() noexcept
{
    if (std::is_constant_evaluated())
        return static_cast<int>(ber_dist<prob, id>()) * 2 - 1;
    else
		return rand_sign(prob);
}

//--------------
template <typename Itr, std::invocable F>
constexpr void dist_gen( Itr const first, Itr const last, F&& dist ) noexcept {
    std::generate(first, last, std::forward<F>(dist));
}

template <typename Itr, typename bounds_t, std::invocable<bounds_t, bounds_t> F>
inline void dist_gen( Itr const first, Itr const last, F&& dist, bounds_t const bound0, bounds_t const bound1 ) noexcept {
    std::generate(first, last, [dist = std::forward<F>(dist), bound0, bound1] () noexcept { return dist(bound0, bound1); });
}

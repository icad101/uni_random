
#include "generators/uni_random.hpp"

#include <tuple>

namespace
{
using std::same_as;
using std::tuple;

template <typename, typename>
struct concat_two_tuples;

template <typename... ts0, typename... ts1>
struct concat_two_tuples<tuple<ts0...>, tuple<ts1...>> final {
    using type = tuple<ts0..., ts1...>;
};
template <typename... tup_ts> requires (sizeof...(tup_ts) == 2)
using concat_two_tuples_t = typename concat_two_tuples<tup_ts...>::type;

template <typename... tup_ts> requires (sizeof...(tup_ts) > 1)
using concat_tuples_t = decltype(std::tuple_cat(std::declval<tup_ts>()...));


//-----------
template <typename T, typename...>
struct unique_elements { using type = T; };

template <typename... Ts, typename U, typename... Us>
struct unique_elements<tuple<Ts...>, U, Us...>
    : std::conditional_t<(std::is_same_v<U, Ts> or ...)
        , unique_elements<tuple<Ts...>, Us...>
        , unique_elements<tuple<Ts..., U>, Us...>> {};

template <typename... Ts> requires (sizeof...(Ts) != 0)
using unique_elements_t = typename unique_elements<tuple<>, Ts...>::type;


template <typename>
struct unique_tuple_elements;

template <typename... Ts>
struct unique_tuple_elements<tuple<Ts...>> final {
    using type = unique_elements_t<Ts...>;
};
template <typename tup_t> using unique_tuple_elements_t = typename unique_tuple_elements<tup_t>::type;

template <typename... tup_ts> requires (sizeof...(tup_ts) > 1)
using unique_tuples_elements_t = typename unique_tuple_elements<concat_tuples_t<tup_ts...>>::type;

//-----------
//fw: fixed width
using tup_uint_nfw_t = tuple<unsigned short, unsigned int, unsigned long, unsigned long long>;
using tup_sint_nfw_t = tuple<short, int, long, long long>;

using tup_uint_fw_t = tuple<std::uint8_t, std::uint16_t, std::uint32_t, std::uint64_t>;
using tup_sint_fw_t = tuple<std::int8_t, std::int16_t, std::int32_t, std::int64_t>;

using tup_uint_t = unique_tuples_elements_t<tup_uint_fw_t, tup_uint_nfw_t>;
using tup_sint_t = unique_tuples_elements_t<tup_sint_fw_t, tup_sint_nfw_t>;
using tup_int_t = unique_tuples_elements_t<tup_uint_t, tup_sint_t>;

using tup_chars_t = tuple<char, signed char, unsigned char, char8_t, char16_t, char32_t, wchar_t>;
using tup_chars_notint_t = tuple<char, char8_t, char16_t, char32_t, wchar_t>;

using tup_fp_t = tuple<float, double, long double>;


using tup_all_ul_t = concat_tuples_t<
    tuple<bool>,
    tup_chars_notint_t,
    tup_uint_t,
    tup_sint_t
#if not defined (__clang__)
    , tup_fp_t
#endif
>;
}

//-------------
#include <string_view>

#define USE_PRINT
#ifdef USE_PRINT
	#include <string>

	#if defined (__clang__) or defined (_MSC_VER)
		#include <format>
		using std::format;
		using std::vformat;
		using std::make_format_args;
	#elif defined (__GNUC__)
		#include <fmt/core.h>
		#include <fmt/format.h>
		using fmt::format;
		using fmt::vformat;
		using fmt::make_format_args;
	#endif

	//#define USE_PUTS
	#ifdef USE_PUTS
		#include <stdio.h>
		inline void printer(std::string const& str) noexcept { std::puts(str.c_str()); }
	#else
		#include <iostream>
		inline void printer(std::string const& str) noexcept { std::cout << str; }
	#endif
#endif

template <typename... Ts>
inline void print(std::string_view const fmt_str, Ts const&... prms) noexcept {
#ifdef USE_PRINT
	printer(vformat(fmt_str, make_format_args(prms...)));
#endif
}

//-------------
[[nodiscard]] consteval auto operator""_uz(unsigned long long const val) noexcept { return static_cast<size_t>(val); }


using tup_all_t = tup_all_ul_t;

template <typename tup_t>
struct tuple_element_bind final {
    template <size_t I> using type = std::tuple_element_t<I, tup_t>;
};


template <typename, typename, typename>
struct same_as_any_in_tuple;

template <typename T, typename tup_t, size_t... Is>
struct same_as_any_in_tuple<T, tup_t, std::index_sequence<Is...>> final {
    static constexpr bool const value = (same_as<T, std::tuple_element_t<Is, tup_t>> or...);
};
template <typename T, typename tup_t>
concept same_as_any_in_tuple_v = same_as_any_in_tuple<T, tup_t, std::make_index_sequence<std::tuple_size_v<tup_t>>>::value;

template <typename T, typename... tups_t>
concept same_as_any_in_tuples_v = (same_as_any_in_tuple_v<T, tups_t> or ...);


int main()
{
[] <size_t... Is> (std::index_sequence<Is...>) consteval noexcept {
    using teb = tuple_element_bind<tup_all_t>;
    static_assert( (same_as<decltype(uni_dist<teb::type<Is>>()), teb::type<Is>> and ...) );
    static_assert( (same_as<decltype(uni_dist<teb::type<Is> const>()), teb::type<Is>> and ...) );
}( std::make_index_sequence<std::tuple_size_v<tup_all_t>>{} );


constexpr auto const get_tup_of_tup_of_arrs = [] <typename tup_all_t> () constexpr noexcept
{
    return [] <size_t... tup_ids> (std::index_sequence<tup_ids...>) constexpr noexcept
    {
        constexpr auto const get_tup_of_arrs = [] <size_t tup_idx> () constexpr noexcept
        {
			constexpr auto const sample_count = 10_uz;

            return [] <size_t... sc_ids> (std::index_sequence<sc_ids...>) constexpr noexcept
            {
				using std::array;
				using te_t = std::tuple_element_t<tup_idx, tup_all_t>;

				#if defined (__clang__) or defined (__GNUC__)
					constexpr
				#endif
				auto const to_ud = [] <te_t prm0, te_t prm1, size_t I> () constexpr noexcept {
                    if (std::is_constant_evaluated())
                        return uni_dist<prm0, prm1, static_cast<counter_id_t>(I)>();
                    else
                        return uni_dist(prm0, prm1);
				};

				constexpr auto const t_0 = static_cast<te_t>(0);
				constexpr auto const t_1 = static_cast<te_t>(1);

                if constexpr (same_as<te_t, bool>) {
                    return tuple{ array{to_ud.template operator()<t_0, t_1, sc_ids>()...} };
                }
                else
                {
                    using nl_t = std::conditional_t<std::integral<te_t>, te_t, int32_t>;
                    using nl = std::numeric_limits<nl_t>;

					constexpr auto const nl_te = [] (nl_t(*const mptr)() noexcept) consteval noexcept { return static_cast<te_t>(mptr()); };

					constexpr auto const max_m1_lowest_p1 =
                    [] <nl_t(*const mptr)() noexcept> () constexpr noexcept
                    {
                        if constexpr (same_as<te_t, bool>) {
                            return false;
                        }
                        else {
                            return static_cast<te_t>(std::conditional_t<
                                mptr() == nl::lowest(), std::plus<te_t>, std::minus<te_t>
                            >{}(mptr(), static_cast<te_t>(1 * std::integral<te_t> + 65 * std::floating_point<te_t>)));
                        }
                    };

                    return std::tuple_cat(
                    #if defined (__clang__) or defined (__GNUC__)
						[to_ud, max_m1_lowest_p1, nl_te]
                    #elif defined (_MSC_VER)
                        [to_ud]
                    #endif
                        () constexpr noexcept {
                            return tuple{
                                array{to_ud.template operator()<t_0, t_1, sc_ids>()...},
                                array{to_ud.template operator()<max_m1_lowest_p1.template operator()<nl::max>(), nl_te(nl::max), sc_ids>()...},
                                array{to_ud.template operator()<t_0, nl_te(nl::max), sc_ids>()...}
                            };
                        }(),
                    #if defined (__clang__) or defined (__GNUC__)
						[to_ud, max_m1_lowest_p1, nl_te]
                    #elif defined (_MSC_VER)
                        [to_ud]
                    #endif
                        () constexpr noexcept {
                            if constexpr (std::is_signed_v<te_t>) {
                                return tuple{
                                    array{to_ud.template operator()<nl_te(nl::lowest), nl_te(nl::max), sc_ids>()...},
                                    array{to_ud.template operator()<static_cast<te_t>(-1), t_0, sc_ids>()...},
                                    array{to_ud.template operator()<nl_te(nl::lowest), max_m1_lowest_p1.template operator()<nl::lowest>(), sc_ids>()...}
                                };
                            }
                            else
                                return tuple{};
                        }()
                    );
                }
            }( std::make_index_sequence<sample_count>{} );
        };
        return tuple{get_tup_of_arrs.template operator()<tup_ids>()...};
    }( std::make_index_sequence<std::tuple_size_v<tup_all_t>>{} );
};
static constexpr auto const cx_tup_of_tup_of_arrs = get_tup_of_tup_of_arrs.operator()<tup_all_t>();
auto const rt_tup_of_tup_of_arrs = get_tup_of_tup_of_arrs.operator()<tup_all_t>();


static constexpr auto const get_tup_of_tup_of_arrs_bd = [] <typename tup_fp_t> () constexpr noexcept
{
    return [] <size_t... tup_ids> (std::index_sequence<tup_ids...>) constexpr noexcept
    {
        constexpr auto const get_tup_of_arrs = [] <size_t tup_idx> () constexpr noexcept
        {
			constexpr auto const sample_count = 10_uz;

            return [] <size_t... sc_ids> (std::index_sequence<sc_ids...>) constexpr noexcept
            {
				using std::array;
				using te_t = std::tuple_element_t<tup_idx, tup_fp_t>;

				constexpr auto const to_bd = []
#if defined (__clang__)
					<size_t I> (std::floating_point auto prob)
#else
					<std::floating_point auto prob, size_t I> ()
#endif
					constexpr noexcept {
#if not defined (__clang__)
                    if (std::is_constant_evaluated())
                        return ber_dist<prob, static_cast<counter_id_t>(I)>();
                    else
#endif
                        return ber_dist(prob);
				};

				return tuple{
#if defined (__clang__)
					array{to_bd.template operator()<sc_ids>(static_cast<te_t>(0.1))...},
                    array{to_bd.template operator()<sc_ids>(static_cast<te_t>(0.5))...},
                    array{to_bd.template operator()<sc_ids>(static_cast<te_t>(0.9))...}
#else
                    array{to_bd.template operator()<static_cast<te_t>(0.1), sc_ids>()...},
                    array{to_bd.template operator()<static_cast<te_t>(0.5), sc_ids>()...},
                    array{to_bd.template operator()<static_cast<te_t>(0.9), sc_ids>()...}
#endif
				};

            }( std::make_index_sequence<sample_count>{} );
        };
        return tuple{get_tup_of_arrs.template operator()<tup_ids>()...};
    }( std::make_index_sequence<std::tuple_size_v<tup_fp_t>>{} );
};
#if not defined (__clang__)
static constexpr auto const cx_tup_of_tup_of_arrs_bd = get_tup_of_tup_of_arrs_bd.operator()<tup_fp_t>();
#endif
auto const rt_tup_of_tup_of_arrs_bd = get_tup_of_tup_of_arrs_bd.operator()<tup_fp_t>();


static constexpr auto const get_tup_of_tup_of_arrs_rs = [] <typename tup_fp_t> () constexpr noexcept
{
    return [] <size_t... tup_ids> (std::index_sequence<tup_ids...>) constexpr noexcept
    {
        constexpr auto const get_tup_of_arrs = [] <size_t tup_idx> () constexpr noexcept
        {
			constexpr auto const sample_count = 10_uz;

            return [] <size_t... sc_ids> (std::index_sequence<sc_ids...>) constexpr noexcept
            {
				using std::array;
				using te_t = std::tuple_element_t<tup_idx, tup_fp_t>;

				constexpr auto const to_rs = []
#if defined (__clang__)
					<size_t I> (std::floating_point auto prob)
#else
					<std::floating_point auto prob, size_t I> ()
#endif
					constexpr noexcept {
#if not defined (__clang__)
                    if (std::is_constant_evaluated())
                        return rand_sign<prob, static_cast<counter_id_t>(I)>();
                    else
#endif
                        return rand_sign(prob);
				};

				return tuple{
#if defined (__clang__)
                    array{to_rs.template operator()<sc_ids>(static_cast<te_t>(0.1))...},
                    array{to_rs.template operator()<sc_ids>(static_cast<te_t>(0.5))...},
                    array{to_rs.template operator()<sc_ids>(static_cast<te_t>(0.9))...}
#else
                    array{to_rs.template operator()<static_cast<te_t>(0.1), sc_ids>()...},
                    array{to_rs.template operator()<static_cast<te_t>(0.5), sc_ids>()...},
                    array{to_rs.template operator()<static_cast<te_t>(0.9), sc_ids>()...}
#endif
				};

            }( std::make_index_sequence<sample_count>{} );
        };
        return tuple{get_tup_of_arrs.template operator()<tup_ids>()...};
    }( std::make_index_sequence<std::tuple_size_v<tup_fp_t>>{} );
};
#if not defined (__clang__)
static constexpr auto const cx_tup_of_tup_of_arrs_rs = get_tup_of_tup_of_arrs_rs.operator()<tup_fp_t>();
#endif
auto const rt_tup_of_tup_of_arrs_rs = get_tup_of_tup_of_arrs_rs.operator()<tup_fp_t>();


static constexpr auto const print_tuples = [] <typename... tup_ts> (tuple<tup_ts...> const& tup_of_tup_of_arrs) noexcept
{
    [&tup_of_tup_of_arrs] <size_t... totoa_ids> (std::index_sequence<totoa_ids...>) noexcept
    {
        constexpr auto const toa_printer = [] <typename... arr_ts> (tuple<arr_ts...> const& tup_of_arrs) noexcept
        {
            [&tup_of_arrs] <size_t... toa_ids> (std::index_sequence<toa_ids...>) noexcept
            {
                constexpr auto const array_printer = [] <typename arr_vt, size_t arr_size> (std::array<arr_vt, arr_size> const& arr) noexcept
                {
                    [&arr] <size_t... Is> (std::index_sequence<Is...>) noexcept
                    {
                        [[maybe_unused]] constexpr auto const print_val = [] (arr_vt const arr_val) noexcept {
                            if constexpr (same_as_any_in_tuples_v<arr_vt, tup_chars_t>)
                                return static_cast<char>(arr_val);
                            else
                                return arr_val;
                        };
                        (print("{} | ", print_val(arr[Is])), ...);

                        constexpr std::string_view const type_pref = std::integral<arr_vt>
                            ? (std::unsigned_integral<arr_vt> ? "u" : "i")
                            : (same_as<arr_vt, float> ? "f" : (same_as<arr_vt, double> ? "d" : "ld"));
                        print("{} {}", type_pref, sizeof(arr_vt) * 8u);
                    }( std::make_index_sequence<arr_size>{} );

                    bool const same_diff = std::all_of(std::cbegin(arr), std::cend(arr),
                        [arr_front = arr.front()] (arr_vt const prm) noexcept {
                            return arr_front == prm;
                        });
                    print("\t{}\n", same_diff ? "same" : "different");

                };
                (array_printer(std::get<toa_ids>(tup_of_arrs)), ...);
                print("\n");
            }( std::index_sequence_for<arr_ts...>{} );
        };
        (toa_printer(std::get<totoa_ids>(tup_of_tup_of_arrs)), ...);
        print("\n");
    }( std::index_sequence_for<tup_ts...>{} );
    print("\n");
};
#if not defined (__clang__)
print_tuples( cx_tup_of_tup_of_arrs_rs );
#endif
print_tuples( rt_tup_of_tup_of_arrs_rs );


///-----------
constexpr auto cx_arr = [] () consteval noexcept {
    std::array<size_t, 3_uz> arr;
    dist_gen(std::begin(arr), std::end(arr), uni_dist<0_uz, 10_uz>);
    return arr;
}();
print("cx_arr: {}, {}, {}\n", cx_arr[0], cx_arr[1], cx_arr[2]);

std::vector<size_t> vec(3_uz);
dist_gen(std::begin(vec), std::end(vec), static_cast<size_t(*)(size_t, size_t)>(uni_dist), 0_uz, 10_uz);
print("vec: {}, {}, {}\n", vec[0], vec[1], vec[2]);
}

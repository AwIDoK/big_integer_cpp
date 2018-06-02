#include "big_integer.h"
#include <string>
#include <algorithm>
#include <vector>
#include <stdexcept>

big_integer::big_integer() : big_integer(0) {}

big_integer::big_integer(int value) {
    number.push_back(static_cast<uint32_t>(value));
    number.push_back(value >= 0 ? 0 : 0xFFFFFFFF);
    normalize();
}

big_integer::big_integer(std::string const &value) : big_integer() {
    if (value.empty()) {
        return;
    }
    if (value[0] != '-' && !isdigit(value[0])) {
        throw std::runtime_error("Unknown symbol");
    }
    size_t start_pos = 0;
    bool sign;
    if (value[0] == '-') {
        start_pos = 1;
        sign = true;
    } else {
        sign = false;
    }
    for (size_t i = start_pos; i < value.size(); i++) {
        if (!isdigit(value[i])) {
            throw std::runtime_error("Unknown symbol");
        }
        multiply_by_short(10);
        *this += big_integer(value[i] - '0');
    }
    number.push_back(0);
    normalize();
    if (sign) {
        negate();
    }
}

big_integer::big_integer(big_integer const &big_int) = default;

big_integer &big_integer::operator=(big_integer const &big_int) = default;

big_integer big_integer::operator+() const {
    return *this;
}

big_integer big_integer::operator-() const {
    auto result = *this;
    result.negate();
    return result;
}

big_integer big_integer::operator~() const {
    auto result = *this;
    result.bitwise_not();
    return result;
}

big_integer const big_integer::operator--(int) {
    auto res = *this;
    *this -= 1;
    return res;
}

big_integer const big_integer::operator++(int) {
    auto res = *this;
    *this += 1;
    return res;
}

big_integer &big_integer::operator--() {
    *this -= 1;
    return *this;
}

big_integer &big_integer::operator++() {
    *this += 1;
    return *this;
}

std::string to_string(big_integer const &big_int) {
    big_integer copy;
    if (big_int.sign()) {
        copy = -big_int;
    } else {
        copy = big_int;
    }
    std::string result;
    do {
        auto tmp = copy.divide_by_short_with_remainder(1000000000);
        for (int i = 0; i < 9; i++) {
            result += char('0' + tmp % 10);
            tmp /= 10;
        }
    } while (copy.number.size() != 1 || copy.number[0] != 0);
    while (result.size() > 1 && result.back() == '0') {
        result.pop_back();
    }
    if (big_int.sign()) {
        result += '-';
    }
    std::reverse(result.begin(), result.end());
    return result;
}

uint32_t big_integer::divide_by_short_with_remainder(uint32_t second) {
    uint64_t carry = 0;
    for (size_t i = number.size(); i-- > 0;) {
        carry = (carry << 32u) + number[i];
        number[i] = (static_cast<uint32_t>(carry / second));
        carry %= second;
    }
    normalize();
    return static_cast<int>(carry);
}

big_integer &big_integer::operator+=(big_integer const &second) {
    uint64_t carry = 0;
    size_t len = std::max(number.size(), second.number.size());
    size_t min_len = std::min(number.size(), second.number.size());
    for (size_t i = 0; i < len && (carry || i < min_len); i++) {
        carry += get_digit(i) + static_cast<uint64_t>(second.get_digit(i));
        set_digit(i, static_cast<uint32_t>(carry));
        carry >>= 32;
    }
    if (number.size() > len) {
        number.pop_back();
    }
    normalize();
    return *this;
}

big_integer &big_integer::operator-=(big_integer const &second) {
    *this += -second;
    return *this;
}

template<class fun>
big_integer &big_integer::bitwiseOperation(fun func, big_integer const &second) {
    size_t len = std::max(number.size(), second.number.size());
    for (size_t i = 0; i < len; i++) {
        set_digit(i, func(get_digit(i), second.get_digit(i)));
    }
    number.pop_back();
    normalize();
    return *this;
}

big_integer &big_integer::operator&=(big_integer const &second) {
    return bitwiseOperation(std::bit_and<>(), second);
}

big_integer &big_integer::operator|=(big_integer const &second) {
    return bitwiseOperation(std::bit_or<>(), second);
}

big_integer &big_integer::operator^=(big_integer const &second) {
    return bitwiseOperation(std::bit_xor<>(), second);
}

big_integer &big_integer::operator>>=(int second) {
    auto delta_full = static_cast<size_t>(second / 32);
    auto delta_local = static_cast<size_t>(second % 32);
    for (size_t i = 0; i < number.size(); i++) {
        uint32_t digit = 0;
        digit |= (get_digit(i + delta_full)) >> delta_local;
        if (delta_local > 0) {
            digit |= (get_digit(i + delta_full + 1) << (32 - delta_local));
        }
        number[i] = digit;
    }
    normalize();
    return *this;
}

big_integer &big_integer::operator<<=(int second) {
    auto delta_full = static_cast<size_t>(second / 32);
    auto delta_local = static_cast<size_t>(second % 32);
    for (size_t i = number.size() + delta_full; i-- > delta_full;) {
        uint32_t digit = 0;
        digit |= get_digit(i - delta_full) << delta_local;
        if (i > delta_full && delta_local > 0) {
            digit |= (get_digit(i - delta_full - 1) >> (32 - delta_local));
        }
        set_digit(i, digit);
    }
    for (size_t i = 0; i < delta_full; i++) {
        number[i] = 0;
    }
    normalize();
    return *this;
}


void big_integer::multiply_by_big(big_integer const &second) {
    std::vector<uint32_t> result(number.size() + second.number.size());
    uint64_t carry = 0;
    for (size_t i = 0; i < number.size(); i++) {
        carry = 0;
        for (size_t j = 0; j < second.number.size(); j++) {
            carry += result[i + j] + number[i] * static_cast<uint64_t>(second.number[j]);
            result[i + j] = static_cast<uint32_t>(carry);
            carry >>= 32;
        }
        result[i + second.number.size()] = static_cast<uint32_t>(carry);
    }
    number.swap(result);
    normalize();
}

big_integer &big_integer::operator*=(big_integer const &second) {
    bool result_sign = sign() != second.sign();
    big_integer a;
    if (sign()) {
        negate();
    }
    big_integer b;
    if (second.sign()) {
        b = -second;
    } else {
        b = second;
    }
    if (b.number.size() == 2) {
        multiply_by_short(b.number[0]);
    } else {
        multiply_by_big(b);
    }
    if (result_sign) {
        negate();
    }
    return *this;
}

void big_integer::divide_by_big(big_integer &dividend) {
    if (compare(dividend) == -1) {
        *this = 0;
        return;
    }
    auto norm = static_cast<uint32_t>((1ull << 32u) / (dividend.number[dividend.number.size() - 2] + 1));
    multiply_by_short(norm);
    dividend.multiply_by_short(norm);
    size_t N = dividend.number.size() - 1;
    size_t M = number.size() - N;
    std::vector<uint32_t> result(M + 1);
    for (size_t i = M + 1; i-- > 0;) {
        uint64_t q = ((static_cast<uint64_t>(get_digit(i + N)) << 32u) + get_digit(i + N - 1)) / dividend.number[N - 1];
        uint64_t r = ((static_cast<uint64_t>(get_digit(i + N)) << 32u) + get_digit(i + N - 1)) % dividend.number[N - 1];
        q = std::min(q, static_cast<uint64_t>(0xFFFFFFFF));
        while (q == (1ull << 32u) || (q * dividend.number[N - 2] > (r << 32u) + get_digit(i + N - 2))) {
            q--;
            r += dividend.number[N - 1];
            if (r >= (1ull << 32u)) {
                break;
            }
        }
        auto delta = dividend << (32 * i);
        big_integer d(delta);
        d.multiply_by_short(static_cast<uint32_t> (q));
        *this -= d;
        while (sign()) {
            *this += delta;
            q--;
        }
        result[i] = static_cast<uint32_t>(q);
    }
    result.push_back(0);
    number.swap(result);
    normalize();
}

big_integer &big_integer::operator/=(big_integer const &second) {
    if (second.number.size() == 1 && second.number[0] == 0) {
        throw std::runtime_error("Division by zero");
    }
    if (number.size() == 1 && number[0] == 0) {
        return *this = 0;
    }
    bool res_sign = sign() ^second.sign();
    if (sign()) {
        negate();
    }
    big_integer dividend;
    if (second.sign()) {
        dividend = -second;
    } else {
        dividend = second;
    }
    if (dividend.number.size() == 2) {
        uint32_t value = dividend.number[0];
        divide_by_short_with_remainder(value);
    } else {
        divide_by_big(dividend);
    }
    if (res_sign) {
        negate();
    }
    return *this;
}

big_integer &big_integer::operator%=(big_integer const &second) {
    *this -= (*this) / second * second;
    return *this;
}

void big_integer::normalize() {
    auto last_digit = number.back();
    while (!number.empty() && number.back() == last_digit) {
        number.pop_back();
    }
    number.push_back(last_digit);
}

big_integer operator&(big_integer first, const big_integer &second) {
    return first &= second;
}

big_integer operator|(big_integer first, const big_integer &second) {
    return first |= second;
}

big_integer operator^(big_integer first, big_integer const &second) {
    return first ^= second;
}

bool operator<(big_integer const &first, big_integer const &second) {
    return first.compare(second) < 0;
}

bool operator<=(big_integer const &first, big_integer const &second) {
    return first.compare(second) <= 0;
}

bool operator>(big_integer const &first, big_integer const &second) {
    return first.compare(second) > 0;
}

bool operator>=(big_integer const &first, big_integer const &second) {
    return first.compare(second) >= 0;
}

bool operator==(big_integer const &first, big_integer const &second) {
    return first.compare(second) == 0;
}

bool operator!=(big_integer const &first, big_integer const &second) {
    return first.compare(second) != 0;
}

int big_integer::compare(big_integer const &second) const {
    int return_value = sign() ? -1 : 1;
    if (sign() != second.sign()) {
        return return_value;
    }
    if (number.size() > second.number.size()) {
        return return_value;
    }
    if (number.size() < second.number.size()) {
        return -return_value;
    }
    for (size_t i = number.size(); i-- > 0;) {
        if (number[i] > second.number[i]) {
            return return_value;
        }
        if (number[i] < second.number[i]) {
            return -return_value;
        }
    }
    return 0;
}


big_integer operator+(big_integer first, const big_integer &second) {
    return first += second;
}

big_integer operator-(big_integer first, const big_integer &second) {
    return first -= second;
}

big_integer operator*(big_integer first, const big_integer &second) {
    return first *= second;
}

big_integer operator/(big_integer first, const big_integer &second) {
    return first /= second;
}

big_integer operator%(big_integer first, const big_integer &second) {
    return first %= second;
}

bool big_integer::sign() const {
    return static_cast<bool>((number.back() >> 31u) & 1u);
}

uint32_t big_integer::get_digit(size_t pos) const {
    if (pos < number.size()) {
        return number[pos];
    }
    return sign() ? 0xFFFFFFFF : 0;
}

void big_integer::set_digit(size_t pos, uint32_t value) {
    if (pos + 1 >= number.size()) {
        number.resize(pos + 2, sign() ? 0xFFFFFFFF : 0);
    }
    number[pos] = value;
}

big_integer big_integer::operator>>(int second) const {
    auto result = *this;
    result >>= second;
    return result;
}

big_integer big_integer::operator<<(int second) const {
    auto result = *this;
    result <<= second;
    return result;
}

void big_integer::negate() {
    bitwise_not();
    *this += 1;
}

void big_integer::bitwise_not() {
    for (auto &digit : number) {
        digit = ~digit;
    }
}

void big_integer::multiply_by_short(uint32_t second) {
    uint64_t carry = 0;
    for (auto &digit : number) {
        carry += digit * static_cast<uint64_t>(second);
        digit = static_cast<uint32_t>(carry);
        carry >>= 32;
    }
    number.push_back(0);
    normalize();
}


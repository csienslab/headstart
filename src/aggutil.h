#include "verifier.h"

#define T_START(s) std::chrono::steady_clock::time_point begin_##s = std::chrono::steady_clock::now();
#define T_END(s) std::chrono::steady_clock::time_point end_##s = std::chrono::steady_clock::now(); \
    std::cout << #s << ":" << std::chrono::duration_cast<std::chrono::microseconds> (end_##s - begin_##s).count() << "us" << std::endl;

static const size_t nthreads = std::thread::hardware_concurrency();

std::vector<unsigned char> integer2bytes(integer x, uint64_t num_bytes) {
    std::vector<unsigned char> bytes;
    bool negative = false;
    if (x < 0) {
        x = abs(x);
        x = x - integer(1);
        negative = true;
    }
    for (int iter = 0; iter < num_bytes; iter++) {
        auto byte = (x % integer(256)).to_vector();
        if (negative)
            byte[0] ^= 255;
        bytes.push_back(byte[0]);
        x = x / integer(256);
    }
    std::reverse(bytes.begin(), bytes.end());
    return bytes;
}

std::string joinYs(std::vector<form> ys) {
    std::stringstream ss;
    int i = 0;
    for(; i < ys.size()-1; i++) {
        ss << ys[i].a.to_string() << "," << ys[i].b.to_string() << ",";
    }
    ss << ys[i].a.to_string() << "," << ys[i].b.to_string();

    return ss.str();
}

std::string joinA_iters(std::vector<int> a_iters) {
    std::stringstream ss;
    int i = 0;
    for(; i < a_iters.size()-1; i++) {
        ss << a_iters[i] << ",";
    }
    ss << a_iters[i];

    return ss.str();
}

std::vector<std::string> split(const string& str, const string& delim){
    std::vector<std::string> res;
    if(str == "") return res;
    std::string strs = str + delim;
    size_t pos, now = 0;
    size_t size = strs.size();
    while(now < size){
        pos = strs.find(delim, now);
        if (pos < size){
            std::string s = str.substr(now, pos-now);
            if (s != "") res.push_back(s);
            now = pos + delim.size();
        }
    }
    return res;
}

std::vector<integer> split2integer(const string& str, const string& delim){
    std::vector<integer> res;
    if(str == "") return res;
    std::string strs = str + delim;
    size_t pos, now = 0;
    size_t size = strs.size();
    while(now < size){
        pos = strs.find(delim, now);
        if (pos < size){
            std::string s = str.substr(now, pos-now);
            if (s != ""){
                if (integer(s) != 0) res.push_back(integer(s));
                else{
                    std::vector<uint8_t> ss(s.begin(), s.end());
                    res.push_back(integer(ss));
                }
            }
            now = pos + delim.size();
        }
    }
    return res;
}

std::vector<int> split2int(const string& str, const string& delim){
    std::vector<int> res;
    if(str == "") return res;
    std::string strs = str + delim;
    size_t pos, now = 0;
    size_t size = strs.size();
    while(now < size){
        pos = strs.find(delim, now);
        if (pos < size){
            std::string s = str.substr(now, pos-now);
            res.push_back(from_string<int>(s));
            now = pos + delim.size();
        }
    }
    return res;
}

integer FastPowMod(integer& base, integer& exp, integer& mod) {
    integer res;
    mpz_powm(res.impl, base.impl, exp.impl, mod.impl);
    return res;
}

std::tuple<form, int> H_G(integer challenge, integer D){
    int int_size = (D.num_bits() + 16) >> 4;
    int length = 256;
    std::vector<uint8_t> seed = integer2bytes(challenge, int_size);
    vector<int> bitmask = {0, 1};

    std::vector<uint8_t> hash(picosha2::k_digest_size);
    std::vector<uint8_t> blob;
    std::vector<uint8_t> sprout = seed;

    int ii = 0;
    while (true) {
        blob.resize(0);
        ii++;
        while ((int) blob.size() * 8 < length) {
            for (int i = (int) sprout.size() - 1; i >= 0; --i) {
                sprout[i]++;
                if (!sprout[i])
                    break;
            }
            picosha2::hash256(sprout.begin(), sprout.end(), hash.begin(), hash.end());
            blob.insert(blob.end(), hash.begin(),
                std::min(hash.end(), hash.begin() + length / 8 - blob.size()));
        }
        assert ((int) blob.size() * 8 == length);
        integer a(blob);
        for (int b: bitmask)
            a.set_bit(b, true);

        if (a.prime())
        {
            integer k = D%a;
            integer iters = (a-integer(1))/integer(2);
            integer r = FastPowMod(k, iters, a);
            if (r == integer(1)) {
                integer iters = (a+integer(1))/integer(4);
                integer b = FastPowMod(k, iters, a);
                
                if ( b%integer(2) == integer(0)) b = a - b;
                return std::make_tuple(form::from_abd(a, b, D), ii);
            }
        }
    }
}

form H_GFast(integer challenge, integer D, int a_iter){
    int int_size = (D.num_bits() + 16) >> 4;
    int length = 256;
    std::vector<uint8_t> seed = integer2bytes(challenge, int_size);
    vector<int> bitmask = {0, 1};
    std::vector<uint8_t> hash(picosha2::k_digest_size);
    std::vector<uint8_t> blob;
    std::vector<uint8_t> sprout = seed;

    std::cout << "H_GFast";
    int ii = 0;
    while (true) {
        blob.resize(0);
        ii++;
        while ((int) blob.size() * 8 < length) {
            for (int i = (int) sprout.size() - 1; i >= 0; --i) {
                sprout[i]++;
                if (!sprout[i])
                    break;
            }
            picosha2::hash256(sprout.begin(), sprout.end(), hash.begin(), hash.end());
            blob.insert(blob.end(), hash.begin(),
                std::min(hash.end(), hash.begin() + length / 8 - blob.size()));
        }
        assert ((int) blob.size() * 8 == length);
        integer a(blob);
        for (int b: bitmask)
            a.set_bit(b, true);

        if (ii == a_iter)
        {
            if (a.prime())
            {
                integer k = D%a;
                integer iters = (a-integer(1))/integer(2);
                integer r = FastPowMod(k, iters, a);
                if (r == integer(1)) {
                    integer iters = (a+integer(1))/integer(4);
                    integer b = FastPowMod(k, iters, a);
                    
                    if ( b%integer(2) == integer(0)) b = a - b;
                    return form::from_abd(a, b, D);
                }
            }
        }
    }
}

std::vector<uint8_t> int2bytes(int value){
    std::vector<uint8_t> result;
    result.push_back(value >> 24);
    result.push_back(value >> 16);
    result.push_back(value >>  8);
    result.push_back(value      );
    return result;
}

// O(t)
form PowFormWithQuotient(form g, integer& D, uint64_t num_iterations, integer& B, integer &L, PulmarkReducer& reducer){
    form x = form::identity(D);
    integer r = integer(1);
    for(int i = 0; i < num_iterations; i++){
        nudupl_form(x, x, D, L);
        if ( r*integer(2) >= B){
            nucomp_form(x, x, g, D, L);
            reducer.reduce(x);
        }
        r = ( r*integer(2) ) % B;
    }
    return x;
}
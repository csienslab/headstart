#include "verifier.h"

// #define TT_START(s) std::chrono::steady_clock::time_point begin_##s = std::chrono::steady_clock::now(); \
//      std::cout << "Start " << #s << " ..." << std::endl;
#define T_START(s) std::chrono::steady_clock::time_point begin_##s = std::chrono::steady_clock::now();
#define T_END(s) std::chrono::steady_clock::time_point end_##s = std::chrono::steady_clock::now(); \
    std::cout << #s << ":" << std::chrono::duration_cast<std::chrono::microseconds> (end_##s - begin_##s).count() << "us" << std::endl;

const std::string X_FILE ("/x_file.txt");
const std::string Y_FILE ("/y_file.txt");
const std::string PROOF_FILE ("/proof_file.txt");
const std::string D_ITER_FILE ("/d_iter.txt");
const std::string A_ITERS_FILE ("/a_iters.txt");
const std::string B_ITER_FILE ("/b_iter.txt");

static const size_t nthreads = std::thread::hardware_concurrency();

std::vector<uint8_t> int2bytes(int value){
    std::vector<uint8_t> result;
    result.push_back(value >> 24);
    result.push_back(value >> 16);
    result.push_back(value >>  8);
    result.push_back(value      );
    return result;
}

std::vector<unsigned char> ConvertIntegerToBytes2(integer x, uint64_t num_bytes) {
    std::vector<unsigned char> bytes;
    bool negative = false;
    if (x < 0) {
        x = abs(x);
        x = x - integer(1);
        negative = true;
    }
    for (int iter = 0; iter < num_bytes; iter++) {
        auto byte = (x % integer(256)).to_vector();
        if (byte.empty()) byte.push_back(0);
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
    std::vector<uint8_t> seed = ConvertIntegerToBytes2(challenge, int_size);
    vector<int> bitmask = {0, 1};
    // a=3(mod 4)
    // 這樣的話假設 d=k(mod a) where 0<k<a
    // b=k^{(a+1)/4}(mod a)就好了
    std::vector<uint8_t> hash(picosha2::k_digest_size);  // output of sha256
    std::vector<uint8_t> blob;  // output of 1024 bit hash expansions
    std::vector<uint8_t> sprout = seed;  // seed plus nonce
    // d^((a-1)/2)=1(mod a)
    int ii = 0;
    // integer dd = (d - integer(1))/(integer(-4));
    while (true) {  // While prime is not found
        blob.resize(0);
        ii++;
        // cuz sha256 returns 32 bytes
        // repeat it to fill blob
        while ((int) blob.size() * 8 < length) {
            // Increment sprout by 1
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

        // when a is prime
        // k <- d mod a
        // b <- k**((a+1)/4) mod a
        if (a.prime())
        {
            // d mod a -> k
            integer k = D%a;
            // std::cout << "a: " << a.to_string() << std::endl;
            integer iters = (a-integer(1))/integer(2);
            integer r = FastPowMod(k, iters, a);
            if (r == integer(1)) {
                // std::cout << "r: " << r.to_string() << std::endl;
                integer iters = (a+integer(1))/integer(4);
                // base, exp, mod
                // k^a mod a -> b
                integer b = FastPowMod(k, iters, a);
                // b=k^{(a+1)/4}(mod a)
                // d^((a-1)/2)=1(mod a)
                
                // std::cout << "generator_hash_a: a=" << a.to_string() << std::endl;
                if ( b%integer(2) == integer(0)) b = a - b;
                return std::make_tuple(form::from_abd(a, b, D), ii);
            }
        }
    }
}

form H_GFast(integer challenge, integer D, int a_iter){
    int int_size = (D.num_bits() + 16) >> 4;
    int length = 256;
    std::vector<uint8_t> seed = ConvertIntegerToBytes2(challenge, int_size);
    vector<int> bitmask = {0, 1};
    std::vector<uint8_t> hash(picosha2::k_digest_size);  // output of sha256
    std::vector<uint8_t> blob;  // output of 1024 bit hash expansions
    std::vector<uint8_t> sprout = seed;  // seed plus nonce
    // d^((a-1)/2)=1(mod a)
    int ii = 0;
    // integer dd = (d - integer(1))/(integer(-4));
    while (true) {  // While prime is not found
        blob.resize(0);
        ii++;
        // cuz sha256 returns 32 bytes
        // repeat it to fill blob
        while ((int) blob.size() * 8 < length) {
            // Increment sprout by 1
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

        // when a is prime
        // k <- d mod a
        // b <- k**((a+1)/4) mod a
        if (ii == a_iter)
        {
            if (a.prime())
            {
                // d mod a -> k
                integer k = D%a;
                // std::cout << "a: " << a.to_string() << std::endl;
                integer iters = (a-integer(1))/integer(2);
                integer r = FastPowMod(k, iters, a);
                if (r == integer(1)) {
                    // std::cout << "r: " << r.to_string() << std::endl;
                    integer iters = (a+integer(1))/integer(4);
                    // base, exp, mod
                    // k^a mod a -> b
                    integer b = FastPowMod(k, iters, a);
                    // b=k^{(a+1)/4}(mod a)
                    // d^((a-1)/2)=1(mod a)
                    
                    // std::cout << "generator_hash_a: a=" << a.to_string() << std::endl;
                    if ( b%integer(2) == integer(0)) b = a - b;
                    return form::from_abd(a, b, D);
                }
            }
        }
    }
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
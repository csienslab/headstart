#include "verifier.h"
#include "prover_slow.h"
#include "aggutil.h"

// O(t)
std::tuple<form, int> EvalVDF(integer D, integer challenge, uint64_t num_iterations, bool time_eval = true) {
    integer L = root(-D, 4);
    PulmarkReducer reducer;
    // T_START(EvalVDF_H_G)
    form y;
    int a_iters;
    tie(y, a_iters) = H_G(challenge, D);
    // T_END(EvalVDF_H_G)
    if(time_eval) {
        T_START(EvalVDF_y)
        for (int i = 0; i < num_iterations; i++) {
            nudupl_form(y, y, D, L);
            reducer.reduce(y);
        }
        T_END(EvalVDF_y)
    }
    else {
        for (int i = 0; i < num_iterations; i++) {
            nudupl_form(y, y, D, L);
            reducer.reduce(y);
        }
    }
    return tie(y, a_iters);
}

// O(L)
std::tuple<form, int> ProveAggVDF(integer D, int agg_num,
    std::vector<integer>& challenges, 
    std::vector<form>& y,
    uint64_t num_iterations, 
    std::vector<int> a_iters)
{
    int int_size = (D.num_bits() + 16) >> 4;
    integer L = root(-D, 4);
    PulmarkReducer reducer;
    std::vector<form> g;
    for (int i = 0; i < agg_num; i++)
        g.push_back( H_GFast(challenges[i], D, a_iters[i]) );
    
    std::vector<uint8_t> s;
    for (int i = 0; i < agg_num; i++){
        std::vector<uint8_t> g_bytes = SerializeForm(g[i], int_size);
        std::vector<uint8_t> y_bytes = SerializeForm(y[i], int_size);
        s.insert(s.end(), g_bytes.begin(), g_bytes.end());
        s.insert(s.end(), y_bytes.begin(), y_bytes.end());
    }

    integer B;
    int b_iter;
    tie(B, b_iter) = HashPrimeReturnsIteration(s, 264, {263});
    form agg_g = form::identity(D);
    for (int i = 0; i < agg_num; i++){
        std::vector<uint8_t> seed = int2bytes(i);
        seed.insert(seed.end(), s.begin(), s.end());
        std::vector<uint8_t> hash(picosha2::k_digest_size);  // output of sha256
        picosha2::hash256(seed.begin(), seed.end(), hash.begin(), hash.end());
        // T_START(alpha)
        // 2**(k_digest_size) = 2**32
        integer alpha(hash);
        std::cout << alpha.to_string() << std::endl;
        // T_END(alpha)
        agg_g = agg_g * FastPowFormNucomp(g[i], D, alpha, L, reducer);
    }

    // O(t)
    form proof = PowFormWithQuotient(agg_g, D, num_iterations, B, L, reducer);
    /*
    std::vector<uint8_t> result = SerializeForm(y, int_size);
    std::vector<uint8_t> proof_bytes = SerializeForm(proof, int_size);
    result.insert(result.end(), proof_bytes.begin(), proof_bytes.end());
    */
    // return result;
    return std::make_tuple(proof, b_iter);
}

void VerifyAggProof(integer &D, int agg_num, std::vector<form>& x, std::vector<form> y, 
                        form proof, uint64_t iters, bool &is_valid){
    PulmarkReducer reducer;
    int int_size = (D.num_bits() + 16) >> 4;
    integer L = root(-D, 4);
    std::vector<uint8_t> s;
    for (int i = 0; i < agg_num; i++){
        std::vector<uint8_t> x_bytes = SerializeForm(x[i], int_size);
        std::vector<uint8_t> y_bytes = SerializeForm(y[i], int_size);
        s.insert(s.end(), x_bytes.begin(), x_bytes.end());
        s.insert(s.end(), y_bytes.begin(), y_bytes.end());
    }
    integer B = HashPrime(s, 264, {263});
    form agg_x = form::identity(D);
    form agg_y = form::identity(D);
    for (int i = 0; i < agg_num; i++){
        std::vector<uint8_t> seed = int2bytes(i);
        seed.insert(seed.end(), s.begin(), s.end());
        std::vector<uint8_t> hash(picosha2::k_digest_size);  // output of sha256
        picosha2::hash256(seed.begin(), seed.end(), hash.begin(), hash.end());
        integer alpha(hash);
        agg_x = agg_x * FastPowFormNucomp(x[i], D, alpha, L, reducer);
        agg_y = agg_y * FastPowFormNucomp(y[i], D, alpha, L, reducer);
    }
    integer r = FastPow(2, iters, B);
    form f1 = FastPowFormNucomp(proof, D, B, L, reducer);
    form f2 = FastPowFormNucomp(agg_x, D, r, L, reducer);
    if (f1 * f2 == agg_y)
    {
        is_valid = true;
    }
    else
    {
        is_valid = false;
    }
}

void VerifyAggProofParallel(integer &D, int agg_num, std::vector<form>& x, std::vector<form>& y, 
                        form proof, uint64_t iters, bool &is_valid, int b_iter = -1){
    PulmarkReducer reducer;
    int int_size = (D.num_bits() + 16) >> 4;
    integer L = root(-D, 4);
    std::vector<uint8_t> s;
    T_START(VerifyAggProof_1)
    for (int i = 0; i < agg_num; i++){
        std::vector<uint8_t> x_bytes = SerializeForm(x[i], int_size);
        std::vector<uint8_t> y_bytes = SerializeForm(y[i], int_size);
        s.insert(s.end(), x_bytes.begin(), x_bytes.end());
        s.insert(s.end(), y_bytes.begin(), y_bytes.end());
    }
    T_END(VerifyAggProof_1)
    
    integer B;
    if (b_iter == -1) {
        T_START(HashPrime_B)
        B = HashPrime(s, 264, {263});
        T_END(HashPrime_B)
    } else {
        T_START(HashPrimeFast_B)
        B = HashPrimeFast(s, 264, {263}, b_iter);
        T_END(HashPrimeFast_B)
    }
    
    T_START(VerifyAggProof_2)
    form agg_x = form::identity(D);
    form agg_y = form::identity(D);
    
    // const size_t nthreads = 4;
    std::cout<<"parallel ("<<nthreads<<" threads):"<<std::endl;
    std::cout<<"size of x " << x.size()<<std::endl;
    std::vector<std::thread> threads(nthreads);
    std::vector<form> agg_xs(nthreads), agg_ys(nthreads);
    // std::mutex g_pages_mutex;
    for(int t = 0;t<nthreads;t++)
    {
        threads[t] = std::thread(std::bind(
        [&](const int bi, const int ei, const int t)
        {
            // PulmarkReducer and hash cannot be used as a shared variable in threads
            PulmarkReducer reducer;
            std::vector<uint8_t> hash(picosha2::k_digest_size);  // output of sha256
            form agg_xx = form::identity(D);
            form agg_yy = form::identity(D);
            for(int i = bi;i<ei;i++)
            {
                // 0us
                std::vector<uint8_t> seed = int2bytes(i);
                // 22us
                seed.insert(seed.end(), s.begin(), s.end());
                // 2000us
                picosha2::hash256(seed.begin(), seed.end(), hash.begin(), hash.end());
                integer alpha(hash);
                // 6000us
                agg_xx = agg_xx * FastPowFormNucomp(x[i], D, alpha, L, reducer);
                // 6000us
                agg_yy = agg_yy * FastPowFormNucomp(y[i], D, alpha, L, reducer);        
            }
            // std::lock_guard<std::mutex> guard(g_pages_mutex);
            // do not use push_back or index racing will happend
            agg_xs[t] = agg_xx;
            agg_ys[t] = agg_yy;
        },t*agg_num/nthreads,(t+1)==nthreads?agg_num:(t+1)*agg_num/nthreads,t));
    }
    std::for_each(threads.begin(),threads.end(),[](std::thread& x){x.join();});

    for (int i=0; i < agg_xs.size(); i++) {
        agg_x = agg_x * agg_xs[i];
        agg_y = agg_y * agg_ys[i];
    }

    T_END(VerifyAggProof_2)
    T_START(VerifyAggProof_3)
    // 0.805089715
    integer r = FastPow(2, iters, B);
    form f1 = FastPowFormNucomp(proof, D, B, L, reducer);
    form f2 = FastPowFormNucomp(agg_x, D, r, L, reducer);
    T_END(VerifyAggProof_3)
    if (f1 * f2 == agg_y)
    {
        is_valid = true;
    }
    else
    {
        is_valid = false;
    }
}
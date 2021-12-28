#include "aggvdf.h"

int main(int argc, char ** argv) {

  bool is_parallel = false;
  int L = 2; // L
  uint64_t T = 1000; // T
  // std::string discriminant_seed_str ("abcdefghijklmnopqrstuvwxyz012345"); // dicriminant seed
  std::string discriminant_seed_str ("abcdefghijklmnopqrstuvwxyz012345"); // dicriminant seed
  int discriminant_size_bits = 6656; // k
  std::string all_challenges = "zxcasdqwe,qweasdzxc"; // X_roots

  T_START(Total)
  std::vector<integer> challenges_integer = split2integer(all_challenges, ",");
  if ( challenges_integer.size() != L ){
    std::cout << "Expect to get " << L << " X_roots, but get " << challenges_integer.size() << " X_roots" << std::endl;
    return 1;
  }
  
  std::vector<uint8_t> discriminant_seed(discriminant_seed_str.begin(), discriminant_seed_str.end());
  T_START(CreateDiscriminant)
  integer D;
  int d_iters;
  tie(D, d_iters) = CreateDiscriminantReturnsIteration(
    discriminant_seed,
    discriminant_size_bits
  );
  T_END(CreateDiscriminant)
  std::cout << "D = " << D.to_string() << std::endl;
  std::cout << "d_iters = " << d_iters << std::endl;

  std::vector<form> y(L);
  std::vector<int> a_iters(L);
  uint64_t t = T/L;
  T_START(EvalAllVDFs)
  // O(T)
  for(int i = 0; i < L; i++){
    // std::cout << "VDF computatation phase " << i << std::endl;
    // T_START(EvalVDF)
    form yy; int aa_iters;
    tie(yy, aa_iters) = EvalVDF(D, challenges_integer[i], t);
    // T_END(EvalVDF)
    // std::cout << "y_a = " << yy.a.to_string() << std::endl;
    // std::cout << "y_b = " << yy.b.to_string() << std::endl;
    y[i] = yy;
    a_iters[i] = aa_iters;
  }
  T_END(EvalAllVDFs)

  // O(L + t)
  std::cout << "asd";
  T_START(ProveAggVDF)
  form proof;
  int b_iter;
  tie(proof, b_iter) = ProveAggVDF(D, L, challenges_integer, y, t, a_iters);
  T_END(ProveAggVDF)
  
  // Verification

  // std::vector<integer> y_integer = split2integer(all_y, ",");
  // if ( y_integer.size() != 2*L ){
  //     std::cout << "Expect to get " << 2*L << " integers, but get " << y_integer.size() << " integers" << std::endl;
  //     return 1;
  // }

  // std::vector<form> g(L), y(L);
  std::vector<form> g(L);
  std::vector<std::thread> threads(nthreads);
  if (is_parallel)
  {
      T_START(H_G_parallel)
      for(int t = 0;t<nthreads;t++)
      {
          threads[t] = std::thread(std::bind(
          [&](const int bi, const int ei, const int t)
          {
              for (int i = bi; i < ei; i++){
                  // T_START(H_G)
                  int u;
                  // tie(g[i], u) = H_G(challenges_integer[i], D);
                  g[i] = H_GFast(challenges_integer[i], D, a_iters[i]);
                  // T_END(H_G)
                  // y[i] = form::from_abd(y_integer[2*i], y_integer[2*i+1], D);
              }
          },t*L/nthreads,(t+1)==nthreads?L:(t+1)*L/nthreads,t));
      }
      std::for_each(threads.begin(),threads.end(),[](std::thread& x){x.join();});
      T_END(H_G_parallel)
  } else {
    T_START(H_G)
      for (int i = 0; i < L; i++){
        // T_START(H_G)
        int u;
        tie(g[i], u) = H_G(challenges_integer[i], D);
        // std::cout << "i, u: " << i << ", "<< u << std::endl;
        // T_END(H_G)
        // y[i] = form::from_abd(y_integer[2*i], y_integer[2*i+1], D);
    }
    T_END(H_G)
  }

  bool is_valid = true;
  if (is_parallel)
  {
    T_START(VerifyAggProofParallel)
    VerifyAggProofParallel(D, L, g, y, proof, t, is_valid);
    // VerifyAggProofParallel(D, L, g, y, proof, t, is_valid, b_iter);
    T_END(VerifyAggProofParallel)
  } else {
    T_START(VerifyAggProof)
    VerifyAggProof(D, L, g, y, proof, t, is_valid);
    T_END(VerifyAggProof)
  }

  T_END(Total)
  std::cout << "is_valid: " << is_valid << std::endl;

  // std::cout << "proof_a = " << proof.a.to_string() << std::endl;
  // std::cout << "proof_b = " << proof.b.to_string() << std::endl;
  // std::cout << std::endl;

  return 0;
}
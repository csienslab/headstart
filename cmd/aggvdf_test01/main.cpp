#include "aggvdf.h"

int main(int argc, char ** argv) {

  bool is_parallel = false;
  // t is the number of iterations of each VDF
  uint64_t t = 83886;
  // discriminant_seed_str is used to hash into the discriminant of all VDFs
  std::string discriminant_seed_str ("abcdefghijklmnopqrstuvwxyz012345");
  int discriminant_size_bits = 1024;
  // all_challenges contains challenges of VDFs
  // separated by commas. For example, "zxcasdqwe,qweasdzxc" generates
  // a VDF from "zxcasdqwe" and another from "qweasdzxc".
  std::string all_challenges = "zxcasdqwea,qweasdzxc";
  std::vector<integer> challenge_integers = split2integer(all_challenges, ",");
  int proofs_num = challenge_integers.size();

  size_t nthreads = 1;
  if (is_parallel) {
    nthreads = std::thread::hardware_concurrency();
  }

  std::cout<<"parallel ("<<nthreads<<" threads):"<<std::endl;

  T_START(Total)
  T_START(CreateDiscriminantWithIteration)
  std::vector<uint8_t> discriminant_seed(discriminant_seed_str.begin(), discriminant_seed_str.end());
  // D is the discriminant used to generate 
  // all the group elements from the input
  // challenges to compute VDFs.
  // Elements with the same discriminant (D)
  // are deemed to be in the same class group.
  integer D;
  // d_iters is the number of iterations to hash the prime D.
  // d_iters can be used to accelerate the CreateDiscriminant in verification. 
  int d_iters;
  tie(D, d_iters) = CreateDiscriminantWithIteration(
    discriminant_seed,
    discriminant_size_bits
  );
  T_END(CreateDiscriminantWithIteration)
  
  std::vector<form> ys(proofs_num);
  std::vector<int> a_iters(proofs_num);

  T_START(EvalAggVdfs)
  // Complexity O(t)
  for(int i = 0; i < proofs_num; i++){
    // a_iters[i] is the number of iterations to hash the first coefficient (a)
    // of the i^th VDF's group element.
    tie(ys[i], a_iters[i]) = EvalAggVdf(D, challenge_integers[i], t);
  }
  T_END(EvalAggVdfs)

  // O(proofs_num + t)
  T_START(AggreateVdfProofs)
  form aggregated_proof;
  int b_iter;
  tie(aggregated_proof, b_iter) = AggreateVdfProofs(D, challenge_integers, ys, t, a_iters);
  T_END(AggreateVdfProofs)
  
  // Verification
  T_START(VerifyAggProof)
  bool is_valid = VerifyAggProof(D, challenge_integers, ys, aggregated_proof, t, nthreads, a_iters, b_iter);
  T_END(VerifyAggProof)

  T_END(Total)
  // is_valid should be 1.
  std::cout << "is_valid: " << is_valid << std::endl;

  return 0;
}
#include "aggvdf.h"

extern "C" {
    #include "TestU01.h"
}

uint32_t chiaVDF() {
    static integer state("zxcasdqwea");
    integer D("-0xaec3f10424ca397314e390fb26839323c0c10ddd16021b431bd5db69a03fd5c3cf77b33eb36f7cd6681eeb759606321065a155b6c6d98c901117f65e6e2c1aa41f31e062edc5c52c9da730b63db93e2da6aff2d0b3deea363689d4e227e81ced81bc397751be7d87610fbcb81cada37313a8ba009a7052f2662bd4851c9f683bbc65bf3809a0ff22e883cc7a89a565f367c01dcd96a23ef4ad3c01ddd9541d4b34d8bb0bcc51d240825bf3aad1ccd77c363897667029f3e0378e8fe714284eabfe93fc2a2ecd9c1373d42ab2ec039bf1f0ff978d6c27310805d76b8ae9ae73561d2ffe46afed1d7821c180af2638b245978536d461c1438a969c508f88cc4c4bb684abdecf4d62f082101e10f3c41334998868c9c4bbf9a1dd274f04f10395e91259f2a58554043d6d512a73185425d55327cea60ce45ac1f6d6e5068cdd9bff7857adf249249b3713e7fd75b5914906b458ec01c077354a06ccabb1f1ad705a7b10a06370392bda1938b8e9ef375f04b4aab14a4ac905eaf1ace6144f65afdd866eab2b5d192f446c6a8544a7287eacdac6aab5fcd7188876d4548b500d0d956eba6c84a0e8e9321980018b02cc5fd35fadb46388cb740354a66683b4840e5096512859bd128a9931146460de149fbcb6cb5641b91ece8a1ed806ab95ca8ebc7551e2a31cb605789ae8a6603b2d1ea41532a91e18fd50e67d15d2c174a4019c191c762cdd3392b8644d892ecb77b32f209cd0a3a0cec501869e5462647da1126e5dab0267b0fe4260c9ea4eb659b6920b00285e163cd6527562519a8758ea8968d165621cdae38139499b579d7a030b66f5f4b4d8d424dc7ef515adfe07c2ddddaea0c69c9b739a2de47c5b87263a65acdcd076886c4b31088827a3de7e6abd39c911d0c5b9ba8f420caef7852649d567cf82a0aa6d5bbd20ae41dd2aed0768d2edce659c342d56e4d5eeccd4d95c91077549050e313f9d508380260ecbb1072e647f3e7d8b35f01af0cd4329a61a2d5d544b80ce543ffa045ccde621ae63763e29457826817e42f76631c06f6762bffd4c00f9124db2bcad3a2cb2ae69409ef66f04fc73f77060555ecefbfcca2f4fdd163ee9d4f96e6bc7289b3f0a359d8d143550741f3eba16d633af8800e2498dd13c07512db9dfbe4f6d5ea8f48846b7");
    form yy; int aa_iters;
    tie(yy, aa_iters) = EvalVDF(D, state, 100, false);
    int int_size = (D.num_bits() + 16) >> 4;
    std::vector<uint8_t> yy_bytes = SerializeForm(yy, int_size);
    std::vector<uint8_t> hash(picosha2::k_digest_size);  // output of sha256
    picosha2::hash256(yy_bytes.begin(), yy_bytes.end(), hash.begin(), hash.end());
    state = integer(hash);
    std::vector<uint64_t> s = state.to_vector();
    uint32_t value = s[0];
    return value;
}

int main()
{
    // Create TestU01 PRNG object for our generator
    string name = "chia VDF";
    unif01_Gen* gen = unif01_CreateExternGenBits((char *) name.c_str(), chiaVDF);
    // for (int i = 0; i < 100; i++) {
    //     cout<<chiaVDF()<<endl;
    // }

    // Run the tests.
    bbattery_SmallCrush(gen);

    // Clean up.
    unif01_DeleteExternGenBits(gen);

    return 0;
}
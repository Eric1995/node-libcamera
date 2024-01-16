//"胶水工具" 解决了大多数原生到Web的跨平台问题
// #include "fec.h"
#include <emscripten.h>

int g_int = 42;

// 条件编译 在C++编译器中以C语言的规则来处理代码，防止Name Mangling处理
#ifdef __cplusplus
extern "C"
{
#endif

    // 利用宏防止函数被DCE
    EMSCRIPTEN_KEEPALIVE int add(int x, int y)
    {
        return x + y;
    }

    EMSCRIPTEN_KEEPALIVE int *get_int_ptr()
    {
        return &g_int;
    }

    EMSCRIPTEN_KEEPALIVE int sum(unsigned char *ptr, int length)
    {
        int total = 0;
        for (int i = 0; i < length; i++)
        {
            total += ptr[i];
        }
        return total;
    }

    // EMSCRIPTEN_KEEPALIVE void wasm_fec_init(void) {
    //     return fec_init();
    // }

    // EMSCRIPTEN_KEEPALIVE void wasm_fec_free(fec_t* p) {
    //     return fec_free(p);
    // }

    // EMSCRIPTEN_KEEPALIVE fec_t* wasm_fec_new(unsigned short k, unsigned short m) {
    //     return fec_new(k, m);
    // }

    // EMSCRIPTEN_KEEPALIVE void wasm_fec_encode(fec_t* code, gf** src, gf** fecs, unsigned* block_nums, size_t num_block_nums, size_t sz) {
    //     return fec_encode(code, src, fecs, block_nums, num_block_nums, sz);
    // }

    // EMSCRIPTEN_KEEPALIVE void wasm_fec_decode(fec_t* code, gf** inpkts, gf** outpkts, unsigned* index, size_t sz) {
    //     return fec_decode(code, inpkts, outpkts, index, sz);
    // }

#ifdef __cplusplus
}
#endif
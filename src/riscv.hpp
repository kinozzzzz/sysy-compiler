#include "koopa.h"
#include <assert.h>
#include <iostream>

using namespace std;

static void print_riscv(const char *buf)
{
    koopa_program_t program;
    koopa_error_code_t ret = koopa_parse_from_string(buf, &program);
    assert(ret == KOOPA_EC_SUCCESS);  // 确保解析时没有出错
    // 创建一个 raw program builder, 用来构建 raw program
    koopa_raw_program_builder_t builder = koopa_new_raw_program_builder();
    // 将 Koopa IR 程序转换为 raw program
    koopa_raw_program_t raw = koopa_build_raw_program(builder, program);
    // 释放 Koopa IR 程序占用的内存
    koopa_delete_program(program);

    cout<<"  .text"<<endl;
  
    for (size_t i = 0; i < raw.funcs.len; ++i) 
    {

        assert(raw.funcs.kind == KOOPA_RSIK_FUNCTION);
        // 获取当前函数
        koopa_raw_function_t func = (koopa_raw_function_t) raw.funcs.buffer[i];

        cout<<"  .globl "<<func->name+1<<endl;
        if(func->bbs.len == 0)  continue;   // 函数声明，跳过
        cout<<func->name+1<<":"<<endl;

        for (size_t j = 0; j < func->bbs.len; ++j) 
        {
            koopa_raw_basic_block_t bb = (koopa_raw_basic_block_t)func->bbs.buffer[j];
            for (size_t k = 0; k < bb->insts.len; ++k)
            {
                koopa_raw_value_t value = (koopa_raw_value_t)bb->insts.buffer[k];
                koopa_raw_value_t ret_value = value->kind.data.ret.value;
                assert(ret_value->kind.tag == KOOPA_RVT_INTEGER);
                int32_t int_val = ret_value->kind.data.integer.value;
                cout<<"  li "<<"a0 , "<<int_val<<endl;
                cout<<"  ret"<<endl;
            }
        }
    }



    // 处理完成, 释放 raw program builder 占用的内存
    // 注意, raw program 中所有的指针指向的内存均为 raw program builder 的内存
    // 所以不要在 raw program 处理完毕之前释放 builder
    koopa_delete_raw_program_builder(builder);
}


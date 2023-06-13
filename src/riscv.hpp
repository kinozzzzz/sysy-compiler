#include "koopa.h"
#include <assert.h>
#include <iostream>
#include <map>
#include <set>

using namespace std;

static map<int,const char*> RegName;
static map<koopa_raw_value_t,int> Instr;   //每条指令所在寄存器(可能某个时刻不存在)
static map<int,koopa_raw_value_t> Reg;   //寄存器中的此时所存得指令的计算值
static set<koopa_raw_value_t> GenerInstr;   //已经生成汇编代码的指令

static int findAvailableReg()
{
    for(int reg=0;reg < 15;reg++)
    {
        int if_ret = 1;
        if(Reg[reg] != NULL)
        {
            koopa_raw_value_t value = Reg[reg];
            for(int i=0;i < value->used_by.len;i++)
            {
                if(!GenerInstr.count((koopa_raw_value_t)(value->used_by).buffer[i]))
                {
                    if_ret=0;
                    break;
                }
            }
            if(if_ret)
                return reg;
        }
        else
            return reg;
    }
    cout<<"test"<<endl;
    cout<<endl;
    return 0;
}

static int findInstrReg(koopa_raw_value_t value)
{
    int reg;
    if(value->kind.tag==KOOPA_RVT_INTEGER)
    {
        if(value->kind.data.integer.value==0)
        {
            reg=-1;
        }
        else
        {
            reg = findAvailableReg();
            cout<<"  li "<<RegName[reg]<<", "<<value->kind.data.integer.value<<endl;
        }
    }
    else 
        reg=Instr[value];
    
    Reg[reg] = value;
    return reg;
}

static inline void genInstr(const char* op,int reg1,int reg2,int reg3)
{
    cout<<"  "<<op<<" "<<RegName[reg1]<<", "<<RegName[reg2]<<", "<<RegName[reg3]<<endl;
}

static void print_binary(koopa_raw_value_t value)
{
    koopa_raw_binary_t bin = value->kind.data.binary;
    int lreg = findInstrReg(bin.lhs);
    int rreg = findInstrReg(bin.rhs);
    int reg = findAvailableReg();
    switch(bin.op)
    {
        case KOOPA_RBO_NOT_EQ:
            genInstr("xor",reg,lreg,rreg);
            cout<<"  snez "<<RegName[reg]<<", "<<RegName[reg]<<endl;
            break;
        case KOOPA_RBO_EQ:
            genInstr("xor",reg,lreg,rreg);
            cout<<"  seqz "<<RegName[reg]<<", "<<RegName[reg]<<endl;
            break;
        case KOOPA_RBO_GT:
            genInstr("sgt",reg,lreg,rreg);
            break;
        case KOOPA_RBO_LT:
            genInstr("slt",reg,lreg,rreg);
            break;
        case KOOPA_RBO_GE:
            genInstr("slt",reg,lreg,rreg);
            cout<<"  seqz "<<RegName[reg]<<", "<<RegName[reg]<<endl;
            break;
        case KOOPA_RBO_LE:
            genInstr("sgt",reg,lreg,rreg);
            cout<<"  seqz "<<RegName[reg]<<", "<<RegName[reg]<<endl;
            break;
        case KOOPA_RBO_ADD:
            genInstr("add",reg,lreg,rreg);
            break;
        case KOOPA_RBO_SUB:
            genInstr("sub",reg,lreg,rreg);
            break;
        case KOOPA_RBO_MUL:
            genInstr("mul",reg,lreg,rreg);
            break;
        case KOOPA_RBO_DIV:
            genInstr("div",reg,lreg,rreg);
            break;
        case KOOPA_RBO_MOD:
            genInstr("rem",reg,lreg,rreg);
            break;
        case KOOPA_RBO_AND:
            genInstr("and",reg,lreg,rreg);
            break;
        case KOOPA_RBO_OR:
            genInstr("or",reg,lreg,rreg);
            break;
        case KOOPA_RBO_XOR:
            genInstr("xor",reg,lreg,rreg);
            break;
        case KOOPA_RBO_SHL:
            break;
        case KOOPA_RBO_SHR:
            break;
        case KOOPA_RBO_SAR:
            break;
    }
    Instr.emplace(value,reg);
    Reg[reg] = value;
    GenerInstr.emplace(bin.lhs);
    GenerInstr.emplace(bin.rhs);
    GenerInstr.emplace(value);
}

static void print_func(koopa_raw_function_t func)
{
    if(func->bbs.len == 0) return;   // 函数声明，跳过
    cout<<func->name+1<<":"<<endl;
    for (size_t i = 0; i < func->bbs.len; i++) 
    {
        koopa_raw_basic_block_t block = (koopa_raw_basic_block_t)func->bbs.buffer[i];
        for (size_t k = 0; k < block->insts.len; k++)
        {
            koopa_raw_value_t value = (koopa_raw_value_t)block->insts.buffer[k];
            /*
                value->kind.tag: IR对应的指令类型
                value->kind.data.binary.op: IR的具体操作类型，加减乘除等
            */
            switch ((value->kind).tag)
            {
                case KOOPA_RVT_INTEGER:
                    break;

                case KOOPA_RVT_ZERO_INIT:
                    break;

                case KOOPA_RVT_UNDEF:
                    break;

                case KOOPA_RVT_AGGREGATE:
                    break;

                case KOOPA_RVT_FUNC_ARG_REF:
                    break;

                case KOOPA_RVT_BLOCK_ARG_REF:
                    break;

                case KOOPA_RVT_ALLOC:
                    break;

                case KOOPA_RVT_GLOBAL_ALLOC:
                    break;

                case KOOPA_RVT_LOAD:
                    break;

                case KOOPA_RVT_STORE:
                    break;
                    
                case KOOPA_RVT_GET_PTR:
                    break;

                case KOOPA_RVT_GET_ELEM_PTR:
                    break;

                case KOOPA_RVT_BINARY:
                    print_binary(value);
                    break;

                case KOOPA_RVT_BRANCH:
                    break;

                case KOOPA_RVT_JUMP:
                    break;

                case KOOPA_RVT_CALL:
                    break;

                case KOOPA_RVT_RETURN:
                    int reg = findInstrReg(value->kind.data.ret.value);
                    if(reg != 7)
                    {
                        cout<<"  mv a0, "<<RegName[reg]<<endl;
                    }
                    cout<<"  ret"<<endl;
                    break;
            }
        }
    }
};

static void system_init()
{
    RegName.emplace(-1,"x0");
    RegName.emplace(0,"t0");
    RegName.emplace(1,"t1");
    RegName.emplace(2,"t2");
    RegName.emplace(3,"t3");
    RegName.emplace(4,"t4");
    RegName.emplace(5,"t5");
    RegName.emplace(6,"t6");
    RegName.emplace(7,"a0");
    RegName.emplace(8,"a1");
    RegName.emplace(9,"a2");
    RegName.emplace(10,"a3");
    RegName.emplace(11,"a4");
    RegName.emplace(12,"a5");
    RegName.emplace(13,"a6");
    RegName.emplace(14,"a7");

    for(int i=0;i < 15;i++)
        Reg.emplace(i,(koopa_raw_value_t)NULL);
}

static void print_riscv(const char *buf)
{
    koopa_program_t program;
    koopa_error_code_t ret = koopa_parse_from_string(buf, &program);
    assert(ret == KOOPA_EC_SUCCESS);  // 确保解析时没有出错
    koopa_raw_program_builder_t builder = koopa_new_raw_program_builder();
    koopa_raw_program_t raw = koopa_build_raw_program(builder, program);
    koopa_delete_program(program);

    system_init();

    cout<<"  .text"<<endl;
  
    for (size_t i = 0; i < raw.funcs.len; i++) 
    {
        koopa_raw_function_t func = (koopa_raw_function_t) raw.funcs.buffer[i];
        cout<<"  .globl "<<func->name+1<<endl;
    }
    for (size_t i = 0;i < raw.funcs.len; i++)
        print_func((koopa_raw_function_t) raw.funcs.buffer[i]);

    // 处理完成, 释放 raw program builder 占用的内存
    koopa_delete_raw_program_builder(builder);
}


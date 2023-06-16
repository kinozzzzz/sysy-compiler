#include "koopa.h"
#include <assert.h>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

using namespace std;

#define REG_t0 0
#define REG_a0 7
#define REG_ra 15

static int middle_block = 0;

static map<int,const char*> RegName;            //每个寄存器的名字
static map<koopa_raw_value_t,int> VarOffset;    //栈中变量的偏移量
static map<koopa_raw_value_t,int> Instr;        //每条指令所在寄存器(可能某个时刻不存在)
static map<int,koopa_raw_value_t> Reg;          //寄存器中的此时所存得指令的计算值
static map<koopa_raw_value_t,koopa_raw_basic_block_t> GenerInstr;       //已经生成汇编代码的指令
static set<koopa_raw_basic_block_t> GenerBlock; //已经生成的block

static int findInstrReg(koopa_raw_value_t value);

static void genMove(int src_reg,int dest_reg)
{
    if(src_reg != dest_reg)
    {
        cout<<"  mv "<<RegName[dest_reg]<<", "<<RegName[src_reg]<<endl;
    }
}

static inline void genStore2Stack(int reg,int offset)
{
    cout<<"  sw "<<RegName[reg]<<", "<<offset<<"(sp)"<<endl;
}

static inline void genLoad4Stack(int reg,int offset)
{
    cout<<"  lw "<<RegName[reg]<<", "<<offset<<"(sp)"<<endl;
}

/*
return 1 if the value in register is not needed anymore
return 0 otherwise
*/
static int RegIfAvailable(int reg,koopa_raw_value_t need_reg)
{
    int not_need=1;
    koopa_raw_value_t value = Reg[reg];
    if(Reg[reg] == NULL)
    {
        return 1;
    }
    for(int i=0;i < value->used_by.len;i++)
    {
        koopa_raw_value_t used = (koopa_raw_value_t)(value->used_by).buffer[i];
        if(!GenerInstr.count(used))     // 值仍被需要，保留寄存器
        {
            if(used == need_reg)   // 恰好可以覆盖寄存器
                continue;
            not_need=0;
            break;
        }
        else
        {
            koopa_raw_basic_block_t block = GenerInstr[used];
            // 如果这条指令和使用结果的指令在同一个block，那么可以覆盖
            if(block == GenerInstr[value])
                continue;
            for(int k=0;k < block->used_by.len;k++)
            {
                koopa_raw_basic_block_t used_block = (koopa_raw_basic_block_t)block->used_by.buffer[k];
                if(!GenerBlock.count(used_block))
                {
                    not_need = 0;
                }
            }
        }
    }
    return not_need;
}

static int findAvailableReg(koopa_raw_value_t need_reg)
{
    for(int reg=0;reg < REG_ra;reg++)
    {
        if(RegIfAvailable(reg,need_reg))
            return reg;
    }
    exit(-100);
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
            reg = findAvailableReg(value);
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
    int reg = findAvailableReg(value);
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
}

static void print_call(koopa_raw_value_t value,int offset)
{
    koopa_raw_slice_t args = value->kind.data.call.args;
    vector<int> save_reg;

    // 保存寄存器
    for(int reg=REG_t0;reg < REG_ra;reg++)  
        if(!RegIfAvailable(reg,NULL))
        {
            genStore2Stack(reg,(offset + reg)*4);
            save_reg.push_back(reg);
        }
            
    // 将函数参数保存在栈上
    for(int i=8;i < args.len;i++)
    {
        koopa_raw_value_t param = (koopa_raw_value_t)args.buffer[i];
        if(param->kind.tag == KOOPA_RVT_INTEGER)
        {
            cout<<"  li "<<RegName[REG_t0]<<", "<<param->kind.data.integer.value<<endl;
            genStore2Stack(REG_t0,(i-8)*4);
        }
        else
        {
            int param_reg = findInstrReg(param);
            genLoad4Stack(REG_t0,(offset + param_reg)*4);
            genStore2Stack(REG_t0,(i-8)*4);
        }
    }

    // 将函数参数保存在寄存器中
    for(int i=0;i < args.len && i < 8;i++)
    {
        koopa_raw_value_t param = (koopa_raw_value_t)args.buffer[i];
        if(param->kind.tag == KOOPA_RVT_INTEGER)
        {
            cout<<"  li "<<RegName[i+REG_a0]<<", "<<param->kind.data.integer.value<<endl;
        }
        else
        {
            genLoad4Stack(i+REG_a0,(offset + Instr[param])*4);
        }
    }
    koopa_raw_function_t callee = value->kind.data.call.callee;
    cout<<"  call "<<callee->name+1<<endl;

    // 考虑函数返回值与原先a0冲突的情况
    int reg=-1;
    if(callee->ty->tag != KOOPA_RTT_UNIT)
    {
        if(!RegIfAvailable(REG_a0,value))
        {
            reg = findAvailableReg(value);
            Instr[value] = reg;
            Reg[reg] = value;
            genMove(REG_a0,reg);
        }
        else
        {
            Instr[value] = REG_a0;
            Reg[REG_a0] = value;
            reg = REG_a0;
        }
    }

    // 将寄存器的值从栈中取出来
    for(int i=0;i < save_reg.size();i++)
    {
        if(save_reg[i] != reg && !RegIfAvailable(reg,value))
            genLoad4Stack(save_reg[i],(offset+save_reg[i])*4);
    }
}

static void print_func(koopa_raw_function_t func)
{
    cout<<"  .globl "<<func->name+1<<endl;
    if(func->bbs.len == 0) return;   // 函数声明，跳过
    cout<<func->name+1<<":"<<endl;

    VarOffset.clear();
    Instr.clear();
    Reg.clear();

    int stack_frame = 0;    // 为临时变量开辟空间
    int max_params = 0;     // 调用的子函数中最多的参数个数
    int extra_params = 0;   // 额外的参数个数(栈中参数)
    int func_params = 0;    // 函数自身已经存储的参数个数
    int if_call = 0;
    for(size_t i = 0;i < func->bbs.len;i++)
    {
        koopa_raw_basic_block_t block = (koopa_raw_basic_block_t)func->bbs.buffer[i];
        for(size_t k = 0;k < block->insts.len;k++)
        {
            koopa_raw_value_t value = (koopa_raw_value_t)block->insts.buffer[k];
            if(value->kind.tag == KOOPA_RVT_CALL)
            {
                if_call = 1;
                if(max_params < value->kind.data.call.args.len)
                    max_params = value->kind.data.call.args.len;
            }
        }
    }
    
    if(max_params > 8)
    {
        stack_frame += (max_params-8) * 4;
        extra_params = max_params-8;
    }

    stack_frame += 4;   // 保存ra
    
    // 保存临时寄存器
    if(if_call)
        stack_frame += 60;

    for(size_t i = 0;i < func->bbs.len;i++)
    {
        koopa_raw_basic_block_t block = (koopa_raw_basic_block_t)func->bbs.buffer[i];
        for(size_t k = 0;k < block->insts.len;k++)
        {
            koopa_raw_value_t value = (koopa_raw_value_t)block->insts.buffer[k];
            if(value->kind.tag == KOOPA_RVT_ALLOC)
            {
                VarOffset[value] = stack_frame;
                stack_frame += 4;
            }
        }
    }

    stack_frame = ((stack_frame+15) / 16 + 1) * 16;     // 1是为了可能存储的临时变量
    cout<<"  addi sp, sp, -"<<stack_frame<<endl;
    genStore2Stack(REG_ra,extra_params*4);

    for(size_t i = 0;i < func->bbs.len;i++)
    {
        koopa_raw_basic_block_t block = (koopa_raw_basic_block_t)func->bbs.buffer[i];
        GenerBlock.emplace(block);
        if(block->name && i != 0) cout<<block->name+1<<":"<<endl;
        for(size_t k = 0;k < block->insts.len;k++)
        {
            koopa_raw_value_t value = (koopa_raw_value_t)block->insts.buffer[k];
            /*
                value->kind.tag: IR对应的指令类型
                value->kind.data.binary.op: IR的具体操作类型，加减乘除等
            */
            koopa_raw_value_tag_t tag = (value->kind).tag;
            if(tag == KOOPA_RVT_LOAD)
            {
                koopa_raw_value_t load_src = value->kind.data.load.src;
                int reg = findAvailableReg(value);
                if(load_src->kind.tag == KOOPA_RVT_ALLOC)
                {
                    genLoad4Stack(reg,VarOffset[load_src]);
                }
                else if(load_src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC)
                {
                    cout<<"  la "<<RegName[reg]<<", "<<load_src->name+1<<endl;
                    cout<<"  lw "<<RegName[reg]<<", 0("<<RegName[reg]<<")"<<endl;
                }
                Instr.emplace(value,reg);
                Reg[reg] = value;
            }
            else if(tag == KOOPA_RVT_STORE)
            {
                koopa_raw_value_t store_value = value->kind.data.store.value;
                koopa_raw_value_t store_dest = value->kind.data.store.dest;
                int reg = findInstrReg(store_value);
                
                if(store_value->kind.tag == KOOPA_RVT_FUNC_ARG_REF)
                {
                    // 如果是函数参数则存储寄存器或从栈上转移值
                    if(func_params < 8)
                    {
                        genStore2Stack(func_params+REG_a0,VarOffset[store_dest]);
                    }
                    else
                    {
                        int reg = findAvailableReg(NULL);
                        genLoad4Stack(reg,stack_frame+(func_params-8)*4);
                        genStore2Stack(reg,VarOffset[store_dest]);
                    }
                    func_params++;
                }
                else if(store_dest->kind.tag == KOOPA_RVT_ALLOC)
                {
                    genStore2Stack(reg,VarOffset[store_dest]);
                }
                else if(store_dest->kind.tag == KOOPA_RVT_GLOBAL_ALLOC)
                {
                    int temp_reg = findAvailableReg(NULL);
                    cout<<"  la "<<RegName[temp_reg]<<", "<<store_dest->name+1<<endl;
                    cout<<"  sw "<<RegName[reg]<<", 0("<<RegName[temp_reg]<<")"<<endl;
                }
            }
            else if(tag == KOOPA_RVT_BINARY)
            {
                print_binary(value);
                GenerInstr.emplace(value->kind.data.binary.lhs,block);
                GenerInstr.emplace(value->kind.data.binary.rhs,block);
            }
            else if(tag == KOOPA_RVT_BRANCH)
            {
                koopa_raw_value_t cond = value->kind.data.branch.cond;
                koopa_raw_basic_block_t true_block = value->kind.data.branch.true_bb;
                koopa_raw_basic_block_t false_block = value->kind.data.branch.false_bb;
                int reg = findInstrReg(cond);
                if(true_block->insts.len <= 500)
                {
                    cout<<"  bnez "<<RegName[reg]<<", "<<true_block->name+1<<endl;
                    cout<<"  j "<<false_block->name+1<<endl;
                }
                else
                {
                    string middle = string("middle") + to_string(middle_block);
                    cout<<"  bnez "<<RegName[reg]<<", "<<middle<<endl;
                    cout<<"  j "<<false_block->name+1<<endl;
                    cout<<middle<<":"<<endl;
                    cout<<"  j "<<true_block->name+1<<endl;
                }
                
            }
            else if(tag == KOOPA_RVT_JUMP)
            {
                koopa_raw_basic_block_t target_block = value->kind.data.jump.target;
                cout<<"  j "<<target_block->name+1<<endl;
            }
            else if(tag == KOOPA_RVT_RETURN)
            {
                koopa_raw_value_t ret_value = value->kind.data.ret.value;
                if(ret_value != NULL)
                {
                    int reg = findInstrReg(ret_value);
                    genMove(reg,REG_a0);
                }
                genLoad4Stack(REG_ra,extra_params*4);
                cout<<"  addi sp, sp, "<<stack_frame<<endl;
                cout<<"  ret"<<endl;
            }
            else if(tag == KOOPA_RVT_CALL)
            {
                print_call(value,extra_params+1);
            }
            GenerInstr.emplace(value,block);
        }
    }
    cout<<endl;
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
    RegName.emplace(15,"ra");

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
    cout<<"  .data"<<endl;
    for(int i = 0;i < raw.values.len;i++)
    {
        koopa_raw_value_t value = (koopa_raw_value_t) raw.values.buffer[i];
        cout<<"  .global "<<value->name+1<<endl;
        koopa_raw_value_t init = value->kind.data.global_alloc.init;
        cout<<value->name+1<<":"<<endl;
        cout<<"  .word "<<init->kind.data.integer.value<<endl;
    }
    cout<<endl;
    cout<<"  .text"<<endl;

    for (size_t i = 0;i < raw.funcs.len; i++)
        print_func((koopa_raw_function_t) raw.funcs.buffer[i]);

    // 处理完成, 释放 raw program builder 占用的内存
    koopa_delete_raw_program_builder(builder);
}

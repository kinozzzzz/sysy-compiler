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
#define REG_sp 16

static int middle_block = 0;
static int temp_offset = 0;
static int replace_reg = REG_t0;

static map<int,const char*> RegName;            //每个寄存器的名字
static map<koopa_raw_value_t,int> VarOffset;    //栈中变量的偏移量
static map<koopa_raw_value_t,int> Instr;        //每条指令所在寄存器(可能某个时刻不存在)
static map<int,koopa_raw_value_t> Reg;          //指示寄存器被哪个指令占用
static map<koopa_raw_value_t,koopa_raw_basic_block_t> GenerInstr;       //已经生成汇编代码的指令
static set<koopa_raw_basic_block_t> GenerBlock; //已经生成的block
static map<koopa_raw_value_t,vector<int>> ArraySize;

static int findInstrReg(koopa_raw_value_t value);
static inline void genInstr(const char* op,int reg1,int reg2,int reg3);
static int findAvailableReg(koopa_raw_value_t need_reg);

static void genMove(int src_reg,int dest_reg)
{
    if(src_reg != dest_reg)
    {
        cout<<"  mv "<<RegName[dest_reg]<<", "<<RegName[src_reg]<<endl;
    }
}

static inline void genStore2Stack(int reg,int offset)
{
    if(offset > 2047)
    {
        int treg = findAvailableReg(NULL);
        cout<<"  li "<<RegName[treg]<<", "<<offset<<endl;
        genInstr("add",treg,REG_sp,treg);
        cout<<"  sw "<<RegName[reg]<<", 0("<<RegName[treg]<<")"<<endl;
    }
    else
        cout<<"  sw "<<RegName[reg]<<", "<<offset<<"(sp)"<<endl;
}

static inline void genLoad4Stack(int reg,int offset)
{
    if(offset > 2047)
    {
        int treg = findAvailableReg(NULL);
        cout<<"  li "<<RegName[treg]<<", "<<offset<<endl;
        genInstr("add",treg,REG_sp,treg);
        cout<<"  lw "<<RegName[reg]<<", 0("<<RegName[treg]<<")"<<endl;
    }
    else
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
    replace_reg++;
    if(replace_reg == REG_ra)
        replace_reg = REG_t0;
    koopa_raw_value_t value = Reg[replace_reg];
    VarOffset[value] = temp_offset;
    Instr[value] = -1;
    genStore2Stack(replace_reg,temp_offset);
    temp_offset += 4;
    Reg[replace_reg] = NULL;
    return replace_reg;
}

static int findInstrReg(koopa_raw_value_t value)
{
    int reg;
    if(value->kind.tag==KOOPA_RVT_INTEGER)
    {
        if(value->kind.data.integer.value==0)
        {
            reg = -1;
        }
        else
        {
            reg = findAvailableReg(value);
            cout<<"  li "<<RegName[reg]<<", "<<value->kind.data.integer.value<<endl;
        }
    }
    else if(value->kind.tag == KOOPA_RVT_ZERO_INIT)
    {
        return -1;
    }
    else
    {
        reg=Instr[value];
        if(reg == -1)
        {
            // -1说明寄存器中的值被存到了内存中
            reg = findAvailableReg(value);
            genLoad4Stack(reg,VarOffset[value]);
        }
        Instr[value] = reg;
    }
    Reg[reg] = value;
    return reg;
}

static inline void genInstr(const char* op,int reg1,int reg2,int reg3)
{
    cout<<"  "<<op<<" "<<RegName[reg1]<<", "<<RegName[reg2]<<", "<<RegName[reg3]<<endl;
}

static void genInit(koopa_raw_value_t init,vector<int> lens)
{
    if(init->kind.tag == KOOPA_RVT_INTEGER)
    {
        cout<<"  .word "<<init->kind.data.integer.value<<endl;
        return;
    }
    int len = *lens.rbegin();
    lens.pop_back();
    if(init->kind.tag == KOOPA_RVT_ZERO_INIT)
    {
        cout<<"  .zero "<<(len<<2)<<endl;
    }
    for(int i=0;i < init->kind.data.aggregate.elems.len;i++)
    {
        genInit((koopa_raw_value_t)init->kind.data.aggregate.elems.buffer[i],lens);
    }
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
            if(param_reg != -1)
                genLoad4Stack(REG_t0,(offset + param_reg)*4);
            else
                genLoad4Stack(REG_t0,VarOffset[param]);
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
            int reg = findInstrReg(param);
            if(reg != -1)
                genLoad4Stack(i+REG_a0,(offset + reg)*4);
            else
                genLoad4Stack(i+REG_a0,VarOffset[param]);
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
                const koopa_raw_type_kind *base = value->ty->data.pointer.base;
                if(base->tag == KOOPA_RTT_POINTER)
                {
                    // 处理数组参数
                    base = base->data.pointer.base;
                }
                int len = 1;
                vector<int> temp;
                while(base->tag == KOOPA_RTT_ARRAY)
                {
                    temp.push_back(base->data.array.len);
                    len *= base->data.array.len;
                    base = base->data.array.base;
                }
                vector<int> lens;
                int tlen = 1;
                for(auto i = temp.rbegin();i != temp.rend();i++)
                {
                    lens.push_back(tlen);
                    tlen *= *i;
                }
                if(value->ty->data.pointer.base->tag == KOOPA_RTT_POINTER)
                {
                    lens.push_back(tlen);
                }
                //cout<<lens.size()<<endl;
                if(len > 1) ArraySize[value] = lens;
                stack_frame += len<<2;
            }
        }
    }
    temp_offset = stack_frame;

    stack_frame = ((stack_frame+255) / 16) << 4;     // 1是为了可能存储的临时变量
    if(stack_frame > 2047)
    {
        int reg = findAvailableReg(NULL);
        cout<<"  li "<<RegName[reg]<<", -"<<stack_frame<<endl;
        genInstr("add",REG_sp,REG_sp,reg);
    }
    else
        cout<<"  addi sp, sp, -"<<stack_frame<<endl;
    genStore2Stack(REG_ra,extra_params*4);

    // 翻译指令部分
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
                ArraySize[value] = ArraySize[load_src];

                if(load_src->kind.tag == KOOPA_RVT_ALLOC)
                {
                    genLoad4Stack(reg,VarOffset[load_src]);
                }
                else if(load_src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC)
                {
                    cout<<"  la "<<RegName[reg]<<", "<<load_src->name+1<<endl;
                    cout<<"  lw "<<RegName[reg]<<", 0("<<RegName[reg]<<")"<<endl;
                }
                else
                {
                    int dest_reg = findInstrReg(load_src);
                    cout<<"  lw "<<RegName[reg]<<", 0("<<RegName[dest_reg]<<")"<<endl;
                }
                Instr[value] = reg;
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
                else
                {
                    int dest_reg = findInstrReg(store_dest);
                    cout<<"  sw "<<RegName[reg]<<", 0("<<RegName[dest_reg]<<")"<<endl;
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
                    string middle = string("middle") + to_string(middle_block++);
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
                    Reg[reg] = NULL;
                }
                genLoad4Stack(REG_ra,extra_params*4);
                if(stack_frame > 2047)
                {
                    int reg = findAvailableReg(NULL);
                    cout<<"  li "<<RegName[reg]<<", "<<stack_frame<<endl;
                    genInstr("add",REG_sp,REG_sp,reg);
                }
                else
                    cout<<"  addi sp, sp, "<<stack_frame<<endl;
                cout<<"  ret"<<endl;
            }
            else if(tag == KOOPA_RVT_CALL)
            {
                print_call(value,extra_params+1);
            }
            else if(tag == KOOPA_RVT_GET_ELEM_PTR)
            {
                koopa_raw_value_t src = (koopa_raw_value_t)value->kind.data.get_elem_ptr.src;
                koopa_raw_value_t index = (koopa_raw_value_t) value->kind.data.get_elem_ptr.index;
                int reg = findAvailableReg(NULL);
                Reg[reg] = value;
                int treg = findAvailableReg(NULL);
                Reg[treg] = value;
                if(src->kind.tag == KOOPA_RVT_ALLOC)
                {
                    int offset = VarOffset[src];
                    cout<<"  li "<<RegName[treg]<<", "<<offset<<endl;
                    genInstr("add",reg,REG_sp,treg);
                }
                else if(src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC)
                {
                    cout<<"  la "<<RegName[reg]<<", "<<src->name+1<<endl;
                }
                else
                {
                    Reg[reg] = NULL;
                    reg = findInstrReg(src);
                    Reg[reg] = value;
                }
                int index_reg = findInstrReg(index);
                vector<int> lens = ArraySize[src];
                int len = *lens.rbegin();
                lens.pop_back();
                ArraySize[value] = lens;
                cout<<"  li "<<RegName[treg]<<", "<<len*4<<endl;
                genInstr("mul",treg,treg,index_reg);
                genInstr("add",reg,reg,treg);
                Instr[value] = reg;
                Reg[treg] = NULL;
            }
            else if(tag == KOOPA_RVT_GET_PTR)
            {
                koopa_raw_value_t src = (koopa_raw_value_t)value->kind.data.get_ptr.src;
                koopa_raw_value_t index = (koopa_raw_value_t) value->kind.data.get_ptr.index;
                int reg = findInstrReg(src);
                Reg[reg] = value;
                int treg = findAvailableReg(NULL);
                Reg[treg] = value;
                int index_reg = findInstrReg(index);
                
                vector<int> lens = ArraySize[src];
                int len;
                if(lens.size() == 0)
                    len = 1;
                else
                {
                    len = *(lens.rbegin());
                    lens.pop_back();
                }
                ArraySize[value] = lens;
                cout<<"  li "<<RegName[treg]<<", "<<(len<<2)<<endl;
                genInstr("mul",treg,treg,index_reg);
                genInstr("add",reg,reg,treg);
                Instr[value] = reg;
                Reg[treg] = NULL;
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
    RegName.emplace(16,"sp");

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
        if(init->ty->tag == KOOPA_RTT_INT32)
        {
            cout<<"  .word "<<init->kind.data.integer.value<<endl;
        }
        else if(init->ty->tag == KOOPA_RTT_ARRAY)
        {
            const koopa_raw_type_kind *base = init->ty;
            int len = 1;
            vector<int> temp;
            while(base->tag == KOOPA_RTT_ARRAY)
            {
                temp.push_back(base->data.array.len);
                len *= base->data.array.len;
                base = base->data.array.base;
            }
            vector<int> lens;
            int tlen = 1;
            for(auto i = temp.rbegin();i != temp.rend();i++)
            {
                tlen *= *i;
                lens.push_back(tlen);
            }
            genInit(init,lens);
            lens.pop_back();
            lens.insert(lens.begin(),1);
            if(len > 1) ArraySize[value] = lens;
        }
    }
    cout<<endl;
    cout<<"  .text"<<endl;

    for (size_t i = 0;i < raw.funcs.len; i++)
        print_func((koopa_raw_function_t) raw.funcs.buffer[i]);

    // 处理完成, 释放 raw program builder 占用的内存
    koopa_delete_raw_program_builder(builder);
}

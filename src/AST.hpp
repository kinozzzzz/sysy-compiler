#pragma once
#include<string>
#include<memory>
#include<iostream>

using namespace std;

#define KOOPA_MODE 0
#define RISCV_MODE 1

class BaseAST
{
    public:
        virtual ~BaseAST() = default;
        virtual void print() = 0;
};

class ID : public BaseAST
{
    public:
        std::string id_name;

        virtual void print()
        {
            std::cout<<id_name;
        }
};

class Number : public BaseAST
{
    public:
        int num;

        virtual void print()
        {
            std::cout<<num;
        }
};

class Stmt : public BaseAST
{
    public:
        BaseAST* number;

        virtual void print()
        {
            number->print();
        }
};

class Block : public BaseAST
{
    public:
        BaseAST* stmt;

        virtual void print()
        {
            cout<<"{\n%"<<"entry:\n  ret ";
            stmt->print();
            cout<<"\n}";
        }
};

class FuncType : public BaseAST
{
    public:
        std::string type_name;

        virtual void print()
        {
            cout<<type_name<<" ";
        }
};

class FuncDef : public BaseAST
{
    public:
        BaseAST* func_type;
        std::string* id;
        BaseAST* block;

        virtual void print()
        {
            std::cout<<"fun @";
            std::cout<<*id<<"(): ";
            func_type->print();
            block->print();
        }
};

class CompUnit : public BaseAST
{
    public:
        BaseAST* func_def;

        virtual void print()
        {
            func_def->print();
        }
};

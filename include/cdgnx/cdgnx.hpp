#pragma once

#include <cstdint>
#include <string>
#include <memory>
#include <vector>

namespace cdgnx
{
    enum class OpType : uint8_t
    {
        /* primitives */
        LABEL,
        ROOT,

        /* immediate values */
        NUM,
        STR,

        /* arithmetics */
        IADD,
        ISUB,
        IMUL,
        IDIV,
        IMOD,
        FADD,
        FSUB,
        FDIV,
        FMOD,

        /* bitwise */
        BAND,
        BOR,
        BXOR,
        BNOT,
        BSHL,
        BSHR,

        /* cmp */
        ICMP,
        FCMP,
        TEST,

        /* mm */
        LOAD,
        STORE,
        LEA,

        /* control */
        CALL,
        RET,
        JMP,
        JE,
        JNE,
        JL,
        JLE,
        JG,
        JGE,

        /* stack */
        PUSH,
        POP,

        /* regop */
        MOV
    };

    struct Addr
    {
        int64_t offset = 0;
        std::string base;
        std::string index;
        uint8_t scale = 1;

        static Addr reg(const std::string &r)
        {
            Addr a;
            a.base = r;
            return a;
        }

        Addr &idx(const std::string &r, const uint8_t s = 1)
        {
            index = r;
            scale = s;
            return *this;
        }

        Addr &off(int64_t o)
        {
            offset = o;
            return *this;
        }
    };

    struct Node
    {
        std::vector<std::unique_ptr<Node> > kids;
        int64_t value;
        Addr addr;
        OpType type;
        std::string name;
        std::string strval;

        explicit Node(const OpType t) : value(0), type(t) {}
    };

    class Backend
    {
    public:
        virtual ~Backend() = default;

        virtual void gen(Node *n) = 0;

        virtual std::string generate(Node *n) = 0;
    };
}

#include <cdgnx/x86_64.hpp>
#include <ranges>

namespace cdgnx::backend
{
    std::string x86_64::new_label()
    {
        return ".L" + std::to_string(label_counter++);
    }

    void x86_64::emit(const std::string &s, bool indent)
    {
        if (indent && s.back() != ':')
            out << "    ";
        out << s << '\n';
    }

    std::string x86_64::format_addr(const Addr &a)
    {
        std::string result;
        if (a.offset)
            result += std::to_string(a.offset);
        if (!a.base.empty())
            result += "(" + a.base + ")";
        if (!a.index.empty())
            result += "," + a.index + "," + std::to_string(a.scale);
        return result;
    }

    void x86_64::gen_strings()
    {
        if (strs.empty())
            return;

        emit(".section .rodata", false);
        emit(".align 8", false);
        for (size_t i = 0; i < strs.size(); ++i)
        {
            emit(".LC" + std::to_string(i) + ":", false);
            emit(".string \"" + strs[i] + "\"", false);
        }
    }

    std::string x86_64::generate(Node *n)
    {
        out.str("");
        strs.clear();

        emit(".section .text", false);
        emit(".align 16", false);
        if (!n->name.empty())
        {
            emit(".global " + n->name, false);
            emit(".type " + n->name + ", @function", false);
            emit(n->name + ":", false);
            emit("pushq %rbp");
            emit("movq %rsp, %rbp");
        }

        gen(n);
        if (!n->name.empty())
        {
            emit("movq %rbp, %rsp");
            emit("popq %rbp");
            emit("ret");
        }
        gen_strings();
        return out.str();
    }

    void x86_64::gen(Node *n)
    {
        if (!n)
            return;

        switch (n->type)
        {
            case OpType::NUM:
            {
                emit("movq $" + std::to_string(n->value) + ", %rax");
                break;
            }

            case OpType::STR:
            {
                strs.push_back(n->strval);
                emit("leaq .LC" + std::to_string(strs.size() - 1) + "(%rip), %rax");
                break;
            }

            case OpType::IADD:
            {
                gen(n->kids[0].get());
                gen(n->kids[1].get());
                emit("popq %rcx");
                emit("popq %rax");
                emit("addq %rcx, %rax");
                emit("pushq %rax");
                break;
            }

            case OpType::ISUB:
            {
                gen(n->kids[0].get());
                gen(n->kids[1].get());
                emit("popq %rcx");
                emit("popq %rax");
                emit("subq %rcx, %rax");
                emit("pushq %rax");
                break;
            }

            case OpType::IMUL:
            {
                gen(n->kids[0].get());
                gen(n->kids[1].get());
                emit("popq %rcx");
                emit("popq %rax");
                emit("imulq %rcx, %rax");
                emit("pushq %rax");
                break;
            }

            case OpType::IDIV:
            {
                gen(n->kids[0].get());
                gen(n->kids[1].get());
                emit("popq %rcx"); /* divisor */
                emit("popq %rax"); /* dividend */
                emit("xorq %rdx, %rdx");
                emit("idivq %rcx");
                emit("pushq %rax");
                break;
            }

            case OpType::IMOD:
            {
                gen(n->kids[0].get());
                gen(n->kids[1].get());

                /* similar to IDIV... */
                emit("popq %rcx");
                emit("popq %rax");
                emit("xorq %rdx, %rdx");
                emit("idivq %rcx");
                emit("pushq %rdx");
                break;
            }

            case OpType::LABEL:
            {
                emit(n->name + ":", false);
                break;
            }

            case OpType::ROOT:
            {
                for (const auto &kid: n->kids)
                {
                    gen(kid.get());
                }
                break;
            }

            case OpType::CALL:
            {
                for (auto &kid: std::ranges::reverse_view(n->kids))
                    gen(kid.get());

                emit("call " + n->name);
                if (!n->kids.empty())
                    emit("addq $" + std::to_string(8 * n->kids.size()) + ", %rsp");

                emit("pushq %rax");
                break;
            }

            case OpType::RET:
            {
                if (!n->kids.empty())
                {
                    gen(n->kids[0].get());
                    emit("popq %rax");
                }
                emit("ret");
                break;
            }

            case OpType::PUSH:
            {
                gen(n->kids[0].get());
                break;
            }

            case OpType::POP:
            {
                emit("popq %rax");
                break;
            }

            case OpType::LEA:
            {
                emit("leaq " + format_addr(n->addr) + ", %rax");
                emit("pushq %rax");
                break;
            }

            case OpType::LOAD:
            {
                gen(n->kids[0].get());
                emit("popq %rax");
                emit("movq (%rax), %rax");
                emit("pushq %rax");
                break;
            }

            case OpType::STORE:
            {
                gen(n->kids[0].get()); /* addr */
                gen(n->kids[1].get()); /* value */
                emit("popq %rcx");     /* value */
                emit("popq %rax");     /* addr */
                emit("movq %rcx, (%rax)");
                break;
            }

            case OpType::JMP:
            {
                emit("jmp " + n->name);
                break;
            }

            case OpType::ICMP:
            {
                gen(n->kids[0].get());
                gen(n->kids[1].get());
                emit("popq %rcx");
                emit("popq %rax");
                emit("cmpq %rcx, %rax");
                break;
            }

            case OpType::JE:
            {
                emit("je " + n->name);
                break;
            }

            case OpType::JNE:
            {
                emit("jne " + n->name);
                break;
            }

            case OpType::JL:
            {
                emit("jl " + n->name);
                break;
            }

            case OpType::JLE:
            {
                emit("jle " + n->name);
                break;
            }

            case OpType::JG:
            {
                emit("jg " + n->name);
                break;
            }

            case OpType::JGE:
            {
                emit("jge " + n->name);
                break;
            }

            case OpType::BOR:
            {
                gen(n->kids[0].get());
                gen(n->kids[1].get());
                emit("popq %rcx");
                emit("popq %rax");
                emit("orq %rcx, %rax");
                emit("pushq %rax");
                break;
            }

            case OpType::BAND:
            {
                gen(n->kids[0].get());
                gen(n->kids[1].get());
                emit("popq %rcx");
                emit("popq %rax");
                emit("andq %rcx, %rax");
                emit("pushq %rax");
                break;
            }

            case OpType::BXOR:
            {
                gen(n->kids[0].get());
                gen(n->kids[1].get());
                emit("popq %rcx");
                emit("popq %rax");
                emit("xorq %rcx, %rax");
                emit("pushq %rax");
                break;
            }

            case OpType::BNOT:
            {
                gen(n->kids[0].get());
                emit("popq %rax");
                emit("notq %rax");
                emit("pushq %rax");
                break;
            }

            case OpType::BSHL:
            {
                gen(n->kids[0].get());
                gen(n->kids[1].get());
                emit("popq %rcx"); /* shift amount */
                emit("popq %rax"); /* val to shift */
                emit("shlq %cl, %rax");
                emit("pushq %rax");
                break;
            }

            case OpType::BSHR:
            {
                gen(n->kids[0].get());
                gen(n->kids[1].get());
                emit("popq %rcx"); /* shift amount */
                emit("popq %rax"); /* val to shift */
                emit("shrq %cl, %rax");
                emit("pushq %rax");
                break;
            }

            case OpType::MOV:
            {
                gen(n->kids[1].get());
                emit("popq %rax");
                emit("movq %rax, " + format_addr(n->kids[0].get()->addr));
                break;
            }

            case OpType::FADD:
            {
                gen(n->kids[0].get());
                gen(n->kids[1].get());
                emit("movsd (%rsp), %xmm0");
                emit("movsd 8(%rsp), %xmm1");
                emit("addsd %xmm1, %xmm0");
                emit("addq $16, %rsp");
                emit("sub $8, %rsp");
                emit("movsd %xmm0, (%rsp)");
                break;
            }

            case OpType::FSUB:
            {
                gen(n->kids[0].get());
                gen(n->kids[1].get());
                emit("movsd (%rsp), %xmm0");
                emit("movsd 8(%rsp), %xmm1");
                emit("subsd %xmm0, %xmm1");
                emit("addq $16, %rsp");
                emit("sub $8, %rsp");
                emit("movsd %xmm1, (%rsp)");
                break;
            }

            case OpType::FDIV:
            {
                gen(n->kids[0].get());
                gen(n->kids[1].get());
                emit("movsd (%rsp), %xmm0");
                emit("movsd 8(%rsp), %xmm1");
                emit("divsd %xmm0, %xmm1");
                emit("addq $16, %rsp");
                emit("sub $8, %rsp");
                emit("movsd %xmm1, (%rsp)");
                break;
            }

            case OpType::FMOD:
            {
                /*
                 * x86_64 doesn't have a direct floating-point modulo
                 * so we'll use FPU for this
                 */
                gen(n->kids[0].get());
                gen(n->kids[1].get());
                emit("fldl 8(%rsp)"); /* load first value */
                emit("fldl (%rsp)"); /* load second val */
                emit("fprem"); /* partial rem */
                emit("fstp %st(1)"); /* store res, discard extra */
                emit("addq $16, %rsp");
                emit("sub $8, %rsp");
                emit("fstpl (%rsp)"); /* res to stack*/
                break;
            }

            case OpType::FCMP:
            {
                gen(n->kids[0].get());
                gen(n->kids[1].get());
                emit("movsd (%rsp), %xmm0");
                emit("movsd 8(%rsp), %xmm1");
                emit("ucomisd %xmm1, %xmm0");
                emit("addq $16, %rsp");
                break;
            }

            case OpType::TEST:
            {
                gen(n->kids[0].get());
                gen(n->kids[1].get());
                emit("popq %rcx");
                emit("popq %rax");
                emit("testq %rcx, %rax");
                break;
            }

            default:
            {
                emit("nop");
                break;
            }
        }
    }
}

#pragma once

#include <sstream>
#include <vector>
#include <cdgnx/cdgnx.hpp>

namespace cdgnx::backend
{
    class x86_64 final : public Backend
    {
    public:
        void gen(Node *n) override;

        std::string generate(Node *n) override;

    private:
        std::stringstream out;
        std::vector<std::string> strs;
        uint32_t label_counter = 0;

        std::string new_label();

        void emit(const std::string &s, bool indent = true);

        static std::string format_addr(const Addr &a);

        void gen_strings();
    };
}
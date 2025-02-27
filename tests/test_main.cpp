#include <cassert>
#include <exception>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <cdgnx/cdgnx.hpp>
#include <cdgnx/x86_64.hpp>

class TestSuite
{
public:
    void add_test(std::string_view name, std::function<std::unique_ptr<cdgnx::Node>()> build_ir
        , std::function<bool(const std::string&)> validate)
    {
        test_cases.emplace_back(TestCase{ name, build_ir, validate });
    }

    bool run()
    {
        uint32_t passed = 0;
        int total = test_cases.size();
        std::cout << "Running " << total << " tests...\n";
        for (const auto& test: test_cases)
        {
            std::cout << "Test: " << test.name.data() << "... ";

            try
            {
                auto ast = test.ir_graph();
                if (!ast)
                {
                    std::cout << "FAILED (AST creation failed)\n";
                    continue;
                }

                cdgnx::backend::x86_64 backend;
                std::string asm_code = backend.generate(ast.get());

                /* stdout  */
                std::string filename = "test_";
                filename.append(test.name);
                filename.append(".asm");
                std::ofstream outfile(filename);
                outfile << asm_code;
                outfile.close();

                bool valid = test.validate(asm_code); /* check asm code */
                if (valid)
                {
                    std::cout << "PASSED\n";
                    passed++;
                }
                else
                {
                    std::cout << "FAILED (validation failed)\n";
                    std::cout << "  See " << filename << " for details\n";
                }
            }
            catch(const std::exception& e)
            {
                std::cout << "FAILED (exception: " << e.what() << ")\n";
            }
        }

        std::cout << "\nTest results: " << passed << "/" << total << " tests passed\n";
        return passed == total;
    }

private:
    struct TestCase 
    {
        std::string_view name;
        std::function<std::unique_ptr<cdgnx::Node>()> ir_graph;
        std::function<bool(const std::string&)> validate;
    };

    std::vector<TestCase> test_cases;
};

std::unique_ptr<cdgnx::Node> make_node(cdgnx::OpType type)
{
    return std::make_unique<cdgnx::Node>(type);
}

int main()
{
    TestSuite suite;
    
    suite.add_test(
        "arithmetic_math", 
        []() -> std::unique_ptr<cdgnx::Node>
        {
            auto root = make_node(cdgnx::OpType::ROOT);
            
            auto num1 = make_node(cdgnx::OpType::NUM);
            num1->value = 42;
            
            auto num2 = make_node(cdgnx::OpType::NUM);
            num2->value = 13;
            
            auto num3 = make_node(cdgnx::OpType::NUM);
            num3->value = 5;
            
            auto add = make_node(cdgnx::OpType::IADD);
            add->kids.push_back(std::move(num1));
            add->kids.push_back(std::move(num2));
            
            auto sub = make_node(cdgnx::OpType::ISUB);
            sub->kids.push_back(std::move(add));
            sub->kids.push_back(std::move(num3));
            
            root->kids.push_back(std::move(sub));
            
            return root;
        },
        [](const std::string& asm_code) -> bool 
        {
            return asm_code.find("movq $42, %rax") != std::string::npos &&
                   asm_code.find("pushq %rax") != std::string::npos &&
                   asm_code.find("movq $13, %rax") != std::string::npos &&
                   asm_code.find("addq %rcx, %rax") != std::string::npos &&
                   asm_code.find("movq $5, %rax") != std::string::npos &&
                   asm_code.find("subq %rcx, %rax") != std::string::npos &&
                   asm_code.find("popq %rax") != std::string::npos;
        }
    );
    
    suite.add_test(
        "bitwise_ops",
        []() -> std::unique_ptr<cdgnx::Node>
        {
            auto root = make_node(cdgnx::OpType::ROOT);
            
            auto num1 = make_node(cdgnx::OpType::NUM);
            num1->value = 0x5A;
            
            auto num2 = make_node(cdgnx::OpType::NUM);
            num2->value = 0x3F;
            
            auto num3 = make_node(cdgnx::OpType::NUM);
            num3->value = 2;
            
            auto band = make_node(cdgnx::OpType::BAND);
            band->kids.push_back(std::move(num1));
            band->kids.push_back(std::move(num2));
            
            auto shl = make_node(cdgnx::OpType::BSHL);
            shl->kids.push_back(std::move(band));
            shl->kids.push_back(std::move(num3));
            
            auto bnot = make_node(cdgnx::OpType::BNOT);
            bnot->kids.push_back(std::move(shl));
            
            root->kids.push_back(std::move(bnot));
            return root;
        },
        [](const std::string& asm_code) -> bool
        {
            return asm_code.find("movq $90, %rax") != std::string::npos &&  // 0x5A = 90
                   asm_code.find("movq $63, %rax") != std::string::npos &&  // 0x3F = 63
                   asm_code.find("andq %rcx, %rax") != std::string::npos &&
                   asm_code.find("movq $2, %rax") != std::string::npos &&
                   asm_code.find("shlq %cl, %rax") != std::string::npos &&
                   asm_code.find("notq %rax") != std::string::npos;
        }
    );
    
    suite.add_test(
        "memory_ops",
        []() -> std::unique_ptr<cdgnx::Node>
        {
            auto root = make_node(cdgnx::OpType::ROOT);
            
            /* base addr value  */
            auto addr_val = make_node(cdgnx::OpType::NUM);
            addr_val->value = 1000;
            
            auto store_val = make_node(cdgnx::OpType::NUM);
            store_val->value = 42;
            
            auto store = make_node(cdgnx::OpType::STORE);
            store->kids.push_back(std::move(addr_val));
            store->kids.push_back(std::move(store_val));
            
            auto addr_val2 = make_node(cdgnx::OpType::NUM);
            addr_val2->value = 1000;
            
            auto load = make_node(cdgnx::OpType::LOAD);
            load->kids.push_back(std::move(addr_val2));
            
            root->kids.push_back(std::move(store));
            root->kids.push_back(std::move(load));
            
            auto pop = make_node(cdgnx::OpType::POP);
            root->kids.push_back(std::move(pop));
            
            return root;
        },
        [](const std::string& asm_code) -> bool
        {
            return asm_code.find("movq $1000, %rax") != std::string::npos &&
                   asm_code.find("movq $42, %rax") != std::string::npos &&
                   asm_code.find("movq %rcx, (%rax)") != std::string::npos &&
                   asm_code.find("movq (%rax), %rax") != std::string::npos;
        }
    );

    suite.add_test(
        "string_ops",
        []() -> std::unique_ptr<cdgnx::Node>
        {
            auto root = make_node(cdgnx::OpType::ROOT);
            auto str1 = make_node(cdgnx::OpType::STR);
            str1->strval = "Hello, world!";
            
            auto str2 = make_node(cdgnx::OpType::STR);
            str2->strval = "Test string";
            
            root->kids.push_back(std::move(str1));
            root->kids.push_back(std::move(str2));
            
            auto pop1 = make_node(cdgnx::OpType::POP);
            auto pop2 = make_node(cdgnx::OpType::POP);
            
            root->kids.push_back(std::move(pop1));
            root->kids.push_back(std::move(pop2));
            
            return root;
        },
        [](const std::string& asm_code) -> bool
        {
            return asm_code.find(".section .rodata") != std::string::npos &&
                   asm_code.find(".string \"Hello, world!\"") != std::string::npos &&
                   asm_code.find(".string \"Test string\"") != std::string::npos &&
                   asm_code.find("leaq .LC") != std::string::npos;
        }
    );
    
    // Run all tests
    return suite.run() ? 0 : 1;
}
# cdgnx

A minimal code generation library featuring lightweight, high level IR, x86-64 backend with simple extensible design and zero dependency.

> [!!INFORMATION]
> This project will not get continued as it is made for learning purposes

## Usage

```cpp
#include <cdgnx/core.hpp>
#include <cdgnx/x86_64.hpp>

int main()
{
    cdgnx::Node root(cdgnx::OpType::ROOT);
    cdgnx::backend::x86_64 backend;

    std::string asm = backend.asm(&root);
}
```

## Building

### CMake

```bash
mkdir build && cd build
cmake ..
make
```

## License

MIT. See [LICENSE](LICENSE.txt) for more info.
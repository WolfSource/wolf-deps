#include <iostream>

#include <msquic.h>

int main()
{
    static_assert(QUIC_API_VERSION_2 == 2);
    std::cout << "msquic version 2." << std::endl;
    return 0;
}

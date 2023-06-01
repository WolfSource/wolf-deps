#include <msquic.h>

int main()
{
    static_assert(QUIC_API_VERSION_2 == 2);

    const QUIC_API_TABLE* api_table;
    if (QUIC_FAILED(MsQuicOpen2(&api_table))) {
        return 1;
    }
    MsQuicClose(api_table);

    return 0;
}

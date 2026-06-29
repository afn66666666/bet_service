#include "AsyncUserService.h"

static constexpr int NUM_THREADS = 16;

int main()
{
    AsyncUserService service(
        "0.0.0.0:50051",
        "host=host.docker.internal port=5432 dbname=betting_db user=betting_admin password=kiba",
        NUM_THREADS);

    service.run();
}

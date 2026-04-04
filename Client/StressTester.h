

#include <iostream>
#include <memory>

class StressTester
{
public:
    StressTester()
    {
    }
    void testUserService();
    void testFillUsers();
    void fillUpBalances();
    std::pair<std::string,std::string> generateLoginData(int chance);
private:
    void worker();

    std::atomic<int> success{0};
    std::atomic<int> failed{0};
};

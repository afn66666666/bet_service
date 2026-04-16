

#include <iostream>
#include <memory>

class StressTester
{
public:
    StressTester()
    {
    }
    void testUserService();
    std::pair<std::string,std::string> generateLoginData(int chance) const;
private:
    void worker();
    std::pair<std::string,std::string> getRandomData() const;
    std::atomic<int> success{0};
    std::atomic<int> failed{0};
};

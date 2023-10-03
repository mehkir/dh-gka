#include <iostream>
#include <boost/asio.hpp>
#include <unordered_map>
#include <vector>
#include <memory>

int main () {
    boost::asio::ip::udp::endpoint ep1(boost::asio::ip::address::from_string("1.2.3.4"), 55);
    boost::asio::ip::udp::endpoint ep2(boost::asio::ip::address::from_string("1.2.3.4"), 56);
    boost::asio::ip::udp::endpoint ep3(boost::asio::ip::address::from_string("1.2.3.5"), 56);
    std::unordered_map<boost::asio::ip::udp::endpoint, int> umap;
    umap[ep1] = 1;
    umap[ep2] = 2;

    for (auto ep : umap) {
        std::cout << ep.first << ", " << ep.second << std::endl;
    }

    std::unique_ptr<int> my_int;
    if(!my_int) {
        std::cout << "null" << std::endl;
    } else {
        std::cout << "not null" << std::endl;
    }

    if (umap.contains(ep3)) {
        std::cout << "contains" << std::endl;
    }


    char bytes[4] = {'A','B','C', 'D'};

    std::vector<char> copybytes(bytes,bytes+4);
    for (char c : copybytes) {
        std::cout << c << std::endl;
    }

    return 0;
}
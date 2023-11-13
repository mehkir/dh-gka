#include "logger.hpp"
#include "multicast_application_impl.hpp"
#include "key_agreement_protocol.hpp"
#include "primitives.hpp"
#include "message_handler.hpp"

class multicast_app_testframe : public key_agreement_protocol, public multicast_application_impl {
    public:
        multicast_app_testframe(bool _is_sponsor) : is_sponsor_(_is_sponsor), multicast_application_impl(boost::asio::ip::address::from_string("127.0.0.1"), boost::asio::ip::address::from_string("239.255.0.1"), 65000), message_handler_(std::make_unique<message_handler>(this)), request_counter_(0) {
            if (is_sponsor_) {
                std::unique_ptr<offer_message> offer = std::make_unique<offer_message>();
                offer->offered_service_ = 0;
                send(offer.operator*());
            }
        }

        ~multicast_app_testframe() {    
        }

        void start() {
            multicast_application_impl::start();
        }

        void received_data(unsigned char* _data, size_t _bytes_recvd, boost::asio::ip::udp::endpoint _remote_endpoint) override {
            std::lock_guard<std::mutex> lock_receive(receive_mutex_);
            if (get_local_endpoint().port() != _remote_endpoint.port()) {
                message_handler_->deserialize_and_callback(_data, _bytes_recvd, _remote_endpoint);
            }
        }

        void process_find(find_message _rcvd_find_message, boost::asio::ip::udp::endpoint _remote_endpoint) override {

        }

        void process_offer(offer_message _rcvd_offer_message, boost::asio::ip::udp::endpoint _remote_endpoint) override {
            std::unique_ptr<request_message> request = std::make_unique<request_message>();
            request->required_service_ = 0;
            send(request.operator*());
        }

        void process_request(request_message _rcvd_request_message, boost::asio::ip::udp::endpoint _remote_endpoint) override {
            if (is_sponsor_) {
                request_counter_++;
                LOG_DEBUG("[<multicast_app_testframe>]: rcvd requests=" << request_counter_)
            }
        }

        void process_response(response_message _rcvd_response_message, boost::asio::ip::udp::endpoint _remote_endpoint) override {

        }

        void process_member_info_request(member_info_request_message _rcvd_member_info_request_message, boost::asio::ip::udp::endpoint _remote_endpoint) override {

        }

        void process_member_info_response(member_info_response_message _rcvd_member_info_response_message, boost::asio::ip::udp::endpoint _remote_endpoint) override {

        }

        void process_synch_token(synch_token_message _rcvd_synch_token_message, boost::asio::ip::udp::endpoint _remote_endpoint) override {

        }

        void process_member_info_synch_request(member_info_synch_request_message _rcvd_member_info_synch_request_message, boost::asio::ip::udp::endpoint _remote_endpoint) override {

        }

        void process_member_info_synch_response(member_info_synch_response_message _rcvd_member_info_synch_response_message, boost::asio::ip::udp::endpoint _remote_endpoint) override {

        }

        void process_distributed_response(distributed_response_message _rcvd_distributed_response_message, boost::asio::ip::udp::endpoint _remote_endpoint) override {

        }

        void process_finish(finish_message _rcvd_finish_message, boost::asio::ip::udp::endpoint _remote_endpoint) override {

        }

        void process_finish_ack(finish_ack_message _rcvd_finish_ack_message, boost::asio::ip::udp::endpoint _remote_endpoint) override {

        }
    protected:
    private:
        bool is_sponsor_;
        uint32_t request_counter_;
        std::unique_ptr<message_handler> message_handler_;
        std::mutex receive_mutex_;

        void send(message& _message) {
            boost::asio::streambuf buffer;
            message_handler_->serialize(_message, buffer);
            multicast_application_impl::send_multicast(buffer);
        }

};

int main (int argc, char* argv[]) {
    std::string is_sponsor(argv[1]);

    if (!boost::iequals(is_sponsor, "true") && !boost::iequals(is_sponsor, "false")) {
      std::cerr << "is_sponsor must be \"true\" or \"false\" (case-insensitive)\n";
      return 1;
    }

    multicast_app_testframe app(boost::iequals(is_sponsor, "true"));
    app.start();
    return 0;
}

// /usr/sbin/cmake --build /root/c++-multicast/build --config Release --target all -j 14 --
// for i in $(seq 1 999); do /root/c++-multicast/build/multicast-app-testframe false & done
// /root/c++-multicast/build/multicast-app-testframe true
// pkill multicast-app-t
// watch "ss -lup | grep multicast-app-t | awk '{print $4}' |  awk -F: '{print $NF}' | grep -v 65000 | sort -u | wc -l"
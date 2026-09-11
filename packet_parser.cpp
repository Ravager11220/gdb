#include <iostream>
#include <cstring>
#include <vector>
#include <cstdint> // Added missing header for uint32_t / uint16_t

struct Packet {
    uint32_t id;
    uint16_t length;
    char* payload;
};

class NetworkBuffer {
private:
    Packet* packets;
    size_t count;

public:
    NetworkBuffer(size_t num_packets) : count(num_packets) {
        packets = new Packet[count];
        for (size_t i = 0; i < count; ++i) {
            packets[i].id = 1000 + i;
            packets[i].length = 16;
            packets[i].payload = new char[packets[i].length];
            snprintf(packets[i].payload, packets[i].length, "PAYLOAD_DATA_%02zu", i);
        }
    }

    ~NetworkBuffer() {
        for (size_t i = 0; i < count; ++i) {
            delete[] packets[i].payload;
        }
        delete[] packets;
    }

    Packet* get_packets() { return packets; }
    size_t get_count() const { return count; }
};

void process_network_data(NetworkBuffer buffer) {
    std::cout << "[+] Processing " << buffer.get_count() << " network packets...\n";
    Packet* pkt_list = buffer.get_packets();

    for (size_t i = 0; i < buffer.get_count(); ++i) {
        std::cout << "  -> Packet ID: " << pkt_list[i].id 
                  << " | Data: " << pkt_list[i].payload << "\n";
    }
}

int main() {
    std::cout << "Initializing Network Interface...\n";

    NetworkBuffer net_buf(3);

    // Pass buffer to processing engine
    process_network_data(net_buf);

    // Try doing a second operation on the buffer
    std::cout << "\n[+] Re-verifying Packet 0 ID: " << net_buf.get_packets()[0].id << "\n";
    std::cout << "[+] Re-verifying Packet 0 Payload: " << net_buf.get_packets()[0].payload << "\n";

    std::cout << "Shutting down interface safely.\n";
    return 0;
}


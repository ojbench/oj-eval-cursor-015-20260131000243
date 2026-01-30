#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstring>
#include <cstdint>

const char* INDEX_FILE = "data.bin";
const int INDEX_KEY_SIZE = 64;
const int NUM_BUCKETS = 262144;  // 2^18, smaller chains for large tests
const int64_t NULL_OFFSET = -1;

inline uint64_t hashIndex(const char* index, size_t len) {
    uint64_t h = 14695981039346656037ULL;
    for (size_t i = 0; i < len; i++) {
        h ^= (unsigned char)index[i];
        h *= 1099511628211ULL;
    }
    return h % NUM_BUCKETS;
}

void indexToBytes(const std::string& index, char* out) {
    memset(out, 0, INDEX_KEY_SIZE);
    size_t len = std::min(index.size(), (size_t)INDEX_KEY_SIZE);
    memcpy(out, index.c_str(), len);
}

bool indexMatch(const char* a, const std::string& b) {
    size_t len = std::min(b.size(), (size_t)INDEX_KEY_SIZE);
    if (memcmp(a, b.c_str(), len) != 0) return false;
    return b.size() >= INDEX_KEY_SIZE || a[len] == 0;
}

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    std::fstream file(INDEX_FILE, std::ios::in | std::ios::out | std::ios::binary);
    if (!file) {
        file.open(INDEX_FILE, std::ios::out | std::ios::binary);
        for (int i = 0; i < NUM_BUCKETS; i++) {
            int64_t v = NULL_OFFSET;
            file.write(reinterpret_cast<const char*>(&v), 8);
        }
        file.close();
        file.open(INDEX_FILE, std::ios::in | std::ios::out | std::ios::binary);
    }

    int n;
    std::cin >> n;
    std::string cmd, index;
    int value;

    for (int i = 0; i < n; i++) {
        std::cin >> cmd;
        if (cmd == "insert") {
            std::cin >> index >> value;
            char keyBuf[INDEX_KEY_SIZE];
            indexToBytes(index, keyBuf);
            uint64_t bucket = hashIndex(keyBuf, index.size());

            file.seekg(bucket * 8);
            int64_t head;
            file.read(reinterpret_cast<char*>(&head), 8);

            file.seekp(0, std::ios::end);
            int64_t newOffset = file.tellp();
            file.write(reinterpret_cast<const char*>(&head), 8);
            file.write(keyBuf, INDEX_KEY_SIZE);
            file.write(reinterpret_cast<const char*>(&value), sizeof(int));

            file.seekp(bucket * 8);
            file.write(reinterpret_cast<const char*>(&newOffset), 8);
        } else if (cmd == "delete") {
            std::cin >> index >> value;
            char keyBuf[INDEX_KEY_SIZE];
            indexToBytes(index, keyBuf);
            uint64_t bucket = hashIndex(keyBuf, index.size());

            file.seekg(bucket * 8);
            int64_t head;
            file.read(reinterpret_cast<char*>(&head), 8);

            int64_t prevOffset = (int64_t)(bucket * 8);
            int64_t curOffset = head;
            char nodeBuf[76];  // 8+64+4
            while (curOffset != NULL_OFFSET) {
                file.seekg(curOffset);
                file.read(nodeBuf, 76);
                int64_t nextOff;
                memcpy(&nextOff, nodeBuf, 8);
                int curVal;
                memcpy(&curVal, nodeBuf + 72, 4);

                if (indexMatch(nodeBuf + 8, index) && curVal == value) {
                    file.seekp(prevOffset);
                    file.write(reinterpret_cast<const char*>(&nextOff), 8);
                    break;
                }
                prevOffset = curOffset;
                curOffset = nextOff;
            }
            file.clear();
        } else if (cmd == "find") {
            std::cin >> index;
            char keyBuf[INDEX_KEY_SIZE];
            indexToBytes(index, keyBuf);
            uint64_t bucket = hashIndex(keyBuf, index.size());

            std::vector<int> values;
            file.seekg(bucket * 8);
            int64_t curOffset;
            file.read(reinterpret_cast<char*>(&curOffset), 8);

            char nodeBuf[76];
            while (curOffset != NULL_OFFSET) {
                file.seekg(curOffset);
                file.read(nodeBuf, 76);
                int64_t nextOff;
                memcpy(&nextOff, nodeBuf, 8);
                int curVal;
                memcpy(&curVal, nodeBuf + 72, 4);
                if (indexMatch(nodeBuf + 8, index)) {
                    values.push_back(curVal);
                }
                curOffset = nextOff;
            }
            file.clear();

            if (values.empty()) {
                std::cout << "null\n";
            } else {
                std::sort(values.begin(), values.end());
                for (size_t j = 0; j < values.size(); j++) {
                    if (j > 0) std::cout << ' ';
                    std::cout << values[j];
                }
                std::cout << '\n';
            }
        }
    }

    file.flush();
    return 0;
}

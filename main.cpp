#include <iostream>
#include <cstdio>
#include <string>
#include <vector>
#include <algorithm>
#include <cstring>
#include <cstdint>

const char* INDEX_FILE = "data.bin";
const int INDEX_KEY_SIZE = 64;
const int NUM_BUCKETS = 262144;
const int64_t NULL_OFFSET = -1;
const int NODE_SIZE = 8 + INDEX_KEY_SIZE + sizeof(int);
const int BUF_SIZE = 1024 * 1024;  // 1MB buffer for file I/O

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

    static char fileBuf[BUF_SIZE];
    FILE* f = fopen(INDEX_FILE, "r+b");
    if (!f) {
        f = fopen(INDEX_FILE, "wb");
        for (int i = 0; i < NUM_BUCKETS; i++) {
            int64_t v = NULL_OFFSET;
            fwrite(&v, 8, 1, f);
        }
        fclose(f);
        f = fopen(INDEX_FILE, "r+b");
    }
    setvbuf(f, fileBuf, _IOFBF, BUF_SIZE);

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

            int64_t head;
            fseek(f, (long)(bucket * 8), SEEK_SET);
            fread(&head, 8, 1, f);

            fseek(f, 0, SEEK_END);
            int64_t newOffset = ftell(f);
            fwrite(&head, 8, 1, f);
            fwrite(keyBuf, INDEX_KEY_SIZE, 1, f);
            fwrite(&value, sizeof(int), 1, f);

            fseek(f, (long)(bucket * 8), SEEK_SET);
            fwrite(&newOffset, 8, 1, f);
        } else if (cmd == "delete") {
            std::cin >> index >> value;
            char keyBuf[INDEX_KEY_SIZE];
            indexToBytes(index, keyBuf);
            uint64_t bucket = hashIndex(keyBuf, index.size());

            int64_t head;
            fseek(f, (long)(bucket * 8), SEEK_SET);
            fread(&head, 8, 1, f);

            int64_t prevOffset = (int64_t)(bucket * 8);
            int64_t curOffset = head;
            while (curOffset != NULL_OFFSET) {
                fseek(f, (long)curOffset, SEEK_SET);
                int64_t nextOff;
                char curKey[INDEX_KEY_SIZE];
                int curVal;
                fread(&nextOff, 8, 1, f);
                fread(curKey, INDEX_KEY_SIZE, 1, f);
                fread(&curVal, sizeof(int), 1, f);

                if (indexMatch(curKey, index) && curVal == value) {
                    fseek(f, (long)prevOffset, SEEK_SET);
                    fwrite(&nextOff, 8, 1, f);
                    break;
                }
                prevOffset = curOffset;
                curOffset = nextOff;
            }
        } else if (cmd == "find") {
            std::cin >> index;
            char keyBuf[INDEX_KEY_SIZE];
            indexToBytes(index, keyBuf);
            uint64_t bucket = hashIndex(keyBuf, index.size());

            std::vector<int> values;
            values.reserve(256);
            fseek(f, (long)(bucket * 8), SEEK_SET);
            int64_t curOffset;
            fread(&curOffset, 8, 1, f);

            while (curOffset != NULL_OFFSET) {
                fseek(f, (long)curOffset, SEEK_SET);
                int64_t nextOff;
                char curKey[INDEX_KEY_SIZE];
                int curVal;
                fread(&nextOff, 8, 1, f);
                fread(curKey, INDEX_KEY_SIZE, 1, f);
                fread(&curVal, sizeof(int), 1, f);

                if (indexMatch(curKey, index)) {
                    values.push_back(curVal);
                }
                curOffset = nextOff;
            }

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

    fflush(f);
    fclose(f);
    return 0;
}

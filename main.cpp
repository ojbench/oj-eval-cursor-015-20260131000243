#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstring>

const char* DATA_FILE = "data.bin";
const int INDEX_SIZE = 64;
const int RECORD_SIZE = 1 + INDEX_SIZE + sizeof(int);  // deleted + index + value

struct Record {
    bool deleted;
    char index[INDEX_SIZE];
    int value;
};

void writeRecord(std::fstream& file, const Record& rec) {
    file.write(reinterpret_cast<const char*>(&rec.deleted), 1);
    file.write(rec.index, INDEX_SIZE);
    file.write(reinterpret_cast<const char*>(&rec.value), sizeof(int));
}

void readRecord(std::fstream& file, Record& rec) {
    file.read(reinterpret_cast<char*>(&rec.deleted), 1);
    file.read(rec.index, INDEX_SIZE);
    file.read(reinterpret_cast<char*>(&rec.value), sizeof(int));
}

void indexToBytes(const std::string& index, char* out) {
    memset(out, 0, INDEX_SIZE);
    size_t len = std::min(index.size(), (size_t)INDEX_SIZE);
    memcpy(out, index.c_str(), len);
}

bool indexMatch(const char* a, const std::string& b) {
    size_t len = std::min(b.size(), (size_t)INDEX_SIZE);
    return memcmp(a, b.c_str(), len) == 0 && (b.size() >= INDEX_SIZE || a[len] == 0);
}

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    std::fstream file(DATA_FILE, std::ios::in | std::ios::out | std::ios::binary);
    if (!file) {
        file.open(DATA_FILE, std::ios::out | std::ios::binary);
        file.close();
        file.open(DATA_FILE, std::ios::in | std::ios::out | std::ios::binary);
    }

    int n;
    std::cin >> n;
    std::string cmd, index;
    int value;

    for (int i = 0; i < n; i++) {
        std::cin >> cmd;
        if (cmd == "insert") {
            std::cin >> index >> value;
            Record rec;
            rec.deleted = false;
            indexToBytes(index, rec.index);
            rec.value = value;
            file.seekp(0, std::ios::end);
            writeRecord(file, rec);
            file.flush();
        } else if (cmd == "delete") {
            std::cin >> index >> value;
            file.seekg(0, std::ios::beg);
            Record rec;
            while (file.peek() != EOF) {
                std::streampos pos = file.tellg();
                readRecord(file, rec);
                if (!rec.deleted && indexMatch(rec.index, index) && rec.value == value) {
                    rec.deleted = true;
                    file.seekp(pos);
                    writeRecord(file, rec);
                    file.flush();
                    break;
                }
            }
            file.clear();
        } else if (cmd == "find") {
            std::cin >> index;
            std::vector<int> values;
            file.seekg(0, std::ios::beg);
            Record rec;
            while (file.peek() != EOF) {
                readRecord(file, rec);
                if (!rec.deleted && indexMatch(rec.index, index)) {
                    values.push_back(rec.value);
                }
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

    return 0;
}

#include <bits/stdc++.h>
#include <corecrt.h>
#include <fstream>
#include <string.h>
using namespace std;

struct AllocInfo {
    string file;
    int line;
};

unordered_map<intptr_t, AllocInfo> allocs, glbuf, gltx, glva;
ifstream f;

AllocInfo GetAllocInfo() {
    AllocInfo ai;
    f >> ai.file >> ai.line;
    return ai;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: memtrace <filename.txt>\n");
        return 0;
    }

    f.open(argv[1]);

    while (true) {
        string cmd;
        f >> cmd;

        if (cmd == "M") {
            intptr_t ptr;
            f >> ptr;

            allocs.insert({ ptr, GetAllocInfo() });
        }
        else if (cmd == "R") {
            intptr_t newptr, oldptr;
            f >> newptr >> oldptr;
            
            allocs.erase(oldptr);
            allocs.insert({ newptr, GetAllocInfo() });
        }
        else if (cmd == "F") {
            intptr_t ptr;
            f >> ptr;

            allocs.erase(ptr);
            GetAllocInfo();
        }
        else if (cmd == "GB") {
            intptr_t ptr;
            f >> ptr;

            glbuf.insert({ ptr, GetAllocInfo() });
        }
        else if (cmd == "GT") {
            intptr_t ptr;
            f >> ptr;

            gltx.insert({ ptr, GetAllocInfo() });
        }
        else if (cmd == "GVA") {
            intptr_t ptr;
            f >> ptr;

            glva.insert({ ptr, GetAllocInfo() });
        }
        else if (cmd == "DB") {
            intptr_t ptr;
            f >> ptr;

            glbuf.erase(ptr);
            GetAllocInfo();
        }
        else if (cmd == "DT") {
            intptr_t ptr;
            f >> ptr;

            gltx.erase(ptr);
            GetAllocInfo();
        }
        else if (cmd == "DVA") {
            intptr_t ptr;
            f >> ptr;

            glva.erase(ptr);
            GetAllocInfo();
        }
        else break;
    }

    printf("%d ALLOCATIONS LEAKED\n", allocs.size());
    for (auto &i : allocs)
        printf("0x%X at %s:%d\n", i.first, i.second.file.c_str(), i.second.line);

    printf("%d GL BUFFERS LEAKED\n", glbuf.size());
    for (auto &i : glbuf)
        printf("%d at %s:%d\n", i.first, i.second.file.c_str(), i.second.line);

    printf("%d GL TEXTURES LEAKED\n", gltx.size());
    for (auto &i : gltx)
        printf("%d at %s:%d\n", i.first, i.second.file.c_str(), i.second.line);

    printf("%d GL VERTEX ARRAYS LEAKED\n", glva.size());
    for (auto &i : glva)
        printf("%d at %s:%d\n", i.first, i.second.file.c_str(), i.second.line);

    f.close();
}
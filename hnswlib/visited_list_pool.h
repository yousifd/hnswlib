#pragma once

#include <mutex>
#include <string.h>

namespace hnswlib {
    typedef unsigned short int vl_type;

    class VisitedList {
    public:
        vl_type curV;
        vl_type *mass;
        unsigned int numelements;
        void* buffer = nullptr;

        VisitedList(int numelements1, void* buffer=nullptr) : buffer(buffer) {
            curV = -1;
            numelements = numelements1;
            if (this->buffer != nullptr) {
                mass = new(this->buffer) vl_type[numelements];
                buffer  = static_cast<char*>(this->buffer) + sizeof(vl_type[numelements]);
            } else {
                mass = new vl_type[numelements];
            }
        }

        void reset() {
            curV++;
            if (curV == 0) {
                memset(mass, 0, sizeof(vl_type) * numelements);
                curV++;
            }
        };

        ~VisitedList() { delete[] mass; }
    };
///////////////////////////////////////////////////////////
//
// Class for multi-threaded pool-management of VisitedLists
//
/////////////////////////////////////////////////////////

    class VisitedListPool {
        std::deque<VisitedList *> pool;
        std::mutex poolguard;
        int numelements;
        void* buffer = nullptr;

    public:
        VisitedListPool(int initmaxpools, int numelements1, void* buffer=nullptr) : buffer(buffer) {
            numelements = numelements1;
            for (int i = 0; i < initmaxpools; i++)
                pool.push_front(new VisitedList(numelements));
        }

        VisitedList *getFreeVisitedList() {
            VisitedList *rez;
            {
                std::unique_lock <std::mutex> lock(poolguard);
                if (pool.size() > 0) {
                    rez = pool.front();
                    pool.pop_front();
                } else {
                    if (this->buffer != nullptr) {
                        rez = new(this->buffer) VisitedList(numelements, buffer);
                        this->buffer = static_cast<char*>(this->buffer) + sizeof(VisitedList);
                    } else {
                        rez = new VisitedList(numelements);
                    }
                }
            }
            rez->reset();
            return rez;
        };

        void releaseVisitedList(VisitedList *vl) {
            std::unique_lock <std::mutex> lock(poolguard);
            pool.push_front(vl);
        };

        ~VisitedListPool() {
            while (pool.size()) {
                VisitedList *rez = pool.front();
                pool.pop_front();
                delete rez;
            }
        };
    };
}


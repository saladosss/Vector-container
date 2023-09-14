#include "vector.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <algorithm>

namespace {

    //  число, используемое для отслеживания жизни объекта,
    inline const uint32_t DEFAULT_COOKIE = 0xdeadbeef;

    struct TestObj {
        TestObj() = default;
        TestObj(const TestObj& other) = default;
        TestObj& operator=(const TestObj& other) = default;
        TestObj(TestObj&& other) = default;
        TestObj& operator=(TestObj&& other) = default;
        ~TestObj() {
            cookie = 0;
        }
        [[nodiscard]] bool IsAlive() const noexcept {
            return cookie == DEFAULT_COOKIE;
        }
        uint32_t cookie = DEFAULT_COOKIE;
    };
}

int main() {

    size_t SIZE = 100500;
    {
        Vector<int> v;
        assert(v.Capacity() == 0);
        assert(v.Size() == 0);

        v.Reserve(SIZE);
        assert(v.Capacity() == SIZE);
        assert(v.Size() == 0);
    }
    {
        Vector<int> v(SIZE);
        const auto &cv(v);
        assert(v.Capacity() == SIZE);
        assert(v.Size() == SIZE);
        assert(v[0] == 0);
        assert(&v[0] == &cv[0]);

        v.Reserve(SIZE * 2);
        assert(v.Size() == SIZE);
        assert(v.Capacity() == SIZE * 2);
    }

    const size_t MEDIUM_SIZE = 100;
    const size_t LARGE_SIZE = 250;


    {
        Vector<int> v_medium(MEDIUM_SIZE);
        Vector<int> v_large(LARGE_SIZE);
        v_large = v_medium;
        assert(v_large.Size() == MEDIUM_SIZE);
        assert(v_large.Capacity() == LARGE_SIZE);
    }

    {
        Vector<double> v(MEDIUM_SIZE);
        Vector<double> v_small(MEDIUM_SIZE / 2);
        v_small.Reserve(MEDIUM_SIZE + 1);
        v_small = v;
        assert(v_small.Size() == v.Size());
        assert(v_small.Capacity() == MEDIUM_SIZE + 1);
    }
    {
        SIZE = 100'500;
        {
            Vector<double> v;
            v.Resize(SIZE);
            assert(v.Size() == SIZE);
            assert(v.Capacity() == SIZE);
        }
        {
            const size_t NEW_SIZE = 10'000;
            Vector<double> v(SIZE);
            v.Resize(NEW_SIZE);
            assert(v.Size() == NEW_SIZE);
            assert(v.Capacity() == SIZE);
        }
        {
            Vector<int> v(SIZE);
            v.PushBack(55);
            assert(v.Size() == SIZE + 1);
            assert(v.Capacity() == SIZE * 2);
        }
        {
            Vector<int> v(SIZE);
            v.PushBack(int('A'));
            assert(v.Size() == SIZE + 1);
            assert(v.Capacity() == SIZE * 2);
        }
        {
            Vector<std::vector<int>> v;
            v.PushBack(std::vector<int>{34, 122});
            v.PopBack();
            assert(v.Size() == 0);
            assert(v.Capacity() == 1);
        }
    }
    {
        using namespace std::literals;
        {
            Vector<std::string> v;
            auto &elem = v.EmplaceBack( "Ivan"s);
            assert(v.Capacity() == 1);
            assert(v.Size() == 1);
            assert(&elem == &v[0]);
            assert(v[0]== "Ivan"s);
        }
        {
            Vector<TestObj> v(1);
            assert(v.Size() == v.Capacity());
            v.EmplaceBack(v[0]);
            assert(v[0].IsAlive());
            assert(v[1].IsAlive());
        }
    }
    {
       SIZE = 10;
        {
            Vector<int> v(SIZE);
            const auto &cv(v);
            v.PushBack(1);
            assert(&*v.begin() == &v[0]);
            *v.begin() = 2;
            assert(v[0] == 2);
            assert(v.begin() == cv.begin());
            assert(v.end() == cv.end());
            assert(v.cbegin() == cv.begin());
            assert(v.cend() == cv.end());
        }
        {
            Vector<int> v{SIZE};
            Vector<int>::iterator pos = v.Insert(v.cbegin() + 1, 34);
            assert(v.Size() == SIZE + 1);
            assert(v.Capacity() == SIZE * 2);
            assert(&*pos == &v[1]);
        }
        {
            Vector<double> v;
            auto *pos = v.Emplace(v.end(), 1.5);
            assert(v.Size() == 1);
            assert(v.Capacity() >= v.Size());
            assert(&*pos == &v[0]);
        }
        {
            Vector<std::string> v;
            v.Reserve(SIZE);
            auto *pos = v.Emplace(v.end(), "Artem");
            assert(v.Size() == 1);
            assert(v.Capacity() >= v.Size());
            assert(&*pos == &v[0]);
        }
        {
            Vector<int> v{SIZE};
            Vector<int>::iterator pos = v.Insert(v.cbegin() + 1, 1);
            assert(v.Size() == SIZE + 1);
            assert(v.Capacity() == SIZE * 2);
            assert(&*pos == &v[1]);
        }
        {
            Vector<TestObj> v{SIZE};
            v.Insert(v.cbegin() + 2, v[0]);
            assert(std::all_of(v.begin(), v.end(), [](const TestObj &obj) {
                return obj.IsAlive();
            }));
        }
    }
}
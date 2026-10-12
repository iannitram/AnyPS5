#include "prx/libc/include/ApplicationHeap.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include <array>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <random>
#include <string>
#include <vector>

namespace {
struct Entry {
    char* key;
    void* data;
};
struct Node {
    void* key;
    Node* left;
    Node* right;
    signed char balance;
};
struct Queue {
    Queue* next;
    Queue* previous;
    int value;
};
using Compare = int (APS5_VABI *)(const void*, const void*);
using Walker = void (APS5_VABI *)(const Node*, int, int);
}

extern "C" {
int* APS5_VABI __error_nid_postfix();
int APS5_VABI hcreate_nid_postfix(std::size_t);
void APS5_VABI hdestroy_nid_postfix();
Entry* APS5_VABI hsearch_nid_postfix(Entry, int);
void* APS5_VABI lfind_nid_postfix(const void*, const void*, std::size_t*, std::size_t, Compare);
void* APS5_VABI lsearch_nid_postfix(const void*, void*, std::size_t*, std::size_t, Compare);
void APS5_VABI insque_nid_postfix(void*, void*);
void APS5_VABI remque_nid_postfix(void*);
Node* APS5_VABI tsearch_nid_postfix(const void*, Node**, Compare);
Node* APS5_VABI tfind_nid_postfix(const void*, Node* const*, Compare);
void* APS5_VABI tdelete_nid_postfix(const void*, Node**, Compare);
void APS5_VABI twalk_nid_postfix(const Node*, Walker);
}

namespace {

void Require(bool value, const char* what) {
    if (!value) {
        std::fprintf(stderr, "failed: %s\n", what);
        std::abort();
    }
}

int APS5_VABI CompareInt(const void* a, const void* b) {
    const int x = *static_cast<const int*>(a), y = *static_cast<const int*>(b);
    return x < y ? -1 : x > y ? 1 : 0;
}

int APS5_VABI CompareKey(const void* a, const void* b) {
    const auto x = reinterpret_cast<std::intptr_t>(a), y = reinterpret_cast<std::intptr_t>(b);
    return x < y ? -1 : x > y ? 1 : 0;
}

void CheckHash() {
    Require(hcreate_nid_postfix(4) == 1, "hcreate");
    std::vector<std::string> keys;
    for (int i = 0; i < 2000; i++) keys.push_back("key" + std::to_string(i));
    for (int i = 0; i < 2000; i++) {
        Entry* entry = hsearch_nid_postfix({keys[i].data(), reinterpret_cast<void*>(std::intptr_t(i))}, 1);
        Require(entry != nullptr && entry->key == keys[i].data(), "enter");
    }
    std::string copy = "key1234";
    Entry* found = hsearch_nid_postfix({copy.data(), nullptr}, 0);
    Require(found != nullptr && found->data == reinterpret_cast<void*>(std::intptr_t(1234)) && found->key == keys[1234].data(), "find after growth");
    Entry* again = hsearch_nid_postfix({copy.data(), reinterpret_cast<void*>(std::intptr_t(-1))}, 1);
    Require(again == found && again->data == reinterpret_cast<void*>(std::intptr_t(1234)), "enter of an existing key keeps it");
    for (int i = 0; i < 2000; i += 97) {
        Entry* entry = hsearch_nid_postfix({keys[i].data(), nullptr}, 0);
        Require(entry != nullptr && entry->data == reinterpret_cast<void*>(std::intptr_t(i)), "every key found");
    }
    *__error_nid_postfix() = 0;
    std::string missing = "absent";
    Require(hsearch_nid_postfix({missing.data(), nullptr}, 0) == nullptr && *__error_nid_postfix() == 3, "find miss sets ESRCH");
    hdestroy_nid_postfix();
    Require(hsearch_nid_postfix({copy.data(), nullptr}, 0) == nullptr, "hdestroy empties the table");
    hdestroy_nid_postfix();
    hdestroy_nid_postfix();
}

void CheckLinear() {
    int table[8] = {5, 3, 9};
    std::size_t count = 3;
    int key = 9;
    Require(lfind_nid_postfix(&key, table, &count, sizeof(int), CompareInt) == &table[2], "lfind found");
    key = 4;
    Require(lfind_nid_postfix(&key, table, &count, sizeof(int), CompareInt) == nullptr && count == 3, "lfind missing");
    Require(lsearch_nid_postfix(&key, table, &count, sizeof(int), CompareInt) == &table[3] && count == 4 && table[3] == 4, "lsearch appends");
    Require(lsearch_nid_postfix(&key, table, &count, sizeof(int), CompareInt) == &table[3] && count == 4, "lsearch finds");
    std::size_t none = 0;
    Require(lfind_nid_postfix(&key, nullptr, &none, sizeof(int), CompareInt) == nullptr, "lfind on an empty table");
}

void CheckQueue() {
    Queue a {nullptr, nullptr, 1}, b {nullptr, nullptr, 2}, c {nullptr, nullptr, 3};
    a.next = a.previous = &b;
    insque_nid_postfix(&a, nullptr);
    Require(a.next == nullptr && a.previous == nullptr, "first element");
    insque_nid_postfix(&c, &a);
    insque_nid_postfix(&b, &a);
    Require(a.next == &b && b.previous == &a && b.next == &c && c.previous == &b && c.next == nullptr, "inserted in order");
    remque_nid_postfix(&b);
    Require(a.next == &c && c.previous == &a, "removed from the middle");
    remque_nid_postfix(&a);
    Require(c.previous == nullptr, "removed the head");
    Queue x {nullptr, nullptr, 0}, y {nullptr, nullptr, 0};
    x.next = x.previous = &x;
    insque_nid_postfix(&y, &x);
    Require(x.next == &y && y.next == &x && x.previous == &y && y.previous == &x, "circular queue");
}

int Height(const Node* node) {
    if (node == nullptr) return 0;
    const int left = Height(node->left), right = Height(node->right);
    Require(node->balance == left - right && node->balance >= -1 && node->balance <= 1, "AVL balance");
    if (node->left) Require(CompareKey(node->left->key, node->key) < 0, "left order");
    if (node->right) Require(CompareKey(node->right->key, node->key) > 0, "right order");
    return 1 + std::max(left, right);
}

std::vector<std::intptr_t> visited;
int maximumLevel = 0;

void APS5_VABI Record(const Node* node, int visit, int level) {
    maximumLevel = std::max(maximumLevel, level);
    if (visit == 1 || visit == 3) visited.push_back(reinterpret_cast<std::intptr_t>(*reinterpret_cast<void* const*>(node)));
}

void CheckTree() {
    Node* root = nullptr;
    std::vector<std::intptr_t> keys;
    for (std::intptr_t i = 1; i <= 3000; i++) keys.push_back(i * 3);
    std::mt19937 random(5);
    std::shuffle(keys.begin(), keys.end(), random);
    for (auto key : keys) {
        Node* node = tsearch_nid_postfix(reinterpret_cast<void*>(key), &root, CompareKey);
        Require(node != nullptr && node->key == reinterpret_cast<void*>(key), "insert");
    }
    Require(tsearch_nid_postfix(reinterpret_cast<void*>(std::intptr_t(30)), &root, CompareKey) == tfind_nid_postfix(reinterpret_cast<void*>(std::intptr_t(30)), &root, CompareKey), "existing key returned");
    Require(Height(root) <= 17, "tree stays balanced");
    Require(tfind_nid_postfix(reinterpret_cast<void*>(std::intptr_t(31)), &root, CompareKey) == nullptr, "tfind miss");
    Require(tfind_nid_postfix(reinterpret_cast<void*>(std::intptr_t(31)), nullptr, CompareKey) == nullptr, "tfind null root");
    Require(tsearch_nid_postfix(reinterpret_cast<void*>(std::intptr_t(31)), nullptr, CompareKey) == nullptr, "tsearch null root");

    visited.clear();
    twalk_nid_postfix(root, Record);
    Require(visited.size() == 3000 && std::is_sorted(visited.begin(), visited.end()), "postorder and leaf visits are in key order");
    Require(maximumLevel == Height(root) - 1, "levels");
    twalk_nid_postfix(nullptr, Record);

    Require(tdelete_nid_postfix(reinterpret_cast<void*>(std::intptr_t(31)), &root, CompareKey) == nullptr, "tdelete miss");
    Require(tdelete_nid_postfix(reinterpret_cast<void*>(std::intptr_t(3)), nullptr, CompareKey) == nullptr, "tdelete null root");
    std::shuffle(keys.begin(), keys.end(), random);
    for (std::size_t i = 0; i < keys.size(); i++) {
        void* rootKey = root->key;
        void* parent = tdelete_nid_postfix(reinterpret_cast<void*>(keys[i]), &root, CompareKey);
        if (rootKey == reinterpret_cast<void*>(keys[i])) Require(parent == reinterpret_cast<void*>(1), "deleting the root returns 1");
        else Require(parent != nullptr && parent != reinterpret_cast<void*>(1), "deleting returns the parent");
        Require(tfind_nid_postfix(reinterpret_cast<void*>(keys[i]), &root, CompareKey) == nullptr, "deleted");
        if (i % 251 == 0) Height(root);
    }
    Require(root == nullptr, "empty tree");
}

}

int main() {
    std::array<void*, 10> api{};
    ApplicationHeapRegister_nid_no_patch(api.data());
    CheckHash();
    CheckLinear();
    CheckQueue();
    CheckTree();
    std::puts("guest search table tests passed");
    return 0;
}

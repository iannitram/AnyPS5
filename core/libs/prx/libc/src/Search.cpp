#include <climits>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>

#include "prx/libc/include/General.hpp"
#include "prx/libc/include/HostEntropy.hpp"

extern "C" {
int* APS5_VABI __error_nid_postfix();
void* APS5_VABI malloc_nid_postfix(std::size_t size);
void* APS5_VABI calloc_nid_postfix(std::size_t count, std::size_t size);
void APS5_VABI free_nid_postfix(void* pointer);
}

namespace {

using Compare = int (APS5_VABI *)(const void*, const void*);

struct Entry {
    char* key;
    void* data;
};

enum Action { Find = 0, Enter = 1 };
enum Visit { Preorder = 0, Postorder = 1, Endorder = 2, Leaf = 3 };

struct HashTable {
    std::size_t offsetBasis;
    std::size_t indexMask;
    std::size_t entriesUsed;
    Entry* entries;
};

struct TreeNode {
    void* key;
    TreeNode* left;
    TreeNode* right;
    signed char balance;
};

struct QueueElement {
    QueueElement* next;
    QueueElement* previous;
};

using WalkAction = void (APS5_VABI *)(const TreeNode*, Visit, int);

constexpr int GuestSearchFailed = 3;

HashTable* globalTable = nullptr;

std::size_t Hash(std::size_t basis, const char* key) {
    std::size_t hash = basis;
    while (*key != '\0') {
        hash ^= static_cast<std::uint8_t>(*key++);
        hash *= UINT64_C(1099511628211);
    }
    return hash;
}

Entry* FreeSlot(HashTable* table, std::size_t hash) {
    for (std::size_t index = hash, i = 0;; index += ++i) {
        Entry* entry = &table->entries[index & table->indexMask];
        if (entry->key == nullptr) return entry;
    }
}

HashTable* CreateTable() {
    auto* table = static_cast<HashTable*>(malloc_nid_postfix(sizeof(HashTable)));
    if (table == nullptr) return nullptr;
    table->entries = static_cast<Entry*>(calloc_nid_postfix(16, sizeof(Entry)));
    if (table->entries == nullptr) {
        free_nid_postfix(table);
        return nullptr;
    }
    HostEntropy_nid_no_patch(&table->offsetBasis, sizeof(table->offsetBasis));
    table->indexMask = 0xf;
    table->entriesUsed = 0;
    return table;
}

Entry* SearchTable(HashTable* table, Entry item, Action action) {
    if (item.key == nullptr) throw std::invalid_argument("hsearch: null key");
    const std::size_t hash = Hash(table->offsetBasis, item.key);
    Entry* entry;
    for (std::size_t index = hash, i = 0;; index += ++i) {
        entry = &table->entries[index & table->indexMask];
        if (entry->key == nullptr) break;
        if (std::strcmp(entry->key, item.key) == 0) return entry;
    }
    if (action == Find) {
        *__error_nid_postfix() = GuestSearchFailed;
        return nullptr;
    }
    if (table->entriesUsed * 2 >= table->indexMask) {
        const std::size_t oldCount = table->indexMask + 1;
        Entry* oldEntries = table->entries;
        const std::size_t newCount = oldCount * 2;
        auto* newEntries = static_cast<Entry*>(calloc_nid_postfix(newCount, sizeof(Entry)));
        if (newEntries == nullptr) return nullptr;
        table->entries = newEntries;
        table->indexMask = newCount - 1;
        for (std::size_t i = 0; i < oldCount; ++i) {
            if (oldEntries[i].key != nullptr) *FreeSlot(table, Hash(table->offsetBasis, oldEntries[i].key)) = oldEntries[i];
        }
        free_nid_postfix(oldEntries);
        entry = FreeSlot(table, hash);
    }
    *entry = item;
    ++table->entriesUsed;
    return entry;
}

struct Path {
    std::uintptr_t steps[2] {};
    unsigned count = 0;

    static constexpr unsigned Bits = sizeof(std::uintptr_t) * CHAR_BIT;

    void Reset() { count = 0; }
    void TakeLeft() {
        steps[count / Bits] |= std::uintptr_t(1) << (count % Bits);
        ++count;
    }
    void TakeRight() {
        steps[count / Bits] &= ~(std::uintptr_t(1) << (count % Bits));
        ++count;
    }
    bool TookLeft() {
        const bool result = steps[0] & 1;
        steps[0] = (steps[0] >> 1) | (steps[1] << (Bits - 1));
        steps[1] >>= 1;
        return result;
    }
};

void Walk(const TreeNode* root, WalkAction action, int level) {
    if (root->left == nullptr && root->right == nullptr) {
        action(root, Leaf, level);
        return;
    }
    action(root, Preorder, level);
    if (root->left != nullptr) Walk(root->left, action, level + 1);
    action(root, Postorder, level);
    if (root->right != nullptr) Walk(root->right, action, level + 1);
    action(root, Endorder, level);
}

}

extern "C" {

int APS5_VABI hcreate_nid_postfix(std::size_t) {
    return 1;
}

void APS5_VABI hdestroy_nid_postfix() {
    if (globalTable == nullptr) return;
    free_nid_postfix(globalTable->entries);
    free_nid_postfix(globalTable);
    globalTable = nullptr;
}

Entry* APS5_VABI hsearch_nid_postfix(Entry item, Action action) {
    if (globalTable == nullptr) {
        globalTable = CreateTable();
        if (globalTable == nullptr) return nullptr;
    }
    return SearchTable(globalTable, item, action);
}

void* APS5_VABI lfind_nid_postfix(const void* key, const void* base, std::size_t* count, std::size_t width, Compare compare) {
    if (count == nullptr || compare == nullptr) throw std::invalid_argument("lfind: null count or comparison");
    auto* element = static_cast<std::uint8_t*>(const_cast<void*>(base));
    for (auto* end = element + width * *count; element < end; element += width) {
        if (compare(key, element) == 0) return element;
    }
    return nullptr;
}

void* APS5_VABI lsearch_nid_postfix(const void* key, void* base, std::size_t* count, std::size_t width, Compare compare) {
    if (void* found = lfind_nid_postfix(key, base, count, width, compare)) return found;
    auto* end = static_cast<std::uint8_t*>(base) + width * *count;
    std::memcpy(end, key, width);
    ++*count;
    return end;
}

void APS5_VABI insque_nid_postfix(void* element, void* predecessor) {
    if (element == nullptr) throw std::invalid_argument("insque: null element");
    auto* inserted = static_cast<QueueElement*>(element);
    auto* previous = static_cast<QueueElement*>(predecessor);
    if (previous == nullptr) {
        inserted->previous = inserted->next = nullptr;
        return;
    }
    QueueElement* next = previous->next;
    if (next != nullptr) next->previous = inserted;
    previous->next = inserted;
    inserted->previous = previous;
    inserted->next = next;
}

void APS5_VABI remque_nid_postfix(void* element) {
    if (element == nullptr) throw std::invalid_argument("remque: null element");
    auto* removed = static_cast<QueueElement*>(element);
    if (removed->previous != nullptr) removed->previous->next = removed->next;
    if (removed->next != nullptr) removed->next->previous = removed->previous;
}

TreeNode* APS5_VABI tfind_nid_postfix(const void* key, TreeNode* const* root, Compare compare) {
    if (root == nullptr) return nullptr;
    while (*root != nullptr) {
        const int order = compare(key, (*root)->key);
        if (order == 0) return *root;
        root = order < 0 ? &(*root)->left : &(*root)->right;
    }
    return nullptr;
}

TreeNode* APS5_VABI tsearch_nid_postfix(const void* key, TreeNode** root, Compare compare) {
    if (root == nullptr) return nullptr;
    Path path;
    TreeNode** leaf = root;
    while (*leaf != nullptr) {
        if ((*leaf)->balance != 0) {
            root = leaf;
            path.Reset();
        }
        const int order = compare(key, (*leaf)->key);
        if (order < 0) {
            path.TakeLeft();
            leaf = &(*leaf)->left;
        } else if (order > 0) {
            path.TakeRight();
            leaf = &(*leaf)->right;
        } else {
            return *leaf;
        }
    }
    TreeNode* result = *leaf = static_cast<TreeNode*>(malloc_nid_postfix(sizeof(TreeNode)));
    if (result == nullptr) return nullptr;
    result->key = const_cast<void*>(key);
    result->left = nullptr;
    result->right = nullptr;
    result->balance = 0;

    for (TreeNode* node = *root; node != *leaf;) {
        if (path.TookLeft()) {
            node->balance += 1;
            node = node->left;
        } else {
            node->balance -= 1;
            node = node->right;
        }
    }

    TreeNode* x = *root;
    if (x->balance > 1) {
        TreeNode* y = x->left;
        if (y->balance < 0) {
            TreeNode* z = y->right;
            y->right = z->left;
            z->left = y;
            x->left = z->right;
            z->right = x;
            *root = z;
            x->balance = z->balance > 0 ? -1 : 0;
            y->balance = z->balance < 0 ? 1 : 0;
            z->balance = 0;
        } else {
            x->left = y->right;
            y->right = x;
            *root = y;
            x->balance = 0;
            y->balance = 0;
        }
    } else if (x->balance < -1) {
        TreeNode* y = x->right;
        if (y->balance > 0) {
            TreeNode* z = y->left;
            x->right = z->left;
            z->left = x;
            y->left = z->right;
            z->right = y;
            *root = z;
            x->balance = z->balance < 0 ? 1 : 0;
            y->balance = z->balance > 0 ? -1 : 0;
            z->balance = 0;
        } else {
            x->right = y->left;
            y->left = x;
            *root = y;
            x->balance = 0;
            y->balance = 0;
        }
    }
    return result;
}

void* APS5_VABI tdelete_nid_postfix(const void* key, TreeNode** root, Compare compare) {
    if (root == nullptr) return nullptr;
    Path path;
    TreeNode** leaf = root;
    auto goLeft = [&] {
        if ((*leaf)->balance == 0 || ((*leaf)->balance < 0 && (*leaf)->right->balance == 0)) {
            root = leaf;
            path.Reset();
        }
        path.TakeLeft();
        leaf = &(*leaf)->left;
    };
    auto goRight = [&] {
        if ((*leaf)->balance == 0 || ((*leaf)->balance > 0 && (*leaf)->left->balance == 0)) {
            root = leaf;
            path.Reset();
        }
        path.TakeRight();
        leaf = &(*leaf)->right;
    };

    TreeNode* result = reinterpret_cast<TreeNode*>(1);
    for (;;) {
        if (*leaf == nullptr) return nullptr;
        const int order = compare(key, (*leaf)->key);
        if (order < 0) {
            result = *leaf;
            goLeft();
        } else if (order > 0) {
            result = *leaf;
            goRight();
        } else {
            break;
        }
    }

    if ((*leaf)->left == nullptr) {
        TreeNode* old = *leaf;
        *leaf = old->right;
        free_nid_postfix(old);
    } else {
        void** keySlot = &(*leaf)->key;
        goLeft();
        while ((*leaf)->right != nullptr) goRight();
        TreeNode* old = *leaf;
        *keySlot = old->key;
        *leaf = old->left;
        free_nid_postfix(old);
    }

    for (TreeNode** node = root; node != leaf;) {
        TreeNode* x = *node;
        if (path.TookLeft()) {
            if (x->balance < 0) {
                TreeNode* y = x->right;
                if (y->balance > 0) {
                    TreeNode* z = y->left;
                    x->right = z->left;
                    z->left = x;
                    y->left = z->right;
                    z->right = y;
                    *node = z;
                    x->balance = z->balance < 0 ? 1 : 0;
                    y->balance = z->balance > 0 ? -1 : 0;
                    z->balance = 0;
                } else {
                    x->right = y->left;
                    y->left = x;
                    *node = y;
                    if (y->balance < 0) {
                        x->balance = 0;
                        y->balance = 0;
                    } else {
                        x->balance = -1;
                        y->balance = 1;
                    }
                }
            } else {
                --x->balance;
            }
            node = &x->left;
        } else {
            if (x->balance > 0) {
                TreeNode* y = x->left;
                if (y->balance < 0) {
                    TreeNode* z = y->right;
                    y->right = z->left;
                    z->left = y;
                    x->left = z->right;
                    z->right = x;
                    *node = z;
                    x->balance = z->balance > 0 ? -1 : 0;
                    y->balance = z->balance < 0 ? 1 : 0;
                    z->balance = 0;
                } else {
                    x->left = y->right;
                    y->right = x;
                    *node = y;
                    if (y->balance > 0) {
                        x->balance = 0;
                        y->balance = 0;
                    } else {
                        x->balance = 1;
                        y->balance = -1;
                    }
                }
            } else {
                ++x->balance;
            }
            node = &x->right;
        }
    }
    return result;
}

void APS5_VABI twalk_nid_postfix(const TreeNode* root, WalkAction action) {
    if (root != nullptr && action != nullptr) Walk(root, action, 0);
}

}

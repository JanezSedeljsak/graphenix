#include <string>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <fstream>
#include <unordered_set>
#include <type_traits>
#include "bptreenode.hpp"

using namespace std;

template <typename T>
class BPTreeIndex
{
    static_assert(is_same<T, string>::value || is_same<T, int64_t>::value,
                  "T must be string or int64_t");

public:
    typedef pair<pair<int, int>, shared_ptr<BPTreeNode<T>>> LeafMatchInterval;
    string ix_filename;
    shared_ptr<BPTreeNode<T>> root;
    int key_size;

    inline void init(const string &model_name, const string &field_name)
    {
        // when stable should use -> get_field_ix_file_name;
        ix_filename = "bpt_ix_" + model_name + "_" + field_name + ".bin";
        root = nullptr;
    }

    BPTreeIndex(string model_name, string field_name)
    {
        init(model_name, field_name);
        key_size = IX_SIZE;
    }

    BPTreeIndex(string model_name, string field_name, int size)
    {
        init(model_name, field_name);
        key_size = size;
    }

    // using built in swap instead of this
    //  template <typename VEC_T>
    //  static void vector_swap(vector<VEC_T> &vec1, vector<VEC_T> &vec2)
    //  {
    //      vector<VEC_T> temp = vec1;
    //      vec1 = vec2;
    //    vec2 = temp;
    //}

    static void print_nodes_with_intervals(const vector<LeafMatchInterval> &nodes)
    {
        for (const auto &node : nodes)
        {
            cout << "Start " << node.first.first << " End: " << node.first.second << endl;
            for (int i = node.first.first; i <= node.first.second; i++)
                cout << "Key: " << node.second->keys[i] << " Value: " << node.second->data[i] << endl;
        }
    }

    static unordered_set<int64_t> flatten_intervals_to_ptrs(const vector<LeafMatchInterval> &nodes)
    {
        unordered_set<int64_t> ptrs;
        for (const auto &node : nodes)
            for (int i = node.first.first; i <= node.first.second; i++)
                ptrs.insert(node.second->data[i]);

        return ptrs;
    }

    void create()
    {
        if (filesystem::exists(ix_filename))
            return;

        ofstream ix_file(ix_filename, ios::binary | ios::out);
        BPTreeNode<T> *node_ptr = new BPTreeNode<T>(2 * IX_SIZE, key_size);
        root = shared_ptr<BPTreeNode<T>>(node_ptr);

        int64_t head_ptr = -1, next_free = -1;
        char buffer[2 * IX_SIZE];
        memcpy(buffer, reinterpret_cast<const char *>(&head_ptr), IX_SIZE);
        memcpy(buffer + IX_SIZE, reinterpret_cast<const char *>(&next_free), IX_SIZE);

        ix_file.seekp(0, ios::beg);
        ix_file.write(buffer, 2 * IX_SIZE);
        ix_file.close();
    }

    void write()
    {
        fstream ix_file(ix_filename, ios::binary | ios::in | ios::out);
        ix_file.seekp(root->offset, ios::beg);
        root->write(ix_file);
        set_head_ptr(ix_file, root->offset);
        ix_file.close();
    }

    inline int64_t get_head_ptr(fstream &ix_file)
    {
        int64_t head_ptr;
        ix_file.seekg(0, ios::beg);
        ix_file.read(reinterpret_cast<char *>(&head_ptr), IX_SIZE);
        return head_ptr;
    }

    inline int64_t get_next_free(fstream &ix_file)
    {
        int64_t next_free;
        ix_file.seekg(IX_SIZE, ios::beg);
        ix_file.read(reinterpret_cast<char *>(&next_free), IX_SIZE);
        return next_free;
    }

    inline void set_head_ptr(fstream &ix_file, int64_t head_ptr)
    {
        ix_file.seekp(0, ios::beg);
        ix_file.write(reinterpret_cast<const char *>(&head_ptr), IX_SIZE);
    }

    inline void set_next_free(fstream &ix_file, int64_t next_free)
    {
        ix_file.seekp(IX_SIZE, ios::beg);
        ix_file.write(reinterpret_cast<const char *>(&next_free), IX_SIZE);
    }

    inline int64_t get_and_set_next_free(fstream &ix_file)
    {
        int64_t next_free = get_next_free(ix_file);
        if (next_free == -1)
        {
            ix_file.seekg(0, ios::end);
            int64_t ix_offset = ix_file.tellg();
            return ix_offset;
        }

        ix_file.seekg(next_free, ios::beg);
        int64_t new_next_free;
        ix_file.read(reinterpret_cast<char *>(&new_next_free), IX_SIZE);
        set_next_free(ix_file, new_next_free);
        return next_free;
    }

    void delete_and_set_next_free(fstream &ix_file, int64_t new_next_free)
    {
        int64_t next_free = get_next_free(ix_file);
        set_next_free(ix_file, new_next_free);
        if (next_free != -1)
        {
            ix_file.seekp(new_next_free, ios::beg);
            ix_file.write(reinterpret_cast<const char *>(&next_free), IX_SIZE);
        }
    }

    void read()
    {
        fstream ix_file(ix_filename, ios::binary | ios::in | ios::out);
        root->offset = get_head_ptr(ix_file);
        root->read(ix_file);
        ix_file.close();
    }

    void load_full_tree()
    {
        fstream ix_file(ix_filename, ios::binary | ios::in | ios::out);
        root->offset = get_head_ptr(ix_file);
        root->read_recursive(ix_file);
        ix_file.close();
    }

    void print_index(bool is_recursive)
    {
        if (root != nullptr)
            root->print(is_recursive);
    }

    void delete_index()
    {
        if (filesystem::exists(ix_filename))
            filesystem::remove(ix_filename);
    }

    inline bool generic_equal(const T &a, const T &b)
    {
        if constexpr (is_same_v<T, string>)
            return strcmp(a.c_str(), b.c_str()) == 0;
        else
            return a == b;
    }

    /**
     * @brief Returns the range[from, to] (interval) in the leaf node where the keys match
     *
     * @param search generic search parameter (the key we search by)
     * @param node the actual node that was read from the file (should be type is_leaf)
     * @param ix_file the reader object
     * @return vector<LeafMatchRange>
     */
    inline LeafMatchInterval search_leaf(const T &search, shared_ptr<BPTreeNode<T>> node, fstream &ix_file)
    {
        less<T> generic_less;
        int low = 0, high = node->keys.size();
        while (low < high)
        {
            int mid = (low + high) / 2;
            if (generic_equal(node->keys[mid], search))
            {
                int from = mid, to = mid;
                while (from > 0 && generic_equal(node->keys[from - 1], search))
                    from--;

                while (to < node->keys.size() - 1 && generic_equal(node->keys[to + 1], search))
                    to++;

                return make_pair(make_pair(from, to), node);
            }

            if (!generic_less(node->keys[mid], search))
                high = mid;
            else
                low = mid + 1;
        }

        return make_pair(make_pair(-1, -1), nullptr);
    }

    inline void extend_node_interval(vector<LeafMatchInterval> &nodes, const T &search, shared_ptr<BPTreeNode<T>> node, fstream &ix_file, const int64_t idx)
    {
        shared_ptr<BPTreeNode<T>> child;
        if (!node->is_cached)
        {
            BPTreeNode<T> *node_ptr = new BPTreeNode<T>(node->children[idx], key_size);
            child = shared_ptr<BPTreeNode<T>>(node_ptr);
            child->read(ix_file);
        }
        else
            child = node->actual_children[idx];

        if (child->is_leaf)
        {
            LeafMatchInterval found = search_leaf(search, child, ix_file);
            if (found.first.first != -1)
                nodes.push_back(found);
        }
        else
        {
            const auto &curr_nodes = search_internal(search, child, ix_file);
            nodes.insert(nodes.end(), curr_nodes.begin(), curr_nodes.end());
        }
    }

    /**
     * Currently not in use
     */
    inline void extend_interval_next(vector<LeafMatchInterval> &nodes, const T &search, shared_ptr<BPTreeNode<T>> current, fstream &ix_file)
    {
        while (current->next != -1)
        {
            shared_ptr<BPTreeNode<T>> next_node = current->get_next(ix_file);
            LeafMatchInterval next_found = search_leaf(search, next_node, ix_file);
            if (next_found.first.first == -1)
                break;

            nodes.push_back(next_found);
            current = next_node;
        }
    }

    /**
     * @brief Returns a vector of leaf nodes that match the specific value
     *
     * @param search
     * @param node
     * @param ix_file
     * @return vector<LeafMatchRange>
     */
    inline vector<LeafMatchInterval> search_internal(const T &search, shared_ptr<BPTreeNode<T>> node, fstream &ix_file)
    {
        vector<LeafMatchInterval> nodes;
        less<T> generic_less;
        bool found_equal = false;
        int low = 0, high = node->keys.size();
        while (low < high)
        {
            int mid = (low + high) / 2;
            if (generic_equal(node->keys[mid], search))
            {
                found_equal = true;
                break;
            }

            if (!generic_less(node->keys[mid], search))
                high = mid;
            else
                low = mid + 1;
        }

        if (!found_equal)
        {
            extend_node_interval(nodes, search, node, ix_file, low);
            return nodes;
        }

        int from = low, to = low;
        while (from > 0 && generic_equal(node->keys[from - 1], search))
            from--;

        while (to < node->keys.size() - 1 && generic_equal(node->keys[to + 1], search))
            to++;

        // check from left to right + 1 (there are more ptrs than keys)
        for (int i = from; i <= to + 1; i++)
            extend_node_interval(nodes, search, node, ix_file, i);

        return nodes;
    }

    vector<LeafMatchInterval> find(const T &search)
    {
        if (root == nullptr)
            return {};

        fstream ix_file(ix_filename, ios::binary | ios::in | ios::out);
        if (!root->is_cached)
            root->read(ix_file);

        vector<LeafMatchInterval> nodes;
        if (root->is_leaf)
        {
            LeafMatchInterval found = search_leaf(search, root, ix_file);
            if (found.first.first != -1)
                nodes.push_back(found);
        }
        else
            nodes = search_internal(search, root, ix_file);

        ix_file.close();
        return nodes;
    }

    // inline BPTreeNode<T> *insert_into_leaf(shared_ptr<BPTreeNode<T>> node, T &key, int64_t record_offset)
    // {
    // }
    // inline BPTreeNode<T> *insert_into_internal(shared_ptr<BPTreeNode<T>> node, T &key, int64_t record_offset)
    // {
    // }

    void insert(T key, int64_t record_offset)
    {
        if (root == nullptr)
            read(); // loads root from file

        fstream ix_file(ix_filename, ios::binary | ios::in | ios::out);
        shared_ptr<BPTreeNode<T>> current = root;
        shared_ptr<BPTreeNode<T>> parent;
        less<T> generic_less;
        int keys_count = current->keys.size();

        //  cout << "Search for leaf" << endl;
        while (!current->is_leaf)
        {
            parent = current;
            for (int i = 0; i < keys_count; i++)
            {
                if (generic_less(key, current->keys[i]))
                {
                    BPTreeNode<T> *node_ptr = new BPTreeNode<T>(current->children[i], key_size);
                    current = shared_ptr<BPTreeNode<T>>(node_ptr);
                    current->read(ix_file);
                    keys_count = current->keys.size();
                    break;
                }

                if (i == keys_count - 1)
                {
                    BPTreeNode<T> *node_ptr = new BPTreeNode<T>(current->children[i + 1], key_size);
                    current = shared_ptr<BPTreeNode<T>>(node_ptr);
                    current->read(ix_file);
                    keys_count = current->keys.size();
                    break;
                }
            }
        }

        if (keys_count < current->get_capacity())
        {
            int i = 0;
            while (i < keys_count && !generic_less(key, current->keys[i]))
                i++;

            // create placeholder item
            current->keys.resize(keys_count + 1);
            current->data.resize(keys_count + 1);

            for (int j = keys_count + 1; j > i; j--)
            {
                current->keys[j] = current->keys[j - 1];
                current->data[j] = current->data[j - 1];
            }

            current->data[i] = record_offset;
            current->keys[i] = key;
            current->write(ix_file);
            if (keys_count == 0)
                set_head_ptr(ix_file, current->offset);
        }
        else
        {
            BPTreeNode<T> *node_ptr = new BPTreeNode<T>(-1, key_size);
            shared_ptr<BPTreeNode<T>> new_leaf = shared_ptr<BPTreeNode<T>>(node_ptr);
            vector<T> tmp_keys(current->get_capacity() + 1);
            vector<int64_t> tmp_data(current->get_capacity() + 1);

            for (int i = 0; i < current->get_capacity(); i++)
            {
                tmp_keys[i] = current->keys[i];
                tmp_data[i] = current->data[i];
            }

            int i = 0;
            while (!generic_less(key, tmp_keys[i]) && i < current->get_capacity())
                i++;

            for (int k = current->get_capacity(); k > i; k--)
            {
                tmp_keys[k] = tmp_keys[k - 1];
                tmp_data[k] = tmp_data[k - 1];
            }

            tmp_keys[i] = key;
            tmp_data[i] = record_offset;

            int new_size = (current->get_capacity() + 1) - (current->get_capacity() + 1) / 2;
            keys_count = (current->get_capacity() + 1) / 2;

            current->keys.resize(keys_count);
            current->data.resize(keys_count);

            new_leaf->keys.resize(new_size);
            new_leaf->data.resize(new_size);

            for (int i = 0; i < keys_count; i++)
            {
                current->keys[i] = tmp_keys[i];
                current->data[i] = tmp_data[i];
            }

            // cout << "new size " << new_size << endl;
            for (int i = 0; i < new_size; i++)
            {
                new_leaf->keys[i] = tmp_keys[i + keys_count];
                new_leaf->data[i] = tmp_data[i + keys_count];
            }

            int64_t new_leaf_offset = get_and_set_next_free(ix_file);
            new_leaf->offset = new_leaf_offset;

            if (current->offset == root->offset)
            {
                BPTreeNode<T> *node_ptr2 = new BPTreeNode<T>(-1, key_size);
                shared_ptr<BPTreeNode<T>> new_root = shared_ptr<BPTreeNode<T>>(node_ptr2);
                new_root->keys.push_back(new_leaf->keys[0]);
                new_root->children.push_back(current->offset);
                new_leaf->set_prev(current); // new_leaf is bottom right node <- prev is current
                new_leaf->write(ix_file);
                new_root->children.push_back(new_leaf_offset);
                new_root->offset = get_and_set_next_free(ix_file);
                new_root->is_leaf = false;
                new_root->write(ix_file);
                current->set_next(new_leaf); // current is bottom left node -> next is new_leaf
                current->write(ix_file);
                root = new_root;
                set_head_ptr(ix_file, new_root->offset);
            }
            else
            {
                current->write(ix_file);
                new_leaf->write(ix_file);
                shift_tree_level(new_leaf->keys[0], parent, new_leaf, ix_file);
            }
        }

        ix_file.close();
    }

    inline void shift_tree_level(T key, shared_ptr<BPTreeNode<T>> parent, shared_ptr<BPTreeNode<T>> child, fstream &ix_file)
    {
        const int keys_count = parent->keys.size();
        if (keys_count < parent->get_capacity())
        {
            int i = 0;
            while (i < keys_count && key > parent->keys[i])
                i++;

            parent->keys.resize(keys_count + 1);
            parent->children.resize(keys_count + 2);

            for (int j = keys_count; j > i; j--)
                parent->keys[j] = parent->keys[j - 1];

            for (int j = keys_count + 1; j > i + 1; j--)
                parent->children[j] = parent->children[j - 1];

            parent->keys[i] = key;
            parent->children[i + 1] = child->offset;

            if (child->is_leaf)
            {
                BPTreeNode<T> *prev_ptr = new BPTreeNode<T>(parent->children[i], key_size);
                shared_ptr<BPTreeNode<T>> prev_node = shared_ptr<BPTreeNode<T>>(prev_ptr);
                prev_node->read(ix_file);
                prev_node->set_next(child);
                child->set_prev(prev_node);
                prev_node->write(ix_file);
            }

            if (i < keys_count && child->is_leaf)
            {
                BPTreeNode<T> *next_ptr = new BPTreeNode<T>(parent->children[i + 2], key_size);
                shared_ptr<BPTreeNode<T>> next_node = shared_ptr<BPTreeNode<T>>(next_ptr);
                next_node->read(ix_file);

                // hack this should be handled differently
                // problem when inserting multiple same keys sometimes current and next are in wrong order
                if (next_node->keys[next_node->keys.size() - 1] < child->keys[child->keys.size() - 1])
                {
                    swap(child->keys, next_node->keys);
                    swap(child->data, next_node->data);
                    swap(child->children, next_node->children);
                }

                next_node->set_prev(child);
                child->set_next(next_node);
                next_node->write(ix_file);
            }

            child->write(ix_file);
            parent->write(ix_file);
        }
        else
        {
            // create new level
            BPTreeNode<T> *new_internal_ptr = new BPTreeNode<T>(-1, key_size);
            shared_ptr<BPTreeNode<T>> new_internal_node = shared_ptr<BPTreeNode<T>>(new_internal_ptr);

            int max_capacity = parent->get_capacity();
            int64_t new_keys[max_capacity + 1];
            int64_t new_offsets[max_capacity + 2];

            for (int i = 0; i < parent->keys.size(); i++)
                new_keys[i] = parent->keys[i];

            for (int i = 0; i < max_capacity + 1; i++)
                new_offsets[i] = parent->children[i];

            int i = 0;
            while (i < max_capacity && key > new_keys[i])
                i++;

            for (int j = max_capacity + 1; j > i; j--)
                new_keys[j] = new_keys[j - 1];
            new_keys[i] = key;

            for (int j = max_capacity + 2; j > i + 1; j--)
                new_offsets[j] = new_offsets[j - 1];
            new_offsets[i + 1] = child->offset;

            new_internal_node->is_leaf = false;
            int child_size = (max_capacity + 1) / 2;
            int new_internal_size = max_capacity - (max_capacity + 1) / 2;

            child->keys.resize(child_size);
            child->children.resize(child_size + 1);
            new_internal_node->keys.resize(new_internal_size);
            new_internal_node->children.resize(new_internal_size + 1);

            for (int i = 0; i < new_internal_size; i++)
                new_internal_node->keys[i] = new_keys[i + child_size + 1];

            for (int i = 0; i < new_internal_size + 1; i++)
                new_internal_node->children[i] = new_offsets[i + child_size + 1];

            child->write(ix_file);
            new_internal_node->offset = get_and_set_next_free(ix_file);
            new_internal_node->write(ix_file);

            if (parent->offset == root->offset)
            {
                BPTreeNode<T> *new_root_ptr = new BPTreeNode<T>(-1, key_size);
                shared_ptr<BPTreeNode<T>> new_root_node = shared_ptr<BPTreeNode<T>>(new_root_ptr);
                new_root_node->keys.push_back(parent->keys[child_size]);
                new_root_node->children.push_back(parent->offset);
                new_root_node->children.push_back(new_internal_node->offset);
                new_root_node->is_leaf = false;
                new_root_node->offset = get_and_set_next_free(ix_file);
                new_root_node->write(ix_file);
                root = new_root_node;
                set_head_ptr(ix_file, new_root_node->offset);
            }
            else
            {
                shift_tree_level(parent->keys[child_size],
                                 get_parent(root, parent, ix_file),
                                 new_internal_node,
                                 ix_file);
            }
        }
    }

    inline shared_ptr<BPTreeNode<T>> get_parent(shared_ptr<BPTreeNode<T>> &current, shared_ptr<BPTreeNode<T>> &search, fstream &ix_file)
    {
        if (current->is_leaf)
            return nullptr;

        BPTreeNode<T> *first_child_ptr = new BPTreeNode<T>(current->children[0], key_size);
        shared_ptr<BPTreeNode<T>> first_child = shared_ptr<BPTreeNode<T>>(first_child_ptr);
        first_child->read(ix_file);
        if (first_child->is_leaf)
            return nullptr;

        for (int i = 0; i < current->keys.size() + 1; i++)
        {
            BPTreeNode<T> *tmp_child_ptr = new BPTreeNode<T>(current->children[0], key_size);
            shared_ptr<BPTreeNode<T>> tmp_child = shared_ptr<BPTreeNode<T>>(tmp_child_ptr);
            tmp_child->read(ix_file);

            if (tmp_child->offset == search->offset)
                return current;

            shared_ptr<BPTreeNode<T>> parent = get_parent(tmp_child, search, ix_file);
            if (parent != nullptr)
                return parent;
        }

        return nullptr;
    }

    inline shared_ptr<BPTreeNode<T>> inner_get_min_leaf(shared_ptr<BPTreeNode<T>> &current, fstream &ix_file)
    {
        if (current->is_leaf)
            return current;

        BPTreeNode<T> *first_child_ptr = new BPTreeNode<T>(current->children[0], key_size);
        shared_ptr<BPTreeNode<T>> first_child = shared_ptr<BPTreeNode<T>>(first_child_ptr);
        first_child->read(ix_file);
        return inner_get_min_leaf(first_child, ix_file);
    }

    shared_ptr<BPTreeNode<T>> get_min_leaf(fstream &ix_file)
    {
        if (root == nullptr)
            read(); // loads root from file

        shared_ptr<BPTreeNode<T>> found = inner_get_min_leaf(root, ix_file);
        return found;
    }

    inline shared_ptr<BPTreeNode<T>> inner_get_max_leaf(shared_ptr<BPTreeNode<T>> &current, fstream &ix_file)
    {
        if (current->is_leaf)
            return current;

        const size_t last_index = current->children.size() - 1;
        BPTreeNode<T> *last_child_ptr = new BPTreeNode<T>(current->children[last_index], key_size);
        shared_ptr<BPTreeNode<T>> last_child = shared_ptr<BPTreeNode<T>>(last_child_ptr);
        last_child->read(ix_file);
        return inner_get_max_leaf(last_child, ix_file);
    }

    shared_ptr<BPTreeNode<T>> get_max_leaf(fstream &ix_file)
    {
        if (root == nullptr)
            read(); // loads root from file

        shared_ptr<BPTreeNode<T>> found = inner_get_max_leaf(root, ix_file);
        return found;
    }

    vector<int64_t> get_first_n_values_with_offset(int amount, int offset)
    {
        fstream ix_file(ix_filename, ios::binary | ios::in | ios::out);
        shared_ptr<BPTreeNode<T>> current = get_min_leaf(ix_file);

        vector<int64_t> offsets;
        const bool ignore_count = amount == -1;
        bool limit_reached = false;
        while (ignore_count || amount > 0)
        {
            for (const auto &child_offset : current->data)
            {
                if (offset > 0)
                {
                    offset--;
                    continue;
                }

                offsets.push_back(child_offset);
                if (!ignore_count && --amount == 0)
                {
                    limit_reached = true;
                    break;
                }
            }

            bool edge_reached = current->next == -1;
            if (limit_reached || edge_reached)
                break;

            current = current->get_next(ix_file);
        }

        ix_file.close();
        return offsets;
    }

    vector<int64_t> get_last_n_values_with_offset(int amount, int offset)
    {
        fstream ix_file(ix_filename, ios::binary | ios::in | ios::out);
        shared_ptr<BPTreeNode<T>> current = get_max_leaf(ix_file);

        vector<int64_t> offsets;
        const bool ignore_count = amount == -1;
        bool limit_reached = false;
        while (ignore_count || amount > 0)
        {
            for (int i = current->data.size() - 1; i >= 0; i--)
            {
                const auto &child_offset = current->data[i];
                if (offset > 0)
                {
                    offset--;
                    continue;
                }

                offsets.push_back(child_offset);
                if (!ignore_count && --amount == 0)
                {
                    limit_reached = true;
                    break;
                }
            }

            bool edge_reached = current->prev == -1;
            if (limit_reached || edge_reached)
                break;

            current = current->get_prev(ix_file);
        }

        ix_file.close();
        return offsets;
    }

    inline bool remove_from_leaf(shared_ptr<BPTreeNode<T>> node, T &key, int64_t record_offset)
    {
        return true;
    }

    inline bool remove_from_internal(shared_ptr<BPTreeNode<T>> node, T &key, int64_t record_offset)
    {
        return true;
    }

    bool remove(T &key, int64_t record_offset)
    {
        if (root == nullptr)
            read(); // loads root from file

        fstream ix_file(ix_filename, ios::binary | ios::in | ios::out);
        return root->is_leaf
                   ? delete_from_leaf(root, key, record_offset)
                   : delete_from_internal(root, key, record_offset);
    }
};
#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <map>
#include <sstream>

struct ListNode {
    ListNode* prev;
    ListNode* next;
    ListNode* rand;

    std::string data;
};

class List {
public:
    void Serialize(FILE* file);
    void Deserialize(FILE* file);

    ListNode* head;
    ListNode* tail;
    int count;
};

struct node_value
{
    int index;
    bool has_value;
};

struct fake_node
{
    node_value next = {};
    node_value prev = {};
    node_value rand = {};
    ListNode* listNode;
};

struct expanded_list
{
private:

    bool push(ListNode* key, int index)
    {
        if (data.find(key) == data.end())
        {
            data[key] = index;
            return false;
        }

        return true;
    }

    void initialize(List* list)
    {
        auto node = list->head;
        auto index = 0;
        auto hasCycle = false;

        while (node != nullptr && !hasCycle)
        {
            hasCycle = push(node, index);
            index++;
            node = node->next;
        }
    }

public:
    std::map<ListNode*, int> data;

    expanded_list(List* originalList)
    {
        initialize(originalList);
    }

    ~expanded_list()
    {
        data.clear();
    }

    node_value get_index(ListNode* key)
    {
        node_value nodeValue = {};
        nodeValue.has_value = false;

        auto value = data.find(key);

        if (value == data.end())
        {
            return nodeValue;
        }

        nodeValue.has_value = true;
        nodeValue.index = value->second;

        return nodeValue;
    }
};

struct file_data
{
    size_t file_size;
    int items_count;
};

void write_to_buffer(char* buffer, size_t& bufferSize, const void* data, size_t dataSize) {
    memcpy(&buffer[bufferSize], data, dataSize);
    bufferSize += dataSize;
}

void read_from_buffer(const char* buffer, size_t& bufferPos, void* data, size_t dataSize) {
    memcpy(data, &buffer[bufferPos], dataSize);
    bufferPos += dataSize;
}

void List::Serialize(FILE* file)
{
    if (file == nullptr)
        return;

    char buffer[4096];
    memset(&buffer, 0, sizeof(buffer));

    size_t buffer_size = 0;
    size_t data_size = 0;

    buffer_size += sizeof(file_data);

    auto list = expanded_list(this);

    for (auto const& item : list.data)
    {
        data_size = item.first->data.size();
        auto nextIndex = list.get_index(item.first->next);
        auto prevIndex = list.get_index(item.first->prev);
        auto randIndex = list.get_index(item.first->rand);

        write_to_buffer(buffer, buffer_size, &data_size, sizeof(size_t));
        write_to_buffer(buffer, buffer_size, item.first->data.data(), sizeof(char) * data_size);
        write_to_buffer(buffer, buffer_size, &nextIndex, sizeof(node_value));
        write_to_buffer(buffer, buffer_size, &prevIndex, sizeof(node_value));
        write_to_buffer(buffer, buffer_size, &randIndex, sizeof(node_value));

        data_size = 0;
    }

    file_data header = {};
    header.file_size = buffer_size;
    header.items_count = list.data.size();
    write_to_buffer(buffer, data_size, &header, sizeof(file_data));

    fwrite(buffer, sizeof(char), buffer_size, file);

    fflush(file);
}

void List::Deserialize(FILE* file)
{
    if (file == nullptr)
        return;

    file_data header = {};
    fread(&header, sizeof(file_data), 1, file);

    char* buffer = new char[header.file_size];
    fread(buffer, sizeof(char), header.file_size - sizeof(file_data), file);

    int itemsCount = header.items_count;
    size_t dataSize = 0;

    fake_node* items = new fake_node[header.items_count]; 

    size_t bufferPos = 0;

    for (size_t i = 0; i < itemsCount; i++)
    {
        items[i].listNode = new ListNode();

        read_from_buffer(buffer, bufferPos, &dataSize, sizeof(size_t));
        read_from_buffer(buffer, bufferPos, (void*)items[i].listNode->data.data(), sizeof(char) * dataSize);
        read_from_buffer(buffer, bufferPos, &items[i].next, sizeof(node_value));
        read_from_buffer(buffer, bufferPos, &items[i].prev, sizeof(node_value));
        read_from_buffer(buffer, bufferPos, &items[i].rand, sizeof(node_value));

        dataSize = 0;
    }

    for (size_t i = 0; i < itemsCount; i++)
    {
        if (items[i].next.has_value)
            items[i].listNode->next = items[items[i].next.index].listNode;

        if (items[i].prev.has_value)
            items[i].listNode->prev = items[items[i].prev.index].listNode;

        if (items[i].rand.has_value)
            items[i].listNode->rand = items[items[i].rand.index].listNode;
    }

    count = itemsCount;
    head = items[0].listNode;
    tail = items[itemsCount - 1].listNode;

    delete[] buffer;
    delete[] items;
}

int main()
{
    auto list = List();

    ListNode head;
    ListNode next;
    ListNode next2;
    ListNode tail;

    head.data = "head";
    head.next = &next;
    head.prev = &tail;
    head.rand = &tail;

    next.data = "next";
    next.next = &next2;
    next.prev = &head;
    next.rand = &head;

    next2.data = "next2";
    next2.next = &tail;
    next2.prev = &next;
    next2.rand = nullptr;

    tail.data = "tail";
    tail.next = &head;
    tail.prev = &next2;
    tail.rand = nullptr;

    list.head = &head;
    list.tail= &tail;
    list.count = 4;

    FILE* file;
    file = fopen("list.bin", "wb");
    list.Serialize(file);
    fclose(file);

    auto list2 = List();
    file = fopen("list.bin", "rb");
    list2.Deserialize(file);
    fclose(file);

    std::cin.get();
}


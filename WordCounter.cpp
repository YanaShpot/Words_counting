#include "WordCounter.h"
#include <iostream>
#include <mutex>
#include <algorithm>
#include <chrono>
#include <atomic>

inline std::chrono::high_resolution_clock::time_point get_current_time_fenced()
{
    std::atomic_thread_fence(std::memory_order_seq_cst);
    auto res_time = std::chrono::high_resolution_clock::now();
    std::atomic_thread_fence(std::memory_order_seq_cst);
    return res_time;
}

template<class D>
inline long long to_us(const D& d)
{
    return std::chrono::duration_cast<std::chrono::microseconds>(d).count();
}



// As per link, English has near 800 000 words, thought it isn't likely that all of them would be at the same file
// https://en.oxforddictionaries.com/explore/how-many-words-are-there-in-the-english-language
const unsigned amountOfWords = 800000u;

// https://en.wikipedia.org/wiki/Longest_word_in_English
const unsigned longestWordSize = strlen("pneumonoultramicroscopicsilicovolcanoconiosis");

WordCounter::WordCounter(string fileName, string outDir,
                         unsigned threadCount) :

        m_threadsCount(threadCount), m_sourceSize(FileSize(fileName)), m_sourcePath(fileName), m_outDir(outDir)
{
    m_threadsWarehouse.resize(m_threadsCount);
    m_timePoints1.resize(m_threadsCount);
    m_timePoints2.resize(m_threadsCount);
}

string WordCounter::SetOutDir(string newOutDir)
{
    string prevDir = m_outDir;
    m_outDir = newOutDir;

    return prevDir;
}

void WordCounter::Run(bool sortByName)
{
    m_threads.clear();
    for (auto warehouse : m_threadsWarehouse)
    {
        warehouse.clear();
    }

    vector<int> parameters(m_threadsCount);
    for (unsigned i = 0; i < m_threadsCount; ++i)
    {
        parameters[i] = i;
        thread* th = new thread(&WordCounter::ChunkedCounting, this, parameters[i]);
        m_threads.push_back(th);

    }

    for (auto& thread : m_threads)
        thread->join();

    m_timePoint3 = get_current_time_fenced();

    if (sortByName)
    {
        MergeByName();
        FlushToFile(true, m_mergedWarehouse);
    }
    else
    {
        MergeByCount();
        FlushToFile(false, m_mergedWarehouseByCount);
    }

    m_timePointFinish = get_current_time_fenced();

    for (auto& thread : m_threads)
        delete thread;
}

void WordCounter::ChunkedCounting(int rank)
{
    m_timePoints1[rank] = get_current_time_fenced();

    const uint64_t chunk = m_sourceSize / m_threadsCount;
    const uint64_t begin = rank * chunk;
    const uint64_t end = rank == m_threadsCount - 1 ?
                         m_sourceSize - 1 : (rank + 1) * chunk - 1;

    // if buff[end] points into the middle of the word, we should
    // move forward to the next whitespace. So, we need to read
    // some additional bytes for that offset
    uint64_t max_size = rank == m_threadsCount - 1 ?
                        end - begin : end - begin + longestWordSize;
    char* buff = new char[max_size + 1];

    ifstream read(m_sourcePath);
    read.seekg(begin, ios::beg);
    read.read(buff, min(max_size + 1, m_sourceSize));

    if (read.fail())
    {
        max_size = read.gcount();
    }

    buff[max_size] = '\0';

    // performing offset at the beginning and at the end of the buffer,
    // to check whether both ends point to whitespaces rather than into
    // the middle of some word
    int begBuff = 0;
    if (rank != 0)
    {
        // first thead's beginning shouldn't pefrorm offset, because it points
        // to the beginning of the first word
        while (_isalnum(buff[begBuff]))
        {
            ++begBuff;
        }
    }

    int endBuff = end - begin;
    if (buff[endBuff] && rank != m_threadsCount - 1)
    {
        while (endBuff < max_size && _isalnum(buff[endBuff]))
        {
            ++endBuff;
        }
    }

    m_timePoints2[rank] = get_current_time_fenced();

    auto& warehouse = m_threadsWarehouse[rank];
    for (unsigned i = begBuff; i < endBuff && buff[i] != '\0'; ++i)
    {
        while (buff[i] != '\0' && !_isalnum(buff[i]))
        {
            ++i;
        }

        if (buff[i] == '\0')
        {
            break;
        }

        char* word = buff + i;
        while (buff[i] != '\0' && _isalnum(buff[i]))
        {
            ++i;
        }

        buff[i] = '\0';
        warehouse[word]++;
    }
    delete[] buff;

}
template <typename T>
void WordCounter::FlushToFile(bool isSortedByName, T& mergedWarehouse)
{
   // string fileName = isSortedByName ? "res_a.txt" : "res_n.txt";
   // ofstream out(m_outDir + fileName, ios::trunc);
    ofstream out(m_outDir, ios::trunc);
    out.width(20);
    if (isSortedByName)
        out << left << "word" << "\t\t\t\t" << "occurance" << endl << endl;
    else
        out << left << "occurance" << "\t\t\t\t" << "word" << endl << endl;

    for (auto word : mergedWarehouse)
    {
        out.width(20);
        out << left << word.first << "\t\t\t\t" << left << word.second << endl;
    }
}

void WordCounter::MergeByName()
{
    m_mergedWarehouse.clear();
    for (auto hashTable : m_threadsWarehouse)
    {
        for (auto word : hashTable)
        {
            m_mergedWarehouse[word.first] += word.second;
        }
    }
}

pair<uint64_t, string> WordCounter::flip_pair(const pair<string, uint64_t> &p)
{
    return pair<uint64_t, string>(p.second, p.first);
}

void WordCounter::MergeByCount()
{
    MergeByName();

    m_mergedWarehouseByCount.clear();
    std::transform(m_mergedWarehouse.begin(), m_mergedWarehouse.end(),
                   std::inserter(m_mergedWarehouseByCount, m_mergedWarehouseByCount.begin()),
                   flip_pair);
}

bool WordCounter::_isalnum(char c)
{
    return 0 <= c && /*c < 256 &&*/ (isalnum(c) || c == '\'');
}

uint64_t WordCounter::FileSize(string filename)
{
    ifstream in(filename, ifstream::ate | ifstream::binary);
    uint64_t size = in.tellg();

    return size;
}

void WordCounter::CountTime()
{

    auto stage1_time = m_timePoints1[0];
    auto stage2_time = m_timePoints2[0];

    for (int i = 1; i < m_threadsCount; ++i)
    {
        stage1_time = m_timePoints1[i] > stage1_time ? m_timePoints1[i] : stage1_time;
        stage2_time = m_timePoints2[i] > stage2_time ? m_timePoints2[i] : stage2_time;

    }
    auto total_time = m_timePointFinish - stage1_time;
    auto read_time = stage2_time - stage1_time;
    auto count_time =  m_timePoint3 - stage2_time;

    cout << "Total time: " << to_us(total_time) << endl;
    cout << "Read time: " << to_us(read_time) << endl;
    cout << "Count time: " << to_us(count_time) << endl;


}
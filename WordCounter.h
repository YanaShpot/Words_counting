#pragma once
#include <fstream>
#include <string>
#include <map>
#include <unordered_map>
#include <thread>
#include <iterator>
#include <vector>

using namespace std;

class WordCounter
{
public:
    WordCounter(string fileName, string outputDir = ".",
                unsigned threadCount = thread::hardware_concurrency());

    string SetOutDir(string newOutDir);
    void Run(bool sortByName = true);
    void CountTime();
private:
    void ChunkedCounting(int rank);
    static uint64_t FileSize(string filename);
    static bool _isalnum(char c);

    void MergeByName();
    void MergeByCount();

    template <typename T>
    void FlushToFile(bool isSortedByName, T& mergedWarehouse);
    static pair<uint64_t, string> flip_pair(const pair<string, uint64_t> &p);
private:
    const unsigned m_threadsCount;
    const uint64_t m_sourceSize;
    vector<unordered_map<string, uint64_t> > m_threadsWarehouse;
    map<string, uint64_t> m_mergedWarehouse;
    multimap<uint64_t, string> m_mergedWarehouseByCount;

    vector<thread*> m_threads;
    string m_outDir;
    string m_sourcePath;

    vector<std::chrono::high_resolution_clock::time_point> m_timePoints1;
    vector<std::chrono::high_resolution_clock::time_point> m_timePoints2;
    std::chrono::high_resolution_clock::time_point m_timePoint3;
    std::chrono::high_resolution_clock::time_point m_timePointFinish;

};

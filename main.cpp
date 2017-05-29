#include "WordCounter.h"
#include <iostream>

using namespace std;


int main(int argc, char* argv[])
{
    string sourceFile , destFile_a, destFile_n;
    int threads;
    ifstream in;
    in.open("/Users/Yasya/Desktop/Results/config.txt");
    if (in.is_open()) {
        getline(in,sourceFile);
        getline(in,destFile_a);
        getline(in,destFile_n);
        in >> threads;
    }
    in.close();
    //const char *sourcePath = "/Users/Yasya/Desktop/Train/The_H_G.txt", *destPath = "/Users/Yasya/Desktop/Train/";
   // int threads = 4;
    bool sortByName = true;

    cout << "Counting is in progress" << endl;

    cout << endl << "Sorting in alphabetical order" << endl;
    WordCounter counter_a(sourceFile, destFile_a, threads);
    counter_a.Run(sortByName);
    counter_a.CountTime();

    sortByName = false;
    cout << endl << "Sorting by the number of occurances" << endl;
    WordCounter counter_n(sourceFile, destFile_n, threads);
    counter_n.Run(sortByName);
    counter_n.CountTime();

    cout << endl << "Counting completed successufully" << endl;

    return 0;
}

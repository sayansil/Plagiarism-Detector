#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>
#include <algorithm>
#include <iterator>
#include <dirent.h>
#include <map>
#include <sstream>

char const * database = "/media/sayan/Data/Programmer/Plagiarism/database";
char const * target_folder = "/media/sayan/Data/Programmer/Plagiarism/target";
char const * stopwords_file = "/media/sayan/Data/Programmer/Plagiarism/stopwords.txt";

int score_accuracy = 1;

float dot_product(std::vector<int> a, std::vector<int> b) {
    float sum = 0 ;
    for(int i=0; i<a.size(); i++)
        sum += a[i] * b[i];
    return sum;
}

float get_multiplier(std::string word) {
    return word.length() * word.length();
}

float cosine_score(std::vector<int> bvector, std::vector<int> tvector) {
    return dot_product(bvector, tvector) / 
            (   sqrt(dot_product(bvector, bvector)) * 
                sqrt(dot_product(tvector, tvector)) );
}

bool endswith (std::string const &fullString, std::string const &ending) {
    if (fullString.length() >= ending.length())
        return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
    else 
        return false;
}

void cleanString(std::string& str) {    
    size_t i = 0;
    size_t len = str.length();
    while(i < len){
        if (!isalnum(str[i]) && str[i] != ' '){
            str.erase(i,1);
            len--;
        }else
            i++;
    }
}

std::string getfile(std::string filepath) {
    std::ifstream mFile(filepath);
    std::string output;
    std::string temp;

    while (mFile >> temp){
        output += std::string(" ") + temp;
    }

    cleanString(output);
    return output;
}

std::map<std::string, int> get_frequency(std::vector<std::string> tokens) {
    std::map<std::string, int> freqs;
    for (auto const & x : tokens)
        ++freqs[x];
    std::vector<std::string> unique_tokens;
    std::vector<int> freq_token;
    for (auto const & p : freqs){
        unique_tokens.push_back(p.first);
        freq_token.push_back(p.second);
    }

    return freqs;
}

std::vector<std::string> string_to_token(std::string str) {
    std::istringstream mstream(str);
    return std::vector<std::string>(std::istream_iterator<std::string>{mstream}, std::istream_iterator<std::string>{});
}

float ngram_score(std::vector<std::string> base, std::vector<std::string> target, int n) {
    std::vector<std::vector<std::string>> bngrams;
    std::vector<std::vector<std::string>> tngrams;
    std::vector<std::string> temp;

    for(int i=0; i<=base.size()-n; i++) {
        temp.clear();
        for(int j=i; j<i+n; j++) 
            temp.push_back(base[j]);
        bngrams.push_back(temp);
    }

    for(int i=0; i<=target.size()-n; i++) {
        temp.clear();
        for(int j=i; j<i+n; j++) 
            temp.push_back(target[j]);
        tngrams.push_back(temp);
    }

    int shared = 0;
    int total = tngrams.size();

    for(auto const & tngram: tngrams)
        for(auto const & bngram: bngrams)
            if(tngram == bngram) {
                shared += 1;
                break;
            }

    return 1.0 * shared / total;
}

float tokenize_test(std::vector<std::string> b_tokens, std::vector<std::string> t_tokens) {
    std::ifstream infile(stopwords_file);
    std::string stopword;
    while (infile >> stopword){
        t_tokens.erase(std::remove(t_tokens.begin(), t_tokens.end(), stopword), t_tokens.end());
    }
    
    auto t_freqs = get_frequency(t_tokens);
    auto b_freqs = get_frequency(b_tokens);
    
    int shared = 0;
    int total = 0;
    
    for(auto const & word : t_freqs) {
        auto search = b_freqs.find(word.first);
        if(search != b_freqs.end()){
            shared += std::min(word.second, search->second) * get_multiplier(word.first);
            total += word.second * get_multiplier(word.first);
        } else {
            total += word.second * get_multiplier(word.first);
        }
    }
    float score = 10.0 * shared / total;

    return score;
}

float ngram_test(std::vector<std::string> b_tokens, std::vector<std::string> t_tokens) {
    float ng3 = ngram_score(b_tokens, t_tokens, 3);
    float ng5 = ngram_score(b_tokens, t_tokens, 5);
    float ng7 = ngram_score(b_tokens, t_tokens, 7);

    float score = 10 * pow((ng7*7 + ng5*5 +ng3*3)/15, 0.4);
    return score;
}

float cosine_test(std::vector<std::string> b_tokens, std::vector<std::string> t_tokens) {
    std::ifstream infile(stopwords_file);
    std::string stopword;
    while (infile >> stopword) {
        t_tokens.erase(std::remove(t_tokens.begin(), t_tokens.end(), stopword), t_tokens.end());
        b_tokens.erase(std::remove(b_tokens.begin(), b_tokens.end(), stopword), b_tokens.end());
    }

    std::vector<std::string> all_tokens;
    all_tokens.reserve( t_tokens.size() + b_tokens.size() );
    all_tokens.insert( all_tokens.end(), t_tokens.begin(), t_tokens.end() );
    all_tokens.insert( all_tokens.end(), b_tokens.begin(), b_tokens.end() );
    sort( all_tokens.begin(), all_tokens.end() );
    all_tokens.erase( unique( all_tokens.begin(), all_tokens.end() ), all_tokens.end() );

    auto t_freqs = get_frequency(t_tokens);
    auto b_freqs = get_frequency(b_tokens);

    std::vector<int> b_vector;
    std::vector<int> t_vector;

    for(auto & token: all_tokens) {
        auto search = b_freqs.find(token);
        if(search != b_freqs.end()) {
            b_vector.push_back(search->second);
        } else {
            b_vector.push_back(0);
        }

        search = t_freqs.find(token);
        if(search != t_freqs.end()) {
            t_vector.push_back(search->second);
        } else {
            t_vector.push_back(0);
        }
    }

    float score = 10.0 * cosine_score(b_vector, t_vector);

    return score;
}

int main() {
    DIR *dir;
    DIR *dirB;
    struct dirent *dir_object;
    
    std::string target_file;
    std::string base_file;

    std::string target;
    std::string base;

    if ((dir = opendir (target_folder)) != NULL) {
        while ((dir_object = readdir (dir)) != NULL)
            if(endswith(std::string(dir_object->d_name), "txt")){
                printf ("\nPlagiarism scores for %s\n", dir_object->d_name);
                target_file = target_folder + std::string("/") + dir_object->d_name;

                target = getfile(target_file);
                float test1 = 0;
                float test2 = 0;
                float test3 = 0;


                if ((dirB = opendir (database)) != NULL) {
                    while ((dir_object = readdir (dirB)) != NULL)
                        if(endswith(std::string(dir_object->d_name), "txt")){
                            base_file = database + std::string("/") + dir_object->d_name;
                            
                            base = getfile(base_file);

                            auto b_tokens = string_to_token(base);
                            auto t_tokens = string_to_token(target);

                            test1 = std::max(test1, tokenize_test(b_tokens, t_tokens));
                            test2 = std::max(test2, ngram_test(b_tokens, t_tokens));
                            test3 = std::max(test3, cosine_test(b_tokens, t_tokens));
                        }
                    closedir (dirB);
                }

                printf("Test 1 score: %.1f/10\n", test1);
                printf("Test 2 score: %.1f/10\n", test2);
                printf("Test 3 score: %.1f/10\n", test3);
            }
                
        closedir (dir);
    }
}
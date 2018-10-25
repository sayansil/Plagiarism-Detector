#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>
#include <algorithm>
#include <iterator>
#include <dirent.h>
#include <map>
#include <sstream>
#include <iomanip>

char const * database = "/media/sayan/Data/Programmer/Plagiarism/database";
char const * target_folder = "/media/sayan/Data/Programmer/Plagiarism/target";
char const * stopwords_file = "/media/sayan/Data/Programmer/Plagiarism/stopwords.txt";

int score_accuracy = 1;
int number_of_tests = 3;

float dot_product(std::vector<int> a, std::vector<int> b) {
    float sum = 0 ;
    for(int i=0; i<a.size(); i++)
        sum += a[i] * b[i];
    return sum;
}

float sum(std::vector<int> v) {
    float sumv = 0;
    for (auto& n : v)
        sumv += n;
    return sumv;
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
    std::vector<int> tests {3, 5, 7};
    std::vector<int> weights {3, 5, 7};

    std::vector<float> ngresults;

    ngresults.push_back(ngram_score(b_tokens, t_tokens, 3));
    ngresults.push_back(ngram_score(b_tokens, t_tokens, 5));
    ngresults.push_back(ngram_score(b_tokens, t_tokens, 7));

    float score = 10 * pow((ngresults[0]*weights[0] + ngresults[1]*weights[1] + ngresults[2]*weights[2])/sum(weights), 0.4);
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

void get_verdict(std::vector<float> t, std::vector<std::string> m) {
    std::vector<int> weights (t.size(), 0);
    
    /**************************
        test1 - tokenize test
        test2 - ngram test
        test3 - cosine test
    ***************************/

    weights[0] = 3;
    weights[1] = 4;
    weights[2] = 3;

    float final_score = (t[0]*weights[0] + t[1]*weights[1] + t[2]*weights[2])/sum(weights);
    std::string verdict;

    if(final_score < 1)
        verdict = "Not plagiarised";
    else if(final_score < 5)
        verdict = "Slightly plagiarised";
    else if(final_score < 8)
        verdict = "Fairly plagiarised";
    else
        verdict = "Highly plagiarised";

    m.erase( remove( m.begin(), m.end(), "" ), m.end() );
    sort( m.begin(), m.end() );
    m.erase( unique( m.begin(), m.end() ), m.end() );

    std::cout<<"********************************************"<<std::endl;
    std::cout<<"\tFinal score: "<<final_score<<std::endl;
    std::cout<<"\tVerdict: "<<verdict<<std::endl;
    if(verdict != "Not plagiarised") {
        std::cout<<"\tMatch found in:"<<std::endl;
        if(m.size() == 0)
            std::cout<<"\t-nil-"<<std::endl;
        for (auto const & file : m)
            std::cout<<"\t\t"<<file<<std::endl;
    }
    
    std::cout<<"********************************************"<<std::endl;

}

int main() {
    DIR *dir;
    DIR *dirB;
    struct dirent *dir_object;
    
    std::string target_file;
    std::string base_file;

    std::string target;
    std::string base;

    float temp;

    if ((dir = opendir (target_folder)) != NULL) {
        while ((dir_object = readdir (dir)) != NULL)
            if(endswith(std::string(dir_object->d_name), ".txt")){
                printf ("\nPlagiarism scores for %s\n", dir_object->d_name);
                target_file = target_folder + std::string("/") + dir_object->d_name;

                target = getfile(target_file);

                std::vector<float> test(number_of_tests, 0.0);
                std::vector<std::string> match(number_of_tests, "");

                if ((dirB = opendir (database)) != NULL) {
                    while ((dir_object = readdir (dirB)) != NULL)
                        if(endswith(std::string(dir_object->d_name), ".txt")){
                            base_file = database + std::string("/") + dir_object->d_name;
                            
                            base = getfile(base_file);

                            auto b_tokens = string_to_token(base);
                            auto t_tokens = string_to_token(target);

                            temp = tokenize_test(b_tokens, t_tokens);
                            if(test[0] < temp) {
                                test[0] = temp;
                                match[0] = dir_object->d_name;
                            }
                            temp = ngram_test(b_tokens, t_tokens);
                            if(test[1] < temp) {
                                test[1] = temp;
                                match[1] = dir_object->d_name;
                            }
                            temp = cosine_test(b_tokens, t_tokens);
                            if(test[2] < temp) {
                                test[2] = temp;
                                match[2] = dir_object->d_name;
                            }

                        }
                    closedir (dirB);
                }

                std::cout<<"Test 1 score: "<<std::fixed<<std::setprecision(score_accuracy)<<test[0]<<"/10"<<std::endl;
                std::cout<<"Test 2 score: "<<std::fixed<<std::setprecision(score_accuracy)<<test[1]<<"/10"<<std::endl;
                std::cout<<"Test 3 score: "<<std::fixed<<std::setprecision(score_accuracy)<<test[2]<<"/10"<<std::endl;

                get_verdict(test, match);

                std::cout<<std::endl;
            }
                
        closedir (dir);
    }
}
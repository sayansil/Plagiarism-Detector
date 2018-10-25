#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtCore>
#include <QtGui>
#include <QMessageBox>
#include <QDesktopServices>
#include <QInputDialog>
#include <vector>
#include <cmath>
#include <fstream>
#include <algorithm>
#include <iterator>
#include <dirent.h>
#include <map>
#include <sstream>
#include <iomanip>

static const int score_accuracy = 1;
static const int number_of_tests = 3;

static const char * stopwords_file = "stop_words.txt";
static const char * github_repo = "https://github.com/sayansil/Plagiarism-Detector";
static const char * linkedin_bio = "https://www.linkedin.com/in/sayansil";

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    std::string database;
    std::ifstream cachefile;
    cachefile.open ("cachefile.txt");
    cachefile >> database;
    cachefile.close();

    ui->lineEdit->setText(QString::fromStdString(database));
}

MainWindow::~MainWindow()
{
    delete ui;
}


double dot_product(std::vector<int> a, std::vector<int> b) {
    double sum = 0 ;
    for(int i=0; i<int(a.size()); i++)
        sum += a[unsigned(i)] * b[unsigned(i)];
    return sum;
}

double sum(std::vector<int> v) {
    double sumv = 0;
    for (auto& n : v)
        sumv += n;
    return sumv;
}

double get_multiplier(std::string word) {
    return word.length() * word.length();
}

std::vector<std::string> string_to_token(std::string str) {
    std::istringstream mstream(str);
    return std::vector<std::string>(std::istream_iterator<std::string>{mstream}, std::istream_iterator<std::string>{});
}

double cosine_score(std::vector<int> bvector, std::vector<int> tvector) {
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

double ngram_score(std::vector<std::string> base, std::vector<std::string> target, int n) {
    std::vector<std::vector<std::string>> bngrams;
    std::vector<std::vector<std::string>> tngrams;
    std::vector<std::string> temp;

    for(int i=0; i<=int(base.size())-n; i++) {
        temp.clear();
        for(int j=i; j<i+n; j++)
            temp.push_back(base[unsigned(j)]);
        bngrams.push_back(temp);
    }

    for(int i=0; i<=int(target.size())-n; i++) {
        temp.clear();
        for(int j=i; j<i+n; j++)
            temp.push_back(target[unsigned(j)]);
        tngrams.push_back(temp);
    }

    int shared = 0;
    int total = int(tngrams.size());

    for(auto const & tngram: tngrams)
        for(auto const & bngram: bngrams)
            if(tngram == bngram) {
                shared += 1;
                break;
            }

    return 1.0 * shared / total;
}

double tokenize_test(std::vector<std::string> b_tokens, std::vector<std::string> t_tokens) {
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
    double score = 10.0 * shared / total;

    return score;
}

double ngram_test(std::vector<std::string> b_tokens, std::vector<std::string> t_tokens) {
    std::vector<int> tests {3, 5, 7};
    std::vector<int> weights {3, 5, 7};

    std::vector<double> ngresults;

    ngresults.push_back(ngram_score(b_tokens, t_tokens, 3));
    ngresults.push_back(ngram_score(b_tokens, t_tokens, 5));
    ngresults.push_back(ngram_score(b_tokens, t_tokens, 7));

    double score = 10 * pow((ngresults[0]*weights[0] + ngresults[1]*weights[1] + ngresults[2]*weights[2])/sum(weights), 0.4);
    return score;
}

double cosine_test(std::vector<std::string> b_tokens, std::vector<std::string> t_tokens) {
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

    double score = 10.0 * cosine_score(b_vector, t_vector);

    return score;
}

double get_verdict(std::vector<double> t, std::vector<int> weights, std::vector<std::string> m) {


    /**************************
        test1 - tokenize test
        test2 - ngram test
        test3 - cosine test
    ***************************/


    double final_score = (t[0]*weights[0] + t[1]*weights[1] + t[2]*weights[2])/sum(weights);
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

    return final_score;
}

void MainWindow::on_pushButton_clicked()
{
    const char * database = (ui->lineEdit->text()).toUtf8().constData();
    std::ofstream cachefile;
    cachefile.open ("cachefile.txt");
    cachefile << database;
    cachefile.close();

    DIR *dirB;
    struct dirent *dir_object;

    std::string target_file;
    std::string base_file;

    std::string target = (ui->textEdit->toPlainText()).toUtf8().constData();
    std::string base;

    cleanString(target);

    std::vector<double> test(number_of_tests, 0.0);
    std::vector<std::string> match(number_of_tests, "");

    double temp;

    if ((dirB = opendir (database)) != NULL) {
        while ((dir_object = readdir (dirB)) != NULL)
            if(endswith(std::string(dir_object->d_name), ".txt")){
                base_file = database + std::string("/") + dir_object->d_name;

                base = getfile(base_file);
                cleanString(base);

                auto b_tokens = string_to_token(base);
                auto t_tokens = string_to_token(target);

                if(t_tokens.size() < 10) {
                    QMessageBox::warning(this, "Input too small", "Enter a minimum of 10 words for proper analysis");
                }

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
    } else {
        ui->label_3->setText("NA");
        QMessageBox::critical(this, "Invalid file path", "Database directory not found!");
        return;
    }

    std::vector<int> weights (test.size(), 0);

    if(!ui->checkBox->checkState() && !ui->checkBox_2->checkState() && !ui->checkBox_3->checkState()) {
        QMessageBox::information(this, "Invalid selection", "No tests selected!");
        return;
    }

    if (ui->checkBox->checkState()) {
        weights[0] = 3;
    }
    if (ui->checkBox_2->checkState()) {
        weights[1] = 4;
    }
    if (ui->checkBox_3->checkState()) {
        weights[2] = 2;
    }

    std::string final_score = std::to_string(get_verdict(test, weights, match));
    if(final_score.find('.') != std::string::npos && final_score.find('.')+score_accuracy < final_score.size()){
        final_score = final_score.substr(0, final_score.find('.')+score_accuracy+1);
    }

    ui->label_3->setText(QString::fromStdString(final_score));
}

void MainWindow::on_actionProject_triggered()
{
    QDesktopServices::openUrl (QUrl(github_repo));
}

void MainWindow::on_actionDeveloper_triggered()
{
    QDesktopServices::openUrl (QUrl(linkedin_bio));

}

void MainWindow::on_actionExit_triggered()
{
    this->close();
}

void MainWindow::on_actionNew_File_triggered()
{
    std::string target_file = QInputDialog::getText(this, "File Path", "Enter the path of text file to be evaluated").toUtf8().constData();
    if(!endswith(target_file, ".txt"))
    {
        QMessageBox::critical(this, "Invalid file type", "Files with \".txt\" extensions are supported");
    } else {
        std::string target = getfile(target_file);
        ui->textEdit->setText(QString::fromStdString(target));
    }
}

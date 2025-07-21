#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <map>
#include <algorithm>
#include <regex>
#include <thread>
#include <chrono>
#include <signal.h>
#include <curl/curl.h>
#include <json/json.h>
#include <sstream>

struct Book {
    std::string title;
    std::string author;
    std::string genre;
    std::string isbn;
};

class BookScanner {
private:
    std::map<std::string, std::vector<Book>> collection;
    std::string outputFile = "book_collection.txt";
    std::string genreFile = "genres.txt";
    std::vector<std::string> knownGenres;
    std::string hfApiUrl = "https://api-inference.huggingface.co/models/facebook/bart-large-mnli";
    std::string hfApiKey = ""; // Hugging Face API Key goes here
    
public:
    BookScanner() {
        loadGenres();
        loadCollection();
    }
    
    void saveAndExit() {
        saveCollection();
        saveGenres();
        std::cout << "\nCollection and genres saved!" << std::endl;
        exit(0);
    }
    
    void loadGenres() {
        std::ifstream file(genreFile);
        std::string line;
        
        if (file.is_open()) {
            while (std::getline(file, line)) {
                if (!line.empty()) {
                    knownGenres.push_back(line);
                }
            }
            file.close();
        } else {
            knownGenres = {
                "Fantasy", "Science Fiction", "Mystery", "Thriller", "Romance",
                "Horror", "Historical Fiction", "Literary Fiction", "Young Adult",
                "Biography", "History", "Science", "Philosophy", "Self-Help",
                "Business", "Art", "Music", "Travel", "Cooking", "Technology",
                "Dark Romance", "Urban Fantasy", "Epic Fantasy", "Cyberpunk", 
                "Steampunk", "Classic Literature"
            };
            saveGenres();
        }
    }
    
    void saveGenres() {
        std::ofstream file(genreFile);
        for (const auto& genre : knownGenres) {
            file << genre << std::endl;
        }
        file.close();
    }
    
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* data) {
        data->append((char*)contents, size * nmemb);
        return size * nmemb;
    }
    
    std::string extractKeySubjects(const Json::Value& subjects) {
        std::vector<std::string> relevantSubjects;
        
        // Look for key genre indicators in subjects
        for (const auto& subject : subjects) {
            std::string subjectName = subject["name"].asString();
            std::transform(subjectName.begin(), subjectName.end(), subjectName.begin(), ::tolower);
            
            // Extract meaningful genre hints
            if (subjectName.find("romance") != std::string::npos ||
                subjectName.find("dark") != std::string::npos ||
                subjectName.find("fantasy") != std::string::npos ||
                subjectName.find("science fiction") != std::string::npos ||
                subjectName.find("mystery") != std::string::npos ||
                subjectName.find("thriller") != std::string::npos ||
                subjectName.find("horror") != std::string::npos ||
                subjectName.find("classic") != std::string::npos ||
                subjectName.find("literature") != std::string::npos ||
                subjectName.find("historical") != std::string::npos ||
                subjectName.find("biography") != std::string::npos ||
                subjectName.find("young adult") != std::string::npos) {
                
                relevantSubjects.push_back(subjectName);
            }
        }
        
        // Combine relevant subjects into context
        std::string context;
        for (size_t i = 0; i < std::min(relevantSubjects.size(), (size_t)3); ++i) {
            if (!context.empty()) context += ", ";
            context += relevantSubjects[i];
        }
        
        return context;
    }
    
    std::string classifyGenreWithAI(const std::string& title, const std::string& author, const std::string& subjects) {
        std::cout << "🤖 Classifying genre with AI..." << std::endl;
        
        CURL* curl = curl_easy_init();
        if (!curl) return "General";
        
        std::string readBuffer;
        
        // Create enhanced input with subject context
        std::string inputText = "The book '" + title + "' by " + author;
        if (!subjects.empty()) {
            inputText += " (subjects: " + subjects + ")";
        }
        
        Json::Value payload;
        payload["inputs"] = inputText;
        payload["parameters"]["candidate_labels"] = Json::Value(Json::arrayValue);
        
        for (const auto& genre : knownGenres) {
            payload["parameters"]["candidate_labels"].append(genre);
        }
        payload["parameters"]["multi_label"] = false;
        
        Json::StreamWriterBuilder builder;
        std::string jsonString = Json::writeString(builder, payload);
        
        curl_easy_setopt(curl, CURLOPT_URL, hfApiUrl.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonString.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);
        
        struct curl_slist* headers = nullptr;
        std::string authHeader = "Authorization: Bearer " + hfApiKey;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, authHeader.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        
        CURLcode res = curl_easy_perform(curl);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        
        if (res != CURLE_OK || readBuffer.empty()) {
            return "General";
        }
        
        return parseAIResponse(readBuffer);
    }
    
    std::string parseAIResponse(const std::string& response) {
        Json::Value root;
        Json::Reader reader;
        
        if (!reader.parse(response, root)) {
            return "General";
        }
        
        if (root.isObject() && root.isMember("labels") && root.isMember("scores")) {
            Json::Value labels = root["labels"];
            Json::Value scores = root["scores"];
            
            if (labels.isArray() && !labels.empty() && scores.isArray()) {
                std::string topGenre = labels[0].asString();
                double topScore = scores[0].asDouble();
                
                // Check for close competitors that might be more specific
                if (labels.size() > 1 && scores.size() > 1) {
                    // FIX: Use a simpler approach to limit the loop size without std::min
                    size_t loopLimit = labels.size();
                    if (loopLimit > 5) loopLimit = 5;
                    
                    for (int i = 1; i < loopLimit; ++i) {
                        std::string altGenre = labels[i].asString();
                        double altScore = scores[i].asDouble();
                        
                        // Prefer more specific genres if they're close
                        if ((topScore - altScore) < 0.25) {
                            if ((topGenre == "Horror" && altGenre == "Dark Romance") ||
                                (topGenre == "Literary Fiction" && altGenre == "Classic Literature") ||
                                (topGenre == "Fiction" && (altGenre.find("Romance") != std::string::npos ||
                                                          altGenre.find("Fantasy") != std::string::npos ||
                                                          altGenre.find("Science Fiction") != std::string::npos))) {
                                std::cout << "📝 Choosing more specific genre: " << altGenre << " over " << topGenre << std::endl;
                                topGenre = altGenre;
                                break;
                            }
                        }
                    }
                }
                
                // Clean up the response
                topGenre = std::regex_replace(topGenre, std::regex("^\\s+|\\s+$"), "");
                
                // Check if this is a new genre
                bool isNewGenre = std::find(knownGenres.begin(), knownGenres.end(), topGenre) == knownGenres.end();
                
                if (isNewGenre && !topGenre.empty() && topGenre != "General") {
                    knownGenres.push_back(topGenre);
                    saveGenres();
                    std::cout << "📝 New genre added: " << topGenre << std::endl;
                }
                
                return topGenre;
            }
        }
        
        // Handle alternative response formats
        if (root.isArray() && !root.empty()) {
            for (const auto& item : root) {
                if (item.isMember("label") && item.isMember("score")) {
                    std::string genre = item["label"].asString();
                    genre = std::regex_replace(genre, std::regex("^\\s+|\\s+$"), "");
                    
                    bool isNewGenre = std::find(knownGenres.begin(), knownGenres.end(), genre) == knownGenres.end();
                    
                    if (isNewGenre && !genre.empty() && genre != "General") {
                        knownGenres.push_back(genre);
                        saveGenres();
                        std::cout << "📝 New genre added: " << genre << std::endl;
                    }
                    
                    return genre;
                }
            }
        }
        
        return "General";
    }
    
    std::string normalizeISBN(const std::string& input) {
        std::string isbn = input;
        isbn.erase(std::remove_if(isbn.begin(), isbn.end(), 
            [](char c) { return !std::isdigit(c) && c != 'X' && c != 'x'; }), 
            isbn.end());
        std::transform(isbn.begin(), isbn.end(), isbn.begin(), ::toupper);
        
        if (isbn.length() == 10 || isbn.length() == 13) {
            return isbn;
        }
        return "";
    }
    
    std::string waitForCompleteISBN() {
        std::string input;
        
        std::cout << "Enter ISBN (scan or type manually, 'quit' to exit): ";
        std::cout.flush();
        
        while (true) {
            std::getline(std::cin, input);
            
            if (input == "quit" || input == "exit") {
                saveAndExit();
            }
            
            std::string normalized = normalizeISBN(input);
            if (!normalized.empty()) {
                return normalized;
            } else {
                std::cout << "Invalid ISBN format. Please try again: ";
            }
        }
    }
    
    Book fetchBookData(const std::string& isbn) {
        Book book;
        book.isbn = isbn;
        
        CURL* curl = curl_easy_init();
        if (!curl) {
            book.title = "Unknown Title";
            book.author = "Unknown Author";
            book.genre = "General";
            return book;
        }
        
        std::string readBuffer;
        std::string url = "http://openlibrary.org/api/books?bibkeys=ISBN:" + isbn + "&format=json&jscmd=data";
        
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        
        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        
        std::string subjectContext;
        if (res == CURLE_OK && !readBuffer.empty()) {
            parseOpenLibraryData(readBuffer, isbn, book, subjectContext);
            
            if (!book.title.empty() && book.title != "Unknown Title") {
                book.genre = classifyGenreWithAI(book.title, book.author, subjectContext);
            }
        }
        
        // Set defaults
        if (book.title.empty()) book.title = "Unknown Title";
        if (book.author.empty()) book.author = "Unknown Author";
        if (book.genre.empty()) book.genre = "General";
        
        return book;
    }
    
    void parseOpenLibraryData(const std::string& jsonData, const std::string& isbn, Book& book, std::string& subjectContext) {
        Json::Value root;
        Json::Reader reader;
        
        if (reader.parse(jsonData, root)) {
            std::string key = "ISBN:" + isbn;
            
            if (root.isMember(key)) {
                Json::Value bookData = root[key];
                
                if (bookData.isMember("title")) {
                    book.title = bookData["title"].asString();
                }
                
                if (bookData.isMember("authors") && bookData["authors"].isArray() && !bookData["authors"].empty()) {
                    book.author = bookData["authors"][0]["name"].asString();
                }
                
                // Extract subject context for better AI classification
                if (bookData.isMember("subjects")) {
                    subjectContext = extractKeySubjects(bookData["subjects"]);
                }
            }
        }
    }
    
    void addBook(const Book& book) {
        collection[book.genre].push_back(book);
        
        std::sort(collection[book.genre].begin(), collection[book.genre].end(),
            [](const Book& a, const Book& b) {
                return a.author < b.author;
            });
    }
    
    void saveCollection() {
        std::ofstream file(outputFile);
        
        if (!file.is_open()) {
            std::cerr << "Error: Could not save collection to file!" << std::endl;
            return;
        }
        
        file << "BOOK COLLECTION - ORGANIZED BY GENRE AND AUTHOR\n";
        file << "=" << std::string(50, '=') << "\n\n";
        
        for (const auto& genrePair : collection) {
            file << genrePair.first << ":\n";
            
            for (const auto& book : genrePair.second) {
                file << "  " << book.author << " - " << book.title << " [" << book.isbn << "]\n";
            }
            file << "\n";
        }
        
        file.close();
    }
    
    void loadCollection() {
        std::ifstream file(outputFile);
        if (file.is_open()) {
            std::cout << "Loaded existing collection from " << outputFile << std::endl;
            file.close();
        }
    }
    
    void run() {
        std::cout << "Book Collection Scanner with AI Genre Classification!" << std::endl;
        std::cout << "Using enhanced context from OpenLibrary for accurate genre detection." << std::endl;
        std::cout << "Use Ctrl+C to save and exit at any time." << std::endl << std::endl;
        
        while (true) {
            std::string isbn = waitForCompleteISBN();
            
            std::cout << "Processing ISBN: " << isbn << "..." << std::endl;
            
            Book book = fetchBookData(isbn);
            
            if (!book.title.empty() && book.title != "Unknown Title") {
                addBook(book);
                std::cout << "✅ SUCCESS: Added \"" << book.title << "\" by " 
                         << book.author << " (Genre: " << book.genre << ")" << std::endl;
            } else {
                std::cout << "❌ FAILED: Could not find book data for ISBN: " << isbn << std::endl;
            }
            
            std::cout << std::endl;
        }
    }
};

BookScanner* globalScanner = nullptr;

void signalHandler(int signal) {
    if (globalScanner) {
        globalScanner->saveAndExit();
    }
}

int main() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    
    BookScanner scanner;
    globalScanner = &scanner;
    
    signal(SIGINT, signalHandler);
    
    scanner.run();
    
    curl_global_cleanup();
    return 0;
}

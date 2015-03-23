#include <curl/curl.h>
#include <iostream>
#include <fstream>
#include <string>
#include <atomic>
#include <thread>
#include <iterator>
#include <mutex>
#include <algorithm>
#include <chrono>
#include <queue>
#include <unordered_set>

#include <boost/regex.hpp>
#include <boost/filesystem/operations.hpp>

typedef std::string URL;

class Timer
{
public:
    Timer(std::string title)
        : title(title)
        , start(std::chrono::high_resolution_clock::now())
    {}

    void restart()
    {
        start = std::chrono::high_resolution_clock::now();
    }

    void stop()
    {
        auto finish = std::chrono::high_resolution_clock::now();
        size_t runningTime = 
            std::chrono::duration_cast<std::chrono::milliseconds>(finish - start).count();
        std::cout << title << ": " << double(runningTime) / 1000 << "s" << std::endl;
    }

private:
    std::string title;
    std::chrono::time_point<std::chrono::high_resolution_clock> start;
};


size_t string_write(void *contents, size_t size, size_t nmemb, void *userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

CURLcode curl_read(const URL& url, std::string& buffer, long timeout = 15) {
        Timer timer("Download: " + url);
    CURLcode code(CURLE_FAILED_INIT);
    CURL* curl = curl_easy_init();

    if(curl) {    
        if(CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_URL, url.c_str()))
        && CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, string_write))
        && CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer))
        && CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L))
        && CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L))
        && CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout))
                && CURLE_OK == (code = curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1))) {

          code = curl_easy_perform(curl);
        }
        curl_easy_cleanup(curl);
    }
        timer.stop();
    return code;
}

template<typename T>
class ConcurrentQueue
{
public:
    void push(const T& t)
    {
        std::lock_guard<std::mutex> lock(mutex);
        queue.push(t);
    }

    T pop()
    {
        std::lock_guard<std::mutex> lock(mutex);
        T value = queue.front();
        queue.pop();
        return value;
    }

    bool tryPop(T& t)
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (queue.empty())
        {
            return false;
        }
        t = queue.front();
        queue.pop();
        return true;
    }

    T front() const
    {
        std::lock_guard<std::mutex> lock(mutex);
        return queue.front();
    }

    bool empty() const
    {
        std::lock_guard<std::mutex> lock(mutex);
        return queue.empty();
    }
    
private:
    mutable std::mutex mutex;
    std::queue<T> queue;
};

template<typename T>
class ConcurrentUnorderedSet
{
public:
    void insert(const T& t)
    {
        std::lock_guard<std::mutex> lock(mutex);
        set.insert(t);
    }

    bool tryInsert(const T& t)
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (set.count(t) > 0)
        {
            return false;
        }
        set.insert(t);
        return true;
    }

    bool contains(const T& t) const
    {
        std::lock_guard<std::mutex> lock(mutex);
        return (set.count(t) > 0);
    }

private:
    mutable std::mutex mutex;
    std::unordered_set<T> set;
};

boost::regex link_regex("<\\s*A\\s+[^>]*href\\s*=\\s*\"(http://|https://)?([^\"]*)\"",
                                boost::regex::normal | boost::regbase::icase);

boost::regex url_regex("(http://|https://)?([^\"]*)",
                                boost::regex::normal | boost::regbase::icase);

boost::regex domain_regex("(http://|https://)?([^\"/]*)",
                                boost::regex::normal | boost::regbase::icase);

boost::regex extension_regex(".(html|php|js)",
                                boost::regex::normal | boost::regbase::icase);

static inline std::string &rTrimChar(std::string &s, char c) {
    s.erase(std::find(s.rbegin(), s.rend(), c).base(), s.end());
    return s;
}

void writeToFile(std::string fileName, const std::string& content)
{
    std::ofstream file(fileName, std::ios::out | std::ios::trunc);
    file << content << std::endl;
}

URL domain(const URL& url)
{
    boost::smatch matches;
    boost::regex_search(url, matches, domain_regex);
    return std::string(matches[0].first, matches[0].second);
}

URL addFileExtension(URL url)
{
    boost::smatch matches;
    if (boost::regex_search(url, matches, extension_regex))
    {
        return url;
    }
    else
    {
        url += ".html";
    }
    return url;
}

std::vector<URL> getUrls(URL rootURL, const std::string& content)
{
    std::vector<URL> urls;

    boost::sregex_iterator it(content.begin(), content.end(), link_regex);
    boost::sregex_iterator end;

    if (rootURL.back() == '/')
    {
        rootURL.pop_back();
    }

    while (it != end)
    {
        auto matches = *it;
        std::string http(matches[1].first, matches[1].second);
        URL url(matches[2].first, matches[2].second);

        if (http.empty())
        {
            if (url.front() == '/')
            {
                if (domain(rootURL).length() == 0)
                {
                    continue;
                }
                url = domain(rootURL) + url;
            }
            else
            {
                URL previousPageURL = rootURL;
                rTrimChar(previousPageURL, '/');
                url = previousPageURL + "/" + url;
            }
        }
        else
        {
            url = http + url;
        }
        urls.push_back(url);
        ++it;
    }

    return urls;
}

class Crawler
{
public:
    Crawler(URL startURL, size_t maxDepth, size_t maxPages, 
                    const std::string& downloadDir, size_t threadsNumber,
                    bool debugOutput = false):
                    startURL(startURL), maxDepth(maxDepth), maxPages(maxPages),
                    downloadDir(downloadDir), threadsNumber(threadsNumber),
                    debugOutput(debugOutput)
    {
        finishedThreads.store(0);
        pagesDownloaded.store(0);
        pagesDownloadingNow.store(0);
        totalSize.store(0);
    }

    void addReadyUrls(std::vector<std::string> readyUrls)
    {

    }

    void addNewUrls(std::vector<std::string> newUrls)
    {

    }

    void start()
    {
        Timer timer("Total time");
        addUrlToQueue(startURL, 0);

        std::vector<std::thread> threads;

        if (debugOutput)
        {
            std::cerr << "threadsNumber: " << threadsNumber << std::endl;
        }
        
        for (std::size_t threadNumber = 0; threadNumber < threadsNumber; 
                    ++threadNumber)
        {
            threads.push_back(std::thread(&Crawler::threadFunction, this));
        }
        for (std::size_t threadNumber = 0; threadNumber < threadsNumber; 
                    ++threadNumber)
        {
            threads[threadNumber].join();
        }

        std::cout << "Total size: " << trunc(double(totalSize) / 1000) / 1000 << 
                                    "mb" << std::endl;

        std::cout << "Pages downloaded: " << pagesDownloaded << std::endl;
        timer.stop();
    }

private:
    
    void threadFunction()
    {
        bool isFinished = false;

        while ((pagesDownloaded.load() < maxPages) && 
                    ((finishedThreads.load() < threadsNumber) ||
                    !urlQueue.empty()))
        {
            std::pair<URL, size_t> urlInfo;
            if (pagesDownloaded.load() + pagesDownloadingNow.load() < maxPages 
                    && urlQueue.tryPop(urlInfo))
            {
                if (isFinished)
                {
                    isFinished = false;
                    finishedThreads -= 1;
                }
                ++pagesDownloadingNow;
                crawl(urlInfo.first, urlInfo.second);
                --pagesDownloadingNow;
            }
            else
            if (!isFinished)
            {
                isFinished = true;
                finishedThreads += 1;
            }
            else
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
        if (debugOutput)
        {
            std::cerr << "Thread: " << std::this_thread::get_id() <<
                         " finished" << std::endl;
        }
    }

    void writePageToFile(URL url, const std::string& content)
    {
        boost::smatch matches;
        if (boost::regex_search(url, matches, url_regex))
        {
            url = std::string(matches[2].first, matches[2].second);
        }
        if (downloadDir.back() == '/')
            downloadDir.pop_back();

        if (url.front() == '/')
            url.erase(url.begin());

        if (url.back() == '/')
            url.pop_back();

        std::string filePath = downloadDir + "/" + addFileExtension(url);

        std::string dirPath = downloadDir + "/" + url;
        rTrimChar(dirPath, '/');
        try
        {
            boost::filesystem::create_directories(dirPath);
        } 
        catch (...) {}

        if (debugOutput)
        {
            std::cerr << "Filename: " << filePath << std::endl;
            std::cerr << "Writing to " << dirPath << std::endl;
        }

        writeToFile(filePath, content);
    }

    void crawl(URL url, size_t depth)
    {
        if (debugOutput)
        {
            std::cerr << "Url " << url << ", depth " << depth << std::endl;
        }
        std::string content;
        CURLcode res = curl_read(url, content);
        if (res == CURLE_OK) 
        {
            ++pagesDownloaded;
            writePageToFile(url, content);

            totalSize += content.size();

            if (depth + 1 <= maxDepth)
            {
                std::vector<URL> urls = getUrls(url, content);
                for (const auto& url : urls)
                {
                    addUrlToQueue(url, depth + 1);
                }
            }
        } 
        else 
        {
            if (debugOutput)
            {
                std::cerr << "ERROR: " << curl_easy_strerror(res) << std::endl;
            }
        }
    }

    bool addUrlToQueue(const URL& url, size_t depth)
    {
        bool inserted = addedToQueuePages.tryInsert(url);
        if (inserted)
        {
            urlQueue.push(std::make_pair(url, depth));
        }
        return inserted;
    }

    URL startURL;
    std::atomic<size_t> totalSize;
    std::atomic<size_t> pagesDownloaded;
    std::atomic<size_t> pagesDownloadingNow;
    std::atomic<size_t> finishedThreads;
    size_t threadsNumber;
    size_t maxDepth, maxPages;
    std::string downloadDir;
    ConcurrentQueue< std::pair<URL, size_t> > urlQueue;
    ConcurrentUnorderedSet<URL> addedToQueuePages;
    bool debugOutput;
};

int main(int argc, const char* argv[]) {
    if (argc < 5) {
            std::printf("Usage: %s start_url max_depth max_pages download_dir \
                                    [debug_output] [threads_number]\n", argv[0]);
            return 1;
    }

    URL startURL = argv[1];
    size_t maxDepth = atoi(argv[2]);
    size_t maxPages = atoi(argv[3]);
    std::string downloadDir = argv[4];

    size_t threadsNumber = std::thread::hardware_concurrency();
    bool debugOutput = false;

    if (argc >= 6)
    {
        debugOutput = atoi(argv[5]);
    }
    if (argc == 7)
    {
        threadsNumber = atoi(argv[6]);
    }

    Crawler crawler(startURL, maxDepth, maxPages, downloadDir, threadsNumber, 
                                    debugOutput);
    crawler.start();
    return 0;
}

#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include <unordered_map>
#include <chrono>
#include <algorithm>
#include <random>
#include <cmath>
#include <climits>
#include <numeric>

static const float EPS = 0.00001;
static unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
static std::default_random_engine generator (seed);

void readData(std::string filename, std::unordered_map<int,std::vector<float> >& dataPoints){
    std::fstream fileStream(filename.c_str(),std::ios::in);
    if (!fileStream.is_open()){
        perror("Could not open file!\n");
        exit (1);
    }
    std::string line;
    int counter = 1;
    while (getline(fileStream,line))
    {
        std::vector<float> pointCoordinates;
        int currentKey = counter++;
        char* chunk = strtok(const_cast<char*>(line.c_str())," \t,");
        while (chunk!=nullptr){
            pointCoordinates.push_back(atof(chunk));
            chunk = strtok(nullptr," \t,");
        }
        dataPoints.insert({currentKey,pointCoordinates});
    }
    fileStream.close();
}

void writeData(std::string filename, const std::vector<std::unordered_map<int,std::vector<float> > >& clusters){

    size_t K = clusters.size();
    size_t maxLength = 0;
    for (size_t index=0;index<K;index++){
        for (const auto& it : clusters[index]){
            maxLength = std::max(maxLength,it.second.size());
        }
    }
    std::fstream fileStream(filename.c_str(),std::ios::out|std::ios::trunc);
    std::string header = "ID,Cluster,";
    for (size_t index=0;index<maxLength;index++){
        header += "coordinate";
        header += std::to_string(index+1);
        if (index!=maxLength-1){
            header+=",";
        }
    }
    header+="\n";
    fileStream.write(header.c_str(),strlen(header.c_str())*sizeof(char));

    for (size_t index=0;index<K;index++){
        for (const auto& it : clusters[index]){
            std::string line = std::to_string(it.first) + "," + std::to_string(index+1) + ",";
            size_t length = it.second.size();
            for (size_t coordinate = 0;coordinate<length;coordinate++){
                line += std::to_string(it.second[coordinate]);
                if (coordinate!= length-1){
                    line += ",";
                }
            }
            line+="\n";
            fileStream.write(line.c_str(),strlen(line.c_str())*sizeof(char));
        }
    }

    fileStream.close();

}

//get L2 distance between 2 points (datapoint and centroid)
float L2distance(std::vector<float> one, std::vector<float> other){
    int length1 = one.size();
    int length2 = other.size();
    if (length1!=length2){
        std::cerr<<"L2distance called with vectors of different lengths!\n";
        return 0;
    }
    for (int index=0;index<length1;index++){
        other[index] -= one[index];
    }
    return sqrt(std::inner_product(other.begin(),other.end(),other.begin(),0.0f));
}

//get the K clusters, corresponding to those K centroids
std::vector<std::unordered_map<int,std::vector<float> > > kMeans(const size_t K, \
                const std::unordered_map<int,std::vector<float> >& normalizedData, \
                const std::unordered_map<int,std::vector<float> >& Data, const std::vector<std::vector<float> >& centroids){

    std::vector<std::unordered_map<int,std::vector<float> > > result(K);
    static std::uniform_int_distribution<int> toss(0,1);

    for (const auto& it : normalizedData){
        std::vector<float> distances(K,0);
        float minVal = INT_MAX;
        int minIndex = 0;
        for (size_t index=0;index<K;index++){
            distances[index] = L2distance(it.second,centroids[index]);
            if (fabs(minVal-distances[index])<EPS){
                if (toss(generator)){
                    minIndex = index;
                }
            }
            else if (minVal - distances[index] > EPS){
                minIndex=index;
                minVal = distances[index];
            }
        }
        int key = it.first;
        result[minIndex].insert({key,Data.at(key)});
    }
    return result;
}

//get the Cohesion from the clusters to measurre improvement (stop criteria)
float getCohesion(const std::vector<std::unordered_map<int,std::vector<float> > >& clusters, \
                  const std::unordered_map<int,std::vector<float>>& normalizedDataPoints, \
                  const std::vector<std::vector<float> >& normalizedCentroids){
    float result = 0;
    const size_t K = clusters.size();
    for (size_t index=0;index<K;index++){
        for (const auto& it : clusters[index]){
            int key = it.first;
            result += pow(L2distance(normalizedCentroids[index],normalizedDataPoints.at(key)),2);
        }
    }
    return result;
}

//the clusters parameter is not normalized
// to get normalized centroids use the keys in clusters to get the normalized values from normalizedData
std::vector<std::vector<float> > getNormalizedCentroids(const std::vector<std::unordered_map<int,std::vector<float> > >& clusters, \
                                              const std::unordered_map<int,std::vector<float>>& normalizedDataPoints){

    size_t maxLength = 0;
    for (const auto& it : normalizedDataPoints){
        maxLength = std::max(maxLength,it.second.size());
    }
    int K = clusters.size();

    std::vector<std::vector<float> > result(K,std::vector<float>(maxLength));
    for (int index=0;index<K;index++){
        std::vector<float> centroid(maxLength,0);
        std::vector<int> count(maxLength,0);
        for (const auto& it : clusters[index]){
            int key = it.first;
            const std::vector<float>& normalized = normalizedDataPoints.at(key);
            size_t length = normalized.size();
            for (size_t innerIndex=0;innerIndex<length;innerIndex++){
                centroid[innerIndex] +=  normalized[innerIndex];
                count[innerIndex]++;
            }
        }
        for (size_t coordinateIndex=0;coordinateIndex<maxLength;coordinateIndex++){
            if (count[coordinateIndex]!=0){
                centroid[coordinateIndex] /= count[coordinateIndex];
            }
        }
        result[index] = centroid;
    }
    return result;
}

//DEBUG
void printData(const std::unordered_map<int,std::vector<float>>& dataPoints){
    for (const auto& v : dataPoints){
        std::cout<<v.first<<"\t";
        for (const float& f : v.second){
            std::cout<<f<<"\t";
        }
        std::cout<<"\n";
    }
}

int main(const int argc, const char** argv)
{
    if (argc<3){
        std::cout<<"Usage: \"kMeans K filename\".\n";
        return 0;
    }

    const size_t K = atoi(*(argv+1));
    const char* filename = *(argv+2);

    if (K<1){
        std::cout<<"K must be >= 1";
        return 0;
    }

    std::unordered_map<int,std::vector<float>> dataPoints;
    readData(filename,dataPoints);

    //get point with most dimentions
    size_t maxLength = 0;
    for (const auto& it : dataPoints){
        maxLength = std::max(maxLength,it.second.size());
    }

    // get mean and sd for each dimention
    std::vector<float> mean(maxLength,0);
    std::vector<int> count(maxLength,0);
    std::vector<float> sd(maxLength,0);
    std::vector<std::pair<float,float>> bounds(maxLength,{0,0});

    for (const auto& it : dataPoints){
        int length = it.second.size();
        for (int index=0;index<length;index++){
            mean[index] += it.second[index];
            count[index]++;
        }
    }
    for (size_t index=0;index<maxLength;index++){
        if (count[index]!=0){
            mean[index] /= count[index];
        }
    }
    for (const auto& it : dataPoints){
        size_t length = it.second.size();
        for (size_t index=0;index<length;index++){
            sd [index] += pow((it.second[index] - mean[index]),2);
        }
    }
    for (size_t index=0;index<maxLength;index++){
        if (count[index]-1!=0){
            sd[index] /= count[index]-1;
        }
        sd[index] = sqrt(sd[index]);
    }

    // get normalized Data and reasonable random initial centroids
    std::unordered_map<int,std::vector<float>> normalizedDataPoints = dataPoints;
    std::vector<std::vector<float> > normalizedCentroids;

    for (auto& it : normalizedDataPoints){
        int length = it.second.size();
        for (int index=0;index<length;index++){
            it.second[index] = (it.second[index] - mean[index]) / sd[index];
            bounds[index].first = std::min(bounds[index].first,it.second[index]);
            bounds[index].second = std::max(bounds[index].first,it.second[index]);
        }
    }
    for (size_t i=0;i<K;i++){
        std::vector<float> centroid(maxLength,0);
        for (size_t index=0;index<maxLength;index++){
            std::uniform_real_distribution<float> rng(bounds[index].first,bounds[index].second);
            centroid[index] = rng(generator);
        }
        normalizedCentroids.push_back(centroid);
    }

    int futileTurns = 0;
    const int LIMIT = 3;
    float previousCohesion = INT_MAX;
    std::vector<std::unordered_map<int,std::vector<float> > > clusters;

    while(futileTurns<LIMIT){
        clusters = kMeans(K,normalizedDataPoints,dataPoints,normalizedCentroids);
        float currentCohesion = getCohesion(clusters,normalizedDataPoints,normalizedCentroids);

        if ( previousCohesion-currentCohesion < EPS ){
            futileTurns++;
        }
        else{
            futileTurns=0;
            previousCohesion = currentCohesion;
        }
        if (futileTurns<LIMIT){
            normalizedCentroids = getNormalizedCentroids(clusters,normalizedDataPoints);
        }
    }

    writeData("output.csv",clusters);

    if (system("python kMeans.py")!=0){
        std::cerr<<"Could not locate kMeans.py to create plot!\nPlease call it manually ('python kMeans.py')\n";
    }

    std::cout<<"Cohesion:"<<previousCohesion<<"\n";
    for (size_t i=1;i<=K;i++){
        std::cout<<"centroid(normalized): "<<i<<std::endl;
        for (auto p : normalizedCentroids[i-1]){
            std::cout<<p<<" ";
        }
        std::cout<<"Assigned points: "<<clusters[i-1].size()<<std::endl;
        std::cout<<std::endl;
    }

    return 0;
}
#include "CSVReader.h"

CSVReader::CSVReader()
{

}

OrderBook CSVReader::readCSV(std::string csvFilename)
{
    
    // OrderBook variable to store data from the CSV File
    // This will return timestamps, products and a map of timestamps to order vectors
    OrderBook ordersData;

    // map of timestamps
    std::map<std::string,bool> timestampsMap;

    // map of products
    std::map<std::string,bool> prodMap;

    std::ifstream csvFile{csvFilename};
    std::string line;
    if (csvFile.is_open())
    {
        while(std::getline(csvFile, line))
        {
            try 
            {
                OrderBookEntry obe = stringsToOBE(tokenise(line, ','));
                ordersData.ordersByTimestamp[obe.timestamp].push_back(obe);

                // add to map of known products
                prodMap[obe.product] = true;
                // add to map of known timestamps
                timestampsMap[obe.timestamp] = true;
            }
            catch(const std::exception& e)
            {
                // std::cout << "CSVReader::readCSV bad data" << std::endl;
            }
        }// end of while
    }

    // flatten the map of timestamps to a vector of strings
    for (auto const& e : timestampsMap)
    {
        ordersData.timestamps.push_back(e.first);
    }

    // flatten the map of products to a vector of strings
    for (auto const& e : prodMap)
    {
        ordersData.products.push_back(e.first);
    }

    // return entries;
    return ordersData;
}

std::vector<std::string> CSVReader::tokenise(std::string csvLine, char separator)
{
    std::vector<std::string> tokens;
    std::string token;
    signed int start,end;
    start = csvLine.find_first_not_of(separator);

    do 
    {
        end = csvLine.find_first_of(separator, start);
        if (start == csvLine.length() || start == end) break;
        if (end >= 0) token = csvLine.substr(start, end - start);
        else token = csvLine.substr(start, csvLine.length() - start);
        tokens.push_back(token);
        start = end + 1;
    } while (end > 0);

    return tokens;
}

OrderBookEntry CSVReader::stringsToOBE(std::vector<std::string> tokens)
{
    double price, amount;
    if (tokens.size() != 5)
    {
        // std::cout << "Bad line" << std::endl;
        throw std::exception{};
    }
    try
    {
        price = std::stod(tokens[3]);
        amount = std::stod(tokens[4]);
    }
    catch(const std::exception& e)
    {
        //std::cout << "CSVReader::stringsToOBE Bad float! " << tokens[3] << std::endl;
        //std::cout << "CSVReader::stringsToOBE Bad float! " << tokens[4] << std::endl;
        throw;
    }
    
    OrderBookEntry obe{price,
                       amount,
                       tokens[0],
                       tokens[1],
                       OrderBookEntry::stringToOrderBookType(tokens[2])};
    return obe;
}

OrderBookEntry CSVReader::stringsToOBE(std::string priceString,
                                       std::string amountString,
                                       std::string timestamp,
                                       std::string product,
                                       OrderBookType orderType)
{
    double price, amount;
    try
    {
        price = std::stod(priceString);
        amount = std::stod(amountString);
    }
    catch (const std::exception& e)
    {
        std::cout << "CSVReader::stringsToOBE Bad float! " << priceString << std::endl;
        std::cout << "CSVReader::stringsToOBE Bad float! " << amountString << std::endl;
        throw; // throw exception to the calling function
    }

    OrderBookEntry obe{price,
                       amount,
                       timestamp,
                       product,
                       orderType};
    return obe;
}
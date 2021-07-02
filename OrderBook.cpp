#include "OrderBook.h"
#include "CSVReader.h"
#include <map> // C++ equivalent of Python dictionary
#include <algorithm>
#include <iostream>
#include <chrono>

/** construct, reading a CSV data file and extracting the data */
OrderBook::OrderBook(std::string filename)
{
    orders = CSVReader::readCSV(filename);
    std::sort(orders.begin(), orders.end(), OrderBookEntry::compareByTimestamp);
    extractTimestampData();
}

/** Function to extract orders data and place it into a map data structure. Also extracts the timestamps in the dataset*/
void OrderBook::extractTimestampData()
{  
    std::map<std::string,bool> timestampsMap;

    for (OrderBookEntry& e : orders)
    {
        timestampsMap[e.timestamp] = true;
        // pushing orders to new data structure
        ordersByTimestamp[e.timestamp].push_back(e);
    }

    // now flatten the map of timestamps to a vector of strings
    for (auto const& e : timestampsMap)
    {
        timestamps.push_back(e.first);
    }
}

/** return vector of all known products in the dataset */
std::vector<std::string> OrderBook::getKnownProducts()
{  
    std::vector<std::string> products;
    std::map<std::string,bool> prodMap;

    for (OrderBookEntry& e : orders)
    {
        prodMap[e.product] = true;
    }

    // now flatten the map of products to a vector of strings
    for (auto const& e : prodMap)
    {
        products.push_back(e.first);
    }

    return products;
}

/** return vector of orders according to the filters applied */
std::vector<OrderBookEntry> OrderBook::getOrders(std::vector<OrderBookEntry>& ordersList,
                                      OrderBookType type, 
                                      std::string product,
                                      std::string timestamp)
{
    std::vector<OrderBookEntry> orders_sub;
    for (OrderBookEntry& e : ordersList)
    {
        if (e.orderType == type &&
            e.product == product &&
            e.timestamp == timestamp &&
            e.orderStatus != "cancelled")
        {
            orders_sub.push_back(e);
        }
    }
    return orders_sub;
}

/** Return the earliest time in the orderbook */
std::string OrderBook::getEarliestTime(std::vector<std::string>& timestamps)
{
    // avoiding erors caused by empty vectors
    if (timestamps.size() == 0)
    {
        std::cout << "OrderBook::getEarliestTime Bad timestamps input supplied!" << std::endl;
        return "";
    }

    std::string earliest_timestamp = timestamps[0];

    for (std::string& value : timestamps)
    {
        if (value < earliest_timestamp) earliest_timestamp = value;
    }

    return earliest_timestamp;
}

/** Return the latest time in the orderbook */
std::string OrderBook::getLatestTime(std::vector<std::string>& timestamps)
{
    // avoiding erors caused by empty vectors
    if (timestamps.size() == 0) 
    {
        std::cout << "OrderBook::getLatestTime Bad timestamps input supplied!" << std::endl;
        return "";
    }

    std::string latest_timestamp = timestamps[0];

    for (std::string& value : timestamps)
    {
        if (value > latest_timestamp) latest_timestamp = value;
    }

    return latest_timestamp;
}

std::string OrderBook::getNextTime(const std::string& timestamp, std::vector<std::string>& timestampsList)
{
    std::string next_timestamp = "";

    for (std::string& value : timestampsList)
    {
        if (value > timestamp)
        {
            next_timestamp = value;
            break;
        }
    }
    if (next_timestamp == "") next_timestamp = getEarliestTime(timestampsList);

    return next_timestamp;
}

/** Insert order to order book */
void OrderBook::insertOrder(OrderBookEntry& order)
{
    ordersByTimestamp[order.timestamp].push_back(order);
}

/** matching engine */
std::vector<OrderBookEntry> OrderBook::matchAsksToBids(std::vector<OrderBookEntry>& currentOrders, 
                                                       std::string product, 
                                                       std::string timestamp)
{
    // starting high resolution clock to measure program running time
    auto start1 = std::chrono::high_resolution_clock::now();

    // extract all asks and bid for a specific product and timestamp
    std::vector<OrderBookEntry> asks = getOrders(currentOrders, OrderBookType::ask, product, timestamp);
    std::vector<OrderBookEntry> bids = getOrders(currentOrders, OrderBookType::bid, product, timestamp);

    std::vector<OrderBookEntry> sales;

    // stopping high resolution clock to measure program running time
    auto stop1 = std::chrono::high_resolution_clock::now();
    // extracting duration and logging to console
    auto duration1 = std::chrono::duration_cast<std::chrono::microseconds>(stop1 - start1);

    // uncomment the line below to output measurements to console
    // std::cout << "Extraction section of the function at " << timestamp << " and product " << product << " ran in " << duration1.count() << " microseconds" << std::endl;

    // starting high resolution clock to measure program running time
    auto start2 = std::chrono::high_resolution_clock::now();

    // sort asks in ascending order by price
    std::sort(asks.begin(), asks.end(), OrderBookEntry::compareByPriceAsc);
    // sort bids in descending order by price
    std::sort(bids.begin(), bids.end(), OrderBookEntry::compareByPriceDesc);

    // stopping high resolution clock to measure program running time
    auto stop2 = std::chrono::high_resolution_clock::now();
    // extracting duration and logging to console
    auto duration2 = std::chrono::duration_cast<std::chrono::microseconds>(stop2 - start2);

    // uncomment the line below to output measurements to console
    // std::cout << "Sorting section of the function at " << timestamp << " and product " << product << " ran in " << duration2.count() << " microseconds" << std::endl;

    // starting high resolution clock to measure program running time
    auto start3 = std::chrono::high_resolution_clock::now();

    for (OrderBookEntry& ask : asks)
    {   
        for (OrderBookEntry& bid : bids)
        {
            // skip bids that have been processed already (matched against asks in previous iterations)
            if (bid.amount == 0)
                continue;

            if (bid.price >= ask.price)
            {
                // creating sale record, username defaulted to dataset
                // storing price difference between bid price and ask price in order to release the amounts from the reserved wallet
                OrderBookEntry sale{ask.price, 0, timestamp, product, OrderBookType::asksale};

                // initializing price difference to the OrderBookEntry for the sale
                sale.priceDifference = bid.price - ask.price;

                // both simusers and botusers can have orders placed
                if (bid.username == "botuser")
                {
                    sale.username = bid.username;
                    sale.orderType = OrderBookType::bidsale;
                }
                if (ask.username == "botuser")
                {
                    sale.username = ask.username;
                    sale.orderType = OrderBookType::asksale;
                }

                if (bid.amount == ask.amount)
                {
                    sale.amount = ask.amount;
                    sales.push_back(sale);
                    // bid has been fully covered so we need to set the bid amount to 0
                    bid.amount = 0;
                    // ask has been fully covered so we need to set the ask amount to 0
                    ask.amount = 0;
                    break;
                }

                if (bid.amount > ask.amount)
                {
                    sale.amount = ask.amount;
                    sales.push_back(sale);
                    // bid has not been fully covered so deducting what was offset (the ask amount)
                    bid.amount = bid.amount - ask.amount;
                    // ask has been fully covered so we need to set the ask amount to 0
                    ask.amount = 0;
                    break;
                }

                if (bid.amount < ask.amount && bid.amount > 0)
                {
                    sale.amount = bid.amount;
                    sales.push_back(sale);
                    // ask has not been fully covered so deducting what was offset (the bid amount)
                    ask.amount = ask.amount - bid.amount;
                    // bid has been fully covered so we need to set the bid amount to 0
                    bid.amount = 0;
                }
            }
        } // end bid for loop

        // if the ask is placed by the bot and has not been fully processed, we add it to the list of active orders
        // this check can be added here as we will not be iterating again over this ask
        if (ask.username == "botuser" && ask.amount > 0)
        {
            activeUserOrders.push_back(ask);
        }       

    } // end ask for loop

    // if the bid is placed by the bot and has not been fully processed, we add it to the list of active orders
    for (OrderBookEntry& bid : bids)
    {
        if (bid.username == "botuser" && bid.amount > 0)
        {
            activeUserOrders.push_back(bid);
        }  
    }

    // stopping high resolution clock to measure program running time
    auto stop3 = std::chrono::high_resolution_clock::now();
    // extracting duration and logging to console
    auto duration3 = std::chrono::duration_cast<std::chrono::microseconds>(stop3 - start3);

    // uncomment the line below to output measurements to console
    // std::cout << "Matching section of the function at " << timestamp << " and product " << product << " ran in " << duration3.count() << " microseconds" << std::endl;

    // uncomment the line below to output measurements to console
    std::cout << "Total running time at " << timestamp << " and product " << product << ": " << duration1.count() + duration2.count() + duration3.count() << " microseconds" << std::endl;

    return sales;
}

/** transfer active user orders from one timestamp to the next */
void OrderBook::transferActiveOrders(std::string prevTimestamp, std::string nextTimestamp)
{
    for (OrderBookEntry& order : activeUserOrders)
    {
        if (order.timestamp == prevTimestamp)
        {
            order.timestamp = nextTimestamp;
            order.orderStatus = "carryover";
            insertOrder(order);
        }
    }
    // empty the list of active user orders
    activeUserOrders.clear();
}


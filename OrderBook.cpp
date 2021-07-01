#include "OrderBook.h"
#include "CSVReader.h"
#include <map> // C++ equivalent of Python dictionary
#include <algorithm>
#include <iostream>
#include <chrono>

/** construct, reading a CSV data file */
OrderBook::OrderBook(std::string filename)
{
    orders = CSVReader::readCSV(filename);
    std::sort(orders.begin(), orders.end(), OrderBookEntry::compareByTimestamp);
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

/** return vector of all known products in the dataset */
std::vector<std::string> OrderBook::getTimestamps()
{  
    std::vector<std::string> timestamps;
    std::map<std::string,bool> timestampsMap;

    for (OrderBookEntry& e : orders)
    {
        timestampsMap[e.timestamp] = true;
        ordersByTimestamp[e.timestamp].push_back(e);
    }

    // now flatten the map of timestamps to a vector of strings
    for (auto const& e : timestampsMap)
    {
        timestamps.push_back(e.first);
    }

    return timestamps;
}
/** return vector of orders according to the filters applied */
// added a vector of order book entries as an input parameter to the function
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

/** function to extract orders for a specific timestamp*/
std::vector<OrderBookEntry> OrderBook::getLiveOrders(std::string& timestamp)
{
    // define vector of orders to store output
    std::vector<OrderBookEntry> orders_sub;

    // iterate through the entire order book
    for (OrderBookEntry& e : orders)
    {   
        // pushing entries for the specific timeframe, excluding cancelled orders
        if (e.timestamp == timestamp && e.orderStatus != "cancelled")
        {
            orders_sub.push_back(e);
        }
    }

    return orders_sub;
}

/** return the price of the highest bid/ask in the sent set */ 
double OrderBook::getHighPrice(std::vector<OrderBookEntry>& orders)
{
    double max = orders[0].price;
    for (OrderBookEntry& e : orders)
    {
        if (e.price > max) max = e.price;
    }
    return max;
}

/** return the price of the lowest bid/ask in the sent set */
double OrderBook::getLowPrice(std::vector<OrderBookEntry>& orders)
{
    double min = orders[0].price;
    for (OrderBookEntry& e : orders)
    {
        if (e.price < min) min = e.price;
    }
    return min;
}

std::string OrderBook::getEarliestTime(std::vector<std::string>& timestamps)
{
    std::string earliest_timestamp = timestamps[0];

    for (std::string& value : timestamps)
    {
        if (value < earliest_timestamp) earliest_timestamp = value;
    }

    return earliest_timestamp;
}

std::string OrderBook::getLatestTime(std::vector<std::string>& timestamps)
{
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

void OrderBook::insertOrder(OrderBookEntry& order)
{
    ordersByTimestamp[order.timestamp].push_back(order);
    // sorting no longer required
    // std::sort(orders.begin(), orders.end(), OrderBookEntry::compareByTimestamp);
}

/** Return the money ratio for a product in the set at a specific time
 * Money ratio = Positive Money Flow / Negative Money Flow
 * https://docs.anychart.com/Stock_Charts/Technical_Indicators/Mathematical_Description
 * normally calculated with all executed transactions in a specific period
 * adapting to the context of the available data */
double OrderBook::getMoneyRatio(const std::string& product,
                                const std::string& timestamp)
{
    // positive Money Flow is the total value of bids for a product (bid price * amount)
    // negative Money Flow is the total value of asks for a product (ask price * amount)
    double positiveMoneyFlow = 0;
    double negativeMoneyFlow = 0;

    // iterating through the orderbook
    for (OrderBookEntry& e : orders)
    {
        // choosing only the entries that meet the filtering conditions supplied in the function parameters
        if (e.timestamp == timestamp && e.product == product)
        {
            // incrementing the money flow variables with the value of bids/asks
            if(e.orderType == OrderBookType::bid) positiveMoneyFlow += e.price * e.amount;
            if(e.orderType == OrderBookType::ask) negativeMoneyFlow += e.price * e.amount;
        }
    }
    // avoiding 0 division
    if (negativeMoneyFlow == 0) return 0;
    // 0 division avoided, applying formula for the money ratio
    return positiveMoneyFlow / negativeMoneyFlow;
}

// added list of orders as a function parameter
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
    std::cout << "Extraction section of the function at " << timestamp << " and product " << product << " ran in " << duration1.count() << " microseconds" << std::endl;

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
    // std::cout << "Total running time at " << timestamp << " and product " << product << ": " << duration1.count() + duration2.count() + duration3.count() << " microseconds" << std::endl;

    return sales;
}

void OrderBook::transferActiveOrders(std::string prevTimestamp, std::string nextTimestamp)
{
    for (OrderBookEntry& order : activeUserOrders)
    {
        if (order.timestamp == prevTimestamp)
        {
            order.timestamp = nextTimestamp;
            order.orderStatus = "carryover";
            
            // avoiding the use of insertOrder, which would sort the full order book after each entry
            ordersByTimestamp[order.timestamp].push_back(order);
        }
    }
    // no need to sort
    // std::sort(orders.begin(), orders.end(), OrderBookEntry::compareByTimestamp);

    // empty the list of active user orders
    activeUserOrders.clear();
}


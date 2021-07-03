#include "OrderBook.h"

/** constructor */
OrderBook::OrderBook()
{

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

            // bids are sorted in a descending order
            // if we have reached a price below the ask price, we can exit the for loop
            if (bid.price < ask.price)
            {
                break;
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
